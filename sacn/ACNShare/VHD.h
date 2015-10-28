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

//VHD.h  
//
// This is a small set of functions that help parse and pack a VHD packet.
// Unfortunately, because the header and data separation is highly protocol
// dependent, there is no nice way to simply have a class that takes care of 
// inheritance, etc.
//

#ifndef _VHD_H_
#define _VHD_H_

#ifndef _DEFTYPES_H_
#error "#include error, VHD.h requires deftypes.h"
#endif

const uint VHD_MAXFLAGBYTES = 7;  //The maximum amount of bytes used to pack the flags, len, and vector
const uint VHD_MAXLEN = 0x0fffff;  //The maximum packet length is 20 bytes long
const uint VHD_MAXMINLENGTH = 4095;  //The highest length that will fit in the "smallest" length pack

//Given a pointer, packs the VHD inheritance flags.  The pointer returned is the 
//SAME pointer, since it is assumed that length will be packed next
uint1* VHD_PackFlags(uint1* pbuffer, bool inheritvec, bool inherithead, bool inheritdata);

//Given a pointer, packs the length.  The pointer returned is the pointer
// to the rest of the buffer.  It is assumed that pbuffer contains at 
// least 3 bytes.  If inclength is true, the resultant length of the flags and
// length field is added to the length parameter before packing
uint1* VHD_PackLength(uint1* pbuffer, uint4 length, bool inclength);

//Given a pointer and vector size, packs the vector.  
//The pointer returned is the pointer to the rest of the buffer.  
//It is assumed that pbuffer contains at least 4 bytes.
uint1* VHD_PackVector(uint1* pbuffer, uint4 vector, uint vecsize);

//Given a pointer, the VHD flags and full PDU length is parsed.  The pointer
// returned is the pointer to the rest of the buffer
const uint1* VHD_GetFlagLength(const uint1* const pbuffer, bool& inheritvec,
								     bool& inherithead, bool& inheritdata, uint4& length);

//Unlike PackVector, which takes a size, it's easier to have 3 different functions that
//do the unpack.  The pointer returned is the pointer to the rest of the buffer
const uint1* VHD_GetVector1(const uint1* const pbuffer, uint1& vector);
const uint1* VHD_GetVector2(const uint1* const pbuffer, uint2& vector);
const uint1* VHD_GetVector4(const uint1* const pbuffer, uint4& vector);

#endif  //_VHD_H_
