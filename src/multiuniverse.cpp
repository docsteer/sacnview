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
#include "addmultidialog.h"

MultiUniverse::MultiUniverse(int firstUniverse, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MultiUniverse),
    m_firstUniverse(firstUniverse)
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


void MultiUniverse::addSource(int universe, int min_address, int max_address,
                              sACNEffectEngine::FxMode mode, QString name, bool startSending,
                              int level, int rate, int priority)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row+1);

    m_senders.append(new sACNSentUniverse(universe));
    m_senders.last()->setPerSourcePriority(priority);
    connect(m_senders.last(), SIGNAL(sendingTimeout()), this, SLOT(senderTimedout()));
    m_fxEngines.append(new sACNEffectEngine());
    m_fxEngines.last()->setSender(m_senders.last());
    m_fxEngines.last()->setEndAddress(max_address-1);
    m_fxEngines.last()->setStartAddress(min_address-1);
    m_fxEngines.last()->setMode(mode);
    m_fxEngines.last()->setRate(rate);
    m_fxEngines.last()->setManualLevel(level);
    m_fxEngines.last()->start();
    m_senders.last()->setName(name);
    if(startSending)
        m_senders.last()->startSending();

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
    sb->setWrapping(true);
    ui->tableWidget->setCellWidget(row, COL_UNIVERSE, sb);
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(universeChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    sb = new QSpinBox(this);
    sb->setMinimum(MIN_DMX_ADDRESS);
    sb->setMaximum(MAX_DMX_ADDRESS);
    sb->setValue(min_address);
    sb->setWrapping(true);
    ui->tableWidget->setCellWidget(row, COL_STARTADDR, sb);
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(startChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    sb = new QSpinBox(this);
    sb->setMinimum(MIN_DMX_ADDRESS);
    sb->setMaximum(MAX_DMX_ADDRESS);
    sb->setValue(max_address);
    sb->setWrapping(true);
    ui->tableWidget->setCellWidget(row, COL_ENDADDR, sb);
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(endChanged(int)));
    connect(sb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));


    sb = new QSpinBox(this);
    sb->setMinimum(MIN_SACN_PRIORITY);
    sb->setMaximum(MAX_SACN_PRIORITY);
    sb->setValue(priority);
    sb->setValue(m_senders.last()->perSourcePriority());
    sb->setWrapping(true);
    ui->tableWidget->setCellWidget(row, COL_PRIORITY, sb);
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(priorityChanged(int)));
    m_widgetToFxEngine[sb] = m_fxEngines.last();
    m_widgetToSender[sb] = m_senders.last();

    QComboBox *cb = new QComboBox(this);
    cb->addItems(FX_MODE_DESCRIPTIONS);
    cb->setCurrentIndex((int) mode);
    ui->tableWidget->setCellWidget(row, COL_EFFECT, cb);
    m_widgetToFxEngine[cb] = m_fxEngines.last();
    m_widgetToSender[cb] = m_senders.last();

    connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(fxChanged(int)));
    connect(cb, SIGNAL(destroyed(QObject*)), this, SLOT(removeWidgetFromIndex(QObject*)));

    if(mode==sACNEffectEngine::FxManual)
        setupControl(row, mode, level);
    else
        setupControl(row, mode, rate);

    QTableWidgetItem *item = new QTableWidgetItem(m_senders.last()->name());
    ui->tableWidget->setItem(row, COL_NAME, item);
}

void MultiUniverse::on_btnAddRow_pressed()
{
    int nextUniverse = m_firstUniverse;
    if(m_senders.count()>0)
        nextUniverse = m_senders.last()->universe() + 1;

    QString name = Preferences::getInstance()->GetDefaultTransmitName();
    QString postfix = tr("_%1").arg(nextUniverse);
    name = name + postfix;
    name.truncate(MAX_SOURCE_NAME_LEN - postfix.length());

    addSource(nextUniverse, MIN_DMX_ADDRESS, MAX_DMX_ADDRESS, sACNEffectEngine::FxManual, name, false);
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

        setupControl(index, (sACNEffectEngine::FxMode)value, 0);
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

void MultiUniverse::setupControl(int row, sACNEffectEngine::FxMode mode, int value)
{
    switch(mode)
    {
    case sACNEffectEngine::FxManual:
        {
        QWidget *controlWidget = new QWidget(this);
        QLabel *controlLabel = new QLabel(Preferences::getInstance()->GetFormattedValue(value), controlWidget);
        QSlider *slider = new QSlider(Qt::Horizontal, controlWidget);
        slider->setMinimum(MIN_SACN_LEVEL);
        slider->setMaximum(MAX_SACN_LEVEL);
        slider->setValue(value);
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
    case sACNEffectEngine::FxChaseSnap:
        {
            QWidget *w = dynamic_cast<QWidget *>(sender());
            if(m_widgetToFxEngine.contains(w))
            {
                sACNEffectEngine *e = m_widgetToFxEngine.value(w);
                e->setManualLevel(255);
            }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    Q_FALLTHROUGH();
#endif
        }
    case sACNEffectEngine::FxChaseRamp:
    case sACNEffectEngine::FxChaseSine:
    case sACNEffectEngine::FxRamp:
    case sACNEffectEngine::FxSinewave:
    case sACNEffectEngine::FxVerticalBar:
    case sACNEffectEngine::FxHorizontalBar:
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

void MultiUniverse::senderTimedout()
{
    sACNSentUniverse *timedOutSender = dynamic_cast<sACNSentUniverse *>(sender());
    if(!timedOutSender) return;

    int index = m_senders.indexOf(timedOutSender);

    QCheckBox *cb = dynamic_cast<QCheckBox *>(ui->tableWidget->cellWidget(index, 0));

    cb->setChecked(false);
}

void MultiUniverse::on_btnAddMulti_clicked()
{
    AddMultiDialog d(this);
    int result = d.exec();

    if(result!=QDialog::Accepted) return;

    for(int i=0; i<d.universeCount(); i++)
    {
        QString name = Preferences::getInstance()->GetDefaultTransmitName();
        QString postfix = tr("_%1").arg(d.startUniverse() + i);
        name = name + postfix;
        name.truncate(MAX_SOURCE_NAME_LEN - postfix.length());
        addSource(d.startUniverse() + i, d.startAddress(), d.endAddress(),
                  d.mode(), name, d.startNow(), d.level(), d.rate(), d.priority());
    }
}

