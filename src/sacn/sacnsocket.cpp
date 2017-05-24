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

bool sACNRxSocket::bindMulticast(quint16 universe)
{
    bool ok = false;

    QNetworkInterface iface = Preferences::getInstance()->networkInterface();

    CIPAddr addr;
    GetUniverseAddress(universe, addr);

    // Bind to mutlicast address
    ok = bind(addr.ToQHostAddress(),
                   addr.GetIPPort(),
                   QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);

    // Join multicast on selected NIC
    if (ok)
    {
        #if (QT_VERSION <= QT_VERSION_CHECK(5, 8, 0)) && Q_OS_MACOS
        #error setMulticastInterface() fails to bind to correct interface on systems running IPV4 and IPv6 with QT <= 5.8.0
        #endif
        setMulticastInterface(iface);
        ok |= joinMulticastGroup(QHostAddress(addr.GetV4Address()), iface);
    }

    if(ok)
    {
        qDebug() << "sACNRxSocket : Bound to interface:" << iface.name();
        qDebug() << "sACNRxSocket : Joining Multicast Group:" << QHostAddress(addr.GetV4Address()).toString();
    }
    else
    {
        close();
        qDebug() << "sACNRxSocket : Failed to bind RX socket";
    }

    return ok;
}

bool sACNRxSocket::bindUnicast()
{
    bool ok = false;

    // Bind to first IPv4 address on selected NIC
    QNetworkInterface iface = Preferences::getInstance()->networkInterface();

    foreach (QNetworkAddressEntry ifaceAddr, iface.addressEntries())
    {
        if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol)
        {
            ok = bind(ifaceAddr.ip(),
                      STREAM_IP_PORT,
                      QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
            qDebug() << "sACNRxSocket : Bound to IP:" << ifaceAddr.ip().toString();
            break;
        }
    }

    if(!ok)
        qDebug() << "sACNRxSocket : Failed to bind RX socket";

    return ok;
}

sACNTxSocket::sACNTxSocket(QObject *parent) : QUdpSocket(parent)
{

}

bool sACNTxSocket::bindMulticast()
{
    bool ok = false;

    // Bind to first IPv4 address on selected NIC
    QNetworkInterface iface = Preferences::getInstance()->networkInterface();

    foreach (QNetworkAddressEntry ifaceAddr, iface.addressEntries())
    {
        if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol)
        {
            ok = bind(ifaceAddr.ip());
            setSocketOption(QAbstractSocket::MulticastLoopbackOption, QVariant(1));
            setMulticastInterface(iface);
            qDebug() << "sACNTxSocket : Bound to IP:" << ifaceAddr.ip().toString();
            break;
        }
    }
  

    if(!ok)
        qDebug() << "sACNTxSocket : Failed to bind TX socket";

    return ok;
}
