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
*/
#ifndef	_DEFPACK_H_
#define	_DEFPACK_H_

#include <QtGlobal>
#include <algorithm>
#include <QByteArray>

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    #define _DEFCOPY_BIG std::reverse_copy
    #define _DEFCOPY_LITTLE std::copy
#else
    #define _DEFCOPY_BIG std::copy
    #define _DEFCOPY_LITTLE std::reverse_copy
#endif

/**
 * @brief Pack QByteArray directly into a buffer
 * @param ptr[in,out] Pointer to buffer
 * @param count Number of bytes to pack
 * @param value Value to pack into buffer, limited to byte count
 */
inline void Pack(quint8* ptr, size_t count, const QByteArray &value)
{
    assert(static_cast<size_t>(value.size()) >= count);
    std::copy(value.begin(), value.begin() + count, ptr);
}

/**
 * @brief Unpack value direcly from a buffer
 * @param ptr[in] Pointer to buffer
 * @param count Number of bytes to unpack
 * @return Unpacked QByteArray
 */
inline QByteArray Upack(const quint8* ptr, size_t count)
{
    QByteArray value;
    value.resize(count);
    std::copy(ptr, ptr + value.size(), std::begin(value));
    return value;
}

/**
 * @brief Pack QByteArray into a big endian buffer
 * @param ptr[in,out] Pointer to buffer
 * @param count Number of bytes to pack
 * @param value Value to pack into buffer, limited to byte count
 */
inline void PackB(quint8* ptr, size_t count, const QByteArray &value)
{
    assert(static_cast<size_t>(value.size()) >= count);
    _DEFCOPY_BIG(value.begin(), value.begin() + count, ptr);
}

/**
 * @brief Unpack value from a big endian buffer
 * @param ptr[in] Pointer to buffer
 * @param count Number of bytes to unpack
 * @return Unpacked QByteArray
 */
inline QByteArray UpackB(const quint8* ptr, size_t count)
{
    QByteArray value;
    value.resize(count);
    _DEFCOPY_BIG(ptr, ptr + value.size(), std::begin(value));
    return value;
}

/**
 * @brief Pack value into a big endian buffer
 * @param ptr[in,out] Pointer to buffer
 * @param count Number of bytes to pack
 * @param value Value to pack into buffer, limited to byte count
 */
template<typename T>
inline void PackBN(quint8* ptr, size_t count, T value)
{
    assert(sizeof(T) >= count);
    const auto valuePtr = reinterpret_cast<quint8*>(&value);
    _DEFCOPY_BIG(valuePtr, valuePtr + count, ptr);
}

/**
 * @brief Unpack value from a big endian buffer
 * @param ptr[in] Pointer to buffer
 * @param count Number of bytes to unpack
 * @return Unpacked value
 */
template<typename T>
inline T UpackBN(const quint8* ptr, size_t count)
{
    assert(sizeof(T) >= count);
    T value = 0;
    auto valuePtr = reinterpret_cast<quint8*>(&value);
    _DEFCOPY_BIG(ptr, ptr + count, valuePtr);
    return value;
}

/**
 * @brief Pack QByteArray into a little endian buffer
 * @param ptr[in,out] Pointer to buffer
 * @param count Number of bytes to pack
 * @param value Value to pack into buffer, limited to byte count
 */
inline void PackL(quint8* ptr, size_t count, const QByteArray &value)
{
    assert(static_cast<size_t>(value.size()) >= count);
    _DEFCOPY_LITTLE(value.begin(), value.begin() + count, ptr);
}

/**
 * @brief Unpack value from a little endian buffer
 * @param ptr[in] Pointer to buffer
 * @param count Number of bytes to unpack
 * @return Unpacked QByteArray
 */
inline QByteArray UpackL(const quint8* ptr, size_t count)
{
    QByteArray value;
    value.resize(count);
    _DEFCOPY_LITTLE(ptr, ptr + value.size(), std::begin(value));
    return value;
}

/**
 * @brief Pack value into a little endian buffer
 * @param ptr[in,out] Pointer to buffer
 * @param count Number of bytes to pack
 * @param value Value to pack into buffer, limited to byte count
 */
template<typename T>
inline void PackLN(quint8* ptr, size_t count, T value)
{
    assert(sizeof(T) >= count);
    const auto valuePtr = reinterpret_cast<quint8*>(&value);
    _DEFCOPY_LITTLE(valuePtr, valuePtr + count, ptr);
}

/**
 * @brief Unpack value from a little endian buffer
 * @param ptr[in] Pointer to buffer
 * @param count Number of bytes to unpack
 * @return Unpacked value
 */
template<typename T>
inline T UpackLN(const quint8* ptr, size_t count)
{
    assert(sizeof(T) >= count);
    T value = 0;
    auto valuePtr = reinterpret_cast<quint8*>(&value);
    _DEFCOPY_LITTLE(ptr, ptr + count, valuePtr);
    return value;
}

//Packs a quint8 to a known big endian buffer
inline void PackBUint8 (quint8* ptr, quint8 value)
{
    PackBN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint8 from a known big endian buffer
inline quint8 UpackBUint8 (const quint8* ptr)
{
    return UpackBN<quint8>(ptr, sizeof(quint8));
}

//Packs a quint8 to a known little endian buffer
inline void PackLUint8 (quint8* ptr, quint8 value)
{
    PackLN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint8 from a known little endian buffer
inline quint8 UpackLUint8 (const quint8* ptr)
{
    return UpackLN<quint8>(ptr, sizeof(quint8));
}

//Packs a quint16 to a known big endian buffer
inline void  PackBUint16 (quint8* ptr, quint16 value)
{
    PackBN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint16 from a known big endian buffer
inline quint16 UpackBUint16 (const quint8* ptr)
{
    return UpackBN<quint16>(ptr, sizeof(quint16));
}

//Packs a quint16 to a known little endian buffer
inline void PackLUint16 (quint8* ptr, quint16 value)
{
    PackLN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint16 from a known little endian buffer
inline quint16 UpackLUint16 (const quint8* ptr)
{
    return UpackLN<quint16>(ptr, sizeof(quint16));
}

//Packs a quint32 to a known big endian buffer
inline void PackBUint32 (quint8* ptr, quint32 value)
{
    PackBN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint32 from a known big endian buffer
inline quint32 UpackBUint32 (const quint8* ptr)
{
    return UpackBN<quint32>(ptr, sizeof(quint32));
}

//Packs a quint32 to a known little endian buffer
inline void PackLUint32 (quint8* ptr, quint32 value)
{
    PackLN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint32 from a known little endian buffer
inline quint32 UpackLUint32 (const quint8* ptr)
{
    return UpackLN<quint32>(ptr, sizeof(quint32));
}

//Packs a quint64 to a known big endian buffer
inline void PackBUint64 (quint8* ptr, quint64 value)
{
    PackBN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint64 from a known big endian buffer
inline quint64 UpackBUint64 (const quint8* ptr)
{
    return UpackBN<quint64>(ptr, sizeof(quint64));
}

//Packs a quint64 to a known little endian buffer
inline void PackLUint64 (quint8* ptr, quint64 value)
{
    PackLN<decltype(value)>(ptr, sizeof(decltype(value)), value);
}

//Unpacks a quint64 from a known little endian buffer
inline quint64 UpackLUint64 (const quint8* ptr)
{
    return UpackLN<quint64>(ptr, sizeof(quint64));
}

#endif	/*_DEFPACK_H_*/

