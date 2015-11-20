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

#ifndef SACNSENDER_H
#define SACNSENDER_H

#include <Qt>
#include <QUdpSocket>
#include <QMutex>
#include <vector>
#include <map>
#include "streamcommon.h"
#include "tock.h"
#include "deftypes.h"
#include "consts.h"

class QTimer;

class sACNSentUniverse : public QObject
{
    Q_OBJECT;
public:
    // Constructor
    sACNSentUniverse(int universe);
    virtual ~sACNSentUniverse();

    enum sACNUniverseEffect{
        FxNone,
        FxFadeRangeRamp,
        FxFadeRangeSinewave,
        FxChaseRange,
        FxText,
        FxDateTime
    };


    void getFxSpeed(int speed) const;
public slots:
    /**
     * @brief setFxMode sets the effect mode for the universe
     * @param mode - the mode to apply
     */
    void setFxMode(sACNUniverseEffect mode);
    /**
     * @brief setLevel sets a single level in the universe
     * @param address - the address to set, 0-based (0-511)
     * @param value - the value to set (0-255)
     */
    void setLevel(uint2 address, uint1 value);
    /**
     * @brief setLevel sets a level range
     * @param start - start address, 0-based (0-511)
     * @param end - end address, 0-based (0-511)
     * @param value - level to set (0-255)
     */
    void setLevel(uint2 start, uint2 end, uint1 value);
    /**
     * @brief setFxSpeed sets the effect speed, which is based on a clock of 10ms.
     * @param speed. Speed in Hz, in increments of 1/10ms. So 1 is max speed (100Hz), 2 is divide by 2 (50Hz), 3 is 33Hz, etc.
     */
    void setFxSpeed(int speed);
    /**
     * @brief setName sets the universe name
     * @param name the name to set
     */
    void setName(const QString &name);
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
     * @brief startSending - starts sending for the selected universe
     */
    void startSending();
    /**
     * @brief stopSending - stops sending for the selected universe
     */
    void stopSending();
    bool isSending() const { return m_isSending;};
signals:
    void fxLevelChanged(int level);
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
#define PREVIEW_DATA_OPTION 0x40
#define STREAM_TERMINATED_OPTION 0x80

class CStreamServer : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief getInstance returns the instance of CStreamServer.
     * @return Returns the one instance of the CStreamServer
     */
    static CStreamServer *getInstance();


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
                              uint send_intervalms = SEND_INTERVAL_DMX);

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

    QUdpSocket * m_sendsock;  //The actual socket used for sending
    QTimer *m_tickTimer;
    QThread *m_thread;


    //Each universe shares its sequence numbers across start codes.
    //This is the central storage location, along with a refcount
    typedef std::pair<int, uint1*> seqref;
    std::map<uint2, seqref > m_seqmap;
    typedef std::map<uint2, seqref >::iterator seqiter;

    //Returns a pointer to the storage location for the universe, adding if need be.
    //The newly-added location contains sequence number 0.
    uint1* GetPSeq(uint2 universe);

    //Removes a reference to the storage location for the universe, removing completely if need be.
    void RemovePSeq(uint2 universe);

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
        uint1* pseq;				//The storage location of the universe sequence number
        QHostAddress sendaddr;      //The multicast address we're sending to

        //and the constructor
      universe():number(0),handle(0), num_terminates(0), psend(NULL),isdirty(false),waited_for_dirty(false),inactive_count(0),pseq(NULL) {}
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


class sACNSender
{
public:
    static sACNSender getInstance();

    sACNSentUniverse *createUniverse(uint2 universe);

private:
    static sACNSender *m_instance;
    sACNSender();
};

#endif // SACNSENDER_H
