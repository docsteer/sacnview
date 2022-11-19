## Copyright 2016 Tom Steer
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

QT       += core gui network multimedia widgets

TARGET = sACNView
TEMPLATE = app
DESCRIPTION = $$shell_quote("A tool for sending and receiving the Streaming ACN control protocol")
URL = $$shell_quote("https://www.sacnview.org")
LICENSE = $$shell_quote("Apache 2.0")

macx {
    ICON = res/icon.icns
}

CONFIG += c++17

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

# Qt6 is now the minimum supported
lessThan(QT_MAJOR_VERSION, 6): {
    error("Qt versions below 6 are unsupported")
}
DEFINES += VERSION=\\\"$$GIT_TAG\\\"


## External Libs
include(libs.pri)

## Project includes

INCLUDEPATH += src \
    src/sacn src/sacn/ACNShare \
    src/widgets \
    src/models \
    src/ui

## Sources

SOURCES += src/main.cpp\
    src/sacn/securesacn.cpp \
    src/widgets/monitorspinbox.cpp \
    src/widgets/qpushbutton_rightclick.cpp \
    src/widgets/qspinbox_resizetocontent.cpp \
    src/qt56.cpp \
    src/ui/newversiondialog.cpp \
    src/ui/mdimainwindow.cpp \
    src/ui/scopewindow.cpp \
    src/ui/universeview.cpp \
    src/sacn/sacnsynchronization.cpp \
    src/models/sacnsynclistmodel.cpp \
    src/sacn/ACNShare/CID.cpp \
    src/sacn/ACNShare/ipaddr.cpp \
    src/sacn/ACNShare/tock.cpp \
    src/sacn/ACNShare/VHD.cpp \
    src/sacn/streamcommon.cpp \
    src/ui/nicselectdialog.cpp \
    src/sacn/streamingacn.cpp \
    src/ui/preferencesdialog.cpp \
    src/preferences.cpp \
    src/sacn/sacnlistener.cpp \
    src/widgets/universedisplay.cpp \
    src/ui/transmitwindow.cpp \
    src/sacn/sacnsender.cpp \
    src/ui/configureperchanpriodlg.cpp \
    src/widgets/gridwidget.cpp \
    src/widgets/scopewidget.cpp \
    src/ui/aboutdialog.cpp \
    src/sacn/sacneffectengine.cpp \
    src/models/sacnuniverselistmodel.cpp \
    src/ui/snapshot.cpp \
    src/commandline.cpp \
    src/ui/multiuniverse.cpp \
    src/ui/flickerfinderinfoform.cpp \
    src/sacn/sacnsocket.cpp \
    src/ui/logwindow.cpp \
    src/firewallcheck.cpp \
    src/ui/bigdisplay.cpp \
    src/ui/addmultidialog.cpp \
    src/theme/darkstyle.cpp \
    src/ipc.cpp \
    src/sacn/sacndiscovery.cpp \
    src/models/sacndiscoveredsourcelistmodel.cpp \
    src/widgets/clssnapshot.cpp \
    src/sacn/fpscounter.cpp \
    src/widgets/grideditwidget.cpp

HEADERS += src/ui/mdimainwindow.h \
    src/sacn/securesacn.h \
    src/widgets/monitorspinbox.h \
    src/widgets/qpushbutton_rightclick.h \
    src/widgets/qspinbox_resizetocontent.h \
    src/ui/newversiondialog.h \
    src/ui/scopewindow.h \
    src/ui/universeview.h \
    src/sacn/sacnsynchronization.h \
    src/models/sacnsynclistmodel.h \
    src/sacn/ACNShare/CID.h \
    src/sacn/ACNShare/defpack.h \
    src/sacn/ACNShare/ipaddr.h \
    src/sacn/ACNShare/tock.h \
    src/sacn/ACNShare/VHD.h \
    src/sacn/streamcommon.h \
    src/ui/nicselectdialog.h \
    src/sacn/streamingacn.h \
    src/ui/preferencesdialog.h \
    src/preferences.h \
    src/sacn/sacnlistener.h \
    src/widgets/universedisplay.h \
    src/ui/transmitwindow.h \
    src/consts.h \
    src/sacn/sacnsender.h \
    src/ui/configureperchanpriodlg.h \
    src/widgets/gridwidget.h \
    src/widgets/scopewidget.h \
    src/ui/aboutdialog.h \
    src/sacn/sacneffectengine.h \
    src/models/sacnuniverselistmodel.h \
    src/ui/snapshot.h \
    src/commandline.h \
    src/fontdata.h \
    src/ui/multiuniverse.h \
    src/ui/flickerfinderinfoform.h \
    src/sacn/sacnsocket.h \
    src/ui/logwindow.h \
    src/firewallcheck.h \
    src/ui/bigdisplay.h \
    src/ui/addmultidialog.h \
    src/theme/darkstyle.h \
    src/sacn/e1_11.h \
    src/ipc.h \
    src/qt56.h \
    src/sacn/sacndiscovery.h \
    src/models/sacndiscoveredsourcelistmodel.h \
    src/widgets/clssnapshot.h \
    src/sacn/fpscounter.h \
    src/widgets/grideditwidget.h

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
