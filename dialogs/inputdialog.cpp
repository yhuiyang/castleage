#include "inputdialog.h"
#include "ui_inputdialog.h"

InputDialog::InputDialog(QWidget *parent, QString inputHint, QString inputLabel) :
    QDialog(parent),
    ui(new Ui::InputDialog)
{
    ui->setupUi(this);

    if (inputLabel.isEmpty())
        ui->verticalLayout->removeWidget(ui->label);
    else
        ui->label->setText(inputLabel);
    if (!inputHint.isEmpty())
        ui->lineEdit->setPlaceholderText(inputHint);
}

InputDialog::~InputDialog()
{
    delete ui;
}

QString InputDialog::getInput()
{
    return ui->lineEdit->text();
}
