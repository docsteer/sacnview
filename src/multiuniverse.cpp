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

#include "multiuniverse.h"
#include "ui_multiuniverse.h"

#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include "sacneffectengine.h"
#include "preferences.h"
#include "consts.h"

MultiUniverse::MultiUniverse(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MultiUniverse)
{
    ui->setupUi(this);
}

MultiUniverse::~MultiUniverse()
{
    delete ui;
    for(int i=0; i<m_senders.count(); i++)
    {
        m_senders[i]->stopSending();
    }
    for(int i=0; i<m_fxEngines.count(); i++)
    {
        m_fxEngines[i]->shutdown();
    }

    qDeleteAll(m_fxEngines);
    qDeleteAll(m_senders);
}


void MultiUniverse::on_btnAddRow_pressed()
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row+1);

    int nextUniverse = 1;
    if(m_senders.count()>0)
        nextUniverse = m_senders.last()->universe() + 1;

    m_senders.append(new sACNSentUniverse(nextUniverse));
    m_fxEngines.append(new sACNEffectEngine());
    m_fxEngines.last()->setSender(m_senders.last());
    m_fxEngines.last()->setStartAddress(MIN_DMX_ADDRESS-1);
    m_fxEngines.last()->setEndAddress(MAX_DMX_ADDRESS-1);
    m_fxEngines.last()->setMode(sACNEffectEngine::FxManual);
    m_fxEngines.last()->start();
    m_senders.last()->setName(tr("sACNView_%1").arg(nextUniverse));


    QCheckBox *enableBox = new QCheckBox(this);
    enableBox->setStyleSheet("margin-left:50%; margin-right:50%;");
    enableBox->setChecked(m_senders.last()->isSending());
    ui->tableWidget->setCellWidget(row, COL_ENABLED, enableBox);
    m_widgetToIndex[enableBox] = row;
    connect(enableBox, SIGNAL(toggled(bool)), this, SLOT(enableChanged(bool)));
    connect(enableBox, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));


    QSpinBox *sb = new QSpinBox(this);
    sb->setMinimum(MIN_SACN_UNIVERSE);
    sb->setMaximum(MAX_SACN_UNIVERSE);
    sb->setValue(m_senders.last()->universe());
    ui->tableWidget->setCellWidget(row, COL_UNIVERSE, sb);
    m_widgetToIndex[sb] = row;
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(universeChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    sb = new QSpinBox(this);
    sb->setMinimum(MIN_DMX_ADDRESS);
    sb->setMaximum(MAX_DMX_ADDRESS);
    sb->setValue(MIN_DMX_ADDRESS);
    ui->tableWidget->setCellWidget(row, COL_STARTADDR, sb);
    m_widgetToIndex[sb] = row;
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(startChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    sb = new QSpinBox(this);
    sb->setMinimum(MIN_DMX_ADDRESS);
    sb->setMaximum(MAX_DMX_ADDRESS);
    sb->setValue(MAX_DMX_ADDRESS);
    ui->tableWidget->setCellWidget(row, COL_ENDADDR, sb);
    m_widgetToIndex[sb] = row;
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(endChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));


    sb = new QSpinBox(this);
    sb->setMinimum(MIN_SACN_PRIORITY);
    sb->setMaximum(MAX_SACN_PRIORITY);
    sb->setValue(m_senders.last()->perSourcePriority());
    ui->tableWidget->setCellWidget(row, COL_PRIORITY, sb);
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(priorityChanged(int)));
    m_widgetToIndex[sb] = row;

    QComboBox *cb = new QComboBox(this);
    cb->addItems(FX_MODE_DESCRIPTIONS);
    //cb->setCurrentIndex(m_sourceInfo.last().effect);
    ui->tableWidget->setCellWidget(row, COL_EFFECT, cb);
    m_widgetToIndex[cb] = row;
    connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(fxChanged(int)));
    connect(cb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    setupControl(row, sACNEffectEngine::FxManual);

    QTableWidgetItem *item = new QTableWidgetItem(m_senders.last()->name());
    ui->tableWidget->setItem(row, COL_NAME, item);

}

void MultiUniverse::removeWidgetFromIndex(QObject *o)
{
    QWidget *w = dynamic_cast<QWidget *>(o);
    if(m_widgetToIndex.contains(w))
    {
        m_widgetToIndex.remove(w);
    }
}

void MultiUniverse::on_btnRemoveRow_pressed()
{
    int row = ui->tableWidget->currentRow();
    ui->tableWidget->removeRow(row);
    delete m_senders[row];
    m_senders.removeAt(row);
}


void MultiUniverse::universeChanged(int value)
{
    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    m_senders[index]->setUniverse(value);
}

void MultiUniverse::priorityChanged(int value)
{
    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    m_senders[index]->setPerSourcePriority(value);
}

