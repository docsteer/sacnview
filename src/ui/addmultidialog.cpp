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

#include "addmultidialog.h"
#include "consts.h"
#include "preferences.h"
#include "sacneffectengine.h"
#include "ui_addmultidialog.h"

AddMultiDialog::AddMultiDialog(QWidget * parent)
    : QDialog(parent), ui(new Ui::AddMultiDialog)
{
    ui->setupUi(this);

    ui->sbStartAddress->setMinimum(MIN_DMX_ADDRESS);
    ui->sbEndAddress->setMinimum(MIN_DMX_ADDRESS);
    ui->sbStartAddress->setMaximum(MAX_DMX_ADDRESS);
    ui->sbEndAddress->setMaximum(MAX_DMX_ADDRESS);
    ui->sbStartAddress->setValue(MIN_DMX_ADDRESS);
    ui->sbEndAddress->setValue(MAX_DMX_ADDRESS);

    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(Preferences::GetTxMaxUiPriority());
    ui->sbPriority->setValue(DEFAULT_SACN_PRIORITY);

    ui->sbNumUniverses->setMinimum(1);
    ui->sbNumUniverses->setMaximum(100);
    ui->sbStartUniverse->setMinimum(MIN_SACN_UNIVERSE);
    ui->sbStartUniverse->setMaximum(MAX_SACN_UNIVERSE);

    ui->cbEffect->addItems(sACNEffectEngine::FxModeDescriptions());

    connect(ui->sbStartUniverse, QOverload<int>::of(&QSpinBox::valueChanged), this, &AddMultiDialog::rangeChanged);
    connect(ui->sbNumUniverses, QOverload<int>::of(&QSpinBox::valueChanged), this, &AddMultiDialog::rangeChanged);

    ui->dlFadeRate->setMinimum(0);
    ui->dlFadeRate->setMaximum(static_cast<int>(FX_FADE_RATES.count() - 1));
    ui->dlFadeRate->setValue(0);

    rangeChanged(0);
    on_dlFadeRate_valueChanged(ui->dlFadeRate->value());
}

AddMultiDialog::~AddMultiDialog()
{
    delete ui;
}

void AddMultiDialog::rangeChanged(int)
{
    const int startUniverse = ui->sbStartUniverse->value();
    const int endRange = startUniverse + ui->sbNumUniverses->value() - 1;
    if (endRange > MAX_SACN_UNIVERSE)
    {
        ui->sbStartUniverse->setValue(startUniverse - (endRange - MAX_SACN_UNIVERSE));
        return;
    }
    ui->lbEndRange->setText(tr("(last universe will be %1)").arg(endRange));
}

void AddMultiDialog::on_cbEffect_currentIndexChanged(int index)
{
    sACNEffectEngine::FxMode mode = (sACNEffectEngine::FxMode)index;
    switch (mode)
    {
        case sACNEffectEngine::FxManual:
            ui->slLevel->setMinimum(MIN_SACN_LEVEL);
            ui->slLevel->setMaximum(MAX_SACN_LEVEL);
            ui->lbDialFunction->setText(tr("Level"));
            break;

        case sACNEffectEngine::FxChaseSnap:
        case sACNEffectEngine::FxChaseRamp:
        case sACNEffectEngine::FxChaseSine:
        case sACNEffectEngine::FxRamp:
        case sACNEffectEngine::FxInverseRamp:
        case sACNEffectEngine::FxSinewave:
        case sACNEffectEngine::FxVerticalBar:
        case sACNEffectEngine::FxHorizontalBar:
        case sACNEffectEngine::FxDate:
        case sACNEffectEngine::FxText:
        default:
            ui->slLevel->setMinimum(1);
            ui->slLevel->setMaximum(500);
            ui->lbDialFunction->setText(tr("Rate"));
            break;
    }

    on_slLevel_sliderMoved(ui->slLevel->value());
}

void AddMultiDialog::on_slLevel_sliderMoved(int value)
{
    sACNEffectEngine::FxMode mode = (sACNEffectEngine::FxMode)ui->cbEffect->currentIndex();
    switch (mode)
    {
        case sACNEffectEngine::FxManual:
            ui->lbDialValue->setText(Preferences::Instance().GetFormattedValue(value, true));
            break;

        case sACNEffectEngine::FxChaseSnap:
        case sACNEffectEngine::FxChaseRamp:
        case sACNEffectEngine::FxRamp:
        case sACNEffectEngine::FxInverseRamp:
        case sACNEffectEngine::FxSinewave:
        case sACNEffectEngine::FxVerticalBar:
        case sACNEffectEngine::FxHorizontalBar:
        case sACNEffectEngine::FxDate:
        case sACNEffectEngine::FxText:
        default: ui->lbDialValue->setText(tr("%1 Hz").arg(value)); break;
    }
}

int AddMultiDialog::startUniverse()
{
    return ui->sbStartUniverse->value();
}

int AddMultiDialog::universeCount()
{
    return ui->sbNumUniverses->value();
}

sACNEffectEngine::FxMode AddMultiDialog::mode()
{
    return (sACNEffectEngine::FxMode)ui->cbEffect->currentIndex();
}

int AddMultiDialog::startAddress()
{
    return qMin(ui->sbEndAddress->value(), ui->sbStartAddress->value());
}

int AddMultiDialog::endAddress()
{
    return qMax(ui->sbEndAddress->value(), ui->sbStartAddress->value());
}

bool AddMultiDialog::startNow()
{
    return ui->cbStartNow->isChecked();
}

int AddMultiDialog::level()
{
    if ((sACNEffectEngine::FxMode)ui->cbEffect->currentIndex() == sACNEffectEngine::FxChaseSnap) return 255;

    return ui->slLevel->value();
}

qreal AddMultiDialog::rate()
{
    qreal rate = FX_FADE_RATES[ui->dlFadeRate->value()];
    return rate;
}

int AddMultiDialog::priority()
{
    return ui->sbPriority->value();
}

void AddMultiDialog::on_slLevel_valueChanged(int value)
{
    on_slLevel_sliderMoved(value);
}

void AddMultiDialog::on_dlFadeRate_valueChanged(int /*value*/)
{
    qreal rate = FX_FADE_RATES[ui->dlFadeRate->value()];
    ui->lblRate->setText(tr("Fade Rate %1 Hz").arg(rate));
}
