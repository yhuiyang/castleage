#include <QtNetwork>
#include <QtSql>
#include "CastleAgeNetworkAccessManager.h"


// -----------------------------------------------------------------------------------------------------
// CastleAgeNetworkAccessManager
// -----------------------------------------------------------------------------------------------------
const QUrl CastleAgeNetworkAccessManager::web3_base = QUrl("https://web3.castleagegame.com/castle_ws/");
const QUrl CastleAgeNetworkAccessManager::web3_login = QUrl("https://web3.castleagegame.com/castle_ws/connect_login.php");
const QUrl CastleAgeNetworkAccessManager::web4_login = QUrl("https://web4.castleagegame.com/castle_ws/connect_login.php");
const QUrl CastleAgeNetworkAccessManager::web3_home = QUrl("https://web3.castleagegame.com/castle_ws/index.php");
const QUrl CastleAgeNetworkAccessManager::web4_home = QUrl("https://web4.castleagegame.com/castle_ws/index.php");

CastleAgeNetworkAccessManager::CastleAgeNetworkAccessManager(qlonglong accountId, QObject *parent)
    : QNetworkAccessManager(parent), _cache(nullptr)
{
    switchAccount(accountId);

    /* connecti signals */
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
    connect(this, SIGNAL(encrypted(QNetworkReply*)), this, SLOT(onEncrypted(QNetworkReply*)));
    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));
    connect(this, SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)), this, SLOT(onNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)));
    connect(this, SIGNAL(networkSessionConnected()), this, SLOT(onNetworkSessionConnected()));
    connect(this, SIGNAL(preSharedKeyAuthenticationRequired(QNetworkReply*,QSslPreSharedKeyAuthenticator*)), this, SLOT(onPreSharedKeyAuthenticationRequired(QNetworkReply*,QSslPreSharedKeyAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this, SLOT(onProxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
}

CastleAgeNetworkAccessManager::~CastleAgeNetworkAccessManager()
{

}

QNetworkReply * CastleAgeNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QString http_method;
    switch (op) {
    case QNetworkAccessManager::HeadOperation:
        http_method = "HEAD";
        break;
    case QNetworkAccessManager::GetOperation:
        http_method = "GET";
        break;
    case QNetworkAccessManager::PutOperation:
        http_method = "PUT";
        break;
    case QNetworkAccessManager::PostOperation:
        http_method = "POST";
        break;
    case QNetworkAccessManager::DeleteOperation:
        http_method = "DELETE";
        break;
    case QNetworkAccessManager::CustomOperation:
        http_method = "Custom";
        break;
    default:
        http_method = "UnknownOp";
        break;
    }

    qDebug() << "CastleAgeNetworkAccessManager" << "createRequest" << http_method << request.url();

    QNetworkReply *reply;
    QUrl request_url = request.url();
    if ((request_url == web3_login || request_url == web4_login) && op == QNetworkAccessManager::GetOperation) {
        // post the login form data
        // load account info from database
        QSqlQuery q;
        q.prepare("SELECT email, password FROM accounts WHERE id = :accountId");
        q.bindValue(":accountId", _currentAccountId);
        q.exec();
        if (q.next()) {
            QByteArray email = q.value(0).toByteArray();
            QByteArray password = q.value(1).toByteArray();
            if (!email.isNull() && !email.isEmpty() && !password.isNull() && !password.isEmpty()) {
                // create http post body
                QUrlQuery body;
                body.addQueryItem("platform_action", "CA_web3_login");
                body.addQueryItem("player_email", email);
                body.addQueryItem("player_password", password);
                body.addQueryItem("x", QString::number(10 + rand() % 120));
                body.addQueryItem("y", QString::number(5 + rand() % 50));
                QByteArray post_body = body.toString(QUrl::FullyEncoded).toUtf8();
                qDebug() << "auto login body:" << post_body;

                QBuffer *buffer = new QBuffer();
                buffer->setData(post_body);
                reply = QNetworkAccessManager::createRequest(QNetworkAccessManager::PostOperation, QNetworkRequest(web3_login), buffer);
                buffer->setParent(reply);
            } else {
                qDebug() << "Account email or password is missing, let user input login form manually.";
                reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
            }
        } else {
            qDebug() << "No account info for this accountId, let user input login form manually.";
            reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
        }
    } else if (request_url.host() == "web3.castleagegame.com" || request_url.host() == "web4.castleagegame.com") {
        dumpRequestHeader(request);
        dumpRequestBody(outgoingData, request.header(QNetworkRequest::ContentLengthHeader).toLongLong());
        reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
    } else {
        /* These may be requests for images, pubnub, facebook,...etc, and we are not interested in. */
        reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
    }

    return reply;
}

void CastleAgeNetworkAccessManager::switchAccount(qlonglong accountId)
{
    _currentAccountId = accountId;

    /* reload cookie */
    loadCookie();

    /* setup per account cache directory */
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!cachePath.endsWith(QLatin1Char('/')))
        cachePath += QLatin1Char('/');
    cachePath += QString::number(accountId);
    if (_cache == nullptr) {
        _cache = new QNetworkDiskCache;
        _cache->setCacheDirectory(cachePath);
        _cache->setMaximumCacheSize(10 * 1024 * 1024);
        this->setCache(_cache);
    } else {
        _cache->setCacheDirectory(cachePath);
    }
}

qlonglong CastleAgeNetworkAccessManager::currentAccount() const
{
    return _currentAccountId;
}

void CastleAgeNetworkAccessManager::dumpRequestHeader(const QNetworkRequest &request)
{
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

void CastleAgeNetworkAccessManager::dumpRequestBody(QIODevice *outgoingData, qint64 length)
{
    if (outgoingData != nullptr) {
        if (length <= 0) {
            qDebug() << "Body (at most first 200 bytes)";
            qDebug() << " +-" << outgoingData->peek(200);
        } else {
            qDebug() << "Body";
            qDebug() << " +-" << outgoingData->peek(length);
        }
    }
}

void CastleAgeNetworkAccessManager::storeCookie(QByteArray setCookieHeader) {
    qDebug() << "storeCookie" << setCookieHeader;
    /* store cookie in database */
    if (!setCookieHeader.isEmpty()) {
        QSqlQuery q;
        q.prepare("INSERT OR REPLACE INTO cookies (accountId, cookie) VALUES(:accountId, :cookie)");
        q.bindValue(":accountId", _currentAccountId);
        q.bindValue(":cookie", setCookieHeader);
        q.exec();
    }
}

void CastleAgeNetworkAccessManager::loadCookie() {

    /* delete old cookieJar and create new one. */
    this->cookieJar()->deleteLater();
    this->setCookieJar(new QNetworkCookieJar);

    /* insert cookie. No cookie if database no record. */
    QSqlQuery q;
    q.prepare("SELECT cookie FROM cookies WHERE accountId = :accountId");
    q.bindValue(":accountId", _currentAccountId);
    q.exec();

    if (q.next()) {
        QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(q.value(0).toByteArray());
        if (cookies.size() > 0) {
            for (QNetworkCookie cookie: cookies)
                this->cookieJar()->insertCookie(cookie);
        }
    }
}

//
// Slots
//
void CastleAgeNetworkAccessManager::onAuthenticationRequired(QNetworkReply * reply, QAuthenticator * authenticator)
{
    qDebug() << "CastleAgeNetworkAccessManager" << "authenticationRequired" << reply << authenticator;
}

void CastleAgeNetworkAccessManager::onEncrypted(QNetworkReply * reply)
{
    qDebug() << "CastleAgeNetworkAccessManager" << "encrypted" << reply->request().url();
}

void CastleAgeNetworkAccessManager::onFinished(QNetworkReply * reply)
{
    QString op;
    switch (reply->operation()) {
    case QNetworkAccessManager::HeadOperation:
        op = "HEAD";
        break;
    case QNetworkAccessManager::GetOperation:
        op = "GET";
        break;
    case QNetworkAccessManager::PutOperation:
        op = "PUT";
        break;
    case QNetworkAccessManager::PostOperation:
        op = "POST";
        break;
    case QNetworkAccessManager::DeleteOperation:
        op = "DELETE";
        break;
    case QNetworkAccessManager::CustomOperation:
        op = "Custom";
        break;
    default:
        op = "UnknownOp";
        break;
    }

    int http_status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
    qDebug() << "CastleAgeNetworkAccessManager" << "finished" << op << http_status_code << reply->request().url() << "from cache:" << fromCache;
    QList<QByteArray> headers = reply->rawHeaderList();
    if (headers.size() > 0) {
        qDebug() << "Response Header";
        for (QByteArray header: headers)
            qDebug() << " +-" << header << reply->rawHeader(header);
    }

    if (http_status_code == 302) {
        QString location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
        qDebug() << "Redirect location:" << location;
        QUrl redirect_url = web3_base.resolved(QUrl(location));
        qDebug() << "Redirect url:" << redirect_url;

        if (redirect_url == web3_login || redirect_url == web4_login) {
            qDebug() << "Login is required";
            // TODO: do what?
        } else if (redirect_url == web3_home || redirect_url == web4_home) {
            qDebug() << "This is probably login successfully";
            QVariant cookie_set = reply->header(QNetworkRequest::SetCookieHeader);
            if (cookie_set.isValid())
                this->storeCookie(reply->rawHeader("Set-Cookie"));
            else
                qDebug() << "Response 302 Found with redirect to index.php, but no valid Set-Cookie header";
        } else {
            qDebug() << "Redirect to where else?" << redirect_url;
        }
    }
}

void CastleAgeNetworkAccessManager::onNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)
{
    qDebug() << "CastleAgeNetworkAccessManager" << "accessibleChanged" << accessible;
}

void CastleAgeNetworkAccessManager::onNetworkSessionConnected() {
    qDebug() << "CastleAgeNetworkAccessManager" << "networkSessionConnected";
}

void CastleAgeNetworkAccessManager::onPreSharedKeyAuthenticationRequired(QNetworkReply * reply, QSslPreSharedKeyAuthenticator * authenticator)
{
    qDebug() << "CastleAgeNetworkAccessManager" << "sharedKeyAuthenticationRequired" << reply << authenticator;
}

void CastleAgeNetworkAccessManager::onProxyAuthenticationRequired(const QNetworkProxy & proxy, QAuthenticator * authenticator)
{
    qDebug() << "CastleAgeNetworkAccessManager" << "proxyAuthenticationRequired" << proxy << authenticator;
}

void CastleAgeNetworkAccessManager::onSslErrors(QNetworkReply * reply, const QList<QSslError> & errors)
{
    qDebug() << "CastleAgeNetworkAccessManager" << "sslErrors" << reply << errors;
}
