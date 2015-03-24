#-------------------------------------------------
#
# Project created by QtCreator 2015-03-07T16:56:29
#
#-------------------------------------------------

QT       += core gui sql network webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CastleAgeHelper
TEMPLATE = app

CONFIG += c++11

QMAKE_CXXFLAGS_DEBUG += -ggdb

INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib

LIBS += -lz

win32 {
    LIBS -= -lz
}

SOURCES += main.cpp\
    mainwindow.cpp \
    newaccountdialog.cpp \
    sqliteopenhelper.cpp \
    castleage.cpp

HEADERS  += mainwindow.h \
    newaccountdialog.h \
    sqliteopenhelper.h \
    castleage.h \
    constant.h

FORMS    += mainwindow.ui \
    newaccountdialog.ui

RESOURCES += \
    res.qrc
