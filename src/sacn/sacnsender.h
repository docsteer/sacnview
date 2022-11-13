// Copyright 2016 Tom Steer
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

#ifndef SACNSENDER_H
#define SACNSENDER_H

#include <Qt>
#include <QMutex>
#include <vector>
#include <map>
#include "streamingacn.h"
#include "streamcommon.h"
#include "securesacn.h"
#include "tock.h"
#include "consts.h"
#include "sacnsocket.h"
#include "e1_11.h"

class QTimer;

class sACNSentUniverse : public QObject
{
    Q_OBJECT
public:
    // Constructor
    sACNSentUniverse(int universe);
    virtual ~sACNSentUniverse();

public slots:
    /**
     * @brief setLevel sets a single level in the universe
     * @param address - the address to set, 0-based (0-511)
     * @param value - the value to set (0-255)
     */
    void setLevel(quint16 address, quint8 value);
    /**
     * @brief setLevelRange sets a level range
     * @param start - start address, 0-based (0-511)
     * @param end - end address, 0-based (0-511)
     * @param value - level to set (0-255)
     */
    void setLevelRange(quint16 start, quint16 end, quint8 value);
    /**
     * @brief setLevel sets a level range
     * @param data - pointer to a data array
     */
    void setLevel(const quint8 *data, int len, int start=0);
    /**
     * @brief sets a vertical bar on a 16x32 grid for the universe
     */
    void setVerticalBar(quint16 index, quint8 level);
    /**
     * @brief sets a horizontal bar on a 16x32 grid for the universe
     */
    void setHorizontalBar(quint16 index, quint8 level);

    /**
     * @brief setName sets the universe name
     * @param name the name to set
     */
    void setName(const QString &name);
    QString name() { return m_name; }

    /**
     * @brief setPriorityMode - sets the priority mode of sACN to transmit, per-universe or per-address
     * @param mode - the mode to use
     */
    void setPriorityMode(PriorityMode mode);
    /**
     * @brief setPerChannelPriorities - sets the per-channel priority data for the source
     * @param priorities - a pointer to an array of priority values, must be 512 bytes
     */
    void setPerChannelPriorities(quint8 *priorities);
    /**
     * @brief setPerSourcePriority - sets the per-source priority for the source
     * @param priority - the priority value
     */
    void setPerSourcePriority(quint8 priority);
    quint8 perSourcePriority() { return m_priority; }
    /**
     * @brief startSending - starts sending for the selected universe
     * @param (Optional) preview - set the preview flag?
     */
    void startSending(bool preview = false);
    /**
     * @brief stopSending - stops sending for the selected universe
     */
    void stopSending();
    bool isSending() const { return m_isSending; }
    /**
     * @brief setUnicastAddress - sets the address to unicast data to
     * @param Address - a QHostAddress, default QHostAddress means multicast
     */
    void setUnicastAddress(const QHostAddress &address) { m_unicastAddress = address; }
    /**
     * @brief setProtocolVersion - sets the protocol version
     * @param version - draft or release
     */
    void setProtocolVersion(StreamingACNProtocolVersion version);
    /**
     * @brief copyLevels - copies the current output levels
     * @param dest - destination to copy to, uint8 array, must be 512 long
     */
    void copyLevels(quint8 *dest);

    /**
     * @brief setUniverse - set the universe of this sender. If active this will stop
     * and restart the source
     * @param universe - new universe.
     */
    void setUniverse(int universe);
    int universe() { return m_universe; }

    /**
     * @brief setSynchronizationAddress - set the Synchronization address of this sender.
     * @param address - new address.
     */
    void setSynchronization(quint16 address);
    int synchronization() { return m_synchronization; }

    /**
     * @brief setCID - set the CID of this sender.
     * If active this will stop and restart the source
     * @param cid - new CID.
     */
    void setCID(CID cid);
    CID cid() { return m_cid; }

    /**
     * @brief setSlotCount - set the Slot Count of this sender.
     * If active this will stop and restart the source
     * @param slotCount - new slotCount.
     */
    void setSlotCount(quint16 slotCount);
    quint16 slotCount() { return m_slotCount; }

    /**
     * @brief setSecurePassword - set the secure password, if used, of this sender
     * @param password - new Password
     */
    void setSecurePassword(const QString &password);
    const QString &getSecurePassword() { return m_password; }

