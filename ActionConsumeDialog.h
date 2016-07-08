#ifndef ACTIONCONSUMEDIALOG_H
#define ACTIONCONSUMEDIALOG_H

#include <QDialog>
#include <QWebPage>

namespace Ui {
class ActionConsumeDialog;
}

class QListWidgetItem;
class SynchronizedNetworkAccessManager;

class ActionConsumeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActionConsumeDialog(QWidget *parent = 0);
    ~ActionConsumeDialog();

public slots:
    void onAttackerTagOrGuildItemChanged(QListWidgetItem *item);
    void onRefreshLandDefenders(bool checked = false);
    void populateDefendGuildOfficersAndMembers(int selectedGuildIndex);

private:
    void showLog(const QString &message);
    SynchronizedNetworkAccessManager *getNetworkAccessManager(const qlonglong accountId, const QByteArray &accountEmail);
    void populateAttackTagOrGuild();
    void populateDefendGuild();

private:
    Ui::ActionConsumeDialog *ui;
    QList<QVariant> _checkedAttackerTagIdOrGuildId;
    QWebPage _page;
    QHash<QString, SynchronizedNetworkAccessManager *> _accountMgrs;
};

#endif // ACTIONCONSUMEDIALOG_H
