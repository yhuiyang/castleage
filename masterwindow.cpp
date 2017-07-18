#include "masterwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QCloseEvent>
#include <QToolBar>

MasterWindow::MasterWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAttribute(Qt::WA_DeleteOnClose, true);

    QToolBar *toolbar = createToolBar();
    addToolBar(toolbar);
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
    browserBar->toggleViewAction()->setEnabled(false);

    return browserBar;
}
