#include <QtSql>
#include <QWebFrame>
#include <QWebElement>
#include <QWebElementCollection>
#include "ActionConsumeDialog.h"
#include "ui_ActionConsumeDialog.h"
#include "SynchronizedNetworkAccessManager.h"

ActionConsumeDialog::ActionConsumeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActionConsumeDialog)
{
    ui->setupUi(this);

    _page.settings()->setAttribute(QWebSettings::AutoLoadImages, false);
    _page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
    setAttribute(Qt::WA_DeleteOnClose);

    /* attackers */
    populateAttackTagOrGuild();
    connect(ui->listWidgetTagOrGuild, &QListWidget::itemChanged, this, &ActionConsumeDialog::onAttackerTagOrGuildItemChanged);

    /* defenders */
    connect(ui->comboBoxDefenderGuild, SIGNAL(currentIndexChanged(int)), this, SLOT(populateDefendGuildOfficersAndMembers(int)));
    populateDefendGuild();
    connect(ui->pushButtonRefreshLand, &QPushButton::clicked, this, &ActionConsumeDialog::onRefreshLandDefenders);

    /* controls */
    connect(ui->pushButtonStart, &QPushButton::pressed, this, &ActionConsumeDialog::onStartActionConsume);
    connect(ui->pushButtonStop, &QPushButton::clicked, this, &ActionConsumeDialog::onStopActionConsume);
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
    } else {
        if (!_checkedAttackerTagIdOrGuildId.removeOne(tagIdOrGuildId))
            qWarning() << "remove tag id or guild id that didn't exist.";
    }

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
                        "LEFT JOIN igns AS i ON i.accountId = a.id "
                        "INNER JOIN account_tag_mapping AS m ON m.accountId = a.id AND m.tagId = :tagId "
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
    Q_UNUSED(checked);
    SynchronizedNetworkAccessManager *mgr;
    QList<QPair<QString,QString>> form;
    QList<QPair<QString,QString>> qs;
    QByteArray response;
    QWebFrame *frame;
    QSqlQuery sql;

    showLog("Refresh land defender...");

    QVariant selectedOfficerId = ui->comboBoxDefenderGuildOfficer->itemData(ui->comboBoxDefenderGuildOfficer->currentIndex());
    QVariant selectedMemberId = ui->comboBoxDefenderGuildMember->itemData(ui->comboBoxDefenderGuildMember->currentIndex());
    if (!selectedOfficerId.isValid() && !selectedMemberId.isValid()) {
        showLog("Please select valid officer or member to refresh land defenders!");
        return;
    }

    ui->listWidgetDefenders->clear();

    // check which accountId/accountIgn is used to refresh land defenders...
    bool accountIsOfficer = selectedOfficerId.isValid();
    QString accountIgn;
    QByteArray accountEmail;
    qlonglong accountId = selectedOfficerId.isValid() ? selectedOfficerId.toLongLong() : selectedMemberId.toLongLong();
    sql.prepare("SELECT ifnull(ign, email), email FROM accounts "
                "LEFT JOIN igns ON accountId = id "
                "WHERE id = :accountId");
    sql.bindValue(":accountId", accountId);
    if (sql.exec() && sql.next()) {
        accountIgn = sql.value(0).toString();
        accountEmail = sql.value(1).toByteArray();
    }
    showLog("Use account " + accountIgn + " to refresh land defenders.");

    // prepare land data, network access manager and parse frame
    QString selectedGuildId = ui->comboBoxDefenderGuild->currentData().toString();
    int selectedSlotId = ui->spinBoxLandId->value();
    qs.append(QPair<QString,QString>("guild_id", selectedGuildId));
    qs.append(QPair<QString,QString>("slot", QString::number(selectedSlotId)));
    mgr = getNetworkAccessManager(accountId, accountEmail);
    frame = _page.mainFrame();

    // refresh land defenders...
    // first, assume land is in protected state
    if (accountIsOfficer) {
        showLog("Officer account with suitable skill powers equiped can refresh land defenders when land is in either protected or vulnerable state.");

        response = mgr->ca_post("guildv2_conquest_expansion_fort.php", form, qs);
        if (!response.isEmpty()) {
            showLog("It seems this land is in protected state now.");
            frame->setHtml(response);
            QWebElementCollection removeDefenderButtons = frame->findAllElements("div#your_guild_member_list_1 > div form[onsubmit*=\"guildv2_conquest_expansion_fort.php\"]");
            int child_index = 0;
            for (QWebElement btn: removeDefenderButtons) {
                child_index++;
                QWebElement member_id = btn.findFirst("input[name=\"member_id\"]");
                if (!member_id.isNull()) {
                    QString member_fbid = member_id.attribute("value");
                    QString member_ign;
                    sql.prepare("SELECT ifnull(i.ign, a.email) FROM igns AS i "
                                "INNER JOIN accounts AS a ON a.id = i.accountId "
                                "INNER JOIN fbids AS f ON a.id = f.accountId AND f.fbid = :fbid");
                    sql.bindValue(":fbid", member_fbid);
                    if (sql.exec() && sql.next())
                        member_ign = sql.value(0).toString();
                    else { // not found account with specific fbid in db, it usuauly means it is not one of our PWS accounts.
                        QWebElement member_name = frame->findFirstElement("div#your_guild_member_list_1 > div:nth-child(" + QString::number(child_index) + ") > div:nth-child(5) > div:nth-child(1) > div:nth-child(2) > div:nth-child(1)");
                        if (!member_name.isNull())
                            member_ign = member_name.toPlainText();
                    }
                    showLog("Find defender: " + member_ign + "(" + member_fbid + ")");

                    // insert into defender list widgets
                    QListWidgetItem *defender = new QListWidgetItem(ui->listWidgetDefenders);
                    defender->setData(Qt::DisplayRole, member_ign.isEmpty() ? member_fbid : member_ign);
                    defender->setData(Qt::UserRole, member_fbid);
                    defender->setData(Qt::CheckStateRole, Qt::Unchecked);
                    ui->listWidgetDefenders->addItem(defender);
                } else {
                    // If there is no 'member_id' field, this is usually a 'JOIN POSITION' button at bottom for this account.
                    // This can be ignored safely.
                }
            }
            showLog("Refresh land defenders completedly.");
            return;
        } else {
            showLog("It seems this land is not in protected state now, let's try refresh defender in vulnerable state.");
        }
    } else {
        showLog("Member account with suitable skill powers equiped can refresh land defenders only when land is in vulnerable state.");
        showLog("If you believe this land is in protected state now, please re-select an officer account to refresh land defenders.");
    }

    // otherwise, assume land is in vulnerable state
    response = mgr->ca_post("guildv2_conquest_expansion.php", form, qs);
    if (!response.isEmpty()) {
        showLog("It seems this land is in vulnerable state now.");
        frame->setHtml(response);

        QString previous_defender_fbid;
        int defender_index = 0;
        QWebElementCollection names = frame->findAllElements("div#your_guild_member_list_1 > div:nth-child(2n+1) > div:nth-child(1) > div:nth-child(4) > div:nth-child(1) > div:nth-child(1) > div:nth-child(1)");
        QWebElementCollection actions = frame->findAllElements("div#your_guild_member_list_1 > div form[onsubmit*=\"guildv2_conquest_expansion.php\"]");
        for (QWebElement action: actions) {
            QWebElement defender_id = action.findFirst("input[name=\"target_id\"]");
            if (!defender_id.isNull()) {
                QString defender_fbid = defender_id.attribute("value");

                // if equip attack skill, there will be repeated attack forms, skip forms for same defender_fbid
                if (!defender_fbid.compare(previous_defender_fbid))
                    continue;
                else
                    previous_defender_fbid = defender_fbid;
                defender_index++;
                QString defender_ign;
                sql.prepare("SELECT ifnull(i.ign, a.email) FROM igns AS i "
                            "INNER JOIN accounts AS a ON a.id = i.accountId "
                            "INNER JOIN fbids AS f ON a.id = f.accountId AND f.fbid = :fbid");
                sql.bindValue(":fbid", defender_fbid);
                if (sql.exec() && sql.next())
                    defender_ign = sql.value(0).toString();
                else { // not found account with specific fbid in db, it usuauly means it is not one of our PWS accounts.
                      defender_ign = names.at(defender_index - 1).toPlainText();
                }
                showLog("Find defender: " + defender_ign + "(" + defender_fbid + ")");

                // insert into defender list widgets
                QListWidgetItem *defender = new QListWidgetItem(ui->listWidgetDefenders);
                defender->setData(Qt::DisplayRole, defender_ign.isEmpty() ? defender_fbid : defender_ign);
                defender->setData(Qt::UserRole, defender_fbid);
                defender->setData(Qt::CheckStateRole, Qt::Unchecked);
                ui->listWidgetDefenders->addItem(defender);
            }
        }

        showLog("Refresh land defenders completedly.");
        return;
    }

    showLog("It seems this land is not in vulnerable state.");
    showLog("Make sure refresh defenders when land is in protected or vulnerable state, instead of collect or hunt state.");

    return;
}

