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

//VHD.cpp		implementation of VHD functions

#include "VHD.h"
#include "defpack.h"

//Defines for the VHD flags
const quint8 VHD_L_FLAG = 0x80;
const quint8 VHD_V_FLAG = 0x40;
const quint8 VHD_H_FLAG = 0x20;
const quint8 VHD_D_FLAG = 0x10;

//Given a pointer, packs the VHD inheritance flags.  The pointer returned is the
//SAME pointer, since it is assumed that length will be packed next
quint8 * VHD_PackFlags(quint8 * pbuffer, bool inheritvec, bool inherithead, bool inheritdata)
{
    quint8 * p = pbuffer;
    quint8 newbyte = UpackBUint8(p) & 0x8f; //Mask out the VHD bits to keep the length intact

    //Update the byte flags
    if (!inheritvec) newbyte |= VHD_V_FLAG;
    if (!inherithead) newbyte |= VHD_H_FLAG;
    if (!inheritdata) newbyte |= VHD_D_FLAG;
    PackBUint8(p, newbyte);
    return pbuffer;
}

//Given a pointer, packs the length.  The pointer returned is the pointer
// to the rest of the buffer.  It is assumed that pbuffer contains at
// least 3 bytes.  If inclength is true, the resultant length of the flags and
// length field is added to the length parameter before packing
quint8 * VHD_PackLength(quint8 * pbuffer, quint32 length, bool inclength)
{
    quint8 * p = pbuffer;
    quint32 mylen = length;
    if (inclength)
    {
        if (length + 1 > VHD_MAXMINLENGTH)
            mylen += 2;
        else
            ++mylen;
    }

    quint8 newbyte = UpackBUint8(p) & 0x70; //Mask out the length bits to keep the other flags intact
    //Set the length bit if necessary
    if (mylen > VHD_MAXMINLENGTH) newbyte |= VHD_L_FLAG;

    //pack the upper length bits
    quint8 packbuf[4]; //we have to manipulate the big endian bits
    PackBUint32(packbuf, mylen);
    if (mylen <= VHD_MAXMINLENGTH)
    {
        //Packbuf[0] and [1] should be empty, and the upper bits of [2] should be empty
        newbyte |= packbuf[2] & 0x0f;
        PackBUint8(p, newbyte);
        ++p;
        PackBUint8(p, packbuf[3]);
        ++p;
    }
    else
    {
        //Packbuf[0] and the upper bits of [1] are ignored
        //We give a max length constant, so the user is warned
        newbyte |= (packbuf[1] & 0x0f);
        PackBUint8(p, newbyte);
        ++p;
        PackBUint8(p, packbuf[2]);
        ++p;
        PackBUint8(p, packbuf[3]);
        ++p;
    }
    return p;
}

//Given a pointer and vector size, packs the vector.
// The pointer returned is the pointer to the rest of the buffer.
//It is assumed that pbuffer contains at least 4 bytes.
quint8 * VHD_PackVector(quint8 * pbuffer, quint32 vector, uint vecsize)
{
    quint8 * p = pbuffer;
    if (vecsize == 1)
    {
        PackBUint8(pbuffer, static_cast<quint8>(vector));
        ++p;
    }
    else if (vecsize == 2)
    {
        PackBUint16(pbuffer, static_cast<quint16>(vector));
        p += 2;
    }
    else
    {
        PackBUint32(pbuffer, vector);
        p += 4;
    }
    return p;
}

const quint8 * VHD_GetFlagLength(
    const quint8 * const pbuffer,
    bool & inheritvec,
    bool & inherithead,
    bool & inheritdata,
    quint32 & length)
{
    inheritvec = (*pbuffer & VHD_V_FLAG) == 0;
    inherithead = (*pbuffer & VHD_H_FLAG) == 0;
    inheritdata = (*pbuffer & VHD_D_FLAG) == 0;
    bool lensmall = (*pbuffer & VHD_L_FLAG) == 0;
    quint8 upackbuf[4]; //We need to manipulate bits
    if (lensmall)
    {
        upackbuf[0] = 0;
        upackbuf[1] = 0;
        upackbuf[2] = *pbuffer & 0x0f;
        upackbuf[3] = *(pbuffer + 1);
        length = UpackBUint32(upackbuf);
        return pbuffer + 2; //flag/len byte and len byte
    }
    else
    {
        upackbuf[0] = 0;
        upackbuf[1] = *pbuffer & 0x0f;
        upackbuf[2] = *(pbuffer + 1);
        upackbuf[3] = *(pbuffer + 2);
        length = UpackBUint32(upackbuf);
        return pbuffer + 3; //flag/len byte, and 2 len bytes
    }
}

const quint8 * VHD_GetVector1(const quint8 * const pbuffer, quint8 & vector)
{
    vector = UpackBUint8(pbuffer);
    return pbuffer + 1;
}

const quint8 * VHD_GetVector2(const quint8 * const pbuffer, quint16 & vector)
{
    vector = UpackBUint16(pbuffer);
    return pbuffer + 2;
}

const quint8 * VHD_GetVector4(const quint8 * const pbuffer, quint32 & vector)
{
    vector = UpackBUint32(pbuffer);
    return pbuffer + 4;
}
