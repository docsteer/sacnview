// Copyright 2016 Tom Steer
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
#include "sacn/ACNShare/ipaddr.h"
#include "sacn/streamingacn.h"
#include "sacn/sacneffectengine.h"
#include "configureperchanpriodlg.h"
#include "preferences.h"
#include <QButtonGroup>
#include <QToolButton>
#include <QMessageBox>

const char* FADERLABELFORMAT = "<b>%1</b><br>%2";
const char* FADERADDRESSPROP = "ADDRESS";
const char* FADERLABELPROP = "LABEL*";

transmitwindow::transmitwindow(int universe, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::transmitwindow),
    m_sender(Q_NULLPTR),
    m_fxEngine(Q_NULLPTR),
    m_recordMode(false)
{
    ui->setupUi(this);

    updateTitle();

    m_levels.fill(0);
    on_cbPriorityMode_currentIndexChanged(ui->cbPriorityMode->currentIndex());

    ui->sbUniverse->setMinimum(1);
    ui->sbUniverse->setMaximum(MAX_SACN_UNIVERSE);
    ui->sbUniverse->setValue(universe);
    ui->sbUniverse->setWrapping(true);

    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(MAX_SACN_PRIORITY);
    ui->sbPriority->setValue(DEFAULT_SACN_PRIORITY);
    ui->sbPriority->setWrapping(true);

    ui->sbSlotCount->setMaximum(MAX_DMX_ADDRESS);
    ui->sbSlotCount->setMinimum(MIN_DMX_ADDRESS);
    m_slotCount = MAX_DMX_ADDRESS;
    ui->sbSlotCount->setValue(m_slotCount);
    ui->sbSlotCount->setWrapping(true);

    ui->sbFadeRangeEnd->setMinimum(MIN_DMX_ADDRESS);
    ui->sbFadeRangeEnd->setMaximum(m_slotCount);
    ui->sbFadeRangeEnd->setValue(m_slotCount);
    ui->sbFadeRangeEnd->setWrapping(true);
    ui->sbFadeRangeStart->setMinimum(MIN_DMX_ADDRESS);
    ui->sbFadeRangeStart->setMaximum(m_slotCount);
    ui->sbFadeRangeStart->setValue(MIN_DMX_ADDRESS);
    ui->sbFadeRangeStart->setWrapping(true);

    ui->leSourceName->setText(Preferences::getInstance()->GetDefaultTransmitName());

    ui->dlFadeRate->setMinimum(0);
    ui->dlFadeRate->setMaximum(FX_FADE_RATES.count()-1);
    ui->dlFadeRate->setValue(0);

    ui->tabWidget->setCurrentIndex(0);

    ui->slFadeLevel->setMinimum(MIN_SACN_LEVEL);
    ui->slFadeLevel->setMaximum(MAX_SACN_LEVEL);

    ui->btnFxStart->setEnabled(false);
    ui->btnFxPause->setEnabled(false);

    // Create preset buttons
    QHBoxLayout *layout = new QHBoxLayout();
    for(int i=0; i<PRESET_COUNT; i++)
    {
        QToolButton *button = new QToolButton(this);
        button->setText(QString::number(i+1));
        button->setMinimumSize(16, 16);
        button->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        layout->addWidget(button);
        m_presetButtons << button;
        connect(button, SIGNAL(pressed()), this, SLOT(presetButtonPressed()));
    }
    QToolButton *recordButton = new QToolButton(this);
    m_presetButtons << recordButton;
    recordButton->setCheckable(true);
    recordButton->setIcon(QIcon(":/icons/record.png"));
    recordButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    layout->addWidget(recordButton);
    ui->gbPresets->setLayout(layout);
    connect(recordButton, SIGNAL(toggled(bool)), this, SLOT(recordButtonPressed(bool)));

    // Create faders
    uint16_t sliderAddress = 0;
    QVBoxLayout *mainLayout = new QVBoxLayout();
    for(int row=0; row<2; row++)
    {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        for(int col=0; col<NUM_SLIDERS/2; col++)
        {
            QVBoxLayout *faderVb = new QVBoxLayout();
            QSlider *slider = new QSlider(this);
            m_sliders << slider;
            slider->setProperty(FADERADDRESSPROP, sliderAddress);
            slider->setMinimum(MIN_SACN_LEVEL);
            slider->setMaximum(MAX_SACN_LEVEL);
            connect(slider, SIGNAL(valueChanged(int)), this, SLOT(on_sliderMoved(int)));
            QLabel *label = new QLabel(this);
            slider->setProperty(FADERLABELPROP, QVariant::fromValue(label));
            label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            label->setMinimumWidth(25);
            label->setText(QString(FADERLABELFORMAT)
                .arg(sliderAddress + 1)
                .arg(Preferences::getInstance()->GetFormattedValue(m_levels.at(sliderAddress))));
            QHBoxLayout *sliderLayout = new QHBoxLayout(); // This keeps the sliders horizontally centered
            sliderLayout->addWidget(slider);
            faderVb->addLayout(sliderLayout);
            faderVb->addWidget(label);
            rowLayout->addLayout(faderVb);

            ++sliderAddress;
        }
        mainLayout->addLayout(rowLayout);
        rowLayout->update();
    }
    ui->gbFaders->setLayout(mainLayout);

    // Set up fader start spinbox
    ui->sbFadersStart->setMinimum(MIN_DMX_ADDRESS);
    ui->sbFadersStart->setMaximum(m_slotCount);
    ui->sbFadersStart->setWrapping(true);

    ui->gbFaders->adjustSize();
    ui->horizontalLayout_9->update();
    ui->tabFaders->adjustSize();
    ui->tabWidget->adjustSize();
    mainLayout->update();


    adjustSize();
    updateGeometry();

    m_perAddressPriorities.fill(DEFAULT_SACN_PRIORITY);

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

    // Set up effect radio buttons
    QButtonGroup *effectGroup = new QButtonGroup(this);
    connect(effectGroup, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(radioFadeMode_toggled(QAbstractButton*, bool)));
    effectGroup->addButton(ui->rbFadeManual);
    effectGroup->addButton(ui->rbFadeRamp);
    effectGroup->addButton(ui->rbFadeSine);
    effectGroup->addButton(ui->rbVerticalBars);
    effectGroup->addButton(ui->rbHorizBars);
    effectGroup->addButton(ui->rbText);
    effectGroup->addButton(ui->rbDateTime);
    effectGroup->addButton(ui->rbChase);

    QButtonGroup *effectDateGroup = new QButtonGroup(this);
    connect(effectDateGroup, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(dateMode_toggled(QAbstractButton*, bool)));
    effectDateGroup->addButton(ui->rbEuDate);
    effectDateGroup->addButton(ui->rbUsDate);

    QButtonGroup *effectChaseGroup = new QButtonGroup(this);
    connect(effectChaseGroup, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(radioFadeMode_toggled(QAbstractButton*, bool)));
    effectChaseGroup->addButton(ui->rbChaseRamp);
    effectChaseGroup->addButton(ui->rbChaseSine);
    effectChaseGroup->addButton(ui->rbChaseSnap);

    setUniverseOptsEnabled(true);

    // Set up keypad
    connect(ui->k0,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key0()));
    connect(ui->k1,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key1()));
    connect(ui->k2,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key2()));
    connect(ui->k3,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key3()));
    connect(ui->k4,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key4()));
    connect(ui->k5,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key5()));
    connect(ui->k6,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key6()));
    connect(ui->k7,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key7()));
    connect(ui->k8,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key8()));
    connect(ui->k9,     SIGNAL(pressed()),  ui->teCommandline,  SLOT(key9()));
    connect(ui->kAnd,   SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyAnd()));
    connect(ui->kMinus, SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyMinus()));
    connect(ui->kAt,    SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyAt()));
    connect(ui->kClear, SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyClear()));
    connect(ui->kFull,  SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyFull()));
    connect(ui->kThru,  SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyThru()));
    connect(ui->kOffset,SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyOffset()));
    connect(ui->kEnter, SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyEnter()));
    connect(ui->kAllOff,SIGNAL(pressed()),  ui->teCommandline,  SLOT(keyAllOff()));

    connect(ui->teCommandline, SIGNAL(setLevels(QSet<int>,int)), this, SLOT(setLevels(QSet<int>,int)));

    ui->gridControl->setMinimum(0);
    if(Preferences::getInstance()->GetDisplayFormat()==Preferences::PERCENT)
        ui->gridControl->setMaximum(100);
    else
        ui->gridControl->setMaximum(255);
    ui->gridControl->setAllValues(0);

    connect(ui->gridControl, SIGNAL(levelsSet(QList<QPair<int,int>>)), this, SLOT(setLevelList(QList<QPair<int,int>>)));

    if(!m_sender)
    {
        m_sender = sACNManager::getInstance()->getSender(ui->sbUniverse->value());
        connect(m_sender.data(), SIGNAL(sendingTimeout()), this, SLOT(sourceTimeout()));
    }
    if(!m_fxEngine)
    {
        m_fxEngine = new sACNEffectEngine(m_sender);
        connect(m_fxEngine, SIGNAL(fxLevelChange(int)), ui->slFadeLevel, SLOT(setValue(int)));
        connect(m_fxEngine, SIGNAL(textImageChanged(QPixmap)), ui->lblTextImage, SLOT(setPixmap(QPixmap)));
        connect(m_fxEngine, &sACNEffectEngine::running, this, [this]() { ui->btnFxStart->setEnabled(false); ui->btnFxPause->setEnabled(true); }, Qt::QueuedConnection);
        connect(m_fxEngine, &sACNEffectEngine::paused, this, [this]() { ui->btnFxStart->setEnabled(true); ui->btnFxPause->setEnabled(false); } , Qt::QueuedConnection);
        m_fxEngine->setRange(ui->sbFadeRangeStart->value()-1, ui->sbFadeRangeEnd->value()-1);
        ui->btnFxStart->setEnabled(true);
    }
}

