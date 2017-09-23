#include <QVBoxLayout>
#include <QToolBar>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QLabel>
#include <QUrl>
#include "browser.h"

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

    mWebView->load(QUrl("https://web3.castleagegame.com/castle_ws/index.php"));
    //mWebView->load(QUrl("https://www.google.com.tw"));
    mWebView->show();
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

    //navigationBar->addSeparator();
    addToolBarBreak();

    QToolBar *accountToolBar = addToolBar("Account");
    accountToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    accountToolBar->toggleViewAction()->setEnabled(false);

    accountToolBar->addWidget(new QLabel("Account", accountToolBar));
    mAccountList = new QComboBox(this);
    accountToolBar->addWidget(mAccountList);
    accountToolBar->addWidget(new QLabel("Filter", accountToolBar));
    mFilterList = new QComboBox(this);
    accountToolBar->addWidget(mFilterList);
    mLockComboBox = new QCheckBox("Lock drop down", this);
    accountToolBar->addWidget(mLockComboBox);
}
