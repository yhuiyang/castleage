#ifndef BROWSER_H
#define BROWSER_H

#include <QMainWindow>
#include <QProgressBar>
#include <QWebEngineView>

class Browser : public QMainWindow
{
    Q_OBJECT
public:
    explicit Browser(QWidget *parent = nullptr);
    ~Browser();

signals:

public slots:

private:
    void setupToolBar();

private:
    QProgressBar *mProgressBar;
    QWebEngineView *mWebView;
};

#endif // BROWSER_H
