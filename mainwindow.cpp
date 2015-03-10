#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    qDebug() << "Add Account clicked";
}

void MainWindow::onRemoveAccount(const QString &email)
{
    Q_UNUSED(email);
}

