#include "MainWindow.h"
#include <QApplication>
//#include <qtwebenginewidgetsglobal.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();

    return app.exec();
}
