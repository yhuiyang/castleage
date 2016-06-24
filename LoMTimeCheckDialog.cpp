#include <QtSql>
#include <QDebug>
#include <QWebFrame>
#include <QWebElement>
#include <QWebElementCollection>
#include "LoMTimeCheckDialog.h"
#include "ui_LoMTimeCheckDialog.h"
#include "SynchronizedNetworkAccessManager.h"

LoMTimeCheckDialog::LoMTimeCheckDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoMTimeCheckDialog)
{
    ui->setupUi(this);
    updateUiEnableState(false);

    populateGuild();


    page.settings()->setAttribute(QWebSettings::AutoLoadImages, false);
    page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onQueryLomLand()));

    this->setAttribute(Qt::WA_DeleteOnClose);
}

LoMTimeCheckDialog::~LoMTimeCheckDialog()
{
    delete ui;
}

void LoMTimeCheckDialog::ShowLog(const QString &message)
{
    if (ui->plainTextEditLog != nullptr)
        ui->plainTextEditLog->appendPlainText(QDateTime::currentDateTime().toString("[MM/dd HH:mm:ss] ") + message);
}

void LoMTimeCheckDialog::postMessageToGuildChat(const QString &message)
{
    if (postToGuildChat) {
        /* reuse network access manager if available */
        SynchronizedNetworkAccessManager *mgr;
        if (mgrs.contains(accountId))
            mgr = mgrs.value(accountId);
        else {
            mgr = new SynchronizedNetworkAccessManager(accountId, this);
            mgrs.insert(accountId, mgr);
        }

        QList<QPair<QString,QString>> form;
        form.append(QPair<QString,QString>("chat_text", message));
        form.append(QPair<QString,QString>("type", "submit_chat"));
        mgr->ca_post("chat_post.php", form);
    }
}

void LoMTimeCheckDialog::populateGuild()
{
    QSqlQuery sql;

    if (sql.exec("SELECT id, name FROM guilds")) {
        while (sql.next()) {
            ui->comboBoxGuild->addItem(sql.value(1).toString(), sql.value(0).toString());
        }
    }
}

void LoMTimeCheckDialog::populateAccount(const QString &guildId)
{
    if (!guildId.isEmpty()) {
        QSqlQuery sql;
        sql.prepare("SELECT id, ifnull(ign, 'UnknownIGN') || ' - ' || email FROM accounts LEFT JOIN igns ON igns.accountId = accounts.id INNER JOIN guildids ON guildids.accountId = accounts.id WHERE guildId = :guildId");
        sql.bindValue(":guildId", guildId);
        if (sql.exec()) {
            ui->comboBoxAccount->clear();
            while (sql.next()) {
                ui->comboBoxAccount->addItem(sql.value(1).toString(), sql.value(0).toLongLong());
            }
        }
    }
}

void LoMTimeCheckDialog::updateUiEnableState(bool scriptRunning)
{
    this->ui->pushButtonRun->setEnabled(!scriptRunning);
    this->ui->pushButtonStop->setEnabled(scriptRunning);
    this->ui->comboBoxGuild->setEnabled(!scriptRunning);
    this->ui->comboBoxAccount->setEnabled(!scriptRunning);
    this->ui->spinBoxCheckInterval->setEnabled(!scriptRunning);
    this->ui->checkBoxPostToGuildChat->setEnabled(!scriptRunning);
}

//
// Slots
//
void LoMTimeCheckDialog::onRun()
{
    QVariant selectedAccountId = ui->comboBoxAccount->currentData();
    if (!selectedAccountId.isValid()) {
        ShowLog("Make sure you have account setup before running this script...");
        return;
    }

    accountId = selectedAccountId.toLongLong();
    postToGuildChat = ui->checkBoxPostToGuildChat->isChecked();

    updateUiEnableState(true);
    ShowLog("Start checking LoM land timer...");

    int interval = ui->spinBoxCheckInterval->value();
    startTime = QDateTime::currentDateTime();
    timer->start(interval * 1000);
}

void LoMTimeCheckDialog::onStop()
{
    timer->stop();
    updateUiEnableState(false);
    ShowLog("Check LoM land timer interrupted by user.");
}

