#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>

class QMdiArea;
class SQLiteOpenHelper;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();
    void readSettings();

protected:
    virtual void closeEvent(QCloseEvent *event);

private:
    void createActions();
    void createStatusBar();

signals:
    void ca_account_updated();

private slots:
    void createChildBrowser();
    void showImportAccountDialog();
    void showAccountManagementDialog();
    /* database */
    void onCreateDatabase(QSqlDatabase &db);
    void onUpgradeDatabase(QSqlDatabase &db, int oldVersion, int newVersion);
    void onDowngradeDatabase(QSqlDatabase &db, int oldVersion, int newVersion);

private:
    QMdiArea *_mdiArea;
    SQLiteOpenHelper *_dbHelper;
};

#endif // MAINWINDOW_H
