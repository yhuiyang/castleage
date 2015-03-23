#include <QtNetwork>
#include <QWebFrame>
#include <QWebElementCollection>
#include <QWebElement>
#include <zlib.h>
#include "castleage.h"
#include "constant.h"


const QByteArray CastleAgeRequestManager::USER_AGENT = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.111 Safari/537.36";
const QUrl CastleAgeRequestManager::URL_BASE = QUrl("https://web3.castleagegame.com/castle_ws/");

CastleAgeRequestManager::CastleAgeRequestManager(QString email, QString password, QObject *parent)
    : QNetworkAccessManager(parent),
      _email(email),
      _password(password)
{
    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));

    /* init variables used internally */
    _state = Ready;
    _activeAction = Idle;

    /* setup QWebPage. */
    _page.settings()->setAttribute(QWebSettings::AutoLoadImages, false);
    _page.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
}

CastleAgeRequestManager::~CastleAgeRequestManager()
{

}

///////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////
QNetworkReply * CastleAgeRequestManager::get(QNetworkRequest &request)
{
    qDebug() << "<<< HTTP GET:" << request.url().toString();
    request.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT);
    request.setRawHeader("Accept-Encoding", "gzip");
    _requestSentAtMilliSecs = QDateTime::currentMSecsSinceEpoch();
    return QNetworkAccessManager::get(request);
}

QNetworkReply * CastleAgeRequestManager::post(QNetworkRequest &request, const QStringList &data)
{
    qDebug() << "<<< HTTP POST:" << request.url().toString();
    request.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Accept-Encoding", "gzip");
    QByteArray payload;
    for (int i = 0; i < data.size(); i++)
    {
        payload.append(data.at(i));
        if (i != data.size() - 1)
            payload.append("&");
    }
    _requestSentAtMilliSecs = QDateTime::currentMSecsSinceEpoch();
    return QNetworkAccessManager::post(request, payload);
}

/* Copied from http://stackoverflow.com/a/7351507/1078792 */
QByteArray CastleAgeRequestManager::ungzip(const QByteArray &gzippedData)
{
    if (gzippedData.size() <= 4) {
        qWarning("ungzip: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = gzippedData.size();
    strm.next_in = (Bytef*)(gzippedData.data());

    ret = inflateInit2(&strm, 15 +  32); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR); // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR; // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

qlonglong CastleAgeRequestManager::httpResponseContentLength(QNetworkReply *reply)
{
    bool remote_sent = false;
    qlonglong length = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong(&remote_sent);
    return remote_sent ? length : -1;
}

bool CastleAgeRequestManager::parseUserStats(const QByteArray &data)
{
    bool result = true;
    QHash<enum UserStatKeys, QString> stats;
    QWebElementCollection elemColl;
    QWebElement elem;
    QString attr;
    QStringList tempList;

    /* html data */
    _page.mainFrame()->setHtml(data);

    /* fbid */
    elem = _page.mainFrame()->findFirstElement("div#app_body table.layout a[href^=\"keep.php?user=\"]");
    if (!elem.isNull())
    {
        attr = elem.attribute("href");
        tempList = attr.split("=");
        stats.insert(UserStatKeys::FacebookId, tempList.at(tempList.size() - 1));
    }
    else
        result = false;

    /* in game name, army size. (also include essences number) */
    elemColl = _page.mainFrame()->findAllElements("div#app_body table.layout div[title]");
    if (elemColl.count() == 16)
    {
        stats.insert(UserStatKeys::InGameName, elemColl.at(0).attribute("title"));
        stats.insert(UserStatKeys::ArmySize, elemColl.at(1).attribute("title"));
        stats.insert(UserStatKeys::AttackEssence, elemColl.at(9).attribute("title").mid(1));
        stats.insert(UserStatKeys::DefenseEssence, elemColl.at(11).attribute("title").mid(1));
        stats.insert(UserStatKeys::DamageEssence, elemColl.at(13).attribute("title").mid(1));
        stats.insert(UserStatKeys::HealthEssence, elemColl.at(15).attribute("title").mid(1));
    }
    else
        result = false;

    /* level: in format 'Level: 123' */
    elem = _page.mainFrame()->findFirstElement("div#main_sts_container div[title^=\"Your XP:\"] a[href=\"keep.php\"] > div");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::Level, elem.toPlainText().mid(7));
    }
    else
        result = false;

    /* max energy: in format '123' or '123 (+456)' */
    elem = _page.mainFrame()->findFirstElement("div[onmouseout^=\"hideItemPopup('energy_max',event)\"] > div > div");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::MaxEnergy, elem.toPlainText().split(" ")[0]);
    }
    else
        result = false;

    /* max stamina: in format '123' or '123 (+456)' */
    elem = _page.mainFrame()->findFirstElement("div[onmouseout^=\"hideItemPopup('stamina_max',event)\"] > div > div");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::MaxStamina, elem.toPlainText().split(" ")[0]);
    }
    else
        result = false;

    /* max health: in format '123' or '123 (+456)' */
    elem = _page.mainFrame()->findFirstElement("div[onmouseout^=\"hideItemPopup('health_max',event)\"] > div > div");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::MaxHealth, elem.toPlainText().split(" ")[0]);
    }
    else
        result = false;

    /* attack: in format '123' or '123 (+456)' */
    elem = _page.mainFrame()->findFirstElement("div[onmouseout^=\"hideItemPopup('attack',event)\"] > div > div");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::Attack, elem.toPlainText().split(" ")[0]);
    }
    else
        result = false;

    /* defense: in format '123' or '123 (+456)' */
    elem = _page.mainFrame()->findFirstElement("div[onmouseout^=\"hideItemPopup('defense',event)\"] > div > div");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::Defense, elem.toPlainText().split(" ")[0]);
    }
    else
        result = false;

    /* battle rank: in format 'Battle Rank - Rank Name' */
    elem = _page.mainFrame()->findFirstElement("img[title^=\"Battle Rank -\"]");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::BattleRank, elem.attribute("title").mid(14));
    }
    else
        result = false;

    /* war rank: in format 'War Rank - Rank Name' */
    elem = _page.mainFrame()->findFirstElement("img[title^=\"War Rank -\"]");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::WarRank, elem.attribute("title").mid(11));
    }
    else
        result = false;

    /* conquest rank: in format 'Conquest Rank - Rank Name' */
    elem = _page.mainFrame()->findFirstElement("img[title^=\"Conquest Rank -\"]");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::ConquestRank, elem.attribute("title").mid(16));
    }
    else
        result = false;

    /* festival rank: in format 'Festival Rank - Rank Name' */
    elem = _page.mainFrame()->findFirstElement("img[title^=\"Festival Rank -\"]");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::FestivalRank, elem.attribute("title").mid(16));
    }
    else
        result = false;

    /* guild name & id */
    elem = _page.mainFrame()->findFirstElement("span[title$=\" Guild\"] > a[href]");
    if (!elem.isNull())
    {
        stats.insert(UserStatKeys::GuildName, elem.toPlainText());
        stats.insert(UserStatKeys::GuildId, elem.attribute("href").split("=")[1]);
    }
    else
    {
        /* user is not in any guild */
        elem = _page.mainFrame()->findFirstElement("span[title$=\" Guild\"]");
        if (!elem.isNull())
        {
            stats.insert(UserStatKeys::GuildName, elem.toPlainText()); // This will be 'No Guild'
            stats.insert(UserStatKeys::GuildId, "");
        }
        else
            result = false;
    }

    if (!result)
        qDebug() << "CA may change the keep page, parser need to be modified...";

    emit StatsAvailable(_email, stats);

    return result;
}

