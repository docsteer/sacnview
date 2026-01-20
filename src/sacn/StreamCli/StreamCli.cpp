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

// StreamCli.cpp : Defines the entry point for the console application.
// 90% of the client side functionality is in this class, instead of a "library"
// class that could be moved between applications.  The main reason for this is
// that, unlike the sending side, the needs of the client side are very variable
// as to its capabilities (e.g. tracking/supporting sources, universes, etc., or
// doing htp/ltp/etc.)  Therefore, this app shows the basics of being a client,
// and leaves the actual usage up to the user.
//

#include <map>
#include <process.h>
#include <tchar.h>
#include <vector>

#include "defpack.h"
#include "streamcommon.h"
#include "tock.h"
#include <conio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

//The amount of ms to wait before a source is considered offline or
//has stopped sending per-channel-priority packets
#define WAIT_OFFLINE 2500

//The amount of ms to wait before determining that a newly discovered source is not doing per-channel-priority
#define WAIT_PRIORITY 1500

//The time during which to sample
#define SAMPLE_TIME 1500

//The maximum time, in milliseconds, to allow the user to hold the last
//look.
#define MAX_HOLD_LAST_LOOK_TIME 65535000

//A VERY simplistic tracker of each source.  We are not tracking source priority for this test app
struct source
{
    CID src_cid;
    bool
        src_valid; //We're ignoring thread issues, so to reuse a slot in the source array we'll flag it valid or invalid
    uint1 lastseq;
    ttimer active; //If this expires, we haven't received any data in over a second
    //The per-channel priority alternate start code policy requires we detect the source only after
    //a STARTCODE_PRIORITY packet was received or 1.5 seconds have expired
    bool waited_for_dd;
    bool doing_dmx; //if true, we are processing dmx data from this source
    bool doing_per_channel; //If true, we are tracking per-channel priority messages for this source
    ttimer priority_wait; //if !initially_notified, used to track if a source is finally detected
    //(either by receiving priority or timeout).  If doing_per_channel,
    //used to time out the 0xdd packets to see if we lost per-channel priority
};

//Universal Hold Last Look Time
//user-configurable time to hold last look from a source that has
//dropped off line, either after the 2.5s timeout or when the source
//gracefully leaves. This may be used as a system wide setting to allow
//backups to have more time to take over from their masters.
//The default time for this is 1s.
int hll = 1000;

/* ***
A sampling period exists in the API of this library.  
The sampling period is used to ensure that all active sources are seen before making
a decision about which source or sources win control in our prioritized HTP scheme.  
Failure to use the SamplingStarted and SamplingEnded notifications means that you will 
sometimes see and act on a lower priority source before a higher priority source as you
begin listening to network traffic.  This failure in control can result in lights flickering
or at worst has caused lamps to be blown in some fixtures that inadvertently were re-struck 
too quickly.  Therefore:

   - On subscription to a new universe, the library issues a SamplingStarted notification.
	  The library will start a 1.5 second sampling period timer for that universe.  
	  After the timer ends, the library will issue a SamplingEnded notification.
	  During this sample period, all packets will be forwarded to the application without
	  waiting to determine if that source is a per-channel-priority source (receiving a
	  packet with a startcode of 0xdd or waiting 1.5 seconds).  After this sample period,
	  the library goes back to waiting for the 0xdd or 1.5 seconds before forwarding data
	  packets from the source.

	- To take care of potential flicker due seeing the wrong source first,
	  do as follows:
	  Buffer the latest UniverseData (level and priority data) of each source that appears in the
	  sample period. When that period has ended, process (HTP, etc) the stored data as a group
	  to obtain the correct control levels (that are then used directly or converted to DMX)
 
     Note: Because your application may see level data before priority for that data it is 
	  essential to avoid executing merge algorithms until the end of the sample period.

While this demo marks the beginning and end of the sample period, it DOES NOT
implement buffering the UniverseData and processing it in a block at the end of 
the sample period.
 */
struct universe_sample
{
    bool sampling;
    ttimer sample_timer;
};

std::map<uint2, universe_sample> sample;
typedef std::map<uint2, universe_sample>::iterator sampleiter;

//This could be a LOT more sophisticated.  We'll have a quick vector of sources
//for each universe we're tracking.
//I'm not going to really bother filling holes with new values, since I'm IGNORING THREAD SAFETY
std::vector<struct source> g_univ1;
std::vector<struct source> g_univ2;
std::vector<struct source> g_univ3;

struct readparam
{
    bool terminate;
    SOCKET s;
};

