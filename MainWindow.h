#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>

class QAction;
class QMdiArea;
class SQLiteOpenHelper;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

private:
    void createActions();
    void createStatusBar();

private slots:
    void createChildBrowser();
    /* database */
    void onCreateDatabase(QSqlDatabase &db);
    void onUpgradeDatabase(QSqlDatabase &db, int oldVersion, int newVersion);
    void onDowngradeDatabase(QSqlDatabase &db, int oldVersion, int newVersion);

private:
    QAction *_actionNewBrowser;
    QMdiArea *_mdiArea;
    SQLiteOpenHelper *_dbHelper;
};

#endif // MAINWINDOW_H
