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

#ifndef SACNSENDER_H
#define SACNSENDER_H

#include <Qt>
#include <QMutex>
#include <vector>
#include <map>
#include <QSharedPointer>
#include <QWeakPointer>
#include "streamingacn.h"
#include "streamcommon.h"
#include "tock.h"
#include "deftypes.h"
#include "consts.h"
#include "sacnsocket.h"

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
    void setPerChannelPriorities(uint1 *priorities);
    /**
     * @brief setPerSourcePriority - sets the per-source priority for the source
     * @param priority - the priority value
     */
    void setPerSourcePriority(uint1 priority);
    uint1 perSourcePriority() { return m_priority;};
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

signals:
    /**
     * @brief sendingTimeout is emitted when the source stops sending
     * due to the send timeout setting
     */
    void sendingTimeout();
private slots:
    void doTimeout();
private:
    bool m_isSending;
    // The handle for the CStreamServer universe
    uint m_handle;
    // The handle for the CStreamServer universe of priority data
    uint m_priorityHandle;
    // The pointer to the data
    uint1 *m_slotData;
    // The priority
    uint1 m_priority;
    // Source name
    QString m_name;
    // Universe
    uint2 m_universe;
    // FX speed
    int m_fx_speed;
    // The CID
    CID m_cid;
    // Priority mode
    PriorityMode m_priorityMode;
    // Per-channel priorities
    uint1 m_perChannelPriorities[MAX_DMX_ADDRESS];
    // Unicast
    QHostAddress m_unicastAddress;
    // Protocol Version
    StreamingACNProtocolVersion m_version;
    // Timer to shutdown after timeout
    QTimer *m_checkTimeoutTimer;
};


//These definitions are to be used with the ignore_inactivity_logic field of CreateUniverse
#define IGNORE_INACTIVE_DMX  false
#define IGNORE_INACTIVE_PRIORITY false /*Any priority change should send three packets anyway, around your frame rate*/

//These definitions are to be used with the send_intervalms parameter of CreateUniverse
#define SEND_INTERVAL_DMX	850	/*If no data has been sent in 850ms, send another DMX packet*/
#define SEND_INTERVAL_PRIORITY 1000	/*By default, per-channel priority packets are sent once per second*/

//Bitflags for the options parameter of Create Universe.
//Alternatively, you can directly set them while a universe is running with
//OptionsPreviewData and OptionsStreamTerminated.  The terminated option doesn't
//really need to be used, as DestroyUniverse handles that functionality for you.
#define PREVIEW_DATA_OPTION 0x80 // Bit 7
#define STREAM_TERMINATED_OPTION 0x40 // Bit 6

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
  bool CreateUniverse(const CID& source_cid, const char* source_name, uint1 priority,
                       uint2 reserved, uint1 options, uint1 start_code,
                              uint2 universe, uint2 slot_count, uint1*& pslots, uint& handle,
                              bool ignore_inactivity_logic = IGNORE_INACTIVE_DMX,
                              uint send_intervalms = SEND_INTERVAL_DMX, CIPAddr unicastAddress = CIPAddr(), bool draft = false);

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
  void setUniversePriority(uint handle, uint1 priority);

  //Use this to destroy a priority universe.
  void DEBUG_DESTROY_PRIORITY_UNIVERSE(uint handle);

  /*DEBUG USAGE ONLY --causes packets to be "dropped" on a particular universe*/
  void DEBUG_DROP_PACKET(uint handle, uint1 decrement);

   //sets the preview_data bit of the options field
   virtual void OptionsPreviewData(uint handle, bool preview);

   //sets the stream_terminated bit of the options field
   virtual void OptionsStreamTerminated(uint handle, bool terminated);
private slots:
  /**
   * @brief Tick - called every 10ms, handles transmission of sACN
   */
  void Tick();

private:
    CStreamServer();
    virtual ~CStreamServer();
    static CStreamServer *m_instance;

    sACNTxSocket * m_sendsock;  //The actual socket used for sending
    QTimer *m_tickTimer;
    QThread *m_thread;


    typedef std::pair<CID, uint2> cidanduniverse;
    //Each universe shares its sequence numbers across start codes.
    //This is the central storage location, along with a refcount
    typedef std::pair<int, uint1*> seqref;
    std::map<cidanduniverse, seqref > m_seqmap;
    typedef std::map<cidanduniverse, seqref >::iterator seqiter;

    //Returns a pointer to the storage location for the universe, adding if need be.
    //The newly-added location contains sequence number 0.
    uint1* GetPSeq(const CID &cid, uint2 universe);

    //Removes a reference to the storage location for the universe, removing completely if need be.
    void RemovePSeq(const CID &cid, uint2 universe);

    //Each universe is just the full buffer and some state
    struct universe
    {
        uint2 number;           //The universe number
        uint1 start_code;       //The start code
        uint handle;            //The handle.  This is needed to help deletions.
        uint1 num_terminates;   //The number of consecutive times the
                                //stream_terminated option flag has been set.
        uint1* psend;           //The full sending buffer, which the user can access the data portion.
                                //If NULL, this is not an active universe (just a hole in the vector)
        uint sendsize;
        bool isdirty;
        bool waited_for_dirty;      //Until we receive a dirty flag, we don't start outputting the universe.
        bool ignore_inactivity;     //If true, we don't bother looking at inactive_count
        uint inactive_count;		//After 3 of these, we start sending at send_interval
        ttimer send_interval;       //Whether or not it's time to send a non-dirty packet
        QHostAddress sendaddr;      //The multicast address we're sending to
        bool draft;                 //Draft or released sACN
        CID cid;                    // The CID

        //and the constructor
      universe():number(0),handle(0), num_terminates(0), psend(NULL),isdirty(false),
          waited_for_dirty(false),inactive_count(0),draft(false), cid() {}
    };

    //The handle is the vector index
    std::vector<universe> m_multiverse;
    typedef std::vector<universe>::iterator verseiter;


   //Perform the logical destruction and cleanup of a universe
   //and its related objects.
   void DoDestruction(uint handle);

   // Mutex for write protection of members
   QMutex m_writeMutex;
};


#endif // SACNSENDER_H
