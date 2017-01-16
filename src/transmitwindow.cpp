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

#include "transmitwindow.h"
#include "ui_transmitwindow.h"

#include "consts.h"
#include "sacn/ACNShare/deftypes.h"
#include "sacn/ACNShare/ipaddr.h"
#include "sacn/streamcommon.h"
#include "sacn/sacnsender.h"
#include "sacn/sacneffectengine.h"
#include "configureperchanpriodlg.h"
#include "preferences.h"
#include <QToolButton>
#include <QMessageBox>

transmitwindow::transmitwindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::transmitwindow), m_sender(0)
{
    ui->setupUi(this);

    m_fxEngine = NULL;

    memset(m_levels, 0, sizeof(m_levels));
    on_cbPriorityMode_currentIndexChanged(ui->cbPriorityMode->currentIndex());

    ui->sbUniverse->setMinimum(1);
    ui->sbUniverse->setMaximum(MAX_SACN_UNIVERSE);

    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(MAX_SACN_PRIORITY);
    ui->sbPriority->setValue(DEFAULT_SACN_PRIORITY);

    ui->sbFadeRangeEnd->setMinimum(MIN_DMX_ADDRESS);
    ui->sbFadeRangeEnd->setMaximum(MAX_DMX_ADDRESS);
    ui->sbFadeRangeStart->setMinimum(MIN_DMX_ADDRESS);
    ui->sbFadeRangeStart->setMaximum(MAX_DMX_ADDRESS);

    ui->leSourceName->setText(DEFAULT_SOURCE_NAME);

    ui->dlFadeRate->setMinimum(0);
    ui->dlFadeRate->setMaximum(FX_FADE_RATES.count()-1);
    ui->dlFadeRate->setValue(0);

    ui->tabWidget->setCurrentIndex(0);

    ui->slFadeLevel->setMinimum(MIN_SACN_LEVEL);
    ui->slFadeLevel->setMaximum(MAX_SACN_LEVEL);

    ui->btnFxStart->setEnabled(false);

    m_buttonBgColor = ui->btnCcBlink->palette().background().color();

    // Create preset buttons
    QHBoxLayout *layout = new QHBoxLayout();
    for(int i=0; i<PRESET_COUNT; i++)
    {
        QToolButton *button = new QToolButton(this);
        button->setText(QString::number(i+1));
        layout->addWidget(button);
    }
    QToolButton *recordButton = new QToolButton(this);
    recordButton->setIcon(QIcon(":/icons/record.png"));
    layout->addWidget(recordButton);
    ui->gbPresets->setLayout(layout);

    // Create faders
    QVBoxLayout *mainLayout = new QVBoxLayout();
    for(int row=0; row<2; row++)
    {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        for(int col=0; col<24; col++)
        {
            QVBoxLayout *faderVb = new QVBoxLayout();
            QSlider *slider = new QSlider(this);
            m_sliders << slider;
            slider->setMinimum(MIN_SACN_LEVEL);
            slider->setMaximum(MAX_SACN_LEVEL);
            connect(slider, SIGNAL(valueChanged(int)), this, SLOT(on_sliderMoved(int)));
            QLabel *label = new QLabel(this);
            m_sliderLabels << label;
            label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            label->setMinimumWidth(25);
            label->setText(QString("%1\r\n%2")
                           .arg(row*24 + col + 1)
                           .arg(0)
                           );
            QHBoxLayout *sliderLayout = new QHBoxLayout(); // This keeps the sliders horizontally centered
            sliderLayout->addWidget(slider);
            faderVb->addLayout(sliderLayout);
            faderVb->addWidget(label);
            rowLayout->addLayout(faderVb);
        }
        mainLayout->addLayout(rowLayout);
        rowLayout->update();
    }
    ui->gbFaders->setLayout(mainLayout);

    // Set up fader start spinbox
    ui->sbFadersStart->setMinimum(1);
    ui->sbFadersStart->setMaximum(MAX_DMX_ADDRESS - 48 + 1);


    mainLayout->update();
    ui->gbFaders->adjustSize();
    adjustSize();


    for(int i=0; i<MAX_DMX_ADDRESS; i++)
        m_perAddressPriorities[i] = DEFAULT_SACN_PRIORITY;

    // Set up slider for channel check
    ui->slChannelCheck->setMinimum(0);
    ui->slChannelCheck->setMaximum(MAX_SACN_LEVEL);
    ui->slChannelCheck->setValue(MAX_SACN_LEVEL);
    ui->lcdNumber->display(1);

    m_blink = false;
    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(BLINK_TIME);
    connect(m_blinkTimer, SIGNAL(timeout()), this, SLOT(doBlink()));

    QTimer::singleShot(30, Qt::CoarseTimer, this, SLOT(fixSize()));

    // Set up effect combo boxes
    connect(ui->rbFadeManual, SIGNAL(toggled(bool)), this, SLOT(on_radioFadeMode_toggled(bool)));
    connect(ui->rbFadeRamp, SIGNAL(toggled(bool)), this, SLOT(on_radioFadeMode_toggled(bool)));
    connect(ui->rbFadeSine, SIGNAL(toggled(bool)), this, SLOT(on_radioFadeMode_toggled(bool)));

    setUniverseOptsEnabled(true);
}

