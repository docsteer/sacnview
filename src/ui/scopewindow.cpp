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

#include "scopewindow.h"
#include "ui_scopewindow.h"
#include "consts.h"
#include "preferences.h"
#include "sacnlistener.h"
#include <QRadioButton>
#include <QColorDialog>
#include <QDebug>

static const QList<int> COLOURS({
                                    Qt::white,
                                    Qt::red	,
                                    Qt::darkRed,
                                    Qt::green,
                                    Qt::darkGreen,
                                    Qt::blue,
                                    Qt::darkBlue,
                                    Qt::cyan,
                                    Qt::darkCyan,
                                    Qt::magenta,
                                    Qt::darkMagenta,
                                    Qt::yellow,
                                    Qt::darkYellow
                                });

static const QList<int> TIMEBASES({
    2000,
    1000,
    500,
    200,
    100,
    50,
    20,
    10,
    5,
});

ScopeWindow::ScopeWindow(int universe, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScopeWindow),
    m_defaultUniverse(universe)
{
    ui->setupUi(this);
    ui->dlTimebase->setMinimum(0);
    ui->dlTimebase->setMaximum(TIMEBASES.count() - 1);
    connect(ui->dlTimebase, SIGNAL(valueChanged(int)), this, SLOT(timebaseChanged(int)));
    ui->tableWidget->setRowCount(0);
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(false);

    m_radioGroup = new QButtonGroup(this);
    connect(m_radioGroup, SIGNAL(buttonPressed(int)), this, SLOT(on_buttonGroup_buttonPressed(int)));
    connect(ui->sbTriggerDelay, SIGNAL(valueChanged(int)), ui->widget, SLOT(setTriggerDelay(int)));
    connect(ui->widget, SIGNAL(stopped()), this, SLOT(on_scopeWidget_stopped()));

    // Set initial value
    ui->dlTimebase->setValue(2);

    // Setup trigger spinbox
    if(Preferences::getInstance()->GetDisplayFormat() == Preferences::PERCENT)
    {
        ui->sbTriggerLevel->setMinimum(0);
        ui->sbTriggerLevel->setMaximum(100);
        ui->sbTriggerLevel->setValue(50);
    }
    else
    {
        ui->sbTriggerLevel->setMinimum(0);
        ui->sbTriggerLevel->setMaximum(MAX_SACN_LEVEL);
        ui->sbTriggerLevel->setValue(MAX_SACN_LEVEL/2);
    }

}

ScopeWindow::~ScopeWindow()
{
    delete ui;
}

void ScopeWindow::timebaseChanged(int value)
{
    int timebase = TIMEBASES[value];
    if(timebase<1000)
        ui->lbTimebase->setText(tr("%1 ms/div").arg(timebase));
    else
        ui->lbTimebase->setText(tr("%1 s/div").arg(timebase/1000));
    ui->widget->setTimebase(timebase);
}

void ScopeWindow::on_btnStart_pressed()
{
    ui->widget->start();
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
}

void ScopeWindow::on_btnStop_pressed()
{
    ui->widget->stop();
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);
}

void ScopeWindow::on_btnAddChannel_pressed()
{
    int rowNumber =  m_channels.count();
    ScopeChannel *channel = new ScopeChannel(m_defaultUniverse, rowNumber % 512);

    QColor col((Qt::GlobalColor)COLOURS[rowNumber % COLOURS.count()]);
    channel->setColor(col);
    ui->widget->addChannel(channel);
    m_channels << channel;

    ui->tableWidget->setRowCount(m_channels.count());
    QTableWidgetItem *item = new QTableWidgetItem(QString::number(channel->universe()));
    ui->tableWidget->setItem(m_channels.count()-1, COL_UNIVERSE, item);

    item = new QTableWidgetItem(QString::number(channel->address()+1));
    ui->tableWidget->setItem(m_channels.count()-1, COL_ADDRESS, item);

    item = new QTableWidgetItem(tr("Yes"));
    item->setCheckState(Qt::Checked);
    ui->tableWidget->setItem(m_channels.count()-1, COL_ENABLED, item);

    item = new QTableWidgetItem();
    item->setBackground(col);
    item->setFlags(Qt::ItemIsEnabled);
    ui->tableWidget->setItem(m_channels.count()-1, COL_COLOUR, item);

    QWidget *containerWidget = new QWidget(this);
    QRadioButton *radio = new QRadioButton(containerWidget);
    m_radioGroup->addButton(radio, m_channels.count()-1);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    layout->addStretch();
    layout->addWidget(radio);
    layout->addStretch();
    containerWidget->setLayout(layout);

    ui->tableWidget->setCellWidget(m_channels.count()-1, COL_TRIGGER, containerWidget);


    item = new QTableWidgetItem(tr("No"));
    item->setCheckState(Qt::Unchecked);
    ui->tableWidget->setItem(m_channels.count()-1, COL_16BIT, item);

    if(!ui->widget->running())
        ui->btnStart->setEnabled(true);
}

