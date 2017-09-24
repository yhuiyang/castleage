#ifndef BROWSER_H
#define BROWSER_H

#include <QMainWindow>
#include <QProgressBar>
#include <QWebEngineView>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>


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
    void populateFilter();
    void onFilterIndexChanged(int index);
    void onAccountIndexChanged(int index);

private:
    QProgressBar *mProgressBar;
    QWebEngineView *mWebView;
    QLineEdit *mLineEdit;
    QComboBox *mAccountList;
    QComboBox *mFilterList;
    QCheckBox *mLockComboBox;
};

#endif // BROWSER_H
