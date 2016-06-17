#ifndef ACCOUNTMANAGEMENTDIALOG_H
#define ACCOUNTMANAGEMENTDIALOG_H

#include <QDialog>
#include <QWebPage>

class SynchronizedNetworkAccessManager;

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
    void showLog(const QString &message);
    SynchronizedNetworkAccessManager *getNetworkAccessManager(const qlonglong accountId, const QByteArray &accountEmail);

private:
    Ui::AccountManagementDialog *ui;
    QWebPage _page;
    QHash<QString, SynchronizedNetworkAccessManager *> _accountMgrs;
};

#endif // ACCOUNTMANAGEMENTDIALOG_H
