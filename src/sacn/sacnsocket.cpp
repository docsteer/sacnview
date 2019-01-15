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

#include <QNetworkAddressEntry>
#include <QDebug>
#include <QThread>
#include "ipaddr.h"
#include "streamcommon.h"

sACNRxSocket::sACNRxSocket(QNetworkInterface iface, QObject *parent) :
    QUdpSocket(parent),
    m_interface(iface)
{
}

bool sACNRxSocket::bind(quint16 universe)
{
    bool ok = false;

    CIPAddr addr;
    GetUniverseAddress(universe, addr);

    // Bind to univcast address
    ok = bindUnicast();

    // Join multicast on selected NIC
    if (ok)
    {
        #if (QT_VERSION <= QT_VERSION_CHECK(5, 8, 0))
            #ifdef Q_OS_WIN
                #pragma message("setMulticastInterface() fails to bind to correct interface on systems running IPV4 and IPv6 with QT <= 5.8.0")
            #else
                #error setMulticastInterface() fails to bind to correct interface on systems running IPV4 and IPv6 with QT <= 5.8.0
            #endif
        #endif
        setMulticastInterface(m_interface);
        ok |= joinMulticastGroup(QHostAddress(addr.GetV4Address()), m_interface);
    }

    if(ok)
    {
        qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Bound to interface:" << m_interface.name();
        qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Joining Multicast Group:" << QHostAddress(addr.GetV4Address()).toString();
    }
    else
    {
        close();
        qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Failed to bind RX socket";
    }

    return ok;
}

bool sACNRxSocket::bindUnicast()
{
    bool ok = false;

    // Bind to first IPv4 address on selected NIC
    foreach (QNetworkAddressEntry ifaceAddr, m_interface.addressEntries())
    {
        if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol)
        {
            ok = QUdpSocket::bind(ifaceAddr.ip(),
                      STREAM_IP_PORT,
                      QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
            if (ok) {
                qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Bound to IP:" << ifaceAddr.ip().toString();
                break;
            }
        }
    }

    if (!ok) {
        close();
        qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Failed to bind RX socket";
    }

    return ok;
}

sACNTxSocket::sACNTxSocket(QNetworkInterface iface, QObject *parent) :
    QUdpSocket(parent),
    m_interface(iface)
{
}

bool sACNTxSocket::bind()
{
    bool ok = false;

    // Bind to first IPv4 address on selected NIC
    foreach (QNetworkAddressEntry ifaceAddr, m_interface.addressEntries())
    {
        if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol)
        {
            ok = QUdpSocket::bind(ifaceAddr.ip());
            if (ok) {
                setSocketOption(QAbstractSocket::MulticastLoopbackOption, QVariant(1));
                setMulticastInterface(m_interface);
                qDebug() << "sACNTxSocket " << QThread::currentThreadId() << ": Bound to IP:" << ifaceAddr.ip().toString();
            }
            break;
        }
    }
  
    if(!ok)
        qDebug() << "sACNTxSocket " << QThread::currentThreadId() << ": Failed to bind TX socket";

    return ok;
}
