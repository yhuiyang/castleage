#include <QtNetwork>
#include <QEventLoop>
#include <QtSql>
#include "SynchronizedNetworkAccessManager.h"

const QUrl SynchronizedNetworkAccessManager::_WEB3_BASE = QUrl("https://web3.castleagegame.com/castle_ws/");
const QUrl SynchronizedNetworkAccessManager::_WEB3_LOGIN_URL = QUrl("https://web3.castleagegame.com/castle_ws/connect_login.php");
const QUrl SynchronizedNetworkAccessManager::_WEB4_LOGIN_URL = QUrl("https://web4.castleagegame.com/castle_ws/connect_login.php");
const QUrl SynchronizedNetworkAccessManager::_WEB3_HOME_URL = QUrl("https://web3.castleagegame.com/castle_ws/index.php");
const QUrl SynchronizedNetworkAccessManager::_WEB4_HOME_URL = QUrl("https://web4.castleagegame.com/castle_ws/index.php");

SynchronizedNetworkAccessManager::SynchronizedNetworkAccessManager(qlonglong accountId, QObject *parent)
    : QNetworkAccessManager(parent), _ACCOUNTID(accountId)
{
    /* load cookie */
    QSqlQuery sql;
    sql.prepare("SELECT cookie FROM cookies WHERE accountId = :accountId");
    sql.bindValue(":accountId", accountId);
    if (sql.exec() && sql.next()) {
        QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(sql.value(0).toByteArray());
        if (cookies.size() > 0) {
           for (QNetworkCookie cookie: cookies)
               this->cookieJar()->insertCookie(cookie);
        }
    }
}

SynchronizedNetworkAccessManager::~SynchronizedNetworkAccessManager()
{
}

QByteArray SynchronizedNetworkAccessManager::ca_get(QString php, const QList<QPair<QString, QString>> &qs)
{
    return ca_get(php, qs, true);
}

QByteArray SynchronizedNetworkAccessManager::ca_post(QString php, const QList<QPair<QString, QString>> &form, const QList<QPair<QString, QString>> &qs, bool appendAjax)
{
    return ca_post(php, form, qs, true, appendAjax);
}

bool SynchronizedNetworkAccessManager::ca_login()
{
    bool success = false;

    // load account info from database
    QSqlQuery sql;
    sql.prepare("SELECT email, password FROM accounts WHERE id = :accountId");
    sql.bindValue(":accountId", _ACCOUNTID);
    if (sql.exec() && sql.next()) {
        QByteArray email = sql.value("email").toByteArray();
        QByteArray password = sql.value("password").toByteArray();
        if (!email.isNull() && !email.isEmpty() && !password.isNull() && !password.isEmpty()) {
            // create http post body
            QUrlQuery body;
            body.addQueryItem("platform_action", "CA_web3_login");
            body.addQueryItem("player_email", email);
            body.addQueryItem("player_password", password);
            body.addQueryItem("x", QString::number(10 + rand() % 120));
            body.addQueryItem("y", QString::number(5 + rand() % 50));
            QByteArray post_body = body.toString(QUrl::FullyEncoded).toUtf8();

            QEventLoop looper;
            QNetworkRequest request(_WEB3_LOGIN_URL);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            QNetworkReply *reply = this->post(request, post_body);
            connect(reply, SIGNAL(finished()), &looper, SLOT(quit()));
            looper.exec();

            int http_status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QVariant ca_server_redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            QByteArray ca_server_set_cookie = reply->rawHeader("Set-Cookie");
            if (http_status_code == 302) {
                QUrl redirect_url = _WEB3_BASE.resolved(QUrl(ca_server_redirect.toString()));
                if (redirect_url == _WEB3_HOME_URL || redirect_url == _WEB4_HOME_URL) {
                    /* save cookie to database for later reuse... */
                    QSqlQuery sql;
                    sql.prepare("INSERT OR REPLACE INTO cookies (accountId, cookie) VALUES(:accountId, :cookie)");
                    sql.bindValue(":accountId", _ACCOUNTID);
                    sql.bindValue(":cookie", ca_server_set_cookie);
                    sql.exec();
                    success = true;
                } else {
                    qDebug() << "After login, redirect url is not home page, it is:" << redirect_url;
                }
            } else {
                qWarning() << "Unexpected login response:" << http_status_code << ca_server_set_cookie;
            }
        }
    }
    return success;
}

