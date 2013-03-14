#-------------------------------------------------
#
# Project created by QtCreator 2013-03-09T19:54:24
#
#-------------------------------------------------

QT       += core gui



greaterThan(QT_MAJOR_VERSION, 4) {
QT += widgets
CONFIG += c++11
}else {
QMAKE_CXXFLAGS += -std=c++0x
}

TARGET = file_browser_example
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    filesystemmodel.cpp

HEADERS  += mainwindow.h \
    filesystemmodel.h

FORMS    += mainwindow.ui
