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

#include "universeview.h"
#include "ui_universeview.h"
#include "streamingacn.h"
#include "sacnlistener.h"
#include "preferences.h"
#include "consts.h"
#include "flickerfinderinfoform.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

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
    m_logger = NULL;
    ui->setupUi(this);
    connect(ui->universeDisplay, SIGNAL(selectedCellChanged(int)), this, SLOT(selectedAddressChanged(int)));


    ui->btnGo->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->sbUniverse->setEnabled(true);
    setUiForLoggingState(NOT_LOGGING);
}

UniverseView::~UniverseView()
{
    delete ui;
}

void UniverseView::startListening(int universe)
{
    ui->twSources->setRowCount(0);
    ui->btnGo->setEnabled(false);
    ui->btnPause->setEnabled(true);
    ui->sbUniverse->setEnabled(false);
    m_listener = sACNManager::getInstance()->getListener(universe);
    ui->universeDisplay->setUniverse(universe);

    // Add the existing sources
    for(int i=0; i<m_listener->sourceCount(); i++)
    {
        sourceOnline(m_listener->source(i));
    }

    connect(m_listener.data(), SIGNAL(sourceFound(sACNSource*)), this, SLOT(sourceOnline(sACNSource*)));
    connect(m_listener.data(), SIGNAL(sourceLost(sACNSource*)), this, SLOT(sourceOffline(sACNSource*)));
    connect(m_listener.data(), SIGNAL(sourceChanged(sACNSource*)), this, SLOT(sourceChanged(sACNSource*)));
    connect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));

    if(ui->sbUniverse->value()!=universe)
        ui->sbUniverse->setValue(universe);
}

void UniverseView::on_btnGo_pressed()
{
    startListening(ui->sbUniverse->value());
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
    ui->twSources->item(row,COL_DD)->setText(source->doing_per_channel ? tr("Yes") : tr("No"));
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
    resizeColumns();
}

void UniverseView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    resizeColumns();
}

void UniverseView::resizeColumns()
{
    // Attempt to resize so all columns fit
    int width = ui->twSources->width();

    int widthUnit = width/14;

    int used = 0;
    for(int i=COL_NAME; i<COL_END; i++)
    {
        switch(i)
        {
        case COL_NAME:
            break;
        case COL_CID:
        case COL_IP:
        case COL_DD:
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

void UniverseView::setUiForLoggingState(UniverseView::LOG_STATE state)
{
    switch(state)
    {
    case LOGGING:
        ui->btnLogToFile->setText(tr("Stop Log to File"));
        break;

    default:
    case NOT_LOGGING:
        ui->btnLogToFile->setText(tr("Start Log to File"));
        break;
    }
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
        int prio;
        if(list[address].winningSource->doing_per_channel)
            prio = list[address].winningSource->priority_array[address];
        else
            prio = list[address].winningSource->priority;

        info.append(tr("Winning Source : %1 @ %2 (Priority %3)")
                    .arg(list[address].winningSource->name)
                    .arg(Preferences::getInstance()->GetFormattedValue(list[address].level))
                    .arg(prio));
        if(list[address].otherSources.count()>0)
        {
            foreach(sACNSource *source, list[address].otherSources)
            {
                if(source->doing_per_channel)
                    prio = source->priority_array[address];
                else
                    prio = source->priority;
                info.append(tr("\nOther Source : %1 @ %2 (Priority %3)")
                            .arg(source->name)
                            .arg(Preferences::getInstance()->GetFormattedValue(source->level_array[address]))
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
    this->disconnect(m_listener.data());
    ui->btnGo->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->sbUniverse->setEnabled(true);
}

void UniverseView::on_btnLogToFile_pressed()
{
    if(!m_logger)
    {
        //Setup dialog box
        QFileDialog dialog(this);
        dialog.setWindowTitle(APP_NAME);
        dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        dialog.setNameFilter("CSV Files (*.csv)");
        dialog.setDefaultSuffix("csv");
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setViewMode(QFileDialog::Detail);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        if(dialog.exec()) {
            QString saveName = dialog.selectedFiles().at(0);
            if(saveName.isEmpty()) {
                return;
            }
            m_logger = new MergedUniverseLogger();
            m_logger->start(saveName, m_listener);
            setUiForLoggingState(LOGGING);
        }

    }
    else
    {
        m_logger->stop();
        m_logger->deleteLater();
        m_logger = nullptr;

        setUiForLoggingState(NOT_LOGGING);
    }

}

void UniverseView::on_btnStartFlickerFinder_pressed()
{
    if(ui->universeDisplay->flickerFinder())
    {
        ui->universeDisplay->setFlickerFinder(false);
        ui->btnStartFlickerFinder->setText(tr("Start Flicker Finder"));
    }
    else
    {
        if(Preferences::getInstance()->getFlickerFinderShowInfo())
        {
            FlickerFinderInfoForm form;
            int result = form.exec();
            if(!result) return;
        }
        ui->universeDisplay->setFlickerFinder(true);
        ui->btnStartFlickerFinder->setText(tr("Stop Flicker Finder"));
    }
}
