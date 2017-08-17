#include "addaccountdialog.h"
#include "ui_addaccountdialog.h"

AddAccountDialog::AddAccountDialog(QWidget *parent, QString email, QString password, bool reveal, bool reopen, bool reserveEmail, bool reservePassword) :
    QDialog(parent),
    ui(new Ui::AddAccountDialog)
{
    ui->setupUi(this);
    if (!email.isEmpty())
        ui->lineEditEmail->setText(email);
    if (!password.isEmpty())
        ui->lineEditPassword->setText(password);
    ui->checkBoxRevealPassword->setChecked(reveal);
    ui->checkBoxReopen->setChecked(reopen);
    ui->checkBoxReserveEmail->setChecked(reserveEmail);
    ui->checkBoxReservePassword->setChecked(reservePassword);

    if (reveal)
        ui->lineEditPassword->setEchoMode(QLineEdit::Normal);
    else
        ui->lineEditPassword->setEchoMode(QLineEdit::Password);

    ui->checkBoxReserveEmail->setEnabled(reopen);
    ui->checkBoxReservePassword->setEnabled(reopen);
}

AddAccountDialog::~AddAccountDialog()
{
    delete ui;
}

QString AddAccountDialog::getEmail()
{
    return ui->lineEditEmail->text();
}

QString AddAccountDialog::getPassword()
{
    return ui->lineEditPassword->text();
}

bool AddAccountDialog::shouldRevealPassword()
{
    return ui->checkBoxRevealPassword->checkState() == Qt::Checked;
}

bool AddAccountDialog::shouldReopenSelf()
{
    return ui->checkBoxReopen->checkState() == Qt::Checked;
}

bool AddAccountDialog::shouldReserveEmail()
{
    return ui->checkBoxReserveEmail->checkState() == Qt::Checked;
}

bool AddAccountDialog::shouldReservePassword()
{
    return ui->checkBoxReservePassword->checkState() == Qt::Checked;
}

void AddAccountDialog::on_checkBoxRevealPassword_clicked(bool checked)
{
    ui->lineEditPassword->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void AddAccountDialog::on_checkBoxReopen_clicked(bool checked)
{
    ui->checkBoxReserveEmail->setEnabled(checked);
    ui->checkBoxReservePassword->setEnabled(checked);
}
