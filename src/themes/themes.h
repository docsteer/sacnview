#ifndef THEMES_H
#define THEMES_H

#include <QObject>
#include <QStringList>

class Themes {
public:
    typedef enum {
        LIGHT,
        DARK
    } theme_e;

    static void apply(theme_e theme);

    static const QStringList getDescriptions() {
        QStringList ret;
        ret << QObject::tr("Light Theme");
        ret << QObject::tr("Dark Theme");
        return ret;
    };
};

#endif // THEMES_H