    /**
     * @brief setSendFrequency - set minimum and maximum data send frequency
     * @param minimum - minimum FPS
     * @param maximum - maximum FPS
     */
    void setSendFrequency(float minimum, float maximum);
    std::pair<float, float> getBackgrounRefreshFrequency() const { return std::pair(m_minSendFreq, m_maxSendFreq); }

signals:
    /**
     * @brief sendingTimeout is emitted when the source stops sending
     * due to the send timeout setting
     */
    void sendingTimeout();

    /**
     * @brief universeChange is emitted when the source changes universe
     */
    void universeChange();

    /**
     * @brief synchronizationChange is emitted when the source changes Synchronization address
     */
    void synchronizationChange();

    /**
     * @brief cidChange is emitted when the source changes CID
     */
    void cidChange();

    /**
     * @brief slotCount is emitted when the source changes slot count
     */
    void slotCountChange();

    /**
     * @brief passwordChange is emitted when the source changes password
     */
    void passwordChange();

    /**
     * @brief sendFrequencyChange is emitted when the source changes minimum or maximum data send frequency
     */
    void sendFrequencyChange();

private slots:
    void doTimeout();
private:
    bool m_isSending;
    // The handle for the CStreamServer universe
    uint m_handle;
    // The handle for the CStreamServer universe of priority data
    uint m_priorityHandle;
    // The pointer to the data
    quint8 *m_slotData;
    // Number of slots
    quint16 m_slotCount;
    // The priority
    quint8 m_priority;
    // Source name
    QString m_name;
    // Universe
    quint16 m_universe;
    // FX speed
    int m_fx_speed;
    // The CID
    CID m_cid;
    // Priority mode
    PriorityMode m_priorityMode;
    // Per-channel priorities
    quint8 m_perChannelPriorities[MAX_DMX_ADDRESS];
    // Unicast
    QHostAddress m_unicastAddress;
    // Protocol Version
    StreamingACNProtocolVersion m_version;
    // Timer to shutdown after timeout
    QTimer *m_checkTimeoutTimer;
    // Synchronization Address
    quint16 m_synchronization;
    // Secure password
    QString m_password;
    // Minimum and maximum send frequencies
    float m_minSendFreq;
    float m_maxSendFreq;
};

class CStreamServer : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief getInstance returns the instance of CStreamServer.
     * @return Returns the one instance of the CStreamServer
     */
    static CStreamServer *getInstance();
    /**
     * @brief shutdown shuts down the CStreamServer.
     * not thread safe!
     */
    static void shutdown();


  //Use this to create a universe for a source cid, startcode, etc.
  //If it returns true, two parameters are filled in: The data buffer for the values that can
  //  be manipulated directly, and the handle to use when calling the rest of these functions.
  //Note that a universe number of 0 is invalid.
  //Set reserved to be 0 and options to either be 0 or PREVIEW_DATA_OPTION (can be changed later
  //  by OptionsPreviewData).
  //if ignore_inactivity_logic is set to false (the default for DMX), Tick will handle sending the
  //  3 identical packets at the lower frequency that sACN requires.  It sends these packets at
  //  send_intervalms intervals (again defaulted for DMX).  Note that even if you are not using the
  //  inactivity logic, send_intervalms expiry will trigger a resend of the current universe packet.
  //Data on this universe will not be initially sent until marked dirty.
  bool CreateUniverse(
          const CID& source_cid, const char* source_name, quint8 priority,
          quint16 synchronization, quint8 options, quint8 start_code,
          quint16 universe, quint16 slot_count, quint8*& pslots, uint& handle,
          float minimum_send_rate = E1_11::MIN_REFRESH_RATE_HZ,
          float maximum_send_rate = E1_11::MAX_REFRESH_RATE_HZ,
          CIPAddr unicastAddress = CIPAddr(),
          StreamingACNProtocolVersion version = StreamingACNProtocolVersion::sACNProtocolRelease);

  //After you add data to the data buffer, call this to trigger the data send
  //on the next Tick boundary.
  //Otherwise, the data won't be sent until the inactivity or send_interval
  //time.
  //Due to the fact that access to the universes needs to be thread safe,
  //this function allows you to set an array of universes dirty at once
  //(to incur the lock overhead only once).  If your algorithm is to
  //SetUniversesDirty and then immediately call Tick, you will save a lock
  //access by passing these in directly to Tick.
  void SetUniverseDirty(uint handle);

  //Use this to destroy a universe.  While this is thread safe internal to the library,
  //this does invalidate the pslots array that CreateUniverse returned, so do not access
  //that memory after or during this call.  This function also handles the logic to
  //mark the stream as Terminated and send a few extra terminated packets.
  void DestroyUniverse(uint handle);

  //In the event that you want to send out a message for a particular
  //universe (and start code) in between ticks, call this function.
  //This is particularly useful if you want to ensure a priority change goes
  //out before a DMX value change.
  //This does not affect the dirty bit for the universe, inactivity count,
  //etc, and the tick will still operate normally when called.
  //This is not thread safe with Tick -- Don't call when Tick is called
  void SendUniverseNow(uint handle);


  void setUniverseName(uint handle, const char *name);
  void setUniversePriority(uint handle, quint8 priority);
  void setSynchronizationAddress(uint handle, quint16 address);
  void setSecurePassword(uint handle, const QString &password);

  // Set the minimum and maximum send frequency (FPS)
  void setSendFrequency(uint handle, float minimum = E1_11::MIN_REFRESH_RATE_HZ, float maximum = E1_11::MAX_REFRESH_RATE_HZ);

  //Use this to destroy a priority universe.
  void DEBUG_DESTROY_PRIORITY_UNIVERSE(uint handle);

  /*DEBUG USAGE ONLY --causes packets to be "dropped" on a particular universe*/
  void DEBUG_DROP_PACKET(uint handle, quint8 decrement);

   //sets the preview_data bit of the options field
   virtual void OptionsPreviewData(uint handle, bool preview);

   //sets the stream_terminated bit of the options field
   virtual void OptionsStreamTerminated(uint handle, bool terminated);
