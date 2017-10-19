#include <QDebug>
#include <QMessageBox>
#include <string.h>
#include "accountmanager.h"
#include "ui_accountmanager.h"
#include "addaccountdialog.h"
#include "updateaccountdialog.h"
#include "castleagehttpclient.h"
#include "gumbo.h"
#include "qgumboparser.h"

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

QString whereami(GumboNode *node) {
    QString out;
    if (!node || !node->parent)
        return out;

    if (node->parent->parent) {
        out += QString::number(node->index_within_parent, 10);
        out += "->";
    } else {
        out += "root";
    }

    return out + whereami(node->parent);
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
        if (keepPage.isEmpty())
            continue;

        // note: we can not only find <a href="keep.php?user=..." ...>
        // because when someone leave message on the wall, that will be first matched.
        // need to check if there is <img> child or not...
        // <img title="view your stats and use the treasury on this page" src="....tab_stats_on.gif">
        QGumboParser gumbo(keepPage.data());

        QList<GumboNode *> keepLinks = gumbo.findNodes(GUMBO_TAG_A, "href", "keep.php?user=", StartsWith);
        for (GumboNode *keepLink : keepLinks) {
            GumboNode *img = gumbo.findNode(keepLink, GUMBO_TAG_IMG, "src", "tab_stats_on.gif", EndsWith);
            if (img != nullptr) {
                QString fbid = QString::fromUtf8(gumbo.attributeValue(keepLink, "href")).mid(strlen("keep.php?user="));

                q.bindValue(":accountId", selectedAccountId);
                q.bindValue(":fbid", fbid);
                if (q.exec())
                    refreshTable();

                break;
            }
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
    //
    // Member html looks like
    // <div id="guildv2_list_body style=...>
    //    <div style=...>
    //        <div style=...>
    //            <div style=...>Do you want to leave this guild?</div>
    // ...
    //
    // Officer html looks like
    // <div id="guildv2_list_body style=...>
    //    <div style=...>
    //        <div style=...>
    //            [Guild Officer] Control Panel<br />
    //            (Only Guild Master and Officers have access to this Control Panel)
    //        </div>
    // ...
    //
    // Master html looks similar to Officer, but [Guild Officer] is changed to [Guild Master]
    //

    QList<int> accountIds = selectedAccountIds();
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO roles VALUES (:accountId, :role)");

    for (int accountId: accountIds) {
        CastleAgeHttpClient client(accountId);
        QByteArray page = client.post_sync("guildv2_panel.php");
        if (page.isEmpty())
            continue;

        GumboOutput *output = gumbo_parse(page.data());

        GumboNode *master_text = nullptr;
        GumboNode *officer_text = nullptr;
        GumboNode *member_text = nullptr;
        auto find_role = [&] (GumboNode *node) {
            if (!node || node->type != GUMBO_NODE_ELEMENT)
                return false;
            GumboAttribute *attr = nullptr;
            if (node->v.element.tag == GUMBO_TAG_DIV
                    && (attr = gumbo_get_attribute(&node->v.element.attributes, "id"))
                    && !strcmp(attr->value, "guildv2_list_body")) {
                GumboNode *div1 = nullptr;
                for (unsigned int c = 0; c < node->v.element.children.length; c++) {
                    GumboNode *div = static_cast<GumboNode *>(node->v.element.children.data[c]);
                    if (div && div->type == GUMBO_NODE_ELEMENT && div->v.element.tag == GUMBO_TAG_DIV) {
                        div1 = div;
                        break;
                    }
                }
                if (div1) {
                    GumboNode *div11 = nullptr;
                    for (unsigned int c = 0; c < div1->v.element.children.length; c++) {
                        GumboNode *div = static_cast<GumboNode *>(div1->v.element.children.data[c]);
                        if (div && div->type == GUMBO_NODE_ELEMENT && div->v.element.tag == GUMBO_TAG_DIV) {
                            div11 = div;
                            break;
                        }
                    }

                    if (div11) {
                        GumboNode *div111 = nullptr;
                        for (unsigned int c = 0; c < div11->v.element.children.length; c++) {
                            GumboNode *t = static_cast<GumboNode *>(div11->v.element.children.data[c]);
                            if (t && t->type == GUMBO_NODE_TEXT) {
                                //qDebug() << "Found role text" << QString::fromUtf8(t->v.text.text);
                                if (contains(t->v.text.text, "[Guild Master]")) {
                                    master_text = t;
                                    return true;
                                } else if (contains(t->v.text.text, "[Guild Officer]")) {
                                    officer_text = t;
                                    return true;
                                }
                            } else if (t && t->type == GUMBO_NODE_ELEMENT && t->v.element.tag == GUMBO_TAG_DIV) {
                                div111 = t;
                                break;
                            }
                        }

                        if (div111) {
                            //qDebug() << whereami(div111);
                            for (unsigned int c = 0; c < div111->v.element.children.length; c++) {
                                GumboNode *t = static_cast<GumboNode *>(div111->v.element.children.data[c]);
                                //qDebug() << QString::fromUtf8(t->v.text.text);
                                if (t && t->type == GUMBO_NODE_TEXT && contains(t->v.text.text, "Do you want to leave this guild?")) {
                                    member_text = t;
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            return false;
        };
        travelTree(output->root, find_role);
        if (master_text) {
            q.bindValue(":accountId", accountId);
            q.bindValue(":role", "Master");
        } else if (officer_text) {
            q.bindValue(":accountId", accountId);
            q.bindValue(":role", "Officer");
        } else if (member_text) {
        } else {
            qWarning() << "Unknown role.";
        }

        if (master_text || officer_text)
            if (q.exec())
                refreshTable();

        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
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
