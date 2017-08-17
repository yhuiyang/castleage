#include "masterwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QCloseEvent>
#include <QToolBar>
#include "accountmanager.h"


MasterWindow::MasterWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAttribute(Qt::WA_DeleteOnClose, true);

    QToolBar *toolbar = createToolBar();
    addToolBar(toolbar);

    /* CentralWidget is a QTabWidget */
    mTabWidget = new QTabWidget(this);
    mTabWidget->setDocumentMode(true);
    this->setCentralWidget(mTabWidget);

    mTabWidget->addTab(new AccountManager(this), "Account Manager");
}

MasterWindow::~MasterWindow()
{
}

QSize MasterWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * qreal(0.9);
    return size;
}

void MasterWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    deleteLater();
}

QToolBar *MasterWindow::createToolBar()
{
    QToolBar *browserBar = new QToolBar(tr("Browser"));
    browserBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    browserBar->toggleViewAction()->setEnabled(true);

    return browserBar;
}