void transmitwindow::fixSize()
{
    // Update the window size
    resize(sizeHint());
}

transmitwindow::~transmitwindow()
{
    if(m_sender)
        delete m_sender;
    if(m_fxEngine)
        m_fxEngine->deleteLater();
    delete ui;
}

void transmitwindow::on_sbUniverse_valueChanged(int value)
{
    CIPAddr address;
    GetUniverseAddress(value, address);

    char string[CIPAddr::ADDRSTRINGBYTES];
    CIPAddr::AddrIntoString(address, string, false, false);

    ui->rbMulticast->setText(tr("Multicast to %1").arg(string));
}

void transmitwindow::on_sliderMoved(int value)
{
    int index = m_sliders.indexOf(qobject_cast<QSlider*>(sender()));
    if(index<0) return;

    int address = index + ui->sbFadersStart->value() - 1; // 0-based address

    QLabel *label = m_sliderLabels[index];

    label->setText(QString("%1\r\n%2")
    .arg(address+1)
    .arg(Preferences::getInstance()->GetFormattedValue(value))
    );

    m_levels[address] = value;

    if(m_sender)
        m_sender->setLevel(address, value);
}

void transmitwindow::on_sbFadersStart_valueChanged(int value)
{
    for(int i=0; i<m_sliders.count(); i++)
    {
        QSlider *slider = m_sliders[i];
        QLabel *label = m_sliderLabels[i];

        slider->blockSignals(true);
        slider->setValue(m_levels[value + i - 1]);
        slider->blockSignals(false);
        label->setText(QString("%1\r\n%2")
                   .arg(i + value)
                   .arg(m_levels[i + value - 1])
                   );
    }
}

void transmitwindow::setUniverseOptsEnabled(bool enabled)
{
    ui->leSourceName->setEnabled(enabled);
    ui->sbUniverse->setEnabled(enabled);
    ui->sbPriority->setEnabled(enabled);
    ui->btnEditPerChan->setEnabled(enabled);
    ui->cbPriorityMode->setEnabled(enabled);
    ui->gbProtocolMode->setEnabled(enabled);
    ui->gbProtocolVersion->setEnabled(enabled);
    ui->cbBlind->setEnabled(enabled);
    ui->tabWidget->setEnabled(!enabled);

    if(enabled)
        ui->btnStart->setText(tr("Start"));
    else
        ui->btnStart->setText(tr("Stop"));

}

