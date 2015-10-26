#-------------------------------------------------
#
# Project created by QtCreator 2015-10-22T19:49:19
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = renamemer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    icon.qrc

win32:RC_ICONS += resources/app_icon.ico
