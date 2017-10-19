#include <QDebug>
#include <QtNetwork>
#include <QtSql>
#include <QApplication>
#include <zlib.h>
#include "castleagehttpclient.h"

const QUrl CastleAgeHttpClient::URL_BASE = QUrl("https://web3.castleagegame.com/castle_ws/");

CastleAgeHttpClient::CastleAgeHttpClient(const int accountId, QObject *parent)
    : QNetworkAccessManager(parent)
{
    switchAccount(accountId);

    connect(this, &QNetworkAccessManager::proxyAuthenticationRequired, this, &CastleAgeHttpClient::onProxyAuthenticationRequired);
    connect(this, &QNetworkAccessManager::authenticationRequired, this, &CastleAgeHttpClient::onAuthenticationRequired);
    //connect(this, &QNetworkAccessManager::finished, this, &CastleAgeHttpClient::onFinished);
    //connect(this, &QNetworkAccessManager::encrypted, this, &CastleAgeHttpClient::onEncrypted);
    connect(this, &QNetworkAccessManager::sslErrors, this, &CastleAgeHttpClient::onSslErrors);
    connect(this, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, &CastleAgeHttpClient::onPreSharedKeyAuthenticationRequired);
    //connect(this, &QNetworkAccessManager::networkSessionConnected, this, &CastleAgeHttpClient::onNetworkSessionConnected);
    connect(this, &QNetworkAccessManager::networkAccessibleChanged, this, &CastleAgeHttpClient::onNetworkAccessibleChanged);
}

CastleAgeHttpClient::~CastleAgeHttpClient()
{

}

void CastleAgeHttpClient::switchAccount(const int accountId)
{
    mAccountId = accountId;

    /* recreate cookieJar */
    this->cookieJar()->deleteLater();
    this->setCookieJar(new QNetworkCookieJar(this));

    /* load cookie for active account to this new created cookieJar */
    QSqlQuery q;
    q.prepare("SELECT cookie FROM cookies WHERE accountId = :accountId");
    q.bindValue(":accountId", accountId);
    if (q.exec() && q.first()) {
        QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(q.value(0).toByteArray());
        for (QNetworkCookie cookie : cookies) {
            if (QString::compare(cookie.domain(), "web3.castleagegame.com") == 0)
                this->cookieJar()->insertCookie(cookie);
            else
                qDebug() << "Ignore cookie for not interested domain:" << cookie.domain();
        }
    } else
        qDebug() << "No cookies found for specific account.";
}

QByteArray CastleAgeHttpClient::get_sync(const QString &php, const QVector<QPair<QString, QString>> &qs)
{
    /* prepare the destination url path */
    QUrl path = URL_BASE.resolved(QUrl(php));
    if (qs.size() > 0) {
        QUrlQuery q;
        for (QPair<QString, QString> d : qs)
            q.addQueryItem(d.first, d.second);
        path.setQuery(q);
    }

    /* setup request header: enable gzip compression */
    QNetworkRequest request(path);
    request.setRawHeader("Accept-Encoding", "gzip, deflate");
    dumpHeader(request);

    /* create local event loop to wait asyncronized network response */
    QEventLoop looper;
    for (int retry = 0; retry < 2; retry++) {
        qint64 t = QDateTime::currentMSecsSinceEpoch();
        QNetworkReply *reply = this->get(request);
        connect(reply, &QNetworkReply::finished, &looper, &QEventLoop::quit, Qt::AutoConnection);
        looper.exec();
        qDebug() << "GET" << path.toString() << "spends" << (QDateTime::currentMSecsSinceEpoch() - t) << "ms";
        reply->deleteLater();

        dumpHeader(reply);
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            if (reply->hasRawHeader("Content-Encoding") && !QString::compare(reply->rawHeader("Content-Encoding"), "gzip"))
                return ungzip(reply->readAll());
            else
                return reply->readAll();
        } else if (status_code == 302) {
            QString location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
            qDebug() << "302 Found: Redirect location" << location;
            if (QString::compare("connect_login.php", location) == 0) {
                qDebug() << "executing login procedure...";
                if (!execute_login()) {
                    qWarning() << "Login failed!";
                    return "";
                }
            }
        }
    }

    return "";
}

