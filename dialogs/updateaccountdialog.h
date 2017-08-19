#ifndef UPDATEACCOUNTDIALOG_H
#define UPDATEACCOUNTDIALOG_H

#include <QDialog>

namespace Ui {
class UpdateAccountDialog;
}

class UpdateAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateAccountDialog(QWidget *parent = 0, QString email = QString(), QString password = QString(), bool reveal = false);
    ~UpdateAccountDialog();

    QString getEmail();
    QString getPassword();

private slots:
    void on_checkBoxRevealPassword_clicked(bool checked);

private:
    Ui::UpdateAccountDialog *ui;
};

#endif // UPDATEACCOUNTDIALOG_H
