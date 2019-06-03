#include <QStyle>
#include <QPainter>
#include <QStyleOptionComplex>
#include "darkstyle.h"


static void qt_fusion_draw_mdibutton(QPainter *painter, const QStyleOptionTitleBar *option, const QRect &tmp, bool hover, bool sunken)
{
    QColor dark;
    dark.setHsv(option->palette.button().color().hue(),
                qMin(255, (int)(option->palette.button().color().saturation())),
                qMin(255, (int)(option->palette.button().color().value()*0.7)));

    QColor highlight = option->palette.highlight().color();

    bool active = (option->titleBarState & QStyle::State_Active);
    QColor titleBarHighlight(255, 255, 255, 60);

    if (sunken)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), option->palette.highlight().color().darker(120));
    else if (hover)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), QColor(255, 255, 255, 20));

    QColor mdiButtonGradientStartColor;
    QColor mdiButtonGradientStopColor;

    mdiButtonGradientStartColor = QColor(0, 0, 0, 40);
    mdiButtonGradientStopColor = QColor(255, 255, 255, 60);

    if (sunken)
        titleBarHighlight = highlight.darker(130);

    QLinearGradient gradient(tmp.center().x(), tmp.top(), tmp.center().x(), tmp.bottom());
    gradient.setColorAt(0, mdiButtonGradientStartColor);
    gradient.setColorAt(1, mdiButtonGradientStopColor);
    QColor mdiButtonBorderColor(active ? option->palette.highlight().color().darker(180): dark.darker(110));

    painter->setPen(QPen(mdiButtonBorderColor));
    const QLine lines[4] = {
        QLine(tmp.left() + 2, tmp.top(), tmp.right() - 2, tmp.top()),
        QLine(tmp.left() + 2, tmp.bottom(), tmp.right() - 2, tmp.bottom()),
        QLine(tmp.left(), tmp.top() + 2, tmp.left(), tmp.bottom() - 2),
        QLine(tmp.right(), tmp.top() + 2, tmp.right(), tmp.bottom() - 2)
    };
    painter->drawLines(lines, 4);
    const QPoint points[4] = {
        QPoint(tmp.left() + 1, tmp.top() + 1),
        QPoint(tmp.right() - 1, tmp.top() + 1),
        QPoint(tmp.left() + 1, tmp.bottom() - 1),
        QPoint(tmp.right() - 1, tmp.bottom() - 1)
    };
    painter->drawPoints(points, 4);

    painter->setPen(titleBarHighlight);
    painter->drawLine(tmp.left() + 2, tmp.top() + 1, tmp.right() - 2, tmp.top() + 1);
    painter->drawLine(tmp.left() + 1, tmp.top() + 2, tmp.left() + 1, tmp.bottom() - 2);

    painter->setPen(QPen(gradient, 1));
    painter->drawLine(tmp.right() + 1, tmp.top() + 2, tmp.right() + 1, tmp.bottom() - 2);
    painter->drawPoint(tmp.right() , tmp.top() + 1);

    painter->drawLine(tmp.left() + 2, tmp.bottom() + 1, tmp.right() - 2, tmp.bottom() + 1);
    painter->drawPoint(tmp.left() + 1, tmp.bottom());
    painter->drawPoint(tmp.right() - 1, tmp.bottom());
    painter->drawPoint(tmp.right() , tmp.bottom() - 1);
}

DarkStyle::DarkStyle():
    DarkStyle(styleBase())
{ }

DarkStyle::DarkStyle(QStyle *style):
    QProxyStyle(style)
{ }

QStyle *DarkStyle::styleBase(QStyle *style) const {
    static QStyle *base = !style ? QStyleFactory::create("Fusion") : style;
    return base;
}

QStyle *DarkStyle::baseStyle() const
{
    return styleBase();
}

