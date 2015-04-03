#include <QStyledItemDelegate>
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QStateMachine>
#include "batchactiondialog.h"
#include "ui_batchactiondialog.h"


#define BATCH_ACTION_TABLE_DISABLED_EDIT_TRIGGER (QAbstractItemView::NoEditTriggers)
#define BATCH_ACTION_TABLE_ENABLED_EDIT_TRIGGER (QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed | QAbstractItemView::EditKeyPressed)

enum BatchActionTableColumns
{
    Who,
    DoWhat,
    Parameter,
};

enum class BatchAction {
    SwitchLoadout,
    DemiPower,
    ItemArchive,
    ResourcesCollect,
    PrayCrystal,
    BattleRewardCollect,
};

const struct ActionDescription {
    BatchAction act;
    QString desc;
} ACTION_DESCRIPTIONS[] = {
    { BatchAction::SwitchLoadout,       "SwitchLoadout"             },
    { BatchAction::DemiPower,           "Demi-Power Blesssing"      },
    { BatchAction::ItemArchive,         "Item Archive Bonus"        },
    { BatchAction::ResourcesCollect,    "Collect Daily Resources"   },
    { BatchAction::PrayCrystal,         "Pray for Crystal"          },
    { BatchAction::BattleRewardCollect, "Collect Battle Reward"     },
};

//////////////////////////////////////////////////////////////////////////////////////////
// Batch Action Table Item Delegate
//////////////////////////////////////////////////////////////////////////////////////////
BatchActionTableItemDelegate::BatchActionTableItemDelegate(bool preferredIgn, QObject *parent)
    : QStyledItemDelegate(parent),
      mPreferredIgn(preferredIgn)
{
    QSqlQuery q;
    q.exec("SELECT a.id, a.email, s.inGameName FROM Accounts AS a LEFT OUTER JOIN Stats AS s ON s.accountId = a.id ORDER BY a.timestamp");
    while (q.next())
    {
        mAccountList[AccountIdx::Id] << q.value("id").toString();
        mAccountList[AccountIdx::Email] << q.value("email").toString();
        mAccountList[AccountIdx::IGN] << q.value("inGameName").toString();
    }
}

