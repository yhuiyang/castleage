#ifndef ARMYCODEANNOUNCEPLAN_H
#define ARMYCODEANNOUNCEPLAN_H

#include <QMainWindow>

namespace Ui {
class ArmyCodeAnnouncePlan;
}

class ArmyCodeAnnouncePlan : public QMainWindow
{
    Q_OBJECT

public:
    explicit ArmyCodeAnnouncePlan(QWidget *parent = 0);
    ~ArmyCodeAnnouncePlan();

private:
    void onAccountTableCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void activeAccountChanged(const int accountId);
    void refreshAccountTable();

private slots:
    void on_actionUpdateArmyCode_triggered();

    void on_actionUpdateFacebookId_triggered();

    void on_actionAnnounce_triggered();

    void on_actionDownloadArmyMembers_triggered();

private:
    Ui::ArmyCodeAnnouncePlan *ui;
};

#endif // ARMYCODEANNOUNCEPLAN_H
