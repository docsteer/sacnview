TRANSLATIONS_DIR = $${_PRO_FILE_PWD_}/translations

SOURCES += \
    $${TRANSLATIONS_DIR}/translationdialog.cpp \
    $${TRANSLATIONS_DIR}/translations.cpp
HEADERS += \
    $${TRANSLATIONS_DIR}/translationdialog.h \
    $${TRANSLATIONS_DIR}/translations.h
FORMS += \
    $${TRANSLATIONS_DIR}/translationdialog.ui

# French
TRANSLATIONS += $$system_path($${TRANSLATIONS_DIR}/sacnview_fr.ts)

# German
TRANSLATIONS += $$system_path($${TRANSLATIONS_DIR}/sacnview_de.ts)

# Spanish
TRANSLATIONS += $$system_path($${TRANSLATIONS_DIR}/sacnview_es.ts)
