#include "scopewidget.h"
#include <QPainter>
#include <QPoint>
#include "sacn/sacnlistener.h"

#define AXIS_WIDTH 50
#define TOP_GAP 10
#define RIGHT_GAP 30
#define AXIS_TO_WINDOW_GAP 5

ScopeChannel::ScopeChannel()
{
    m_universe = 1;
    m_address = 1;
    m_color = Qt::white;
    clear();
}


ScopeChannel::ScopeChannel(int universe, int address)
{
    m_universe = universe;
    m_address = address;
    m_color = Qt::white;
}

void ScopeChannel::addPoint(QPoint point)
{
    m_points[m_last] = point;
    m_last = (m_last+1) % RING_BUF_SIZE;
    if(m_size<RING_BUF_SIZE) m_size++;
}

void ScopeChannel::clear()
{
    m_size = 0;
    m_last = 0;
}

int ScopeChannel::count()
{
    return m_size;
}

QPoint ScopeChannel::getPoint(int index)
{
    int pos = (m_last + index) % RING_BUF_SIZE;
    return m_points[pos];
}


ScopeWidget::ScopeWidget(QWidget *parent) : QWidget(parent)
{
    m_timebase = 10;
}


void ScopeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);


    QPen gridPen;
    gridPen.setColor(QColor("#434343"));
    QVector<qreal> dashPattern;
    dashPattern << 1 << 1;

    QPen textPen;
    textPen.setColor(Qt::white);

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), Qt::black);

    // Origin is the bottom left of the scope window
    QPoint origin(rect().bottomLeft().x() + AXIS_WIDTH, rect().bottomLeft().y() - AXIS_WIDTH);

    QRect scopeWindow;
    scopeWindow.setBottomLeft(origin);
    scopeWindow.setTop(rect().top() + TOP_GAP);
    scopeWindow.setRight(rect().right() - RIGHT_GAP);
    painter.setBrush(QBrush(Qt::black));
    painter.drawRect(scopeWindow);

    // Draw vertical axis
    painter.translate(scopeWindow.topLeft().x(), scopeWindow.topLeft().y());
    gridPen.setDashPattern(dashPattern);

    painter.setPen(gridPen);
    painter.drawLine(0, 0, 0, scopeWindow.height());
    int percent = 100;

    QFont font;
    QFontMetricsF metrics(font);

    for(int i=0; i<11; i++)
    {
        qreal y = i*scopeWindow.height()/10.0;
        painter.setPen(gridPen);
        painter.drawLine(-10, y, scopeWindow.width(), y);
        QString text = QString("%1%%").arg(percent);
        QRectF fontRect = metrics.boundingRect(text);

        fontRect.moveCenter(QPointF(-20, y));

        painter.setPen(textPen);
        painter.drawText(fontRect, text, QTextOption(Qt::AlignLeft));

        percent-=10;
    }

    // Draw horizontal axis
    painter.resetTransform();
    painter.translate(scopeWindow.bottomLeft().x(), scopeWindow.bottomLeft().y());

    int time = 0;

    for(int i=0; i<11; i++)
    {
        qreal x = i * scopeWindow.width() / 10;
        painter.setPen(gridPen);
        painter.drawLine(x, 0, x, -scopeWindow.height());

        QString text = QString("%1ms").arg(time);
        QRectF fontRect = metrics.boundingRect(text);

        fontRect.moveCenter(QPointF(x, 20));

        painter.setPen(textPen);
        painter.drawText(fontRect, text, QTextOption(Qt::AlignLeft));

        time += m_timebase;

    }

    // Plot the points
    painter.resetTransform();
    painter.setRenderHint(QPainter::Antialiasing, false);

    int latestTime = 0;
    if(m_channels.count()>0)
        latestTime = m_channels[0]->latestTime;

    painter.translate(scopeWindow.topLeft().x() , scopeWindow.topLeft().y());
    // Scale by the timebase, which is units per division, 10 divisions per window
    double scale = m_timebase * 10;
    painter.scale(scopeWindow.width()/scale, scopeWindow.height() / 255.0);
    painter.setBrush(QBrush());
    painter.setRenderHint(QPainter::Antialiasing);
    foreach(ScopeChannel *ch, m_channels)
    {
        QPen pen;
        pen.setColor(ch->color());
        pen.setWidth(2);
        painter.setPen(pen);
        QPainterPath path;
        for(int i=0; i<ch->count(); i++)
        {
            QPoint p = ch->getPoint(i);
            int x = p.x() - latestTime + scale;
            int y = 255 - p.y();
            path.lineTo(x, y);
        }
        /*
        path.moveTo(0,255-ch->getPoint(0));
        for(int x=1; x<1000-1; x++)
        {
            int value = ch->getPoint(x);
            if(value>=0)
                path.lineTo(x, 255-ch->getPoint(x));
        }*/
        painter.drawPath(path);
    }

}


void ScopeWidget::setTimebase(int timebase)
{
    m_timebase = timebase;
    // Data is invalid once we change timebase, dump it
    foreach(ScopeChannel *ch, m_channels)
    {
        ch->clear();
    }

    stop();
    start();
    update();
}

void ScopeWidget::addChannel(ScopeChannel *channel)
{
    m_channels << channel;
    sACNListener *listener = sACNManager::getInstance()->getListener(channel->universe());
    listener->monitorAddress(channel->address());

}

void ScopeWidget::dataReady(int address, QPoint p)
{
    Q_ASSERT(m_channels.count()>address);

    ScopeChannel *ch  = m_channels[address];
    ch->addPoint(p);
    update();
}

void ScopeWidget::start()
{
    foreach(ScopeChannel *ch, m_channels)
    {
        ch->clear();
        sACNListener *listener = sACNManager::getInstance()->getListener(ch->universe());
        listener->setMonitorTimer(5);
        connect(listener, SIGNAL(dataReady(int, QPoint)), this, SLOT(dataReady(int, QPoint)));
    }
    update();

}

void ScopeWidget::stop()
{
    foreach(ScopeChannel *ch, m_channels)
    {
        sACNListener *listener = sACNManager::getInstance()->getListener(ch->universe());
        disconnect(this, SLOT(tick()));
    }
}
