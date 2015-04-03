#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "newaccountdialog.h"
#include "sqliteopenhelper.h"
#include "castleage.h"
#include "batchactiondialog.h"


struct StatKeyMap
{
    enum UserStatKeys key;
    QString statLabel;
    QString dbColumn;
};

const struct StatKeyMap STATS_MAPPING[] =
{
    { UserStatKeys::FacebookId,     "FacebookId",       "facebookId"    },
    { UserStatKeys::InGameName,     "InGameName",       "inGameName"    },
    { UserStatKeys::Level,          "Level",            "level"         },
    { UserStatKeys::MaxEnergy,      "MaxEnergy",        "maxEnergy"     },
    { UserStatKeys::MaxStamina,     "MaxStamina",       "maxStamina"    },
    { UserStatKeys::MaxHealth,      "MaxHealth",        "maxHealth"     },
    { UserStatKeys::Attack,         "Attack",           "attack"        },
    { UserStatKeys::Defense,        "Defense",          "defense"       },
    { UserStatKeys::ArmySize,       "ArmySize",         "armySize"      },
    { UserStatKeys::BattleRank,     "BattleRank",       "battleRank"    },
    { UserStatKeys::WarRank,        "WarRank",          "warRank"       },
    { UserStatKeys::ConquestRank,   "ConquestRank",     "conquestRank"  },
    { UserStatKeys::FestivalRank,   "FestivalRank",     "festivalRank"  },
    { UserStatKeys::AttackEssence,  "AttackEssence",    "attackEssence" },
    { UserStatKeys::DefenseEssence, "DefenseEssence",   "defenseEssence"},
    { UserStatKeys::DamageEssence,  "DamageEssence",    "damageEssence" },
    { UserStatKeys::HealthEssence,  "HealthEssence",    "healthEssence" },
    { UserStatKeys::GuildId,        "GuildId",          "guildId"       },
    { UserStatKeys::GuildName,      "GuildName",        "guildName"     }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    /* manage database generation or upgrade. */
    mSQLiteOpenHelper = new SQLiteOpenHelper("castleage.sqlite", 1);
    connect(mSQLiteOpenHelper, SIGNAL(createDatabase(QSqlDatabase&)), this, SLOT(onCreateDatabase(QSqlDatabase&)));
    connect(mSQLiteOpenHelper, SIGNAL(upgradeDatabase(QSqlDatabase&,int,int)), this, SLOT(onUpgradeDatabase(QSqlDatabase&,int,int)));
    connect(mSQLiteOpenHelper, SIGNAL(downgradeDatabase(QSqlDatabase&,int,int)), this, SLOT(onDowngradeDatabase(QSqlDatabase&,int,int)));
    mSQLiteOpenHelper->init();

    /* load app prefs from database into local cache */
    initAppPrefs();

    /* setup ui */
    ui->setupUi(this);
    ui->actionPreferredShowIGN->setChecked(getAppPrefs(AppPrefs::AccountShowIgn).toBool());

    /* populate the account list */
    populateAccounts();

    /* init request manager per account */
    initRequestManagers();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (mSQLiteOpenHelper != nullptr)
    {
        delete mSQLiteOpenHelper;
        mSQLiteOpenHelper = nullptr;
    }
}

//
// Helper methods
//
void MainWindow::populateAccounts()
{
    QStringList emails;
    QStringList igns;
    QVariantList ids;

    /* retrieve from database */
    QSqlQuery q;
    q.exec("SELECT a.id, a.email, s.inGameName FROM Accounts AS a LEFT OUTER JOIN Stats AS s ON a.id = s.accountId ORDER BY a.timestamp");
    while (q.next())
    {
        ids << q.value("id");
        emails << q.value("email").toString();
        igns << q.value("inGameName").toString();
    }

    /* update ui */
    int size = ids.size();
    ui->listAccount->clear();
    for (int idx = 0; idx < size; idx++)
    {
        QListWidgetItem *item;
        if (mAppPrefs.value(AppPrefs::AccountShowIgn).toBool() && !igns.at(idx).isEmpty())
        {
            item = new QListWidgetItem(igns.at(idx));
            item->setToolTip(emails.at(idx));
        }
        else
        {
            item = new QListWidgetItem(emails.at(idx));
            item->setToolTip(igns.at(idx));
        }
        item->setData(Qt::UserRole, ids.at(idx));
        ui->listAccount->addItem(item);
    }
}

