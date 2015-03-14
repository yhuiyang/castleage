#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "newaccountdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//
// Slot implementations
//
void MainWindow::onAddAccount()
{
    NewAccountDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        qDebug() << dlg.getEmail();
        qDebug() << dlg.getPassword();
    }

}

void MainWindow::onRemoveAccount(const QString &email)
{
    Q_UNUSED(email);
}

