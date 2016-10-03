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
#include "consts.h"
#include <sstream>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);


    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QVBoxLayout *nicLayout = new QVBoxLayout;

    foreach(QNetworkInterface interface, interfaces)
    {
        bool ok = false;
        // We want interfaces which are IPv4 and can multicast
        QString ipString;
        foreach (QNetworkAddressEntry e, interface.addressEntries()) {
            if(!ipString.isEmpty())
                ipString.append(",");
            ipString.append(e.ip().toString());
            if(e.ip().protocol() == QAbstractSocket::IPv4Protocol)
               ok = true;
        }
        ok = ok & (interface.flags() | QNetworkInterface::CanMulticast);

        if(ok)
        {
            QRadioButton *radio  = new QRadioButton(ui->gbNetworkInterface);
            radio->setText(QString("%1 (%2)")
                           .arg(interface.humanReadableName())
                           .arg(ipString));

            radio->setChecked(Preferences::getInstance()->networkInterface().hardwareAddress() == interface.hardwareAddress());

            nicLayout->addWidget(radio);
            m_interfaceList << interface;
        }
    }
    ui->gbNetworkInterface->setLayout(nicLayout);

    switch (Preferences::getInstance()->GetDisplayFormat())
    {
        case Preferences::DECIMAL:       ui->DecimalDisplayFormat->setChecked (true); break;
        case Preferences::PERCENT:       ui->PercentDisplayFormat->setChecked(true); break;
        case Preferences::HEXADECIMAL:   ui->HexDisplayFormat->setChecked(true); break;
    }

    ui->cbDisplayBlind->setChecked(Preferences::getInstance()->GetBlindVisualizer());

    int timeout = Preferences::getInstance()->GetNumSecondsOfSacn();
    if(timeout>0)
    {
        ui->gbTransmitTimeout->setChecked(true);
        int hour = (timeout/(60*60));
        int min = ((timeout/60)-(hour*60));
        int sec = (timeout - (hour*60*60) - (min*60) );
        ui->NumOfHoursOfSacn->setValue(hour);
        ui->NumOfMinOfSacn->setValue(min);
        ui->NumOfSecOfSacn->setValue(sec);
    }
    else
    {
        ui->gbTransmitTimeout->setChecked(false);
    }

}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}


void PreferencesDialog::on_buttonBox_accepted()
{
    Preferences *p = Preferences::getInstance();
    int displayFormat=0;
    if(ui->DecimalDisplayFormat->isChecked())
        displayFormat = Preferences::DECIMAL;
    if(ui->HexDisplayFormat->isChecked())
        displayFormat = Preferences::HEXADECIMAL;
    if(ui->PercentDisplayFormat->isChecked())
        displayFormat = Preferences::PERCENT;

    p->SetDisplayFormat(displayFormat);
    p->SetBlindVisualizer(ui->cbDisplayBlind->isChecked());

    int seconds = ui->NumOfHoursOfSacn->value()*60*60 + ui->NumOfMinOfSacn->value()*60 + ui->NumOfSecOfSacn->value();
    if(!ui->gbTransmitTimeout->isChecked())
        seconds = 0;
    p->SetNumSecondsOfSacn(seconds);
}
