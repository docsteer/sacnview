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
        #if (QT_VERSION <= QT_VERSION_CHECK(5, 8, 0))
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