void ScopeWindow::on_btnRemoveChannel_pressed()
{
    int index = ui->tableWidget->currentRow();
    if(index<0) return;

    ScopeChannel *channel = m_channels[index];
    ui->widget->removeChannel(channel);
    ui->tableWidget->removeRow(index);
    m_channels.removeAt(index);

    if(ui->tableWidget->rowCount() == 0)
    {
        ui->widget->stop();
        ui->btnStart->setEnabled(false);
        ui->btnStop->setEnabled(false);
    }
}


void ScopeWindow::on_tableWidget_cellDoubleClicked(int row, int col)
{
    if(col==COL_COLOUR)
    {
        ScopeChannel *channel = m_channels[row];
        QColorDialog dlg;
        QColor newColor = dlg.getColor(channel->color(), this);
        channel->setColor(newColor);
        QTableWidgetItem *w = ui->tableWidget->item(row, col);
        w->setBackground(newColor);
    }
}

void ScopeWindow::on_tableWidget_itemChanged(QTableWidgetItem * item)
{
    ScopeChannel *ch = m_channels[item->row()];
    int address = ch->address();
    int universe = ch->universe();
    bool ok;

    switch(item->column())
    {
    case COL_ADDRESS:
        address = item->text().toInt(&ok);
        if(address>=1 && address<=512 && ok)
        {
            sACNManager::tListener listener;
            if(!m_universes.contains(ch->universe()))
            {
                m_universes[ch->universe()] = sACNManager::getInstance()->getListener(ch->universe());
            }

            listener = m_universes[ch->universe()];

            listener->unMonitorAddress(ch->address(), this);
            ch->setAddress(address-1);
            listener->monitorAddress(ch->address(), this);
            ch->clear();
        }
        else
        {
            ui->tableWidget->blockSignals(true);
            item->setText(QString::number(ch->address()+1));
            ui->tableWidget->blockSignals(false);
        }
        break;

    case COL_UNIVERSE:
        universe = item->text().toInt(&ok);
        if(universe>=1 && universe<=MAX_SACN_UNIVERSE && ok && ch->universe()!=universe)
        {
            // Changing universe
            sACNManager::tListener listener;
            if(!m_universes.contains(universe))
            {
                m_universes[ch->universe()] = sACNManager::getInstance()->getListener(universe);
            }

            listener = m_universes[ch->universe()];

            ch->setUniverse(universe);
            disconnect(listener.data(), 0, this->ui->widget, 0);
            connect(listener.data(), SIGNAL(dataReady(int, QPointF)), this->ui->widget, SLOT(dataReady(int, QPointF)));
            listener->monitorAddress(ch->address(), this);
            ch->clear();
        }
        else
        {
            ui->tableWidget->blockSignals(true);
            item->setText(QString::number(ch->universe()));
            ui->tableWidget->blockSignals(false);
        }
        break;
    case COL_ENABLED:
        if(item->checkState() == Qt::Checked)
        {
            item->setText(tr("Yes"));
            ch->setEnabled(true);
        }
            else
        {
            item->setText(tr("No"));
            ch->setEnabled(false);
        }
        break;

    case COL_16BIT:
        if(item->checkState() == Qt::Checked)
        {
            item->setText(tr("Yes"));
            ch->setSixteenBit(true);
        }
            else
        {
            item->setText(tr("No"));
            ch->setSixteenBit(false);
        }
        break;
    }
}

void ScopeWindow::on_buttonGroup_buttonPressed(int id)
{
    int row = id;

    int universe = ui->tableWidget->item(row, COL_UNIVERSE)->text().toInt();
    int address = ui->tableWidget->item(row, COL_ADDRESS)->text().toInt()-1;

    ui->widget->setTriggerAddress(universe, address);
}

void ScopeWindow::on_cbTriggerMode_currentIndexChanged(int index)
{
    ui->widget->setTriggerMode((ScopeWidget::TriggerMode)index);
}

void ScopeWindow::on_sbTriggerLevel_valueChanged(int value)
{
    ui->widget->setTriggerThreshold(value);
}

void ScopeWindow::on_scopeWidget_stopped()
{
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);
}
