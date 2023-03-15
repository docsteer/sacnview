#ifndef DARKTHEME_H
#define DARKTHEME_H

#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionComplex>
#include "DarkStyle.h"

/*
 * DarkStyle theme with
 * - non-modified font point sizing
 * - Mdiwindow stylings
 * - Titlebar darkcolours (Windows only)
 */
class DarkTheme : public DarkStyle
{
    Q_OBJECT
    public:
        DarkTheme();
        ~DarkTheme();

        void polish(QPalette &palette) override;
        void polish(QApplication *app) override;

        void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const override;
        void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const override;

    private:
        class applyDarkFilter : public QObject
        {
            public:
                explicit applyDarkFilter(QObject *parent = nullptr) : QObject(parent) {}

            protected:
                bool eventFilter(QObject *obj, QEvent *event) override;
        };

        applyDarkFilter darkFilter;
};

#endif // DARKTHEME_H
