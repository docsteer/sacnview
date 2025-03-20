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

#include <QSharedPointer>
#include <QWeakPointer>
#include <QHash>
#include <QString>
#include <QHostAddress>
#include <QMutex>

#include "CID.h"
#include "tock.h"
#include "streamcommon.h"
#include "fpscounter.h"

#include <array>

// Forward Declarations
class sACNListener;
class sACNSentUniverse;
class sACNSource;

enum StreamingACNProtocolVersion
{
    sACNProtocolUnknown = 0,
    sACNProtocolDraft,
    sACNProtocolRelease,
    sACNProtocolPathwaySecure // Pathway Connectivity Secure DMX Protocol
};

QString GetProtocolVersionString(StreamingACNProtocolVersion value);

// The sACNManager class is a singleton that manages the lifespan of sACNTransmitters and sACNListeners.
class sACNManager : public QObject
{
    Q_OBJECT
public:
  static sACNManager& Instance();

    typedef QSharedPointer<sACNListener> tListener;
  typedef QWeakPointer<sACNListener> wListener;

    typedef QSharedPointer<sACNSentUniverse> tSender;
  typedef QWeakPointer<sACNSentUniverse> wSender;

  ~sACNManager();

  inline static tock GetTock() { return Tock_GetTock(); }
  inline static qint64 nsecsElapsed() { return Tock_GetTock().Get().count(); }
  inline static qint64 elapsed() { return std::chrono::duration_cast<std::chrono::milliseconds>(Tock_GetTock().Get()).count(); }
  inline static qreal secsElapsed() { return std::chrono::duration<qreal>(Tock_GetTock().Get()).count(); }

public slots:
  void listenerDelete(QObject* obj = Q_NULLPTR);

  void senderDelete(QObject* obj = Q_NULLPTR);
private:
    sACNManager();
    QMutex sACNManager_mutex;

    QHash<QObject*, quint16> m_objToUniverse;
    QHash<QObject*, CID> m_objToCid;

  QHash<quint16, wListener> m_listenerHash;

  // The pool of threads to use for listener objects
  std::vector<QThread*> m_threadPool;
  size_t m_nextThread = 0;

  QThread* GetThread();

    tSender createSender(CID cid, quint16 universe);
  QHash<CID, QHash<quint16, wSender> > m_senderHash;

public:
    tListener getListener(quint16 universe);
  const decltype(m_listenerHash)& getListenerList() const { return m_listenerHash; }

    tSender getSender(quint16 universe, CID cid = CID::CreateCid());
  const decltype(m_senderHash)& getSenderList() const { return m_senderHash; }

signals:
    void newSender();
    void deletedSender(CID cid, quint16 universe);

    void newListener(quint16 universe);
    void deletedListener(quint16 universe);

  void sourceFound(quint16 universe, sACNSource* source);
  void sourceLost(quint16 universe, sACNSource* source);
  void sourceResumed(quint16 universe, sACNSource* source);
  void sourceChanged(quint16 universe, sACNSource* source);

private slots:
    void senderUniverseChanged();
    void senderCIDChanged();

private:
};

class sACNSource
{
  Q_DISABLE_COPY(sACNSource)
public:
  explicit sACNSource(const CID& source_cid, uint16_t universe);
  const CID src_cid;
  const uint16_t universe = 0;
  bool src_valid = false;
  bool src_stable = false;
  uint8_t lastseq = 0;
    ttimer active;  //If this expires, we haven't received any data in over a second
    //The per-channel priority alternate start code policy requires we detect the source only after
    //a STARTCODE_PRIORITY packet was received or 1.5 seconds have expired
  bool waited_for_dd = false;
  bool doing_dmx = false; //if true, we are processing dmx data from this source
  bool doing_per_channel = false;  // If true, we are tracking per-channel priority messages for this source
  bool isPreview = false;
  ttimer priority_wait;  // If !initially_notified, used to track if a source is finally detected
  // (either by receiving priority or timeout).  If doing_per_channel,
  // used to time out the 0xdd packets to see if we lost per-channel priority
  std::array<uint8_t, DMX_SLOT_MAX> level_array = {};
  std::array<uint8_t, DMX_SLOT_MAX> last_level_array = {};
  std::array<uint8_t, DMX_SLOT_MAX> priority_array = {};
  std::array<uint8_t, DMX_SLOT_MAX> last_priority_array = {};
  std::array<bool, DMX_SLOT_MAX> dirty_array = {}; // Set if an individual level or priority has changed
  quint16 slot_count = 0; // Number of slots actually received
  bool source_params_change = false; // Set if any parameter of the source changes between packets
  bool source_levels_change = false; // Set if any levels
  bool priority_array_bad = false;

  uint8_t priority = 0;
  uint16_t synchronization = 0;
    sACNManager::tListener sync_listener;
    QString name;
    QHostAddress ip;
    FpsCounter fpscounter;
    // The number of sequence errors from this source
  unsigned int seqErr = 0;
    // The number of jumps (increments by anything other than 1) of this source
  unsigned int jumps = 0;
    // Protocol Version
    StreamingACNProtocolVersion protocol_version;

    // Pathways Secure DMX
    struct {
        bool passwordOk = false;
        bool sequenceOk = false;
        bool digestOk = false;
    bool isSecure() const { return passwordOk && sequenceOk && digestOk; }
    } pathway_secure;

  void resetSeqErr() { seqErr = 0; }
  void resetJumps() { jumps = 0; }

  // Have received new levels, store and check for changes
  void storeReceivedLevels(const uint8_t* pdata, uint16_t rx_slot_count);
  // Have received new priority array, store and check for changes
  void storeReceivedPriorities(const uint8_t* pdata, uint16_t rx_slot_count);

  // Any priority value is invalid
  bool HasInvalidPriority() const;
};

#endif // STREAMINGACN_H
