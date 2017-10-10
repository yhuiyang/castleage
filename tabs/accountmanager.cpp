#include <QDebug>
#include <QMessageBox>
#include <string.h>
#include "accountmanager.h"
#include "ui_accountmanager.h"
#include "addaccountdialog.h"
#include "updateaccountdialog.h"
#include "castleagehttpclient.h"
#include "gumbo.h"

static bool startsWith(const std::string & long_string, const std::string & short_string) {
    return short_string.length() <= long_string.length()
            && std::equal(short_string.begin(), short_string.end(), long_string.begin());
}

static bool startsWith(const char *_big, const char *_little) {
    size_t big_length = strlen(_big);
    size_t little_length = strlen(_little);
    return (big_length >= little_length) && !strncmp(_big, _little, little_length);
}

static bool contains(const char *_big, const char *_little) {
    size_t big_length = strlen(_big);
    size_t little_length = strlen(_little);
    return (little_length != 0) && (big_length >= little_length) && (strstr(_big, _little) != nullptr);
}

static bool endsWith(const char *_big, const char *_little) {
    size_t big_length = strlen(_big);
    size_t little_length = strlen(_little);
    return (big_length >= little_length) && !strncmp(_big + big_length - little_length, _little, little_length);
}

template<typename Func>
bool travelTree(GumboNode *node, Func& functor) {
    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return false;

    if (functor(node))
        return true;

    for (unsigned int i = 0; i < node->v.element.children.length; i++)
        if (travelTree(static_cast<GumboNode *>(node->v.element.children.data[i]), functor))
            return true;

    return false;
}

template<typename Func>
bool travelChildren(GumboNode *node, Func& functor) {
    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return false;

    for (unsigned int i = 0; i < node->v.element.children.length; i++)
        if (functor(node))
            return true;

    return false;
}

// ----------------------------------------------------------------------------------
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
    QList<int> selectedAccountIds = this->selectedAccountIds();
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO igns VALUES (:accountId, :ign)");

    for (int selectedAccountId : selectedAccountIds) {
        CastleAgeHttpClient client(selectedAccountId);

        QByteArray keepPage = client.post_sync("keep.php");
        if (!keepPage.isEmpty()) {
            qint64 t = QDateTime::currentMSecsSinceEpoch();
            GumboOutput *output = gumbo_parse(keepPage.data());
            qDebug() << "parse & build tree" << (QDateTime::currentMSecsSinceEpoch() - t) << "ms";

            GumboNode *pDiv = nullptr;
            auto find_div_withid_app_body = [&] (GumboNode *node) {
                if (!node || node->type != GUMBO_NODE_ELEMENT)
                    return false;
                GumboAttribute *attr;
                if (node->v.element.tag == GUMBO_TAG_DIV
                        && (attr = gumbo_get_attribute(&node->v.element.attributes, "id"))
                        && !strcmp(attr->value, "app_body")) {
                    pDiv = node;
                    return true;
                }
                return false;
            };
            travelTree(output->root, find_div_withid_app_body);

            GumboNode *pDivBg = nullptr;
            if (pDiv != nullptr) {
                auto find_div_with_bg = [&] (GumboNode *node) {
                    if (!node || node->type != GUMBO_NODE_ELEMENT)
                        return false;
                    GumboAttribute *attr;
                    if (node->v.element.tag == GUMBO_TAG_DIV
                            && (attr = gumbo_get_attribute(&node->v.element.attributes, "style"))
                            && contains(attr->value, "keep_top.jpg")) {
                        pDivBg = node;
                        return true;
                    }
                    return false;
                };
                travelTree(pDiv, find_div_with_bg);
            }

            const char *pIgn = nullptr;
            if (pDivBg != nullptr) {
                auto find_div_with_title = [&] (GumboNode *node) {
                    if (!node || node->type != GUMBO_NODE_ELEMENT)
                        return false;
                    GumboAttribute *attr;
                    if (node->v.element.tag == GUMBO_TAG_DIV
                            && (attr = gumbo_get_attribute(&node->v.element.attributes, "title"))
                            && (strlen(attr->value) > 0)) {
                        pIgn = attr->value;
                        return true;
                    }
                    return false;
                };
                travelTree(pDivBg, find_div_with_title);
            }

            if (pIgn != nullptr) {
                q.bindValue(":accountId", selectedAccountId);
                q.bindValue(":ign", QString::fromUtf8(pIgn));
                if (q.exec())
                    refreshTable();
            }

            gumbo_destroy_output(&kGumboDefaultOptions, output);
        }
    }
}

