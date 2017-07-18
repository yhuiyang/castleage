#include "mainwindow.h"
#include "masterwindow.h"
#include <QApplication>
#include <QWebEngineSettings>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    MasterWindow *window = new MasterWindow();
    window->show();

    return app.exec();
}
