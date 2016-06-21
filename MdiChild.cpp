#include <cstdlib>
#include <QtWidgets>
#include <QtNetwork>
#include <QWebView>
#include <QWebPage>
#include <QtSql>
#include <QVariant>
#include "MdiChild.h"
#include "sqliteopenhelper.h"
#include "CastleAgeNetworkAccessManager.h"


// -----------------------------------------------------------------------------------------------------
// WebView: tab inside every mdi child.
// -----------------------------------------------------------------------------------------------------
TabWebView::TabWebView(QWidget *parent) : QMainWindow(parent)
{
    _webView = new QWebView(this);
    _netMgr = new CastleAgeNetworkAccessManager(0, this);
    QWebPage *page = _webView->page();
    page->setNetworkAccessManager(_netMgr);

    setupActionToolBar();
    populateToolBar();

    connect(_webView, SIGNAL(loadStarted()), this, SLOT(onWebViewLoadStarted()));
    connect(_webView, SIGNAL(loadProgress(int)), this, SLOT(onWebViewLoadProgress(int)));
    connect(_webView, SIGNAL(loadFinished(bool)), this, SLOT(onWebViewLoadFinished(bool)));
    connect(_webView, SIGNAL(urlChanged(QUrl)), this, SLOT(onWebViewUrlChanged(QUrl)));
    connect(page, SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(onWebPageLinkHovered(QString,QString,QString)));
    connect(_netMgr, SIGNAL(ca_login_done(qlonglong,bool)), this, SLOT(onCastleAgeLoginResult(qlonglong,bool)));

    _webView->load(QUrl("https://web3.castleagegame.com/castle_ws/index.php"));
    _webView->show();

    this->setCentralWidget(_webView);
}

QUrl TabWebView::loadUrl(const QString &url)
{
    // accept only url to https://web3.castleagegame.com/castle_ws/
    QUrl targetUrl(url.trimmed());
    qDebug() << "load url" << url;
    if (targetUrl.scheme() == "http") {
        qDebug() << "http -> https";
        targetUrl.setScheme("https");
    }
    if (targetUrl.host() == "web4.castleagegame.com") {
        qDebug() << "web4 -> web3";
        targetUrl.setHost("web3.castleagegame.com");
    }
    if (targetUrl.scheme() != "https" || targetUrl.host() != "web3.castleagegame.com") {
        qDebug() << "invalid url -> index.php";
        targetUrl.setUrl("https://web3.castleagegame.com/castle_ws/index.php");
    }

    _webView->load(targetUrl);

    return targetUrl;
}

QUrl TabWebView::getCurrentUrl() const
{
    return _webView->url();
}

void TabWebView::setupActionToolBar()
{
    QToolBar *toolbarNavigation = addToolBar("Navigation");
    toolbarNavigation->addAction(_webView->pageAction(QWebPage::Reload));
    toolbarNavigation->addAction(_webView->pageAction(QWebPage::Stop));
    toolbarNavigation->addWidget(_address = new QLineEdit("", toolbarNavigation));

    addToolBarBreak();

    QToolBar *toolbarAccount = addToolBar("Account");
    toolbarAccount->addWidget(new QLabel("Account:", toolbarAccount));
    toolbarAccount->addWidget(_comboBoxAccount = new QComboBox(toolbarAccount));

    connect(_address, &QLineEdit::returnPressed, this, &TabWebView::onAddressReturnKeyPressed);
    connect(_comboBoxAccount, SIGNAL(currentIndexChanged(int)), this, SLOT(onAccountIndexChanged(int)));
}

void TabWebView::populateToolBar()
{
    _comboBoxAccount->clear();
    _comboBoxAccount->addItem("------ Select Account ------", 0);

    QSqlQuery sql;
    if (sql.exec("SELECT a.id, ifnull(ign, 'Unknown') || ' - ' || email FROM accounts AS a LEFT JOIN igns AS i ON i.accountId = a.id ORDER BY timestamp")) {
        while (sql.next())
            _comboBoxAccount->addItem(sql.value(1).toString(), sql.value(0).toLongLong());
    }
}

void TabWebView::onAddressReturnKeyPressed()
{
    QUrl url = loadUrl(_address->text());
    _address->setText(url.toString());
}

void TabWebView::onAccountIndexChanged(int index)
{
    qlonglong accountId = _comboBoxAccount->itemData(index).toLongLong();
    this->_netMgr->switchAccount(accountId);
    if (this->_webView->url().fileName() != "connect_login.php")
        this->_webView->reload();
    else
        this->_webView->load(QUrl("https://web3.castleagegame.com/castle_ws/index.php"));

    emit requestUpdateTabInfo(this, accountId);
}

void TabWebView::onWebViewLoadStarted()
{

}

void TabWebView::onWebViewLoadProgress(int progress)
{

}

void TabWebView::onWebViewLoadFinished(bool ok)
{

}

void TabWebView::onWebViewUrlChanged(const QUrl &url)
{
    if (_address != nullptr)
        _address->setText(url.toString());
}

