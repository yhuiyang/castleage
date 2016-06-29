#ifndef ACCOUNTMANAGEMENTDIALOG_H
#define ACCOUNTMANAGEMENTDIALOG_H

#include <QDialog>
#include <QWebPage>
#include <QModelIndex>

class SynchronizedNetworkAccessManager;
class QTreeWidgetItem;

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
    void tag_updated();

public slots:
    void populuteAccounts();
    void populateTags();
    void onAccountImport();
    void onAccountDelete();
    void onUpdateGuilds();
    void onUpdateIGNs();
    void onUpdateFBIDs();
    void onAccountMoveUp();
    void onAccountMoveDown();
    void onAccountActivated(QModelIndex);
    void onCreateTag();
    void onTagByWhatChanged(bool);
    void onTagItemDoubleClicked(QTreeWidgetItem*,int);

private:
    void showLog(const QString &message);
    SynchronizedNetworkAccessManager *getNetworkAccessManager(const qlonglong accountId, const QByteArray &accountEmail);

private:
    Ui::AccountManagementDialog *ui;
    QWebPage _page;
    QHash<QString, SynchronizedNetworkAccessManager *> _accountMgrs;
};

#endif // ACCOUNTMANAGEMENTDIALOG_H