void MainWindow::initRequestManagers()
{
    QSqlQuery q;
    if (q.exec("SELECT id, email, password FROM Accounts"))
    {
        while (q.next())
        {
            qlonglong id = q.value("id").toLongLong();
            QString email = q.value("email").toString();
            QString password = q.value("password").toString();
            this->mRequestManagers.insert(id, createRequestManager(id, email, password));
        }
    }
}

CastleAgeRequestManager * MainWindow::createRequestManager(const qlonglong id, const QString &email, const QString &password)
{
    CastleAgeRequestManager *mgr = new CastleAgeRequestManager(id, email, password, this);
    connect(mgr, SIGNAL(StatsAvailable(qlonglong,QHash<UserStatKeys,QString>&)), this, SLOT(onStatsAvailable(qlonglong,QHash<UserStatKeys,QString>&)));
    connect(mgr, SIGNAL(AuthorizedFailure(qlonglong)), this, SLOT(onAuthorizedFailure(qlonglong)));
    return mgr;
}

void MainWindow::updateStatsItem(const QString &statLabel, const QString &statValue)
{
    QList<QTreeWidgetItem *> found = ui->treeAccountStat->findItems(statLabel, Qt::MatchExactly|Qt::MatchRecursive);
    if (found.size())
        found[0]->setText(1, statValue);
}

void MainWindow::initAppPrefs()
{
    QList<AppPrefs> keys;
    QVariantList values;
    int key;
    QSqlQuery q;
    q.exec("SELECT key, value FROM AppPrefs");
    while (q.next())
    {
        key = q.value("key").toInt();
        if (key >= AppPrefs::First && key < AppPrefs::Last)
        {
            keys << static_cast<AppPrefs>(key);
            values << q.value("value");
        }
    }

    for (int i = keys.size() - 1; i >= 0; i--)
        mAppPrefs.insert(keys.at(i), values.at(i));
}

QVariant MainWindow::getAppPrefs(AppPrefs key, const QVariant &defValue) const
{
    return mAppPrefs.value(key, defValue);
}

bool MainWindow::setAppPrefs(AppPrefs key, const QVariant &value)
{
    mAppPrefs.insert(key, value);
    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO AppPrefs(key, value) VALUES(:key, :value)");
    q.bindValue(":key", key);
    q.bindValue(":value", value.toString());
    return q.exec();
}

//
// Slot implementations
//
void MainWindow::onAbout()
{
    QMessageBox::about(this,
                       "Castle Age Helper, Version 0.1",
                       "This application is built for educational purpose only. "
                       "It performs http operations with remote server and parses replied content. "
                       "\n\nBuild at " __DATE__ " " __TIME__
                       );
}

void MainWindow::onAboutQt()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::onAddAccount()
{
    NewAccountDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        QSqlQuery q;
        q.prepare("INSERT INTO Accounts(email, password) VALUES(:email, :password)");
        q.bindValue(":email", dlg.getEmail());
        q.bindValue(":password", dlg.getPassword());
        if (q.exec())
        {
            /* append to QListWidget */
            QListWidgetItem *item = new QListWidgetItem(dlg.getEmail());
            item->setToolTip("Reload stat for this account to show in game name");
            item->setData(Qt::UserRole, q.lastInsertId());
            ui->listAccount->addItem(item);

            /* prepare request manager for this account. */
            qlonglong accountId = q.lastInsertId().toLongLong();
            this->mRequestManagers.insert(accountId, createRequestManager(accountId, dlg.getEmail(), dlg.getPassword()));
        }
    }

}

void MainWindow::onRemoveAccount()
{
    /* QListWidget::currentItem() return 1st item before we really select one, so don't use it, use our cached one. */
    if (mSelectedAccountItem == nullptr)
        return;

    QMessageBox dlg(QMessageBox::Question,
                    "Confirm remove account",
                    "Are you sure you want to remove account: " + mSelectedAccountItem->text(),
                    QMessageBox::Yes | QMessageBox::No,
                    this);
    //dlg.setInformativeText("Add informative text here");
    //dlg.setDetailedText("Add detail text here");
    if (dlg.exec() == QMessageBox::Yes)
    {
        QVariant id = mSelectedAccountItem->data(Qt::UserRole);
        QSqlQuery q;
        q.prepare("DELETE FROM Accounts WHERE id = :id");
        q.bindValue(":id", id);
        if (q.exec() && q.numRowsAffected() == 1)
        {
            delete mSelectedAccountItem;
            /* do NOT set mSeleectedAccountItem to nullptr here,
             * onAccountSelectionChanged() will assign another selected item to it, if there exists one. */

            /* disconnect to slots and delete request manager */
            CastleAgeRequestManager *mgr = this->mRequestManagers.take(id.toLongLong());
            disconnect(mgr, SIGNAL(StatsAvailable(qlonglong,QHash<UserStatKeys,QString>&)), this, SLOT(onStatsAvailable(qlonglong,QHash<UserStatKeys,QString>&)));
            delete mgr;
        }
        else
            qDebug() << "Failed to delete account:" << mSelectedAccountItem->text();
    }

    return;
}

