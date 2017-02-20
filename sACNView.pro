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

macx {
QMAKE_MAC_SDK = macosx10.12
ICON = res/icon.icns
}

QMAKE_CXXFLAGS += -std=gnu++0x

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sACNView
TEMPLATE = app

INCLUDEPATH += src src/sacn src/sacn/ACNShare

GIT_VERSION = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)

DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS=1 GIT_CURRENT_SHA1=\\\"$$GIT_VERSION\\\"

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
    src/scopewidget.cpp \
    src/aboutdialog.cpp \
    src/sacn/sacneffectengine.cpp \
    src/mergeduniverselogger.cpp \
    src/sacn/sacnuniverselistmodel.cpp \
    src/snapshot.cpp \
    src/commandline.cpp
	
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
    src/scopewidget.h \
    src/aboutdialog.h \
    src/sacn/sacneffectengine.h \
    src/mergeduniverselogger.h \
    src/sacn/sacnuniverselistmodel.h \
    src/snapshot.h \
    src/commandline.h \
    src/fontdata.h

FORMS    += ui/mdimainwindow.ui \
    ui/scopewindow.ui \
    ui/universeview.ui \
    ui/nicselectdialog.ui \
    ui/preferencesdialog.ui \
    ui/transmitwindow.ui \
    ui/configureperchanpriodlg.ui \
    ui/aboutdialog.ui \
    ui/snapshot.ui

RESOURCES += \
    res/resources.qrc

RC_FILE = res/sacnview.rc

DISTFILES += \
    res/codemess.png

isEmpty(TARGET_EXT) {
    win32 {
        TARGET_CUSTOM_EXT = .exe
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}

win32 {
    DEPLOY_COMMAND = windeployqt
    DEPLOY_DIR = $${_PRO_FILE_PWD_}/install/deploy
    DEPLOY_TARGET = $${OUT_PWD}/release/$${TARGET}$${TARGET_CUSTOM_EXT}
    DEPLOY_OPT = --dir $${DEPLOY_DIR}
    DEPLOY_CLEANUP = $$QMAKE_COPY $${DEPLOY_TARGET} $${DEPLOY_DIR}
}
macx {
    DEPLOY_COMMAND = macdeployqt
    DEPLOY_DIR = $${_PRO_FILE_PWD_}/install/
    PRE_DEPLOY_COMMAND = $${QMAKE_DEL_FILE} $${_PRO_FILE_PWD_}/install/mac/sACNView.dmg
    DEPLOY_TARGET = $${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}
    DEPLOY_CLEANUP = $${_PRO_FILE_PWD_}/install/mac/create-dmg --volname "sACNView_Installer" --volicon "$${_PRO_FILE_PWD_}/res/icon.icns"
    DEPLOY_CLEANUP += --background "$${_PRO_FILE_PWD_}/res/mac_install_bg.png" --window-pos 200 120 --window-size 800 400 --icon-size 100 --icon sACNView.app 200 190 --hide-extension sACNView.app --app-drop-link 600 185
    DEPLOY_CLEANUP += $${_PRO_FILE_PWD_}/install/mac/sACNView.dmg $${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}
}

CONFIG( release , debug | release) {
    QMAKE_POST_LINK += $${PRE_DEPLOY_COMMAND} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} $${DEPLOY_OPT}
    QMAKE_POST_LINK += $$escape_expand(\\n\\t) $${DEPLOY_CLEANUP}
}

