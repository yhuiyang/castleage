#ifndef SYNCHRONIZEDNETWORKACCESSMANAGER_H
#define SYNCHRONIZEDNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include <QUrl>

class SynchronizedNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    SynchronizedNetworkAccessManager(qlonglong accountId, QObject *parent = 0);
    virtual ~SynchronizedNetworkAccessManager();
    QByteArray ca_get(QString php, const QList<QPair<QString,QString>> &qs = QList<QPair<QString,QString>>());
    QByteArray ca_post(QString php, const QList<QPair<QString,QString>> &form = QList<QPair<QString,QString>>(), const QList<QPair<QString,QString>> &qs = QList<QPair<QString,QString>>(), bool appendAjax = true);

private:
    bool ca_login();
    QByteArray ca_get(QString php, const QList<QPair<QString,QString>> &qs, bool retryAfterLogin);
    QByteArray ca_post(QString php, const QList<QPair<QString,QString>> &form, const QList<QPair<QString,QString>> &qs, bool appendAjax, bool retryAfterLogin);

    void dumpRequestHeader(const QNetworkRequest &request);
    void dumpRequestBody(const QByteArray &body);

private:
    const static QUrl _WEB3_BASE;
    const static QUrl _WEB3_LOGIN_URL;
    const static QUrl _WEB4_LOGIN_URL;
    const static QUrl _WEB3_HOME_URL;
    const static QUrl _WEB4_HOME_URL;
    const qlonglong _ACCOUNTID;
};

#endif // SYNCHRONIZEDNETWORKACCESSMANAGER_H