QByteArray CastleAgeHttpClient::post_sync(const QString &php, const QVector<QPair<QString, QString>> &form, const QVector<QPair<QString, QString>> &qs)
{
    /* prepare the destination url path */
    QUrl path = URL_BASE.resolved(QUrl(php));
    if (qs.size() > 0) {
        QUrlQuery q;
        for (QPair<QString, QString> d : qs)
            q.addQueryItem(d.first, d.second);
        path.setQuery(q);
    }

    /* setup request header: enable gzip compression */
    QNetworkRequest request(path);
    request.setRawHeader("Accept-Encoding", "gzip, deflate");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    dumpHeader(request);

    /* prepare request body */
    QUrlQuery payload;
    payload.addQueryItem("ajax", "1");
    for (QPair<QString, QString> d : form)
        payload.addQueryItem(d.first, d.second);
    QByteArray body = payload.toString(QUrl::FullyEncoded).toUtf8();

    /* create local event loop to wait asyncronized network response */
    QEventLoop looper;
    for (int retry = 0; retry < 2; retry++) {
        qint64 t = QDateTime::currentMSecsSinceEpoch();
        QNetworkReply *reply = this->post(request, body);
        connect(reply, &QNetworkReply::finished, &looper, &QEventLoop::quit, Qt::AutoConnection);
        looper.exec();
        qDebug() << QString("POST[%1]").arg(mAccountId) << path.toString() << body  << "time spends" << (QDateTime::currentMSecsSinceEpoch() - t) << "ms";
        reply->deleteLater();

        dumpHeader(reply);
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status_code == 200) {
            if (reply->hasRawHeader("Content-Encoding") && !QString::compare(reply->rawHeader("Content-Encoding"), "gzip"))
                return ungzip(reply->readAll());
            else
                return reply->readAll();
        } else if (status_code == 302) {
            QString location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
            qDebug() << "302 Found: Redirect location" << location;
            if (QString::compare("connect_login.php", location) == 0) {
                qDebug() << "executing login procedure...";
                if (!execute_login()) {
                    qWarning() << "Login failed!";
                    return "";
                }
            } else if (location.endsWith("connect_login.php")) {
                qDebug() << "*** CA server try to redirect us to web4, however we insist on try login on web3... ***";
                if (!execute_login()) {
                    qWarning() << "Login failed!";
                    return "";
                }
            }
        }
    }

    return "";
}

