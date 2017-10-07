#ifndef GAEHTTPCLIENT_H
#define GAEHTTPCLIENT_H

#include <QNetworkAccessManager>

class GAEHttpClient : public QNetworkAccessManager
{
public:
    GAEHttpClient(QObject *parent = Q_NULLPTR);
    virtual ~GAEHttpClient() {}

    QDateTime acap_announce(const QString &armyCode, const QString &facebookId);
    QList<QPair<QString, QString>> acap_download_armypool();
};

#endif // GAEHTTPCLIENT_H
