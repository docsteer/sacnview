## Copyright 2016 Tom Barthel-Steer
## http://www.tomsteer.net
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
## http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

QT       += core gui network multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sACNView
TEMPLATE = app
DESCRIPTION = $$shell_quote("A tool for sending and receiving the Streaming ACN control protocol")
URL = $$shell_quote("https://www.sacnview.org")
LICENSE = $$shell_quote("Apache 2.0")

macx {
    ICON = res/icon.icns
}

!msvc {
    QMAKE_CXXFLAGS += -std=gnu++0x
}

# Translations
include(translations.pri)

# Version defines

GIT_COMMAND = git --git-dir $$shell_quote($$PWD/.git) --work-tree $$shell_quote($$PWD)
GIT_VERSION = $$system($$GIT_COMMAND describe --always --tags)
GIT_DATE_DAY = $$system($$GIT_COMMAND show -1 -s --date=format:\"%a\" --format=\"%cd\" $$GIT_VERSION)
GIT_DATE_DATE = $$system($$GIT_COMMAND show -1 -s --date=format:\"%d\" --format=\"%cd\" $$GIT_VERSION)
GIT_DATE_MONTH = $$system($$GIT_COMMAND show -1 -s --date=format:\"%b\" --format=\"%cd\" $$GIT_VERSION)
GIT_DATE_YEAR = $$system($$GIT_COMMAND show -1 -s --date=format:\"%Y\" --format=\"%cd\" $$GIT_VERSION)
GIT_TAG = $$system($$GIT_COMMAND describe --abbrev=0 --always --tags)
GIT_SHA1 = $$system($$GIT_COMMAND rev-parse --short HEAD)

DEFINES += GIT_CURRENT_SHA1=\\\"$$GIT_VERSION\\\"
DEFINES += GIT_DATE_DAY=\\\"$$GIT_DATE_DAY\\\"
DEFINES += GIT_DATE_DATE=\\\"$$GIT_DATE_DATE\\\"
DEFINES += GIT_DATE_MONTH=\\\"$$GIT_DATE_MONTH\\\"
DEFINES += GIT_DATE_YEAR=\\\"$$GIT_DATE_YEAR\\\"

# Debug symbols
win32 {
    QMAKE_CXXFLAGS += /Zi
    QMAKE_LFLAGS += /INCREMENTAL:NO /Debug
}
unix {
    QMAKE_CXXFLAGS += -g
}

# Windows XP Special Build?
win32 {
    lessThan(QT_MAJOR_VERSION, 6):lessThan(QT_MINOR_VERSION, 7) {
        QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.01
        DEFINES += _ATL_XP_TARGETING
        DEFINES += VERSION=\\\"$$GIT_TAG-WindowsXP\\\"
        TARGET_WINXP = 1
        DEFINES += TARGET_WINXP
    } else {
        DEFINES += VERSION=\\\"$$GIT_TAG\\\"
        TARGET_WINXP = 0
    }
} else {
    DEFINES += VERSION=\\\"$$GIT_TAG\\\"
}

## External Libs
include(libs.pri)

## Project includes

INCLUDEPATH += src src/sacn src/sacn/ACNShare

## Sources

SOURCES += src/main.cpp\
    src/mdimainwindow.cpp \
    src/sacn/sacnsynchronization.cpp \
    src/sacn/sacnsynclistmodel.cpp \
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
    src/scopewidget.cpp \
    src/aboutdialog.cpp \
    src/sacn/sacneffectengine.cpp \
    src/sacn/sacnuniverselistmodel.cpp \
    src/snapshot.cpp \
    src/commandline.cpp \
    src/multiuniverse.cpp \
    src/flickerfinderinfoform.cpp \
    src/sacn/sacnsocket.cpp \
    src/logwindow.cpp \
    src/versioncheck.cpp \
    src/sacn/firewallcheck.cpp \
    src/bigdisplay.cpp \
    src/addmultidialog.cpp \
    src/theme/darkstyle.cpp \
    src/ipc.cpp \
    src/sacn/sacndiscovery.cpp \
    src/sacn/sacndiscoveredsourcelistmodel.cpp \
    src/clssnapshot.cpp \
    src/sacn/fpscounter.cpp \
    src/grideditwidget.cpp

HEADERS += src/mdimainwindow.h \
    src/sacn/sacnsynchronization.h \
    src/sacn/sacnsynclistmodel.h \
    src/scopewindow.h \
    src/universeview.h \
    src/sacn/ACNShare/CID.h \
    src/sacn/ACNShare/defpack.h \
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
    src/scopewidget.h \
    src/aboutdialog.h \
    src/sacn/sacneffectengine.h \
    src/sacn/sacnuniverselistmodel.h \
    src/snapshot.h \
    src/commandline.h \
    src/fontdata.h \
    src/multiuniverse.h \
    src/flickerfinderinfoform.h \
    src/sacn/sacnsocket.h \
    src/logwindow.h \
    src/versioncheck.h \
    src/sacn/firewallcheck.h \
    src/bigdisplay.h \ 
    src/addmultidialog.h \
    src/ethernetstrut.h \
    src/theme/darkstyle.h \
    src/xpwarning.h \
    src/sacn/e1_11.h \
    src/ipc.h \
    src/qt56.h \
    src/sacn/sacndiscovery.h \
    src/sacn/sacndiscoveredsourcelistmodel.h \
    src/clssnapshot.h \
    src/sacn/fpscounter.h \
    src/grideditwidget.h

FORMS += ui/mdimainwindow.ui \
    ui/scopewindow.ui \
    ui/universeview.ui \
    ui/nicselectdialog.ui \
    ui/preferencesdialog.ui \
    ui/transmitwindow.ui \
    ui/configureperchanpriodlg.ui \
    ui/aboutdialog.ui \
    ui/snapshot.ui \
    ui/multiuniverse.ui \
    ui/flickerfinderinfoform.ui \
    ui/logwindow.ui \
    ui/bigdisplay.ui \
    ui/newversiondialog.ui \
    ui/addmultidialog.ui

RESOURCES += \
    res/resources.qrc \
    src/theme/darkstyle.qrc

RC_FILE = res/sacnview.rc

## Deploy
include(deploy.pri)

## Translations
DISTFILES += \
    translations.pri
