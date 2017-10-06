#include <QtNetwork>
#include "gaehttpclient.h"
#include "armycode.pb.h"
#include "google/protobuf/timestamp.pb.h"

const static QUrl GAE_HOST("https://castleage-helper.appspot.com");
const static QUrl API_URL_ACAP_ANNOUNCE = GAE_HOST.resolved(QUrl("api/acap"));

GAEHttpClient::GAEHttpClient(QObject *parent)
    : QNetworkAccessManager(parent)
{

}

QDateTime GAEHttpClient::acap_announce(QString &armyCode, QString &facebookId)
{
    QNetworkRequest request(API_URL_ACAP_ANNOUNCE);
    request.setRawHeader("Accept", "application/x-protobuf");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-protobuf");
    request.setHeader(QNetworkRequest::UserAgentHeader, "CABrowser v2");

    QEventLoop looper;
    std::string payload;
    com::yhsoftlab::app::cahelper::protobuf::ArmyCodeAnnounce announce;
    announce.set_code(armyCode.toStdString());
    announce.set_fbid(facebookId.toStdString());
    if (announce.SerializeToString(&payload)) {
        QNetworkReply *reply = this->post(request, QByteArray::fromStdString(payload));
        connect(reply, &QNetworkReply::finished, &looper, &QEventLoop::quit);
        looper.exec();
        /*
         * check response
         * 406 Not Acceptable - 'Accept' or 'Content-Type' headers missed or unrecognized.
         * 400 Bad Request - army code or facebook id missed. (This should not be happened)
         * 409 Conflict - Announce before cool down timer expired.
         * 200 OK - Successfully.
         * Other - Real GAE server issues.
         */

        google::protobuf::Timestamp t;
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        //qDebug() << "ACAP Announce response status:" << status;
        switch (status) {
        case 406:
            qDebug() << "Check 'Accept' or 'Content-Type' request headers...";
            break;
        case 400:
            qDebug() << "Check if army code or facebook id missed.";
            break;
        case 409:
            qDebug() << "Previous announce is still valid.";
        case 200:
            if (t.ParseFromString(reply->readAll().toStdString())) {
                //qDebug() << "Announce timestamp" << QString::fromStdString(t.Utf8DebugString());
                qint64 ms = t.seconds() * 1000 + (qint64)(t.nanos() / 1000000);
                return QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::utc());
            } else
                qDebug() << "Failed to parse Timestamp protobuf responsed from GAE server.";
            break;
        default:
            qDebug() << "GAE Internal error:" << status;
            break;
        }
    }

    return QDateTime();
}
