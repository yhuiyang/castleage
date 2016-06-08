#include "ImportAccountDialog.h"
#include "ui_ImportAccountDialog.h"

ImportAccountDialog::ImportAccountDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportAccountDialog)
{
    ui->setupUi(this);
}

ImportAccountDialog::~ImportAccountDialog()
{
    delete ui;
}

void ImportAccountDialog::getAccountData(QString &email, QString &password)
{
    email = ui->email->text();
    password = ui->password->text();
}
