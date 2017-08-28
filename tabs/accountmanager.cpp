#include "accountmanager.h"
#include "ui_accountmanager.h"
#include "addaccountdialog.h"
#include "updateaccountdialog.h"
#include <QDebug>
#include <QMessageBox>

AccountManager::AccountManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AccountManager)
{
    ui->setupUi(this);

    mModel = new AccountModel(ui->tableView);
    ui->tableView->setModel(mModel);
    ui->tableView->setColumnHidden(0, true);
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
                query.prepare("INSERT INTO accounts (email, password, sequence) SELECT :email, :password, ifnull(max(sequence), 0) + 1 FROM accounts");
                query.bindValue(":email", new_email);
                query.bindValue(":password", new_password);
                if (query.exec()) {
                    // refresh, how to do it?
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
    /*
     * QItemSelectionModel store selection for every cell in our view (QTableView).
     * We are only interested in the rows selection.
     */
    QVariantList selectedAccountIds;
    QList<QString> selectedAccountEmails;
    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();
    for (QModelIndex index : selectionIndexList) {
        int accountId = mModel->data(mModel->index(index.row(), 0)).toInt();
        if (selectedAccountIds.contains(accountId))
            continue;
        selectedAccountIds << accountId;
        selectedAccountEmails << mModel->data(mModel->index(index.row(), 1)).toString();
    }

    if (selectedAccountIds.isEmpty())
        return;

    /* confirm from user */
    QString text;
    QString informativeText;
    int count = selectedAccountEmails.size();
    if (count > 1) {
        text = "Are you sure you want to remove the following accounts?";
        for (QString email : selectedAccountEmails) {
            informativeText += email;
            informativeText += "\n";
        }
    } else {
        text = "Are you sure you want to remove account?";
        informativeText = selectedAccountEmails.at(0);
    }
    QMessageBox msgBox(this);
    msgBox.setText(text);
    msgBox.setInformativeText(informativeText);
    if (count > 1)
        msgBox.setStandardButtons(QMessageBox::YesToAll | QMessageBox::Cancel);
    else
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    switch (msgBox.exec()) {
    case QMessageBox::Yes:
    case QMessageBox::YesToAll:
        break;
    default:
        return;
    }

    /* remove selected account emails from database. */
    QSqlQuery q;
    q.prepare("DELETE FROM accounts WHERE _id = ?");
    q.addBindValue(selectedAccountIds);
    if (q.execBatch())
        mModel->setQuery(mModel->query().executedQuery());
    else {
        QMessageBox notify(this);
        notify.setText("Failed to remove selected accounts");
        notify.setInformativeText(q.lastError().text());
        notify.exec();
    }
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

void AccountManager::on_tableView_doubleClicked(const QModelIndex &index)
{
    /* lookup model to retrieve the selected account id */
    int accountId = mModel->data(mModel->index(index.row(), 0)).toInt();

    /* query db to retrieve account data */
    QString email;
    QString password;
    QSqlQuery query;
    query.prepare("SELECT email, password FROM accounts WHERE _id = :accountId");
    query.bindValue(":accountId", accountId);
    if (query.exec() && query.next()) {
        email = query.value("email").toString();
        password = query.value("password").toString();
    }

    /* invoke update account dialog */
    UpdateAccountDialog dlg(this, email, password, false);
    if (dlg.exec() == QDialog::Accepted) {
        QString updated_email = dlg.getEmail();
        QString updated_password = dlg.getPassword();
        if (updated_email.compare(email) || updated_password.compare(password)) {
            query.prepare("UPDATE accounts SET email = :email, password = :password WHERE _id = :accountId");
            query.bindValue(":email", updated_email);
            query.bindValue(":password", updated_password);
            query.bindValue(":accountId", accountId);
            if (query.exec())
                mModel->setQuery(mModel->query().executedQuery());
            else
                qDebug() << query.lastError();
        } else
            qDebug() << "No change on account data. Do nothing!";
    }
}

// ----------------------------------------------------------------
AccountModel::AccountModel(QWidget *parent) :
    QSqlQueryModel(parent)
{
    QString sql;
    sql.append("SELECT a._id AS 'Id', a.email AS 'Email address', i.ign AS 'In game name', f.fbid AS 'Facebook id', g.guildId AS 'Guide', r.role AS 'Role' FROM accounts AS a ");
    sql.append("LEFT JOIN igns AS i ON i.accountId = a._id ");
    sql.append("LEFT JOIN fbids AS f ON f.accountId = a._id ");
    sql.append("LEFT JOIN account_guild_mappings AS g ON g.accountId == a._id ");
    sql.append("LEFT JOIN roles AS r ON r.accountId = a._id ");
    sql.append("ORDER BY a.sequence");
    this->setQuery(sql);
}

AccountModel::~AccountModel()
{

}
