#include <QVBoxLayout>
#include <QToolBar>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QLabel>
#include <QUrl>
#include <QtSql>
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
    populateFilter();

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
                "SELECT 'No filter', 0 "
                "UNION "
                "SELECT 'Guild: ' || name, _id * -1 FROM guilds "
                "UNION "
                "SELECT 'Tag: ' || name, _id FROM tags"
                );
    mFilterList->setModel(filterModel);
}

void Browser::onFilterIndexChanged(int filterIndex)
{
    qDebug() << "filter index changes to" << filterIndex << ", update account ComboBox.";

    /* index -> filter id */
    int filterId = mFilterList->model()->data(mFilterList->model()->index(filterIndex, 1)).toInt();

    /* update account list */
    QSqlQueryModel *accountModel = new QSqlQueryModel;
    if (filterId == 0) { // All
        accountModel->setQuery(
                    "SELECT '------ Select Account ------' AS account, 0 AS _id, 0 AS sequence "
                    "UNION "
                    "SELECT IFNULL(i.ign, 'Unknown IGN') || ' - ' || email AS account, _id, sequence "
                    "FROM accounts AS a "
                    "LEFT JOIN igns AS i ON i.accountId = a._id "
                    "ORDER BY a.sequence"  // Note 'ORDER BY' executes _AFTER_ 'UNION', so first query must also contains sequence column.
                    );
    } else if (filterId < 0) { // by selected guild
        QString sql;
        sql.append("SELECT '------ Select Account ------' AS account, 0 AS _id ");
        sql.append("UNION ");
        sql.append("SELECT IFNULL(i.ign, 'UnknownIGN') || ' - ' || a.email AS account, a._id FROM account_guild_mappings AS m ");
        sql.append("INNER JOIN accounts AS a ON a._id = m.accountId ");
        sql.append("LEFT JOIN igns AS i ON i.accountId = m.accountId ");
        sql.append("WHERE m.guildId = %1 ");
        accountModel->setQuery(sql.arg(-filterId));
    } else { // by selected tag
        QString sql;
        sql.append("SELECT '------ Select Account ------' AS account, 0 AS _id, 0 AS sequence ");
        sql.append("UNION ");
        sql.append("SELECT IFNULL(i.ign, 'UnknownIGN') || ' - ' || a.email AS account, a._id, m.sequence FROM account_tag_mappings AS m ");
        sql.append("INNER JOIN accounts AS a ON a._id = m.accountId ");
        sql.append("LEFT JOIN igns AS i ON i.accountId = m.accountId ");
        sql.append("WHERE m.tagId = %1 ");
        sql.append("ORDER BY m.sequence");
        accountModel->setQuery(sql.arg(filterId));
    }

    mAccountList->setModel(accountModel);
}

void Browser::onAccountIndexChanged(int accountIndex)
{
    qDebug() << "account index changes to" << accountIndex;

    /* index -> account id */
    int accountId = mAccountList->model()->data(mAccountList->model()->index(accountIndex, 1)).toInt();

    qDebug() << "accountId =" << accountId;
}
