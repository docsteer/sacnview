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

#include <vector>

#include "cid.h"
#include "deftypes.h"
#include "ipaddr.h"
#include "streamcommon.h"
#include "streamserver.h"
#include "tock.h"

CStreamServer::CStreamServer(SOCKET sendsock)
    : m_sendsock(sendsock)
{}

CStreamServer::~CStreamServer()
{
    //Clean up the sequence numbers
    for (seqiter it3 = m_seqmap.begin(); it3 != m_seqmap.end(); ++it3)
        if (it3->second.second) delete it3->second.second;
}

//Returns a pointer to the storage location for the universe, adding if
//need be.
//The newly-added location contains sequence number 0.
uint1 * CStreamServer::GetPSeq(uint2 universe)
{
    seqiter it = m_seqmap.find(universe);
    if (it != m_seqmap.end())
    {
        ++it->second.first;
        return it->second.second;
    }
    uint1 * p = new uint1;
    if (!p) return NULL;
    *p = 0;
    m_seqmap.insert(std::pair<uint2, seqref>(universe, seqref(1, p)));
    return p;
}

//Removes a reference to the storage location for the universe, removing
//completely if need be.
void CStreamServer::RemovePSeq(uint2 universe)
{
    seqiter it = m_seqmap.find(universe);
    if (it != m_seqmap.end())
    {
        --it->second.first;
        if (it->second.first <= 0)
        {
            delete it->second.second;
            m_seqmap.erase(it);
        }
    }
}

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
int CStreamServer::Tick()
{
    int valid_count = 0;
    for (verseiter it = m_multiverse.begin(); it != m_multiverse.end(); ++it)
    {
        if (it->psend) ++valid_count;

        //If this has been send 3 times (or more?) with a termination flag
        //then it's time to kill it
        if (it->num_terminates >= 3)
        {
            DoDestruction(it->handle);
        }

        //If valid, either a dirty, inactivity count < 3 (if we're using that logic), or send_interval will cause a send
        if (it->psend
            && (it->isdirty
                || (it->waited_for_dirty
                    && ((!it->ignore_inactivity && it->inactive_count < 3) || it->send_interval.Expired()))))
        {
            //Before the send, properly reset state
            if (it->isdirty)
                it->inactive_count = 0; //To recover from inactivity
            else if (it->inactive_count < 3) //We don't want the Expired case to reset the inactivity count
                ++it->inactive_count;

            //Add the sequence number and send
            uint1 * pseq = it->pseq;
            if (pseq)
            {
                SetStreamHeaderSequence(it->psend, *pseq);
                ++*pseq;
            }
            else
                SetStreamHeaderSequence(it->psend, 0);

            sendto(m_sendsock, (char *)it->psend, it->sendsize, 0, (sockaddr *)&(it->sendaddr), sizeof(sockaddr_in));

            if (GetStreamTerminated(it->psend))
            {
                it->num_terminates++;
            }

            //Finally, set the timing/dirtiness for the next interval
            it->isdirty = false;
            it->send_interval.Reset();
        }
    }

    return valid_count;
}

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
bool CStreamServer::CreateUniverse(
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
    bool ignore_inactivity_logic,
    uint send_intervalms)
{
    if (universe == 0) return false;

    //Before we attempt to create the universe, make sure we can create the buffer.
    uint sendsize = STREAM_HEADER_SIZE + slot_count;
    uint1 * pbuf = new uint1[sendsize];
    if (!pbuf) return false;
    memset(pbuf, 0, sendsize);

    //Find an empty spot for the universe
    bool found = false;
    for (uint i = 0; i < m_multiverse.size(); ++i)
    {
        if (m_multiverse[i].number == universe && m_multiverse[i].start_code == start_code)
        {
            found = true;
            handle = i;
            //get rid of the old one first
            DoDestruction(i);
            break;
        }
        if (!found && m_multiverse[i].psend == NULL)
        {
            found = true;
            handle = i;
            //			break;
        }
    }
    if (!found)
    {
        struct universe tmp;
        handle = m_multiverse.size();
        m_multiverse.push_back(tmp);
    }

    //Init/reinit the state
    m_multiverse[handle].number = universe;
    m_multiverse[handle].handle = handle;
    m_multiverse[handle].start_code = start_code;
    m_multiverse[handle].isdirty = false;
    m_multiverse[handle].waited_for_dirty = false;
    m_multiverse[handle].num_terminates = 0;
    m_multiverse[handle].ignore_inactivity = ignore_inactivity_logic;
    m_multiverse[handle].inactive_count = 0;
    m_multiverse[handle].send_interval.SetInterval(send_intervalms);
    m_multiverse[handle].pseq = GetPSeq(universe);

    CIPAddr addr;
    GetUniverseAddress(universe, addr);
    memset(&m_multiverse[handle].sendaddr, 0, sizeof(m_multiverse[handle].sendaddr));
    m_multiverse[handle].sendaddr.sin_family = AF_INET;
    m_multiverse[handle].sendaddr.sin_port = htons(addr.GetIPPort());
    m_multiverse[handle].sendaddr.sin_addr.s_addr = htonl(addr.GetV4Address());

    InitStreamHeader(pbuf, source_cid, source_name, priority, reserved, options, start_code, universe, slot_count);
    m_multiverse[handle].psend = pbuf;
    m_multiverse[handle].sendsize = sendsize;
    pslots = pbuf + STREAM_HEADER_SIZE;
    return true;
}

