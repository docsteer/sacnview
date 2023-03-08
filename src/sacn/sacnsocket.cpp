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

#include "sacnsocket.h"

#include <QNetworkAddressEntry>
#include <QDebug>
#include <QThread>
#include "ipaddr.h"
#include "streamcommon.h"
#include "streamingacn.h"
#include "sacnlistener.h"
#include "preferences.h"

sACNRxSocket::sACNRxSocket(QNetworkInterface iface, QObject *parent) :
    QUdpSocket(parent),
    m_interface(iface)
{}

sACNRxSocket::sBindStatus sACNRxSocket::bind(quint16 universe)
{
    sACNRxSocket::sBindStatus status;
    m_universe = universe;
    bool ok = false;

    CIPAddr multicastAddr;
    GetUniverseAddress(universe, multicastAddr);
    m_multicastAddr = QHostAddress(multicastAddr.GetV4Address());

    QHostAddress listenAddr = QHostAddress::AnyIPv4;
    ok = QUdpSocket::bind(listenAddr,
              STREAM_IP_PORT,
              QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);

    status.unicast = ok ? BIND_OK : BIND_FAILED;

    // Join multicast on selected NIC
    if (ok) {
        #if (QT_VERSION <= QT_VERSION_CHECK(5, 8, 0))
            #ifdef Q_OS_WIN
                #pragma message("setMulticastInterface() fails to bind to correct interface on systems running IPV4 and IPv6 with QT <= 5.8.0")
            #else
                #error setMulticastInterface() fails to bind to correct interface on systems running IPV4 and IPv6 with QT <= 5.8.0
            #endif
        #endif
        setMulticastInterface(m_interface);
        ok |= joinMulticastGroup(m_multicastAddr, m_interface);
        status.multicast = ok ? BIND_OK : BIND_FAILED;
    }

    if(ok) {
        qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Bound to interface:" << multicastInterface().name();
        qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Joining Multicast Group:" << QHostAddress(multicastAddr.GetV4Address()).toString();
    } else {
        close();
        qDebug() << "sACNRxSocket " << QThread::currentThreadId() << ": Failed to bind RX socket";
    }

    return status;
}

sACNTxSocket::sACNTxSocket(QNetworkInterface iface, QObject *parent) :
    QUdpSocket(parent),
    m_interface(iface)
{}

bool sACNTxSocket::bind()
{
    bool ok = false;

    // Bind to first IPv4 address on selected NIC
    foreach (QNetworkAddressEntry ifaceAddr, m_interface.addressEntries()) {
        if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol) {
            ok = QUdpSocket::bind(ifaceAddr.ip());
            if (ok) {
                qDebug() << "sACNTxSocket " << QThread::currentThreadId() << ": Bound to IP:" << ifaceAddr.ip().toString();

#ifndef Q_OS_WIN
                // Don't send to self, this will cause issues on multihomed systems
                setSocketOption(QAbstractSocket::MulticastLoopbackOption, QVariant(0));
#endif

                setSocketOption(QAbstractSocket::MulticastTtlOption, QVariant(Preferences::getInstance()->GetMulticastTtl()));

                setMulticastInterface(m_interface);
                qDebug() << "sACNTxSocket " << QThread::currentThreadId() << ": Bound to interface:" << multicastInterface().name();
            }
            break;
        }
    }

    if (!ok) qDebug() << "sACNTxSocket " << QThread::currentThreadId() << ": Failed to bind TX socket";

    return ok;
}

bool writeDatagramLoopbackTest(const QHostAddress &dest, QHostAddress localAddress) {
#ifdef Q_OS_WIN
    Q_UNUSED(dest);
    Q_UNUSED(localAddress);
    return false;
#endif

#ifdef Q_OS_LINUX
    return (dest.isMulticast() && !localAddress.isLoopback());
#endif

#ifdef Q_OS_MACOS
    Q_UNUSED(localAddress);
    return (dest.isMulticast());
#endif
}

qint64 sACNTxSocket::writeDatagram(const char *data, qint64 len, const QHostAddress &host, quint16 port)
{
    // Copy to correct internal listener(s) for local display, if required by platform
    if (writeDatagramLoopbackTest(host, localAddress())) {
        CIPAddr addr;
        for (auto &weakListener : sACNManager::getInstance()->getListenerList()) {
            sACNManager::tListener listener(weakListener);
            if (listener) {
                GetUniverseAddress(listener->universe(), addr);
                if (QHostAddress(addr.GetV4Address()) == host) {
                    listener->processDatagram(
                                QByteArray(data, len),
                                QHostAddress(host),
                                localAddress());
                }
            }
        }
    }

    // Send to world
    return QUdpSocket::writeDatagram(data, len, host, port);
}