void LoMTimeCheckDialog::onGuildIndexChanged(int guildIndex)
{
    QVariant guildId = ui->comboBoxGuild->itemData(guildIndex);

    /* repopulate account from selected guild */
    if (guildId.isValid())
        populateAccount(guildId.toString());
}

void LoMTimeCheckDialog::onQueryLomLand()
{
    /* check elapsed time */
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = startTime.secsTo(now);
    if (elapsed > 60 * 60) {
        timer->stop();
        updateUiEnableState(false);
        ShowLog("Check LoM land timer accomplished.");
        return;
    }

    /* reuse network access manager if available */
    SynchronizedNetworkAccessManager *mgr;
    if (mgrs.contains(accountId))
        mgr = mgrs.value(accountId);
    else {
        mgr = new SynchronizedNetworkAccessManager(accountId, this);
        mgrs.insert(accountId, mgr);
    }

    QWebElementCollection collection;
    QWebElement landNameElement, landLinkElement, landStateElement, landStateHourElement, landExpireHourElement;
    QString landName, landLink, landState;
    int intSlot = -1;
    int intProtectedHrs = -1;
    int intExpireHrs = -1;
    QString msg;

    QByteArray response = mgr->ca_get("guildv2_conquest_command.php");
    QWebFrame *frame = page.mainFrame();
    frame->setHtml(response);
    collection = frame->findAllElements("div[style*=\"conq3_mid_notop.jpg\"] > div:nth-child(3) > div:nth-child(1) > div[style*=\"width\"] > div");
    for (QWebElement element: collection) {
        landNameElement = element.findFirst("div:nth-child(1) > div:nth-child(1) > div:nth-child(1)");
        landName = landNameElement.toPlainText();
        if (landName.startsWith('?')) // don't care about land name = "?????\n"
            continue;
        landLinkElement = element.findFirst("div:nth-child(3) > div:nth-child(3) a[href*=\"&slot=\"]");
        landLink = landLinkElement.attribute("href");
        for (QString s: landLink.split('&')) {
            if (s.startsWith("slot")) {
                intSlot = s.split('=')[1].toInt();
                break;
            }
        }
        landStateElement = element.findFirst("div:nth-child(5) > div:nth-child(1) > div:nth-child(1) > span:nth-child(1)");
        landState = landStateElement.toPlainText(); // "PROTECTED" or "BATTLE"
        landStateHourElement = element.findFirst("div:nth-child(5) > div:nth-child(1) > div:nth-child(1) > span:nth-child(2)"); // "X HR"
        if (landState == "PROTECTED") {
            intProtectedHrs = landStateHourElement.toPlainText().split(' ')[0].toInt();

            if (landProtectedHours.contains(landName)) {
                if (landProtectedHours.value(landName) != intProtectedHrs) {
                    msg = landName.trimmed() + "(slot=" + QString::number(intSlot) + ") vulnerable @ " + now.addSecs(intProtectedHrs * 60 * 60).toString("MM/dd HH:mm:ss");
                    ShowLog(msg);
                    postMessageToGuildChat(msg);
                    landProtectedHours.insert(landName, intProtectedHrs);
                }
            } else {
                landProtectedHours.insert(landName, intProtectedHrs);
            }
        }
        landExpireHourElement = element.findFirst("div:nth-child(5) > div:nth-child(2) > div:nth-child(1) > span:nth-child(2)"); // "X HR"
        intExpireHrs = landExpireHourElement.toPlainText().split(' ')[0].toInt();
        if (landExpireHours.contains(landName)) {
            if (landExpireHours.value(landName) != intExpireHrs) {
                msg = landName.trimmed() + "(slot=" + QString::number(intSlot) + ") expire @ " + now.addSecs(intExpireHrs * 60 * 60).toString("MM/dd HH:mm:ss");
                ShowLog(msg);
                postMessageToGuildChat(msg);
                landExpireHours.insert(landName, intExpireHrs);
            }
        } else {
            landExpireHours.insert(landName, intExpireHrs);
        }

        //qDebug() << now.toString() << landName << intSlot << intProtectedHrs << intExpireHrs;
    }
}
