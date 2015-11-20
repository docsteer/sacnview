#-------------------------------------------------
#
# Project created by QtCreator 2015-10-16T13:42:54
#
#-------------------------------------------------

QT       += core gui network

QMAKE_CXXFLAGS += -std=gnu++0x

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sACNView
TEMPLATE = app

INCLUDEPATH += src src/sacn src/sacn/ACNShare

SOURCES += src/main.cpp\
        src/mdimainwindow.cpp \
    src/scopewindow.cpp \
    src/universeview.cpp \
    src/sacn/ACNShare/CID.cpp \
    src/sacn/ACNShare/ipaddr.cpp \
    src/sacn/ACNShare/tock.cpp \
    src/sacn/ACNShare/VHD.cpp \
    src/sacn/streamcommon.cpp \
    src/nicselectdialog.cpp \
    src/sacn/streamingacn.cpp \
    src/preferencesdialog.cpp \
    src/preferences.cpp \
    src/sacn/sacnlistener.cpp \
    src/universedisplay.cpp \
    src/transmitwindow.cpp \
    src/sacn/sacnsender.cpp \
    src/configureperchanpriodlg.cpp \
    src/gridwidget.cpp \
    src/priorityeditwidget.cpp \
    src/scopewidget.cpp

HEADERS  += src/mdimainwindow.h \
    src/scopewindow.h \
    src/universeview.h \
    src/sacn/ACNShare/CID.h \
    src/sacn/ACNShare/defpack.h \
    src/sacn/ACNShare/deftypes.h \
    src/sacn/ACNShare/ipaddr.h \
    src/sacn/ACNShare/tock.h \
    src/sacn/ACNShare/VHD.h \
    src/sacn/streamcommon.h \
    src/nicselectdialog.h \
    src/sacn/streamingacn.h \
    src/preferencesdialog.h \
    src/preferences.h \
    src/sacn/sacnlistener.h \
    src/universedisplay.h \
    src/transmitwindow.h \
    src/consts.h \
    src/sacn/sacnsender.h \
    src/configureperchanpriodlg.h \
    src/gridwidget.h \
    src/priorityeditwidget.h \
    src/scopewidget.h


FORMS    += ui/mdimainwindow.ui \
    ui/scopewindow.ui \
    ui/universeview.ui \
    ui/nicselectdialog.ui \
    ui/preferencesdialog.ui \
    ui/transmitwindow.ui \
    ui/configureperchanpriodlg.ui

RESOURCES += \
    res/resources.qrc
