#include <QApplication>
#include <QWebEngineSettings>
#include <QDebug>
#include <QtSql>
#include "masterwindow.h"
#include "sqlitehelper.h"

// ---------------------------------------------
// Database creation/upgrade/downgrade methods
// ---------------------------------------------
void CreateDatabaseV1(QSqlDatabase &db)
{
    qDebug() << "Creating database v1...";

    QSqlQuery q(db);

    q.exec("CREATE TABLE IF NOT EXISTS accounts ("
           "_id INTEGER PRIMARY KEY AUTOINCREMENT"
           ", email TEXT UNIQUE NOT NULL"
           ", password TEXT NOT NULL"
           ", timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
           ", sequence INTEGER UNIQUE"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS tags ("
           "_id INTEGER PRIMARY KEY AUTOINCREMENT"
           ", name TEXT UNIQUE"
           ", timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
           ", sequence INTEGER UNIQUE"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS account_tag_mappings ("
           "accountId INTEGER REFERENCES accounts ON DELETE CASCADE"
           ", tagId INTEGER REFERENCES tags ON DELETE CASCADE"
           ", sequence INTEGER"
           ", UNIQUE(accountId, tagId) ON CONFLICT IGNORE"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS cookies ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", cookie TEXT NOT NULL"
           ", modified DATETIME DEFAULT CURRENT_TIMESTAMP"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS igns ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", ign TEXT NOT NULL"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS fbids ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", fbId TEXT NOT NULL"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS armycodes ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", armyCode TEXT NOT NULL"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS acapPool ("
           "armyCode TEXT NOT NULL"
           ", fbId TEXT NOT NULL"
           ", UNIQUE(armyCode, fbId)"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS ownedArmies ("
           "accountId INTEGER REFERENCES accounts ON DELETE CASCADE"
           ", fbId TEXT NOT NULL"
           ", level INTEGER NOT NULL"
           ", name TEXT NOT NULL"
           ", UNIQUE(accountId, fbId)"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS acapTimestamps ("
           "accountId INTEGER REFERENCES accounts ON DELETE CASCADE"
           ", announceTimestamp DATETIME"
           ", UNIQUE(accountId, announceTimestamp)"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS guilds ("
           "_id TEXT UNIQUE PRIMARY KEY ON CONFLICT REPLACE"
           ", name TEXT NOT NULL"
           ", creatorId TEXT NOT NULL"
           ", createdAt DATETIME"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS account_guild_mappings ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", guildId TEXT REFERENCES guilds ON DELETE CASCADE"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS roles ("
           "accountId INTEGER UNIQUE REFERENCES accounts ON DELETE CASCADE"
           ", role TEXT NOT NULL CHECK (role IN ('Master', 'Officer'))"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS fbid2igns ("
           "fbId TEXT UNIQUE ON CONFLICT REPLACE NOT NULL ON CONFLICT IGNORE"
           ", ign TEXT NOT NULL"
           ")");
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
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL); // without this, QWebEngineView::load() seems to crash randomly.

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