void MultiUniverse::startChanged(int value)
{

    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    m_fxEngines[index]->setStartAddress(value-1);
}

void MultiUniverse::endChanged(int value)
{

    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    m_fxEngines[index]->setEndAddress(value-1);
}

void MultiUniverse::fxChanged(int value)
{
    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    m_fxEngines[index]->setMode((sACNEffectEngine::FxMode)value);

    setupControl(index, (sACNEffectEngine::FxMode)value);
}

void MultiUniverse::enableChanged(bool enable)
{
    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    if(enable)
        m_senders[index]->startSending();
    else
        m_senders[index]->stopSending();
}

void MultiUniverse::setupControl(int row, sACNEffectEngine::FxMode mode)
{
    switch(mode)
    {
    case sACNEffectEngine::FxManual:
        {
        QWidget *controlWidget = new QWidget(this);
        QLabel *controlLabel = new QLabel(controlWidget);
        m_levelLabels[row] = controlLabel;
        QSlider *slider = new QSlider(Qt::Horizontal, controlWidget);
        slider->setMinimum(MIN_SACN_LEVEL);
        slider->setMaximum(MAX_SACN_LEVEL);
        QHBoxLayout *controlLayout = new QHBoxLayout();
        controlLayout->setMargin(0);
        controlLayout->addWidget(controlLabel);
        controlLayout->addWidget(slider);
        controlWidget->setLayout(controlLayout);
        ui->tableWidget->setCellWidget(row, COL_CONTROL, controlWidget);
        m_widgetToIndex[slider] = row;
        connect(slider, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));
        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(controlSliderMoved(int)));
        }
        break;

    case sACNEffectEngine::FxChase:
    case sACNEffectEngine::FxRamp:
    case sACNEffectEngine::FxSinewave:
        {
        QWidget *controlWidget = new QWidget(this);
        QLabel *slowerLabel = new QLabel(tr("Slower"), controlWidget);
        QLabel *fasterLabel = new QLabel(tr("Faster"), controlWidget);
        QSlider *slider = new QSlider(Qt::Horizontal, controlWidget);
        slider->setMinimum(1);
        slider->setMaximum(500);
        slider->setValue(m_fxEngines[row]->rate());
        QHBoxLayout *controlLayout = new QHBoxLayout();
        controlLayout->setMargin(0);
        controlLayout->addWidget(slowerLabel);
        controlLayout->addWidget(slider);
        controlLayout->addWidget(fasterLabel);
        controlWidget->setLayout(controlLayout);
        ui->tableWidget->setCellWidget(row, COL_CONTROL, controlWidget);
        m_widgetToIndex[slider] = row;
        connect(controlWidget, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));
        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(controlSliderMoved(int)));
        }
        break;
    case sACNEffectEngine::FxDate:
        {
        QComboBox *cb = new QComboBox(this);
        cb->addItem(tr("EU Date Style"));
        cb->addItem(tr("US Date Style"));
        ui->tableWidget->setCellWidget(row, COL_CONTROL, cb);
        m_widgetToIndex[cb] = row;
        connect(cb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));
        connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(controlComboChanged(int)));
        }
        break;
    case sACNEffectEngine::FxText:
        {
        ui->tableWidget->setCellWidget(row, COL_CONTROL, nullptr);
        QTableWidgetItem *item = new QTableWidgetItem(m_fxEngines[row]->text());
        ui->tableWidget->setItem(row, COL_CONTROL, item);
        m_fxEngines[row]->setRate(1);
        }
        break;
    default:
        break;
    }

}

void MultiUniverse::controlSliderMoved(int value)
{
    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    if(m_fxEngines[index]->mode() == sACNEffectEngine::FxManual)
    {
        m_fxEngines[index]->setManualLevel(value);
        m_levelLabels[index]->setText(Preferences::getInstance()->GetFormattedValue(value));
    }
    else
        m_fxEngines[index]->setRate(value);
}

void MultiUniverse::controlComboChanged(int value)
{

    int index = m_widgetToIndex.value(dynamic_cast<QWidget *>(sender()), -1);
    if(index==-1) return;

    if(value==0)
        m_fxEngines[index]->setDateStyle(sACNEffectEngine::dsEU);
    else
        m_fxEngines[index]->setDateStyle(sACNEffectEngine::dsUSA);
}

void MultiUniverse::on_tableWidget_cellChanged(int row, int column)
{
    if(column==COL_NAME)
    {
        if(ui->tableWidget->item(row, column))
            m_senders[row]->setName(ui->tableWidget->item(row, column)->text());
    }
    if(column==COL_CONTROL)
    {
        if(ui->tableWidget->item(row, column))
            m_fxEngines[row]->setText(ui->tableWidget->item(row, column)->text());
    }
}

