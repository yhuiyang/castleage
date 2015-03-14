#include <QPushButton>
#include "newaccountdialog.h"
#include "ui_newaccountdialog.h"

NewAccountDialog::NewAccountDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewAccountDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
}

NewAccountDialog::~NewAccountDialog()
{
    delete ui;
}

QString NewAccountDialog::getEmail()
{
    return ui->editEmail->text();
}

QString NewAccountDialog::getPassword()
{
    return ui->editPassword->text();
}

void NewAccountDialog::getAccount(QString &email, QString &password)
{
    email = ui->editEmail->text();
    password = ui->editPassword->text();
}

void NewAccountDialog::onEmailChanged(const QString &email)
{
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(!email.isEmpty() && !ui->editPassword->text().isEmpty());
}

void NewAccountDialog::onPasswordChanged(const QString &password)
{
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(!password.isEmpty() && !ui->editEmail->text().isEmpty());
}