enum listenTo
{
    NOTHING = 0,
    DRAFT = 1,
    SPEC = 2,
    ALL = 3
};

//sets the amount of time in milliseconds we will wait for a backup to
//appear after a source drops off line
//returns the value being set on success and -1 on failure
int setUniversalHoldLastLook(int hold_time)
{
    if (hold_time < 0 || hold_time > MAX_HOLD_LAST_LOOK_TIME)
    {
        return -1;
    }
    return hll = hold_time;
}

//returns the amount of time in milliseconds we will wait for a backup
//to appear after a source drops off line
int getUniversalHoldLastLook()
{
    return hll;
}

//Called by CheckSourceExpiration periodically to determine whether or not we have entered a
//data-loss sampling period.
void CheckSampleExpiration()
{
    for (sampleiter it = sample.begin(); it != sample.end(); ++it)
    {
        if (it->second.sampling && it->second.sample_timer.Expired())
        {
            //stop sampling
            it->second.sampling = false;

            //notify
            printf("\nSampling has ended on universe %d.\n", it->first);
        }
    }
}

void CheckSourceExpiration(std::vector<struct source> & univ, uint2 universe)
{
    char cidstr[CID::CIDSTRINGBYTES];
    for (std::vector<struct source>::iterator it = univ.begin(); it != univ.end(); ++it)
    {
        if (it->src_valid)
        {
            if (it->active.Expired())
            {
                it->src_valid = false;
                CID::CIDIntoString(it->src_cid, cidstr);
                printf("\nUniverse %d lost source %s\n", universe, cidstr);
            }
            else if (it->doing_per_channel && it->priority_wait.Expired())
            {
                CID::CIDIntoString(it->src_cid, cidstr);
                it->doing_per_channel = false;
                printf("\nSource %s stopped sending per-channel priority on universe %d\n", cidstr, universe);
            }
        }
    }
}

//We'll just show a display per universe
char g_disp[4] = {'\\', '|', '/', '-'};
bool preview[3] = {false, false, false};
char g_dispPrev[4] = {'P', 'P', 'p', 'p'};
uint1 g_show1 = 0;
uint1 g_show2 = 0;
uint1 g_show3 = 0;
uint1 g_showpri = 0;
uint1 g_0 = 0;
uint1 g_1 = 0;
uint1 g_2 = 0;
uint1 g_3 = 0;
listenTo version = ALL;

unsigned __stdcall LostSourceThread(void * p)
{
    struct readparam * pread = (struct readparam *)(p);

    while (!pread->terminate)
    {
        CheckSourceExpiration(g_univ1, 1);
        CheckSourceExpiration(g_univ2, 2);
        CheckSourceExpiration(g_univ3, 3);
        CheckSampleExpiration();

        //We'll do a hack and print the status as well -- I didn't want to stall the read thread with IO
        printf(
            "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bu1"
            ":%c u2:%c u3:%c, u1(4): 0x%02x 0x%02x 0x%02x 0x%02x, u1pri:%c",
            preview[0] ? g_dispPrev[g_show1 % 4] : g_disp[g_show1 % 4],
            preview[1] ? g_dispPrev[g_show2 % 4] : g_disp[g_show2 % 4],
            preview[2] ? g_dispPrev[g_show3 % 4] : g_disp[g_show3 % 4],
            g_0,
            g_1,
            g_2,
            g_3,
            g_disp[g_showpri % 4]);
        Sleep(20);
    }

    return 0;
}