void transmitwindow::fixSize()
{
    // Update the window size
    adjustSize();
    updateGeometry();
    //resize(sizeHint());
}

transmitwindow::~transmitwindow()
{
    if(m_sender)
        m_sender->deleteLater();
    if(m_fxEngine)
    {
        m_fxEngine->deleteLater();
    }
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
    auto slider = qobject_cast<QSlider*>(sender());
    if (!slider)
        return;
    int index = m_sliders.indexOf(qobject_cast<QSlider*>(sender()));
    if(index<0) return;

    bool ok;
    auto address = slider->property(FADERADDRESSPROP).toUInt(&ok);
    if (!ok && address >= m_levels.size())
        return;

    QLabel *label = slider->property(FADERLABELPROP).value<QLabel*>();
    if (label) {
        label->setText(QString(FADERLABELFORMAT)
            .arg(address+1)
            .arg(Preferences::getInstance()->GetFormattedValue(value)));
    }

    setLevel(address, value);
}

void transmitwindow::on_sbFadersStart_valueChanged(int address)
{
    for (const auto &slider : qAsConst(m_sliders))
    {
        slider->blockSignals(true);

        auto level = m_levels[address - 1];

        // Store address in slider for later reference
        slider->setProperty(FADERADDRESSPROP, address - 1);

        // Fader level
        slider->setValue(level);

        // Address/level text display
        QLabel *label = slider->property(FADERLABELPROP).value<QLabel*>();
        if (label) {
            label->setText(QString(FADERLABELFORMAT)
                .arg(address)
                .arg(Preferences::getInstance()->GetFormattedValue(level)));
        }

        if (static_cast<size_t>(address) < m_levels.size())
            ++address;
        else
            address = 1;

        slider->blockSignals(false);
    }
}

