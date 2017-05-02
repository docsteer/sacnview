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
#ifdef Q_OS_WIN
    bool ok = bind(a);
#else
    bool ok = bind();
#endif
    if(ok)
        qDebug() << "sACNTxSocket : Bound to IP: " << a.toString().toLatin1();
    else
        qDebug() << "sACNTxSocket : Failed to bind TX socket";
    setSocketOption(QAbstractSocket::MulticastLoopbackOption, QVariant(1));
    setMulticastInterface(iface);
}
