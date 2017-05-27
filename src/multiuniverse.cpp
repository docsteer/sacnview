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

    while (m_fxEngines.size())
    {
       m_fxEngines.front()->deleteLater();
       m_fxEngines.removeFirst();
    }

    while (m_senders.size())
    {
       m_senders.front()->deleteLater();
       m_senders.removeFirst();
    }
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
    {
        QString name = Preferences::getInstance()->GetDefaultTransmitName();
        QString postfix = tr("_%1").arg(nextUniverse);
        name.truncate(MAX_SOURCE_NAME_LEN - postfix.length());
        m_senders.last()->setName(name.trimmed() + postfix);
    }

    QCheckBox *enableBox = new QCheckBox(this);
    enableBox->setStyleSheet("margin-left:50%; margin-right:50%;");
    enableBox->setChecked(m_senders.last()->isSending());
    ui->tableWidget->setCellWidget(row, COL_ENABLED, enableBox);
    m_widgetToFxEngine[enableBox] = m_fxEngines.last();
    m_widgetToSender[enableBox] = m_senders.last();
    connect(enableBox, SIGNAL(toggled(bool)), this, SLOT(enableChanged(bool)));
    connect(enableBox, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));


    QSpinBox *sb = new QSpinBox(this);
    sb->setMinimum(MIN_SACN_UNIVERSE);
    sb->setMaximum(MAX_SACN_UNIVERSE);
    sb->setValue(m_senders.last()->universe());
    ui->tableWidget->setCellWidget(row, COL_UNIVERSE, sb);
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(universeChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    sb = new QSpinBox(this);
    sb->setMinimum(MIN_DMX_ADDRESS);
    sb->setMaximum(MAX_DMX_ADDRESS);
    sb->setValue(MIN_DMX_ADDRESS);
    ui->tableWidget->setCellWidget(row, COL_STARTADDR, sb);
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(startChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    sb = new QSpinBox(this);
    sb->setMinimum(MIN_DMX_ADDRESS);
    sb->setMaximum(MAX_DMX_ADDRESS);
    sb->setValue(MAX_DMX_ADDRESS);
    ui->tableWidget->setCellWidget(row, COL_ENDADDR, sb);
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(endChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));


    sb = new QSpinBox(this);
    sb->setMinimum(MIN_SACN_PRIORITY);
    sb->setMaximum(MAX_SACN_PRIORITY);
    sb->setValue(m_senders.last()->perSourcePriority());
    ui->tableWidget->setCellWidget(row, COL_PRIORITY, sb);
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(priorityChanged(int)));
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();

    QComboBox *cb = new QComboBox(this);
    cb->addItems(FX_MODE_DESCRIPTIONS);
    //cb->setCurrentIndex(m_sourceInfo.last().effect);
    ui->tableWidget->setCellWidget(row, COL_EFFECT, cb);
    m_widgetToFxEngine[cb] = m_fxEngines.last();
    m_widgetToSender[cb] = m_senders.last();

    connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(fxChanged(int)));
    connect(cb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    setupControl(row, sACNEffectEngine::FxManual);

    QTableWidgetItem *item = new QTableWidgetItem(m_senders.last()->name());
    ui->tableWidget->setItem(row, COL_NAME, item);

}

void MultiUniverse::removeWidgetFromIndex(QObject *o)
{
    QWidget *w = dynamic_cast<QWidget *>(o);
    m_widgetToFxEngine.remove(w);
    m_widgetToSender.remove(w);
}

void MultiUniverse::on_btnRemoveRow_pressed()
{
    int row = ui->tableWidget->currentRow();
    if (row == -1) return;
    ui->tableWidget->removeRow(row);
    m_fxEngines[row]->deleteLater();
    m_senders[row]->deleteLater();
    m_fxEngines.removeAt(row);
    m_senders.removeAt(row);
}


void MultiUniverse::universeChanged(int value)
{
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToSender.contains(w))
    {
        sACNSentUniverse *u = m_widgetToSender.value(w);
        u->setUniverse(value);
    }
}

void MultiUniverse::priorityChanged(int value)
{
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToSender.contains(w))
    {
        sACNSentUniverse *u = m_widgetToSender.value(w);
        u->setPerSourcePriority(value);
    }
}

void MultiUniverse::startChanged(int value)
{
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToFxEngine.contains(w))
    {
        sACNEffectEngine *e = m_widgetToFxEngine.value(w);
        e->setStartAddress(value-1);
    }
}

void MultiUniverse::endChanged(int value)
{
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToFxEngine.contains(w))
    {
        sACNEffectEngine *e = m_widgetToFxEngine.value(w);
        e->setEndAddress(value-1);
    }
}

void MultiUniverse::fxChanged(int value)
{
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToFxEngine.contains(w))
    {
        sACNEffectEngine *e = m_widgetToFxEngine.value(w);
        e->setMode((sACNEffectEngine::FxMode)value);

        int index = m_fxEngines.indexOf(e);

        setupControl(index, (sACNEffectEngine::FxMode)value);
    }
}

void MultiUniverse::enableChanged(bool enable)
{
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToSender.contains(w))
    {
        sACNSentUniverse *u = m_widgetToSender.value(w);

        if(enable)
            u->startSending();
        else
            u->stopSending();
    }
}

void MultiUniverse::setupControl(int row, sACNEffectEngine::FxMode mode)
{
    switch(mode)
    {
    case sACNEffectEngine::FxManual:
        {
        QWidget *controlWidget = new QWidget(this);
        QLabel *controlLabel = new QLabel("0", controlWidget);
        QSlider *slider = new QSlider(Qt::Horizontal, controlWidget);
        slider->setMinimum(MIN_SACN_LEVEL);
        slider->setMaximum(MAX_SACN_LEVEL);
        m_widgetToLevelLabel[slider] = controlLabel;
        QHBoxLayout *controlLayout = new QHBoxLayout();
        controlLayout->setMargin(0);
        controlLayout->addWidget(controlLabel);
        controlLayout->addWidget(slider);
        controlWidget->setLayout(controlLayout);
        ui->tableWidget->setCellWidget(row, COL_CONTROL, controlWidget);
        m_widgetToFxEngine[slider] = m_fxEngines[row];
        m_widgetToSender[slider] = m_senders[row];
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
        m_widgetToFxEngine[slider] = m_fxEngines[row];
        m_widgetToSender[slider] = m_senders[row];
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
        m_widgetToFxEngine[cb] = m_fxEngines[row];
        m_widgetToSender[cb] = m_senders[row];
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
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToFxEngine.contains(w))
    {
        sACNEffectEngine *e = m_widgetToFxEngine[w];
        if(e->mode() == sACNEffectEngine::FxManual)
        {
            e->setManualLevel(value);
            m_widgetToLevelLabel[w]->setText(Preferences::getInstance()->GetFormattedValue(value));
        }
        else
            e->setRate(value);
    }
}

void MultiUniverse::controlComboChanged(int value)
{

    QWidget *w = dynamic_cast<QWidget *>(sender());
    if(m_widgetToFxEngine.contains(w))
    {
        sACNEffectEngine *e = m_widgetToFxEngine[w];

        if(value==0)
            e->setDateStyle(sACNEffectEngine::dsEU);
        else
            e->setDateStyle(sACNEffectEngine::dsUSA);
    }
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

