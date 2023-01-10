// Copyright 2016 Tom Steer
// http://www.tomsteer.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "scopewidget.h"
#include <QPainter>
#include <QPointF>
#include <QDebug>
#include <QPainterPath>
#include "sacn/sacnlistener.h"

#define AXIS_WIDTH 50
#define TOP_GAP 10
#define RIGHT_GAP 30
#define AXIS_TO_WINDOW_GAP 5

ScopeChannel::ScopeChannel()
{
    m_universe = 1;
    m_address = 1;
    m_sixteenBit = false;
    m_color = Qt::white;
    clear();
}


ScopeChannel::ScopeChannel(int universe, int address)
{
    m_universe = universe;
    m_address = address;
    m_sixteenBit = false;
    m_color = Qt::white;
    clear();
}

void ScopeChannel::addPoint(QPointF point)
{
    m_points[m_last] = point;
    m_last = (m_last+1) % RING_BUF_SIZE;
    if(m_size<RING_BUF_SIZE) m_size++;
    m_highestTime = point.x();
}

void ScopeChannel::clear()
{
    m_size = 0;
    m_last = 0;
    m_highestTime = 0;
}

int ScopeChannel::count()
{
    return m_size;
}

QPointF ScopeChannel::getPoint(int index)
{
    Q_ASSERT(index < m_size);

    int pos = (m_last + index) % m_size;
    return m_points[pos];
}

void ScopeChannel::setUniverse(int value)
{
    if(m_universe!=value)
    {
        m_universe = value;
        clear();
    }
}

void ScopeChannel::setAddress(int value)
{
    if(m_address!=value)
    {
        m_address = value;
        clear();
    }
}



ScopeWidget::ScopeWidget(QWidget *parent) : QWidget(parent)
{
    m_timebase          = 10;
    m_running           = false;
    m_triggered         = false;
    m_triggerMode       = tmNormal;
    m_triggerUniverse   = -1;
    m_triggerChannel    = -1;
    m_triggerLevel      = 0;
    m_triggerDelay      = 0;
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

        fontRect.moveCenter(QPoint(-20, y));

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

        QString text;
        if(m_timebase<1000)
            text = QString("%1ms").arg(time);
        else
            text = QString("%1s").arg(time/1000);

        QRectF fontRect = metrics.boundingRect(text);

        fontRect.moveCenter(QPoint(x, 20));

        painter.setPen(textPen);
        painter.drawText(fontRect, text, QTextOption(Qt::AlignLeft));

        time += m_timebase;

    }


    // Plot the points
    painter.resetTransform();
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.translate(scopeWindow.topLeft().x() , scopeWindow.topLeft().y());
    // Scale by the timebase, which is units per division, 10 divisions per window
    qreal x_scale = (qreal)scopeWindow.width() / ( m_timebase * 10.0 );
    qreal y_scale = (qreal)scopeWindow.height() / 65535.0;

    painter.setBrush(QBrush());
    painter.setRenderHint(QPainter::Antialiasing);

    int maxTime = m_timebase * 10; // The X-axis size of the display, in milliseconds

    if(m_triggerMode==tmNormal || (m_triggerMode!=tmNormal && m_triggered))
    {
        foreach(ScopeChannel *ch, m_channels)
        {
            if(!ch->enabled())
                continue;

            QPen pen;
            pen.setColor(ch->color());
            pen.setWidth(2);
            painter.setPen(pen);
            QPainterPath path;
            bool first = true;

            for(int i=0; i<ch->count()-1; i++)
            {
                QPointF p = ch->getPoint(i);
                qreal normalizedTime = ch->m_highestTime - p.x();

                qreal x = x_scale * (maxTime - normalizedTime);
                qreal y = y_scale * (65535 - p.y());
                if(x>=0)
                {
                    if(first)
                    {
                        path.moveTo(x,y);
                        first = false;
                    }
                    else
                        path.lineTo(x, y);
                }
            }
            painter.drawPath(path);
        }
    }

    // Draw the trigger line
    if(m_triggerMode!=tmNormal)
    {
        QPen pen;
        pen.setColor(QColor(Qt::yellow));

        pen.setStyle(Qt::DashDotLine);
        painter.setPen(pen);
        painter.drawLine(x_scale * m_triggerDelay, 0, x_scale * m_triggerDelay, 65535 * y_scale);
    }

}


void ScopeWidget::setTimebase(int timebase)
{
    m_timebase = timebase;
    update();
}

void ScopeWidget::addChannel(ScopeChannel *channel)
{
    m_channels << channel;
}

void ScopeWidget::removeChannel(ScopeChannel *channel)
{
    m_channels.removeAll(channel);
}

void ScopeWidget::dataReady(int address, QPointF p)
{
    if(!m_running) return;


    sACNListener *listener = static_cast<sACNListener *>(sender());

    if(!listener) return; // Check for deletion

    if(m_triggerMode != tmNormal && !m_triggered)
    {
        // Waiting for trigger
        if(listener->universe()==m_triggerUniverse && address==m_triggerChannel)
        {
            int value = p.y();
            if(m_triggerMode==tmRisingEdge && value>m_triggerLevel)
            {
                // Triggered by rising edge
                m_triggered = true;
                m_endTriggerTime = p.x() + m_timebase*10 - m_triggerDelay;
            }
            if(m_triggerMode==tmFallingEdge && value<m_triggerLevel)
            {
                // Triggered by rising edge
                m_triggered = true;
                m_endTriggerTime = p.x() + m_timebase*10 - m_triggerDelay;
            }
        }
    }

    if(m_triggerMode!=tmNormal && m_triggered)
    {
        if(p.x() > m_endTriggerTime)
        {
            // Trigger delay time is done - stop running
            m_running = false;
            emit stopped();
            return;
        }
    }


    ScopeChannel *ch = NULL;
    for(int i=0; i<m_channels.count(); i++)
    {
        if(m_channels[i]->address() == address && m_channels[i]->universe()==listener->universe())
        {
            ch = m_channels[i];

            // To save data space, no point in storing more than 100 points per timebase division
            if(p.x() - ch->m_highestTime < (m_timebase/100.0))
                   continue;
            if(ch->sixteenBit())
            {
                quint8 fineLevel = listener->mergedLevels()[ch->address()+1].level;
                p.setY(p.y() * 255.0 + fineLevel);
                ch->addPoint(p);
            }
            else
            {
                p.setY(p.y() * 255.0);
                ch->addPoint(p);
            }
        }
    }
    update();
}

void ScopeWidget::start()
{
    m_running = true;
    m_triggered = false;
    foreach(ScopeChannel *ch, m_channels)
    {
        ch->clear();
        auto listener = sACNManager::getInstance()->getListener(ch->universe());
        connect(listener.data(), SIGNAL(dataReady(int, QPointF)), this, SLOT(dataReady(int, QPointF)));
    }
    update();

}

void ScopeWidget::stop()
{
    m_running = false;
}

void ScopeWidget::setTriggerMode(TriggerMode mode)
{
    m_triggerMode = mode;
    m_triggered = false;
    update();
}

void ScopeWidget::setTriggerAddress(int universe, int channel)
{
    m_triggerChannel = channel;
    m_triggerUniverse = universe;
}

void ScopeWidget::setTriggerThreshold(int value)
{
    m_triggerLevel = value;
}

void ScopeWidget::setTriggerDelay(int triggerDelay)
{
    m_triggerDelay = triggerDelay;
    update();
}
