// Copyright (c) 2015 Electronic Theatre Controls, http://www.etcconnect.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef STREAMINGACN_H
#define STREAMINGACN_H

#include <Qt>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QHash>
#include <QString>
#include <QHostAddress>
#include <QElapsedTimer>
#include <QMutex>

#include "CID.h"
#include "tock.h"
#include "streamcommon.h"
#include "fpscounter.h"

// Forward Declarations
class sACNListener;
class sACNSentUniverse;

enum StreamingACNProtocolVersion
{
    sACNProtocolUnknown = 0,
    sACNProtocolDraft,
    sACNProtocolRelease,
};


class sACNSource : public QObject
{
    Q_OBJECT
public:
    explicit sACNSource();
    CID src_cid;
    bool src_valid;
    quint8 lastseq;
    ttimer active;  //If this expires, we haven't received any data in over a second
    //The per-channel priority alternate start code policy requires we detect the source only after
    //a STARTCODE_PRIORITY packet was received or 1.5 seconds have expired
    bool waited_for_dd;
    bool doing_dmx; //if true, we are processing dmx data from this source
    bool doing_per_channel;  //If true, we are tracking per-channel priority messages for this source
    bool isPreview;
    ttimer priority_wait;  //if !initially_notified, used to track if a source is finally detected
                                  //(either by receiving priority or timeout).  If doing_per_channel,
                                  //used to time out the 0xdd packets to see if we lost per-channel priority
    quint16 universe;
    quint8 level_array[DMX_SLOT_MAX];
    quint16 slot_count; // Number of slots actually received
    quint8 priority_array[DMX_SLOT_MAX];
    quint8 last_level_array[DMX_SLOT_MAX];
    quint8 last_priority_array[DMX_SLOT_MAX];
    bool dirty_array[DMX_SLOT_MAX]; // Set if an individual level or priority has changed
    bool source_params_change; // Set if any parameter of the source changes between packets
    bool source_levels_change;

    quint8 priority;
    quint16 synchronization;
    QString name;
    QString cid_string();
    QHostAddress ip;
    fpsCounter fpscounter;
    // The number of sequence errors from this source
    int seqErr;
    // The number of jumps (increments by anything other than 1) of this source
    int jumps;
    // Protocol Version
    StreamingACNProtocolVersion protocol_version;

public slots:
    void resetSeqErr() {
        seqErr = 0;
    }

    void resetJumps() {
        jumps = 0;
    }
};


// The sACNManager class is a singleton that manages the lifespan of sACNTransmitters and sACNListeners.
class sACNManager : public QObject
{
    Q_OBJECT
public:
    static sACNManager *getInstance();

    typedef QSharedPointer<sACNListener> tListener;
    typedef QSharedPointer<sACNSentUniverse> tSender;

public slots:
    void listenerDelete(QObject *obj = Q_NULLPTR);

    void senderDelete(QObject *obj = Q_NULLPTR);
private:
    sACNManager();
    static sACNManager *m_instance;
    QMutex sACNManager_mutex;

    QHash<QObject*, quint16> m_objToUniverse;
    QHash<QObject*, CID> m_objToCid;

    QHash<quint16, QWeakPointer<sACNListener>> m_listenerHash;
    QHash<quint16, QThread *> m_listenerThreads;

    tSender createSender(CID cid, quint16 universe);
    QHash<CID, QHash<quint16, QWeakPointer<sACNSentUniverse>> > m_senderHash;

public:
    tListener getListener(quint16 universe);
    const decltype(m_listenerHash) getListenerList() { return m_listenerHash; }

    tSender getSender(quint16 universe, CID cid = CID::CreateCid());
    const decltype(m_senderHash) getSenderList() { return m_senderHash; }

signals:
    void newSender();

private slots:
    void senderUniverseChanged();
    void senderCIDChanged();

private:
};

#endif // STREAMINGACN_H
