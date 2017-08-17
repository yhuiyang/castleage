#ifndef ADDACCOUNTDIALOG_H
#define ADDACCOUNTDIALOG_H

#include <QDialog>

namespace Ui {
class AddAccountDialog;
}

class AddAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddAccountDialog(QWidget *parent = 0, QString email = QString(), QString password = QString(), bool reveal = false, bool reopen = false, bool reserveEmail = false, bool reservePassword = false);
    ~AddAccountDialog();

    QString getEmail();
    QString getPassword();
    bool shouldRevealPassword();
    bool shouldReopenSelf();
    bool shouldReserveEmail();
    bool shouldReservePassword();

private slots:
    void on_checkBoxRevealPassword_clicked(bool checked);

    void on_checkBoxReopen_clicked(bool checked);

private:
    Ui::AddAccountDialog *ui;
};

#endif // ADDACCOUNTDIALOG_H
