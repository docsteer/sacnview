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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "preferences.h"
#include <sstream>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
    SetFieldsToCurrentState();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_DecimalDisplayFormat_toggled(bool checked)
{
    Preferences *p  = Preferences::getInstance();
    p->SetDisplayFormat(DECIMAL);
    return;
}

void PreferencesDialog::on_PercentDisplayFormat_toggled(bool checked)
{
    Preferences *p = Preferences::getInstance();
    p->SetDisplayFormat(PERCENT);
    return;
}

void PreferencesDialog::on_HexDisplayFormat_toggled(bool checked)
{
    Preferences *p = Preferences::getInstance();
    p->SetDisplayFormat(HEXADECIMAL);
    return;
}

void PreferencesDialog::on_checkBox_toggled(bool checked)
{
    Preferences *p = Preferences::getInstance();
    p->SetBlindVisualizer(checked);
    return;
}

void PreferencesDialog::on_NumOfSecOfSacn_valueChanged(int arg1)
{
    m_nSec = arg1;
    ConvertHourMinSecToSec();
    refreshTransmitTimeFields();
    return;
}

void PreferencesDialog::on_NumOfMinOfSacn_valueChanged(int arg1)
{
    m_nMin = arg1;
    ConvertHourMinSecToSec();
    refreshTransmitTimeFields();
}

void PreferencesDialog::on_NumOfHoursOfSacn_valueChanged(int arg1)
{
    m_nHour = arg1;
    ConvertHourMinSecToSec();
    refreshTransmitTimeFields();
}

void PreferencesDialog::ConvertHourMinSecToSec()
{
    Preferences *p = Preferences::getInstance();
    int nTotalNumOfSec = ((m_nHour*nNumOfSecPerHour) + (m_nMin*nNumberOfSecPerMin) + m_nSec);
    p->SetNumSecondsOfSacn(nTotalNumOfSec);
}

void PreferencesDialog::refreshTransmitTimeFields()
{
    Preferences *p = Preferences::getInstance();
    int nTotalNumOfSec = p->GetNumSecondsOfSacn();

    m_nHour = (nTotalNumOfSec/nNumOfSecPerHour);
    m_nMin = ((nTotalNumOfSec/nNumberOfSecPerMin)-(m_nHour*nNumOfMinPerHour));
    m_nSec = (nTotalNumOfSec - (m_nHour*nNumOfSecPerHour) - (m_nMin*nNumberOfSecPerMin) );

    ui->NumOfHoursOfSacn->setValue(m_nHour);
    ui->NumOfMinOfSacn->setValue(m_nMin);
    ui->NumOfSecOfSacn->setValue(m_nSec);
}

void PreferencesDialog::SetFieldsToCurrentState()
{
    Preferences *p = Preferences::getInstance();
    switch (p->GetDisplayFormat())
    {
        case DECIMAL:       ui->DecimalDisplayFormat->setChecked (true); break;
        case PERCENT:       ui->PercentDisplayFormat->setChecked(true); break;
        case HEXADECIMAL:   ui->HexDisplayFormat->setChecked(true); break;
    }

    if (p->GetBlindVisualizer())
        ui->checkBox->setChecked(true);
    else
        ui->checkBox->setChecked(false);

    refreshTransmitTimeFields();

}