void transmitwindow::on_btnStart_pressed()
{
    QHostAddress unicast;
    // Check settings
    if(ui->rbUnicast->isChecked())
    {
        unicast = QHostAddress(ui->leUnicastAddress->text());
        if(unicast.isNull())
        {
            QMessageBox::warning(this, tr("Invalid Unicast Address"),
                                 tr("Enter a valid unicast address"));
            return;
        }
    }
    if(!m_sender)
        m_sender = new sACNSentUniverse(ui->sbUniverse->value());

    m_sender->setUnicastAddress(unicast);

    if(m_sender->isSending())
    {
        m_sender->stopSending();
        if(m_fxEngine)
            m_fxEngine->deleteLater();
        m_fxEngine = NULL;
        setUniverseOptsEnabled(true);
        ui->tabWidget->setCurrentIndex(0);
    }
    else
    {
        m_sender->setName(ui->leSourceName->text());
        if(ui->cbPriorityMode->currentIndex() == pmPER_ADDRESS_PRIORITY)
        {
            m_sender->setPriorityMode(pmPER_ADDRESS_PRIORITY);
            m_sender->setPerChannelPriorities(m_perAddressPriorities);
        }

        m_sender->startSending();
        setUniverseOptsEnabled(false);
        for(unsigned int i=0; i<sizeof(m_levels); i++)
            m_sender->setLevel(i, m_levels[i]);
        m_fxEngine = new sACNEffectEngine();
        connect(m_fxEngine, SIGNAL(fxLevelChange(int)), ui->slFadeLevel, SLOT(setValue(int)));
        m_fxEngine->setSender(m_sender);
    }
}

void transmitwindow::on_slFadeLevel_valueChanged(int value)
{
    ui->lblFadeLevel->setText(Preferences::getInstance()->GetFormattedValue(value));

    if(m_fxEngine)
    {
        m_fxEngine->setManualLevel(value);
    }
}

void transmitwindow::on_btnFxPause_pressed()
{
    if(m_fxEngine)
    {
        m_fxEngine->pause();
        ui->btnFxStart->setEnabled(true);
        ui->btnFxPause->setEnabled(false);
    }
}

void transmitwindow::on_btnFxStart_pressed()
{
    if(m_fxEngine)
    {
        m_fxEngine->start();
        ui->btnFxStart->setEnabled(false);
        ui->btnFxPause->setEnabled(true);
    }
}


void transmitwindow::on_btnEditPerChan_pressed()
{
    ConfigurePerChanPrioDlg dlg;
    dlg.setData(m_perAddressPriorities);
    int result = dlg.exec();
    if(result==QDialog::Accepted)
    {
        memcpy(m_perAddressPriorities, dlg.data(), MAX_DMX_ADDRESS);
    }
}

void transmitwindow::on_cbPriorityMode_currentIndexChanged(int index)
{
    if(index==pmPER_ADDRESS_PRIORITY)
    {
        ui->sbPriority->setEnabled(false);
        ui->btnEditPerChan->setEnabled(true);
    }
    else
    {
        ui->sbPriority->setEnabled(true);
        ui->btnEditPerChan->setEnabled(false);
    }
}

void transmitwindow::on_btnCcNext_pressed()
{
    int value = ui->lcdNumber->value();

    value++;
    if(value>MAX_DMX_ADDRESS) return;

    ui->lcdNumber->display(value);
    value--;
    if(m_sender)
    {
        m_sender->setLevel(value, ui->slChannelCheck->value());
        m_sender->setLevel(value-1, 0);
    }
}

void transmitwindow::on_btnCcPrev_pressed()
{
    int value = ui->lcdNumber->value();

    value--;
    if(value<1) return;
    value--;

    ui->lcdNumber->display(value+1);

    if(m_sender)
    {
        m_sender->setLevel(value, ui->slChannelCheck->value());
        m_sender->setLevel(value+1, 0);
    }
}


void transmitwindow::on_slChannelCheck_valueChanged(int value)
{
    int address = ui->lcdNumber->value();

    if(m_sender)
    {
        m_sender->setLevel(address-1, value);
    }
}

