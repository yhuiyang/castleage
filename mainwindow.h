#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariant>
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
    void onAbout();
    void onAboutQt();
    void onAddAccount();
    void onRemoveAccount();
    void onReloadSelectedAccount();
    void onReloadAllAccounts();
    void onShowIGN(bool);
    void onBatchAction();
    void onCreateDatabase(QSqlDatabase &db);
    void onUpgradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);
    void onDowngradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);
    void onAccountSelectionChanged();
    void onAuthorizedFailure(qlonglong);
    void onStatsAvailable(qlonglong, QHash<UserStatKeys, QString> &stats);

private:
    void populateAccounts();
    void initRequestManagers();
    CastleAgeRequestManager* createRequestManager(const qlonglong id, const QString &email, const QString &password);
    void updateStatsItem(const QString &statLabel, const QString &statValue);
    void initAppPrefs();
    QVariant getAppPrefs(AppPrefs key, const QVariant& defValue = QVariant(0)) const;
    bool setAppPrefs(AppPrefs key, const QVariant &value);

private:
    Ui::MainWindow *ui;
    SQLiteOpenHelper *mSQLiteOpenHelper;
    QListWidgetItem *mSelectedAccountItem;
    QHash<qlonglong, CastleAgeRequestManager *> mRequestManagers;
    QHash<AppPrefs, QVariant> mAppPrefs;
};

#endif // MAINWINDOW_H
