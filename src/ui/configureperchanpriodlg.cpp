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

#include "configureperchanpriodlg.h"
#include "ui_configureperchanpriodlg.h"
#include "consts.h"
#include "preferences.h"

ConfigurePerChanPrioDlg::ConfigurePerChanPrioDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigurePerChanPrioDlg)
{
    ui->setupUi(this);
    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbSetAll->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(Preferences::GetTxMaxUiPriority());
    ui->sbSetAll->setMaximum(Preferences::GetTxMaxUiPriority());
    ui->sbPriority->setValue(DEFAULT_SACN_PRIORITY);
    ui->sbSetAll->setValue(DEFAULT_SACN_PRIORITY);
    ui->sbPriority->setEnabled(false);
    ui->sbSetAll->setEnabled(true);
    ui->sbPriority->setWrapping(true);
    ui->sbSetAll->setWrapping(true);

    for(int i=0; i<PRIORITYPRESET_COUNT; i++)
    {
        QToolButton *presetButton = new QToolButton(ui->presetWidget);
        presetButton->setText(QString::number(i+1));
        ui->presetWidget->layout()->addWidget(presetButton);
        connect(presetButton, &QToolButton::pressed, this, &ConfigurePerChanPrioDlg::presetButtonPressed);
        m_presetButtons << presetButton;
    }
    ui->widget->setMinimum(MIN_SACN_PRIORITY);
    ui->widget->setMaximum(Preferences::GetTxMaxUiPriority());
    ui->widget->setAllValues(100);
}

ConfigurePerChanPrioDlg::~ConfigurePerChanPrioDlg()
{
    delete ui;
}

void SetCell(GridEditWidget* widget, int cell, uint8_t value)
{
  widget->setCellValue(cell, QString::number(value));
  widget->setCellColor(cell, value > MAX_SACN_PRIORITY ?
    Preferences::Instance().colorForStatus(Preferences::Status::Bad) :
    QColor());

}

void ConfigurePerChanPrioDlg::setData(quint8 *data)
{
    memcpy(m_data, data, MAX_DMX_ADDRESS);
    for (int i = 0; i < MAX_DMX_ADDRESS; i++)
    {
      SetCell(ui->widget, i, m_data[i]);
    }
    ui->widget->update();
}

quint8 *ConfigurePerChanPrioDlg::data()
{
    for(int i=0; i<MAX_DMX_ADDRESS; i++)
        m_data[i] = ui->widget->cellValue(i).toInt();
    return m_data;
}

void ConfigurePerChanPrioDlg::on_btnSetAll_pressed()
{
    for(int i=0; i<512; i++)
        SetCell(ui->widget, i, ui->sbSetAll->value());
    ui->widget->update();
}

void ConfigurePerChanPrioDlg::on_widget_selectedCellsChanged(QList<int> cells)
{
    ui->sbPriority->setEnabled(cells.count()>0);
    ui->btnSet->setEnabled(cells.count()>0);
    m_selectedCells = cells;
}

void ConfigurePerChanPrioDlg::on_btnPresetRec_toggled(bool on)
{
    for(auto button: m_presetButtons)
    {
        if(on)
            button->setStyleSheet("background-color: red");
        else
            button->setStyleSheet("");
    }
}

void ConfigurePerChanPrioDlg::presetButtonPressed()
{
    QToolButton *button = dynamic_cast<QToolButton *>(sender());
    if(!button) return;
    const auto index = static_cast<int>(m_presetButtons.indexOf(button));

    if(ui->btnPresetRec->isChecked())
    {
        // Record the preset
        QByteArray preset;
        for(int i=0; i<MAX_DMX_ADDRESS; i++)
            preset.append(ui->widget->cellValue(i).toInt() & 0xFF);
        Preferences::Instance().SetPriorityPreset(preset, index);

        ui->btnPresetRec->setChecked(false);
    }
    else
    {
        // Playback preset
        QByteArray data = Preferences::Instance().GetPriorityPreset(index);
        setData(reinterpret_cast<quint8 *>(data.data()));
    }
}


void ConfigurePerChanPrioDlg::on_btnSet_pressed()
{
    for(int i : m_selectedCells)
        SetCell(ui->widget, i, ui->sbPriority->value());
    ui->widget->update();
}
