#ifndef NEWACCOUNTDIALOG_H
#define NEWACCOUNTDIALOG_H

#include <QDialog>

namespace Ui {
class NewAccountDialog;
}

class NewAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewAccountDialog(QWidget *parent = 0);
    ~NewAccountDialog();

    QString getEmail();
    QString getPassword();
    void getAccount(QString &email, QString &password);

public Q_SLOTS:
    void onEmailChanged(const QString &email);
    void onPasswordChanged(const QString &password);

private:
    Ui::NewAccountDialog *ui;
};

#endif // NEWACCOUNTDIALOG_H
