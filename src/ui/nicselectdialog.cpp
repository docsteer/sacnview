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

#include "nicselectdialog.h"
#include "ui_nicselectdialog.h"
#include "preferences.h"
#include <QNetworkInterface>
#include <QMessageBox>

NICSelectDialog::NICSelectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NICSelectDialog)
{
    ui->setupUi(this);
    m_selectedInterface = QNetworkInterface();

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for(const QNetworkInterface &interface : interfaces)
    {
        // We want interfaces which are up, IPv4, and can multicast
        if(Preferences::getInstance()->interfaceSuitable(interface)) {
            QString ipString;
            foreach (QNetworkAddressEntry e, interface.addressEntries()) {
                if(e.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    if(!ipString.isEmpty())
                        ipString.append(",");
                    ipString.append(e.ip().toString());
                }
            }

            ui->listWidget->addItem(QString("%1 (%2)")
                                    .arg(interface.humanReadableName())
                                    .arg(ipString));

            m_interfaceList << interface;
        }
    }

    on_listWidget_itemSelectionChanged();
}

NICSelectDialog::~NICSelectDialog()
{
    delete ui;
}

void NICSelectDialog::on_listWidget_itemSelectionChanged()
{
    // Empty or no selection
    if (!ui->listWidget->count() || ui->listWidget->currentRow() == -1)
    {
        ui->btnSelect->setEnabled(false);
        return;
    }

    // Show select if not loopback
    qDebug() << ui->listWidget->currentRow();
    ui->btnSelect->setEnabled(
                !m_interfaceList[ui->listWidget->currentRow()].flags().testFlag(QNetworkInterface::IsLoopBack));
}


void NICSelectDialog::on_btnSelect_pressed()
{
    m_selectedInterface = m_interfaceList[ui->listWidget->currentRow()];
    accept();
}

void NICSelectDialog::on_btnWorkOffline_pressed()
{
    // Select localhost
    for (auto interface : m_interfaceList)
    {
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack))
            m_selectedInterface = interface;
        accept();
        return;
    }

    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("Work Offline failed\r\nUnable to locate localhost adapter");
    msgBox.exec();
    reject();
}
