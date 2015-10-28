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

/*defpack.h
  This file defines the big and little endian packing/unpacking utilities that
  common code uses.

  All functions follow the form PackXY and UpackXY, 
  where X denotes endianness (B or L, for big and little, respecively),
  and Y denotes the number of bytes (1, 2, 4, 8)

  Windows version follows, 
  which is a little endian machine with no boundary problems.
  
  I've added explicit masking to this version to allow anyone running with "smaller type"
  checking to not have debug exceptions.  
*/
#ifndef	_DEFPACK_H_
#define	_DEFPACK_H_

//defpack requires the default types
#ifndef _DEFTYPES_H_
#error "#include error: defpack.h requires deftypes.h"
#endif

//Declarations
void  PackB1 (uint1* ptr, uint1 val);	//Packs a uint1 to a known big endian buffer
uint1 UpackB1(const uint1* ptr);		//Unpacks a uint1 from a known big endian buffer
void  PackL1 (uint1* ptr, uint1 val);	//Packs a uint1 to a known little endian buffer
uint1 UpackL1(const uint1* ptr);		//Unpacks a uint1 from a known little endian buffer
void  PackB2 (uint1* ptr, uint2 val);	//Packs a uint2 to a known big endian buffer
uint2 UpackB2(const uint1* ptr);		//Unpacks a uint2 from a known big endian buffer
void  PackL2 (uint1* ptr, uint2 val);	//Packs a uint2 to a known little endian buffer
uint2 UpackL2(const uint1* ptr);		//Unpacks a uint2 from a known little endian buffer
void  PackB4 (uint1* ptr, uint4 val);	//Packs a uint4 to a known big endian buffer
uint4 UpackB4(const uint1* ptr);		//Unpacks a uint4 from a known big endian buffer
void  PackL4 (uint1* ptr, uint4 val);	//Packs a uint4 to a known little endian buffer
uint4 UpackL4(const uint1* ptr);		//Unpacks a uint4 from a known little endian buffer
void  PackB8 (uint1* ptr, uint8 val);	//Packs a uint8 to a known big endian buffer
uint8 UpackB8(const uint1* ptr);		//Unpacks a uint4 from a known big endian buffer
void  PackL8 (uint1* ptr, uint8 val);	//Packs a uint8 to a known little endian buffer
uint8 UpackL8(const uint1* ptr);		//Unpacks a uint8 from a known little endian buffer

//---------------------------------------------------------------------------------
//implementation

//Packs a uint1 to a known big endian buffer
inline void	PackB1(uint1* ptr, uint1 val)
{
	*ptr = val;
}

//Unpacks a uint1 from a known big endian buffer
inline uint1 UpackB1(const uint1* ptr)
{
	return *ptr;
}

//Packs a uint1 to a known little endian buffer
inline void	PackL1(uint1* ptr, uint1 val)
{
	*ptr = val;
}

//Unpacks a uint1 from a known little endian buffer
inline uint1 UpackL1(const uint1* ptr)
{
	return *ptr;
}

//Packs a uint2 to a known big endian buffer
inline void PackB2(uint1* ptr, uint2 val)
{
	ptr[1] = (uint1)(val & 0xff);
	ptr[0] = (uint1)((val & 0xff00) >> 8);
}

//Unpacks a uint2 from a known big endian buffer
inline uint2 UpackB2(const uint1* ptr)
{
	return (uint2)(ptr[1] | ptr[0] << 8);
}

//Packs a uint2 to a known little endian buffer
inline void PackL2(uint1* ptr, uint2 val)
{
	*((uint2*)ptr) = val;
}

//Unpacks a uint2 from a known little endian buffer
inline uint2 UpackL2(const uint1* ptr)
{
	return *((uint2*)ptr);
}

//Packs a uint4 to a known big endian buffer
inline void PackB4(uint1* ptr, uint4 val)
{
	ptr[3] = (uint1) (val & 0xff);
	ptr[2] = (uint1)((val & 0xff00) >> 8);
	ptr[1] = (uint1)((val & 0xff0000) >> 16);
	ptr[0] = (uint1)((val & 0xff000000) >> 24);
}

//Unpacks a uint4 from a known big endian buffer
inline uint4 UpackB4(const uint1* ptr)
{
	return (uint4)(ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24));
}

//Packs a uint4 to a known little endian buffer
inline void PackL4(uint1* ptr, uint4 val)
{
	*((uint4*)ptr) = val;
}

//Unpacks a uint4 from a known little endian buffer
inline uint4 UpackL4(const uint1* ptr)
{
	return *((uint4*)ptr);
}

//Packs a uint8 to a known big endian buffer
inline void PackB8(uint1* ptr, uint8 val)
{
	ptr[7] = (uint1) (val & 0xff);
	ptr[6] = (uint1)((val & 0xff00) >> 8);
	ptr[5] = (uint1)((val & 0xff0000) >> 16);
	ptr[4] = (uint1)((val & 0xff000000) >> 24);
	ptr[3] = (uint1)((val & 0xff00000000) >> 32);
	ptr[2] = (uint1)((val & 0xff0000000000) >> 40);
	ptr[1] = (uint1)((val & 0xff000000000000) >> 48);
	ptr[0] = (uint1)((val & 0xff00000000000000) >> 56);
}

//Unpacks a uint4 from a known big endian buffer
inline uint8 UpackB8(const uint1* ptr)
{
	return ((uint8)ptr[7]) | (((uint8)ptr[6]) << 8) | (((uint8)ptr[5]) << 16) |
		   (((uint8)ptr[4]) << 24) | (((uint8)ptr[3]) << 32) | 
		   (((uint8)ptr[2]) << 40) | (((uint8)ptr[1]) << 48) | 
		   (((uint8)ptr[0]) << 56);
}

//Packs a uint8 to a known little endian buffer
inline void PackL8(uint1* ptr, uint8 val)
{
	*((uint8*)ptr) = val;
}

//Unpacks a uint8 from a known little endian buffer
inline uint8 UpackL8(const uint1* ptr)
{
	return *((uint8*)ptr);
}


#endif	/*_DEFPACK_H_*/

