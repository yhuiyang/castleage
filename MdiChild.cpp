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
    // accept only url to https://web3.castleagegame.com/castle_ws/
    bool addressUpdated = false;
    QString addressString = _address->text();
    addressString = addressString.trimmed();
    if (QString::compare(addressString, _address->text()))
        addressUpdated = true;
    if (addressString.startsWith("http://")) {
        addressString.replace("http://", "https://");
        addressUpdated = true;
    }
    if (addressString.startsWith("https://web4.")) {
        addressString.replace("https://web4.", "https://web3.");
        addressUpdated = true;
    }
    if (addressUpdated)
        _address->setText(addressString);
    if (addressString.startsWith("https://web3.castleagegame.com/castle_ws/")) {
        _webView->load(QUrl(addressString));
    } else {
        QMessageBox::critical(this, "Wrong URL address!", "Only url addresses start with 'http://web3.castleagegame.com/castle_ws/' are allowed.");
    }
}

void TabWebView::onAccountIndexChanged(int index)
{
    qlonglong accountId = _comboBoxAccount->itemData(index).toLongLong();
    this->_netMgr->switchAccount(accountId);
    this->_webView->reload();

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
    _tabWidget->setTabsClosable(true);
    _tabWidget->setCornerWidget(btnAddTab);
    this->setCentralWidget(_tabWidget);
    this->setMinimumWidth(800);

    connect(btnAddTab, &QPushButton::clicked, this, &MdiChild::addTabWebView);

    addTabWebView();

    setAttribute(Qt::WA_DeleteOnClose);
}

int MdiChild::addTabWebView()
{
    QString tabText = "NO ACCOUNT SELECTED";
    TabWebView *tabWebView = new TabWebView(this);
    int tabIndex = _tabWidget->addTab(tabWebView, tabText);

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
