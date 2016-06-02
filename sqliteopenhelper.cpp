#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtSql>
#include <QVariant>
#include <QDebug>
#include "sqliteopenhelper.h"

SQLiteOpenHelper::SQLiteOpenHelper(QString name, int version)
    : DATABASE_VERSION(version)
{
    /* create a default connection. (didn't pass name in 2nd argument of addDatabase()) */
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    if (!name.endsWith(".sqlite"))
        name += ".sqlite";
    db.setDatabaseName(name);

    /* then open it */
    bool ok = db.open();
    if (!ok) {
        qWarning() << "Failed to open database. Reason:" << db.lastError();
    }
}

SQLiteOpenHelper::~SQLiteOpenHelper()
{
   QSqlDatabase db = QSqlDatabase::database();
   if (db.isOpen())
       db.close();
   QSqlDatabase::removeDatabase(db.connectionName());
}

bool SQLiteOpenHelper::init()
{
    if (DATABASE_VERSION < 1) {
        qCritical() << "code version must be >= 1, was" << DATABASE_VERSION;
        return false;
    }

    QSqlQuery query;
    query.exec("PRAGMA foreign_keys = ON"); /* explicitly enable foreign keys support */
    query.exec("PRAGMA user_version");
    if (!query.next()) {
        qDebug() << "Faild to lookup database version";
        return false;
    } else {
        int dbVersion = query.value(0).toInt();
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
}

