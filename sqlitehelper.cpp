#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QtSql>
#include "sqlitehelper.h"

/**
 * @brief SQLiteHelper::SQLiteHelper
 * @param name
 * @param version
 *
 * This SQLite helper class expects all database files are located in directory to store app local data.
 */
SQLiteHelper::SQLiteHelper(QString name, int version)
    : DATABASE_VERSION(version)
{
    /* make sure app local data directory exists */
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dbPath);
    QString dbName = dbPath + "/" + name;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbName);

    bool ok = db.open();
    if (!ok)
        qWarning() << "Failed to open database. Reason:"
                   << db.lastError();
}

SQLiteHelper::~SQLiteHelper()
{
    QString connectionName;
    {
        QSqlDatabase db = QSqlDatabase::database();
        if (db.isOpen())
            db.close();
        connectionName = db.connectionName();
    }
    // Warning:
    // There should be no open queries on the database connection when QSqlDatabase::removeDatabase() is called,
    // otherwise a resource leak will occur.
    // Above code block will make sure the 'db' is destroyed, because it is out of scope.
    // Or we will get the follow warning message in app output.
    // "QSqlDatabasePrivate::removeDatabase: connection 'qt_sql_default_connection' is still in use, all queries will cease to work."
    QSqlDatabase::removeDatabase(connectionName);
}

bool SQLiteHelper::init()
{
    if (DATABASE_VERSION < 1) {
        qCritical() << "code version must be >= 1, was"
                    << DATABASE_VERSION;
        return false;
    }

    QSqlQuery query;
    query.exec("PRAGMA foreign_keys = ON");
    query.exec("PRAGMA user_version");
    if (!query.next()) {
        qCritical() << "Failed to lookup database version.";
        return false;
    }

    bool ok = false;
    int dbVersion = query.value(0).toInt(&ok);
    if (!ok) {
        qCritical() << "Unsupported user_version format.";
        return false;
    }

    if (dbVersion != DATABASE_VERSION) {
        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();
        if (dbVersion == 0)
            emit createDatabase(db);
        else if (dbVersion > DATABASE_VERSION)
            emit downgradeDatabase(db, dbVersion, DATABASE_VERSION);
        else if (dbVersion < DATABASE_VERSION)
            emit upgradeDatabase(db, dbVersion, DATABASE_VERSION);
        QSqlQuery queryInTransaction(db);
        queryInTransaction.exec(QString("PRAGMA user_version = %1").arg(DATABASE_VERSION));
        db.commit();
    }

    return true;
}