//After you add data to the data buffer, call this to trigger the data send on
//the next Tick boundary.
//Otherwise, the data won't be sent until the inactivity or send_interval time.
//NOT THREAD SAFE
void CStreamServer::SetUniverseDirty(uint handle)
{
    m_multiverse[handle].isdirty = true;
    m_multiverse[handle].waited_for_dirty = true;
}

//In the event that you want to send out a message for a particular
//universe (and start code) in between ticks, call this function.
//This does not affect the dirty bit for the universe, inactivity count,
//etc, and the tick will still operate normally when called.
//This is not thread safe with Tick -- Don't call when Tick is called
void CStreamServer::SendUniverseNow(uint handle)
{
    //Basically, a copy of the sending part of Tick

    universe * puni = &m_multiverse[handle];
    uint1 * pseq = puni->pseq;
    if (pseq)
    {
        SetStreamHeaderSequence(puni->psend, *pseq);

        ++*pseq;
        //Never use a 0 sequence number after the first time
        if (0 == *pseq) ++*pseq;
    }
    else
        SetStreamHeaderSequence(puni->psend, 0);

    sendto(m_sendsock, (char *)puni->psend, puni->sendsize, 0, (sockaddr *)&(puni->sendaddr), sizeof(sockaddr_in));
}

//Use this to destroy a priority universe.
void CStreamServer::DEBUG_DESTROY_PRIORITY_UNIVERSE(uint handle)
{
    DoDestruction(handle);
}

//Use this to destroy a universe.
//this does invalidate the pslots array that CreateUniverse returned, so do not access
//that memory after or during this call.
//Not Thread Safe -- Don't call when Tick is called
void CStreamServer::DestroyUniverse(uint handle)
{
    SetStreamTerminated(m_multiverse[handle].psend, true);
    /*
	if(m_multiverse[handle].psend)
	{
		delete [] m_multiverse[handle].psend;
		m_multiverse[handle].psend = NULL;

		RemovePSeq(m_multiverse[handle].number);
	}
  */
}

//Perform the logical destruction and cleanup of a universe and its related
//objects.
void CStreamServer::DoDestruction(uint handle)
{
    if (m_multiverse[handle].psend)
    {
        m_multiverse[handle].num_terminates = 0;
        delete[] m_multiverse[handle].psend;
        m_multiverse[handle].psend = NULL;
        //      m_multiverse[handle].wheretosend.clear();

        RemovePSeq(m_multiverse[handle].number);
        m_multiverse[handle].pseq = NULL;
    }
}

/*DEBUG USAGE ONLY --causes packets to be "dropped" on a particular universe*/
void CStreamServer::DEBUG_DROP_PACKET(uint handle, uint1 decrement)
{
    *(m_multiverse[handle].pseq) = *(m_multiverse[handle].pseq) - decrement; //-= causes size problem
}

//sets the preview_data bit of the options field
void CStreamServer::OptionsPreviewData(uint handle, bool preview)
{
    SetPreviewData(m_multiverse[handle].psend, preview);
}

//sets the stream_terminated bit of the options field
void CStreamServer::OptionsStreamTerminated(uint handle, bool terminated)
{
    SetStreamTerminated(m_multiverse[handle].psend, terminated);
}
