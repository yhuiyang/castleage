#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QObject>
#include <QSqlDatabase>

class SQLiteHelper : public QObject
{
    Q_OBJECT
public:
    explicit SQLiteHelper(QString dbFilename, int version);
    virtual ~SQLiteHelper();
    bool init();

signals:
    void createDatabase(QSqlDatabase &db);
    void upgradeDatabase(QSqlDatabase &db, int fromVer, int toVer);
    void downgradeDatabase(QSqlDatabase &db, int fromVer, int toVer);

private:
    const int DATABASE_VERSION;
};

#endif // SQLITEHELPER_H
