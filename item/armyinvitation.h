#ifndef ARMYINVITATION_H
#define ARMYINVITATION_H

#include <QWidget>

namespace Ui {
class ArmyInvitation;
}

class ArmyInvitation : public QWidget
{
    Q_OBJECT

public:
    explicit ArmyInvitation(const int accountId, const QString &description, const QString &link, QWidget *parent = 0);
    ~ArmyInvitation();

private slots:
    void on_btnAccept_clicked();

private:
    Ui::ArmyInvitation *ui;
    const int mAccountId;
    const QString mAcceptLink;
};

#endif // ARMYINVITATION_H
