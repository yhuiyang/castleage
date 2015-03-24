#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include "sqliteopenhelper.h"

SQLiteOpenHelper::SQLiteOpenHelper(QString name, int version)
    : mCodeVersion(version)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    if (!name.endsWith(".sqlite"))
        name += ".sqlite";
    db.setDatabaseName(name);
    db.open();
}

SQLiteOpenHelper::~SQLiteOpenHelper()
{
   QSqlDatabase db = QSqlDatabase::database();
   if (db.isOpen())
       db.close();
}

bool SQLiteOpenHelper::init()
{
    if (mCodeVersion < 1)
    {
        qCritical() << "code version must be >= 1, was" << mCodeVersion;
        return false;
    }

    QSqlQuery query;
    query.exec("PRAGMA foreign_keys = ON"); /* explicitly enable foreign keys support */
    query.exec("PRAGMA user_version");
    if (!query.next())
    {
        qDebug() << "Faild to lookup database version";
        return false;
    }
    else
    {
        int dbVersion = query.value(0).toInt();
        if (dbVersion != mCodeVersion)
        {
            QSqlDatabase db = QSqlDatabase::database();
            db.transaction();
            if (dbVersion == 0)
                emit createDatabase(db);
            else if (dbVersion > mCodeVersion)
                emit downgradeDatabase(db, dbVersion, mCodeVersion);
            else if (dbVersion < mCodeVersion)
                emit upgradeDatabase(db, dbVersion, mCodeVersion);
            QSqlQuery queryInTransaction(db);
            queryInTransaction.exec(QString("PRAGMA user_version = %1").arg(mCodeVersion));
            db.commit();
        }

        return true;
    }
}

