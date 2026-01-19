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

// CID.cpp: implementation of the CID class.
//
//////////////////////////////////////////////////////////////////////

#include "CID.h"
#include <stdio.h>
#include <string.h>

#include <QUuid>

////////////////////////////////////////////////////////////////////////////////////////
// Non-static functions
CID::CID()
{
    qRegisterMetaType<CID>("CID");
    memset(m_cid, 0, CIDBYTES);
}

CID::~CID() {}

CID::CID(const quint8 * pCID)
{
    CID();
    memcpy(m_cid, pCID, CIDBYTES);
}

CID::CID(const CID & cid)
{
    CID();
    memcpy(m_cid, cid.m_cid, CIDBYTES);
}

CID & CID::operator=(const CID & cid)
{
    memcpy(m_cid, cid.m_cid, CIDBYTES);
    return (*this);
}

void CID::Pack(quint8 * pbuf) const
{
    memcpy(pbuf, m_cid, CIDBYTES);
}

void CID::Unpack(const quint8 * pbuf)
{
    memcpy(m_cid, pbuf, CIDBYTES);
}

bool operator<(const CID & c1, const CID & c2)
{
    return 0 > memcmp(c1.m_cid, c2.m_cid, CID::CIDBYTES);
}

bool operator==(const CID & c1, const CID & c2)
{
    return 0 == memcmp(c1.m_cid, c2.m_cid, CID::CIDBYTES);
}

bool operator!=(const CID & c1, const CID & c2)
{
    return 0 != memcmp(c1.m_cid, c2.m_cid, CID::CIDBYTES);
}

bool CID::isNull() const
{
    char null_data[CIDBYTES];
    memset(null_data, 0, CIDBYTES);
    int result = memcmp(m_cid, null_data, CIDBYTES);
    if (result == 0) return true;

    return false;
}

DCID::DCID(const quint8 * pDCID)
    : m_cid(pDCID)
{}
DCID::DCID(const DCID & dcid)
    : m_cid(dcid.m_cid)
{}
DCID::DCID()
    : m_cid()
{}
DCID::~DCID() {}
void DCID::Pack(quint8 * pbuf) const
{
    m_cid.Pack(pbuf);
}
void DCID::Unpack(const quint8 * pbuf)
{
    m_cid.Unpack(pbuf);
}
DCID & DCID::operator=(const DCID & dcid)
{
    m_cid = dcid.m_cid;
    return *this;
}
bool operator<(const DCID & d1, const DCID & d2)
{
    return d1.m_cid < d2.m_cid;
}
bool operator==(const DCID & d1, const DCID & d2)
{
    return d1.m_cid == d2.m_cid;
}
bool operator!=(const DCID & d1, const DCID & d2)
{
    return d1.m_cid != d2.m_cid;
}

///////////////////////////////////////////////////////////////
// Static functions
//A quick helper function that puts fills a UUID string
static void UUIDFillString(quint8 * pbuffer, const char * ptext)
{
    //This function assumes a lot of safety issues...
    //We'll at least only put in CIDBYTES bytes
    const char * pt = ptext;
    quint8 * pb = pbuffer;
    bool first = true; //Whether we are doing the first or second nibble of the byte

    while ((*pt != 0) && (pb - pbuffer < CID::CIDBYTES))
    {
        quint8 offset = 0;
        if ((*pt >= 0x30) && (*pt <= 0x39))
            offset = 0x30;
        else if ((*pt >= 0x41) && (*pt <= 0x46))
            offset = 0x37; //0x41 - 0x37 = 0xa
        else if ((*pt >= 0x61) && (*pt <= 0x66))
            offset = 0x57; //0x61 - 0x57 = 0xa

        if (offset != 0)
        {
            if (first)
            {
                *pb = static_cast<quint8>(*pt - offset);
                *pb <<= 4;
                first = false;
            }
            else
            {
                *pb |= *pt - offset;
                ++pb;
                first = true;
            }
        }
        ++pt;
    }
}

CID CID::StringToCID(const char * ptext)
{
    quint8 pbuffer[CIDBYTES]; //Temporarily holds the cid string
    memset(pbuffer, 0, CIDBYTES); //So if the string is invalid, a CID() is created
    UUIDFillString(pbuffer, ptext);
    return CID(pbuffer);
}

//Translates a cid to a preallocated text string of 37 bytes (includes terminating NULL0
void CID::CIDIntoString(const CID & cid, char * ptxt)
{
    snprintf(
        ptxt,
        37,
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        cid.m_cid[0],
        cid.m_cid[1],
        cid.m_cid[2],
        cid.m_cid[3],
        cid.m_cid[4],
        cid.m_cid[5],
        cid.m_cid[6],
        cid.m_cid[7],
        cid.m_cid[8],
        cid.m_cid[9],
        cid.m_cid[10],
        cid.m_cid[11],
        cid.m_cid[12],
        cid.m_cid[13],
        cid.m_cid[14],
        cid.m_cid[15]);
}

QString CID::CIDIntoQString(const CID & cid)
{
    char buffer[CID::CIDSTRINGBYTES];
    CID::CIDIntoString(cid, buffer);
    return QString(buffer);
}

CID::operator QString() const
{
    return CIDIntoQString(*this);
}

// Create a CID
CID CID::CreateCid()
{
    const QUuid uuid = QUuid::createUuid();
    const QByteArray bits = uuid.toRfc4122();

    return CID(reinterpret_cast<const quint8 *>(bits.data()));
}

DCID DCID::StringToDCID(const char * ptext)
{
    quint8 pbuffer[DCIDBYTES]; //Temporarily holds the cid string
    memset(pbuffer, 0, DCIDBYTES); //So if the string is invalid, a DCID() is created
    UUIDFillString(pbuffer, ptext);
    return DCID(pbuffer);
}

//Translates a cid to a preallocated text string of DCIDSTRINGBYTES bytes (includes terminating NULL)
void DCID::DCIDIntoString(const DCID & dcid, char * ptxt)
{
    CID::CIDIntoString(dcid.m_cid, ptxt);
}

//Translates a dcid as a tftp-able filename to a preallocated text string of DCIDFILEBYTES bytes, including the terminating NULL
void DCID::DCIDIntoFileName(const DCID & dcid, char * ptxt)
{
    CID::CIDIntoString(dcid.m_cid, ptxt);
    strcat(ptxt, ".ddl");
}

// Used for maintaining Qt Hash lists using CIDs
uint qHash(const CID & c)
{
    uint result;
    quint8 buf[CID::CIDBYTES];
    c.Pack(buf);
    memcpy(&result, buf, sizeof(uint));
    return result;
}
