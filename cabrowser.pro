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
    MdiChildWebView.cpp \
    sqliteopenhelper.cpp

HEADERS  += MainWindow.h \
    MdiChildWebView.h \
    sqliteopenhelper.h
