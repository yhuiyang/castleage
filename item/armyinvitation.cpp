#include "armyinvitation.h"
#include "ui_armyinvitation.h"
#include "castleagehttpclient.h"
#include "qgumboparser.h"

ArmyInvitation::ArmyInvitation(const int accountId, const QString &description, const QString &link, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ArmyInvitation),
    mAccountId(accountId),
    mAcceptLink(link)
{
    ui->setupUi(this);
    ui->labelDescription->setText(description);
}

ArmyInvitation::~ArmyInvitation()
{
    delete ui;
}

void ArmyInvitation::on_btnAccept_clicked()
{
    CastleAgeHttpClient client(mAccountId);
    QByteArray response = client.post_sync(mAcceptLink);
    if (response.isEmpty())
        return;

    QGumboParser gumbo(response.data());
    QList<GumboNode *> spans = gumbo.findNodes(GUMBO_TAG_SPAN, "class", "result_body", Equals);
    for (GumboNode *span : spans) {
        QString t = gumbo.textContent(span).trimmed();
        if (t.startsWith("You and ")) {
            ui->labelDescription->setText(t);
            ui->btnAccept->hide();
            return;
        }
    }
}
