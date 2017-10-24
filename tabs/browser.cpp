#include <QVBoxLayout>
#include <QToolBar>
#include <QtWebEngineWidgets>
#include <QLabel>
#include <QUrl>
#include <QtSql>
#include <QCryptographicHash>
#include "browser.h"

const QUrl WEB3_BASE("https://web3.castleagegame.com/castle_ws/");
const QUrl LOGIN_URL("connect_login.php");
// -----------------------------------------------------------------------------

class Interceptor : public QWebEngineUrlRequestInterceptor {
public:
    Interceptor(QWebEngineView *webView, int accountId, QObject *p = nullptr)
        : QWebEngineUrlRequestInterceptor(p),
          mAccountId(accountId),
          mWebView(webView)
    {

    }

    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        if (info.requestUrl().fileName().endsWith(".php"))
        qDebug() << "****** InterceptRequest ******" << info.requestMethod() << info.requestUrl();
//        dumpNavigationType(info);
//        dumpResourceType(info);

        if (QString::compare("connect_login.php", info.requestUrl().fileName()) == 0
                && QString::compare("GET", info.requestMethod(), Qt::CaseInsensitive) == 0) {
            qDebug() << "Login is required.";

            /* auto sign in by somehow... */
            QSqlQuery sql;
            sql.prepare("SELECT email, password FROM accounts WHERE _id = :accountId");
            sql.bindValue(":accountId", mAccountId);
            if (sql.exec() && sql.first()) {
                QByteArray email = sql.value("email").toByteArray();
                QByteArray password = sql.value("password").toByteArray();
                if (!email.isEmpty() && !password.isEmpty()) {
                    QUrlQuery body;
                    body.addQueryItem("platform_action", "CA_web3_login");
                    body.addQueryItem("player_email", email);
                    body.addQueryItem("player_password", password);
                    body.addQueryItem("x", QString::number(10 + rand() % 120));
                    body.addQueryItem("y", QString::number(5 + rand() % 50));
                    QByteArray post_body = body.toString().toUtf8();
                    qDebug() << post_body;

                    info.block(true);
                    QWebEngineHttpRequest login_request(WEB3_BASE.resolved(LOGIN_URL), QWebEngineHttpRequest::Post);
                    login_request.setPostData(post_body);
                    login_request.setHeader("Content-Type", "application/x-www-form-urlencoded");
                    mWebView->load(login_request);
                }
            }
        }
    }

    void dumpNavigationType(QWebEngineUrlRequestInfo &info) {
        switch (info.navigationType()) {
        case QWebEngineUrlRequestInfo::NavigationTypeLink:
            qDebug() << "Navigation Type: initiated by clicking a link.";
            break;
        case QWebEngineUrlRequestInfo::NavigationTypeTyped:
            qDebug() << "Navigation Type: explicity initated by typing a URL.";
            break;
        case QWebEngineUrlRequestInfo::NavigationTypeFormSubmitted:
            qDebug() << "Navigation Type: submits a form.";
            break;
        case QWebEngineUrlRequestInfo::NavigationTypeBackForward:
            qDebug() << "Navigation Type: initated by a history action.";
            break;
        case QWebEngineUrlRequestInfo::NavigationTypeReload:
            qDebug() << "Navigation Type: initiated by refreshing the page.";
            break;
        case QWebEngineUrlRequestInfo::NavigationTypeOther:
            qDebug() << "Navigation Type: None of the above.";
            break;
        default:
            qDebug() << "Navigation Type: Unknown";
            break;
        }
    }

    void dumpResourceType(QWebEngineUrlRequestInfo &info) {
        switch (info.resourceType()) {
        case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
            qDebug() << "Resource Type: top level page.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
            qDebug() << "Resource Type: frame or iframe.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:
            qDebug() << "Resource Type: a CSS stylesheet.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeScript:
            qDebug() << "Resource Type: an external script.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeImage:
            qDebug() << "Resource Type: an image (jpg/gif/png/etc)";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeFontResource:
            qDebug() << "Resource Type: a font";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeSubResource:
            qDebug() << "Resource Type: an \"other\" subresource.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeObject:
            qDebug() << "Resource Type: an object (or embed) tag for a plugin, or a resource that a plugin requested.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeMedia:
            qDebug() << "Resource Type: a media resource.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeWorker:
            qDebug() << "Resource Type: the main resource of a dedicated worker.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeSharedWorker:
            qDebug() << "Resource Type: the main resource of a shared worker.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypePrefetch:
            qDebug() << "Resource Type: an explicitly requested prefetch";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeFavicon:
            qDebug() << "Resource Type: a favicon";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeXhr:
            qDebug() << "Resource Type: a XMLHttpRequest";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypePing:
            qDebug() << "Resource Type: a ping request for <a ping>";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeServiceWorker:
            qDebug() << "Resource Type: the main resource of a service worker.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeCspReport:
            qDebug() << "Resource Type: Content Security Policy (CSP) violation report";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypePluginResource:
            qDebug() << "Resource Type: A resource requested by a plugin.";
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeUnknown:
            qDebug() << "Resource Type: Unknown.";
        default:
            qDebug() << "Resource Type: ??";
            break;
        }
    }