unsigned __stdcall ReadThread(void * p)
{
    struct readparam * pread = (struct readparam *)(p);
    struct sockaddr_in from; //A slight hack, but we're only doing IPV4
    int fromlen;

    char * pbuf = new char[1500]; //MTU-sized buffer
    if (!pbuf) return 1;

    while (!pread->terminate)
    {
        //recvfrom will block for us.  numread may be 0 even if the socket isn't closed
        fromlen = sizeof(sockaddr_in);
        int numread = recvfrom(pread->s, pbuf, 1500, 0, (sockaddr *)&from, &fromlen);
        if (numread && !pread->terminate)
        {
            //Process packet
            CID source_cid;
            uint1 start_code;
            uint1 sequence;
            uint2 universe;
            uint2 slot_count;
            uint1 * pdata;
            std::vector<struct source> * psrces;
            char source_name[SOURCE_NAME_SIZE];
            uint1 priority;
            //These only apply to the ratified version of the spec, so we will hardwire
            //them to be 0 just in case they never get set.
            uint2 reserved = 0;
            uint1 options = 0;

            if (!ValidateStreamHeader(
                    (uint1 *)pbuf,
                    numread,
                    source_cid,
                    source_name,
                    priority,
                    start_code,
                    reserved,
                    sequence,
                    options,
                    universe,
                    slot_count,
                    pdata))
                continue;

            //Unpacks a uint4 from a known big endian buffer
            int root_vect = UpackB4((uint1 *)pbuf + ROOT_VECTOR_ADDR);

            //We're not going to deal with this if we're not supposed to
            //If we only listen to the officially specced version, and this is
            //carrying the vector for the draft, then we drop it on the floor.
            //If we are only listening to the draft version, and this is carrying
            //the vector for the official spec, we will stomp on it.
            //If we are listening to all of them, then we continue to listen.
            if ((root_vect == ROOT_VECTOR && version == DRAFT)
                || (root_vect == DRAFT_ROOT_VECTOR && version == SPEC)
                || version == NOTHING)
            {
                continue;
            }

            //We are only reading universe 1,2,3
            if ((universe < 1) || (universe > 3)) continue;

            preview[universe - 1] = (0x80 == (options & 0x80));

            //Validate the source, adding the new one if necessary
            if (universe == 1)
                psrces = &g_univ1;
            else if (universe == 2)
                psrces = &g_univ2;
            else
                psrces = &g_univ3;

            bool foundsource = false;
            bool newsourcenotify = false;
            bool validpacket = true; //whether or not we will actually process the packet
            bool is_sampling = sample[universe].sampling;

            for (std::vector<struct source>::iterator it = psrces->begin(); it != psrces->end(); ++it)
            {
                if (it->src_valid && (it->src_cid == source_cid))
                {
                    foundsource = true;

                    if ((root_vect == ROOT_VECTOR) && ((options & 0x40) == 0x40))
                    {
                        //by setting this flag to false, 0xdd packets that may come in while the terminated data
                        //packets come in won't reset the priority_wait timer
                        it->waited_for_dd = false;
                        if (start_code == STARTCODE_DMX) it->doing_dmx = false;

                        //"Upon receipt of a packet containing this bit set
                        //to a value of 1, a receiver shall enter network
                        //data loss condition.  Any property values in
                        //these packets shall be ignored"
                        it->active.SetInterval(hll); //We factor in the hold last look time here, rather than 0

                        if (it->doing_per_channel)
                            it->priority_wait.SetInterval(
                                hll); //We factor in the hold last look time here, rather than 0

                        validpacket = false;
                        break;
                    }

                    //Based on the start code, update the timers
                    if (start_code == STARTCODE_DMX)
                    {
                        //No matter how valid, we got something -- but we'll tweak the interval for any hll change
                        it->doing_dmx = true;
                        it->active.SetInterval(WAIT_OFFLINE + hll);
                    }
                    else if (start_code == STARTCODE_PRIORITY && it->waited_for_dd)
                    {
                        it->doing_per_channel = true; //The source could have stopped sending dd for a while.
                        it->priority_wait.Reset();
                    }

                    //Validate the sequence number, updating the stored one
                    //The two's complement math is to handle rollover, and we're explicitly
                    //doing assignment to force the type sizes.  A negative number means
                    //we got an "old" one, but we assume that anything really old is possibly
                    //due the device having rebooted and starting the sequence over.
                    int1 result = ((int1)sequence) - ((int1)(it->lastseq));
                    if ((result <= 0) && (result > -20))
                        validpacket = false;
                    else
                        it->lastseq = sequence;

                    //This next bit is a little tricky.  We want to wait for dd packets (sampling period
                    //tweaks aside) and notify them with the dd packet first, but we don't want to do that
                    //if we've never seen a dmx packet from the source.
                    if (!it->doing_dmx)
                    {
                        validpacket = false;
                        it->priority_wait.Reset(); //We don't want to let the priority timer run out
                    }
                    else if (!it->waited_for_dd && validpacket)
                    {
                        if (start_code == STARTCODE_PRIORITY)
                        {
                            it->waited_for_dd = true;
                            it->doing_per_channel = true;
                            it->priority_wait.SetInterval(WAIT_OFFLINE + hll);
                            newsourcenotify = true;
                        }
                        else if (it->priority_wait.Expired())
                        {
                            it->waited_for_dd = true;
                            it->doing_per_channel = false;
                            it->priority_wait.SetInterval(
                                WAIT_OFFLINE + hll); //In case the source later decides to sent 0xdd packets
                            newsourcenotify = true;
                        }
                        else
                            newsourcenotify = validpacket = false;
                    }

                    //Found the source, and we're ready to process the packet
                    break;
                }
            }

            if (!validpacket)
            {
                continue;
            }

            if (!foundsource) //Add our own source
            {
                struct source s, *ps;

                bool found_gap = false;
                for (std::vector<struct source>::iterator it = psrces->begin(); it != psrces->end(); ++it)
                {
                    if (!it->src_valid)
                    {
                        found_gap = true;
                        ps = &(*it);
                        break;
                    }
                }

                if (!found_gap)
                {
                    psrces->push_back(s);
                    ps = &((*psrces)[psrces->size() - 1]);
                }

                ps->active.SetInterval(WAIT_OFFLINE + hll);
                ps->lastseq = sequence;
                ps->src_cid = source_cid;
                ps->src_valid = true;
                ps->doing_dmx = (start_code == STARTCODE_DMX);
                //If we are in the sampling period, let all packets through
                if (is_sampling)
                {
                    ps->waited_for_dd = true;
                    ps->doing_per_channel = (start_code == STARTCODE_PRIORITY);
                    newsourcenotify = true;
                    ps->priority_wait.SetInterval(WAIT_OFFLINE + hll);
                }
                else
                {
                    //If we aren't sampling, we want the earlier logic to set the state
                    ps->doing_per_channel = ps->waited_for_dd = false;
                    newsourcenotify = false;
                    ps->priority_wait.SetInterval(WAIT_PRIORITY);
                }

                validpacket = newsourcenotify;
            }

            if (newsourcenotify)
            {
                char cidstr[CID::CIDSTRINGBYTES];
                CID::CIDIntoString(source_cid, cidstr);
                //Source name is utf8 -- convert to wide string
                WCHAR wname[200];
                MultiByteToWideChar(CP_UTF8, 0, source_name, -1, wname, 200);
                //convert to dos prompt characters
                char dname[200];
                WideCharToMultiByte(CP_OEMCP, 0, wname, -1, dname, 200, NULL, NULL);
                printf(
                    "\nUniverse %d found priority %d source %s (%s), start code 0x%x\n",
                    universe,
                    priority,
                    dname,
                    cidstr,
                    start_code);
            }

            //Finally, Process the buffer
            if (validpacket)
            {
                if (universe == 1)
                {
                    if (start_code == STARTCODE_DMX)
                    {
                        g_0 = pdata[0];
                        g_1 = pdata[1];
                        g_2 = pdata[2];
                        g_3 = pdata[3];
                        ++g_show1;
                    }
                    else if (start_code == STARTCODE_PRIORITY)
                        ++g_showpri;
                }
                else if ((universe == 2) && (start_code == STARTCODE_DMX))
                    ++g_show2;
                else if ((universe == 3) && (start_code == STARTCODE_DMX))
                    ++g_show3;
            }
        }
        else if (numread < 0)
            break;
    }

    if (pbuf) delete[] pbuf;
    return 0;
}

