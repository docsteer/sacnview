#ifndef SACNDISCOVERY_H
#define SACNDISCOVERY_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QMutex>
#include "sacnsocket.h"
#include "CID.h"

#define E131_UNIVERSE_DISCOVERY_INTERVAL 10000
#define E131_UNIVERSE_DISCOVERY_SIZE_MIN 120
#define E131_UNIVERSE_DISCOVERY_SIZE_MAX 1143

class sacndiscoveryTX : public QObject
{
    Q_OBJECT
public:
    explicit sacndiscoveryTX(QObject *parent);
    virtual ~sacndiscoveryTX();

signals:

public slots:

private slots:
    void sendDiscoveryPacket();

private:
    QMutex *m_mutex;
    QTimer *m_sendTimer;
    sACNTxSocket *m_sendSock;
};

#endif // SACNDISCOVERY_H
