#include <QtSql>
#include "armymemberdialog.h"
#include "ui_armymemberdialog.h"
#include "castleagehttpclient.h"
#include "qgumboparser.h"
#include "armyinvitation.h"

//---------------------------------------------------------------------------
#define PROPERTY_ARMY_SUGGESTION "ArmySuggestion"

ArmyMemberDialog::ArmyMemberDialog(const int accountId, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ArmyMemberDialog),
    ACCOUNT_ID(accountId)
{
    ui->setupUi(this);
    ui->pushButton->hide();
    ui->groupBox->hide();
    //this->adjustSize();

    /* update pushButton based on army suggestion */
    QStringList armySuggestion = getArmySuggestion();
    ui->pushButton->setProperty(PROPERTY_ARMY_SUGGESTION, armySuggestion);
    int armySuggestionSize = armySuggestion.size();
    if (armySuggestionSize > 1)
        ui->pushButton->setText(QString("There are %1 army suggestions").arg(armySuggestionSize));
    else if (armySuggestionSize == 1)
        ui->pushButton->setText("There is 1 army suggestion");
    else {
        ui->pushButton->setText("There is no army suggestion");
        ui->pushButton->setDisabled(true);
    }

    /* check allies news feed */
    checkAlliesNewsFeed();
}

ArmyMemberDialog::~ArmyMemberDialog()
{
    delete ui;
}

QStringList ArmyMemberDialog::getArmySuggestion()
{
    QStringList suggestion;
    QString sql;

#if 1
    sql.append("SELECT p.armyCode ");
    sql.append("FROM acapPool AS p ");
    sql.append("WHERE p.fbId NOT IN (SELECT o.fbId FROM ownedArmies AS o WHERE o.accountId = :accountId)");
    sql.append(" AND p.armyCode IS NOT (SELECT a.armyCode FROM armycodes AS a WHERE a.accountId = :accountId)");
#else
    sql.append("SELECT p.armyCode ");
    sql.append("FROM acapPool AS p ");
    sql.append("LEFT JOIN ownedArmies AS o ON o.fbId = p.fbId AND o.accountId = :accountId ");
    sql.append("LEFT JOIN armycodes AS c ON c.accountId = :accountId ");
    sql.append("WHERE o.fbId IS NULL AND p.armyCode != c.armyCode");
#endif
    QSqlQuery q;
    q.prepare(sql);
    q.bindValue(":accountId", ACCOUNT_ID);

    if (q.exec())
        while (q.next())
            suggestion << q.value(0).toString();
    else
        qWarning() << q.lastError().databaseText() << q.lastError().driverText();

    return suggestion;
}

void ArmyMemberDialog::checkAlliesNewsFeed()
{
    CastleAgeHttpClient *client = new CastleAgeHttpClient(ACCOUNT_ID, this);

    QVector<QPair<QString, QString>> form;
    form.push_back({"feed", "allies"});
    client->post_async("news_feed_view.php", form);
    connect(client, &CastleAgeHttpClient::post_async_response, this, &ArmyMemberDialog::parseAlliesNewsFeed);
}

void ArmyMemberDialog::parseAlliesNewsFeed(const QByteArray &html)
{
    QStringList invitationAcceptLinks;
    QStringList invitationSenders;

    // parse response
    QGumboParser gumbo(html.data());
    GumboNode *army_invitation_recevied_container = gumbo.findNode(GUMBO_TAG_DIV, "id", "alliesFeed_1", Equals);

    // invitation accept links
    QList<GumboNode *> army_invitation_accept_links = gumbo.findNodes(army_invitation_recevied_container, GUMBO_TAG_A, "href", nullptr, Exists);
    for (GumboNode *link : army_invitation_accept_links)
        invitationAcceptLinks << QString::fromUtf8(gumbo.attributeValue(link, "href"));

    // invitation senders
    QList<GumboNode *> army_invitation_descriptions = gumbo.findNodes(army_invitation_recevied_container, GUMBO_TAG_DIV, "style", "news_innercontainer_mid.gif", Contains);
    for (GumboNode *desc : army_invitation_descriptions) {
        const GumboNode *div_level1 = gumbo.getChildByTag(desc, GUMBO_TAG_DIV, 0);
        const GumboNode *div_level2 = gumbo.getChildByTag(div_level1, GUMBO_TAG_DIV, 0);
        QString army_invitation_desc = gumbo.textContent(div_level2);
        invitationSenders << army_invitation_desc.trimmed();
    }

    if (invitationAcceptLinks.size() == invitationSenders.size()) {
        ui->progressBar->hide();
        ui->pushButton->show();
        ui->groupBox->show();

        int n = invitationSenders.size();
        for (int i = 0; i < n; i++) {
            QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
            ArmyInvitation *customWidget = new ArmyInvitation(ACCOUNT_ID, invitationSenders.at(i), invitationAcceptLinks.at(i));
            item->setSizeHint(customWidget->sizeHint()); // without this, can not see customWidget
            ui->listWidget->addItem(item);
            ui->listWidget->setItemWidget(item, customWidget);
        }

        //this->adjustSize();
    } else {
        qWarning() << "Counter of invitation sender and accept link do not match!";
    }
}
