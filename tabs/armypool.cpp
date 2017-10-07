#include <QtSql>
#include "armypool.h"
#include "ui_armypool.h"
#include "gaehttpclient.h"

class ArmyPoolModel : public QSqlQueryModel
{
public:
    ArmyPoolModel(QObject *parent = Q_NULLPTR)
        : QSqlQueryModel(parent)
    {
        QString sql;
        sql.append("SELECT armyCode AS 'Army Code', fbId AS 'Facebook Id' FROM acapPool");
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
        q.exec("DELETE FROM acapPool");
        qDebug() << "Prepare to recreate army pool, total" << pool.size();
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
