#include <QtWidgets>
#include "MainWindow.h"
#include "MdiChild.h"
#include "sqliteopenhelper.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    _mdiArea = new QMdiArea;
    _mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setCentralWidget(_mdiArea);
    this->setMinimumHeight(600);
    this->setMinimumWidth(800);

    _dbHelper = new SQLiteOpenHelper("cabrowser.sqlite", 1);
    connect(_dbHelper, SIGNAL(createDatabase(QSqlDatabase&)), this, SLOT(onCreateDatabase(QSqlDatabase&)));
    connect(_dbHelper, SIGNAL(upgradeDatabase(QSqlDatabase&,int,int)), this, SLOT(onUpgradeDatabase(QSqlDatabase&,int,int)));
    connect(_dbHelper, SIGNAL(downgradeDatabase(QSqlDatabase&,int,int)), this, SLOT(onDowngradeDatabase(QSqlDatabase&,int,int)));
    _dbHelper->init();


    createActions();
    createStatusBar();
    setUnifiedTitleAndToolBarOnMac(true);
}

MainWindow::~MainWindow()
{
    if (_dbHelper != nullptr) {
        delete _dbHelper;
        _dbHelper = nullptr;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

void MainWindow::readSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));

    _actionNewBrowser = new QAction(tr("&New Browser"), this);
    _actionNewBrowser->setStatusTip(tr("Create a new browser"));;
    connect(_actionNewBrowser, &QAction::triggered, this, &MainWindow::createChildBrowser);
    fileMenu->addAction(_actionNewBrowser);
    fileToolBar->addAction(_actionNewBrowser);

}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

// ------------------------------------------------------------------------------------------
// Slots
// ------------------------------------------------------------------------------------------
void MainWindow::createChildBrowser() {
    MdiChild *child = new MdiChild;
    _mdiArea->addSubWindow(child);
    child->show();
}

void MainWindow::onCreateDatabase(QSqlDatabase &db)
{
    qDebug() << "Database" << "createDatabase";

    QSqlQuery q(db);

    q.exec("CREATE TABLE IF NOT EXISTS accounts ("
           "id INTEGER PRIMARY KEY"
           ", email TEXT UNIQUE NOT NULL"
           ", password TEXT NOT NULL"
           ", timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS cookies ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", cookie TEXT NOT NULL"
           ", modified DATETIME DEFAULT CURRENT_TIMESTAMP"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS igns ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", ign TEXT NOT NULL"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS guilds ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", guild TEXT NOT NULL"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS fbids ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", fbid TEXT NOT NULL"
           ")");
}

void MainWindow::onUpgradeDatabase(QSqlDatabase &db, int oldVersion, int newVersion)
{
    Q_UNUSED(db);
    qDebug() << "Database" << "upgradeDatabase" << oldVersion << "->" << newVersion;
}

void MainWindow::onDowngradeDatabase(QSqlDatabase &db, int oldVersion, int newVersion)
{
    Q_UNUSED(db);
    qDebug() << "Database" << "downgradeDatabase" << oldVersion << "->" << newVersion;
}
