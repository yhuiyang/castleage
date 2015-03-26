#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "constant.h"

class QSqlDatabase;
class SQLiteOpenHelper;
class QListWidgetItem;
class CastleAgeRequestManager;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:
    void onAddAccount();
    void onRemoveAccount();
    void onReloadSelectedAccount();
    void onReloadAllAccounts();
    void onBatchAction();
    void onCreateDatabase(QSqlDatabase &db);
    void onUpgradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);
    void onDowngradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);
    void onAccountSelectionChanged();
    void onStatsAvailable(QString email, QHash<UserStatKeys, QString> &stats);

private:
    void populateAccounts();
    void initRequestManagers();
    CastleAgeRequestManager* createRequestManager(const QString &email, const QString &password);
    void updateStatsItem(const QString &statLabel, const QString &statValue);

private:
    Ui::MainWindow *ui;
    SQLiteOpenHelper *mSQLiteOpenHelper;
    QListWidgetItem *mSelectedAccountItem;
    QHash<QString, CastleAgeRequestManager *> mRequestManagers;
};

#endif // MAINWINDOW_H
