#include "themes.h"
#include "darktheme.h"

void Themes::apply(theme_e theme)
{
    switch (theme) {
        case DARK:
            QApplication::setStyle(new DarkTheme);
            break;

        case LIGHT:
            QApplication::setStyle(QStyleFactory::create("Fusion"));
            break;
    }

    #if QT_VERSION == QT_VERSION_CHECK(5, 15, 0)
    // https://codereview.qt-project.org/c/qt/qtbase/+/285996
    QApplication::setPalette(QApplication::style()->standardPalette());
    #endif
}