private slots:
  /**
   * @brief TickLoop - Handles transmission of sACN
   */
  void TickLoop();

private:
    CStreamServer();
    virtual ~CStreamServer();
    static CStreamServer *m_instance;

    sACNTxSocket * m_sendsock;  //The actual socket used for sending
    QThread *m_thread;
    bool m_thread_stop;

    typedef std::pair<CID, quint16> cidanduniverse;
    //Each universe shares its sequence numbers across start codes.
    //This is the central storage location, along with a refcount
    typedef std::pair<int, quint8*> seqref;
    std::map<cidanduniverse, seqref > m_seqmap;
    typedef std::map<cidanduniverse, seqref >::iterator seqiter;

    //Returns a pointer to the storage location for the universe, adding if need be.
    //The newly-added location contains sequence number 0.
    quint8* GetPSeq(const CID &cid, quint16 universe);

    //Removes a reference to the storage location for the universe, removing completely if need be.
    void RemovePSeq(const CID &cid, quint16 universe);

    //Each universe is just the full buffer and some state
    struct universe
    {
        quint16 number;           //The universe number
        quint8 start_code;       //The start code
        uint handle;            //The handle.  This is needed to help deletions.
        quint8 num_terminates;   //The number of consecutive times the
                                //stream_terminated option flag has been set.
        quint8* psend;           //The full sending buffer, which the user can access the data portion.
                                //If NULL, this is not an active universe (just a hole in the vector)
        uint sendsize;
        bool isdirty;
        bool suppresed;             //Transmission rate suppresed?
        ttimer send_interval;       //Whether or not it's time to send a non-dirty packet
        ttimer min_interval;        //Whether it's too soon to send a packet
        QHostAddress sendaddr;      //The multicast address we're sending to
        StreamingACNProtocolVersion version;                 //Draft or released sACN
        CID cid;                    // The CID
        PathwaySecure::password_t password; // Pathway Secure Protocol Password

        //and the constructor
      universe():number(0),handle(0), num_terminates(0), psend(NULL),isdirty(false),
          suppresed(false),version(sACNProtocolRelease), cid(), password("") {}
    };

    //The handle is the vector index
    std::vector<universe> m_multiverse;

   //Perform the logical destruction and cleanup of a universe
   //and its related objects.
   void DoDestruction(uint handle);

   // Mutex for write protection of members
   QMutex m_writeMutex;
};


#endif // SACNSENDER_H
