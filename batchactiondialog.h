#ifndef BATCHACTIONDIALOG_H
#define BATCHACTIONDIALOG_H

#include <QDialog>
#include <QStyledItemDelegate>

namespace Ui {
class BatchActionDialog;
}

class QStateMachine;

class BatchActionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchActionDialog(bool preferredIgn, QWidget *parent = 0);
    ~BatchActionDialog();

private:
    void setupStateMachine();
    void setBatchActionTableText(int row, int col, const QString &cellText, qlonglong userId = -1);

private Q_SLOTS:
    void onNewBatch();
    void onDeleteBatch();
    void onRenameBatch();
    void onCloneBatch();
    void onModifyBatch();
    void onSaveBatch();
    void onRunBatch();
    void onActiveBatchIndexChanged(int);

Q_SIGNALS:
    void beginBatchActionEditMode(qlonglong batchId);
    void endBatchActionEditMode(qlonglong batchId);
    void runBatchActionStarted();
    void runBatchActionProgress(int current, int total);
    void runBatchActionCompleted(int success, int failure);

private:
    Ui::BatchActionDialog *ui;
    bool _preferredIgn;
    QStateMachine *_stateMachine;
};


class BatchActionTableItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    enum AccountIdx { Id, Email, IGN, Max };

public:
    BatchActionTableItemDelegate(bool preferredIgn, QObject *parent = 0);
    virtual ~BatchActionTableItemDelegate() {}

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void setEditorData (QWidget *editor, const QModelIndex &index) const;
    virtual void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

private:
    QStringList mAccountList[AccountIdx::Max];
    bool mPreferredIgn;
};

#endif // BATCHACTIONDIALOG_H