QByteArray SynchronizedNetworkAccessManager::ca_get(QString php, const QList<QPair<QString, QString>> &qs, bool retryAfterLogin)
{
    QEventLoop looper;
    QNetworkRequest request;

    /* resolve to abs url */
    QUrl requestUrl = _WEB3_BASE.resolved(QUrl(php));

    /* setup query string */
    QUrlQuery uq;
    uq.setQueryItems(qs);
    requestUrl.setQuery(uq);
    request.setUrl(requestUrl);

    dumpRequestHeader(request);

    /* send request and wait */
    QNetworkReply * reply = this->get(request);
    connect(reply, SIGNAL(finished()), &looper, SLOT(quit()));
    looper.exec();

    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status_code == 302) {
        QString location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
        QUrl redirect_url = _WEB3_BASE.resolved(QUrl(location));
        if (redirect_url == _WEB3_LOGIN_URL || redirect_url == _WEB4_LOGIN_URL) {
            qDebug() << "login is required";
            if (ca_login()) {
                if (retryAfterLogin) {
                    qDebug() << "login successfully, resend request...";
                    return ca_get(php, qs, false);
                } else {
                    qDebug() << "login successfully.";
                    return QByteArray();
                }
            } else {
                qDebug() << "login failed!";
                return QByteArray();
            }
        } else {
            qDebug() << "CA Server redirect url to" << redirect_url;
            qDebug() << "Is this expected?";
            return QByteArray();
        }
    } else if (status_code == 200) {
        //qDebug() << "200";
        return reply->readAll();
    } else {
        QByteArray status_phrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray();
        qWarning() << "Unexpected HTTP code" << status_code << status_phrase;
        return reply->readAll(); // FIXME: return what in this case?
    }
}

QByteArray SynchronizedNetworkAccessManager::ca_post(QString php, const QList<QPair<QString, QString>> &form, const QList<QPair<QString, QString>> &qs, bool retryAfterLogin, bool appendAjax)
{
    QEventLoop looper;
    QNetworkRequest request;

    /* resolve to abs url */
    QUrl requestUrl = _WEB3_BASE.resolved(QUrl(php));

    /* setup query string */
    QUrlQuery uq;
    uq.setQueryItems(qs);
    requestUrl.setQuery(uq);
    request.setUrl(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    /* setup body payload */
    QUrlQuery body;
    body.setQueryItems(form);
    if (appendAjax)
        body.addQueryItem("ajax", "1");
    QByteArray payload = body.toString(QUrl::FullyEncoded).toUtf8();

    dumpRequestHeader(request);
    dumpRequestBody(payload);

    /* send request and wait */
    QNetworkReply * reply = this->post(request, payload);
    connect(reply, SIGNAL(finished()), &looper, SLOT(quit()));
    looper.exec();

    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status_code == 302) {
        QString location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
        QUrl redirect_url = _WEB3_BASE.resolved(QUrl(location));
        if (redirect_url == _WEB3_LOGIN_URL || redirect_url == _WEB4_LOGIN_URL) {
            qDebug() << "login is required";
            if (ca_login()) {
                if (retryAfterLogin) {
                    qDebug() << "login successfully, resend request...";
                    return ca_post(php, form, qs, appendAjax, false);
                } else {
                    qDebug() << "login successfully.";
                    return QByteArray();
                }
            } else {
                qDebug() << "login failed!";
                return QByteArray();
            }
        } else {
            qDebug() << "CA Server redirect url to" << redirect_url;
            qDebug() << "Is this expected?";
            return QByteArray();
        }
    } else if (status_code == 200) {
        //qDebug() << "200";
        return reply->readAll();
    } else {
        QByteArray status_phrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray();
        qWarning() << "Unexpected HTTP code" << status_code << status_phrase;
        return reply->readAll(); // FIXME: return what in this case?
    }
}

void SynchronizedNetworkAccessManager::dumpRequestHeader(const QNetworkRequest &request)
{
    return;
    qDebug() << "-----------------------------------------";
    qDebug() << "Network request:";
    qDebug() << "Url" << request.url();
    QNetworkCookieJar *cookieJar = this->cookieJar();
    if (cookieJar != nullptr) {
        QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl(request.url());
        if (cookies.size() > 0)
            qDebug() << "CookieJar";
        for (QNetworkCookie cookie: cookies)
            qDebug() << " +-" << cookie;
    }
    qDebug() << "Headers";
    for (QByteArray &header: request.rawHeaderList())
        qDebug() << " +-" << header << request.rawHeader(header);
}

void SynchronizedNetworkAccessManager::dumpRequestBody(const QByteArray &body)
{
    return;
    qDebug() << "Body";
    qDebug() << " +-" << body;
}
