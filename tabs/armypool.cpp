#include <QtSql>
#include "armypool.h"
#include "ui_armypool.h"
#include "gaehttpclient.h"
#include "castleagehttpclient.h"
#include "gumbo.h"

static bool contains(const char *_big, const char *_little) {
    size_t big_length = strlen(_big);
    size_t little_length = strlen(_little);
    return (little_length != 0) && (big_length >= little_length) && (strstr(_big, _little) != nullptr);
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

// -----------------------------------------------------------------------------------------ß
class ArmyPoolModel : public QSqlQueryModel
{
public:
    ArmyPoolModel(QObject *parent = Q_NULLPTR)
        : QSqlQueryModel(parent)
    {
        QString sql;
        sql.append("SELECT p.armyCode AS 'Army Code', p.fbId AS 'Facebook Id', m.ign AS 'IGN' FROM acapPool AS p ");
        sql.append("LEFT JOIN fbid2igns AS m ON m.fbId = p.fbId");
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

void ArmyPool::on_actionUpdateIGN_triggered()
{
    QList<QString> selectedFbIds;
    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();
    QModelIndexList selectedRows = selectionModel->selectedRows(1);
    for (QModelIndex selectedRowModelIndex : selectedRows) {
        selectedFbIds << ui->tableView->model()->data(selectedRowModelIndex).toString();
    }

    QSqlQuery q;
    q.prepare("INSERT INTO fbid2igns VALUES (:fbId, :ign)");
    CastleAgeHttpClient client(1);
    QVector<QPair<QString,QString>> qs;

    for (QString fbid : selectedFbIds) {
        qs.clear();
        qs.push_back({"user", fbid});

        QByteArray response = client.get_sync("keep.php", qs);
        if (response.isEmpty())
            continue;

        GumboOutput *output = gumbo_parse(response.data());

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

        if (pIgn != nullptr && (strlen(pIgn) > 0)) {
            //qDebug() << "IGN" << QString::fromUtf8(pIgn);
            q.bindValue(":fbId", fbid);
            q.bindValue(":ign", QString::fromUtf8(pIgn));
            if (q.exec())
                ((QSqlQueryModel *)ui->tableView->model())->setQuery(((QSqlQueryModel *)ui->tableView->model())->query().executedQuery());
        }

        gumbo_destroy_output(&kGumboDefaultOptions, output);;
    }
}
