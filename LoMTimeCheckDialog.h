#ifndef LOMTIMECHECKDIALOG_H
#define LOMTIMECHECKDIALOG_H

#include <QDialog>
#include <QWebPage>
#include <QDateTime>

namespace Ui {
class LoMTimeCheckDialog;
}

class SynchronizedNetworkAccessManager;

class LoMTimeCheckDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoMTimeCheckDialog(QWidget *parent = 0);
    ~LoMTimeCheckDialog();

public slots:
    void onRun();
    void onStop();
    void onGuildIndexChanged(int);
    void onQueryLomLand();

private:
    void ShowLog(const QString &message);
    void postMessageToGuildChat(const QString &message);
    void populateGuild();
    void populateAccount(const QString &guildId);

private:
    Ui::LoMTimeCheckDialog *ui;
    QWebPage page;
    QHash<qlonglong, SynchronizedNetworkAccessManager *> mgrs;
    QHash<QString, int> landProtectedHours;
    QHash<QString, int> landExpireHours;
    QDateTime startTime;
    QTimer *timer;
    qlonglong accountId;
    bool postToGuildChat;
};

#endif // LOMTIMECHECKDIALOG_H
