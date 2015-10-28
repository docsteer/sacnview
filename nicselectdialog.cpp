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

#include "nicselectdialog.h"
#include "ui_nicselectdialog.h"
#include <QNetworkInterface>

NICSelectDialog::NICSelectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NICSelectDialog)
{
    ui->setupUi(this);
    m_selectedInterface = QNetworkInterface();

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();


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
            ui->listWidget->addItem(QString("%1 (%2)")
                                    .arg(interface.humanReadableName())
                                    .arg(ipString));

            m_interfaceList << interface;
        }
    }

    ui->btnSelect->setEnabled(false);
}

NICSelectDialog::~NICSelectDialog()
{
    delete ui;
}

void NICSelectDialog::on_listWidget_itemSelectionChanged()
{
    ui->btnSelect->setEnabled(true);
}


void NICSelectDialog::on_btnSelect_pressed()
{
    m_selectedInterface = m_interfaceList[ui->listWidget->currentRow()];
    accept();
}

void NICSelectDialog::on_btnWorkOffline_pressed()
{
    reject();
}
