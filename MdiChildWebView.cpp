#include <cstdlib>
#include <QtWidgets>
#include <QWebView>
#include <QWebPage>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QtSql>
#include <QVariant>
//#include <QSqlQuery>
//#include <QSqlDatabase>
#include "MdiChildWebView.h"
#include "sqliteopenhelper.h"

// -----------------------------------------------------------------------------------------------------
// CastleAgeNetworkAccessManager
// -----------------------------------------------------------------------------------------------------
const QUrl CastleAgeNetworkAccessManager::web3_base = QUrl("https://web3.castleagegame.com/castle_ws/");
const QUrl CastleAgeNetworkAccessManager::web3_login = QUrl("https://web3.castleagegame.com/castle_ws/connect_login.php");
const QUrl CastleAgeNetworkAccessManager::web4_login = QUrl("https://web4.castleagegame.com/castle_ws/connect_login.php");
const QUrl CastleAgeNetworkAccessManager::web3_home = QUrl("https://web3.castleagegame.com/castle_ws/index.php");
const QUrl CastleAgeNetworkAccessManager::web4_home = QUrl("https://web4.castleagegame.com/castle_ws/index.php");

CastleAgeNetworkAccessManager::CastleAgeNetworkAccessManager(qlonglong accountId, QObject *parent)
    : QNetworkAccessManager(parent)
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
    loadCookie();
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
    qDebug() << "CastleAgeNetworkAccessManager" << "finished" << op << http_status_code << reply->request().url();

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

// -----------------------------------------------------------------------------------------------------
// MdiChildWebVeiw
// -----------------------------------------------------------------------------------------------------

