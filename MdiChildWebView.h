#ifndef MDICHILDWEBVIEW_H
#define MDICHILDWEBVIEW_H

#include <QMainWindow>
#include <QWebPage>

class QWebView;
class QLineEdit;
class QComboBox;
class CastleAgeNetworkAccessManager;

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
    void onAddressLineReturnPressed();

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
