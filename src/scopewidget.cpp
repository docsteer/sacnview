#include "scopewidget.h"
#include <QPainter>

#define AXIS_WIDTH 30
#define TOP_GAP 10

ScopeWidget::ScopeWidget(QWidget *parent) : QWidget(parent)
{

}


void ScopeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);

    QPoint origin(rect().bottomLeft().x() + AXIS_WIDTH, rect().bottomLeft().y() - AXIS_WIDTH);

    QRect scopeWindow;
    scopeWindow.setBottomLeft(origin);
    scopeWindow.setTopRight(rect().topRight());
    scopeWindow.setTop(rect().top() + TOP_GAP);
    painter.setBrush(QBrush(Qt::blue));
    painter.drawRoundedRect(scopeWindow, 10, 10);

    // Draw vertical axis
    painter.setPen(QPen(Qt::white));
    painter.drawLine(scopeWindow.bottomLeft(), scopeWindow.topLeft());
    for(qreal y = 0; y<scopeWindow.height(); y+= scopeWindow.height()/32)
    {
        painter.drawLine(0, y, 10, y);
    }

}
