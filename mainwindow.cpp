#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "newaccountdialog.h"
#include "sqliteopenhelper.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    /* manage database generation or upgrade. */
    mSQLiteOpenHelper = new SQLiteOpenHelper("castleage.sqlite", 1);
    connect(mSQLiteOpenHelper, SIGNAL(createDatabase(QSqlDatabase&)), this, SLOT(onCreateDatabase(QSqlDatabase&)));
    connect(mSQLiteOpenHelper, SIGNAL(upgradeDatabase(QSqlDatabase&,int,int)), this, SLOT(onUpgradeDatabase(QSqlDatabase&,int,int)));
    connect(mSQLiteOpenHelper, SIGNAL(downgradeDatabase(QSqlDatabase&,int,int)), this, SLOT(onDowngradeDatabase(QSqlDatabase&,int,int)));
    mSQLiteOpenHelper->init();

    /* setup ui */
    ui->setupUi(this);

    /* populate the account list */
    populateAccounts();

}

MainWindow::~MainWindow()
{
    delete ui;
    if (mSQLiteOpenHelper != nullptr)
    {
        delete mSQLiteOpenHelper;
        mSQLiteOpenHelper = nullptr;
    }
}

//
// Helper methods
//
void MainWindow::populateAccounts()
{
    QSqlQuery q;
    if (q.exec("SELECT email FROM Accounts ORDER BY timestamp"))
    {
        QStringList l;
        while (q.next())
            l << q.value("email").toString();
        ui->listAccount->addItems(l);
    }
}

//
// Slot implementations
//
void MainWindow::onAddAccount()
{
    NewAccountDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        QSqlQuery q;
        q.prepare("INSERT INTO Accounts(email, password) VALUES(:email, :password)");
        q.bindValue(":email", dlg.getEmail());
        q.bindValue(":password", dlg.getPassword());
        if (q.exec())
            ui->listAccount->addItem(dlg.getEmail());
    }

}

void MainWindow::onRemoveAccount()
{
    /* QListWidget::currentItem() return 1st item before we really select one, so don't use it, use our cached one. */
    if (mSelectedAccountItem == nullptr)
        return;

    QMessageBox dlg(QMessageBox::Question,
                    "Confirm remove account",
                    "Are you sure you want to remove account: " + mSelectedAccountItem->text(),
                    QMessageBox::Yes | QMessageBox::No,
                    this);
    //dlg.setInformativeText("Add informative text here");
    //dlg.setDetailedText("Add detail text here");
    if (dlg.exec() == QMessageBox::Yes)
    {
        QSqlQuery q;
        q.prepare("DELETE FROM Accounts WHERE email = :email");
        q.bindValue(":email", mSelectedAccountItem->text());
        if (q.exec())
        {
            delete mSelectedAccountItem;
            /* do NOT set mSeleectedAccountItem to nullptr here,
             * onAccountSelectionChanged() will assign another selected item to it, if there exists one. */
        }
        else
            qDebug() << "Failed to delete account: " << mSelectedAccountItem->text();
    }

    return;
}

void MainWindow::onReloadSelectedAccount()
{
    if (this->mSelectedAccountItem != nullptr)
    {
        qDebug() << "Reload stats for account:" << this->mSelectedAccountItem->text();
    }
}

void MainWindow::onReloadAllAccounts()
{
    qDebug() << "Reload stats for all accounts.";
}

void MainWindow::onCreateDatabase(QSqlDatabase &db)
{
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS Accounts(email TEXT UNIQUE, password TEXT, timestamp TIMESTAMT DEFAULT CURRENT_TIMESTAMP)");
}

void MainWindow::onUpgradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion)
{
    Q_UNUSED(db);
    Q_UNUSED(dbVersion);
    Q_UNUSED(codeVersion);

//    switch (dbVersion)
//    {
//    case 1:
//        qDebug() << "Upgrade database version from 1 to 2...";
//    case 2:
//        qDebug() << "Upgrade database version from 2 to 3...";
//    }
}

void MainWindow::onDowngradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion)
{
    Q_UNUSED(db);
    Q_UNUSED(dbVersion);
    Q_UNUSED(codeVersion);
}

void MainWindow::onAccountSelectionChanged()
{
    mSelectedAccountItem = ui->listAccount->currentItem();
}
