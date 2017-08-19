#-------------------------------------------------
#
# Project created by QtCreator 2017-06-26T17:09:11
#
#-------------------------------------------------
TEMPLATE = app
QT += webenginewidgets sql
TARGET = cabrowser
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    masterwindow.cpp \
    sqlitehelper.cpp \
    main.cpp \
    tabs/browser.cpp \
    tabs/accountmanager.cpp \
    dialogs/addaccountdialog.cpp \
    dialogs/updateaccountdialog.cpp

INCLUDEPATH += \
    $$PWD/tabs \
    $$PWD/dialogs

HEADERS += \
    sqlitehelper.h \
    masterwindow.h \
    tabs/browser.h \
    tabs/accountmanager.h \
    dialogs/addaccountdialog.h \
    dialogs/updateaccountdialog.h

FORMS += \
    tabs/accountmanager.ui \
    dialogs/addaccountdialog.ui \
    dialogs/updateaccountdialog.ui