MdiChildWebView::MdiChildWebView(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setupToolBarAndStatusBar();

    _view = new QWebView;

    /* connect signals from QWebView */
    connect(_view, SIGNAL(iconChanged(void)), this, SLOT(onWebViewIconChanged(void)));
    connect(_view, SIGNAL(linkClicked(QUrl)), this, SLOT(onWebViewLinkClicked(QUrl)));
    connect(_view, SIGNAL(loadFinished(bool)), this, SLOT(onWebViewLoadFinished(bool)));
    connect(_view, SIGNAL(loadProgress(int)), this, SLOT(onWebViewLoadProgress(int)));
    connect(_view, SIGNAL(loadStarted(void)), this, SLOT(onWebViewLoadStarted(void)));
    //connect(_view, SIGNAL(selectionChanged()), this, SLOT(onWebViewSelectionChanged()));
    connect(_view, SIGNAL(statusBarMessage(QString)), this, SLOT(onWebViewStatusBarMessage(QString)));
    connect(_view, SIGNAL(titleChanged(QString)), this, SLOT(onWebViewTitleChanged(QString)));
    connect(_view, SIGNAL(urlChanged(QUrl)), this, SLOT(onWebViewUrlChanged(QUrl)));

    /* connect signals from QWebPage */
    QWebPage *page = _view->page();
    connect(page, SIGNAL(applicationCacheQuotaExceeded(QWebSecurityOrigin*,quint64,quint64)), this, SLOT(onWebPageApplicationCacheQuotaExceeded(QWebSecurityOrigin*,quint64,quint64)));
    //connect(page, SIGNAL(contentsChanged()), this, SLOT(onWebPageContentsChanged()));
    connect(page, SIGNAL(databaseQuotaExceeded(QWebFrame*,QString)), this, SLOT(onWebPageDatabaseQuotaExceeded(QWebFrame*,QString)));
    connect(page, SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(onWebPageDownloadRequested(QNetworkRequest)));
    connect(page, SIGNAL(featurePermissionRequestCanceled(QWebFrame*,QWebPage::Feature)), this, SLOT(onWebPageFeaturePermissionRequestCanceled(QWebFrame*,QWebPage::Feature)));
    connect(page, SIGNAL(featurePermissionRequested(QWebFrame*,QWebPage::Feature)), this, SLOT(onWebPageFeaturePermissionRequested(QWebFrame*,QWebPage::Feature)));
    connect(page, SIGNAL(frameCreated(QWebFrame*)), this, SLOT(onWebPageFrameCreated(QWebFrame*)));
    connect(page, SIGNAL(geometryChangeRequested(QRect)), this, SLOT(onWebPageGeometryChangeRequested(QRect)));
    connect(page, SIGNAL(linkClicked(QUrl)), this, SLOT(onWebPageLinkClicked(QUrl)));
    //connect(page, SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(onWebPageLinkHovered(QString,QString,QString)));
    connect(page, SIGNAL(loadFinished(bool)), this, SLOT(onWebPageLoadFinished(bool)));
    connect(page, SIGNAL(loadProgress(int)), this, SLOT(onWebPageLoadProgress(int)));
    connect(page, SIGNAL(loadStarted()), this, SLOT(onWebPageLoadStarted()));
    connect(page, SIGNAL(menuBarVisibilityChangeRequested(bool)), this, SLOT(onWebPageMenuBarVisibilityChangeRequested(bool)));
    //connect(page, SIGNAL(microFocusChanged()), this, SLOT(onWebPageMicroFocusChanged()));
    connect(page, SIGNAL(printRequested(QWebFrame*)), this, SLOT(onWebPagePrintRequested(QWebFrame*)));
    //connect(page, SIGNAL(repaintRequested(QRect)), this, SLOT(onWebPageRepaintRequested(QRect)));
    connect(page, SIGNAL(restoreFrameStateRequested(QWebFrame*)), this, SLOT(onWebPageFrameCreated(QWebFrame*)));
    connect(page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)), this, SLOT(onWebPageSaveFrameStateRequested(QWebFrame*,QWebHistoryItem*)));
    //connect(page, SIGNAL(scrollRequested(int,int,QRect)), this, SLOT(onWebPageScrollRequested(int,int,QRect)));
    //connect(page, SIGNAL(selectionChanged()), this, SLOT(onWebPageSelectionChanged()));
    connect(page, SIGNAL(statusBarMessage(QString)), this, SLOT(onWebPageStatusBarMessage(QString)));
    connect(page, SIGNAL(statusBarVisibilityChangeRequested(bool)), this, SLOT(onWebPageStatusBarVisibilityChangeRequested(bool)));
    connect(page, SIGNAL(toolBarVisibilityChangeRequested(bool)), this, SLOT(onWebPageToolBarVisibilityChangeRequested(bool)));
    connect(page, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(onWebPageUnsupportedContent(QNetworkReply*)));
    connect(page, SIGNAL(viewportChangeRequested()), this, SLOT(onWebPageViewportChangeRequested()));
    connect(page, SIGNAL(windowCloseRequested()), this, SLOT(onWebPageWindowCloseRequested()));

    /* load accountId from account combobox */
    qlonglong accountId = getCurrentAccountIdFromComboBox();

    /* CastlAgeNetworkAccessManager */
    _netMgr = new CastleAgeNetworkAccessManager(accountId, this);
    page->setNetworkAccessManager(_netMgr);

    /* setup QWebView and initial url */
    //_view->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    //_view->settings()->enablePersistentStorage(QDir::tempPath());
    _view->load(QUrl("https://web3.castleagegame.com/castle_ws/index.php"));
    _view->show();

    this->setCentralWidget(_view);
    this->setMinimumWidth(800);
}

