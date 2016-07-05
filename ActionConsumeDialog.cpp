#include <QtSql>
#include "ActionConsumeDialog.h"
#include "ui_ActionConsumeDialog.h"

ActionConsumeDialog::ActionConsumeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActionConsumeDialog)
{
    ui->setupUi(this);

    /* attackers */
    populateAttackTagOrGuild();
    connect(ui->listWidgetTagOrGuild, &QListWidget::itemChanged, this, &ActionConsumeDialog::onAttackerTagOrGuildItemChanged);

    /* defenders */
    populateDefendGuild();
    connect(ui->pushButtonRefreshLand, &QPushButton::clicked, this, &ActionConsumeDialog::onRefreshLandDefenders);

    setAttribute(Qt::WA_DeleteOnClose);
}

ActionConsumeDialog::~ActionConsumeDialog()
{
    delete ui;
}

//
// Slots
//
void ActionConsumeDialog::onAttackerTagOrGuildItemChanged(QListWidgetItem *item)
{
    QVariant tagIdOrGuildId = item->data(Qt::UserRole);
    QVariant checkState = item->data(Qt::CheckStateRole);
    if (checkState == Qt::Checked) {
        _checkedAttackerTagIdOrGuildId.append(tagIdOrGuildId);
    } else
        if (!_checkedAttackerTagIdOrGuildId.removeOne(tagIdOrGuildId))
            qWarning() << "remove tag id or guild id that didn't exist.";

    /* reset attacker table */
    ui->tableWidgetAttackers->setColumnCount(4);
    ui->tableWidgetAttackers->setRowCount(0);
    QStringList headerLabels;
    headerLabels << "IGN" << "Email" << "Tokens" << "Conquest Points";
    ui->tableWidgetAttackers->setHorizontalHeaderLabels(headerLabels);
    ui->tableWidgetAttackers->clearContents();

    /* update attacker table */
    QSqlQuery sql;
    int row = 0;
    for (QVariant id: _checkedAttackerTagIdOrGuildId) {
        QString idAsString = id.toString();
        if (idAsString.contains('_')) { // guild id
            sql.prepare("SELECT a.id, ifnull(i.ign, 'UnknownIGN'), a.email FROM accounts AS a "
                        "INNER JOIN guildids AS g ON g.accountId = a.id AND g.guildid = :guildId "
                        "LEFT JOIN igns AS i ON i.accountId = a.id "
                        "ORDER BY a.id");
            sql.bindValue(":guildId", idAsString);
        } else { // tag id
            sql.prepare("SELECT a.id, ifnull(i.ign, 'UnknownIGN'), a.email FROM accounts AS a "
                        "LEFT JOIN account_tag_mapping AS m ON m.accountId = a.id AND m.tagId = :tagId "
                        "ORDER BY a.id");
            sql.bindValue(":tagId", id.toInt());

        }

        if (sql.exec()) {
            while (sql.next()) {
                QTableWidgetItem *itemIgn = new QTableWidgetItem(sql.value(1).toString());
                QTableWidgetItem *itemEmail = new QTableWidgetItem(sql.value(2).toString());
                itemIgn->setData(Qt::UserRole, sql.value(0).toInt());
                itemEmail->setData(Qt::UserRole, sql.value(0).toInt());
                ui->tableWidgetAttackers->setRowCount(row + 1);
                ui->tableWidgetAttackers->setItem(row, 0, itemIgn);
                ui->tableWidgetAttackers->setItem(row, 1, itemEmail);
                row++;
            }
        }
    }

    if (row > 0)
        ui->tableWidgetAttackers->resizeColumnsToContents();
}

void ActionConsumeDialog::onRefreshLandDefenders(bool checked)
{

}

//
// Internal helper methods
//
void ActionConsumeDialog::populateAttackTagOrGuild()
{
    QSqlQuery sql;

    /* guilds */
    if (sql.exec("SELECT guildid, name FROM guildids INNER JOIN guilds ON guilds.id = guildids.guildid GROUP BY guildid")) {
        while (sql.next()) {
            QListWidgetItem *item = new QListWidgetItem(ui->listWidgetTagOrGuild);
            item->setData(Qt::DisplayRole, "Guild: " + sql.value(1).toString());
            item->setData(Qt::CheckStateRole, Qt::Unchecked);
            item->setData(Qt::UserRole, sql.value(0).toString());
            ui->listWidgetTagOrGuild->addItem(item);
        }
    }
    /* tags */
    if (sql.exec("SELECT id, name FROM tags")) {
        while (sql.next()) {
            QListWidgetItem *item = new QListWidgetItem(ui->listWidgetTagOrGuild);
            item->setData(Qt::DisplayRole, "Tag: " + sql.value(1).toString());
            item->setData(Qt::CheckStateRole, Qt::Unchecked);
            item->setData(Qt::UserRole, sql.value(0).toInt());
            ui->listWidgetTagOrGuild->addItem(item);
        }
    }
}

void ActionConsumeDialog::populateDefendGuild()
{
    QSqlQuery sql;

    if (sql.exec("SELECT g.id, g.name FROM guilds AS g "
                 "INNER JOIN guildids AS m ON m.guildId = g.id "
                 "GROUP BY g.id")) {
        ui->comboBoxDefenderGuild->clear();
        while (sql.next()) {
            ui->comboBoxDefenderGuild->addItem(sql.value(1).toString(), sql.value(0).toString());
        }
    }
}