void DarkStyle::polish(QPalette &palette)
{
    // modify palette to dark
    palette.setColor(QPalette::Window,QColor(53,53,53));
    palette.setColor(QPalette::WindowText,QColor("#e2e2e2"));
    palette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
    palette.setColor(QPalette::Base,QColor(42,42,42));
    palette.setColor(QPalette::AlternateBase,QColor(66,66,66));
    palette.setColor(QPalette::ToolTipBase,QColor("#e2e2e2"));
    palette.setColor(QPalette::ToolTipText,QColor("#e2e2e2"));
    palette.setColor(QPalette::Text,QColor("#e2e2e2"));
    palette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
    palette.setColor(QPalette::Dark,QColor(35,35,35));
    palette.setColor(QPalette::Shadow,QColor(20,20,20));
    palette.setColor(QPalette::Button,QColor(53,53,53));
    palette.setColor(QPalette::ButtonText,Qt::white);
    palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
    palette.setColor(QPalette::BrightText,Qt::red);
    palette.setColor(QPalette::Link,QColor(42,130,218));
    palette.setColor(QPalette::Highlight,QColor(42,130,218));
    palette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
    palette.setColor(QPalette::HighlightedText,Qt::white);
    palette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));
}

void DarkStyle::polish(QApplication *app)
{
    if (!app) return;

    // increase font size for better reading,
    // setPointSize was reduced from +2 because when applied this way in Qt5, the font is larger than intended for some reason
    QFont defaultFont = QApplication::font();
    defaultFont.setPointSize(defaultFont.pointSize()+1);
    app->setFont(defaultFont);

    // loadstylesheet    
    QFile qfDarkstyle(QString(":/darkstyle/darkstyle.qss"));
    if (qfDarkstyle.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // set stylesheet
        QString qsStylesheet = QString(qfDarkstyle.readAll());
        app->setStyleSheet(qsStylesheet);
        qfDarkstyle.close();
    }
}

void DarkStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                   QPainter *painter, const QWidget *widget) const
{
    if(control==QStyle::CC_TitleBar)
    {
        const QStyleOptionTitleBar *titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
        const int buttonMargin = 5;
        QPalette palette = option->palette;
        QColor highlight = option->palette.highlight().color();
        bool active = (titleBar->titleBarState & State_Active);
        QColor textColor(active ? 0xffffff : 0xff000000);
        QColor textAlphaColor(active ? 0xffffff : 0xff000000 );


        QColor titlebarColor = QColor(active ? highlight: palette.background().color());
                        QLinearGradient gradient(option->rect.center().x(), option->rect.top(),
                                                 option->rect.center().x(), option->rect.bottom());

                        gradient.setColorAt(0, titlebarColor.lighter(114));
                        gradient.setColorAt(0.5, titlebarColor.lighter(102));
                        gradient.setColorAt(0.51, titlebarColor.darker(104));
                        gradient.setColorAt(1, titlebarColor);
                        painter->fillRect(option->rect.adjusted(1, 1, -1, 0), gradient);


        QRect textRect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarLabel, widget);
        painter->setPen(active? (titleBar->palette.text().color().lighter(120)) :
                                titleBar->palette.text().color() );
        // Note workspace also does elliding but it does not use the correct font
        QString title = painter->fontMetrics().elidedText(titleBar->text, Qt::ElideRight, textRect.width() - 14);
        painter->drawText(textRect.adjusted(1, 1, 1, 1), title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));

        // Icon

        if ((titleBar->subControls & SC_TitleBarSysMenu) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
            QRect iconRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarSysMenu, widget);
            if (iconRect.isValid()) {
                if (!titleBar->icon.isNull()) {
                    const QPixmap orig = titleBar->icon.pixmap(iconRect.size());

                    // getting size if the icon is non square
                    int size = qMax(orig.width(), orig.height());

                    // creating a new transparent pixmap with equal sides
                    QPixmap rounded = QPixmap(size, size);
                    rounded.fill(Qt::transparent);

                    // creating circle clip area
                    QPainterPath path;
                    path.addEllipse(rounded.rect());

                    QPainter roundPainter(&rounded);
                    roundPainter.setRenderHint(QPainter::Antialiasing);
                    roundPainter.setClipPath(path);

                    // filling rounded area if needed
                    roundPainter.fillRect(rounded.rect(), Qt::black);
                    // getting offsets if the original picture is not square
                    int x = qAbs(orig.width() - size) / 2;
                    int y = qAbs(orig.height() - size) / 2;
                    roundPainter.drawPixmap(x, y, orig.width(), orig.height(), orig);

                   painter->drawPixmap(iconRect, rounded);

                } else {
                    QStyleOption tool = *titleBar;
                    QPixmap pm = proxy()->standardIcon(SP_TitleBarMenuButton, &tool, widget).pixmap(16, 16);
                    tool.rect = iconRect;
                    painter->save();
                    proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                    painter->restore();
                }
            }
        }

        // min button
                    if ((titleBar->subControls & SC_TitleBarMinButton) && (titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                            !(titleBar->titleBarState& Qt::WindowMinimized)) {
                        QRect minButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMinButton, widget);
                        if (minButtonRect.isValid()) {
                            bool hover = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_MouseOver);
                            bool sunken = (titleBar->activeSubControls & SC_TitleBarMinButton) && (titleBar->state & State_Sunken);
                            qt_fusion_draw_mdibutton(painter, titleBar, minButtonRect, hover, sunken);
                            QRect minButtonIconRect = minButtonRect.adjusted(buttonMargin ,buttonMargin , -buttonMargin, -buttonMargin);
                            painter->setPen(textColor);
                            painter->drawLine(minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 3,
                                              minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 3);
                            painter->drawLine(minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 4,
                                              minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 4);
                            painter->setPen(textAlphaColor);
                            painter->drawLine(minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 3,
                                              minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 4);
                            painter->drawLine(minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 3,
                                              minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 4);
                        }
                    }
                    // max button
                    if ((titleBar->subControls & SC_TitleBarMaxButton) && (titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                            !(titleBar->titleBarState & Qt::WindowMaximized)) {
                        QRect maxButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMaxButton, widget);
                        if (maxButtonRect.isValid()) {
                            bool hover = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_MouseOver);
                            bool sunken = (titleBar->activeSubControls & SC_TitleBarMaxButton) && (titleBar->state & State_Sunken);
                            qt_fusion_draw_mdibutton(painter, titleBar, maxButtonRect, hover, sunken);

                            QRect maxButtonIconRect = maxButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);

                            painter->setPen(textColor);
                            painter->drawRect(maxButtonIconRect.adjusted(0, 0, -1, -1));
                            painter->drawLine(maxButtonIconRect.left() + 1, maxButtonIconRect.top() + 1,
                                              maxButtonIconRect.right() - 1, maxButtonIconRect.top() + 1);
                            painter->setPen(textAlphaColor);
                            const QPoint points[4] = {
                                maxButtonIconRect.topLeft(),
                                maxButtonIconRect.topRight(),
                                maxButtonIconRect.bottomLeft(),
                                maxButtonIconRect.bottomRight()
                            };
                            painter->drawPoints(points, 4);
                        }
                    }
                    // normalize button
                                if ((titleBar->subControls & SC_TitleBarNormalButton) &&
                                        (((titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                                          (titleBar->titleBarState & Qt::WindowMinimized)) ||
                                         ((titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                                          (titleBar->titleBarState & Qt::WindowMaximized)))) {
                                    QRect normalButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarNormalButton, widget);
                                    if (normalButtonRect.isValid()) {

                                        bool hover = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_MouseOver);
                                        bool sunken = (titleBar->activeSubControls & SC_TitleBarNormalButton) && (titleBar->state & State_Sunken);
                                        QRect normalButtonIconRect = normalButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);
                                        qt_fusion_draw_mdibutton(painter, titleBar, normalButtonRect, hover, sunken);

                                        QRect frontWindowRect = normalButtonIconRect.adjusted(0, 3, -3, 0);
                                        painter->setPen(textColor);
                                        painter->drawRect(frontWindowRect.adjusted(0, 0, -1, -1));
                                        painter->drawLine(frontWindowRect.left() + 1, frontWindowRect.top() + 1,
                                                          frontWindowRect.right() - 1, frontWindowRect.top() + 1);
                                        painter->setPen(textAlphaColor);
                                        const QPoint points[4] = {
                                            frontWindowRect.topLeft(),
                                            frontWindowRect.topRight(),
                                            frontWindowRect.bottomLeft(),
                                            frontWindowRect.bottomRight()
                                        };
                                        painter->drawPoints(points, 4);

                                        QRect backWindowRect = normalButtonIconRect.adjusted(3, 0, 0, -3);
                                        QRegion clipRegion = backWindowRect;
                                        clipRegion -= frontWindowRect;
                                        painter->save();
                                        painter->setClipRegion(clipRegion);
                                        painter->setPen(textColor);
                                        painter->drawRect(backWindowRect.adjusted(0, 0, -1, -1));
                                        painter->drawLine(backWindowRect.left() + 1, backWindowRect.top() + 1,
                                                          backWindowRect.right() - 1, backWindowRect.top() + 1);
                                        painter->setPen(textAlphaColor);
                                        const QPoint points2[4] = {
                                            backWindowRect.topLeft(),
                                            backWindowRect.topRight(),
                                            backWindowRect.bottomLeft(),
                                            backWindowRect.bottomRight()
                                        };
                                        painter->drawPoints(points2, 4);
                                        painter->restore();
                                    }
                                }


                    // close button
                    if ((titleBar->subControls & SC_TitleBarCloseButton) && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
                        QRect closeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarCloseButton, widget);
                        if (closeButtonRect.isValid()) {
                            bool hover = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_MouseOver);
                            bool sunken = (titleBar->activeSubControls & SC_TitleBarCloseButton) && (titleBar->state & State_Sunken);
                            qt_fusion_draw_mdibutton(painter, titleBar, closeButtonRect, hover, sunken);
                            QRect closeIconRect = closeButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);
                            painter->setPen(textAlphaColor);
                            const QLine lines[4] = {
                                QLine(closeIconRect.left() + 1, closeIconRect.top(),
                                closeIconRect.right(), closeIconRect.bottom() - 1),
                                QLine(closeIconRect.left(), closeIconRect.top() + 1,
                                closeIconRect.right() - 1, closeIconRect.bottom()),
                                QLine(closeIconRect.right() - 1, closeIconRect.top(),
                                closeIconRect.left(), closeIconRect.bottom() - 1),
                                QLine(closeIconRect.right(), closeIconRect.top() + 1,
                                closeIconRect.left() + 1, closeIconRect.bottom())
                            };
                            painter->drawLines(lines, 4);
                            const QPoint points[4] = {
                                closeIconRect.topLeft(),
                                closeIconRect.topRight(),
                                closeIconRect.bottomLeft(),
                                closeIconRect.bottomRight()
                            };
                            painter->drawPoints(points, 4);

                            painter->setPen(textColor);
                            painter->drawLine(closeIconRect.left() + 1, closeIconRect.top() + 1,
                                              closeIconRect.right() - 1, closeIconRect.bottom() - 1);
                            painter->drawLine(closeIconRect.left() + 1, closeIconRect.bottom() - 1,
                                              closeIconRect.right() - 1, closeIconRect.top() + 1);
                        }
                    }

        return;
     }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void DarkStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                              QPainter *painter, const QWidget *widget) const
{

    if(element==QStyle::PE_FrameWindow)
    {
        painter->fillRect(option->rect, option->palette.color(QPalette::Window));
        return;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}
