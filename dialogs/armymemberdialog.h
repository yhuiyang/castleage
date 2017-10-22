#ifndef ARMYMEMBERDIALOG_H
#define ARMYMEMBERDIALOG_H

#include <QDialog>

namespace Ui {
class ArmyMemberDialog;
}

class ArmyMemberDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ArmyMemberDialog(const int accountId, QWidget *parent = 0);
    ~ArmyMemberDialog();

private:
    QStringList getArmySuggestion();
    void checkAlliesNewsFeed();
    void parseAlliesNewsFeed(const QByteArray &html);

private:
    Ui::ArmyMemberDialog *ui;
    const int ACCOUNT_ID;
};

#endif // ARMYMEMBERDIALOG_H
