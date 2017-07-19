#include <QApplication>
#include <QWebEngineSettings>
#include <QDebug>
#include "masterwindow.h"
#include "sqlitehelper.h"

// ---------------------------------------------
// Database creation/upgrade/downgrade methods
// ---------------------------------------------
void CreateDatabaseV1(QSqlDatabase &db)
{
    Q_UNUSED(db);
    qDebug() << "Creating database v1...";
}

void UpgradeDatabase_v1_to_v2(QSqlDatabase &db)
{
    Q_UNUSED(db);
    qDebug() << "Upgrading database v1 -> v2...";
}

void CreateDatabase(QSqlDatabase &db)
{
    CreateDatabaseV1(db);
}

void UpgradeDatabase(QSqlDatabase &db, int from, int to)
{
    Q_UNUSED(to);
    switch (from) {
    case 1:
        UpgradeDatabase_v1_to_v2(db);
    //case 2:
    //    UpgradeDatabase_v2_to_v3(db);
    default:
        break;
    }
}

// ----------------------------------------------------

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    /* init database */
    SQLiteHelper sqliteHelper("cabrowser.db", 1);
    QObject::connect(&sqliteHelper, &SQLiteHelper::createDatabase, [](QSqlDatabase &db) {
        CreateDatabase(db);
    });
    QObject::connect(&sqliteHelper, &SQLiteHelper::upgradeDatabase, [](QSqlDatabase &db, int from, int to) {
        UpgradeDatabase(db, from, to);
    });
    sqliteHelper.init();

    /* web engine settings */
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    /* create first window */
    MasterWindow *window = new MasterWindow();
    window->show();

    return app.exec();
}