bool CastleAgeHttpClient::execute_login()
{
    bool ok = false;

    QString email;
    QString password;
    QSqlQuery q;
    q.prepare("SELECT email, password FROM accounts WHERE _id = :accountId");
    q.bindValue(":accountId", mAccountId);
    if (q.exec() && q.first()) {
        email = q.value(0).toString();
        password = q.value(1).toString();
    }

    if (!email.isEmpty() && !password.isEmpty()) {
        // create http post body
        QUrlQuery body;
        body.addQueryItem("platform_action", "CA_web3_login");
        body.addQueryItem("player_email", email);
        body.addQueryItem("player_password", password);
        body.addQueryItem("x", QString::number(10 + rand() % 120));
        body.addQueryItem("y", QString::number(5 + rand() % 50));
        QByteArray post_body = body.toString(QUrl::FullyEncoded).toUtf8();

        QEventLoop looper;

        // send login credential
        QNetworkRequest request(URL_BASE.resolved(QUrl("connect_login.php")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QNetworkReply *reply = this->post(request, post_body);
        connect(reply, &QNetworkReply::finished, &looper, &QEventLoop::quit);
        looper.exec();
        reply->deleteLater();

        // check response
        int http_status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QVariant ca_server_redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        QByteArray ca_server_set_cookie = reply->rawHeader("Set-Cookie");
        if (http_status_code == 302) {
            QUrl redirect_url = URL_BASE.resolved(QUrl(ca_server_redirect.toString()));
            if (QString::compare(redirect_url.fileName(), "index.php") == 0) {
                /* save cookie to database for later reuse... */
                QSqlQuery sql;
                sql.prepare("INSERT OR REPLACE INTO cookies (accountId, cookie) VALUES(:accountId, :cookie)");
                sql.bindValue(":accountId", mAccountId);
                sql.bindValue(":cookie", ca_server_set_cookie);
                sql.exec();
                ok = true;
            } else {
                qDebug() << "After login, redirect url is not home page, it is:" << redirect_url;
            }
        } else {
            qWarning() << "Unexpected login response:" << http_status_code << ca_server_set_cookie;
        }
    }

    return ok;
}

/* Implementation is copied from http://stackoverflow.com/a/7351507/1078792 */
QByteArray CastleAgeHttpClient::ungzip(const QByteArray &gzippedData)
{
    if (gzippedData.size() <= 4) {
        qWarning() << "ungzip: Input data is truncated";
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = gzippedData.size();
    strm.next_in = (Bytef*)(gzippedData.data());

    ret = inflateInit2(&strm, 15 +  32); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR); // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR; // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

void CastleAgeHttpClient::dumpHeader(const QNetworkRequest &request)
{
#if 1
    Q_UNUSED(request);
#else
    qDebug() << ">>>>>> Request headers >>>>>>";
    for (QByteArray header : request.rawHeaderList())
        qDebug() << header << "=>" << request.rawHeader(header);
    qDebug() << ">>>>>> End of request headers >>>>>>";
#endif
}

void CastleAgeHttpClient::dumpHeader(const QNetworkReply *reply)
{
#if 1
    Q_UNUSED(reply);
#else
    qDebug() << "<<<<<< Response headers <<<<<<";
    for (QByteArray header : reply->rawHeaderList())
        qDebug() << header << "=>" << reply->rawHeader(header);
    qDebug() << "<<<<<< End of response headers <<<<<<";
#endif
}

QNetworkReply *CastleAgeHttpClient::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
//    if (op == QNetworkAccessManager::GetOperation && QString::compare("connect_login.php", request.url().fileName()) == 0) {
//        qDebug() << "Login is required, try auto sign in...";

//        QSqlQuery q;
//        q.prepare("SELECT email, password FROM accounts WHERE _id = :accountId");
//        q.bindValue(":accountId", mAccountId);
//        if (q.exec() && q.first()) {
//            QByteArray email = q.value(0).toByteArray();
//            QByteArray password = q.value(1).toByteArray();
//            if (!email.isEmpty() && !password.isEmpty()) {

//            } else {
//                qWarning() << "Email and/or password is emptied, bypass request!";
//                return QNetworkAccessManager::createRequest(op, request, outgoingData);
//            }
//        } else {
//            qWarning() << "No account credentials, bypass request!";
//            return QNetworkAccessManager::createRequest(op, request, outgoingData);
//        }
//    } else {
//        qDebug() << "not interested quest, bypass it...";
//    }

    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

void CastleAgeHttpClient::onProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    Q_UNUSED(proxy);
    Q_UNUSED(authenticator);
    qDebug() << __func__;
}

void CastleAgeHttpClient::onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    Q_UNUSED(authenticator);
    qDebug() << __func__;
}

void CastleAgeHttpClient::onFinished(QNetworkReply *reply)
{
    qDebug() << __func__;

    int response_status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << reply->request().url() << response_status_code;
    QByteArray response = reply->readAll();
    qDebug() << response;
}

void CastleAgeHttpClient::onEncrypted(QNetworkReply *reply)
{
    Q_UNUSED(reply);
    qDebug() << __func__;
}

void CastleAgeHttpClient::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(reply);
    Q_UNUSED(errors);
    qDebug() << __func__;
}

void CastleAgeHttpClient::onPreSharedKeyAuthenticationRequired(QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    Q_UNUSED(authenticator);
    qDebug() << __func__;
}

void CastleAgeHttpClient::onNetworkSessionConnected()
{
    qDebug() << __func__;
}

void CastleAgeHttpClient::onNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)
{
    Q_UNUSED(accessible);
    qDebug() << __func__;
}
