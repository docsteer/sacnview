#include "translations/translations.h"

const QList<Translations::sTranslations> Translations::lTranslations =
    {
        {
            "English",
            QLocale::English,
            QStringList{}
        },
        {
            "Français",
            QLocale::French,
            QStringList{"Tom Wickens"}
        },
        {
            "Deutsch",
            QLocale::German,
            QStringList{"Tom Wickens"}
        },
        {
            "Español",
            QLocale::Spanish,
            QStringList{"Tom Wickens"}
        }
    };
