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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "preferences.h"
#include "consts.h"
#include <sstream>
#include <QMessageBox>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    // Populate lang
    m_translation = new TranslationDialog(Preferences::getInstance()->GetLocale(), ui->vlLanguage, this);

    // Network interfaces
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface interface, interfaces)
    {
        // If the interface is ok for use...
        if(Preferences::getInstance()->interfaceSuitable(&interface))
        {
            // List IPv4 Addresses
            QString ipString;
            foreach (QNetworkAddressEntry e, interface.addressEntries()) {
                if (e.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    if(!ipString.isEmpty())
                        ipString.append(",");
                    ipString.append(e.ip().toString());
                }
            }

            QRadioButton *radio  = new QRadioButton(ui->gbNetworkInterface);
            radio->setText(QString("%1 (%2)")
                           .arg(interface.humanReadableName())
                           .arg(ipString));

            radio->setChecked(Preferences::getInstance()->networkInterface().hardwareAddress() == interface.hardwareAddress());

            ui->verticalLayout_NetworkInterfaces->addWidget(radio);
            m_interfaceList << interface;
            m_interfaceButtons << radio;
        }
    }
    ui->cbListenAll->setChecked(Preferences::getInstance()->GetNetworkListenAll());

    switch (Preferences::getInstance()->GetDisplayFormat())
    {
        case Preferences::DECIMAL:       ui->DecimalDisplayFormat->setChecked (true); break;
        case Preferences::PERCENT:       ui->PercentDisplayFormat->setChecked(true); break;
        case Preferences::HEXADECIMAL:   ui->HexDisplayFormat->setChecked(true); break;
    }

    ui->cbDisplayBlind->setChecked(Preferences::getInstance()->GetBlindVisualizer());
    ui->cbDisplayDDOnlys->setChecked(Preferences::getInstance()->GetDisplayDDOnly());
    ui->cbRestoreWindows->setChecked(Preferences::getInstance()->GetSaveWindowLayout());

    ui->leDefaultSourceName->setText(Preferences::getInstance()->GetDefaultTransmitName());

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

    ui->cbTxRateOverride->setChecked(Preferences::getInstance()->GetTXRateOverride());

    ui->cbTheme->clear();
    ui->cbTheme->addItems(Preferences::ThemeDescriptions);
    ui->cbTheme->setCurrentIndex((int)Preferences::getInstance()->GetTheme());
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}


void PreferencesDialog::on_buttonBox_accepted()
{
    bool requiresRestart = false;

    Preferences *p = Preferences::getInstance();

    // Display Format
    int displayFormat=0;
    if(ui->DecimalDisplayFormat->isChecked())
        displayFormat = Preferences::DECIMAL;
    if(ui->HexDisplayFormat->isChecked())
        displayFormat = Preferences::HEXADECIMAL;
    if(ui->PercentDisplayFormat->isChecked())
        displayFormat = Preferences::PERCENT;
    p->SetDisplayFormat(displayFormat);

    // Display Blind
    p->SetBlindVisualizer(ui->cbDisplayBlind->isChecked());

    // Display DD
    if (ui->cbDisplayDDOnlys->isChecked() != p->GetDisplayDDOnly() ) {requiresRestart = true;}
    p->SetDisplayDDOnly(ui->cbDisplayDDOnlys->isChecked());

    // Save layout
    p->SetSaveWindowLayout(ui->cbRestoreWindows->isChecked());

    // Transmit timeout
    int seconds = ui->NumOfHoursOfSacn->value()*60*60 + ui->NumOfMinOfSacn->value()*60 + ui->NumOfSecOfSacn->value();
    if(!ui->gbTransmitTimeout->isChecked())
        seconds = 0;
    p->SetNumSecondsOfSacn(seconds);

    // Default source name
    p->SetDefaultTransmitName(ui->leDefaultSourceName->text());

    if (ui->cbTxRateOverride->isChecked() != p->GetTXRateOverride())
        requiresRestart = true;
    p->SetTXRateOverride(ui->cbTxRateOverride->isChecked());

    // Interfaces
    for(int i=0; i<m_interfaceButtons.count(); i++)
    {
        if(m_interfaceButtons[i]->isChecked())
        {
            QNetworkInterface interface = m_interfaceList[i];
            if(interface.index() != p->networkInterface().index())
            {
                p->setNetworkInterface(m_interfaceList[i]);

                requiresRestart = true;

                break;
            }
        }
    }
    requiresRestart |= ui->cbListenAll->isChecked() != p->GetNetworkListenAll();
    p->SetNetworkListenAll(ui->cbListenAll->isChecked());

    // Theme
    Preferences::Theme theme = (Preferences::Theme)ui->cbTheme->currentIndex();
    if(p->GetTheme()!=theme)
    {
        p->SetTheme(theme);
        requiresRestart = true;
    }

    // Language
    if (m_translation->GetSelectedLocale() != p->GetLocale())
    {
        p->SetLocale(m_translation->GetSelectedLocale());
        requiresRestart = true;
    }

    // Resstart to apply?
    if (requiresRestart) {
        QMessageBox::information(this, tr("Restart requied"),
                                 tr("To apply these preferences, you will need to restart the application. \nsACNView will now close and restart"),
                                 QMessageBox::Ok);
        p->RESTART_APP = true;
        qApp->quit();
    }
}
