#ifndef ACTIONCONSUMEDIALOG_H
#define ACTIONCONSUMEDIALOG_H

#include <QDialog>

namespace Ui {
class ActionConsumeDialog;
}

class QListWidgetItem;

class ActionConsumeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActionConsumeDialog(QWidget *parent = 0);
    ~ActionConsumeDialog();

public slots:
    void onAttackerTagOrGuildItemChanged(QListWidgetItem *item);
    void onRefreshLandDefenders(bool checked = false);

private:
    void populateAttackTagOrGuild();
    void populateDefendGuild();

private:
    Ui::ActionConsumeDialog *ui;
    QList<QVariant> _checkedAttackerTagIdOrGuildId;
};

#endif // ACTIONCONSUMEDIALOG_H
