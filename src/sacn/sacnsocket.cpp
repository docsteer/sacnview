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

#include "sacnsocket.h"

#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QDebug>
#include "ipaddr.h"
#include "streamcommon.h"

sACNRxSocket::sACNRxSocket(QObject *parent) : QUdpSocket(parent)
{

}



void sACNRxSocket::bindMulticast(quint16 universe)
{
    QNetworkInterface iface = Preferences::getInstance()->networkInterface();

    CIPAddr addr;
    GetUniverseAddress(universe, addr);

    quint16 port = addr.GetIPPort();

    // Bind to first IPv4 address on selected NIC
    foreach (QNetworkAddressEntry ifaceAddr, iface.addressEntries())
    {
        if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol)
        {
            bool ok = bind(QHostAddress::AnyIPv4, /*ifaceAddr.ip(), */
                           port,
                           QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);

            if(ok)
                qDebug() << "sACNRxSocket : Bound to IP: " << ifaceAddr.ip().toString().toLatin1();
            else
                qDebug() << "sACNRxSocket : Failed to bind RX socket";

            break;
        }
    }

    joinMulticastGroup(QHostAddress(addr.GetV4Address()), iface);
    qDebug() << "sACNRxSocket : Joining Multicast Group: " << QHostAddress(addr.GetV4Address());
}

sACNTxSocket::sACNTxSocket(QObject *parent) : QUdpSocket(parent)
{

}

void sACNTxSocket::bindMulticast()
{
    QNetworkInterface iface = Preferences::getInstance()->networkInterface();
    QHostAddress a;
    QList<QNetworkAddressEntry> addressEntries = iface.addressEntries();
    for(int i=0; i<addressEntries.count(); i++)
    {
        if(addressEntries[i].ip().protocol() == QAbstractSocket::IPv4Protocol)
        {
            a = addressEntries[i].ip();
        }
    }
    bool ok = bind();

    if(ok)
        qDebug() << "sACNTxSocket : Bound to IP: " << a.toString().toLatin1();
    else
        qDebug() << "sACNTxSocket : Failed to bind TX socket";
    setSocketOption(QAbstractSocket::MulticastLoopbackOption, QVariant(1));
    setMulticastInterface(iface);
}
