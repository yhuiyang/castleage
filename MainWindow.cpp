#include <QtWidgets>
#include "MainWindow.h"
#include "MdiChild.h"
#include "sqliteopenhelper.h"
#include "ImportAccountDialog.h"
#include "AccountManagementDialog.h"

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
    /* create actions first, then add actions to menu and toolbar, finally connect necessary signals/slots. */

    QAction *actionNewBrowser = new QAction(QIcon(":toolbar/new_window.png"), tr("New &Browser Window"), this);
    QAction *actionShowAccountManagementDialog = new QAction(tr("Account Management..."), this);
    QAction *actionToggleBrowserToolBar = new QAction("Browser", this);
    QAction *actionToggleAccountToolBar = new QAction("Account", this);
    actionToggleBrowserToolBar->setCheckable(true);
    actionToggleAccountToolBar->setCheckable(true);

    QMenu *menuView = menuBar()->addMenu(tr("&View"));
    QMenu *menuAccount = menuBar()->addMenu(tr("&Account"));
    QMenu *menuToolbar = menuView->addMenu(tr("Toolbar"));

    menuView->addAction(actionNewBrowser);
    menuView->addSeparator();
    menuView->addMenu(menuToolbar);
    menuAccount->addAction(actionShowAccountManagementDialog);
    menuToolbar->addAction(actionToggleBrowserToolBar);
    menuToolbar->addAction(actionToggleAccountToolBar);

    QToolBar *toolbarBrowser = addToolBar(tr("Browser"));
    QToolBar *toolbarAccount = addToolBar(tr("Account"));

    toolbarBrowser->addAction(actionNewBrowser);
    toolbarAccount->addAction(actionShowAccountManagementDialog);

    connect(actionNewBrowser, SIGNAL(triggered(bool)), this, SLOT(createChildBrowser()));
    connect(actionShowAccountManagementDialog, SIGNAL(triggered(bool)), this, SLOT(showAccountManagementDialog()));
    connect(actionToggleBrowserToolBar, SIGNAL(triggered(bool)), toolbarBrowser, SLOT(setVisible(bool)));
    connect(actionToggleAccountToolBar, SIGNAL(triggered(bool)), toolbarAccount, SLOT(setVisible(bool)));
    connect(toolbarBrowser, SIGNAL(visibilityChanged(bool)), actionToggleBrowserToolBar, SLOT(setChecked(bool)));
    connect(toolbarAccount, SIGNAL(visibilityChanged(bool)), actionToggleAccountToolBar, SLOT(setChecked(bool)));
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

void MainWindow::showImportAccountDialog() {
    ImportAccountDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString email, password;
        dlg.getAccountData(email, password);
        if (email.isEmpty() || password.isEmpty()) {
            qDebug() << "Email and password fields can not be empty";
            QMessageBox::warning(this, "Warning", "Email and password fields can not be empty.");
        } else {
            QSqlQuery q;
            q.prepare("INSERT OR REPLACE INTO accounts (email, password) VALUES (:email, :password)");
            q.bindValue(":email", email);
            q.bindValue(":password", password);
            if (q.exec())
                emit ca_account_updated();
        }
    }
}

void MainWindow::showAccountManagementDialog()
{
    AccountManagementDialog dlg(this);
    dlg.exec();
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
    q.exec("CREATE TABLE IF NOT EXISTS guildids ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", guildId TEXT REFERENCES guilds ON UPDATE CASCADE"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS fbids ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", fbid TEXT NOT NULL"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS guilds ("
           "id TEXT PRIMARY KEY"
           ", name TEXT NOT NULL"
           ", creatorId TEXT NOT NULL"
           ", createdAt DATETIME"
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
