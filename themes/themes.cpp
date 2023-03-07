#include "themes.h"
#include "Qt-Frameless-Window-DarkStyle/DarkStyle.h"

/*
 * DarkStyle theme with non-modified font point sizing
 */
class DarkMode : public DarkStyle
{
    public:
        void polish(QApplication *app) override
        {
            const auto pointSize = QApplication::font().pointSize();

            DarkStyle::polish(app);

            // Restore original point size
            auto font = app->font();
            font.setPointSize(pointSize);
            app->setFont(font);
        }
};

void Themes::apply(theme_e theme)
{
    switch (theme) {
        case DARK:
            QApplication::setStyle(new DarkMode);
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