void transmitwindow::setUniverseOptsEnabled(bool enabled)
{
    ui->frSourceOpts->setEnabled(enabled);
    ui->cbBlind->setEnabled(enabled ? ui->rbRatified->isChecked() : false);

    if(enabled)
    {
        ui->btnStart->setText(tr("Start"));
        on_cbPriorityMode_currentIndexChanged(ui->cbPriorityMode->currentIndex());
    }
    else
    {
        ui->btnStart->setText(tr("Stop"));
    }

}

void transmitwindow::updateTitle()
{
    QString title = tr("Transmit");
    if (m_sender && m_sender->isSending())
        title.append(tr(" - Universe %1").arg(QString::number(m_sender->universe())));
    else
        title.append(tr(" - Not Active"));
    this->setWindowTitle(title);
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

    m_sender->setUnicastAddress(unicast);
    if(ui->rbRatified->isChecked())
        m_sender->setProtocolVersion(StreamingACNProtocolVersion::sACNProtocolRelease);
    else
        m_sender->setProtocolVersion(StreamingACNProtocolVersion::sACNProtocolDraft);


    if(m_sender->isSending())
    {
        m_sender->stopSending();
        on_btnFxPause_pressed();
        setUniverseOptsEnabled(true);
    }
    else
    {
        m_sender->setSlotCount(ui->sbSlotCount->value());
        m_sender->setName(ui->leSourceName->text());
        m_sender->setUniverse(ui->sbUniverse->value());
        if(ui->cbPriorityMode->currentIndex() == pmPER_ADDRESS_PRIORITY)
        {
            m_sender->setPriorityMode(pmPER_ADDRESS_PRIORITY);
            m_sender->setPerChannelPriorities(m_perAddressPriorities.data());
        }
        else
        {
            m_sender->setPriorityMode(pmPER_SOURCE_PRIORITY);
            m_sender->setPerSourcePriority(ui->sbPriority->value());
        }

        m_sender->startSending(ui->cbBlind->isChecked());

        setUniverseOptsEnabled(false);
        on_tabWidget_currentChanged(ui->tabWidget->currentIndex());

        m_sender->setLevel(m_levels.data(), static_cast<int>(std::min(m_levels.size(), static_cast<size_t>(m_slotCount) - 1)));
    }

    updateTitle();
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
        m_fxEngine->pause();
}

