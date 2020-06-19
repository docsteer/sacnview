#ifndef SACNDISCOVERY_H
#define SACNDISCOVERY_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QMutex>
#include <QThread>
#include "sacnsocket.h"
#include "sacnlistener.h"
#include "CID.h"

#define E131_UNIVERSE_DISCOVERY_INTERVAL 10000
#define E131_UNIVERSE_DISCOVERY_SIZE_MIN 120
#define E131_UNIVERSE_DISCOVERY_SIZE_MAX 1144

class sACNDiscoveryTX : public QObject
{
    Q_OBJECT
public:
    explicit sACNDiscoveryTX();
    ~sACNDiscoveryTX();

    static void start();
    static sACNDiscoveryTX *getInstance();
    void sendDiscoveryPacketNow() { m_sendTimer->start(0); }

signals:

public slots:

private slots:
    void sendDiscoveryPacket();

private:
    static sACNDiscoveryTX *m_instance;

    QMutex *m_mutex;
    QTimer *m_sendTimer;
    sACNTxSocket *m_sendSock;
};

class sACNDiscoveryRX : public QObject
{
    Q_OBJECT
public:
    explicit sACNDiscoveryRX();
    ~sACNDiscoveryRX();

    static void start();
    static sACNDiscoveryRX *getInstance();

    struct sACNSourceDetail
    {
        sACNSourceDetail() {}
        QString Name;
        QHash<quint16, ttimer> Universe;
        bool operator ==(const sACNSourceDetail& detail) const
        {
            return (detail.Name == Name) && (detail.Universe == Universe);
        }
        bool operator!=(const sACNSourceDetail& detail) const
        {
            return !operator==(detail);
        }
    };



    typedef QHash<CID, sACNSourceDetail*> tDiscoveryList;

    void processPacket(quint8* pbuf, uint buflen);
    tDiscoveryList getDiscoveryList() { return m_discoveryList; }

signals:
    void newSource(QString cid);
    void newUniverse(QString cid, quint16 universe);

    void expiredSource(QString cid);
    void expiredUniverse(QString cid, quint16 universe);
public slots:

private slots:
    void timeoutUniverses();

private:
    static sACNDiscoveryRX *m_instance;

    QMutex *m_mutex;

    sACNListener *m_listener;
    QThread *m_thread;

    QTimer *m_expiredTimer;

    tDiscoveryList m_discoveryList;
};
#endif // SACNDISCOVERY_H
