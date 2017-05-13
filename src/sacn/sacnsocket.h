#ifndef SACNSOCKET_H
#define SACNSOCKET_H

#include <QObject>
#include <QUdpSocket>
#include "preferences.h"

class sACNRxSocket : public QUdpSocket
{
    Q_OBJECT
public:
    sACNRxSocket(QObject *parent = Q_NULLPTR);

    bool bindMulticast(quint16 universe);
    bool bindUnicast();
};

class sACNTxSocket : public QUdpSocket
{
    Q_OBJECT
public:
    sACNTxSocket(QObject *parent = Q_NULLPTR);

    bool bindMulticast();
};


#endif // SACNSOCKET_H