void transmitwindow::on_btnFxStart_pressed()
{
    if(m_fxEngine)
        m_fxEngine->run();
}


void transmitwindow::on_btnEditPerChan_pressed()
{
    ConfigurePerChanPrioDlg dlg;
    dlg.setWindowTitle(tr("Per address priority universe %1").arg(ui->sbUniverse->value()));
    dlg.setData(m_perAddressPriorities.data());
    int result = dlg.exec();
    if(result==QDialog::Accepted)
    {
        memcpy(m_perAddressPriorities.data(), dlg.data(), m_slotCount);
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

    if(++value>m_slotCount)
        value = MIN_DMX_ADDRESS;

    ui->lcdNumber->display(value);

    if(m_sender)
    {
        m_sender->setLevelRange(MIN_DMX_ADDRESS - 1, m_slotCount - 1, 0);
        m_sender->setLevel(value - 1, ui->slChannelCheck->value());
    }
}

void transmitwindow::on_btnCcPrev_pressed()
{
    int value = ui->lcdNumber->value();

    if(--value<MIN_DMX_ADDRESS)
        value = m_slotCount;

    ui->lcdNumber->display(value);

    if(m_sender)
    {
        m_sender->setLevelRange(MIN_DMX_ADDRESS - 1, m_slotCount - 1, 0);
        m_sender->setLevel(value - 1, ui->slChannelCheck->value());
    }
}

void transmitwindow::on_lcdNumber_valueChanged(int value)
{
    if(m_sender)
    {
        m_sender->setLevelRange(0, m_slotCount - 1, 0);
        m_sender->setLevel(value-1, ui->slChannelCheck->value());
    }
}

void transmitwindow::on_lcdNumber_toggleOff()
{
    if(ui->slChannelCheck->value()==0)
    {
        ui->slChannelCheck->setValue(ui->slChannelCheck->maximum());
    }
    else
    {
        ui->slChannelCheck->setValue(0);
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
        int address = ui->lcdNumber->value();
        ui->blinkIndicator->setPixmap(QPixmap());
        if(m_sender)
                m_sender->setLevel(address-1, ui->slChannelCheck->value());
    }
    else
    {
        m_blinkTimer->start();

    }

    QFont font = ui->btnCcBlink->font();
    font.setBold(m_blinkTimer->isActive());
    ui->btnCcBlink->setFont(font);
}

void transmitwindow::doBlink()
{
    int address = ui->lcdNumber->value();
    m_blink = !m_blink;

    if(m_blink)
    {
        ui->blinkIndicator->setPixmap(QPixmap(":/icons/record.png"));
        if(m_sender)
            m_sender->setLevel(address-1, ui->slChannelCheck->value());
    }
    else
    {
        ui->blinkIndicator->setPixmap(QPixmap());
        if(m_sender)
                m_sender->setLevel(address-1, 0);
    }
}

void transmitwindow::on_tabWidget_currentChanged(int index)
{
    // Don't do anything if we aren't actively sending
    if(!m_sender)
        return;

    // Stop FX, and clear output
    QMetaObject::invokeMethod(
                m_fxEngine,"pause");
    m_sender->setLevelRange(0, m_slotCount-1, 0);

    switch (index)
    {
        case tabChannelCheck:
        {
            auto address = ui->lcdNumber->value() - 1;
            m_sender->setLevel(address, ui->slChannelCheck->value());

            ui->lcdNumber->setFocus();
            break;
        }

        case tabSliders:
        {
            // Reassert fader and programmer levels
            on_sbFadersStart_valueChanged(ui->sbFadersStart->value());
            m_sender->setLevel(m_levels.data(), static_cast<int>(m_levels.size()));
            ui->teCommandline->setFocus();
            break;
        }

        case tabEffects:
        {
            QMetaObject::invokeMethod(
                        m_fxEngine,"run");
            break;
        }

        case tabGrid:
        {
            // Reassert levels
            m_sender->setLevel(m_levels.data(), static_cast<int>(m_levels.size()));
            break;
    }
}
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
    if(value>ui->sbFadeRangeEnd->value())
    {
        ui->sbFadeRangeEnd->setValue(value);
    }

    if(m_fxEngine)
    {
        m_fxEngine->setRange(ui->sbFadeRangeStart->value()-1, ui->sbFadeRangeEnd->value()-1);
    }
}

void transmitwindow::on_sbFadeRangeEnd_valueChanged(int value)
{
    if(value<ui->sbFadeRangeStart->value())
    {
        ui->sbFadeRangeStart->setValue(value);
    }

    if(m_fxEngine)
    {
        m_fxEngine->setRange(ui->sbFadeRangeStart->value()-1, ui->sbFadeRangeEnd->value()-1);
    }
}

void transmitwindow::radioFadeMode_toggled(QAbstractButton* id, bool checked)
{
    Q_UNUSED(id);
    Q_UNUSED(checked)

    if(ui->rbFadeManual->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxManual));
        ui->swFx->setCurrentIndex(0);
    }
    if(ui->rbFadeRamp->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxRamp));
        ui->swFx->setCurrentIndex(0);
    }
    if(ui->rbFadeSine->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxSinewave));
        ui->swFx->setCurrentIndex(0);
    }
    if(ui->rbVerticalBars->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxVerticalBar));
        ui->swFx->setCurrentIndex(0);
    }
    if(ui->rbHorizBars->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxHorizontalBar));
        ui->swFx->setCurrentIndex(0);
    }
    if(ui->rbText->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxText));
        QMetaObject::invokeMethod(
                    m_fxEngine,"setText", Q_ARG(QString, ui->leScrollText->text()));
        ui->swFx->setCurrentIndex(1);
    }
    if(ui->rbDateTime->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, sACNEffectEngine::FxDate));
        ui->swFx->setCurrentIndex(2);
    }
    ui->fChaseOptions->setEnabled(ui->rbChase->isChecked());
    if(ui->rbChase->isChecked())
    {
        sACNEffectEngine::FxMode mode;
        if (ui->rbChaseRamp->isChecked())
            mode = sACNEffectEngine::FxChaseRamp;
        else if (ui->rbChaseSnap->isChecked())
             mode = sACNEffectEngine::FxChaseSnap;
        else if (ui->rbChaseSine->isChecked())
             mode = sACNEffectEngine::FxChaseSine;
        QMetaObject::invokeMethod(
                    m_fxEngine,"setMode", Q_ARG(sACNEffectEngine::FxMode, mode));
        ui->swFx->setCurrentIndex(0);
    }
}

