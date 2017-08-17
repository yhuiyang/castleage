#include "accountmanager.h"
#include "ui_accountmanager.h"
#include "addaccountdialog.h"
#include <QDebug>

AccountManager::AccountManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AccountManager)
{
    ui->setupUi(this);

    mModel = new AccountModel(ui->tableView);
    ui->tableView->setModel(mModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

AccountManager::~AccountManager()
{
    delete ui;
}

void AccountManager::on_actionAddAccount_triggered()
{
    QString email = QString();
    QString password = QString();
    bool reveal = false;
    bool reopen = false;
    bool reserveEmail = false;
    bool reservePassword = false;

    while (true) {
        AddAccountDialog dlg(this, email, password, reveal, reopen, reserveEmail, reservePassword);
        // exec() - application modal dialog; open() - window modal dialog
        if (dlg.exec() == QDialog::Accepted) {

            /* insert new account data */
            QString new_email = dlg.getEmail();
            QString new_password = dlg.getPassword();
            qDebug() << "Account Added: Email" << new_email << "Password" << new_password;
            if (!new_email.isEmpty() && !new_password.isEmpty()) {

                QSqlQuery query;
                query.prepare("INSERT INTO accounts (email, password) VALUES (:email, :password)");
                query.bindValue(":email", new_email);
                query.bindValue(":password", new_password);
                if (query.exec()) {
                    // refresh, how to do it?
                    //emit mModel->dataChanged(QModelIndex(), QModelIndex());
                    mModel->setQuery(mModel->query().executedQuery());
                } else {
                    qWarning() << "Failed to insert account to database. Reason: " << query.lastError();
                }
            }

            /* prepare dialog reopen if needed */
            if (dlg.shouldReopenSelf()) {
                reveal = dlg.shouldRevealPassword();
                reopen = dlg.shouldReopenSelf();
                reserveEmail = dlg.shouldReserveEmail();
                reservePassword = dlg.shouldReservePassword();
                email = reserveEmail ? new_email : QString();
                password = reservePassword ? new_password : QString();
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

void AccountManager::on_actionRemoveAccount_triggered()
{

}

void AccountManager::on_actionUpdateIGN_triggered()
{

}

void AccountManager::on_actionUpdateFBId_triggered()
{

}

void AccountManager::on_actionUpdateGuild_triggered()
{

}

void AccountManager::on_actionUpdateRole_triggered()
{

}

// ----------------------------------------------------------------
AccountModel::AccountModel(QWidget *parent) :
    QSqlQueryModel(parent)
{
    QString sql;
    sql.append("SELECT a.email, i.ign, f.fbid, g.guildId, r.role FROM accounts AS a ");
    sql.append("LEFT JOIN igns AS i ON i.accountId = a._id ");
    sql.append("LEFT JOIN fbids AS f ON f.accountId = a._id ");
    sql.append("LEFT JOIN account_guild_mappings AS g ON g.accountId == a._id ");
    sql.append("LEFT JOIN roles AS r ON r.accountId = a._id ");
    sql.append("ORDER BY a.sequence");
    this->setQuery(sql);

    this->setHeaderData(0, Qt::Horizontal, QString("Email address"));
    this->setHeaderData(1, Qt::Horizontal, QString("In game name"));
    this->setHeaderData(2, Qt::Horizontal, QString("Facebook id"));
    this->setHeaderData(3, Qt::Horizontal, QString("Guild"));
    this->setHeaderData(4, Qt::Horizontal, QString("Role"));
}

AccountModel::~AccountModel()
{

}
