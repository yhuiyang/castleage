#ifndef CASTLE_AGE_HTTP_CLIENT_H
#define CASTLE_AGE_HTTP_CLIENT_H

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QAuthenticator>
#include <QNetworkReply>


class CastleAgeHttpClient : public QNetworkAccessManager
{
    Q_OBJECT
public:
    CastleAgeHttpClient(const int accountId, QObject *parent = Q_NULLPTR);
    virtual ~CastleAgeHttpClient();
    void switchAccount(const int accountId);

    QByteArray get_sync(const QString &php, const QVector<QPair<QString, QString>> &qs = QVector<QPair<QString, QString>>());
    QByteArray post_sync(const QString &php, const QVector<QPair<QString, QString>> &form = QVector<QPair<QString, QString>>(), const QVector<QPair<QString, QString>> &qs = QVector<QPair<QString, QString>>());

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = Q_NULLPTR) override;

private:
    void onProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
    void onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void onFinished(QNetworkReply *reply);
    void onEncrypted(QNetworkReply *reply);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void onPreSharedKeyAuthenticationRequired(QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator);
    void onNetworkSessionConnected();
    void onNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);
    bool execute_login();
    QByteArray ungzip(const QByteArray &gzippedData);
    void dumpHeader(const QNetworkRequest &request);
    void dumpHeader(const QNetworkReply *reply);

private:
    const static QUrl URL_BASE;
    int mAccountId;
};

#endif // CASTLE_AGE_HTTP_CLIENT_H
