#ifndef SQLITEOPENHELPER_H
#define SQLITEOPENHELPER_H

#include <QObject>
#include <QSqlDatabase>


class SQLiteOpenHelper : public QObject
{
    Q_OBJECT

public:
    SQLiteOpenHelper(QString name, int version);
    virtual ~SQLiteOpenHelper();
    bool init();

Q_SIGNALS:
    void createDatabase(QSqlDatabase &db);
    void upgradeDatabase(QSqlDatabase &db, int version, int newVersion);
    void downgradeDatabase(QSqlDatabase &db, int version, int newVersion);

private:
    const int DATABASE_VERSION;
};

#endif // SQLITEOPENHELPER_H
