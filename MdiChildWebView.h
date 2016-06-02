#ifndef MDICHILDWEBVIEW_H
#define MDICHILDWEBVIEW_H

#include <QMainWindow>
#include <QObject>
#include <QWebPage>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkAccessManager>

class QWebView;
class QLineEdit;
class QWebSecurityOrigin;
class QWebFrame;
class QWebHistoryItem;
class QNetworkRequest;
class QNetworkReply;
class QAuthenticator;
class QSslPreSharedKeyAuthenticator;
class QComboBox;

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
    void dumpRequestBody(QIODevice *outgoingData);
    void storeCookie(QByteArray setCookieHeader);
    void loadCookie();

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
};

class MdiChildWebView : public QMainWindow
{
    Q_OBJECT
public:
    explicit MdiChildWebView(QWidget *parent = 0);

signals:

public slots:
    void onWebViewIconChanged();
    void onWebViewLinkClicked(const QUrl &url);
    void onWebViewLoadFinished(bool ok);
    void onWebViewLoadProgress(int progress);
    void onWebViewLoadStarted();
    void onWebViewSelectionChanged();
    void onWebViewStatusBarMessage(const QString &text);
    void onWebViewTitleChanged(const QString &title);
    void onWebViewUrlChanged(const QUrl &url);
    void onWebPageApplicationCacheQuotaExceeded(QWebSecurityOrigin * origin, quint64 defaultOriginQuota, quint64 totalSpaceNeeded);
    void onWebPageContentsChanged();
    void onWebPageDatabaseQuotaExceeded(QWebFrame * frame, QString databaseName);
    void onWebPageDownloadRequested(const QNetworkRequest & request);
    void onWebPageFeaturePermissionRequestCanceled(QWebFrame * frame, QWebPage::Feature feature);
    void onWebPageFeaturePermissionRequested(QWebFrame * frame, QWebPage::Feature feature);
    void onWebPageFrameCreated(QWebFrame * frame);
    void onWebPageGeometryChangeRequested(const QRect & geom);
    void onWebPageLinkClicked(const QUrl & url);
    void onWebPageLinkHovered(const QString & link, const QString & title, const QString & textContent);
    void onWebPageLoadFinished(bool ok);
    void onWebPageLoadProgress(int progress);
    void onWebPageLoadStarted();
    void onWebPageMenuBarVisibilityChangeRequested(bool visible);
    void onWebPageMicroFocusChanged();
    void onWebPagePrintRequested(QWebFrame * frame);
    void onWebPageRepaintRequested(const QRect & dirtyRect);
    void onWebPageRestoreFrameStateRequested(QWebFrame * frame);
    void onWebPageSaveFrameStateRequested(QWebFrame * frame, QWebHistoryItem * item);
    void onWebPageScrollRequested(int dx, int dy, const QRect & rectToScroll);
    void onWebPageSelectionChanged();
    void onWebPageStatusBarMessage(const QString & text);
    void onWebPageStatusBarVisibilityChangeRequested(bool visible);
    void onWebPageToolBarVisibilityChangeRequested(bool visible);
    void onWebPageUnsupportedContent(QNetworkReply * reply);
    void onWebPageViewportChangeRequested();
    void onWebPageWindowCloseRequested();
    void onAccountComboBoxIndexChanged(int row);

private:
    void setupToolBarAndStatusBar();
    qlonglong getCurrentAccountIdFromComboBox();

private:
    QWebView *_view;
    QLineEdit *_address;
    CastleAgeNetworkAccessManager *_netMgr;
    QComboBox *_accountComboBox;
};

#endif // MDICHILDWEBVIEW_H
