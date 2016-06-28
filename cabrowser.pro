#-------------------------------------------------
#
# Project created by QtCreator 2016-04-29T16:55:39
#
#-------------------------------------------------

QT      += core gui sql network webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = cabrowser
TEMPLATE = app


SOURCES += main.cpp \
    MainWindow.cpp \
    sqliteopenhelper.cpp \
    CastleAgeNetworkAccessManager.cpp \
    MdiChild.cpp \
    ImportAccountDialog.cpp \
    AccountManagementDialog.cpp \
    SynchronizedNetworkAccessManager.cpp \
    LomTimeCheckDialog.cpp \
    TagEditorDialog.cpp

HEADERS  += MainWindow.h \
    sqliteopenhelper.h \
    CastleAgeNetworkAccessManager.h \
    MdiChild.h \
    ImportAccountDialog.h \
    AccountManagementDialog.h \
    SynchronizedNetworkAccessManager.h \
    LomTimeCheckDialog.h \
    TagEditorDialog.h

RESOURCES += \
    gfx/gfx.qrc

FORMS += \
    ImportAccountDialog.ui \
    AccountManagementDialog.ui \
    LomTimeCheckDialog.ui \
    TagEditorDialog.ui