///////////////////////////////////////////////////////////////////////////
// Slot implementations
///////////////////////////////////////////////////////////////////////////
void CastleAgeRequestManager::onFinished(QNetworkReply *reply)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError)
    {
        _state = Ready;
        /* TODO: emit error signal */
        qDebug() << ">>> HTTP Response error:" << reply->errorString() << (QDateTime::currentMSecsSinceEpoch() - _requestSentAtMilliSecs) << "ms";
        return;
    }

    qint64 delta = QDateTime::currentMSecsSinceEpoch() - _requestSentAtMilliSecs;
    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    QVariant reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
    qDebug() << ">>> HTTP Response" << status.toInt() << reason.toString() << ", took" << delta << "ms.";

    QVariant redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectTarget.isValid())
    {
        QUrl redirectUrl = redirectTarget.toUrl();
        if (redirectUrl.isRelative())
            redirectUrl = reply->url().resolved(redirectUrl);

        if (_state == WaitActionResponse) // expect redirecting to login page
        {
            if (redirectUrl != URL_BASE.resolved(QUrl("connect_login.php")))
            {
                qDebug() << "need redirect, but not to login form:" << redirectUrl.toString();
                /* TODO: emit error signal */
                _state = Ready;
                return;
            }

            /* send login form data */
            qDebug() << "send login data:" << _email;
            QNetworkRequest request(redirectUrl);
            QStringList loginData;
            loginData << "platform_action=CA_web3_login";
            loginData << "player_email=" + _email;
            loginData << "player_password=" + _password;
            this->post(request, loginData);
            _state = WaitLoginResponse;
        }
        else if (_state == WaitLoginResponse) // expect redirecting to index page
        {
            if (redirectUrl != URL_BASE.resolved(QUrl("index.php")))
            {
                qDebug() << "need redirct, but not to default page:" << redirectUrl.toString();
                /* TODO: emit error signal */
                _state = Ready;
                return;
            }

            /* re-send previous action */
            qDebug() << "login successfully, re-send previous query:" << _activeUrl.toString();
            QNetworkRequest request(_activeUrl);
            if ((_activeAction & 0xF0000) == CastleAgeAction::GetBase)
                this->get(request);
            else if ((_activeAction & 0xF0000) == CastleAgeAction::PostBase)
                this->post(request, _activePayload);
            else
                qDebug() << "Don't remember what is sent in the past...";
            _state = WaitActionResponse;
        }

        return;
    }

    if (_state == WaitLoginResponse)
    {
        /* This usually means wrong account setting after sending login data but no redirect happened. */
        qDebug() << "You probably with wrong email or password";
        _state = Ready;
        return;
    }
    else if (_state == WaitActionResponse)
    {
        QByteArray rawData = reply->readAll();
        bool handled;

        switch (_activeAction)
        {
        case QueryUserStats:
            handled = parseUserStats(ungzip(rawData));
            break;
        default:
            qDebug() << "Not handler for current active action" << _activeAction;
            break;
        }

        _state = Ready;
    }
    else if (_state == Ready)
    {
        qDebug() << "Received reply in ready state. Figure out why this happened.";
        return;
    }
}

///////////////////////////////////////////////////////////////////////////
// Public apis
///////////////////////////////////////////////////////////////////////////
void CastleAgeRequestManager::retrieveStats()
{
    if (_state != Ready)
    {
        qDebug() << "Still waiting for response of precede request, ignore new request.";
        return;
    }

    _activeUrl = URL_BASE.resolved(QUrl("keep.php"));
    QNetworkRequest request(_activeUrl);
    this->get(request);
    _activeAction = QueryUserStats;
    _state = WaitActionResponse;
}
