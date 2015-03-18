#-------------------------------------------------
#
# Project created by QtCreator 2015-03-07T16:56:29
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CastleAgeHelper
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
    mainwindow.cpp \
    newaccountdialog.cpp \
    sqliteopenhelper.cpp

HEADERS  += mainwindow.h \
    newaccountdialog.h \
    sqliteopenhelper.h

FORMS    += mainwindow.ui \
    newaccountdialog.ui

RESOURCES += \
    res.qrc
