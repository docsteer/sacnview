#ifndef SACNDISCOVERY_H
#define SACNDISCOVERY_H

#include "CID.h"
#include "sacnlistener.h"
#include "sacnsocket.h"
#include <QList>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QTimer>

#define E131_UNIVERSE_DISCOVERY_SIZE_MIN 120
#define E131_UNIVERSE_DISCOVERY_SIZE_MAX 1144

class sACNDiscoveryTX : public QObject
{
    Q_OBJECT
public:

    explicit sACNDiscoveryTX();
    ~sACNDiscoveryTX();

    static void start();
    static sACNDiscoveryTX * getInstance();
    void sendDiscoveryPacketNow() { m_sendTimer->start(0); }

signals:

public slots:

private slots:
    void sendDiscoveryPacket();

private:

    static sACNDiscoveryTX * m_instance;

    QMutex * m_mutex;
    QTimer * m_sendTimer;
    sACNTxSocket * m_sendSock;
};

class sACNDiscoveryRX : public QObject
{
    Q_OBJECT
public:

    explicit sACNDiscoveryRX();
    ~sACNDiscoveryRX();

    static void start();
    static sACNDiscoveryRX * getInstance();

    struct sACNSourceDetail
    {
        sACNSourceDetail() {}
        QString Name;
        QHash<quint16, ttimer> Universe;
        bool operator==(const sACNSourceDetail & detail) const
        {
            return (detail.Name == Name) && (detail.Universe == Universe);
        }
        bool operator!=(const sACNSourceDetail & detail) const { return !operator==(detail); }
    };

    typedef QHash<CID, sACNSourceDetail *> tDiscoveryList;

    void processPacket(const quint8 * pbuf, size_t buflen);
    const tDiscoveryList & getDiscoveryList() const { return m_discoveryList; }

signals:
    void newSource(CID cid);
    void newUniverse(CID cid, quint16 universe);

    void expiredSource(CID cid);
    void expiredUniverse(CID cid, quint16 universe);
public slots:

private slots:
    void timeoutUniverses();

private:

    static sACNDiscoveryRX * m_instance;

    QMutex * m_mutex;

    sACNListener * m_listener;
    QThread * m_thread;

    QTimer * m_expiredTimer;

    tDiscoveryList m_discoveryList;
};
#endif // SACNDISCOVERY_H
