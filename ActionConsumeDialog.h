#ifndef ACTIONCONSUMEDIALOG_H
#define ACTIONCONSUMEDIALOG_H

#include <QDialog>
#include <QWebPage>

namespace Ui {
class ActionConsumeDialog;
}

class QListWidgetItem;
class SynchronizedNetworkAccessManager;

class ActionConsumeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActionConsumeDialog(QWidget *parent = 0);
    ~ActionConsumeDialog();

public slots:
    void onAttackerTagOrGuildItemChanged(QListWidgetItem *item);
    void onRefreshLandDefenders(bool checked = false);
    void populateDefendGuildOfficersAndMembers(int selectedGuildIndex);
    void onStartActionConsume();
    void onStopActionConsume();

private:
    void showLog(const QString &message);
    SynchronizedNetworkAccessManager *getNetworkAccessManager(const qlonglong accountId, const QByteArray &accountEmail);
    void populateAttackTagOrGuild();
    void populateDefendGuild();
    void disableMofifyConfig(bool disable);
    QString tickToTime(const QString &tick);
    void parseActionAndTimeLeftFromWebFrame(const QString &ign, QWebFrame *htmlFrame, int &actionLeft, int &tickLeft, QString &timeLeft);
    void parseAttackerTokenLeft(const QString &ign, QWebFrame *htmlFrame, int &tokenLeft);
    QList<QList<QPair<QString,QString>>> parseAttackerHitParams(const QString &ign, QWebFrame *htmlFrame);
    QList<QPair<QString,QString>> findHitParam(QList<QList<QPair<QString,QString>>> hitParams, QString target_fbid);

private:
    Ui::ActionConsumeDialog *ui;
    QList<QVariant> _checkedAttackerTagIdOrGuildId;
    QWebPage _page;
    QHash<QString, SynchronizedNetworkAccessManager *> _accountMgrs;
};

#endif // ACTIONCONSUMEDIALOG_H
