#include <QtSql>
#include "armycodeannounceplan.h"
#include "ui_armycodeannounceplan.h"
#include "castleagehttpclient.h"
#include "gaehttpclient.h"
#include "qgumboparser.h"

class ArmyMemberModel : public QSqlQueryModel
{
public:
    ArmyMemberModel(const int accountId, QWidget *parent = nullptr) : QSqlQueryModel(parent) {
        QString sql;
        sql.append("SELECT ");
        sql.append("o.fbId AS 'Facebook Id'");
        sql.append(", a.level AS 'Level'");
        sql.append(", a.ign AS 'IGN' ");
        sql.append("FROM ownedArmies AS o ");
        sql.append("LEFT JOIN fbIdAccounts AS a ON o.fbId = a.fbId ");
        sql.append("WHERE o.accountId = %1 ");
        sql.append("ORDER BY length(o.fbId) ASC, o.fbId ASC"); // this is same order as members in ca army page.
        this->setQuery(sql.arg(accountId));
    }

    ~ArmyMemberModel() {}
};

// ----------------------------------------------------------------------
class ACAPModel : public QSqlQueryModel
{
public:
    ACAPModel(QWidget *parent) : QSqlQueryModel(parent) {
        QString sql;
        sql.append("SELECT a._id AS 'Id'");
        //sql.append(", a.email AS 'Email'");
        //sql.append(", IFNULL(i.ign, 'UnknownIGN') AS 'IGN'");
        sql.append(", (IFNULL(i.ign, 'UnknownIGN') || ' - ' || a.email) AS 'IGN & Email'");
        sql.append(", c.armyCode AS 'Army Code'");
        sql.append(", f.fbId AS 'Facebook Id'");
        sql.append(", datetime(MAX(t.announceTimestamp), 'localtime') AS 'Announce Time' ");
        sql.append("FROM accounts AS a ");
        sql.append("LEFT JOIN igns AS i ON i.accountId = a._id ");
        sql.append("LEFT JOIN armycodes AS c ON c.accountId = a._id ");
        sql.append("LEFT JOIN fbids AS f ON f.accountId = a._id ");
        sql.append("LEFT JOIN acapTimestamps AS t ON t.accountId = a._id ");
        sql.append("GROUP BY a._id ");
        sql.append("ORDER BY announceTimestamp ASC");
        this->setQuery(sql);
    }

    ~ACAPModel() {

    }

public:
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (index.column() == 4 && role == Qt::BackgroundColorRole) {
            QDateTime previous = QDateTime::fromString(QSqlQueryModel::data(index, Qt::DisplayRole).toString(), Qt::ISODate); // display timestamp is localtime.
            QDateTime now = QDateTime::currentDateTime();
            if (previous.addDays(6) <= now)
                return QVariant(QColor(Qt::yellow));
            if (previous.addDays(7) <= now)
                return QVariant(QColor(Qt::red));
        }
        return QSqlQueryModel::data(index, role);
    }
};

// --------------------------------------------------------------------------------------------------
ArmyCodeAnnouncePlan::ArmyCodeAnnouncePlan(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ArmyCodeAnnouncePlan)
{
    ui->setupUi(this);

    ui->tableViewAccounts->setModel(new ACAPModel(ui->tableViewAccounts));
    ui->tableViewAccounts->setColumnHidden(0, true);
    ui->tableViewAccounts->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //ui->tableView->resizeRowsToContents();

    connect(ui->tableViewAccounts->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &ArmyCodeAnnouncePlan::onAccountTableCurrentRowChanged);

    ui->tableViewArmyMembers->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

ArmyCodeAnnouncePlan::~ArmyCodeAnnouncePlan()
{
    delete ui;
}

void ArmyCodeAnnouncePlan::onAccountTableCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    int accountId = ui->tableViewAccounts->model()->data(ui->tableViewAccounts->model()->index(current.row(), 0)).toInt();
    if (accountId >= 0)
        activeAccountChanged(accountId);
    else
        qWarning() << "invalid account id selected";
}

