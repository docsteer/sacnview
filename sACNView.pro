#-------------------------------------------------
#
# Project created by QtCreator 2015-10-16T13:42:54
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sACNView
TEMPLATE = app

INCLUDEPATH += sacn/ACNShare sacn

SOURCES += main.cpp\
        mdimainwindow.cpp \
    scopewindow.cpp \
    universeview.cpp \
    sacn/ACNShare/CID.cpp \
    sacn/ACNShare/ipaddr.cpp \
    sacn/ACNShare/tock.cpp \
    sacn/ACNShare/VHD.cpp \
    sacn/streamcommon.cpp \
    nicselectdialog.cpp \
    sacn/streamingacn.cpp \
    preferencesdialog.cpp \
    preferences.cpp \
    sacn/sacnlistener.cpp \
    universedisplay.cpp

HEADERS  += mdimainwindow.h \
    scopewindow.h \
    universeview.h \
    sacn/ACNShare/CID.h \
    sacn/ACNShare/defpack.h \
    sacn/ACNShare/deftypes.h \
    sacn/ACNShare/ipaddr.h \
    sacn/ACNShare/tock.h \
    sacn/ACNShare/VHD.h \
    sacn/streamcommon.h \
    nicselectdialog.h \
    sacn/streamingacn.h \
    preferencesdialog.h \
    preferences.h \
    sacn/sacnlistener.h \
    universedisplay.h

FORMS    += mdimainwindow.ui \
    scopewindow.ui \
    universeview.ui \
    nicselectdialog.ui \
    preferencesdialog.ui

RESOURCES += \
    res/resources.qrc
