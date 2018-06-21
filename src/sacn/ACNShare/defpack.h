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

#include <QtGlobal>

//Declarations
void  PackB1 (quint8* ptr, quint8 val);	//Packs a quint8 to a known big endian buffer
quint8 UpackB1(const quint8* ptr);		//Unpacks a quint8 from a known big endian buffer
void  PackL1 (quint8* ptr, quint8 val);	//Packs a quint8 to a known little endian buffer
quint8 UpackL1(const quint8* ptr);		//Unpacks a quint8 from a known little endian buffer
void  PackB2 (quint8* ptr, quint16 val);	//Packs a quint16 to a known big endian buffer
quint16 UpackB2(const quint8* ptr);		//Unpacks a quint16 from a known big endian buffer
void  PackL2 (quint8* ptr, quint16 val);	//Packs a quint16 to a known little endian buffer
quint16 UpackL2(const quint8* ptr);		//Unpacks a quint16 from a known little endian buffer
void  PackB4 (quint8* ptr, quint32 val);	//Packs a quint32 to a known big endian buffer
quint32 UpackB4(const quint8* ptr);		//Unpacks a quint32 from a known big endian buffer
void  PackL4 (quint8* ptr, quint32 val);	//Packs a quint32 to a known little endian buffer
quint32 UpackL4(const quint8* ptr);		//Unpacks a quint32 from a known little endian buffer
void  PackB8 (quint8* ptr, quint64 val);	//Packs a quint64 to a known big endian buffer
quint64 UpackB8(const quint8* ptr);		//Unpacks a quint32 from a known big endian buffer
void  PackL8 (quint8* ptr, quint64 val);	//Packs a quint64 to a known little endian buffer
quint64 UpackL8(const quint8* ptr);		//Unpacks a quint64 from a known little endian buffer

//---------------------------------------------------------------------------------
//implementation

//Packs a quint8 to a known big endian buffer
inline void	PackB1(quint8* ptr, quint8 val)
{
	*ptr = val;
}

//Unpacks a quint8 from a known big endian buffer
inline quint8 UpackB1(const quint8* ptr)
{
	return *ptr;
}

//Packs a quint8 to a known little endian buffer
inline void	PackL1(quint8* ptr, quint8 val)
{
	*ptr = val;
}

//Unpacks a quint8 from a known little endian buffer
inline quint8 UpackL1(const quint8* ptr)
{
	return *ptr;
}

//Packs a quint16 to a known big endian buffer
inline void PackB2(quint8* ptr, quint16 val)
{
    ptr[1] = (quint8)(val & 0xff);
    ptr[0] = (quint8)((val & 0xff00) >> 8);
}

//Unpacks a quint16 from a known big endian buffer
inline quint16 UpackB2(const quint8* ptr)
{
    return (quint16)(ptr[1] | ptr[0] << 8);
}

//Packs a quint16 to a known little endian buffer
inline void PackL2(quint8* ptr, quint16 val)
{
    *((quint16*)ptr) = val;
}

//Unpacks a quint16 from a known little endian buffer
inline quint16 UpackL2(const quint8* ptr)
{
    return *((quint16*)ptr);
}

//Packs a quint32 to a known big endian buffer
inline void PackB4(quint8* ptr, quint32 val)
{
    ptr[3] = (quint8) (val & 0xff);
    ptr[2] = (quint8)((val & 0xff00) >> 8);
    ptr[1] = (quint8)((val & 0xff0000) >> 16);
    ptr[0] = (quint8)((val & 0xff000000) >> 24);
}

//Unpacks a quint32 from a known big endian buffer
inline quint32 UpackB4(const quint8* ptr)
{
    return (quint32)(ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24));
}

//Packs a quint32 to a known little endian buffer
inline void PackL4(quint8* ptr, quint32 val)
{
    *((quint32*)ptr) = val;
}

//Unpacks a quint32 from a known little endian buffer
inline quint32 UpackL4(const quint8* ptr)
{
    return *((quint32*)ptr);
}

//Packs a quint64 to a known big endian buffer
inline void PackB8(quint8* ptr, quint64 val)
{
    ptr[7] = (quint8) (val & 0xff);
    ptr[6] = (quint8)((val & 0xff00) >> 8);
    ptr[5] = (quint8)((val & 0xff0000) >> 16);
    ptr[4] = (quint8)((val & 0xff000000) >> 24);
    ptr[3] = (quint8)((val & 0xff00000000) >> 32);
    ptr[2] = (quint8)((val & 0xff0000000000) >> 40);
    ptr[1] = (quint8)((val & 0xff000000000000) >> 48);
    ptr[0] = (quint8)((val & 0xff00000000000000) >> 56);
}

//Unpacks a quint32 from a known big endian buffer
inline quint64 UpackB8(const quint8* ptr)
{
    return ((quint64)ptr[7]) | (((quint64)ptr[6]) << 8) | (((quint64)ptr[5]) << 16) |
           (((quint64)ptr[4]) << 24) | (((quint64)ptr[3]) << 32) |
           (((quint64)ptr[2]) << 40) | (((quint64)ptr[1]) << 48) |
           (((quint64)ptr[0]) << 56);
}

//Packs a quint64 to a known little endian buffer
inline void PackL8(quint8* ptr, quint64 val)
{
    *((quint64*)ptr) = val;
}

//Unpacks a quint64 from a known little endian buffer
inline quint64 UpackL8(const quint8* ptr)
{
    return *((quint64*)ptr);
}


#endif	/*_DEFPACK_H_*/

