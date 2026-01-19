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

// StreamSrv.cpp : This is the main test application.  It shows how to build
// in the necessary files and use the CStreamServer class.
// It doesn't use the full libraries as built for ACN, but it does include
// and build in some of the files that are normally associated with the libraries

#include "streamcommon.h"
#include "streamserver.h"

#include <conio.h>
#include <process.h>
#include <tchar.h>
#include <windows.h>
#include <ws2tcpip.h>

int _tmain(int /*argc*/, _TCHAR * /*argv*/[])
{
    /*Start up*/
    Tock_StartLib();
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //Set up the CStreamServer
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    int ttl = 20;
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)(&ttl), sizeof(ttl));

    CStreamServer srv(s);
    CID mycid = CID::StringToCID("{67F9D986-544E-4abb-8986-D5F79382586C}");

    /*This test will start a source on the first two universes, streaming changing values,
	  and universe 3 will just be doing keepalives (e.g. no data change)*/
    uint handle1, handle2, handle3, handle4;
    uint1 *puniv1, *puniv2, *puniv3, *puniv4; //universe 4 is actually a PRIORITY startcode

    srv.CreateUniverse(mycid, "Source 1", 0, 0, 0, STARTCODE_DMX, 1, 512, puniv1, handle1);
    srv.CreateUniverse(mycid, "Source 2", 105, 0, 0, STARTCODE_DMX, 2, 512, puniv2, handle2);
    //We want source 3 to have a UTF8 name for testing
    WCHAR * wname = L"Söúrçê 3";
    char name[100];
    WideCharToMultiByte(CP_UTF8, 0, wname, -1, name, 100, NULL, NULL);
    srv.CreateUniverse(mycid, name, 0, 0, 0, STARTCODE_DMX, 3, 512, puniv3, handle3);
    srv.CreateUniverse(mycid, "Source 1", 0, 0, 0, STARTCODE_PRIORITY, 1, 512, puniv4, handle4, true, 1000);

    /*The actual test*/
    memset(puniv3, 128, 512);
    srv.SetUniverseDirty(handle3); //So we start sending out on universe 3.
    memset(puniv4, 123, 512);
    srv.SetUniverseDirty(handle4); //Force out an initial packet before waiting send_interval
    uint1 val = 0;
    ttimer tick(22);
    printf("Streaming Data on universes 1 and 2, and static look on 3.\n\
  Press 'x' to fake an old packet on universe 1,\n\
        'y' to force a send on universe 3 -- does not affect inactivity,\n\
        'z' to fake 22 old packets on universe 1.\n\
        'a' to stop sending 0xdd packets on universe 1.\n\
        'b' to start sending 0xdd packets on universe 1.\n\
        'c' to stop sending 0x00 packets on universe 1.\n\
        'd' to start sending 0x00 packets on universe 1.\n\
        'e' to set the preview data option to blind on universe 2.\n\
        'f' to set the preview data option to live on universe 2.\n\
        'g' to change the static data on universe 3 by one -- resets inactivity.\n\
  Press any other key to stop\n");
    bool doing_dd = true;
    bool doing_00 = true;
    bool done = false;
    while (!done)
    {
        int hit = _kbhit();
        if (hit == 0)
        {
            if (tick.Expired())
            {
                tick.Reset();

                //The first universe is a straight value
                memset(puniv1, val, 512);
                srv.SetUniverseDirty(handle1);

                //The second universe just shifts
                for (int i = 0; i < 512; ++i) puniv2[i] = (uint1)(val + i);
                srv.SetUniverseDirty(handle2);

                ++val;
                srv.Tick();
            }
        }
        else
        {
            int hitval = _getch();
            if (hitval == 'x')
                srv.DEBUG_DROP_PACKET(handle1, 1);
            else if (hitval == 'z')
                srv.DEBUG_DROP_PACKET(handle1, 22);
            else if (hitval == 'y')
                srv.SendUniverseNow(handle3);
            else if (hitval == 'a')
            {
                if (doing_dd)
                {
                    doing_dd = false;
                    srv.DEBUG_DESTROY_PRIORITY_UNIVERSE(handle4);
                }
            }
            else if (hitval == 'c')
            {
                if (doing_00)
                {
                    doing_00 = false;
                    srv.DestroyUniverse(handle1);
                    puniv1 = new uint1[513];
                }
            }
            else if (hitval == 'b')
            {

                if (!doing_dd)
                {
                    doing_dd = true;
                    srv.CreateUniverse(
                        mycid,
                        "Source 1",
                        0,
                        0,
                        0,
                        STARTCODE_PRIORITY,
                        1,
                        512,
                        puniv4,
                        handle4,
                        true,
                        1000);
                    srv.SetUniverseDirty(handle4); //So we start sending
                }
            }
            else if (hitval == 'd')
            {
                if (!doing_00)
                {
                    doing_00 = true;
                    srv.CreateUniverse(mycid, "Source 1", 100, 0, 0, STARTCODE_DMX, 1, 512, puniv1, handle1);
                }
            }
            else if (hitval == 'e')
            {
                srv.OptionsPreviewData(handle2, true);
            }
            else if (hitval == 'f')
            {
                srv.OptionsPreviewData(handle2, false);
            }
            else if (hitval == 'g')
            {
                ++puniv3[0];
                srv.SetUniverseDirty(handle3);
            }

            else
                done = true;
        }

        Sleep(5);
    }

    /*Clean up*/

    srv.DestroyUniverse(handle1);
    srv.DestroyUniverse(handle2);
    srv.DestroyUniverse(handle3);

    closesocket(s);

    WSACleanup();
    Tock_StopLib();
    return 0;
}
