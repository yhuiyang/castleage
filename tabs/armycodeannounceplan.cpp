#include <QtSql>
#include "armycodeannounceplan.h"
#include "ui_armycodeannounceplan.h"
#include "castleagehttpclient.h"
#include "gaehttpclient.h"

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
};

// --------------------------------------------------------------------------------------------------
ArmyCodeAnnouncePlan::ArmyCodeAnnouncePlan(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ArmyCodeAnnouncePlan)
{
    ui->setupUi(this);

    ui->tableView->setModel(new ACAPModel(ui->tableView));
    ui->tableView->setColumnHidden(0, true);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //ui->tableView->resizeRowsToContents();
}

ArmyCodeAnnouncePlan::~ArmyCodeAnnouncePlan()
{
    delete ui;
}

void ArmyCodeAnnouncePlan::on_actionUpdateArmyCode_triggered()
{
    /* find out selected accountId from QItemSelectionModel. */
    QList<int> selectedAccountIds;
    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();
    for (QModelIndex index : selectionIndexList) {
        int accountId = ui->tableView->model()->data(ui->tableView->model()->index(index.row(), 0)).toInt();
        if (selectedAccountIds.contains(accountId))
            continue;
        selectedAccountIds << accountId;
    }

    // if none selected, treat this case as all accounts are selected.
    if (selectedAccountIds.size() == 0) {
        int rowCount = ui->tableView->model()->rowCount();
        for (int row = 0; row < rowCount; row++) {
            selectedAccountIds << ui->tableView->model()->data(ui->tableView->model()->index(row, 0)).toInt();
        }
    }

    QRegularExpression pattern("Your army code:\\s+([0-9A-F]{6,})");
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO armycodes VALUES (:accountId, :armyCode);");
    CastleAgeHttpClient *caHttpClient = new CastleAgeHttpClient(0);
    bool any_updated = false;
    for (int accountId : selectedAccountIds) {
        caHttpClient->switchAccount(accountId);
        QByteArray response = caHttpClient->post_sync("army.php");

        QRegularExpressionMatch m = pattern.match(response);
        if (m.hasMatch()) {
            q.bindValue(":accountId", accountId);
            q.bindValue(":armyCode", m.captured(1));
            any_updated |= q.exec();
        }
    }

    /* if any update successful, refresh */
    if (any_updated) {
        ((QSqlQueryModel *) ui->tableView->model())->setQuery(((QSqlQueryModel *)ui->tableView->model())->query().executedQuery());
    }
}

void ArmyCodeAnnouncePlan::on_actionUpdateFacebookId_triggered()
{
    /* find out selected accountId from QItemSelectionModel. */
    QList<int> selectedAccountIds;
    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();
    for (QModelIndex index : selectionIndexList) {
        int accountId = ui->tableView->model()->data(ui->tableView->model()->index(index.row(), 0)).toInt();
        if (selectedAccountIds.contains(accountId))
            continue;
        selectedAccountIds << accountId;
    }

    // if none selected, treat this case as all accounts are selected.
    if (selectedAccountIds.size() == 0) {
        int rowCount = ui->tableView->model()->rowCount();
        for (int row = 0; row < rowCount; row++) {
            selectedAccountIds << ui->tableView->model()->data(ui->tableView->model()->index(row, 0)).toInt();
        }
    }

    QRegularExpression pattern("a href=\\\"keep\\.php\\?user=([0-9]+)\\\"");
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO fbids VALUES (:accountId, :fbId);");
    CastleAgeHttpClient *caHttpClient = new CastleAgeHttpClient(0, this);
    bool any_updated = false;
    for (int accountId : selectedAccountIds) {
        caHttpClient->switchAccount(accountId);
        QByteArray response = caHttpClient->post_sync("keep.php");

        QRegularExpressionMatch m = pattern.match(response);
        if (m.hasMatch()) {
            q.bindValue(":accountId", accountId);
            q.bindValue(":fbId", m.captured(1));
            any_updated |= q.exec();
        }
    }

    /* if any update successful, refresh */
    if (any_updated) {
        ((QSqlQueryModel *) ui->tableView->model())->setQuery(((QSqlQueryModel *)ui->tableView->model())->query().executedQuery());
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
    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();
    for (QModelIndex index : selectionIndexList) {
        int accountId = ui->tableView->model()->data(ui->tableView->model()->index(index.row(), 0)).toInt();
        if (selectedAccountIds.contains(accountId))
            continue;

        QString armyCode = ui->tableView->model()->data(ui->tableView->model()->index(index.row(), 2)).toString();
        QString facebookId = ui->tableView->model()->data(ui->tableView->model()->index(index.row(), 3)).toString();
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
                ((QSqlQueryModel *) ui->tableView->model())->setQuery(((QSqlQueryModel *)ui->tableView->model())->query().executedQuery());
            }
        }
    }

}
