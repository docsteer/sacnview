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

#include "universeview.h"
#include "ui_universeview.h"
#include "streamingacn.h"
#include "sacnlistener.h"
#include "preferences.h"

QString onlineToString(bool value)
{
    if(value)
        return QString("Online");
    return QString("Offline");
}

QString protocolVerToString(int value)
{
    switch(value)
    {
    case sACNProtocolDraft:
        return QString("Draft");
    case sACNProtocolRelease:
        return QString("Release");
    }

    return QString("Unknown");
}

UniverseView::UniverseView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UniverseView)
{
    m_selectedAddress = -1;
    m_listener = NULL;
    ui->setupUi(this);
    connect(ui->universeDisplay, SIGNAL(selectedCellChanged(int)), this, SLOT(selectedAddressChanged(int)));


    ui->btnGo->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->sbUniverse->setEnabled(true);
}

UniverseView::~UniverseView()
{
    delete ui;
}

void UniverseView::on_btnGo_pressed()
{
    ui->twSources->setRowCount(0);
    ui->btnGo->setEnabled(false);
    ui->btnPause->setEnabled(true);
    ui->sbUniverse->setEnabled(false);
    m_listener = sACNManager::getInstance()->getListener(ui->sbUniverse->value());
    ui->universeDisplay->setUniverse(ui->sbUniverse->value());

    // Add the existing sources
    for(int i=0; i<m_listener->sourceCount(); i++)
    {
        sourceOnline(m_listener->source(i));
    }

    connect(m_listener, SIGNAL(sourceFound(sACNSource*)), this, SLOT(sourceOnline(sACNSource*)));
    connect(m_listener, SIGNAL(sourceLost(sACNSource*)), this, SLOT(sourceOffline(sACNSource*)));
    connect(m_listener, SIGNAL(sourceChanged(sACNSource*)), this, SLOT(sourceChanged(sACNSource*)));
    connect(m_listener, SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));
}

void UniverseView::sourceChanged(sACNSource *source)
{
    if(!m_listener) return;
    if(!m_sourceToTableRow.contains(source))
    {
        return;
    }

    int row = m_sourceToTableRow[source];
    ui->twSources->item(row,COL_NAME)->setText(source->name);
    ui->twSources->item(row,COL_NAME)->setBackgroundColor(Preferences::getInstance()->colorForCID(source->src_cid));
    ui->twSources->item(row,COL_CID)->setText(source->cid_string());
    ui->twSources->item(row,COL_PRIO)->setText(QString::number(source->priority));
    ui->twSources->item(row,COL_IP)->setText(source->ip.toString());
    ui->twSources->item(row,COL_FPS)->setText(QString::number((source->fps)));
    ui->twSources->item(row,COL_SEQ_ERR)->setText(QString::number(source->seqErr));
    ui->twSources->item(row,COL_JUMPS)->setText(QString::number(source->jumps));
    ui->twSources->item(row,COL_ONLINE)->setText(onlineToString(source->src_valid));
    ui->twSources->item(row,COL_VER)->setText(protocolVerToString(source->protocol_version));
    ui->twSources->item(row,COL_DD)->setText(QString::number(source->doing_per_channel));
}

void UniverseView::sourceOnline(sACNSource *source)
{
    if(!m_listener) return;
    int row = ui->twSources->rowCount();
    ui->twSources->setRowCount(row+1);
    m_sourceToTableRow[source] = row;

    ui->twSources->setItem(row,COL_NAME,    new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_CID,     new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_PRIO,    new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_IP,      new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_FPS,     new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_SEQ_ERR, new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_JUMPS,   new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_ONLINE,  new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_VER,     new QTableWidgetItem() );
    ui->twSources->setItem(row,COL_DD,      new QTableWidgetItem() );

    sourceChanged(source);

}

void UniverseView::sourceOffline(sACNSource *source)
{
    if(!m_listener) return;
    sourceChanged(source);
}

void UniverseView::levelsChanged()
{
    if(!m_listener) return;
    if(m_selectedAddress>-1)
        selectedAddressChanged(m_selectedAddress);
}

void UniverseView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Attempt to resize so all columns fit
    // 7 small columns, 1 large column (CID), 1 IP, 1 sized to fill remainig space (Name)
    // CID is around 4x width of small columns
    // IP is 2x width of small columns
    // Name is around 2x width of small columns

    int width = ui->twSources->width();

    int widthUnit = width/15;

    int used = 0;
    for(int i=COL_NAME; i<COL_END; i++)
    {
        switch(i)
        {
        case COL_NAME:
            break;
        case COL_CID:
            ui->twSources->setColumnWidth(i, 4*widthUnit);
            used += 4*widthUnit;
            break;
        case COL_IP:
            ui->twSources->setColumnWidth(i, 2*widthUnit);
            used += 2*widthUnit;
            break;
        default:
            ui->twSources->setColumnWidth(i, widthUnit);
            used += widthUnit;
            break;
        }
    }

    ui->twSources->setColumnWidth(COL_NAME, width-used-5);
}

void UniverseView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // Attempt to resize so all columns fit
    // 7 small columns, 1 large column (CID), 1 IP, 1 sized to fill remainig space (Name)
    // CID is around 4x width of small columns
    // IP is 2x width of small columns
    // Name is around 2x width of small columns

    int width = ui->twSources->width();

    int widthUnit = width/15;

    int used = 0;
    for(int i=COL_NAME; i<COL_END; i++)
    {
        switch(i)
        {
        case COL_NAME:
            break;
        case COL_CID:
            ui->twSources->setColumnWidth(i, 4*widthUnit);
            used += 4*widthUnit;
            break;
        case COL_IP:
            ui->twSources->setColumnWidth(i, 2*widthUnit);
            used += 2*widthUnit;
            break;
        default:
            ui->twSources->setColumnWidth(i, widthUnit);
            used += widthUnit;
            break;
        }
    }

    ui->twSources->setColumnWidth(COL_NAME, width-used-5);

}

void UniverseView::selectedAddressChanged(int address)
{
    ui->teInfo->clear();
    m_selectedAddress = address;
    ui->teInfo->clear();
    if(address<0) return;

    if(!m_listener) return;
    sACNMergedSourceList list = m_listener->mergedLevels();

    QString info;

    info.append(tr("Address : %1\n")
                .arg(address+1));

    if(list[address].winningSource)
    {
            info.append(tr("Winning Source : %1 @ %2")
                        .arg(list[address].winningSource->name)
                        .arg(list[address].level));
            if(list[address].otherSources.count()>0)
            {
                foreach(sACNSource *source, list[address].otherSources)
                {
                    int prio;
                    if(source->doing_per_channel)
                        prio = source->priority_array[address];
                    else
                        prio = source->priority;
                    info.append(tr("\nOther Source : %1 @ %2 (Priority %3)")
                                .arg(source->name)
                                .arg(source->level_array[address])
                                .arg(prio));
                }
            }
    }
    if(!list[address].winningSource)
    {
        info.append(tr("No Sources"));
    }

    ui->teInfo->setPlainText(info);
}

void UniverseView::on_btnPause_pressed()
{
    ui->universeDisplay->pause();
    this->disconnect(m_listener);
    m_listener = NULL;
    ui->btnGo->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->sbUniverse->setEnabled(true);
}