void ActionConsumeDialog::onStartActionConsume()
{
    disableMofifyConfig(true);

    /* attackers */
    QList<QPair<QByteArray, qlonglong>> attackers;
    for (int r = 0; r < ui->tableWidgetAttackers->rowCount(); r++) {
        QTableWidgetItem *attacker_email_cell = ui->tableWidgetAttackers->item(r, 1);
        attackers.append(QPair<QByteArray, qlonglong>(attacker_email_cell->data(Qt::DisplayRole).toByteArray(), attacker_email_cell->data(Qt::UserRole).toLongLong()));
    }

    /* target defenders */
    QList<QString> targets;
    for (int r = 0; r < ui->listWidgetDefenders->count(); r++) {
        QListWidgetItem *defender = ui->listWidgetDefenders->item(r);
        if (defender->data(Qt::CheckStateRole) == Qt::Checked)
            targets.append(defender->data(Qt::UserRole).toString());
    }

    if (attackers.count() == 0 || targets.count() == 0) {
        disableMofifyConfig(false);
        if (attackers.count() == 0)
            showLog("You didn't select any attackers.");
        if (targets.count() == 0)
            showLog("You didn't select any target defenders.");
        return;
    }

    /* options */
    QString defenderGuildId = ui->comboBoxDefenderGuild->currentData().toString();
    bool bUseAllToken = ui->radioButtonTokenUseAll->isChecked();
    bool bUseAtMostToken = ui->radioButtonTokenUseAtMost->isChecked();
    bool bUseTokenUntil = ui->radioButtonTokenUseUntil->isChecked();
    int useAtMostToken = bUseAtMostToken ? ui->spinBoxUseAtMostTokens->value() : -1;
    int useTokenUntil = bUseTokenUntil ? ui->spinBoxUseUntil->value() : -1;
    int stopActionLeft = ui->spinBoxStopActionLeft->value();

    /* hit */
    QSqlQuery sql;
    QString target;
    QString attackerIgn;
    QWebFrame *frame = _page.mainFrame();
    QList<QPair<QString,QString>> empty;
    QList<QPair<QString,QString>> hitForm;
    QList<QPair<QString,QString>> qs;
    qs.append(QPair<QString,QString>("guild_id", ui->comboBoxDefenderGuild->currentData().toString()));
    qs.append(QPair<QString,QString>("slot", QString::number(ui->spinBoxLandId->value())));

    qsrand((uint)QTime::currentTime().msec());

    SynchronizedNetworkAccessManager *attackerNAM;
    QByteArray response;
    sql.prepare("SELECT ifnull(i.ign, a.email) FROM accounts AS a LEFT JOIN igns AS i ON i.accountId = a.id WHERE a.id = :accountId");
    for (QPair<QByteArray, qlonglong> attacker : attackers) {
        sql.bindValue(":accountId", attacker.second);
        if (sql.exec() && sql.next())
            attackerIgn = "[" + sql.value(0).toString() + "]";
        else
            attackerIgn = "[XXX]";
        attackerNAM = getNetworkAccessManager(attacker.second, attacker.first);

        /* check land */
        response = attackerNAM->ca_post("guildv2_conquest_expansion.php", empty, qs);
        if (response.isEmpty()) {
            showLog(attackerIgn + ": can not check land state. Make sure land is in vulnerable state now!");
            continue;
        }

        int actionLeft;
        int tickLeft;
        QString timeLeft;
        int tokenLeft, initialToken;
        QList<QList<QPair<QString,QString>>> hitParams;

        frame->setHtml(response);

        /* action left & time left */
        parseActionAndTimeLeftFromWebFrame(attackerIgn, frame, actionLeft, tickLeft, timeLeft);
        if (actionLeft <= 0) {
            showLog(attackerIgn + ": no action left, ignore.");
            continue;
        }
        /* attacker token left */
        parseAttackerTokenLeft(attackerIgn, frame, tokenLeft);
        initialToken = tokenLeft;
        /* hit defender params: note, if attacker has multiple attack methods, the params count will be increased. */
        hitParams = parseAttackerHitParams(attackerIgn, frame);

        target = targets.at(qrand() % targets.count());
        hitForm = findHitParam(hitParams, target);
        if (hitForm.size() == 0) {
            showLog(attackerIgn + ": can not find param to hit specific target. Ignore.");
            continue;
        }

        showLog(attackerIgn + ": time left: " + timeLeft + ", action left: " + QString::number(actionLeft) + ", token left: " + QString::number(tokenLeft) + ", next target: " + target);
        while (
               ((bUseAllToken || bUseAtMostToken) ? (tokenLeft > 0) : (tokenLeft > useTokenUntil))
               && ((bUseAllToken || bUseTokenUntil) ? (initialToken - tokenLeft) < 20 : (initialToken - tokenLeft) < useAtMostToken)
               && actionLeft > stopActionLeft
               && hitForm.size() > 0
               //&& false
            )
        {
            response = attackerNAM->ca_post("guildv2_conquest_expansion.php", hitForm, empty);
            if (response.isEmpty()) {
                showLog(attackerIgn + ": can not check response after hit.");
                break;
            }

            frame->setHtml(response);
            parseActionAndTimeLeftFromWebFrame(attackerIgn, frame, actionLeft, tickLeft, timeLeft);
            parseAttackerTokenLeft(attackerIgn, frame, tokenLeft);
            target = targets.at(qrand() % targets.count());
            hitForm = findHitParam(hitParams, target);
            showLog(attackerIgn + ": time left: " + timeLeft + ", action left: " + QString::number(actionLeft) + ", token left: " + QString::number(tokenLeft) + ", next target: " + target);
        }
    }

    disableMofifyConfig(false);
}