void TabWebView::onWebPageLinkHovered(const QString &link, const QString &title, const QString &textContent)
{
    Q_UNUSED(title);
    Q_UNUSED(textContent);
    emit requestShowStatusBarMessage(link);
}

void TabWebView::onCastleAgeLoginResult(qlonglong accountId, bool successful)
{

}

// -----------------------------------------------------------------------------------------------------
// MdiChild: mdi child or main window.
// -----------------------------------------------------------------------------------------------------

MdiChild::MdiChild(QWidget *parent) : QMainWindow(parent)
{
    setStatusBar(new QStatusBar);

    QPushButton *btnAddTab = new QPushButton("+");
    _tabWidget = new QTabWidget(this);
    _tabWidget->setDocumentMode(true);
    _tabWidget->setTabsClosable(false);
    _tabWidget->setMovable(true);
    _tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    _tabWidget->setCornerWidget(btnAddTab);
    this->setCentralWidget(_tabWidget);
    this->setMinimumWidth(800);

    connect(_tabWidget->tabBar(), &QTabBar::customContextMenuRequested, this, &MdiChild::onTabRequestShowContextMenu);
    connect(btnAddTab, &QPushButton::clicked, this, &MdiChild::addTabWebView);

    addTabWebView();

    setAttribute(Qt::WA_DeleteOnClose);
}

int MdiChild::addTabWebView()
{
    QString tabText = "NO ACCOUNT SELECTED";
    TabWebView *tabWebView = new TabWebView(this);
    int tabIndex = _tabWidget->addTab(tabWebView, tabText);
    _tabWidget->setCurrentIndex(tabIndex);

    connect(tabWebView, &TabWebView::requestUpdateTabInfo, this, &MdiChild::onTabRequestUpdateTabInfo);
    connect(tabWebView, &TabWebView::requestShowStatusBarMessage, this, &MdiChild::onTabRequestShowStatusBarMessage);

    return tabIndex;
}

//
// Slots
//
void MdiChild::onTabRequestUpdateTabInfo(TabWebView *const tabPage, qlonglong accountId)
{
    int tabIndex = _tabWidget->indexOf(tabPage);

    if (tabIndex != -1) {
        QSqlQuery sql;
        sql.prepare("SELECT email, ifnull(ign, 'Unknown IGN') FROM accounts LEFT JOIN igns ON accounts.id = igns.accountId WHERE accounts.id = :accountId");
        sql.bindValue(":accountId", accountId);
        if (sql.exec() && sql.next()) {
            _tabWidget->setTabText(tabIndex, sql.value(1).toString());
            _tabWidget->setTabToolTip(tabIndex, sql.value(0).toString());
        }
    }
}

void MdiChild::onTabRequestShowStatusBarMessage(const QString &message)
{
    statusBar()->showMessage(message);
}

void MdiChild::onTabRequestShowContextMenu(const QPoint &point)
{
    int clickedTabIndex = -1;
    int tabCount = _tabWidget->tabBar()->count();
    for (int tabIndex = 0; tabIndex < tabCount; tabIndex++) {
        QRect rect = _tabWidget->tabBar()->tabRect(tabIndex);
        if (rect.contains(point)) {
            clickedTabIndex = tabIndex;
            break;
        }
    }

    /* not clicked on tab */
    if (clickedTabIndex == -1)
        return;

    /* populate context menu */
    QMenu contextMenu;
    QAction *closeSelf = contextMenu.addAction("Close this tab");
    closeSelf->setEnabled(tabCount > 1);
    QAction *closeOthers = contextMenu.addAction("Close other tabs");
    closeOthers->setEnabled(tabCount > 1);
    contextMenu.addSeparator();
    QAction *loadUrlInOthers = contextMenu.addAction("Load current url in other tabs");
    loadUrlInOthers->setEnabled(tabCount > 1);

    /* show it in synchronized mode */
    QAction *selectedAction = contextMenu.exec(QCursor::pos());
    if (selectedAction == 0) {
        return;
    } else if (selectedAction == closeSelf) {
        _tabWidget->removeTab(clickedTabIndex);
    } else if (selectedAction == closeOthers) {
        for (int tabIndex = tabCount - 1; tabIndex > clickedTabIndex; tabIndex--)
            _tabWidget->removeTab(tabIndex);
        for (int tabIndex = 0; tabIndex < clickedTabIndex; tabIndex++)
            _tabWidget->removeTab(0);
    } else if (selectedAction == loadUrlInOthers) {
        TabWebView *tabWebView = nullptr;
        tabWebView = qobject_cast<TabWebView *>(_tabWidget->widget(clickedTabIndex));
        if (tabWebView != nullptr) {
            QUrl currentUrl = tabWebView->getCurrentUrl();
            qDebug() << "current url" << currentUrl;
            for (int tabIndex = 0; tabIndex < tabCount; tabIndex++) {
                if (tabIndex == clickedTabIndex)
                    continue;
                tabWebView = qobject_cast<TabWebView *>(_tabWidget->widget(tabIndex));
                if (tabWebView != nullptr)
                    tabWebView->loadUrl(currentUrl.toString());
            }
        }
    }
}
