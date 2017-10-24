#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QMainWindow>
#include <QtSql>

namespace Ui {
class AccountManager;
}

class AccountModel;
class QProgressBar;

class AccountManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit AccountManager(QWidget *parent = 0);
    ~AccountManager();

private:
    QList<int> selectedAccountIds();
    void refreshTable();

private slots:
    void on_actionAddAccount_triggered();

    void on_actionRemoveAccount_triggered();

    void on_actionUpdateIGN_triggered();

    void on_actionUpdateRole_triggered();

    void on_actionUpdateFBId_triggered();

    void on_actionUpdateGuild_triggered();

    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    Ui::AccountManager *ui;
    AccountModel *mModel;
    QProgressBar *mProgressBar;
};

class AccountModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    explicit AccountModel(QWidget *parent = 0);
    virtual ~AccountModel();
};

#endif // ACCOUNTMANAGER_H
