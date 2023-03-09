#ifndef DARKTHEME_H
#define DARKTHEME_H

#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionComplex>
#include "Qt-Frameless-Window-DarkStyle/DarkStyle.h"

/*
 * DarkStyle theme with non-modified font point sizing
 * and Mdiwindow stylings
 */
class DarkTheme : public DarkStyle
{
    public:
        void polish(QPalette &palette) override;
        void polish(QApplication *app) override;

        void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const override;
        void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const override;
};

#endif // DARKTHEME_H
