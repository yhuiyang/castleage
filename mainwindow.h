#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSqlDatabase;
class SQLiteOpenHelper;

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
    void onCreateDatabase(QSqlDatabase &db);
    void onUpgradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);
    void onDowngradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion);

private:
    Ui::MainWindow *ui;
    SQLiteOpenHelper *mSQLiteOpenHelper;
    QString mSqlQueryStringRetrieveAcctountList;
};

#endif // MAINWINDOW_H
