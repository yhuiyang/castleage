#-------------------------------------------------
#
# Project created by QtCreator 2017-06-26T17:09:11
#
#-------------------------------------------------
TEMPLATE = app
QT += webenginewidgets sql
TARGET = cabrowser
CONFIG += c++11

QMAKE_MAC_SDK = macosx10.13

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -lz -L/usr/local/lib -lprotobuf
QMAKE_CXXFLAGS += -isystem /usr/local/include

SOURCES += \
    venders/gumbo-parser/src/attribute.c \
    venders/gumbo-parser/src/char_ref.c \
    venders/gumbo-parser/src/error.c \
    venders/gumbo-parser/src/parser.c \
    venders/gumbo-parser/src/string_buffer.c \
    venders/gumbo-parser/src/string_piece.c \
    venders/gumbo-parser/src/tag.c \
    venders/gumbo-parser/src/tokenizer.c \
    venders/gumbo-parser/src/utf8.c \
    venders/gumbo-parser/src/util.c \
    venders/gumbo-parser/src/vector.c \
    masterwindow.cpp \
    sqlitehelper.cpp \
    main.cpp \
    tabs/browser.cpp \
    tabs/accountmanager.cpp \
    dialogs/addaccountdialog.cpp \
    dialogs/updateaccountdialog.cpp \
    tabs/tagmanager.cpp \
    dialogs/inputdialog.cpp \
    castleagehttpclient.cpp \
    tabs/armycodeannounceplan.cpp \
    gaehttpclient.cpp \
    protobuf/armycode.pb.cc \
    tabs/armypool.cpp \
    tabs/gumboparsertestbed.cpp

INCLUDEPATH += \
    $$PWD/venders/gumbo-parser/src \
    $$PWD/tabs \
    $$PWD/dialogs \
    $$PWD/protobuf

HEADERS += \
    venders/gumbo-parser/src/attribute.h \
    venders/gumbo-parser/src/char_ref.h \
    venders/gumbo-parser/src/error.h \
    venders/gumbo-parser/src/gumbo.h \
    venders/gumbo-parser/src/insertion_mode.h \
    venders/gumbo-parser/src/parser.h \
    venders/gumbo-parser/src/string_buffer.h \
    venders/gumbo-parser/src/string_piece.h \
    venders/gumbo-parser/src/tag_enum.h \
    venders/gumbo-parser/src/tag_gperf.h \
    venders/gumbo-parser/src/tag_sizes.h \
    venders/gumbo-parser/src/tag_strings.h \
    venders/gumbo-parser/src/token_type.h \
    venders/gumbo-parser/src/tokenizer.h \
    venders/gumbo-parser/src/tokenizer_states.h \
    venders/gumbo-parser/src/utf8.h \
    venders/gumbo-parser/src/util.h \
    venders/gumbo-parser/src/vector.h \
    sqlitehelper.h \
    masterwindow.h \
    tabs/browser.h \
    tabs/accountmanager.h \
    dialogs/addaccountdialog.h \
    dialogs/updateaccountdialog.h \
    tabs/tagmanager.h \
    dialogs/inputdialog.h \
    castleagehttpclient.h \
    tabs/armycodeannounceplan.h \
    gaehttpclient.h \
    protobuf/armycode.pb.h \
    tabs/armypool.h \
    tabs/gumboparsertestbed.h

FORMS += \
    tabs/accountmanager.ui \
    dialogs/addaccountdialog.ui \
    dialogs/updateaccountdialog.ui \
    tabs/tagmanager.ui \
    dialogs/inputdialog.ui \
    tabs/armycodeannounceplan.ui \
    tabs/armypool.ui \
    tabs/gumboparsertestbed.ui
