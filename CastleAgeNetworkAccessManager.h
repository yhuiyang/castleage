#ifndef CASTLEAGENETWORKACCESSMANAGER_H
#define CASTLEAGENETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

class QNetworkDiskCache;

class CastleAgeNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    CastleAgeNetworkAccessManager(qlonglong accountId, QObject *parent = 0);
    virtual ~CastleAgeNetworkAccessManager();

    void switchAccount(qlonglong accountId);
    qlonglong currentAccount() const;

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0);

private:
    void dumpRequestHeader(const QNetworkRequest &request);
    void dumpRequestBody(QIODevice *outgoingData, qint64 length = 0);
    void storeCookie(QByteArray setCookieHeader);
    void loadCookie();

signals:

public slots:
    void onAuthenticationRequired(QNetworkReply * reply, QAuthenticator * authenticator);
    void onEncrypted(QNetworkReply * reply);
    void onFinished(QNetworkReply * reply);
    void onNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);
    void onNetworkSessionConnected();
    void onPreSharedKeyAuthenticationRequired(QNetworkReply * reply, QSslPreSharedKeyAuthenticator * authenticator);
    void onProxyAuthenticationRequired(const QNetworkProxy & proxy, QAuthenticator * authenticator);
    void onSslErrors(QNetworkReply * reply, const QList<QSslError> & errors);

private:
    const static QUrl web3_base;
    const static QUrl web3_login;
    const static QUrl web4_login;
    const static QUrl web3_home;
    const static QUrl web4_home;
    qlonglong _currentAccountId;
    QNetworkDiskCache *_cache;
};

#endif // CASTLEAGENETWORKACCESSMANAGER_H
