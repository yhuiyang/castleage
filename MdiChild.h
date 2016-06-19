#ifndef MDICHILD_H
#define MDICHILD_H

#include <QMainWindow>
#include <QWebPage>

class QWebView;
class QLineEdit;
class QComboBox;
class CastleAgeNetworkAccessManager;

class TabWebView : public QMainWindow
{
    Q_OBJECT
public:
    explicit TabWebView(QWidget *parent = 0);

private:
    void setupActionToolBar();
    void populateToolBar();

signals:
    void requestShowStatusBarMessage(const QString &message);
    void requestUpdateTabInfo(TabWebView * const self, qlonglong accountId);

public slots:
    void onAddressReturnKeyPressed();
    void onAccountIndexChanged(int index);
    void onWebViewLoadStarted();
    void onWebViewLoadProgress(int progress);
    void onWebViewLoadFinished(bool ok);
    void onWebViewUrlChanged(const QUrl &url);
    void onWebPageLinkHovered(const QString &link, const QString &title, const QString &textContent);
    void onCastleAgeLoginResult(qlonglong accountId, bool successful);

private:
    QLineEdit *_address;
    QWebView *_webView;
    CastleAgeNetworkAccessManager *_netMgr;
    QComboBox *_comboBoxAccount;
};

class MdiChild : public QMainWindow
{
    Q_OBJECT
public:
    explicit MdiChild(QWidget *parent = 0);
    int addTabWebView();

signals:

public slots:
    void onTabRequestUpdateTabInfo(TabWebView * const tabPage, qlonglong accountId);
    void onTabRequestShowStatusBarMessage(const QString &message);

private:

private:
    QTabWidget *_tabWidget;
};

#endif // MDICHILD_H
