#include <cstdlib>
#include <QtWidgets>
#include <QtNetwork>
#include <QWebView>
#include <QWebPage>
#include <QtSql>
#include <QVariant>
#include "MdiChildWebView.h"
#include "sqliteopenhelper.h"
#include "CastleAgeNetworkAccessManager.h"


// -----------------------------------------------------------------------------------------------------
// MdiChildWebVeiw
// -----------------------------------------------------------------------------------------------------

MdiChildWebView::MdiChildWebView(QWidget *parent) : QMainWindow(parent)
{
    _view = new QWebView;
    setupToolBarAndStatusBar();

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
    _view->load(QUrl("https://web3.castleagegame.com/castle_ws/index.php"));
    _view->show();

    this->setCentralWidget(_view);
    this->setMinimumWidth(800);

    setAttribute(Qt::WA_DeleteOnClose);
}

void MdiChildWebView::setupToolBarAndStatusBar()
{
    /* navigation tool bar */
    QToolBar *navigationToolBar = this->addToolBar(tr("Navigation ToolBar"));

    // actions
    navigationToolBar->addAction(_view->pageAction(QWebPage::Reload));
    navigationToolBar->addAction(_view->pageAction(QWebPage::Stop));

    // address line edit
    _address = new QLineEdit(this);
    _address->setSizePolicy(QSizePolicy::Expanding, _address->sizePolicy().verticalPolicy());
    connect(_address, SIGNAL(returnPressed()), this, SLOT(onAddressLineReturnPressed()));
    navigationToolBar->addWidget(_address);

    this->addToolBarBreak();

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

    _address->setText(url.toString());
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

void MdiChildWebView::onAddressLineReturnPressed()
{
    // accept only url to https://web3.castleagegame.com/castle_ws/
    bool addressUpdated = false;
    QString addressString = _address->text();
    if (addressString.startsWith("http://")) {
        addressString.replace("http://", "https://");
        addressUpdated = true;
    }
    if (addressString.startsWith("https://web4.")) {
        addressString.replace("https://web4.", "https://web3.");
        addressUpdated = true;
    }
    if (addressString.startsWith("https://web3.castleagegame.com/castle_ws/")) {
        if (addressUpdated)
            _address->setText(addressString);
        _view->load(QUrl(addressString));
    } else {
        qWarning() << "Url is not in white list. Ignore!";
    }
}
