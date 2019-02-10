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
#include <QToolTip>

#define FIRST_COL_WIDTH 60
#define ROW_COUNT 16
#define COL_COUNT 32
#define CELL_WIDTH 25
#define CELL_HEIGHT 18
#define CELL_COUNT 512


GridWidget::GridWidget(QWidget *parent)
    : QWidget(parent)
    , m_colors(CELL_COUNT, this->palette().color(QPalette::Base))
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

    painter.fillRect(QRectF(0,0, wantedWidth*scaleWidth, wantedHeight*scaleHeight), pal.color(QPalette::Base));

    painter.setPen(pal.color(QPalette::Text));


    // Determine the font size. This seems to look best based on increments of scale, not continuous scaling
    QFont font = this->font();

    if(scaleHeight<=1.2)
        font.setPointSize(8);
    else if(scaleHeight<1.9)
        font.setPointSize(10);
    else
        font.setPointSize(12);
    painter.setFont(font);

    // Fill bars for background of the row and column labels
    QRect rowLabelRect(0, 0,  FIRST_COL_WIDTH*scaleWidth, height());
    painter.fillRect(rowLabelRect, QBrush(pal.color(QPalette::AlternateBase)));

    QRect colLabelRect(0, 0,  width(), m_cellHeight*scaleHeight);
    painter.fillRect(colLabelRect, QBrush(pal.color(QPalette::AlternateBase)));


    for(int row=1; row<ROW_COUNT+1; row++)
    {
        QRect textRect(0, row*m_cellHeight*scaleHeight, FIRST_COL_WIDTH*scaleWidth, m_cellHeight*scaleHeight);
        QString rowLabel = QString("%1-%2")
                .arg(1+(row-1)*32)
                .arg((row)*32);

        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, rowLabel);
    }
    for(int col=0; col<COL_COUNT; col++)
    {
        QRect textRect((FIRST_COL_WIDTH + col*CELL_WIDTH) * scaleWidth, 0, CELL_WIDTH*scaleWidth, m_cellHeight*scaleHeight);
        QString rowLabel = QString("%1")
                .arg(col+1);

        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, rowLabel);
    }

    for(int row=0; row<ROW_COUNT; row++)
        for(int col=0; col<COL_COUNT; col++)
        {
            int address = row*COL_COUNT + col;
            QRect textRect((FIRST_COL_WIDTH + col*CELL_WIDTH)*scaleWidth, (row+1)*m_cellHeight*scaleHeight, CELL_WIDTH*scaleWidth, m_cellHeight*scaleHeight);
            QString value = m_values[address];

            if(!value.isEmpty())
            {
                QColor fillColor = m_colors[address];

                QString rowLabel = value;
                painter.fillRect(textRect, fillColor);
                painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, rowLabel);
            }

        }

    painter.setPen(pal.color(QPalette::AlternateBase));

    for(int row=0; row<ROW_COUNT + 1; row++)
    {
        QPoint start(0, row*m_cellHeight*scaleHeight);
        QPoint end(wantedWidth*scaleWidth, row*m_cellHeight*scaleHeight);
        painter.drawLine(start, end);
    }
    for(int col=0; col<COL_COUNT + 1; col++)
    {
        QPoint start((FIRST_COL_WIDTH + col*CELL_WIDTH)*scaleWidth, 0);
        QPoint end((FIRST_COL_WIDTH + col*CELL_WIDTH)*scaleWidth, wantedHeight*scaleHeight);
        painter.drawLine(start, end);
    }

    // Draw the highlight for the selected ones
    QListIterator<int> i(m_selectedAddresses);
    while(i.hasNext())
    {
        int selectedAddress = i.next();
        int col = selectedAddress % 32;
        int row = selectedAddress / 32;
        QRect textRect((FIRST_COL_WIDTH + col*CELL_WIDTH)*scaleWidth, (row+1)*m_cellHeight*scaleHeight, CELL_WIDTH*scaleWidth, m_cellHeight*scaleHeight);
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

    if(!(event->buttons() & Qt::LeftButton))
        return;

    int address = cellHitTest(event->pos());

    // Handle multi-selection
    if(m_allowMultiSelect)
    {
        if(address==-1)
        {
            m_selectedAddresses.clear();
            emit selectedCellsChanged(m_selectedAddresses);
            update();
            return;
        }
        if((event->modifiers() & Qt::ShiftModifier) && m_selectedAddresses.count()>0)
        {
            int previous = m_selectedAddresses.last();
            int low = qMin(previous, address);
            int high = qMax(previous, address);
            for(int i=low; i<=high; i++)
                if(!m_selectedAddresses.contains(i))
                    m_selectedAddresses << i;
            update();
            emit selectedCellsChanged(m_selectedAddresses);
            return;
        }
        if(event->modifiers() & Qt::ControlModifier)
        {

            if(m_selectedAddresses.contains(address)) {
                m_selectedAddresses.removeAll(address);
            }
            else {
                m_selectedAddresses << address;
            }
            update();
            emit selectedCellsChanged(m_selectedAddresses);
            return;
        }

        m_selectedAddresses.clear();
        m_selectedAddresses << address;
        update();
        emit selectedCellsChanged(m_selectedAddresses);
        return;
    }
    else // Handle single-selection
    {
        if(address==-1)
        {
            m_selectedAddresses.clear();
            emit selectedCellsChanged(m_selectedAddresses);
            update();
            return;
        }
        else {
            m_selectedAddresses.clear();
            m_selectedAddresses << address;
            emit selectedCellsChanged(m_selectedAddresses);
            update();
            return;
        }

    }

}

void GridWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if(!(event->buttons() & Qt::LeftButton))
        return;

    int address = cellHitTest(event->pos());

    // Handle multi-selection
    if(m_allowMultiSelect)
    {
        if(address==-1)
        {
            m_selectedAddresses.clear();
            emit selectedCellsChanged(m_selectedAddresses);
            update();
            return;
        }
        if((event->modifiers() & Qt::ShiftModifier) && m_selectedAddresses.count()>0)
        {
            int previous = m_selectedAddresses.last();
            int low = qMin(previous, address);
            int high = qMax(previous, address);
            for(int i=low; i<=high; i++)
                if(!m_selectedAddresses.contains(i))
                    m_selectedAddresses << i;
            update();
            emit selectedCellsChanged(m_selectedAddresses);
            return;
        }
        if((event->modifiers() & Qt::ControlModifier) )
        {
            if(m_selectedAddresses.contains(address) && (address != m_selectedAddresses.last()) ) {
                m_selectedAddresses.removeAll(address);
            }
            else {
                m_selectedAddresses << address;
            }
            update();
            emit selectedCellsChanged(m_selectedAddresses);
            return;
        }

        m_selectedAddresses.clear();
        m_selectedAddresses << address;
        update();
        emit selectedCellsChanged(m_selectedAddresses);
        return;
    }
    else // Handle single-selection
    {
        if(address==-1)
        {
            m_selectedAddresses.clear();
            emit selectedCellsChanged(m_selectedAddresses);
            update();
            return;
        }
        else {
            m_selectedAddresses.clear();
            m_selectedAddresses << address;
            emit selectedCellsChanged(m_selectedAddresses);
            update();
            return;
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

bool GridWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        int cell = cellHitTest(helpEvent->pos());
        if (cell != -1) {
            QToolTip::showText(helpEvent->globalPos(), QString::number(cell+1));
        } else {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }
    return QWidget::event(event);
}
