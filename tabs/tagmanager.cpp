#include "tagmanager.h"
#include "ui_tagmanager.h"

TagManager::TagManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TagManager)
{
    ui->setupUi(this);
}

TagManager::~TagManager()
{
    delete ui;
}