void transmitwindow::on_leScrollText_textChanged(const QString & text)
{
    if(m_fxEngine)
        m_fxEngine->setText(text);
}


void transmitwindow::presetButtonPressed()
{
    QToolButton *btn = dynamic_cast<QToolButton *>(sender());
    if(!btn) return;

    int index = m_presetButtons.indexOf(btn);

    if(m_recordMode)
    {
        // Record a preset
        Preferences::getInstance()->SetPreset(QByteArray(reinterpret_cast<const char*>(m_levels.data()), static_cast<int>(m_levels.size())), index);
        m_recordMode = false;

        foreach(QToolButton *btn, m_presetButtons)
        {
            btn->setStyleSheet(QString(""));
            btn->setChecked(false);
        }
    }
    else
    {
        // Play back a preset
        QByteArray baPreset = Preferences::getInstance()->GetPreset(index);
        uint16_t address = 0;
        for (uint8_t level : qAsConst(baPreset))
            setLevel(address++, level);
    }
}

void transmitwindow::recordButtonPressed(bool on)
{
    m_recordMode = on;
    if(m_recordMode)
    {
        foreach(QToolButton *btn, m_presetButtons)
        {
            if (!btn->isChecked()) btn->setStyleSheet(QString("background-color: red;"));
        }
    }
    else
    {
        foreach(QToolButton *btn, m_presetButtons)
        {
            btn->setStyleSheet(QString(""));
        }
    }
}