void MainWindow::onReloadSelectedAccount()
{
    if (this->mSelectedAccountItem != nullptr)
    {
        qlonglong id = this->mSelectedAccountItem->data(Qt::UserRole).toLongLong();
        CastleAgeRequestManager *mgr = mRequestManagers.value(id, nullptr);
        if (mgr != nullptr)
            mgr->retrieveStats();
    }
}

void MainWindow::onReloadAllAccounts()
{
    qlonglong accountId;
    CastleAgeRequestManager *mgr;
    int size = ui->listAccount->count();
    for (int idx = 0; idx < size; idx++)
    {
        accountId = ui->listAccount->item(idx)->data(Qt::UserRole).toLongLong();
        mgr = mRequestManagers.value(accountId);
        if (mgr != nullptr)
            mgr->retrieveStats();
    }
}

void MainWindow::onShowIGN(bool checked)
{
    setAppPrefs(AppPrefs::AccountShowIgn, checked);
    populateAccounts();
}

void MainWindow::onBatchAction()
{
    BatchActionDialog dlg(this);
    dlg.exec();
}

void MainWindow::onCreateDatabase(QSqlDatabase &db)
{
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS Accounts("
             "id INTEGER PRIMARY KEY,"
             "email TEXT UNIQUE,"
             "password TEXT,"
             "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS Stats("
             "id INTEGER PRIMARY KEY,"
             "accountId INTEGER UNIQUE REFERENCES Accounts ON DELETE CASCADE,"
             "facebookId TEXT UNIQUE,"
             "inGameName TEXT,"
             "level NUMERIC,"
             "maxEnergy NUMERIC,"
             "maxStamina NUMERIC,"
             "maxHealth NUMERIC,"
             "attack NUMERIC,"
             "defense NUMERIC,"
             "armySize NUMERIC,"
             "battleRank TEXT,"
             "warRank TEXT,"
             "conquestRank TEXT,"
             "festivalRank TEXT,"
             "attackEssence NUMERIC,"
             "defenseEssence NUMERIC,"
             "damageEssence NUMERIC,"
             "healthEssence NUMERIC,"
             "guildId TEXT,"
             "guildName TEXT,"
             "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS AppPrefs("
             "key NUMERIC UNIQUE,"
             "value TEXT"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS Batches("
             "id INTEGER PRIMARY KEY,"
             "name TEXT UNIQUE,"
             "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
           ")");
    q.exec("CREATE TABLE IF NOT EXISTS BatchContents("
             "id INTEGER PRIMARY KEY,"
             "batchId INTEGER REFERENCES Batches ON DELETE CASCADE,"
             "accountId INTEGER REFERENCES Accounts ON DELETE CASCADE,"
             "doWhat INTEGER,"
             "parameter TEXT"
           ")");
}

void MainWindow::onUpgradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion)
{
    Q_UNUSED(db);
    Q_UNUSED(dbVersion);
    Q_UNUSED(codeVersion);

//    switch (dbVersion)
//    {
//    case 1:
//        qDebug() << "Upgrade database version from 1 to 2...";
//    case 2:
//        qDebug() << "Upgrade database version from 2 to 3...";
//    }
}

void MainWindow::onDowngradeDatabase(QSqlDatabase &db, int dbVersion, int codeVersion)
{
    Q_UNUSED(db);
    Q_UNUSED(dbVersion);
    Q_UNUSED(codeVersion);
}

void MainWindow::onAccountSelectionChanged()
{
    mSelectedAccountItem = ui->listAccount->currentItem();
    bool clear = false;

    if (mSelectedAccountItem != nullptr)
    {
        /* select to specific account */
        QSqlQuery q;
        q.prepare("SELECT * FROM Stats WHERE accountId = :id");
        q.bindValue(":id", mSelectedAccountItem->data(Qt::UserRole).toLongLong());
        q.exec();

        if (q.next())
        {
            /* found stats record */
            for (int i = sizeof(STATS_MAPPING)/sizeof(struct StatKeyMap) - 1; i >= 0; i--)
                updateStatsItem(STATS_MAPPING[i].statLabel, q.value(STATS_MAPPING[i].dbColumn).toString());
        }
        else
        {
            /* no stats record */
            clear = true;
        }
    }
    else
    {
        /* select to nothing */
        clear = true;
    }

    if (clear)
    {
        for (int i = sizeof(STATS_MAPPING)/sizeof(struct StatKeyMap) - 1; i >= 0; i--)
            updateStatsItem(STATS_MAPPING[i].statLabel, "");
    }
}