private:
    QWebEngineView *mWebView;
    int mAccountId;
};

static void setupCustomWebEnginePage(QWebEngineView *webEngineView, const int accountId) {
    QString email;
    QSqlQuery sql;
    sql.prepare("SELECT email FROM accounts WHERE _id = :accountId");
    sql.bindValue(":accountId", accountId);
    if (sql.exec() && sql.first())
        email = sql.value(0).toString();
    sql.finish();

    QWebEnginePage *page;
    if (email.isEmpty()) {
        page = new QWebEnginePage(QWebEngineProfile::defaultProfile(), webEngineView);
    } else {
        QCryptographicHash crypto(QCryptographicHash::Sha256);
        crypto.addData(email.toUtf8());
        QWebEngineProfile *profile = new QWebEngineProfile(crypto.result().toHex());
        profile->setProperty("", "");
        page = new QWebEnginePage(profile, webEngineView);
        profile->setParent(page);
        profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
        profile->setRequestInterceptor(new Interceptor(webEngineView, accountId, page));
        webEngineView->setPage(page);
    }
    webEngineView->setPage(page);

    QWebEngineHttpRequest request(WEB3_BASE.resolved(QUrl("index.php")), QWebEngineHttpRequest::Get);

    webEngineView->load(request);
}

// -------------------------------------------------------------------------------------
Browser::Browser(QWidget *parent) : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);

    mProgressBar = new QProgressBar(this);
    mProgressBar->setMaximumHeight(1);
    mProgressBar->setTextVisible(false);
    mProgressBar->setStyleSheet(QStringLiteral("QProgressBar {border: 0px } QProgressBar::chunk { background-color: red; }"));
    layout->addWidget(mProgressBar);
    mWebView = new QWebEngineView(this);
    layout->addWidget(mWebView);

    connect(mWebView, &QWebEngineView::loadProgress, [&](int progress) {
        mProgressBar->setValue(progress < 100 ? progress : 0);
    });
    connect(mWebView, &QWebEngineView::loadFinished, [&] {
        mProgressBar->setValue(0);
    });

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    setupToolBar();
    populateFilter();

    //mWebView->page()->profile()->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    //mWebView->page()->profile()->setRequestInterceptor(new Interceptor(this));
    /* cookie store */
    //QWebEngineCookieStore *store = mWebView->page()->profile()->cookieStore();
    //connect(store, &QWebEngineCookieStore::cookieAdded, [](const QNetworkCookie &cookie){
    //    qDebug() << "Cookie added: " << cookie.toRawForm();
    //});
    //connect(store, &QWebEngineCookieStore::cookieRemoved, [](const QNetworkCookie &cookie){
    //    qDebug() << "Cookie removed: " << cookie.name();
    //});
    //store->loadAllCookies();

    //mWebView->load(QUrl("https://web3.castleagegame.com/castle_ws/index.php"));
    //QWebEngineHttpRequest request = QWebEngineHttpRequest(WEB3_BASE.resolved(QUrl("index.php")), /* QUrl("https://web3.castleagegame.com/castle_ws/index.php"),*/ QWebEngineHttpRequest::Get);
    //if (!request.hasHeader("Cookie")) {
        //request.setHeader("Cookie", "CA_46755028429=804f7a4ef5192d2493119b8755cc867ec5a0644558cecd30b4e05e6c846d75637edc11344129aee92728d9adbab2be0854c33091f51ce4991188aa08f96ccac9%3A116900752006833%3A1506337753");
    //}
    //mWebView->load(request);
    //mWebView->load(QUrl("https://www.google.com.tw"));
    //mWebView->show();
}

Browser::~Browser()
{
    delete centralWidget();
}

