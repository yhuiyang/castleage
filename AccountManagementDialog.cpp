#include <QMessageBox>
#include <QtSql>
#include "AccountManagementDialog.h"
#include "ui_AccountManagementDialog.h"

AccountManagementDialog::AccountManagementDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccountManagementDialog)
{
    ui->setupUi(this);
    populuteAccounts();
    connect(this, SIGNAL(account_updated()), this, SLOT(populuteAccounts()));
}

AccountManagementDialog::~AccountManagementDialog()
{
    delete ui;
}

void AccountManagementDialog::populuteAccounts()
{
    QSqlQueryModel *model = new QSqlQueryModel;
    model->setQuery("SELECT email, ign, g.name, fbid FROM accounts AS a "
                    "LEFT JOIN igns ON igns.accountId = a.id "
                    "LEFT JOIN guildids ON guildids.accountId = a.id "
                    "LEFT JOIN guilds AS g ON g.id = guildids.guildId "
                    "LEFT JOIN fbids ON fbids.accountId = a.id "
                    "ORDER BY timestamp");
    QAbstractItemModel *oldModel = ui->accountTable->model();
    ui->accountTable->setModel(model);
    if (oldModel != nullptr)
        oldModel->deleteLater();
    ui->accountTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

void AccountManagementDialog::onAccountImport()
{
    QString email = ui->lineEditEmail->text();
    QString password = ui->lineEditPassword->text();
    if (email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Both email and password can not be empty.");
        return;
    }

    QSqlQuery sql;
    sql.prepare("INSERT OR REPLACE INTO accounts (email, password) VALUES (:email, :password)");
    sql.bindValue(":email", email);
    sql.bindValue(":password", password);
    if (sql.exec())
        emit account_updated();
}

void AccountManagementDialog::onAccountDelete()
{
    QString email = ui->lineEditEmail->text();
    if (email.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Email can not be empty");
        return;
    }

    QSqlQuery sql;
    sql.prepare("DELETE FROM accounts WHERE email = :email");
    sql.bindValue(":email", email);
    if (sql.exec())
        emit account_updated();
}

void AccountManagementDialog::onUpdateGuilds()
{
    qDebug() << "Update Guilds";
}

void AccountManagementDialog::onUpdateIGNs()
{
    QSqlQuery sql;
    if (!sql.exec("SELECT id, email, password FROM accounts")) {
        qCritical() << "Failed to retrieve accounts from database.";
        return;
    }

    while (sql.next()) {
        qlonglong id = sql.value("id").toLongLong();
        QByteArray email = sql.value("email").toByteArray();
        QByteArray password = sql.value("password").toByteArray();
    }

}

void AccountManagementDialog::onUpdateFBIDs()
{
    qDebug() << "Update FBIDs";
}

void AccountManagementDialog::onAccountMoveUp()
{
    qDebug() << "Move account up";
}

void AccountManagementDialog::onAccountMoveDown()
{
    qDebug() << "Move account down";
}
