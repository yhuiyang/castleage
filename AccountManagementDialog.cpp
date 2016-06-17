#include <QMessageBox>
#include <QtSql>
#include <QWebFrame>
#include <QWebElement>
#include <QWebElementCollection>
#include "AccountManagementDialog.h"
#include "ui_AccountManagementDialog.h"
#include "SynchronizedNetworkAccessManager.h"

AccountManagementDialog::AccountManagementDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AccountManagementDialog)
{
    ui->setupUi(this);

    populuteAccounts();
    connect(this, SIGNAL(account_updated()), this, SLOT(populuteAccounts()));

    _page.settings()->setAttribute(QWebSettings::AutoLoadImages, false);
    _page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

AccountManagementDialog::~AccountManagementDialog()
{
    delete ui;
}

void AccountManagementDialog::populuteAccounts()
{
    QSqlQueryModel *model = new QSqlQueryModel;
    model->setQuery("SELECT email, ign AS \"IGN\", g.name AS \"Guild\", fbid AS \"FBID\" FROM accounts AS a "
                    "LEFT JOIN igns ON igns.accountId = a.id "
                    "LEFT JOIN guildids ON guildids.accountId = a.id "
                    "LEFT JOIN guilds AS g ON g.id = guildids.guildId "
                    "LEFT JOIN fbids ON fbids.accountId = a.id "
                    "ORDER BY timestamp");
    QAbstractItemModel *oldModel = ui->accountTable->model();
    ui->accountTable->setModel(model);
    if (oldModel != nullptr)
        oldModel->deleteLater();
    for (int col = 0; col < 4; col++)
        ui->accountTable->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
}

void AccountManagementDialog::showLog(const QString &message)
{
    QString msg(message);
    ui->messageOutput->append(msg);
}

SynchronizedNetworkAccessManager * AccountManagementDialog::getNetworkAccessManager(const qlonglong accountId, const QByteArray &accountEmail)
{
    if (_accountMgrs.contains(QString::number(accountId)))
        return _accountMgrs.value(QString::number(accountId));
    if (_accountMgrs.contains(accountEmail))
        return _accountMgrs.value(accountEmail);

    SynchronizedNetworkAccessManager *mgr = new SynchronizedNetworkAccessManager(accountId, this);
    _accountMgrs.insert(QString::number(accountId), mgr);
    _accountMgrs.insert(accountEmail, mgr);
    return mgr;
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
    SynchronizedNetworkAccessManager *mgr;
    QList<QPair<qlonglong, QByteArray>> check;

    QSqlQuery sql;
    if (!sql.exec("SELECT id, email FROM accounts")) {
        qCritical() << "Failed to retrieve accounts from database.";
        showLog("Failed to retrieve accounts from database.");
        return;
    }
    while (sql.next())
        check.append(QPair<qlonglong, QByteArray>(sql.value("id").toLongLong(), sql.value("email").toByteArray()));
    sql.finish();

    bool guildIdUpdated = false;
    QSqlQuery sqlInsertAccountGuildId;
    QSqlQuery sqlInsertGuild;
    sqlInsertAccountGuildId.prepare("INSERT OR REPLACE INTO guildids VALUES (:accountId, :guildId)");
    sqlInsertGuild.prepare("INSERT OR IGNORE INTO guilds VALUES (:guildId, :guildName, :guildCreatorId, :guildCreatedAt)");
    for (QPair<qlonglong, QByteArray> account: check) {
        mgr = getNetworkAccessManager(account.first, account.second);

        QByteArray response = mgr->ca_get("keep.php");
        QWebFrame *frame = _page.mainFrame();
        frame->setHtml(response);
        QWebElement guildElem = frame->findFirstElement("span[title$=\"'s Guild\"] > a[href^=\"guildv2_home.php?guild_id=\"]");
        QWebElement noGuildElem = frame->findFirstElement("a[href=\"guildv2_list.php\"] > img[alt^=\"Join A Guild\"]");
        if (!guildElem.isNull()) {
            QString guildLink = guildElem.attribute("href");
            QString guildName = guildElem.toPlainText();
            QString guildId = guildLink.split('=').value(1);
            QString guildCreatorId = guildId.split('_').value(0);
            QString guildCreatedAt = guildId.split('_').value(1);
            showLog("account[" + account.second + "] guild => " + guildName + " (" + guildId + ").");

            sqlInsertGuild.bindValue(":guildId", guildId);
            sqlInsertGuild.bindValue(":guildName", guildName);
            sqlInsertGuild.bindValue(":guildCreatorId", guildCreatorId);
            sqlInsertGuild.bindValue(":guildCreatedAt", guildCreatedAt);
            sqlInsertGuild.exec();

            sqlInsertAccountGuildId.bindValue(":accountId", account.first);
            sqlInsertAccountGuildId.bindValue(":guildId", guildId);
            guildIdUpdated |= sqlInsertAccountGuildId.exec();
        } else if (!noGuildElem.isNull()){
            showLog("account[" + account.second + "] didn't join any guild.");
            sqlInsertAccountGuildId.bindValue(":accountId", account.first);
            sqlInsertAccountGuildId.bindValue(":guildId", "");
            guildIdUpdated |= sqlInsertAccountGuildId.exec();
        } else {
            showLog("Failed to find guild of account[" + account.second + "]");
        }
    }

    if (guildIdUpdated)
        emit account_updated();
}

void AccountManagementDialog::onUpdateIGNs()
{
    SynchronizedNetworkAccessManager *mgr;
    QList<QPair<qlonglong, QByteArray>> check;

    QSqlQuery sql;
    if (!sql.exec("SELECT id, email FROM accounts")) {
        qCritical() << "Failed to retrieve accounts from database.";
        showLog("Failed to retrieve accounts from database.");
        return;
    }
    while (sql.next())
        check.append(QPair<qlonglong, QByteArray>(sql.value("id").toLongLong(), sql.value("email").toByteArray()));
    sql.finish();

    bool ignUpdated = false;
    sql.clear();
    sql.prepare("INSERT OR REPLACE INTO igns VALUES (:accountId, :ign)");
    for (QPair<qlonglong, QByteArray> account: check) {
        mgr = getNetworkAccessManager(account.first, account.second);

        QByteArray response = mgr->ca_get("keep.php");
        QWebFrame *frame = _page.mainFrame();
        frame->setHtml(response);
        QWebElement elem = frame->findFirstElement("div#app_body div[style*=\"keep_top.jpg\"] div[title]");
        if (!elem.isNull()) {
            QString ign = elem.attribute("title");
            showLog("Update ign for account[" + account.second + "] => " + ign);
            sql.bindValue(":accountId", account.first);
            sql.bindValue(":ign", ign);
            ignUpdated |= sql.exec();
        } else {
            showLog("Failed to update ign for account[" + account.second + "]");
        }
    }

    if (ignUpdated)
        emit account_updated();
}

void AccountManagementDialog::onUpdateFBIDs()
{
    SynchronizedNetworkAccessManager *mgr;
    QList<QPair<qlonglong, QByteArray>> check;

    QSqlQuery sql;
    if (!sql.exec("SELECT id, email FROM accounts")) {
        qCritical() << "Failed to retrieve accounts from database.";
        showLog("Failed to retrieve accounts from database.");
        return;
    }
    while (sql.next())
        check.append(QPair<qlonglong, QByteArray>(sql.value("id").toLongLong(), sql.value("email").toByteArray()));
    sql.finish();

    bool fbidUpdated = false;
    sql.clear();
    sql.prepare("INSERT OR REPLACE INTO fbids VALUES(:accountId, :fbid)");
    for (QPair<qlonglong, QByteArray> account: check) {
        mgr = getNetworkAccessManager(account.first, account.second);

        QByteArray response = mgr->ca_get("keep.php");
        QWebFrame *frame = _page.mainFrame();
        frame->setHtml(response);
        QWebElement elem = frame->findFirstElement("a[href^=\"keep.php?user=\"] > img[title^=\"view your stats\"]");
        if (!elem.isNull()) {
            QString href = elem.parent().attribute("href");
            QString fbid = href.split('=').value(1);
            showLog("Update fbid for account[" + account.second + "] => " + fbid);
            sql.bindValue(":accountId", account.first);
            sql.bindValue(":fbid", fbid);
            fbidUpdated |= sql.exec();
        } else {
            showLog("Failed to update fbid for account[" + account.second + "]");
        }
    }

    if (fbidUpdated)
        emit account_updated();
}

void AccountManagementDialog::onAccountMoveUp()
{
    qDebug() << "Move account up";
}

void AccountManagementDialog::onAccountMoveDown()
{
    qDebug() << "Move account down";
}
