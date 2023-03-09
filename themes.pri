THEMES_PATH = $$system_path($${_PRO_FILE_PWD_}/themes)

INCLUDEPATH += $${THEMES_PATH}
SOURCES += \
    $${THEMES_PATH}/themes.cpp \
    $${THEMES_PATH}/darktheme.cpp
HEADERS += \
    $${THEMES_PATH}/themes.h \
    $${THEMES_PATH}/darktheme.h

# Qt-Frameless-Window-DarkStyle
THEMES_DARKSYLE_PATH = $$system_path($${THEMES_PATH}/Qt-Frameless-Window-DarkStyle)
SOURCES += $${THEMES_DARKSYLE_PATH}/darkstyle.cpp
HEADERS += $${THEMES_DARKSYLE_PATH}/darkstyle.h
RESOURCES += $${THEMES_DARKSYLE_PATH}/darkstyle.qrc
