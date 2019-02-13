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

#ifndef SACNSOCKET_H
#define SACNSOCKET_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkInterface>

class sACNRxSocket : public QUdpSocket
{
    Q_OBJECT
public:
    sACNRxSocket(QNetworkInterface iface, QObject *parent = Q_NULLPTR);

    bool bind(quint16 universe);
    int getBoundUniverse() { return m_universe; }
    QNetworkInterface getBoundInterface() { return m_interface; }

private:
    QNetworkInterface m_interface;
    int m_universe;
};

class sACNTxSocket : public QUdpSocket
{
    Q_OBJECT
public:
    sACNTxSocket(QNetworkInterface iface, QObject *parent = Q_NULLPTR);

    bool bind();

    //qint64 writeDatagram(const QNetworkDatagram &datagram);
    qint64 writeDatagram(const char *data, qint64 len, const QHostAddress &host, quint16 port);
    inline qint64 writeDatagram(const QByteArray &datagram, const QHostAddress &host, quint16 port)
        { return writeDatagram(datagram.constData(), datagram.size(), host, port); }

private:
    QNetworkInterface m_interface;
};


#endif // SACNSOCKET_H