void AccountManager::on_actionUpdateFBId_triggered()
{
    QList<int> selectedAccountIds = this->selectedAccountIds();
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO fbids VALUES (:accountId, :fbid)");

    for (int selectedAccountId : selectedAccountIds) {
        CastleAgeHttpClient client(selectedAccountId);

        QByteArray keepPage = client.post_sync("keep.php");
        if (!keepPage.isEmpty()) {
            GumboOutput *output = gumbo_parse(keepPage.data());

            const char *pFbid = nullptr;
            auto find_fbid_in_a_href = [&] (GumboNode * node) {
                if (!node || node->type != GUMBO_NODE_ELEMENT)
                    return false;
                GumboAttribute *attr;
                if (node->v.element.tag == GUMBO_TAG_A
                        && (attr = gumbo_get_attribute(&node->v.element.attributes, "href"))
                        && startsWith(attr->value, "keep.php?user=")) {
                    pFbid = &attr->value[strlen("keep.php?user=")];
                    return true;
                }
                return false;
            };
            travelTree(output->root, find_fbid_in_a_href);
            if (pFbid != nullptr) {
                q.bindValue(":accountId", selectedAccountId);
                q.bindValue(":fbid", QString::fromUtf8(pFbid));
                if (q.exec())
                    refreshTable();
            }

            gumbo_destroy_output(&kGumboDefaultOptions, output);
        }
    }
}

