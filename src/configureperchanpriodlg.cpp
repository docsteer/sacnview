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

#include "configureperchanpriodlg.h"
#include "ui_configureperchanpriodlg.h"
#include "consts.h"

ConfigurePerChanPrioDlg::ConfigurePerChanPrioDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigurePerChanPrioDlg)
{
    ui->setupUi(this);
    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbSetAll->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(MAX_SACN_PRIORITY);
    ui->sbSetAll->setMaximum(MAX_SACN_PRIORITY);
    ui->sbPriority->setValue(DEFAULT_SACN_PRIORITY);
    ui->sbSetAll->setValue(DEFAULT_SACN_PRIORITY);
}

ConfigurePerChanPrioDlg::~ConfigurePerChanPrioDlg()
{
    delete ui;
}


void ConfigurePerChanPrioDlg::setData(uint1 *data)
{
    memcpy(m_data, data, MAX_DMX_ADDRESS);
    for(int i=0; i<MAX_DMX_ADDRESS; i++)
        ui->widget->setCellValue(i, QString::number(m_data[i]));
}

uint1 *ConfigurePerChanPrioDlg::data()
{
    for(int i=0; i<MAX_DMX_ADDRESS; i++)
        m_data[i] = ui->widget->cellValue(i).toInt();
    return m_data;
}


void ConfigurePerChanPrioDlg::on_sbPriority_valueChanged(int value)
{
    int currentCell = ui->widget->selectedCell();
    if(currentCell<0) return;
    ui->widget->setCellValue(currentCell, QString::number(value));
    ui->widget->update();
}

void ConfigurePerChanPrioDlg::on_btnSetAll_pressed()
{
    for(int i=0; i<512; i++)
        ui->widget->setCellValue(i, QString::number(ui->sbSetAll->value()));
    ui->widget->update();
}

void ConfigurePerChanPrioDlg::on_widget_selectedCellChanged(int cell)
{
    if(cell>-1)
    {
        ui->sbPriority->setEnabled(true);
        int iValue = ui->widget->cellValue(cell).toInt();
        ui->sbPriority->setValue(iValue);
        ui->lblAddress->setText(tr("Address %1, Priority = ").arg(cell+1));
    }
    else
    {
        ui->sbPriority->setEnabled(false);
        ui->lblAddress->setText("");
    }
}
