#include "updateaccountdialog.h"
#include "ui_updateaccountdialog.h"

UpdateAccountDialog::UpdateAccountDialog(const int accountId, const QString &email, const QString &password, bool reveal, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateAccountDialog)
{
    ui->setupUi(this);

    ui->lineEditAccountId->setText(QString::number(accountId));
    ui->lineEditEmail->setText(email);
    ui->lineEditPassword->setText(password);
    ui->lineEditPassword->setEchoMode(reveal ? QLineEdit::Normal : QLineEdit::Password);
    ui->checkBoxRevealPassword->setChecked(reveal);
}

UpdateAccountDialog::~UpdateAccountDialog()
{
    delete ui;
}

QString UpdateAccountDialog::getEmail()
{
    return ui->lineEditEmail->text();
}

QString UpdateAccountDialog::getPassword()
{
    return ui->lineEditPassword->text();
}

void UpdateAccountDialog::on_checkBoxRevealPassword_clicked(bool checked)
{
    ui->lineEditPassword->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}
