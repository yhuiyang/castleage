#include <QtSql>
#include "armypool.h"
#include "ui_armypool.h"
#include "gaehttpclient.h"
#include "castleagehttpclient.h"
#include "qgumboparser.h"

// -----------------------------------------------------------------------------------------ß
class ArmyPoolModel : public QSqlQueryModel
{
public:
    ArmyPoolModel(QObject *parent = Q_NULLPTR)
        : QSqlQueryModel(parent)
    {
        QString sql;
        sql.append("SELECT p.armyCode AS 'Army Code', p.fbId AS 'Facebook Id', a.ign AS 'IGN', a.level AS 'Level' FROM acapPool AS p ");
        sql.append("LEFT JOIN fbidAccounts AS a ON a.fbId = p.fbId");
        this->setQuery(sql);
    }
    ~ArmyPoolModel() {}
};

// ----------------------------------------------------------------
ArmyPool::ArmyPool(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ArmyPool)
{
    ui->setupUi(this);

    ui->tableView->setModel(new ArmyPoolModel(ui->tableView));
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //ui->tableView->resizeRowsToContents();
}

ArmyPool::~ArmyPool()
{
    delete ui;
}

void ArmyPool::on_actionDownloadArmy_triggered()
{
    QSqlQuery q;

    GAEHttpClient client;
    auto pool = client.acap_download_armypool();

    if (pool.size() > 0) {
        qDebug() << "Prepare to recreate army pool, total" << pool.size();
        if (!q.exec("DELETE FROM acapPool"))
            qWarning() << "Database Error:" << q.lastError().databaseText();
    }
    q.prepare("INSERT INTO acapPool VALUES (:armyCode, :fbId)");
    bool any = false;
    for (auto d : pool) {
        q.bindValue(":armyCode", d.first);
        q.bindValue(":fbId", d.second);
        any |= q.exec();
    }

    if (any)
        ((QSqlQueryModel *) ui->tableView->model())->setQuery(((QSqlQueryModel *)ui->tableView->model())->query().executedQuery());
}

void ArmyPool::on_actionUpdateIgnLvl_triggered()
{
    /* only update ign missed accounts */
    QList<QString> ignMissedFbIds;
    for (int row = 0; row < ui->tableView->model()->rowCount(); row++) {
        QAbstractItemModel *m = ui->tableView->model();
        QString ign = m->data(m->index(row, 2)).toString();
        if (ign.isEmpty())
            ignMissedFbIds << ui->tableView->model()->data(ui->tableView->model()->index(row, 1)).toString();
    }

    /* use first account to query accounts from ca server */
    int observerAccountId = 0;
    QSqlQuery q0("SELECT _id FROM accounts LIMIT 1");
    if (q0.exec() && q0.first())
        observerAccountId = q0.value(0).toInt();
    if (observerAccountId == 0) {
        qWarning() << "No observer available!";
        return;
    }
    CastleAgeHttpClient client(observerAccountId);

    QSqlQuery q;
    q.prepare("INSERT INTO fbidAccounts VALUES (:fbId, :ign, :level)");
    QVector<QPair<QString,QString>> qs;

    QRegularExpression pattern("Level ([0-9]+) -");
    for (QString fbId : ignMissedFbIds) {
        qs.clear();
        qs.push_back({"user", fbId});

        QByteArray response = client.get_sync("keep.php", qs);
        if (response.isEmpty())
            continue;

        QGumboParser gumbo(response.data());

        QString ign, levelText;
        int level = 0;
        GumboNode *div_bg = gumbo.findNode(GUMBO_TAG_DIV, "style", "keep_top.jpg", Contains);
        GumboNode *div_ign = gumbo.findNode(div_bg, GUMBO_TAG_DIV, "title", nullptr, Exists);
        const char *title = gumbo.attributeValue(div_ign, "title");
        if (title != nullptr)
            ign = QString::fromUtf8(title);

        QList<GumboNode *> divs_style = gumbo.findNodes(div_bg, GUMBO_TAG_DIV, "style", nullptr, Exists);
        if (divs_style.size() > 0) {
            levelText = gumbo.textContent(divs_style.last()); // original level text = "\n        Level XX - battle rank name     "
            QRegularExpressionMatch m = pattern.match(levelText);
            if (m.hasMatch())
                level = m.captured(1).toInt();
        }

        if (!ign.isEmpty()) {
            q.bindValue(":fbId", fbId);
            q.bindValue(":ign", ign);
            q.bindValue(":level", level);
            if (q.exec()) {
                QSqlQueryModel *m = static_cast<QSqlQueryModel *>(ui->tableView->model());
                m->setQuery(m->query().executedQuery());
            }
        }
    }
}
