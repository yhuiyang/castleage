#include <QDebug>
#include <QtSql>
#include "tagmanager.h"
#include "ui_tagmanager.h"
#include "inputdialog.h"

/*
 * Below models are used by internal only, so no need to put inside header file.
 * When put both declaration and definition in cpp file, don't use Q_OBJECT macro
 * since we don't use any singal/slot, or you will get some linker errors.
 * If put declaration in header file, Q_OBJECT will no effect.
 */
class TagListModel : public QSqlQueryModel
{
    // Q_OBJECT
public:
    explicit TagListModel(QWidget *parent = 0) :
        QSqlQueryModel(parent)
    {
        this->setQuery("SELECT _id, name FROM tags ORDER BY sequence");
    }

    ~TagListModel() {}
};

class AccountNotInTagModel : public QSqlQueryModel
{
    // Q_OBJECT
public:
    explicit AccountNotInTagModel(int tagId, QWidget *parent = 0) :
        QSqlQueryModel(parent)
    {
        QString sql;
        sql.append("SELECT a._id, ifnull(i.ign, 'UnknownIGN') || '\n' || a.email FROM accounts AS a ");
        sql.append("LEFT JOIN igns AS i ON i.accountId = a._id ");
        sql.append("WHERE a._id NOT IN (");
        sql.append("SELECT m.accountId FROM account_tag_mappings AS m ");
        sql.append("WHERE m.tagId = %1");
        sql.append(")");
        this->setQuery(sql.arg(tagId));
    }

    ~AccountNotInTagModel() {}
};

class AccountInTagModel : public QSqlQueryModel
{
    // Q_OBJECT
public:
    explicit AccountInTagModel(int tagId, QWidget *parent = 0) :
        QSqlQueryModel(parent)
    {
        QString sql;
        sql.append("SELECT m.accountId, IFNULL(i.ign, 'UnknownIGN') || '\n' || a.email FROM account_tag_mappings AS m ");
        sql.append("INNER JOIN accounts AS a ON a._id = m.accountId ");
        sql.append("LEFT JOIN igns AS i ON a._id = i.accountId ");
        sql.append("WHERE m.tagId = %1 ");
        sql.append("ORDER BY m.sequence");
        this->setQuery(sql.arg(tagId));
    }
    ~AccountInTagModel() {}
};

// -------------------------------------------------------------------------------------------------------

TagManager::TagManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TagManager),
    mSelectedTagId(0)
{
    ui->setupUi(this);

    ui->listViewTagList->setModel(new TagListModel(this));
    ui->listViewTagList->setModelColumn(1);

    /* handle current tag changed signal (no matter by mouse or keyboard) */
    connect(ui->listViewTagList->selectionModel(), &QItemSelectionModel::currentChanged, this, &TagManager::handleTagCurrentChanged);
}

TagManager::~TagManager()
{
    delete ui;
}

void TagManager::on_actionAddTag_triggered()
{
    InputDialog dlg(this, "new tag name");
    if (dlg.exec() == QDialog::Accepted) {
        QString input = dlg.getInput();
        if (!input.isEmpty()) {
            /* insert into database */
            QSqlQuery statement;

            statement.prepare("INSERT INTO tags (name, sequence) SELECT :name, IFNULL(max(sequence) + 1, 1) FROM tags");
            statement.bindValue(":name", input);

            if (statement.exec()) {
                TagListModel *model = static_cast<TagListModel *>(ui->listViewTagList->model());
                model->setQuery(model->query().executedQuery());
            } else {
                qWarning() << "Failed to insert tag into database. Reason:" << statement.lastError();
            }
        }
    }
}

void TagManager::handleTagCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    // Normally I don't need this because the current selection is highlighted correctly.
    // But, when initially no selection, press 'tab' key will move focus to this tag list,
    // and then there is selection but no highlight.
    // Use this method, setCurrentIndex, to gain the highlight on selection.
    ui->listViewTagList->setCurrentIndex(current);

    /* find out and update current selected tagId from model of listview. */
    TagListModel *model = static_cast<TagListModel *>(ui->listViewTagList->model());
    mSelectedTagId = model->data(model->index(current.row(), 0)).toInt();

    /* refresh both in tag and not in tag account list. */
    refreshNotInSelectedTagAccountList();
    refreshInSelectedTagAccountList();
}

void TagManager::refreshInSelectedTagAccountList()
{
    QAbstractItemModel *old = ui->listViewAccountInTag->model();
    if (old != nullptr) {
        old->deleteLater();
        old = nullptr;
    }
    ui->listViewAccountInTag->setModel(new AccountInTagModel(mSelectedTagId, this));
    ui->listViewAccountInTag->setModelColumn(1);
}

void TagManager::refreshNotInSelectedTagAccountList()
{
    QAbstractItemModel *old = ui->listViewAccountNotInTag->model();
    if (old != nullptr) {
        old->deleteLater();
        old = nullptr;
    }
    ui->listViewAccountNotInTag->setModel(new AccountNotInTagModel(mSelectedTagId, this));
    ui->listViewAccountNotInTag->setModelColumn(1);
}

void TagManager::on_pushButtonAddTag_clicked()
{
    QItemSelectionModel *selectionModel = ui->listViewAccountNotInTag->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();

    if (selectionIndexList.count() == 0)
        return;

    /* Use index of selection model to retrieve selected accountIds from listView model */
    QAbstractItemModel *itemModel = ui->listViewAccountNotInTag->model();
    QVariantList selectedAccountIds;
    for (QModelIndex index : selectionIndexList)
        selectedAccountIds << itemModel->data(itemModel->index(index.row(), 0)).toInt();

    /* insert into database */
    QString sql;
    sql.append("INSERT INTO account_tag_mappings (tagId, accountId, sequence) ");
    sql.append("SELECT %1, ?, IFNULL(max(sequence)+1, 1) FROM account_tag_mappings WHERE tagId = %1");
    QSqlQuery statement;
    statement.prepare(sql.arg(mSelectedTagId));
    statement.addBindValue(selectedAccountIds);
    if (statement.execBatch()) {
        refreshInSelectedTagAccountList();
        refreshNotInSelectedTagAccountList();
    } else {
        qDebug() << statement.lastError();
    }
}

void TagManager::on_pushButtonRemoveTag_clicked()
{
    QItemSelectionModel *selectionModel = ui->listViewAccountInTag->selectionModel();
    QModelIndexList selectionIndexList = selectionModel->selection().indexes();

    if (selectionIndexList.count() == 0)
        return;

    /* Use index of selection model to retrieve selection accountId from listview model */
    QAbstractItemModel *itemModel = ui->listViewAccountInTag->model();
    QVariantList selectedAccountIds;
    for (QModelIndex index : selectionIndexList)
        selectedAccountIds << itemModel->data(itemModel->index(index.row(), 0)).toInt();

    /* remove from database */
    QString sql;
    sql.append("DELETE FROM account_tag_mappings WHERE tagId = %1 AND accountId = ?");
    QSqlQuery statement;
    statement.prepare(sql.arg(mSelectedTagId));
    statement.addBindValue(selectedAccountIds);
    if (statement.execBatch()) {
        refreshInSelectedTagAccountList();
        refreshNotInSelectedTagAccountList();
    } else {
        qDebug() << statement.lastError();
    }
}