void transmitwindow::on_btnCcBlink_pressed()
{
    if(m_blinkTimer->isActive())
    {
        m_blinkTimer->stop();

        QPalette buttonPal = ui->btnCcBlink->palette();
        buttonPal.setColor(QPalette::Button, m_buttonBgColor );
        ui->btnCcBlink->setPalette(buttonPal);
    }
    else
    {
        m_blinkTimer->start();
    }
}

void transmitwindow::doBlink()
{
    int address = ui->lcdNumber->value();
    m_blink = !m_blink;

    QPalette buttonPal = ui->btnCcBlink->palette();
    buttonPal.setColor(QPalette::Button, (m_blink) ? QColor(Qt::red) : m_buttonBgColor );
    ui->btnCcBlink->setPalette(buttonPal);
    if(m_blink)
    {
        if(m_sender)
            m_sender->setLevel(address-1, ui->slChannelCheck->value());
    }
    else
    {
        if(m_sender)
                m_sender->setLevel(address-1, 0);
    }
}

void transmitwindow::on_tabWidget_currentChanged(int index)
{
    // Don't do anything if we aren't actively sending
    if(!m_sender)
        return;

    if(index==tabChannelCheck)
    {
        int value = ui->lcdNumber->value() - 1;
        m_sender->setLevel(0, MAX_DMX_ADDRESS-1, 0);
        m_sender->setLevel(value, ui->slChannelCheck->value());

        QMetaObject::invokeMethod(
                    m_fxEngine,"pause");
    }

    if(index==tabSliders)
    {
        m_sender->setLevel(0, MAX_DMX_ADDRESS-1, 0);
        QMetaObject::invokeMethod(
                    m_fxEngine,"pause");
    }

    if(index==tabFadeRange)
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxRamp));
        QMetaObject::invokeMethod(
                    m_fxEngine,"start");
    }
    if(index==tabText)
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxText));
        QMetaObject::invokeMethod(
                    m_fxEngine,"start");
    }
}


void transmitwindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_PageDown:
        if(ui->tabWidget->currentIndex()==tabChannelCheck)
            on_btnCcPrev_pressed();
        event->accept();
        break;
    case Qt::Key_PageUp:
        if(ui->tabWidget->currentIndex()==tabChannelCheck)
            on_btnCcNext_pressed();
        event->accept();
        break;
    }
    QWidget::keyPressEvent(event);
}

void transmitwindow::on_dlFadeRate_valueChanged(int value)
{
    qreal rate = FX_FADE_RATES[value];
    ui->lblFadeSpeed->setText(tr("Fade Rate %1 Hz").arg(rate));

    if(m_fxEngine)
        m_fxEngine->setRate(rate);
}

void transmitwindow::on_sbFadeRangeStart_valueChanged(int value)
{

    if(m_fxEngine)
        m_fxEngine->setStartAddress(value-1);
}

void transmitwindow::on_sbFadeRangeEnd_valueChanged(int value)
{

    if(m_fxEngine)
        m_fxEngine->setEndAddress(value-1);
}


void transmitwindow::on_sbFadeRangeStart_editingFinished()
{
    if(ui->sbFadeRangeStart->value() > ui->sbFadeRangeEnd->value())
    {
        ui->sbFadeRangeEnd->setValue(ui->sbFadeRangeStart->value());
    }
}

void transmitwindow::on_sbFadeRangeEnd_editingFinished()
{
    if(ui->sbFadeRangeEnd->value() < ui->sbFadeRangeStart->value())
    {
        ui->sbFadeRangeStart->setValue(ui->sbFadeRangeEnd->value());
    }
}

void transmitwindow::on_radioFadeMode_toggled(bool checked)
{
    Q_UNUSED(checked);

    if(ui->rbFadeManual->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxManual));
    }
    if(ui->rbFadeRamp->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxRamp));
    }
    if(ui->rbFadeSine->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxSinewave));
    }
}

void transmitwindow::on_leScrollText_textChanged(const QString & text)
{
    m_fxEngine->setText(text);
}
