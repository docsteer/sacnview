#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <QString>
#include <QStringList>
#include <QList>

class Translations {

public:
    struct sTranslations
    {
        QString LanguageName;
        QStringList Translators;
        QString FileName;
    };

    static const QList<sTranslations> lTranslations;
};


#endif // TRANSLATIONS_H