void ActionConsumeDialog::onStopActionConsume()
{
    disableMofifyConfig(false);
}

//
// Internal helper methods
//
void ActionConsumeDialog::showLog(const QString &message)
{
    QString output = QDateTime::currentDateTime().toString("[MM/dd HH:mm:ss] ") + message;
    ui->plainTextEditLog->appendPlainText(output);
    qDebug() << output;
}

SynchronizedNetworkAccessManager * ActionConsumeDialog::getNetworkAccessManager(const qlonglong accountId, const QByteArray &accountEmail)
{
    if (accountId > 0 && _accountMgrs.contains(QString::number(accountId)))
        return _accountMgrs.value(QString::number(accountId));
    if (!accountEmail.isEmpty() && _accountMgrs.contains(accountEmail))
        return _accountMgrs.value(accountEmail);

    SynchronizedNetworkAccessManager *mgr = new SynchronizedNetworkAccessManager(accountId, this);
    if (accountId > 0)
        _accountMgrs.insert(QString::number(accountId), mgr);
    if (!accountEmail.isEmpty())
        _accountMgrs.insert(accountEmail, mgr);
    return mgr;
}

void ActionConsumeDialog::populateAttackTagOrGuild()
{
    QSqlQuery sql;

    ui->listWidgetTagOrGuild->clear();
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

void ActionConsumeDialog::populateDefendGuildOfficersAndMembers(int selectedGuildIndex)
{
    QVariant defendGuildData = ui->comboBoxDefenderGuild->itemData(selectedGuildIndex);

    if (!defendGuildData.isValid()) {
        qWarning() << "No data assigned to this guild item!";
        return;
    }

    QString guildId = defendGuildData.toString();
    QSqlQuery sql;

    // load officers or master
    sql.prepare("SELECT a.id, ifnull(i.ign, a.email) FROM accounts AS a "
                "LEFT JOIN igns AS i ON i.accountId = a.id "
                "INNER JOIN roles AS r ON r.accountId = a.id "
                "INNER JOIN guildids AS g ON g.accountId = a.id AND g.guildId = :guildId "
                "ORDER BY a.id");
    sql.bindValue(":guildId", guildId);
    if (sql.exec()) {
        ui->comboBoxDefenderGuildOfficer->clear();
        ui->comboBoxDefenderGuildOfficer->addItem("--- Guild Master/Officers ---");
        while (sql.next())
            ui->comboBoxDefenderGuildOfficer->addItem(sql.value(1).toString(), sql.value(0).toLongLong());
    }

    // load members
    sql.prepare("SELECT a.id, ifnull(i.ign, a.email) FROM accounts AS a "
                "LEFT JOIN igns AS i ON i.accountId = a.id "
                "INNER JOIN guildids AS g ON g.accountId = a.id AND g.guildId = :guildId "
                "WHERE a.id NOT IN (SELECT r.accountId FROM roles AS r) "
                "ORDER BY a.id");
    sql.bindValue(":guildId", guildId);
    if (sql.exec()) {
        ui->comboBoxDefenderGuildMember->clear();
        ui->comboBoxDefenderGuildMember->addItem("--- Guild Members ---");
        while (sql.next())
            ui->comboBoxDefenderGuildMember->addItem(sql.value(1).toString(), sql.value(0).toLongLong());
    }
}

void ActionConsumeDialog::disableMofifyConfig(bool disable) {
    ui->listWidgetTagOrGuild->setEnabled(!disable);
    ui->listWidgetDefenders->setEnabled(!disable);
    ui->groupBoxOptions->setEnabled(!disable);
    ui->groupBoxDefender->setEnabled(!disable);

    ui->pushButtonStart->setEnabled(!disable);
    ui->pushButtonStop->setEnabled(disable);
}

QString ActionConsumeDialog::tickToTime(const QString &tick)
{
    int tickInt = tick.toInt();
    int s = tickInt % 60;
    int m = (tickInt / 60) % 60;
    int h = tickInt / 3600;
    return QString().sprintf("%02d:%02d:%02d", h, m, s);
}

void ActionConsumeDialog::parseActionAndTimeLeftFromWebFrame(const QString &ign, QWebFrame *htmlFrame, int &actionLeft, int &tickLeft, QString &timeLeft)
{
    QWebElement timeLeftElement = htmlFrame->findFirstElement("div > div > input#monsterTickerSecs");
    if (!timeLeftElement.isNull()) {
        QString tickString = timeLeftElement.attribute("value");
        tickLeft = tickString.toInt();
        timeLeft = tickToTime(tickString);
        QWebElement actionLeftElement = timeLeftElement.parent().parent().nextSibling().findFirst("span");
        if (!actionLeftElement.isNull()) {
            QString actionLeftString = actionLeftElement.toPlainText();
            //qDebug() << actionLeftString;
            actionLeft = actionLeftString.toInt(nullptr, 0);
        } else {
            actionLeft = 0;
            showLog(ign + ": can not find action counter.");
        }
    } else {
        showLog(ign + ": can not find time left and action counter.");
        actionLeft = 0;
        tickLeft = 0;
        timeLeft = "00:00:00";
    }
}

void ActionConsumeDialog::parseAttackerTokenLeft(const QString &ign, QWebFrame *htmlFrame, int &tokenLeft)
{
    QWebElement tokenLeftElement = htmlFrame->findFirstElement("div#persistHomeConquestPlateOpen > div:nth-child(2) > div:nth-child(1)");
    if (!tokenLeftElement.isNull()) {
        QString tokenLeftValue = tokenLeftElement.toPlainText();
        bool version1 = tokenLeftValue.startsWith("Tokens:");
        // There are two version of tokenLeftValue
        // 1. "Tokens:\n8"
        // 2. "\n\t\t\t\t\t\t\t\t\t\t\tTokens:\n\n\t\t\t\t\t\t\t\t\t\t\t9\t\t\t\t\t\t\t\t\t\t"
        QStringList tokenLeftSplitResult = tokenLeftValue.split('\n');
        QString tokenLeftString = version1
                ? (tokenLeftSplitResult.length() > 1 ? tokenLeftSplitResult[1] : "")
                : (tokenLeftSplitResult.length() > 3 ? tokenLeftSplitResult[3] : "");
        //qDebug() << tokenLeftString;
        tokenLeft = tokenLeftString.toInt();
    } else {
        showLog(ign + ": can not find hown many token left.");
        tokenLeft = 0;
    }
}

QList<QList<QPair<QString,QString>>> ActionConsumeDialog::parseAttackerHitParams(const QString &ign, QWebFrame *htmlFrame)
{
    Q_UNUSED(ign);
    QList<QList<QPair<QString,QString>>> hitParams;
    QList<QPair<QString,QString>> hitParam;

    QWebElementCollection hitDefenderParams = htmlFrame->findAllElements("div#your_guild_member_list_1 > div form[onsubmit*=\"guildv2_conquest_expansion.php\"]");
    for (QWebElement hitDefenderParam: hitDefenderParams) {
        hitParam.clear();
        QWebElementCollection params = hitDefenderParam.findAll("input[type=hidden]");
        for (QWebElement param: params) {
            hitParam.append(QPair<QString,QString>(param.attribute("name"), param.attribute("value")));
        }
        if (hitParam.size() > 0)
            hitParams.append(hitParam);
    }

    return hitParams;
}

QList<QPair<QString,QString>> ActionConsumeDialog::findHitParam(QList<QList<QPair<QString, QString> > > hitParams, QString target_fbid)
{
    bool fbid_matched, attack_type_matched;
    for (QList<QPair<QString,QString>> hitParam : hitParams) {
        fbid_matched = false;
        attack_type_matched = false;
        for (QPair<QString,QString> param : hitParam) {
            if (param.first == "attack_type" && param.second == "normal")
                attack_type_matched = true;
            else if (param.first == "target_id" && param.second == target_fbid)
                fbid_matched = true;
        }

        if (fbid_matched && attack_type_matched)
            return hitParam;
    }

    return QList<QPair<QString,QString>>();
}
