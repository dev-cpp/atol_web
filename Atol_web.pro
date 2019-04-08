#-------------------------------------------------
#
# Project created by QtCreator 2017-07-18T14:06:09
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Atol_web
TEMPLATE = app
LIBS += $$_PRO_FILE_PWD_/fptr.dll

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    webserver.cpp \
    printer.cpp \
    logmessage.cpp \
    settings.cpp \
    eou.cpp

HEADERS  += mainwindow.h \
    webserver.h \
    include/ifptr.h \
    printer.h \
    logmessage.h \
    settings.h \
    eou.h

FORMS    += mainwindow.ui \
    logmessage.ui \
    settings.ui

RESOURCES += \
    img.qrc

VERSION = 1.0.0.5
QMAKE_TARGET_DESCRIPTION = port 9655
RC_ICONS = icons-atol_03.ico
DESTDIR = Atol_web
