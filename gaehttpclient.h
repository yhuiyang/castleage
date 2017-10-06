#ifndef GAEHTTPCLIENT_H
#define GAEHTTPCLIENT_H

#include <QNetworkAccessManager>

class GAEHttpClient : public QNetworkAccessManager
{
public:
    GAEHttpClient(QObject *parent = Q_NULLPTR);
    virtual ~GAEHttpClient() {}

    QDateTime acap_announce(const QString &armyCode, const QString &facebookId);
};

#endif // GAEHTTPCLIENT_H