void transmitwindow::setLevels(QSet<int> addresses, int level)
{
    foreach(int addr, addresses)
    {
        addr -= 1; // Convert to 0-based
        setLevel(addr, level);
    }
}

void transmitwindow::dateMode_toggled(QAbstractButton* id, bool checked)
{
    Q_UNUSED(id);
    Q_UNUSED(checked);

    if(ui->rbEuDate->isChecked())
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setDateStyle", Q_ARG(sACNEffectEngine::DateStyle, sACNEffectEngine::dsEU));
    }
    else
    {
        QMetaObject::invokeMethod(
                    m_fxEngine,"setDateStyle", Q_ARG(sACNEffectEngine::DateStyle, sACNEffectEngine::dsUSA));
    }
}

void transmitwindow::on_rbDraft_clicked()
{
    ui->cbBlind->setEnabled(false);
    ui->cbBlind->setChecked(false);
}

void transmitwindow::on_rbRatified_clicked()
{
    ui->cbBlind->setEnabled(true);
}

void transmitwindow::sourceTimeout()
{
    setUniverseOptsEnabled(true);
}

void transmitwindow::on_sbSlotCount_valueChanged(int arg1)
{
    Q_ASSERT(arg1 >= MIN_DMX_ADDRESS);
    Q_ASSERT(arg1 <= MAX_DMX_ADDRESS);
    m_slotCount = arg1;

    ui->sbFadeRangeStart->setValue(std::min(static_cast<decltype(m_slotCount)>(ui->sbFadeRangeStart->value()), m_slotCount));
    ui->sbFadeRangeStart->setMaximum(m_slotCount);

    ui->sbFadeRangeEnd->setValue(std::min(static_cast<decltype(m_slotCount)>(ui->sbFadeRangeEnd->value()), m_slotCount));
    ui->sbFadeRangeEnd->setMaximum(m_slotCount);

    ui->sbFadersStart->setMinimum(MIN_DMX_ADDRESS);
    ui->sbFadersStart->setMaximum(m_slotCount);
    on_sbFadersStart_valueChanged(ui->sbFadersStart->value());

    ui->lcdNumber->display(std::min(static_cast<decltype(m_slotCount)>(ui->lcdNumber->value()), m_slotCount));
}

void transmitwindow::setLevel(int address, int value)
{
    m_levels[address] = value;

    ui->gridControl->setCellValue(address, Preferences::getInstance()->GetFormattedValue(value));

    if(m_sender)
        m_sender->setLevel(address, value);

    for (const auto &slider : qAsConst(m_sliders))
    {
        bool ok;
        auto sliderAddr = slider->property(FADERADDRESSPROP).toUInt(&ok);
        if (ok && sliderAddr == static_cast<unsigned int>(address)) {
            slider->setValue(value);
            break;
        }
    }
}

void transmitwindow::setLevelList(QList<QPair<int, int>> levelList)
{
    // Levels are in localized format, need to convert
    for(auto i: levelList)
    {
        if(Preferences::getInstance()->GetDisplayFormat()==Preferences::PERCENT)
            setLevel(i.first, PTOHT[i.second]);
        else
            setLevel(i.first, i.second);
    }
}