void GetMultiAddr(uint2 universe, struct sockaddr_in & addr)
{
    CIPAddr caddr;
    GetUniverseAddress(universe, caddr);

    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(caddr.GetIPPort());
    addr.sin_addr.s_addr = htonl(caddr.GetV4Address());
}

int _tmain(int /*argc*/, _TCHAR * /*argv*/[])
{
    bool not_done = true;
    char * listenMap[] = {"NOTHING", "DRAFT", "SPEC", "ALL"};
    printf("Listening for startcode 0 data on universes 1, 2, and 3.\n  Press any key to stop\n");

    /*Start up*/
    Tock_StartLib();
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //Set up the socket to listen on all three multicast addresses
    //Note that this method is fine for most systems, but if you are using
    //a lot of sACN and ACN(SDT/DMP), you may want to use separate sockets
    //explicitly bound to the multicast address on platforms that don't
    //track/filter what subscriptions a socket has (like Linux).
    //This is because sACN and ACN use the same UDP port for communications.
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in bindaddr;
    memset(&bindaddr, 0, sizeof(sockaddr_in));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(STREAM_IP_PORT);
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(s, (sockaddr *)&bindaddr, sizeof(sockaddr_in));
    //I'm ignoring multiple NIC issues here
    struct ip_mreq mreq;
    sockaddr_in subscribeaddr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    GetMultiAddr(1, subscribeaddr);
    mreq.imr_multiaddr.s_addr = subscribeaddr.sin_addr.s_addr; //Already network order
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    printf("\nSampling started on universe %d.\n", 1);
    sample[1].sample_timer.SetInterval(SAMPLE_TIME);

    GetMultiAddr(2, subscribeaddr);
    mreq.imr_multiaddr.s_addr = subscribeaddr.sin_addr.s_addr; //Already network order
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    printf("\nSampling started on universe %d.\n", 2);
    sample[2].sample_timer.SetInterval(SAMPLE_TIME);
    GetMultiAddr(3, subscribeaddr);
    mreq.imr_multiaddr.s_addr = subscribeaddr.sin_addr.s_addr; //Already network order
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));

    printf("\nSampling started on universe %d.\n", 3);
    sample[3].sample_timer.SetInterval(SAMPLE_TIME);

    /*Now the thread stuff*/
    struct readparam readp;
    readp.s = s;
    readp.terminate = false;

    uint hsource, hread;
    _beginthreadex(NULL, 0, LostSourceThread, &readp, 0, &hsource);
    _beginthreadex(NULL, 0, ReadThread, &readp, 0, &hread);

    /*The actual test*/
    printf(
        "\nHold last look is %d ms.  We are currently listening to %s\n\
  Press 'l' to toggle listening to the official spec to ON.\n\
        'm' to toggle listening to the offfical spec to OFF.\n\
        'n' to toggle listening to the draft spec to ON.\n\
        'o' to toggle listening to the draft spec to OFF.\n\
        'j' to add one second to hold last look time.\n\
        'k' to subtract one second from hold last look time.\n\
  Press other any key to stop\n\n",
        getUniversalHoldLastLook(),
        listenMap[version]);

    while (not_done)
    {
        if (_kbhit() == 0)
        {
            Sleep(20);
        }
        else
        {
            int choice = _getch();

            if (choice == 'l')
            {
                if (version == DRAFT)
                {
                    version = ALL;
                }
                else if (version == NOTHING)
                {
                    version = SPEC;
                }
            }
            else if (choice == 'm')
            {
                if (version == ALL)
                {
                    version = DRAFT;
                }
                else if (version == SPEC)
                {
                    version = NOTHING;
                }
            }
            else if (choice == 'n')
            {
                if (version == SPEC)
                {
                    version = ALL;
                }
                else if (version == NOTHING)
                {
                    version = DRAFT;
                }
            }
            else if (choice == 'o')
            {
                if (version == ALL)
                {
                    version = SPEC;
                }
                else if (version == DRAFT)
                {
                    version = NOTHING;
                }
            }
            else if (choice == 'j')
            {
                int current_hold_last_look_time = getUniversalHoldLastLook();
                setUniversalHoldLastLook(current_hold_last_look_time + 1000);
            }
            else if (choice == 'k')
            {
                int current_hold_last_look_time = getUniversalHoldLastLook();
                setUniversalHoldLastLook(current_hold_last_look_time - 1000);
            }
            else
            {
                not_done = false;
            }

            printf(
                "\nHold last look is %d ms.  We are currently listening to %s\n\
  Press 'l' to toggle listening to the official spec to ON.\n\
        'm' to toggle listening to the offfical spec to OFF.\n\
        'n' to toggle listening to the draft spec to ON.\n\
        'o' to toggle listening to the draft spec to OFF.\n\
        'j' to add one second to hold last look time.\n\
        'k' to subtract one second from hold last look time.\n\
  Press other any key to stop\n\n",
                getUniversalHoldLastLook(),
                listenMap[version]);
        }
    }

    /*Clean up -- setting terminate and closing the socket should trigger the threads to expire*/

    readp.terminate = true;

    GetMultiAddr(1, subscribeaddr);
    mreq.imr_multiaddr.s_addr = subscribeaddr.sin_addr.s_addr; //Already network order
    setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    GetMultiAddr(2, subscribeaddr);
    mreq.imr_multiaddr.s_addr = subscribeaddr.sin_addr.s_addr; //Already network order
    setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    GetMultiAddr(3, subscribeaddr);
    mreq.imr_multiaddr.s_addr = subscribeaddr.sin_addr.s_addr; //Already network order
    setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    closesocket(s);

    if (WaitForSingleObject((HANDLE)hsource, 500) == WAIT_TIMEOUT) _endthreadex(hsource);
    if (WaitForSingleObject((HANDLE)hread, 500) == WAIT_TIMEOUT) _endthreadex(hread);

    WSACleanup();
    Tock_StopLib();
    return 0;
}
