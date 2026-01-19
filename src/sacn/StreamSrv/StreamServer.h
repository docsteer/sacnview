/***************************************************************************/
/* Copyright (C) 2006 Electronic Theatre Controls, Inc.                    */
/* All rights reserved.                                                    */
/*                                                                         */
/* This software is provided by Electronic Theatre Controls, Inc. ("ETC")  */
/* at no charge to the user to facilitate the use and implementation of    */
/* ANSI BSR E1.31-2006 BSR E1.31, Entertainment Technology - DMX512-A      */
/* Streaming Protocol (DSP).                                               */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */
/*                                                                         */
/*      Redistributions of source code, whether in binary form or          */
/*      otherwise must retain the above copyright notice, a list of        */
/*      modifications made, if any, a description of modifications made,   */
/*      this list of conditions and the following disclaimer.              */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED "AS IS" BY THE COPYRIGHT HOLDER AND WITHOUT   */
/* WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT     */
/* LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,            */
/* MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.  THE     */
/* ENTIRE RISK AS TO THE QUALITY, OPERATION, AND PERFORMANCE OF THE        */
/* SOFTWARE IS WITH THE USER.                                              */
/*                                                                         */
/* UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY, WHETHER TORT          */
/* (INCLUDING NEGLIGENCE), CONTRACT OR OTHERWISE, SHALL ETC BE LIABLE TO   */
/* ANY PERSON OR ENTITY FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR          */
/* CONSEQUENTIAL DAMAGES OF ANY KIND INCLUDING WITHOUT LIMITATION, DAMAGES */
/* FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR MALFUNCTION,   */
/* OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF SUCH         */
/* PARTY SHALL HAVE BEEN INFORMED OF THE POSSIBILITY OF SUCH DAMAGES.      */
/* THIS LIMITATION OF LIABILITY SHALL NOT APPLY (I) TO DEATH OR PERSONAL   */
/* INJURY RESULTING FROM SUCH PARTY'S NEGLIGENCE TO THE EXTENT APPLICABLE  */
/* LAW PROHIBITS SUCH LIMITATION, (II) FOR CONSUMERS WITH RESIDENCE IN A   */
/* MEMBER COUNTRY OF THE EUROPEAN UNION (A) TO DEATH OR PERSONAL INJURY    */
/* RESULTING FROM ETC'S NEGLIGENCE, (B) TO THE VIOLATION OF ESSENTIAL      */
/* CONTRACTUAL DUTIES BY ETC, AND (C) TO DAMAGES RESULTING FROM            */
/* ETC'S GROSS NEGLIGENT OR WILLFUL BEHAVIOR, SOME JURISDICTIONS DO NOT    */
/* ALLOW THE EXCLUSION OR LIMITATION OF INCIDENTAL OR CONSEQUENTIAL        */
/* DAMAGES, SO THIS EXCLUSION AND LIMITATION MAY NOT APPLY TO YOU.         */
/*                                                                         */
/* Neither the name of ETC, its affiliates, nor the names of its           */
/* contributors may be used to endorse or promote products derived from    */
/* this software without express prior written permission of the President */
/* of ETC.                                                                 */
/*                                                                         */
/* The user of this software agrees to indemnify and hold ETC, its         */
/* affiliates, and contributors harmless from and against any and all      */
/* claims, damages, and suits that are in any way connected to or arising  */
/* out of, either in whole or in part, this software or the user's use of  */
/* this software.                                                          */
/*                                                                         */
/* Except for the limited license granted herein, all right, title and     */
/* interest to and in the Software remains forever with ETC.               */
/***************************************************************************/

/*StreamServer.h  Definition of the CStreamServer class.
This class acts as a streaming ACN server.  It allows you to instance universes,
and will automatically send the buffer for a universe periodically, based on a 
call to Tick

Normal usage:
-For each universe you want to source:
   - If you are sourcing data with per-channel-priorities, call
     CreateUniverse with a startcode of STARTCODE_PRIORITY, ignore_inactivity_logic
     set to IGNORE_INACTIVE_PRIORITY, and send_intervalms set to SEND_INTERVAL_PRIORITY.
     You will receive a data buffer and universe handle that you can use to manipulate
     the per-channel priorities.  0 is not a valid universe number
   - Call CreateUniverse with a startcode of STARTCODE_DMX.  You will receive a data buffer
     and handle that you can use to manipulate the data you are sourcing.
-call Tick at your DMX rate (e.g. every 23 ms) to drive the sending rate.
-Whenever you change data in the buffer, set the universe dirty.  This includes the initial
 setting of the values in a static look!  This class won't send out a universe until it 
 is marked dirty for the first time.
-When you're done with a universe, destroy it.  Keep calling Tick until it returns a 0,
 otherwise you won't send out all the recommended "terminated" packets for that universe
*/

