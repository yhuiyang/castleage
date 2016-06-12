#ifndef ACCOUNTMANAGEMENTDIALOG_H
#define ACCOUNTMANAGEMENTDIALOG_H

#include <QDialog>

namespace Ui {
class AccountManagementDialog;
}

class AccountManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountManagementDialog(QWidget *parent = 0);
    ~AccountManagementDialog();

signals:
    void account_updated();

public slots:
    void populuteAccounts();
    void onAccountImport();
    void onAccountDelete();
    void onUpdateGuilds();
    void onUpdateIGNs();
    void onUpdateFBIDs();
    void onAccountMoveUp();
    void onAccountMoveDown();

private:
    Ui::AccountManagementDialog *ui;
};

#endif // ACCOUNTMANAGEMENTDIALOG_H
