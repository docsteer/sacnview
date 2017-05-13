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
            qDebug() << "sACNTxSocket : Bound to IP: " << ifaceAddr.ip().toString();
            break;
        }
    }

    if(!ok)
        qDebug() << "sACNTxSocket : Failed to bind TX socket";

    return ok;
}
