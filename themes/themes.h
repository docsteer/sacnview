#ifndef THEMES_H
#define THEMES_H

#include <QObject>
#include <QStringList>

enum class DisplayFormat
{
  DECIMAL = 0,
  PERCENT = 1,
  HEXADECIMAL = 2,
  COUNT = 3
};

enum class WindowMode
{
  MDI = 0,
  Floating = 1,
  COUNT,
};

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