void ArmyCodeAnnouncePlan::activeAccountChanged(const int accountId)
{
    QAbstractItemModel *m = ui->tableViewArmyMembers->model();
    if (m != nullptr)
        m->deleteLater();
    ui->tableViewArmyMembers->setModel(new ArmyMemberModel(accountId));
}

void ArmyCodeAnnouncePlan::refreshAccountTable()
{
    QSqlQueryModel *m = static_cast<QSqlQueryModel *>(ui->tableViewAccounts->model());
    if (m)
        m->setQuery(m->query().executedQuery());
}

void ArmyCodeAnnouncePlan::on_actionUpdateArmyCode_triggered()
{
    /* only update accounts with empty army code. */
    QList<int> emptyArmyCodeAccountIds;
    QSqlQueryModel *m = static_cast<QSqlQueryModel *>(ui->tableViewAccounts->model());
    for (int row = 0; row < m->rowCount(); row++) {
        QString armyCode = m->data(m->index(row, 2)).toString();
        if (armyCode.isEmpty())
            emptyArmyCodeAccountIds << m->data(m->index(row, 0)).toInt();
    }

    qDebug() << "Required to update army code accounts:" << emptyArmyCodeAccountIds.size();

    /* check army code from CA server. */
    QRegularExpression pattern("Your army code:\\s+([0-9A-F]{6,})");
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO armycodes VALUES (:accountId, :armyCode);");
    bool any_updated = false;
    for (int accountId : emptyArmyCodeAccountIds) {
        CastleAgeHttpClient client(accountId);
        QByteArray response = client.post_sync("army.php");

        QRegularExpressionMatch m = pattern.match(response);
        if (m.hasMatch()) {
            q.bindValue(":accountId", accountId);
            q.bindValue(":armyCode", m.captured(1));
            any_updated |= q.exec();
        }
    }

    /* if any update successful, refresh */
    if (any_updated) {
        refreshAccountTable();
    }
}

void ArmyCodeAnnouncePlan::on_actionUpdateFacebookId_triggered()
{
    /* only update accounts with empty facebook id */
    QList<int> emptyFacebookIdAccountIds;
    QSqlQueryModel *m = static_cast<QSqlQueryModel *>(ui->tableViewAccounts->model());
    for (int row = 0; row < m->rowCount(); row++) {
        QString facebookId = m->data(m->index(row, 3)).toString();
        if (facebookId.isEmpty())
            emptyFacebookIdAccountIds << m->data(m->index(row, 0)).toInt();
    }

    qDebug() << "Required to update facebook id accounts:" << emptyFacebookIdAccountIds.size();

    /* check facebook id from CA server. */  /* facebook id may also be found from cookie. */
    QRegularExpression pattern("a href=\\\"keep\\.php\\?user=([0-9]+)\\\"");
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO fbids VALUES (:accountId, :fbId);");
    bool any_updated = false;
    for (int accountId : emptyFacebookIdAccountIds) {
        CastleAgeHttpClient client(accountId);
        QByteArray response = client.post_sync("keep.php");

        QRegularExpressionMatch m = pattern.match(response);
        if (m.hasMatch()) {
            q.bindValue(":accountId", accountId);
            q.bindValue(":fbId", m.captured(1));
            any_updated |= q.exec();
        }
    }

    /* if any update successful, refresh */
    if (any_updated) {
        refreshAccountTable();
    }
}

