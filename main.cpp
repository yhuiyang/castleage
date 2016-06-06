#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("nobody");
    app.setApplicationName("cabrowser");
    app.setApplicationVersion("0.1");
    MainWindow w;
    w.readSettings();
    w.show();

    return app.exec();
}