void MdiChildWebView::setupToolBarAndStatusBar()
{
    /* address tool bar */
    QToolBar *addressToolBar = this->addToolBar(tr("Address ToolBar"));

    /* address line edit */
    addressToolBar->addWidget(new QLabel(tr("Address: "), this));
    addressToolBar->addWidget(_address = new QLineEdit(this));

    this->addToolBarBreak(Qt::TopToolBarArea);

    /* account tool bar */
    QToolBar *accountToolBar = this->addToolBar(tr("Account ToolBar"));
    accountToolBar->addWidget(new QLabel(tr("Account: "), this));
    _accountComboBox = new QComboBox(this);
    QSqlQueryModel *model = new QSqlQueryModel;
    model->setQuery("SELECT tbl_a.id, ifnull(ign, email), email, ifnull(ign, 'Unknown') || ' - ' || email FROM accounts AS tbl_a LEFT JOIN igns AS tbl_i ON tbl_i.accountId = tbl_a.id ORDER BY timestamp");
    _accountComboBox->setModel(model);
    _accountComboBox->setModelColumn(3); // 0: accountId, 1: ign, 2: email, 3: ign & email
    accountToolBar->addWidget(_accountComboBox);

    connect(_accountComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onAccountComboBoxIndexChanged(int)));

    /* status bar */
    this->setStatusBar(new QStatusBar);
}

qlonglong MdiChildWebView::getCurrentAccountIdFromComboBox()
{
    int row = _accountComboBox->currentIndex();
    int col = 0; // column 0: accountId, 1: ign, 2: email, 3: ign & email
    QModelIndex idx = _accountComboBox->model()->index(row, col);
    return _accountComboBox->model()->data(idx).toLongLong();
}

//
// Slots
//
void MdiChildWebView::onWebViewIconChanged()
{
    qDebug() << "QWebView iconChanged";
}

void MdiChildWebView::onWebViewLinkClicked(const QUrl &url)
{
    qDebug() << "QWebView linkClicked:" << url;
}

void MdiChildWebView::onWebViewLoadFinished(bool ok)
{
    qDebug() << "QWebView loadFinished. ok:" << ok;
}

void MdiChildWebView::onWebViewLoadProgress(int progress)
{
    qDebug() << "QWebView loadProgress:" << progress;
}

void MdiChildWebView::onWebViewLoadStarted()
{
    qDebug() << "QWebView loadStarted";
}

void MdiChildWebView::onWebViewSelectionChanged()
{
    qDebug() << "QWebView selectionChanged";
}

void MdiChildWebView::onWebViewStatusBarMessage(const QString &text)
{
    qDebug() << "QWebView statusBarMessage:" << text;
}

void MdiChildWebView::onWebViewTitleChanged(const QString &title)
{
    qDebug() << "QWebView titleChanged:" << title;
}

void MdiChildWebView::onWebViewUrlChanged(const QUrl &url)
{
    qDebug() << "QWebView urlChanged:" << url;
}

void MdiChildWebView::onWebPageApplicationCacheQuotaExceeded(QWebSecurityOrigin * origin, quint64 defaultOriginQuota, quint64 totalSpaceNeeded)
{
    qDebug() << "QWebPage applicationCacheQuotaExceeded" << origin << defaultOriginQuota << totalSpaceNeeded;
}

void MdiChildWebView::onWebPageContentsChanged()
{
    qDebug() << "QWebPage contentsChanged";
}

void MdiChildWebView::onWebPageDatabaseQuotaExceeded(QWebFrame * frame, QString databaseName)
{
    qDebug() << "QWebPage databaseQuotaExceeded" << frame << databaseName;
}

void MdiChildWebView::onWebPageDownloadRequested(const QNetworkRequest & request)
{
    qDebug() << "QWebPage downloadRequested" << request.url();
}

void MdiChildWebView::onWebPageFeaturePermissionRequestCanceled(QWebFrame * frame, QWebPage::Feature feature)
{
    qDebug() << "QWebPage featurePermissionRequestCanceled" << frame << feature;
}

void MdiChildWebView::onWebPageFeaturePermissionRequested(QWebFrame * frame, QWebPage::Feature feature)
{
    qDebug() << "QWebPage featurePermissionRequested" << frame << feature;
}