void ArmyCodeAnnouncePlan::on_actionAnnounce_triggered()
{
    GAEHttpClient *gaeHttpClient = new GAEHttpClient(this);
    QSqlQuery q;
    q.prepare("INSERT OR IGNORE INTO acapTimestamps VALUES (:accountId, :announceTimestamp)");

    QList<QPair<QString, QString>> d;
    /* find out selected accountId from QItemSelectionModel. */
    QList<int> selectedAccountIds;
    QItemSelectionModel *selectionModel = ui->tableViewAccounts->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();
    for (QModelIndex index : selectionIndexList) {
        int accountId = ui->tableViewAccounts->model()->data(ui->tableViewAccounts->model()->index(index.row(), 0)).toInt();
        if (selectedAccountIds.contains(accountId))
            continue;

        QString armyCode = ui->tableViewAccounts->model()->data(ui->tableViewAccounts->model()->index(index.row(), 2)).toString();
        QString facebookId = ui->tableViewAccounts->model()->data(ui->tableViewAccounts->model()->index(index.row(), 3)).toString();
        if (armyCode.isEmpty() || facebookId.isEmpty())
            continue;

        selectedAccountIds << accountId;
        d << QPair<QString, QString>(armyCode, facebookId);
    }

    /* do nothing if no selected */
    if (selectedAccountIds.size() == 0)
        return;

    /* announce & update database & refresh ui */
    for (int i = 0; i < selectedAccountIds.size(); i++) {
        QDateTime announceTimestamp = gaeHttpClient->acap_announce(d.at(i).first, d.at(i).second);
        qDebug() << "Announce datetime:" << announceTimestamp.toString(Qt::ISODateWithMs); // datetime in 8601 format. Ready to insert into database.
        if (announceTimestamp.isValid()) {
            q.bindValue(":accountId", selectedAccountIds.at(i));
            q.bindValue(":announceTimestamp", announceTimestamp.toString(Qt::ISODateWithMs));
            if (q.exec()) {
                refreshAccountTable();
            }
        }
    }

}

void ArmyCodeAnnouncePlan::on_actionDownloadArmyMembers_triggered()
{
    QList<int> accountIds;
    for (QModelIndex rowItemIndex : ui->tableViewAccounts->selectionModel()->selectedRows()) {
        accountIds << ui->tableViewAccounts->model()->data(rowItemIndex).toInt();
    }

    QSqlQuery q;
    q.prepare("INSERT INTO ownedArmies VALUES (:accountId, :fbId)");

    for (int accountId : accountIds) {
        CastleAgeHttpClient client(accountId);

        int last_page = -1;
        int current_page = 1;
        QVector<QPair<QString, QString>> form;
        QVector<QPair<QString, QString>> qs;

        do {
            qs.clear();
            qs.append({"page", QString::number(current_page)});

            QByteArray response = client.post_sync("army_member.php", form, qs);
            if (response.isEmpty())
                continue;

            QGumboParser gumbo(response.data());

            GumboNode *td = gumbo.findNode(GUMBO_TAG_TD, "style", "tg_bg_center.jpg", Contains);

            // update last_page when invalid
            if (last_page == -1) {
                QList<GumboNode *> pages = gumbo.findNodes(td, GUMBO_TAG_A, "href", "army_member.php?page=", StartsWith);
                if (pages.size() > 0) {
                    QString last_page_link = QString::fromUtf8(gumbo.attributeValue(pages.last(), "href"));
                    last_page = last_page_link.mid(strlen("army_member.php?page=")).toInt();
                }
            }

            // collect army members in this page and update database...
            bool needRefresh = false;
            QList<QString> profileChecked;
            QList<GumboNode *> army_profiles = gumbo.findNodes(td, GUMBO_TAG_A, "href", "keep.php?casuser=", StartsWith);
            for (GumboNode *profile : army_profiles) {
                // note: the nodes are repeated 4 times, so ignore the same attributes
                QString href_profile = QString::fromUtf8(gumbo.attributeValue(profile, "href"));
                if (profileChecked.contains(href_profile))
                    continue;
                profileChecked << href_profile;
                QString army_member_fbid = href_profile.mid(strlen("keep.php?casuser=")) ;

                if (!army_member_fbid.isEmpty()) {
                    q.bindValue(":accountId", accountId);
                    q.bindValue(":fbId", army_member_fbid);
                    if (q.exec())
                        needRefresh = true;
                }
            }

            if (needRefresh)
                activeAccountChanged(accountId);

        } while (current_page++ < last_page);
    }
}
