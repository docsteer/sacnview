// Copyright 2016 Tom Barthel-Steer
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

#include "gridwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPalette>
#include <QApplication>
#include <QStyle>

#define FIRST_COL_WIDTH 60
#define ROW_COUNT 16
#define COL_COUNT 32
#define CELL_WIDTH 25
#define CELL_HEIGHT 18
#define CELL_COUNT 512


GridWidget::GridWidget(QWidget *parent)
    : QWidget(parent)
    , m_selectedAddress(-1)
    , m_colors(CELL_COUNT, Qt::white)
    , m_cellHeight(CELL_HEIGHT)
{
    for(int i=0; i<CELL_COUNT; i++)
    {
        m_values << QString();
    }
}

QSize GridWidget::minimumSizeHint() const
{
    return QWidget::minimumSizeHint(); //sizeHint();
}

QSize GridWidget::sizeHint() const
{
    return QWidget::sizeHint();
    //return QSize( FIRST_COL_WIDTH + CELL_WIDTH * COL_COUNT, CELL_HEIGHT * (ROW_COUNT + 1));
}

void GridWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPalette pal = this->palette();

    qreal wantedHeight = m_cellHeight * (ROW_COUNT + 1);
    qreal wantedWidth = FIRST_COL_WIDTH + CELL_WIDTH * COL_COUNT;

    qreal scaleWidth = width()/wantedWidth;
    qreal scaleHeight = height()/ wantedHeight;

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.scale(scaleWidth,scaleHeight);

    painter.fillRect(QRectF(0,0, wantedWidth, wantedHeight), pal.color(QPalette::Base));

    painter.setPen(pal.color(QPalette::Text));
    painter.setFont(QFont("Segoe UI", 8));
    for(int row=1; row<ROW_COUNT+1; row++)
    {
        QRect textRect(0, row*m_cellHeight, FIRST_COL_WIDTH, m_cellHeight);
        QString rowLabel = QString("%1 - %2")
                .arg(1+(row-1)*32)
                .arg((row)*32);
        painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
    }
    for(int col=0; col<COL_COUNT; col++)
    {
        QRect textRect(FIRST_COL_WIDTH + col*CELL_WIDTH, 0, CELL_WIDTH, m_cellHeight);
        QString rowLabel = QString("%1")
                .arg(col+1);
        painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
    }
    for(int row=0; row<ROW_COUNT; row++)
        for(int col=0; col<COL_COUNT; col++)
        {
            int address = row*COL_COUNT + col;
            QRect textRect(FIRST_COL_WIDTH + col*CELL_WIDTH, (row+1)*m_cellHeight, CELL_WIDTH, m_cellHeight);
            QString value = m_values[address];

            if(!value.isEmpty())
            {
                QColor fillColor = m_colors[address];

                QString rowLabel = value;
                painter.fillRect(textRect, fillColor);
                painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            }

        }

    painter.setPen(pal.color(QPalette::AlternateBase));

    for(int row=0; row<ROW_COUNT + 1; row++)
    {
        QPoint start(0, row*m_cellHeight);
        QPoint end(wantedWidth, row*m_cellHeight);
        painter.drawLine(start, end);
    }
    for(int col=0; col<COL_COUNT + 1; col++)
    {
        QPoint start(FIRST_COL_WIDTH + col*CELL_WIDTH, 0);
        QPoint end(FIRST_COL_WIDTH + col*CELL_WIDTH, wantedHeight);
        painter.drawLine(start, end);
    }

    // Draw the highlight for the selected one
    if(m_selectedAddress>-1)
    {
        int col = m_selectedAddress % 32;
        int row = m_selectedAddress / 32;
        QRect textRect(FIRST_COL_WIDTH + col*CELL_WIDTH, (row+1)*m_cellHeight, CELL_WIDTH, m_cellHeight);
        painter.setPen(pal.color(QPalette::Highlight));
        painter.drawRect(textRect);
    }
}

int GridWidget::cellHitTest(const QPoint &point)
{
    qreal wantedHeight = m_cellHeight * (ROW_COUNT + 1);
    qreal wantedWidth = FIRST_COL_WIDTH + CELL_WIDTH * COL_COUNT;

    qreal scaleWidth = width()/wantedWidth;
    qreal scaleHeight = height()/ wantedHeight;

    QRectF drawnRect(0, 0, wantedWidth * scaleWidth, wantedHeight * scaleHeight);
    drawnRect.moveCenter(this->rect().center());
    drawnRect.moveTop(0);

    if(!drawnRect.contains(point))
        return -1;


    qreal cellWidth = (drawnRect.width() - FIRST_COL_WIDTH * scaleWidth) / COL_COUNT;
    qreal firstColWidth = drawnRect.width() - (COL_COUNT * cellWidth);
    qreal cellHeight = drawnRect.height() / (ROW_COUNT + 1);

    int col = (point.x() - drawnRect.left() - firstColWidth) / cellWidth;
    int row = (point.y() - drawnRect.top() - cellHeight) / cellHeight;

    if(col<0)  return -1;
    if(row<0)  return -1;

    int address = (row * 32) + col;
    if(address>511 || address<0) return -1;
    return address;

}

void GridWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    if(event->buttons() & Qt::LeftButton)
    {
        int address = cellHitTest(event->pos());
        if(m_selectedAddress!=address)
        {
            m_selectedAddress = address;
            emit selectedCellChanged(m_selectedAddress);
            update();
        }
    }

}

void GridWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if(event->buttons() & Qt::LeftButton)
    {
        int address = cellHitTest(event->pos());
        if(m_selectedAddress!=address)
        {
            m_selectedAddress = address;
            emit selectedCellChanged(m_selectedAddress);
            update();
        }
    }
}

void GridWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if(event->buttons() & Qt::LeftButton)
    {
        quint16 address = cellHitTest(event->pos());
        emit cellDoubleClick(address);
    }
}

void GridWidget::setCellColor(int cell, const QColor &color)
{
    m_colors[cell] = color;
}

void GridWidget::setCellValue(int cell, const QString &value)
{
    m_values[cell] = value;
}

QString GridWidget::cellValue(int cell)
{
    return m_values[cell];
}
