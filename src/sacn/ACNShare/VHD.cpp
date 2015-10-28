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

#include "deftypes.h"
#include "defpack.h"
#include "VHD.h"

//Defines for the VHD flags
const uint1 VHD_L_FLAG = 0x80;
const uint1 VHD_V_FLAG = 0x40;
const uint1 VHD_H_FLAG = 0x20;
const uint1 VHD_D_FLAG = 0x10;

//Given a pointer, packs the VHD inheritance flags.  The pointer returned is the 
//SAME pointer, since it is assumed that length will be packed next
uint1* VHD_PackFlags(uint1* pbuffer, bool inheritvec, bool inherithead, bool inheritdata)
{
	uint1* p = pbuffer;
	uint1 newbyte = UpackB1(p) & 0x8f;  //Mask out the VHD bits to keep the length intact

	//Update the byte flags
	if(!inheritvec)
		newbyte |= VHD_V_FLAG;
	if(!inherithead)
		newbyte |= VHD_H_FLAG;
	if(!inheritdata)
		newbyte |= VHD_D_FLAG;
	PackB1(p, newbyte);
	return pbuffer;
}

//Given a pointer, packs the length.  The pointer returned is the pointer
// to the rest of the buffer.  It is assumed that pbuffer contains at 
// least 3 bytes.  If inclength is true, the resultant length of the flags and
// length field is added to the length parameter before packing
uint1* VHD_PackLength(uint1* pbuffer, uint4 length, bool inclength)
{	
	uint1* p = pbuffer;
	uint4 mylen = length;
	if(inclength)
	{
		if(length + 1 > VHD_MAXMINLENGTH)
			mylen += 2;
		else
			++mylen;
	}

	uint1 newbyte = UpackB1(p) & 0x70;  //Mask out the length bits to keep the other flags intact
	//Set the length bit if necessary
	if(mylen > VHD_MAXMINLENGTH)
		newbyte |= VHD_L_FLAG;

	//pack the upper length bits
	uint1 packbuf[4];  //we have to manipulate the big endian bits
	PackB4(packbuf, mylen);
	if(mylen <= VHD_MAXMINLENGTH)
	{
		//Packbuf[0] and [1] should be empty, and the upper bits of [2] should be empty
		newbyte |= packbuf[2] & 0x0f;
		PackB1(p, newbyte);
		++p;
		PackB1(p, packbuf[3]);
		++p;
	}
	else
	{
		//Packbuf[0] and the upper bits of [1] are ignored
		//We give a max length constant, so the user is warned
		newbyte |= (packbuf[1] & 0x0f);
		PackB1(p, newbyte);
		++p;
		PackB1(p, packbuf[2]);
		++p;
		PackB1(p, packbuf[3]);
		++p;
	}
	return p;
}

//Given a pointer and vector size, packs the vector.  
// The pointer returned is the pointer to the rest of the buffer.
//It is assumed that pbuffer contains at least 4 bytes.
uint1* VHD_PackVector(uint1* pbuffer, uint4 vector, uint vecsize)
{
	uint1* p = pbuffer;
	if(vecsize == 1)
	{
		PackB1(pbuffer, static_cast<uint1>(vector));
		++p;
	}
	else if(vecsize == 2)
	{
		PackB2(pbuffer, static_cast<uint2>(vector));
		p += 2;
	}
	else
	{
		PackB4(pbuffer, vector);
		p += 4;
	}
	return p;
}

const uint1* VHD_GetFlagLength(const uint1* const pbuffer, bool& inheritvec,
									 bool& inherithead, bool& inheritdata, uint4& length)
{
	inheritvec = (*pbuffer & VHD_V_FLAG) == 0;
	inherithead = (*pbuffer & VHD_H_FLAG) == 0;
	inheritdata = (*pbuffer & VHD_D_FLAG) == 0;
	bool lensmall = (*pbuffer & VHD_L_FLAG) == 0;
	uint1 upackbuf[4];  //We need to manipulate bits
	if(lensmall)
	{
		upackbuf[0] = 0;
		upackbuf[1] = 0;
		upackbuf[2] = *pbuffer & 0x0f;
		upackbuf[3] = *(pbuffer + 1);
		length = UpackB4(upackbuf);
		return pbuffer + 2;	//flag/len byte and len byte
	}
	else
	{
		upackbuf[0] = 0;
		upackbuf[1] = *pbuffer & 0x0f;
		upackbuf[2] = *(pbuffer + 1);
		upackbuf[3] = *(pbuffer + 2);
		length = UpackB4(upackbuf);
		return pbuffer + 3; //flag/len byte, and 2 len bytes
	}
}


const uint1* VHD_GetVector1(const uint1* const pbuffer, uint1& vector)
{
	vector = UpackB1(pbuffer);
	return pbuffer + 1;
}

const uint1* VHD_GetVector2(const uint1* const pbuffer, uint2& vector)
{
	vector = UpackB2(pbuffer);
	return pbuffer + 2;
}

const uint1* VHD_GetVector4(const uint1* const pbuffer, uint4& vector)
{
	vector = UpackB4(pbuffer);
	return pbuffer + 4;
}
