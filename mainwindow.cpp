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

    /* setup ui */
    ui->setupUi(this);

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
    QSqlQuery q;
    if (q.exec("SELECT email FROM Accounts ORDER BY timestamp"))
    {
        QStringList l;
        while (q.next())
            l << q.value("email").toString();
        ui->listAccount->addItems(l);
    }
}

void MainWindow::initRequestManagers()
{
    QSqlQuery q;
    if (q.exec("SELECT email, password FROM Accounts"))
    {
        while (q.next())
        {
            QString email = q.value("email").toString();
            QString password = q.value("password").toString();
            this->mRequestManagers.insert(email, createRequestManager(email, password));
        }
    }
}

CastleAgeRequestManager * MainWindow::createRequestManager(const QString &email, const QString &password)
{
    CastleAgeRequestManager *mgr = new CastleAgeRequestManager(email, password, this);
    connect(mgr, SIGNAL(StatsAvailable(QString,QHash<UserStatKeys,QString>&)), this, SLOT(onStatsAvailable(QString,QHash<UserStatKeys,QString>&)));
    return mgr;
}

void MainWindow::updateStatsItem(const QString &statLabel, const QString &statValue)
{
    QList<QTreeWidgetItem *> found = ui->treeAccountStat->findItems(statLabel, Qt::MatchExactly|Qt::MatchRecursive);
    if (found.size())
        found[0]->setText(1, statValue);
}

//
// Slot implementations
//
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
            ui->listAccount->addItem(dlg.getEmail());

            /* prepare request manager for this account. */
            this->mRequestManagers.insert(dlg.getEmail(), createRequestManager(dlg.getEmail(), dlg.getPassword()));
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
        QString email = mSelectedAccountItem->text();
        QSqlQuery q;
        q.prepare("DELETE FROM Accounts WHERE email = :email");
        q.bindValue(":email", email);
        if (q.exec())
        {
            delete mSelectedAccountItem;
            /* do NOT set mSeleectedAccountItem to nullptr here,
             * onAccountSelectionChanged() will assign another selected item to it, if there exists one. */

            /* disconnect to slots and delete request manager */
            CastleAgeRequestManager *mgr = this->mRequestManagers.take(email);
            disconnect(mgr, SIGNAL(StatsAvailable(QString,QHash<UserStatKeys,QString>&)), this, SLOT(onStatsAvailable(QString,QHash<UserStatKeys,QString>&)));
            delete mgr;
        }
        else
            qDebug() << "Failed to delete account: " << mSelectedAccountItem->text();
    }

    return;
}

void MainWindow::onReloadSelectedAccount()
{
    if (this->mSelectedAccountItem != nullptr)
    {
        QString email = this->mSelectedAccountItem->text();
        CastleAgeRequestManager *mgr = mRequestManagers.value(email, nullptr);
        if (mgr != nullptr)
            mgr->retrieveStats();
    }
}

void MainWindow::onReloadAllAccounts()
{
    qDebug() << "Reload stats for all accounts.";
}

void MainWindow::onBatchAction()
{
    qDebug() << "Open batch action dialog.";
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
        q.prepare("SELECT * FROM Stats WHERE accountId = (SELECT id FROM accounts WHERE email = :email)");
        q.bindValue(":email", mSelectedAccountItem->text());
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

void MainWindow::onStatsAvailable(QString email, QHash<UserStatKeys, QString> &stats)
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
                     "(SELECT id FROM Accounts WHERE email = :email),"
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
    insert.bindValue(":email", email);
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

    if (mSelectedAccountItem != nullptr && mSelectedAccountItem->text() == email)
    {
        /* query response is current selected, so tree widget can be updated */
        for (int i = sizeof(STATS_MAPPING)/sizeof(struct StatKeyMap) - 1; i >= 0; i--)
            updateStatsItem(STATS_MAPPING[i].statLabel, stats.value(STATS_MAPPING[i].key));
    }
}