#ifndef _STREAMSERVER_H_
#define _STREAMSERVER_H_

#include "streamcommon.h"
#include "tock.h"
#include <map>
#include <vector>
#include <winsock2.h>

//These definitions are to be used with the ignore_inactivity_logic field of CreateUniverse
#define IGNORE_INACTIVE_DMX false
#define IGNORE_INACTIVE_PRIORITY false /*Any priority change should send three packets anyway, around your frame rate*/

//These definitions are to be used with the send_intervalms parameter of CreateUniverse
#define SEND_INTERVAL_DMX 850 /*If no data has been sent in 850ms, send another DMX packet*/
#define SEND_INTERVAL_PRIORITY 1000 /*By default, per-channel priority packets are sent once per second*/

//Bitflags for the options parameter of Create Universe.
//Alternatively, you can directly set them while a universe is running with
//OptionsPreviewData and OptionsStreamTerminated.  The terminated option doesn't
//really need to be used, as DestroyUniverse handles that functionality for you.
#define PREVIEW_DATA_OPTION 0x40
#define STREAM_TERMINATED_OPTION 0x80

class CStreamServer
{
public:

    CStreamServer(SOCKET sendsock);
    virtual ~CStreamServer(void);

    //Must be called at your DMX rate -- usually 22 or 23 ms
    //Yes, MUST be called at your DMX rate.  This function processes the inactivity timers,
    // and calling this function at a slower rate may cause an inactivity timer to be triggered at an
    // time point that is past the universe transmission timeout, causing your sinks to consider
    // you offline.  The absolute minumum rate that this function can be called is 10hz (every 100ms).
    // Sends out any Dirty universes, universes that have hit their send_interval,
    // and depending on how you created each universe performs the DMX inactivity logic.
    //This function returns the current number of valid universes in the system.
    //This function also handles sending the extra packets with the terminated flag set, and destroys
    // the universe for you, so call this function for at least a few more cycles after you
    // call DestroyUniverse (or until tick returns 0 if you know you aren't creating any more universes)
    //NOT THREAD SAFE
    int Tick();

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
    //Also: If you want to change any of these parameters, you can call CreateUniverse again with the
    //      same start_code and universe.  It will destroy and reallocate pslots, however.
    //NOT THREAD SAFE
    bool CreateUniverse(
        const CID & source_cid,
        const char * source_name,
        uint1 priority,
        uint2 reserved,
        uint1 options,
        uint1 start_code,
        uint2 universe,
        uint2 slot_count,
        uint1 *& pslots,
        uint & handle,
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

private:

    SOCKET m_sendsock; //The actual socket used for sending

    //Each universe shares its sequence numbers across start codes.
    //This is the central storage location, along with a refcount
    typedef std::pair<int, uint1 *> seqref;
    std::map<uint2, seqref> m_seqmap;
    typedef std::map<uint2, seqref>::iterator seqiter;

    //Returns a pointer to the storage location for the universe, adding if need be.
    //The newly-added location contains sequence number 0.
    uint1 * GetPSeq(uint2 universe);

    //Removes a reference to the storage location for the universe, removing completely if need be.
    void RemovePSeq(uint2 universe);

    //Each universe is just the full buffer and some state
    struct universe
    {
        uint2 number; //The universe number
        uint1 start_code; //The start code
        uint handle; //The handle.  This is needed to help deletions.
        uint1 num_terminates; //The number of consecutive times the
        //stream_terminated option flag has been set.
        uint1 * psend; //The full sending buffer, which the user can access the data portion.
        //If NULL, this is not an active universe (just a hole in the vector)
        uint sendsize;
        bool isdirty;
        bool waited_for_dirty; //Until we receive a dirty flag, we don't start outputting the universe.
        bool ignore_inactivity; //If true, we don't bother looking at inactive_count
        uint inactive_count; //After 3 of these, we start sending at send_interval
        ttimer send_interval; //Whether or not it's time to send a non-dirty packet
        uint1 * pseq; //The storage location of the universe sequence number
        struct sockaddr_in sendaddr; //The multicast address we're sending to

        //and the constructor
        universe()
            : number(0)
            , handle(0)
            , num_terminates(0)
            , psend(NULL)
            , isdirty(false)
            , waited_for_dirty(false)
            , pseq(NULL)
            , inactive_count(0)
        {}
    };

    //The handle is the vector index
    std::vector<universe> m_multiverse;
    typedef std::vector<universe>::iterator verseiter;

    //Perform the logical destruction and cleanup of a universe
    //and its related objects.
    void DoDestruction(uint handle);
};

#endif
