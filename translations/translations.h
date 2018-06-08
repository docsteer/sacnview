#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QLocale>

class Translations {

public:
    struct sTranslations
    {
        QString NativeLanguageName;
        QLocale::Language Language;
        QStringList Translators;
    };

    static const QList<sTranslations> lTranslations;
};


#endif // TRANSLATIONS_H