void Browser::setupToolBar()
{
    QToolBar *navigationToolBar = addToolBar(tr("Navigation"));
    navigationToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    navigationToolBar->toggleViewAction()->setEnabled(false);

    navigationToolBar->addAction(mWebView->pageAction(QWebEnginePage::Reload));
    navigationToolBar->addAction(mWebView->pageAction(QWebEnginePage::Stop));

    mLineEdit = new QLineEdit(this);
    navigationToolBar->addWidget(mLineEdit);

    connect(mWebView, &QWebEngineView::urlChanged, [&](const QUrl &url){
        mLineEdit->setText(url.toDisplayString());
    });
    connect(mLineEdit, &QLineEdit::returnPressed, [&] () {
        mWebView->load(QUrl(mLineEdit->text()));
    });


    //navigationBar->addSeparator();
    addToolBarBreak();

    QToolBar *accountToolBar = addToolBar("Account");
    accountToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    accountToolBar->toggleViewAction()->setEnabled(false);

    accountToolBar->addWidget(new QLabel("Account", accountToolBar));
    accountToolBar->addWidget(mAccountList = new QComboBox);
    accountToolBar->addWidget(new QLabel("Filter", accountToolBar));
    accountToolBar->addWidget(mFilterList = new QComboBox);
    accountToolBar->addWidget(mLockComboBox = new QCheckBox("Lock Account & Filter"));

    //connect(mFilterList, &QComboBox::currentIndexChanged, this, &Browser::onFilterIndexChanged);  // this fails due to there is other overload function of currentIndexChanged, need to cast
    //connect(mFilterList, SIGNAL(currentIndexChanged(int)), this, SLOT(onFilterIndexChanged(int)));  // old connect way (string base) works
    connect(mFilterList, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Browser::onFilterIndexChanged);
    connect(mAccountList, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Browser::onAccountIndexChanged);
}

void Browser::populateFilter()
{
    QSqlQueryModel *filterModel = new QSqlQueryModel(this);
    filterModel->setQuery(
                "SELECT 'No filter', 0 AS _id "
                "UNION "
                "SELECT 'Tag: ' || name, _id FROM tags "
                "UNION "
                "SELECT 'Guild: ' || name, _id FROM guilds "
                "ORDER BY _id ASC"
                );
    mFilterList->setModel(filterModel);
}

void Browser::onFilterIndexChanged(int filterIndex)
{
    //qDebug() << "filter index changes to" << filterIndex << ", update account ComboBox.";

    /* index -> filter id */
    QVariant filter = mFilterList->model()->data(mFilterList->model()->index(filterIndex, 1));

    /* update account list */
    QSqlQueryModel *accountModel = new QSqlQueryModel;
    QString sql;
    switch (filter.type()) {
    case QMetaType::QString:
        sql.append("SELECT '------ Select Account ------' AS account, 0 AS _id, 0 AS sequence ");
        sql.append("UNION ");
        sql.append("SELECT IFNULL(i.ign, 'UnknownIGN') || ' - ' || a.email AS account, a._id, a.sequence FROM account_guild_mappings AS m ");
        sql.append("INNER JOIN accounts AS a ON a._id = m.accountId ");
        sql.append("LEFT JOIN igns AS i ON i.accountId = m.accountId ");
        sql.append("WHERE m.guildId = '").append(filter.toString()).append("' ");
        sql.append("ORDER BY sequence");
        accountModel->setQuery(sql);
        break;
    case QMetaType::LongLong:
        if (filter.toInt() == 0) {
            sql.append("SELECT '------ Select Account ------' AS account, 0 AS _id, 0 AS sequence ");
            sql.append("UNION ");
            sql.append("SELECT IFNULL(i.ign, 'Unknown IGN') || ' - ' || email AS account, _id, sequence ");
            sql.append("FROM accounts AS a ");
            sql.append("LEFT JOIN igns AS i ON i.accountId = a._id ");
            sql.append("ORDER BY a.sequence"); // Note 'ORDER BY' executes _AFTER_ 'UNION', so first query must also contains sequence column.
            accountModel->setQuery(sql);
        } else {
            sql.append("SELECT '------ Select Account ------' AS account, 0 AS _id, 0 AS sequence ");
            sql.append("UNION ");
            sql.append("SELECT IFNULL(i.ign, 'UnknownIGN') || ' - ' || a.email AS account, a._id, m.sequence FROM account_tag_mappings AS m ");
            sql.append("INNER JOIN accounts AS a ON a._id = m.accountId ");
            sql.append("LEFT JOIN igns AS i ON i.accountId = m.accountId ");
            sql.append("WHERE m.tagId = %1 ");
            sql.append("ORDER BY m.sequence");
            accountModel->setQuery(sql.arg(filter.toInt()));
        }
        break;
    default:
        qDebug() << "Unsupported filter!";
        break;
    }

    mAccountList->setModel(accountModel);
}

void Browser::onAccountIndexChanged(int accountIndex)
{
    //qDebug() << "account index changes to" << accountIndex;

    /* index -> account id */
    int accountId = mAccountList->model()->data(mAccountList->model()->index(accountIndex, 1)).toInt();
    setupCustomWebEnginePage(mWebView, accountId);

    //qDebug() << "accountId =" << accountId;
}
