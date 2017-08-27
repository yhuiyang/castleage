#include "masterwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QCloseEvent>
#include <QToolBar>
#include "accountmanager.h"

enum TAB_PROPERTY {
    ACCOUNT_MANAGER,
};
#define TAB_PROP "tabProp"

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

    QAction *accountMgr = browserBar->addAction("Account Manager");
    connect(accountMgr, &QAction::triggered, this, [this](){
        /* activate old tab if already exists */
        for (int i = 0; i < mTabWidget->count(); i++) {
            QWidget *oldTab = mTabWidget->widget(i);
            if (oldTab->property(TAB_PROP) == TAB_PROPERTY::ACCOUNT_MANAGER) {
                mTabWidget->setCurrentWidget(oldTab);
                return;
            }
        }

        /* create new tab if not exists */
        QWidget *newTab = new AccountManager(this);
        newTab->setProperty(TAB_PROP, TAB_PROPERTY::ACCOUNT_MANAGER);
        mTabWidget->addTab(newTab, "Account Manager");
    });

    return browserBar;
}
