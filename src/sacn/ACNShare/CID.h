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

// CID.h: interface for the CID class and DCID class
//
// The CID class encapsulates the functionality for using, passing,
// copying, packing, and comparing SDT CIDs.
// The same goes for the DCID class
//////////////////////////////////////////////////////////////////////

#ifndef _CID_H_
#define _CID_H_

#include <QDataStream>
#include <QMetaType>
#include <QtGlobal>

class CID
{
public:

    enum
    {
        CIDBYTES = 16, //The number of bytes in a CID
        // See format string in CIDIntoString().
        // The 16 CID bytes expand into 32 ASCII hexadecimal characters.
        // There are 4 internal dashes, plus 1 terminating null,
        // totaling 37 bytes.
        CIDSTRINGBYTES = 37 //The number of bytes in a CID string, INCLUDING terminating NULL
    };

    CID(const quint8 * pCID); //The CID buffer (in binary form -- Use StringToCID to convert)
    CID(const CID & cid);
    CID();
    virtual ~CID();

    void Pack(quint8 * pbuf) const; //Packs the cid into the buffer -- There must be enough space
    void Unpack(const quint8 * pbuf); //Unpacks the cid from the buffer

    CID & operator=(const CID & cid);

    friend bool operator<(const CID & c1, const CID & c2);
    friend bool operator==(const CID & c1, const CID & c2);
    friend bool operator!=(const CID & c1, const CID & c2);

    friend QDataStream & operator<<(QDataStream & lhs, const CID & rhs)
    {
        lhs.writeRawData(reinterpret_cast<const char *>(rhs.m_cid), CIDBYTES);
        return lhs;
    }
    friend QDataStream & operator>>(QDataStream & lhs, CID & rhs)
    {
        lhs.readRawData(reinterpret_cast<char *>(rhs.m_cid), CIDBYTES);
        return lhs;
    }

    //Returns a CID based on the string.
    static CID StringToCID(const char * ptext);

    //Translates a cid to a preallocated text string of 37 bytes, including the terminating NULL
    static void CIDIntoString(const CID & cid, char * ptxt);
    static QString CIDIntoQString(const CID & cid);
    operator QString() const;

    // Create a CID using the platforms UUID methods
    static CID CreateCid();

    // Is the CID null?
    bool isNull() const;

private:

    quint8 m_cid[CIDBYTES];
};
Q_DECLARE_METATYPE(CID);

//Essentially the same type as a CID, but for a different semantic purpose
//Just masks the CID type
class DCID
{
public:

    enum
    {
        DCIDBYTES = CID::CIDBYTES, //The number of bytes in a DCID
        DCIDSTRINGBYTES = CID::CIDSTRINGBYTES, //The number of bytes in a DCID string, INCLUDING terminating NULL
        DCIDFILEBYTES = CID::CIDSTRINGBYTES
            + 4 //The DCID in the tftp request format: [DCID].ddl, INCLUDING terminating NULL
    };

    DCID(const quint8 * pDCID); //The DCID buffer (in binary form -- Use StringToDCID to convert)
    DCID(const DCID & dcid);
    DCID();
    virtual ~DCID();

    void Pack(quint8 * pbuf) const; //Packs the cid into the buffer -- There must be enough space
    void Unpack(const quint8 * pbuf); //Unpacks the cid from the buffer

    DCID & operator=(const DCID & dcid);

    friend bool operator<(const DCID & d1, const DCID & d2);
    friend bool operator==(const DCID & d1, const DCID & d2);
    friend bool operator!=(const DCID & d1, const DCID & d2);

    //Returns a DCID based on the string.
    static DCID StringToDCID(const char * ptext);

    //Translates a dcid to a preallocated text string of DCIDSTRINGBYTES bytes, including the terminating NULL
    static void DCIDIntoString(const DCID & dcid, char * ptxt);

    //Translates a dcid as a tftp-able filename to a preallocated text string of DCIDFILEBYTES bytes, including the terminating NULL
    static void DCIDIntoFileName(const DCID & dcid, char * ptxt);

private:

    CID m_cid;
};

bool operator<(const CID & c1, const CID & c2);
bool operator==(const CID & c1, const CID & c2);
bool operator!=(const CID & c1, const CID & c2);
bool operator<(const DCID & d1, const DCID & d2);
bool operator==(const DCID & d1, const DCID & d2);
bool operator!=(const DCID & d1, const DCID & d2);

uint qHash(const CID & c);

#endif // !defined(_CID_H_)