void MainWindow::onAuthorizedFailure(qlonglong id)
{
    int count = ui->listAccount->count();
    QListWidgetItem *item;
    for (int row = 0; row < count; row++)
    {
        item = ui->listAccount->item(row);
        if (item->data(Qt::UserRole).toLongLong() == id)
        {
            item->setTextColor(QColor::fromRgb(255, 0, 0));
            break;
        }
    }


}

void MainWindow::onStatsAvailable(qlonglong id, QHash<UserStatKeys, QString> &stats)
{
    QSqlQuery insert;

    insert.prepare("INSERT OR REPLACE INTO Stats("
                     "id,"
                     "accountId,"
                     "facebookId,"
                     "inGameName,"
                     "level,"
                     "maxEnergy,"
                     "maxStamina,"
                     "maxHealth,"
                     "attack,"
                     "defense,"
                     "armySize,"
                     "battleRank,"
                     "warRank,"
                     "conquestRank,"
                     "festivalRank,"
                     "attackEssence,"
                     "defenseEssence,"
                     "damageEssence,"
                     "healthEssence,"
                     "guildId,"
                     "guildName"
                   ") VALUES("
                     "NULL,"
                     ":id,"
                     ":facebookId,"
                     ":inGameName,"
                     ":level,"
                     ":maxEnergy,"
                     ":maxStamina,"
                     ":maxHealth,"
                     ":attack,"
                     ":defense,"
                     ":armySize,"
                     ":battleRank,"
                     ":warRank,"
                     ":conquestRank,"
                     ":festivalRank,"
                     ":attackEssence,"
                     ":defenseEssence,"
                     ":damageEssence,"
                     ":healthEssence,"
                     ":guildId,"
                     ":guildName"
                   ")");
    insert.bindValue(":id", id);
    insert.bindValue(":facebookId", stats.value(UserStatKeys::FacebookId));
    insert.bindValue(":inGameName", stats.value(UserStatKeys::InGameName));
    insert.bindValue(":level", stats.value(UserStatKeys::Level));
    insert.bindValue(":maxEnergy", stats.value(UserStatKeys::MaxEnergy));
    insert.bindValue(":maxStamina", stats.value(UserStatKeys::MaxStamina));
    insert.bindValue(":maxHealth", stats.value(UserStatKeys::MaxHealth));
    insert.bindValue(":attack", stats.value(UserStatKeys::Attack));
    insert.bindValue(":defense", stats.value(UserStatKeys::Defense));
    insert.bindValue(":armySize", stats.value(UserStatKeys::ArmySize));
    insert.bindValue(":battleRank", stats.value(UserStatKeys::BattleRank));
    insert.bindValue(":warRank", stats.value(UserStatKeys::WarRank));
    insert.bindValue(":conquestRank", stats.value(UserStatKeys::ConquestRank));
    insert.bindValue(":festivalRank", stats.value(UserStatKeys::FestivalRank));
    insert.bindValue(":attackEssence", stats.value(UserStatKeys::AttackEssence));
    insert.bindValue(":defenseEssence", stats.value(UserStatKeys::DefenseEssence));
    insert.bindValue(":damageEssence", stats.value(UserStatKeys::DamageEssence));
    insert.bindValue(":healthEssence", stats.value(UserStatKeys::HealthEssence));
    insert.bindValue(":guildId", stats.value(UserStatKeys::GuildId));
    insert.bindValue(":guildName", stats.value(UserStatKeys::GuildName));

    if (!insert.exec())
        qDebug() << "Failed to write to stats table...?!";

    if (mSelectedAccountItem != nullptr && mSelectedAccountItem->data(Qt::UserRole).toLongLong() == id)
    {
        /* query response is current selected, so tree widget can be updated */
        for (int i = sizeof(STATS_MAPPING)/sizeof(struct StatKeyMap) - 1; i >= 0; i--)
            updateStatsItem(STATS_MAPPING[i].statLabel, stats.value(STATS_MAPPING[i].key));
    }
}