QWidget *BatchActionTableItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QComboBox *combo;
    int batchActionCount, accountCount;

    int column = index.column();
    switch (column)
    {
    case BatchActionTableColumns::Who:
        combo = new QComboBox(parent);
        accountCount = mAccountList[AccountIdx::Id].size();
        for (int i = 0; i < accountCount; i++)
            combo->addItem((mPreferredIgn && !mAccountList[AccountIdx::IGN].at(i).isEmpty()) ? mAccountList[AccountIdx::IGN].at(i) : mAccountList[AccountIdx::Email].at(i), mAccountList[AccountIdx::Id].at(i).toLongLong());
        return combo;
    case BatchActionTableColumns::DoWhat:
        combo = new QComboBox(parent);
        batchActionCount = sizeof(ACTION_DESCRIPTIONS) / sizeof(ACTION_DESCRIPTIONS[0]);
        for (int i = 0; i < batchActionCount; i++)
            combo->addItem(ACTION_DESCRIPTIONS[i].desc, static_cast<qlonglong>(ACTION_DESCRIPTIONS[i].act));
        return combo;
    default:
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void BatchActionTableItemDelegate::setEditorData (QWidget *editor, const QModelIndex &index) const
{
    if (QComboBox *cb = qobject_cast<QComboBox *>(editor))
    {
        // get the index of the text in the combobox that matches the current value of the itenm
        QString currentText = index.data(Qt::EditRole).toString();
        int cbIndex = cb->findText(currentText);
        // if it is valid, adjust the combobox
        if (cbIndex >= 0)
            cb->setCurrentIndex(cbIndex);
    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void BatchActionTableItemDelegate::setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (QComboBox *cb = qobject_cast<QComboBox *>(editor))
    {
        // save the current text of the combo box as the current value of the item
        model->setData(index, cb->currentText(), Qt::EditRole);
        model->setData(index, cb->currentData(Qt::UserRole), Qt::UserRole); // account id or action id
    }
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}


///////////////////////////////////////////////////////////////////////////////////////////
// Implementation of BatchActionDialog
///////////////////////////////////////////////////////////////////////////////////////////
BatchActionDialog::BatchActionDialog(bool preferredIgn, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchActionDialog),
    _preferredIgn(preferredIgn)
{
    ui->setupUi(this);

    this->setupStateMachine();

    QStringList names;
    QVariantList ids;
    QSqlQuery q;
    /* collect and populate batch list */
    q.exec("SELECT id, name FROM Batches ORDER BY timestamp");
    while (q.next())
    {
        names << q.value("name").toString();
        ids << q.value("id");
    }
    for (int i = 0; i < names.size(); i++)
        ui->comboBoxBatches->addItem(names.at(i), ids.at(i));

    /* setup editor */
    ui->tableWidget->setItemDelegate(new BatchActionTableItemDelegate(preferredIgn, ui->tableWidget));
}

BatchActionDialog::~BatchActionDialog()
{
    delete ui;
    delete _stateMachine;
}

void BatchActionDialog::setupStateMachine()
{
    /* state machine */
    _stateMachine = new QStateMachine(this);

    /* create states */
    QState *idle = new QState();
    QState *running = new QState();
    QState *edit = new QState();

    /* setup properties when entering states */
    idle->assignProperty(this, "windowTitle", "Batch Action - idle");
    idle->assignProperty(ui->comboBoxBatches, "enabled", true);
    idle->assignProperty(ui->pushButtonRunBatch, "enabled", true);
    idle->assignProperty(ui->pushButtonNewBatch, "enabled", true);
    idle->assignProperty(ui->pushButtonDeleteBatch, "enabled", true);
    idle->assignProperty(ui->pushButtonRenameBatch, "enabled", true);
    idle->assignProperty(ui->pushButtonCloneBatch, "enabled", true);
    idle->assignProperty(ui->pushButtonModifyBatch, "enabled", true);
    idle->assignProperty(ui->pushButtonSaveBatch, "enabled", false);
    idle->assignProperty(ui->tableWidget, "editTriggers", BATCH_ACTION_TABLE_DISABLED_EDIT_TRIGGER);
    running->assignProperty(this, "windowTitle", "Batch Action - running");
    running->assignProperty(ui->comboBoxBatches, "enabled", false);
    running->assignProperty(ui->pushButtonRunBatch, "enabled", false);
    running->assignProperty(ui->pushButtonNewBatch, "enabled", false);
    running->assignProperty(ui->pushButtonDeleteBatch, "enabled", false);
    running->assignProperty(ui->pushButtonRenameBatch, "enabled", false);
    running->assignProperty(ui->pushButtonCloneBatch, "enabled", false);
    running->assignProperty(ui->pushButtonModifyBatch, "enabled", false);
    running->assignProperty(ui->pushButtonSaveBatch, "enabled", false);
    running->assignProperty(ui->tableWidget, "editTriggers", BATCH_ACTION_TABLE_DISABLED_EDIT_TRIGGER);
    edit->assignProperty(this, "windowTitle", "Batch Action - edit");
    edit->assignProperty(ui->comboBoxBatches, "enabled", false);
    edit->assignProperty(ui->pushButtonRunBatch, "enabled", false);
    edit->assignProperty(ui->pushButtonNewBatch, "enabled", false);
    edit->assignProperty(ui->pushButtonDeleteBatch, "enabled", false);
    edit->assignProperty(ui->pushButtonRenameBatch, "enabled", false);
    edit->assignProperty(ui->pushButtonCloneBatch, "enabled", false);
    edit->assignProperty(ui->pushButtonModifyBatch, "enabled", false);
    edit->assignProperty(ui->pushButtonSaveBatch, "enabled", true);
    edit->assignProperty(ui->tableWidget, "editTriggers", (int)BATCH_ACTION_TABLE_ENABLED_EDIT_TRIGGER);

    /* state transitions */
    idle->addTransition(this, SIGNAL(runBatchActionStarted()), running);
    idle->addTransition(this, SIGNAL(beginBatchActionEditMode(qlonglong)), edit);
    running->addTransition(this, SIGNAL(runBatchActionCompleted(int,int)), idle);
    edit->addTransition(this, SIGNAL(endBatchActionEditMode(qlonglong)), idle);

    /* add states and start state machine */
    _stateMachine->addState(idle);
    _stateMachine->addState(running);
    _stateMachine->addState(edit);
    _stateMachine->setInitialState(idle);
    _stateMachine->start();
}

void BatchActionDialog::setBatchActionTableText(int row, int col, const QString &cellText, qlonglong userId)
{
    QTableWidget *t = ui->tableWidget;
    QTableWidgetItem *cellItem;
    cellItem = t->item(row, col);
    if (cellItem == nullptr)
    {
        cellItem = new QTableWidgetItem(cellText);
        if (userId != -1)
            cellItem->setData(Qt::UserRole, userId);
        t->setItem(row, col, cellItem);
    }
    else
    {
        cellItem->setText(cellText);
        if (userId != -1)
            cellItem->setData(Qt::UserRole, userId);
    }
}

void BatchActionDialog::onNewBatch()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Create new batch", "Please input batch name", QLineEdit::Normal, "", &ok);

    if (!ok || name.isEmpty())
        return;

    /* insert into db */
    QSqlQuery q;
    q.prepare("INSERT INTO Batches(name) VALUES(:name)");
    q.bindValue(":name", name);
    if (!q.exec())
    {
        qDebug() << "Failed to create batch" << name << "Reason:" << q.lastError();
        return;
    }
    qlonglong id = q.lastInsertId().toLongLong();

    /* insert into combobox */
    ui->comboBoxBatches->addItem(name, id);
    ui->comboBoxBatches->setCurrentIndex(ui->comboBoxBatches->count() - 1);

    emit beginBatchActionEditMode(id);
}

void BatchActionDialog::onDeleteBatch()
{
    QString batchName = ui->comboBoxBatches->currentText();
    if (batchName.isEmpty())
        return;

    QMessageBox::StandardButton answer;
    answer = QMessageBox::question(this,
                                  "Confirm delete batch",
                                  "Are you sure you want to delete current active batch '" + batchName + "'?");
    if (answer != QMessageBox::Yes)
        return;

    /* delete from database */
    QSqlQuery q;
    q.prepare("DELETE FROM Batches WHERE name = :name");
    q.bindValue(":name", batchName);
    if (!q.exec())
    {
        qDebug() << "Failed to delete batch" << batchName << "Reason:" << q.lastError();
        return;
    }

    /* delete from ui */
    ui->comboBoxBatches->removeItem(ui->comboBoxBatches->currentIndex());
}

void BatchActionDialog::onRenameBatch()
{
    QString oldName = ui->comboBoxBatches->currentText();
    if (oldName.isEmpty())
        return;

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename current batch", "Please input the new batch name:", QLineEdit::NoEcho, "", &ok);
    if (!ok || newName.isEmpty())
        return;

    qlonglong batchId = ui->comboBoxBatches->itemData(ui->comboBoxBatches->currentIndex()).toLongLong(&ok);
    if (!ok)
        return;
    /* update database */
    QSqlQuery q;
    q.prepare("UPDATE Batches SET name = :newName WHERE id = :batchId");
    q.bindValue(":newName", newName);
    q.bindValue(":batchId", batchId);
    if (!q.exec())
    {
        qDebug() << "Failed to update batch name from" << oldName << "to" << newName << "Reason:" << q.lastError();
        return;
    }

    /* update ui */
    ui->comboBoxBatches->setCurrentText(newName);
}

void BatchActionDialog::onCloneBatch()
{
    QString srcName = ui->comboBoxBatches->currentText();
    if (srcName.isEmpty())
        return;

    bool ok;
    QString dstName = QInputDialog::getText(this, "Cloney current batch", "Please input the destination batch name:", QLineEdit::Normal, "", &ok);
    if (!ok || dstName.isEmpty())
        return;
    
    qlonglong srcBatchId = ui->comboBoxBatches->itemData(ui->comboBoxBatches->currentIndex()).toLongLong(&ok);
    if (!ok)
        return;
    
    /* insert into database */
    QSqlQuery q;
    q.prepare("INSERT INTO Batches(name) VALUES(:dstName)");
    q.bindValue(":dstName", dstName);
    if (!q.exec())
    {
        qDebug() << "Failed to clone batch. Reason:" << q.lastError();
        return;
    }
    qlonglong dstBatchId = q.lastInsertId().toLongLong(&ok);
    if (!ok)
    {
        q.prepare("SELECT id FROM Batches WHERE name = :dstName");
        q.bindValue(":dstName", dstName);
        if (!q.exec() || !q.next())
        {
            qDebug() << "Cannot find out cloned batch id, failed to clone..." << q.lastError();
            return;
        }
        dstBatchId = q.value("id").toLongLong();
    }
    q.prepare("INSERT INTO BatchContents(batchId, accountId, doWhat, parameter) SELECT :dstBatchId, accountId, doWhat, parameter FROM BatchContents WHERE batchId = :srcBatchId");
    q.bindValue(":dstBatchId", dstBatchId);
    q.bindValue(":srcBatchId", srcBatchId);
    if (!q.exec())
    {
        qDebug() << "Failed to clone batch contents. Reason:" << q.lastError();
        return;
    }

    /* append cloned batch to ui combobox */
    ui->comboBoxBatches->addItem(dstName, dstBatchId);
    ui->comboBoxBatches->setCurrentIndex(ui->comboBoxBatches->count() - 1);
}

void BatchActionDialog::onModifyBatch()
{
    if (ui->comboBoxBatches->currentIndex() != -1 && !ui->comboBoxBatches->currentText().isEmpty())
        emit beginBatchActionEditMode(ui->comboBoxBatches->currentData().toLongLong());
}

void BatchActionDialog::onSaveBatch()
{
    bool ok;
    qlonglong currentBatchId = ui->comboBoxBatches->currentData().toLongLong(&ok);

    if (!ok)
        return;

    QSqlDatabase::database().transaction();
    QSqlQuery remove;
    remove.prepare("DELETE FROM BatchContents WHERE batchId = :batchId");
    remove.bindValue(":batchId", currentBatchId);
    remove.exec();
    QSqlQuery insert;
    insert.prepare("INSERT INTO BatchContents(batchId, accountId, doWhat, parameter) VALUES(:batchId, :accountId, :doWhat, :parameter)");
    for (int row = 0; row < 100; row++)
    {
        QTableWidgetItem *account = ui->tableWidget->item(row, BatchActionTableColumns::Who);
        QTableWidgetItem *doWhat = ui->tableWidget->item(row, BatchActionTableColumns::DoWhat);
        QTableWidgetItem *parameter = ui->tableWidget->item(row, BatchActionTableColumns::Parameter);
        if (account->text().isEmpty() || doWhat->text().isEmpty() || parameter->text().isEmpty())
            continue;
        insert.bindValue(":batchId", currentBatchId);
        insert.bindValue(":accountId", account->data(Qt::UserRole));
        insert.bindValue(":doWhat", doWhat->data(Qt::UserRole));
        insert.bindValue(":parameter", parameter->text());
        insert.exec();
    }
    QSqlDatabase::database().commit();

    emit endBatchActionEditMode(currentBatchId);
}

void BatchActionDialog::onRunBatch()
{
    QSqlQuery q;

    q.exec("SELECT rowid, email, accountId, doWhat, parameter FROM ActiveBatchContents ORDER BY rowid");
    while (q.next())
    {
        qDebug() << q.value("email").toString() << q.value("accountId").toString() << q.value("doWhat").toString() << q.value("parameter").toString();
    }

    emit runBatchActionCompleted(0, 0);
}

void BatchActionDialog::onActiveBatchIndexChanged(int index)
{
    qDebug() << __FUNCTION__ << index;
    bool ok;
    int filledRow = -1;
    qlonglong batchId = ui->comboBoxBatches->itemData(index).toLongLong(&ok);

    if (ok)
    {
        QSqlQuery q;
        if (!q.exec("DROP TABLE IF EXISTS ActiveBatchContents"))
            qDebug() << "Failed to drop ActiveBatchContents table" << q.lastError();

        if (!q.prepare("CREATE TEMP TABLE IF NOT EXISTS ActiveBatchContents AS SELECT a.email, s.inGameName AS ign, b.accountId, doWhat, parameter FROM BatchContents as b INNER JOIN Accounts AS a ON a.id = b.accountId LEFT OUTER JOIN Stats AS s ON s.accountId = a.id WHERE batchId = :batchId"))
            qDebug() << "Failed to prepare create temp table..." << q.lastError();
        q.bindValue(":batchId", batchId);
        if (!q.exec())
            qDebug() << "Failed to create temp table." << q.lastError();

        if (!q.exec("SELECT rowid, email, ign, accountId, doWhat, parameter FROM ActiveBatchContents ORDER BY rowid"))
            qDebug() << "Failed to retrieve from ActiveBatchContents" << q.lastError();
        while (q.next())
        {
            filledRow = q.value("rowid").toInt() - 1;

            setBatchActionTableText(filledRow, BatchActionTableColumns::Who, (_preferredIgn && !q.value("ign").toString().isEmpty()) ? q.value("ign").toString() : q.value("email").toString(), q.value("accountId").toLongLong());
            setBatchActionTableText(filledRow, BatchActionTableColumns::DoWhat, ACTION_DESCRIPTIONS[q.value("doWhat").toInt()].desc, q.value("doWhat").toLongLong());
            setBatchActionTableText(filledRow, BatchActionTableColumns::Parameter, q.value("parameter").toString());
        }
    }

    for (filledRow++; filledRow < 100; filledRow++)
    {
        setBatchActionTableText(filledRow, BatchActionTableColumns::Who, "");
        setBatchActionTableText(filledRow, BatchActionTableColumns::DoWhat, "");
        setBatchActionTableText(filledRow, BatchActionTableColumns::Parameter, "");
    }
}