void AccountManager::on_actionUpdateGuild_triggered()
{
    QList<int> selectedAccountIds = this->selectedAccountIds();
    QSqlQuery sqlGuild, sqlMapping;
    sqlGuild.prepare("INSERT INTO guilds VALUES (:guildId, :guildName, :creatorFbId, :createdAt)");
    sqlMapping.prepare("INSERT INTO account_guild_mappings VALUES (:accountId, :guildId)");

    for (int selectedAccountId : selectedAccountIds) {
        CastleAgeHttpClient client(selectedAccountId);
        QByteArray keepPage = client.post_sync("keep.php");
        if (keepPage.isEmpty())
            continue;

        GumboOutput *output = gumbo_parse(keepPage.data());

        GumboNode *span = nullptr;
        auto find_span_with_title = [&] (GumboNode *node) {
            if (!node || node->type != GUMBO_NODE_ELEMENT)
                return false;
            GumboAttribute *attr = nullptr;
            if (node->v.element.tag == GUMBO_TAG_SPAN
                    && (attr = gumbo_get_attribute(&node->v.element.attributes, "title"))
                    && endsWith(attr->value, "'s Guild")) {
                span = node;
                return true;
            }
            return false;
        };
        travelTree(output->root, find_span_with_title);

        const char *pGuildId = nullptr;
        const char *pGuildName = nullptr;
        if (span != nullptr) {
            auto find_a_href = [&] (GumboNode *node) {
                if (!node || node->type != GUMBO_NODE_ELEMENT)
                    return false;
                GumboAttribute *attr;
                if (node->v.element.tag == GUMBO_TAG_A
                        && (attr = gumbo_get_attribute(&node->v.element.attributes, "href"))
                        && startsWith(attr->value, "guildv2_home.php?guild_id=")) {
                    pGuildId = &attr->value[strlen("guildv2_home.php?guild_id=")];
                    if (node->v.element.children.length == 1) {
                        GumboNode *firstChild = static_cast<GumboNode *>(node->v.element.children.data[0]);
                        if (firstChild->type == GUMBO_NODE_TEXT) {
                            pGuildName = firstChild->v.text.text;
                        }
                    }
                    return true;
                }
                return false;
            };
            travelTree(span, find_a_href);
        }

        if (pGuildId != nullptr && pGuildName != nullptr) {
            QString guildId = QString::fromUtf8(pGuildId);
            QString guildName = QString::fromUtf8(pGuildName);

            sqlGuild.bindValue(":guildId", guildId);
            sqlGuild.bindValue(":guildName", guildName);
            QStringList l = guildId.split("_");
            sqlGuild.bindValue(":creatorFbId", l[0]);
            sqlGuild.bindValue(":createdAt", l[1]);

            sqlMapping.bindValue(":accountId", selectedAccountId);
            sqlMapping.bindValue(":guildId", guildId);

            if (sqlGuild.exec() && sqlMapping.exec())
                refreshTable();
        } else {
            qDebug() << "Checking if account doesn't join any guild.";

            GumboNode *node_a_guild_list = nullptr;
            auto find_a_href_guild_list = [&] (GumboNode *node) {
                if (!node || node->type != GUMBO_NODE_ELEMENT)
                    return false;
                GumboAttribute *attr = nullptr;
                if (node->v.element.tag == GUMBO_TAG_A
                        && (attr = gumbo_get_attribute(&node->v.element.attributes, "href"))
                        && !strcmp(attr->value, "guildv2_list.php")) {
                    node_a_guild_list = node;
                    return true;
                }
                return false;
            };
            travelTree(output->root, find_a_href_guild_list);

            if (node_a_guild_list) {
                GumboNode *node_img_alt_join_guild = nullptr;
                auto find_img_alt_join_guild = [&] (GumboNode * node) {
                    if (!node || node->type != GUMBO_NODE_ELEMENT)
                        return false;
                    GumboAttribute *attr = nullptr;
                    if (node->v.element.tag == GUMBO_TAG_IMG
                            && (attr = gumbo_get_attribute(&node->v.element.attributes, "alt"))
                            && startsWith(attr->value, "Join A Guild")) {
                        node_img_alt_join_guild = node;
                        return true;
                    }
                    return false;
                };
                travelChildren(node_a_guild_list, find_img_alt_join_guild);

                if (node_img_alt_join_guild) {
                    sqlMapping.bindValue(":accountId", selectedAccountId);
                    sqlMapping.bindValue(":guildId", "");
                    if (sqlMapping.exec())
                        refreshTable();
                }
            }
        }

        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
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

QList<int> AccountManager::selectedAccountIds()
{
    QList<int> selectedAccountIds;
    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();
    for (QModelIndex index : selectionIndexList) {
        int accountId = mModel->data(mModel->index(index.row(), 0)).toInt();
        if (selectedAccountIds.contains(accountId))
            continue;
        selectedAccountIds << accountId;
    }
    return selectedAccountIds;
}

void AccountManager::refreshTable()
{
    if (mModel != nullptr)
        mModel->setQuery(mModel->query().executedQuery());
    else {
        ((QSqlQueryModel *) ui->tableView->model())->setQuery(((QSqlQueryModel *)ui->tableView->model())->query().executedQuery());
    }
}

// ----------------------------------------------------------------
AccountModel::AccountModel(QWidget *parent) :
    QSqlQueryModel(parent)
{
    QString sql;
    sql.append("SELECT a._id AS 'Id', a.email AS 'Email address', i.ign AS 'In game name', f.fbid AS 'Facebook id', g.name AS 'Guild', r.role AS 'Role' FROM accounts AS a ");
    sql.append("LEFT JOIN igns AS i ON i.accountId = a._id ");
    sql.append("LEFT JOIN fbids AS f ON f.accountId = a._id ");
    sql.append("LEFT JOIN account_guild_mappings AS m ON m.accountId == a._id ");
    sql.append("LEFT JOIN guilds AS g ON m.guildId = g._id ");
    sql.append("LEFT JOIN roles AS r ON r.accountId = a._id ");
    sql.append("ORDER BY a.sequence");
    this->setQuery(sql);
}

AccountModel::~AccountModel()
{

}
