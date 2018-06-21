# Supported languages
## **Add new .qm to translations.qrc**
## **Update translations.cpp**
LANGUAGES = en fr de es

TRANSLATIONS_DIR = $${_PRO_FILE_PWD_}/translations
SOURCES += \
    $${TRANSLATIONS_DIR}/translationdialog.cpp \
    $${TRANSLATIONS_DIR}/translations.cpp
HEADERS += \
    $${TRANSLATIONS_DIR}/translationdialog.h \
    $${TRANSLATIONS_DIR}/translations.h
FORMS += \
    $${TRANSLATIONS_DIR}/translationdialog.ui
RESOURCES += \
    $${TRANSLATIONS_DIR}/translations.qrc

# Create/update .ts Files
qtPrepareTool(LUPDATE, lupdate)
command = $$LUPDATE $$shell_quote($$_PRO_FILE_)
system($$command)|error("Failed to run: $$command")

## https://appbus.wordpress.com/2016/04/28/howto-translations-i18n/
# Available translations
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}
tsroot = $$join(TARGET,,,.ts)
tstarget = $$join(TARGET,,,_)
TRANSLATIONS = $${TRANSLATIONS_DIR}/$$tsroot
TRANSLATIONS += $$prependAll(LANGUAGES, $${TRANSLATIONS_DIR}/$$tstarget, .ts)

# run LRELEASE to generate the qm files
qtPrepareTool(LRELEASE, lrelease)
for(tsfile, TRANSLATIONS) {
    command = $$LRELEASE $$shell_quote($$tsfile)
    system($$command)|error("Failed to run: $$command")
}
