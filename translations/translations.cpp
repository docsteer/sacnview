#include "translations/translations.h"

const QList<Translations::sTranslations> Translations::lTranslations =
    {
        {"English", QStringList{}, ":/sACNView.qm"},
        {"Français", QStringList{"Tom Wickens"}, ":/sACNView_fr.qm"},
        {"Deutsche", QStringList{"Tom Wickens"}, ":/sACNView_de.qm"},
        {"Español", QStringList{"Tom Wickens"}, ":/sACNView_es.qm"}
    };
