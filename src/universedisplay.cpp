// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "universedisplay.h"
#include "preferences.h"
#include <QPainter>
#include <QMouseEvent>

#define FIRST_COL_WIDTH 60
#define ROW_COUNT 16
#define COL_COUNT 32

static const QColor gridLineColor = QColor::fromRgb(0xc0, 0xc0, 0xc0);
static const QColor textColor = QColor::fromRgb(0x0, 0x0, 0x0);

UniverseDisplay::UniverseDisplay(QWidget *parent) : QWidget(parent)
{
    m_listener = 0;
    for(int i=0; i<512; i++)
        m_sources << sACNMergedAddress();
    setMinimumWidth(FIRST_COL_WIDTH + COL_COUNT * 12);
    m_selectedAddress = -1;
}

void UniverseDisplay::setUniverse(int universe)
{
    m_listener = sACNManager::getInstance()->getListener(universe);
    connect(m_listener, SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));
}

void UniverseDisplay::levelsChanged()
{
    if(m_listener)
    {
        m_sources = m_listener->mergedLevels();
        update();
    }
}

void UniverseDisplay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);


    painter.fillRect(this->rect(), QBrush(QColor("#FFF")));

    int cellWidth = (this->width() - FIRST_COL_WIDTH) / COL_COUNT;
    int firstColWidth = this->width() - COL_COUNT * cellWidth; // Use any leftover space to expand col 0
    int cellHeight = this->height() / (ROW_COUNT + 1);


    painter.setPen(textColor);
    painter.setFont(QFont("Segoe UI", 8));
    for(int row=1; row<ROW_COUNT+1; row++)
    {
        QRect textRect(0, row*cellHeight, firstColWidth, cellHeight);
        QString rowLabel = QString("%1 - %2")
                .arg(1+(row-1)*32)
                .arg((row)*32);
        painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
    }
    for(int col=0; col<COL_COUNT; col++)
    {
        QRect textRect(firstColWidth + col*cellWidth, 0, cellWidth, cellHeight);
        QString rowLabel = QString("%1")
                .arg(col+1);
        painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
    }
    for(int row=0; row<ROW_COUNT; row++)
        for(int col=0; col<COL_COUNT; col++)
        {
            int address = row*COL_COUNT + col;
            QRect textRect(firstColWidth + col*cellWidth, (row+1)*cellHeight, cellWidth, cellHeight);
            sACNMergedAddress addressInfo = m_sources[address];

            if(addressInfo.winningSource)
            {
                QColor fillColor = Preferences::getInstance()->colorForCID(addressInfo.winningSource->src_cid);

                QString rowLabel = QString("%1")
                        .arg(addressInfo.level);
                painter.fillRect(textRect, fillColor);
                painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            }

        }

    painter.setPen(gridLineColor);

    for(int row=0; row<ROW_COUNT + 1; row++)
    {
        QPoint start(0, row*cellHeight);
        QPoint end(this->width(), row*cellHeight);
        painter.drawLine(start, end);
    }
    for(int col=0; col<COL_COUNT + 1; col++)
    {
        QPoint start(firstColWidth + col*cellWidth, 0);
        QPoint end(firstColWidth + col*cellWidth, this->height());
        painter.drawLine(start, end);
    }

    // Draw the highlight for the selected one
    if(m_selectedAddress>-1)
    {
        int col = m_selectedAddress % 32;
        int row = m_selectedAddress / 32;
        QRect textRect(firstColWidth + col*cellWidth, (row+1)*cellHeight, cellWidth, cellHeight);
        painter.setPen(QColor("black"));
        painter.drawRect(textRect);


    }
}


void UniverseDisplay::mousePressEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);

    if(event->buttons() & Qt::LeftButton)
    {
        QPoint pos = event->pos();

        int cellWidth = (this->width() - FIRST_COL_WIDTH) / COL_COUNT;
        int firstColWidth = this->width() - COL_COUNT * cellWidth; // Use any leftover space to expand col 0
        int cellHeight = this->height() / (ROW_COUNT + 1);

        if(pos.x() < firstColWidth || pos.y() < cellHeight)
        {
            // We are in the top row or left column - clear selection
            m_selectedAddress = -1;
            update();
            return;
        }

        int col = (pos.x() - firstColWidth) / cellWidth;
        int row = (pos.y() - cellHeight) / cellHeight;

        int address = (row * 32) + col;
        if(m_selectedAddress!=address)
        {
            m_selectedAddress = address;
            emit selectedAddressChanged(m_selectedAddress);
            update();
        }
    }

}

void UniverseDisplay::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if(event->buttons() & Qt::LeftButton)
    {
        QPoint pos = event->pos();

        int cellWidth = (this->width() - FIRST_COL_WIDTH) / COL_COUNT;
        int firstColWidth = this->width() - COL_COUNT * cellWidth; // Use any leftover space to expand col 0
        int cellHeight = this->height() / (ROW_COUNT + 1);

        if(pos.x() < firstColWidth || pos.y() < cellHeight)
        {
            // We are in the top row or left column - clear selection
            m_selectedAddress = -1;
            update();
            return;
        }

        int col = (pos.x() - firstColWidth) / cellWidth;
        int row = (pos.y() - cellHeight) / cellHeight;

        int address = (row * 32) + col;
        if(m_selectedAddress!=address)
        {
            m_selectedAddress = address;
            emit selectedAddressChanged(m_selectedAddress);
            update();
        }
    }
}
