#ifndef CASTLEAGE_H
#define CASTLEAGE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QWebPage>
#include "constant.h"

class CastleAgeRequestManager : public QNetworkAccessManager
{
    Q_OBJECT

    enum State
    {
        Ready,
        WaitActionResponse,
        WaitLoginResponse,
    };

    enum CastleAgeAction
    {
        Idle,
        GetBase = 0x10000,
        QueryUserStats,

        PostBase = 0x20000,
    };

public:
    CastleAgeRequestManager(QString email, QString password, QObject *parent = 0);
    ~CastleAgeRequestManager();

private Q_SLOTS:
    void onFinished(QNetworkReply *reply);

Q_SIGNALS:
    void StatsAvailable(QString email, QHash<enum UserStatKeys, QString> &stats);

public:
    void retrieveStats();

private:
    QNetworkReply *get(QNetworkRequest &request);
    QNetworkReply *post(QNetworkRequest &request, const QStringList &data);
    QByteArray ungzip(const QByteArray &gzippedData);
    qlonglong httpResponseContentLength(QNetworkReply *reply);
    /* parse helpers */
    bool parseUserStats(const QByteArray &data);

    static const QByteArray USER_AGENT;
    static const QUrl URL_BASE;
    const QString _email;
    const QString _password;
    qint64 _requestSentAtMilliSecs;
    QWebPage _page;
    enum State _state;
    QUrl _activeUrl;
    enum CastleAgeAction _activeAction;
    QStringList _activePayload;
};

#endif // CASTLEAGE_H
