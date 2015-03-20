#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSqlDatabase;
class SQLiteOpenHelper;
class QListWidgetItem;

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
    void onCreateDatabase(QSqlDatabase &db);
    void onUpgradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);
    void onDowngradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);
    void onAccountSelectionChanged();

private:
    void populateAccounts();

private:
    Ui::MainWindow *ui;
    SQLiteOpenHelper *mSQLiteOpenHelper;
    QListWidgetItem *mSelectedAccountItem;
};

#endif // MAINWINDOW_H
