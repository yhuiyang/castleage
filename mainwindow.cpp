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
    mSqlQueryStringRetrieveAcctountList = "SELECT email, password FROM Accounts ORDER BY timestamp";
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery(mSqlQueryStringRetrieveAcctountList);
    ui->listAccount->setModel(model);
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
        {
            /* not sure what is correct way to refresh account list and keep selected item. */
            int selectedCount = ui->listAccount->selectionModel()->selectedIndexes().count();
            QModelIndex selected = ui->listAccount->selectionModel()->currentIndex();
            QSqlQueryModel *m = qobject_cast<QSqlQueryModel *>(ui->listAccount->model());
            m->setQuery(mSqlQueryStringRetrieveAcctountList);
            if (selectedCount > 0)
                ui->listAccount->selectionModel()->setCurrentIndex(selected, QItemSelectionModel::Select);
        }
    }

}

void MainWindow::onRemoveAccount()
{
    int selectedCount = ui->listAccount->selectionModel()->selectedIndexes().count();
    if (selectedCount > 0)
    {
        QModelIndexList selectedIndexList = ui->listAccount->selectionModel()->selectedIndexes();
        QString email = selectedIndexList.at(0).data().toString();

        QMessageBox dlg(QMessageBox::Question,
                        "Confirm remove account",
                        "Are you sure you want to remove account: " + email,
                        QMessageBox::Yes | QMessageBox::No,
                        this);
        //dlg.setInformativeText("infomative text");
        //dlg.setDetailedText("detail text");
        if (dlg.exec() == QMessageBox::Yes)
        {
            QSqlQuery q;
            q.prepare("DELETE FROM Accounts WHERE email = :email");
            q.bindValue(":email", email);
            if (q.exec())
            {
                QSqlQueryModel *m = qobject_cast<QSqlQueryModel *>(ui->listAccount->model());
                m->setQuery(mSqlQueryStringRetrieveAcctountList);
            }
        }
    }
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