void MdiChildWebView::onWebPageFrameCreated(QWebFrame * frame)
{
    qDebug() << "QWebPage frameCreated" << frame;
}

void MdiChildWebView::onWebPageGeometryChangeRequested(const QRect & geometry)
{
    qDebug() << "QWebPage geometryChangeRequested" << geometry;
}

void MdiChildWebView::onWebPageLinkClicked(const QUrl & url)
{
    qDebug() << "QWebPage linkClicked" << url;
}

void MdiChildWebView::onWebPageLinkHovered(const QString & link, const QString & title, const QString & textContent)
{
    qDebug() << "QWebPage linkHovered" << link << title << textContent;
}

void MdiChildWebView::onWebPageLoadFinished(bool ok)
{
    qDebug() << "QWebPage loadFinished" << ok;
}

void MdiChildWebView::onWebPageLoadProgress(int progress)
{
    qDebug() << "QWebPage loadProgress" << progress;
}

void MdiChildWebView::onWebPageLoadStarted()
{
    qDebug() << "QWebPage loadStarted";
}

void MdiChildWebView::onWebPageMenuBarVisibilityChangeRequested(bool visible)
{
    qDebug() << "QWebPage menuBarVisibilityChangeRequested" << visible;
}

void MdiChildWebView::onWebPageMicroFocusChanged()
{
    qDebug() << "QWebPage microFocusChanged";
}

void MdiChildWebView::onWebPagePrintRequested(QWebFrame * frame)
{
    qDebug() << "QWebPage printRequested" << frame;
}

void MdiChildWebView::onWebPageRepaintRequested(const QRect & dirtyRect)
{
    qDebug() << "QWebPage repaintRequested" << dirtyRect;
}

void MdiChildWebView::onWebPageRestoreFrameStateRequested(QWebFrame * frame)
{
    qDebug() << "QWebPage restoreFrameStateRequested" << frame;
}

void MdiChildWebView::onWebPageSaveFrameStateRequested(QWebFrame * frame, QWebHistoryItem * item)
{
    qDebug() << "QWebPage saveFrameStateRequested" << frame << item;
}

void MdiChildWebView::onWebPageScrollRequested(int dx, int dy, const QRect & rectToScroll)
{
    qDebug() << "QWebPage scrollRequested" << dx << dy << rectToScroll;
}

void MdiChildWebView::onWebPageSelectionChanged()
{
    qDebug() << "QWebPage selectionChanged";
}

void MdiChildWebView::onWebPageStatusBarMessage(const QString & text)
{
    qDebug() << "QWebPage statusBarMessage" << text;
}

void MdiChildWebView::onWebPageStatusBarVisibilityChangeRequested(bool visible)
{
    qDebug() << "QWebPage statusBarVisibilityChangeRequested" << visible;
}

void MdiChildWebView::onWebPageToolBarVisibilityChangeRequested(bool visible)
{
    qDebug() << "QWebPage toolBarVisibilityChangeRequested" << visible;
}

void MdiChildWebView::onWebPageUnsupportedContent(QNetworkReply * reply)
{
    qDebug() << "QWebPage unsupportedContent" << reply;
}

void MdiChildWebView::onWebPageViewportChangeRequested()
{
    qDebug() << "QWebPage viewportChangeRequested";
}

void MdiChildWebView::onWebPageWindowCloseRequested()
{
    qDebug() << "QWebPage windowCloseRequested";
}

void MdiChildWebView::onAccountComboBoxIndexChanged(int row)
{
    int col = 0; // column 0: accountId, 1: ign, 2: email, 3: ign & email
    QModelIndex idx = _accountComboBox->model()->index(row, col);
    qlonglong accountId = _accountComboBox->model()->data(idx).toLongLong();
    qDebug() << "AccountComboBox index changed" << row << accountId;
    this->_netMgr->switchAccount(accountId);
    this->_view->reload();
}
