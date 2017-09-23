#include "masterwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QCloseEvent>
#include <QToolBar>
#include "browser.h"
#include "accountmanager.h"
#include "tagmanager.h"

enum TAB_PROPERTY {
    ACCOUNT_MANAGER,
    TAG_MANAGER,
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
    QToolBar *browserBar = new QToolBar(tr("Main ToolBar"));
    browserBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    browserBar->toggleViewAction()->setEnabled(false);

    QAction *browser = browserBar->addAction("CaBrowser");
    connect(browser, &QAction::triggered, this, [this](){
        QWidget *newTab = new Browser(this);
        mTabWidget->addTab(newTab, "Browser");
        mTabWidget->setCurrentWidget(newTab);
    });

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
        mTabWidget->setCurrentWidget(newTab);
    });

    QAction *tagMgr = browserBar->addAction("Tag Manager");
    connect(tagMgr, &QAction::triggered, this, [this](){
        /* activate old tab if already exists */
        for (int i = 0; i < mTabWidget->count(); i++) {
            QWidget *oldTab = mTabWidget->widget(i);
            if (oldTab->property(TAB_PROP) == TAB_PROPERTY::TAG_MANAGER) {
                mTabWidget->setCurrentWidget(oldTab);
                return;
            }
        }

        /* create new tab if not exists */
        QWidget *newTab = new TagManager(this);
        newTab->setProperty(TAB_PROP, TAB_PROPERTY::TAG_MANAGER);
        mTabWidget->addTab(newTab, "Tag Manager");
        mTabWidget->setCurrentWidget(newTab);
    });

    return browserBar;
}
