/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#ifndef WATER_BYTEORDER_H_INCLUDED
#define WATER_BYTEORDER_H_INCLUDED

#include "../water.h"

#ifdef CARLA_OS_MAC
# include <libkern/OSByteOrder.h>
#endif

namespace water {

//==============================================================================
/** Contains static methods for converting the byte order between different
    endiannesses.
*/
class ByteOrder
{
public:
    //==============================================================================
    /** Swaps the upper and lower bytes of a 16-bit integer. */
    static uint16 swap (uint16 value) noexcept;

    /** Reverses the order of the 4 bytes in a 32-bit integer. */
    static uint32 swap (uint32 value) noexcept;

    /** Reverses the order of the 8 bytes in a 64-bit integer. */
    static uint64 swap (uint64 value) noexcept;

    //==============================================================================
    /** Swaps the byte order of a 16-bit unsigned int if the CPU is big-endian */
    static uint16 swapIfBigEndian (uint16 value) noexcept;

    /** Swaps the byte order of a 32-bit unsigned int if the CPU is big-endian */
    static uint32 swapIfBigEndian (uint32 value) noexcept;

    /** Swaps the byte order of a 64-bit unsigned int if the CPU is big-endian */
    static uint64 swapIfBigEndian (uint64 value) noexcept;

    /** Swaps the byte order of a 16-bit signed int if the CPU is big-endian */
    static int16 swapIfBigEndian (int16 value) noexcept;

    /** Swaps the byte order of a 32-bit signed int if the CPU is big-endian */
    static int32 swapIfBigEndian (int32 value) noexcept;

    /** Swaps the byte order of a 64-bit signed int if the CPU is big-endian */
    static int64 swapIfBigEndian (int64 value) noexcept;

    /** Swaps the byte order of a 32-bit float if the CPU is big-endian */
    static float swapIfBigEndian (float value) noexcept;

    /** Swaps the byte order of a 64-bit float if the CPU is big-endian */
    static double swapIfBigEndian (double value) noexcept;

    /** Swaps the byte order of a 16-bit unsigned int if the CPU is little-endian */
    static uint16 swapIfLittleEndian (uint16 value) noexcept;

    /** Swaps the byte order of a 32-bit unsigned int if the CPU is little-endian */
    static uint32 swapIfLittleEndian (uint32 value) noexcept;

    /** Swaps the byte order of a 64-bit unsigned int if the CPU is little-endian */
    static uint64 swapIfLittleEndian (uint64 value) noexcept;

    /** Swaps the byte order of a 16-bit signed int if the CPU is little-endian */
    static int16 swapIfLittleEndian (int16 value) noexcept;

    /** Swaps the byte order of a 32-bit signed int if the CPU is little-endian */
    static int32 swapIfLittleEndian (int32 value) noexcept;

    /** Swaps the byte order of a 64-bit signed int if the CPU is little-endian */
    static int64 swapIfLittleEndian (int64 value) noexcept;

    /** Swaps the byte order of a 32-bit float if the CPU is little-endian */
    static float swapIfLittleEndian (float value) noexcept;

    /** Swaps the byte order of a 64-bit float if the CPU is little-endian */
    static double swapIfLittleEndian (double value) noexcept;

    //==============================================================================
    /** Turns 4 bytes into a little-endian integer. */
    static uint32 littleEndianInt (const void* bytes) noexcept;

    /** Turns 8 bytes into a little-endian integer. */
    static uint64 littleEndianInt64 (const void* bytes) noexcept;

    /** Turns 2 bytes into a little-endian integer. */
    static uint16 littleEndianShort (const void* bytes) noexcept;

    /** Turns 4 bytes into a big-endian integer. */
    static uint32 bigEndianInt (const void* bytes) noexcept;

    /** Turns 8 bytes into a big-endian integer. */
    static uint64 bigEndianInt64 (const void* bytes) noexcept;

    /** Turns 2 bytes into a big-endian integer. */
    static uint16 bigEndianShort (const void* bytes) noexcept;

    //==============================================================================
    /** Converts 3 little-endian bytes into a signed 24-bit value (which is sign-extended to 32 bits). */
    static int littleEndian24Bit (const void* bytes) noexcept;

    /** Converts 3 big-endian bytes into a signed 24-bit value (which is sign-extended to 32 bits). */
    static int bigEndian24Bit (const void* bytes) noexcept;

    /** Copies a 24-bit number to 3 little-endian bytes. */
    static void littleEndian24BitToChars (int value, void* destBytes) noexcept;

    /** Copies a 24-bit number to 3 big-endian bytes. */
    static void bigEndian24BitToChars (int value, void* destBytes) noexcept;

    //==============================================================================
    /** Returns true if the current CPU is big-endian. */
    static bool isBigEndian() noexcept;

private:
    ByteOrder() WATER_DELETED_FUNCTION;

    CARLA_DECLARE_NON_COPY_CLASS (ByteOrder)
};


//==============================================================================
inline uint16 ByteOrder::swap (uint16 n) noexcept
{
    return static_cast<uint16> ((n << 8) | (n >> 8));
}

inline uint32 ByteOrder::swap (uint32 n) noexcept
{
   #ifdef CARLA_OS_MAC
    return OSSwapInt32 (n);
   #elif defined(CARLA_OS_WIN) || ! (defined (__arm__) || defined (__arm64__) || defined (__aarch64__))
    asm("bswap %%eax" : "=a"(n) : "a"(n));
    return n;
   #else
    return (n << 24) | (n >> 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8);
   #endif
}

inline uint64 ByteOrder::swap (uint64 value) noexcept
{
   #ifdef CARLA_OS_MAC
    return OSSwapInt64 (value);
   #else
    return (((uint64) swap ((uint32) value)) << 32) | swap ((uint32) (value >> 32));
   #endif
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
 inline uint16 ByteOrder::swapIfBigEndian (const uint16 v) noexcept                                  { return v; }
 inline uint32 ByteOrder::swapIfBigEndian (const uint32 v) noexcept                                  { return v; }
 inline uint64 ByteOrder::swapIfBigEndian (const uint64 v) noexcept                                  { return v; }
 inline int16 ByteOrder::swapIfBigEndian (const int16 v) noexcept                                    { return v; }
 inline int32 ByteOrder::swapIfBigEndian (const int32 v) noexcept                                    { return v; }
 inline int64 ByteOrder::swapIfBigEndian (const int64 v) noexcept                                    { return v; }
 inline float ByteOrder::swapIfBigEndian (const float v) noexcept                                    { return v; }
 inline double ByteOrder::swapIfBigEndian (const double v) noexcept                                  { return v; }

 inline uint16 ByteOrder::swapIfLittleEndian (const uint16 v) noexcept                               { return swap (v); }
 inline uint32 ByteOrder::swapIfLittleEndian (const uint32 v) noexcept                               { return swap (v); }
 inline uint64 ByteOrder::swapIfLittleEndian (const uint64 v) noexcept                               { return swap (v); }
 inline int16 ByteOrder::swapIfLittleEndian (const int16 v) noexcept                                 { return static_cast<int16> (swap (static_cast<uint16> (v))); }
 inline int32 ByteOrder::swapIfLittleEndian (const int32 v) noexcept                                 { return static_cast<int32> (swap (static_cast<uint32> (v))); }
 inline int64 ByteOrder::swapIfLittleEndian (const int64 v) noexcept                                 { return static_cast<int64> (swap (static_cast<uint64> (v))); }
 inline float ByteOrder::swapIfLittleEndian (const float v) noexcept                                 { union { uint32 asUInt; float asFloat;  } n; n.asFloat = v; n.asUInt = ByteOrder::swap (n.asUInt); return n.asFloat; }
 inline double ByteOrder::swapIfLittleEndian (const double v) noexcept                               { union { uint64 asUInt; double asFloat; } n; n.asFloat = v; n.asUInt = ByteOrder::swap (n.asUInt); return n.asFloat; }

 inline uint32 ByteOrder::littleEndianInt (const void* const bytes) noexcept                         { return *static_cast<const uint32*> (bytes); }
 inline uint64 ByteOrder::littleEndianInt64 (const void* const bytes) noexcept                       { return *static_cast<const uint64*> (bytes); }
 inline uint16 ByteOrder::littleEndianShort (const void* const bytes) noexcept                       { return *static_cast<const uint16*> (bytes); }
 inline uint32 ByteOrder::bigEndianInt (const void* const bytes) noexcept                            { return swap (*static_cast<const uint32*> (bytes)); }
 inline uint64 ByteOrder::bigEndianInt64 (const void* const bytes) noexcept                          { return swap (*static_cast<const uint64*> (bytes)); }
 inline uint16 ByteOrder::bigEndianShort (const void* const bytes) noexcept                          { return swap (*static_cast<const uint16*> (bytes)); }
 inline bool ByteOrder::isBigEndian() noexcept                                                       { return false; }
#else
 inline uint16 ByteOrder::swapIfBigEndian (const uint16 v) noexcept                                  { return swap (v); }
 inline uint32 ByteOrder::swapIfBigEndian (const uint32 v) noexcept                                  { return swap (v); }
 inline uint64 ByteOrder::swapIfBigEndian (const uint64 v) noexcept                                  { return swap (v); }
 inline int16 ByteOrder::swapIfBigEndian (const int16 v) noexcept                                    { return static_cast<int16> (swap (static_cast<uint16> (v))); }
 inline int32 ByteOrder::swapIfBigEndian (const int32 v) noexcept                                    { return static_cast<int16> (swap (static_cast<uint16> (v))); }
 inline int64 ByteOrder::swapIfBigEndian (const int64 v) noexcept                                    { return static_cast<int16> (swap (static_cast<uint16> (v))); }
 inline float ByteOrder::swapIfBigEndian (const float v) noexcept                                    { union { uint32 asUInt; float asFloat;  } n; n.asFloat = v; n.asUInt = ByteOrder::swap (n.asUInt); return n.asFloat; }
 inline double ByteOrder::swapIfBigEndian (const double v) noexcept                                  { union { uint64 asUInt; double asFloat; } n; n.asFloat = v; n.asUInt = ByteOrder::swap (n.asUInt); return n.asFloat; }

 inline uint16 ByteOrder::swapIfLittleEndian (const uint16 v) noexcept                               { return v; }
 inline uint32 ByteOrder::swapIfLittleEndian (const uint32 v) noexcept                               { return v; }
 inline uint64 ByteOrder::swapIfLittleEndian (const uint64 v) noexcept                               { return v; }
 inline int16 ByteOrder::swapIfLittleEndian (const int16 v) noexcept                                 { return v; }
 inline int32 ByteOrder::swapIfLittleEndian (const int32 v) noexcept                                 { return v; }
 inline int64 ByteOrder::swapIfLittleEndian (const int64 v) noexcept                                 { return v; }
 inline float ByteOrder::swapIfLittleEndian (const float v) noexcept                                 { return v; }
 inline double ByteOrder::swapIfLittleEndian (const double v) noexcept                               { return v; }

 inline uint32 ByteOrder::littleEndianInt (const void* const bytes) noexcept                         { return swap (*static_cast<const uint32*> (bytes)); }
 inline uint64 ByteOrder::littleEndianInt64 (const void* const bytes) noexcept                       { return swap (*static_cast<const uint64*> (bytes)); }
 inline uint16 ByteOrder::littleEndianShort (const void* const bytes) noexcept                       { return swap (*static_cast<const uint16*> (bytes)); }
 inline uint32 ByteOrder::bigEndianInt (const void* const bytes) noexcept                            { return *static_cast<const uint32*> (bytes); }
 inline uint64 ByteOrder::bigEndianInt64 (const void* const bytes) noexcept                          { return *static_cast<const uint64*> (bytes); }
 inline uint16 ByteOrder::bigEndianShort (const void* const bytes) noexcept                          { return *static_cast<const uint16*> (bytes); }
 inline bool ByteOrder::isBigEndian() noexcept                                                       { return true; }
#endif

inline int  ByteOrder::littleEndian24Bit (const void* const bytes) noexcept                          { return (((int) static_cast<const int8*> (bytes)[2]) << 16) | (((int) static_cast<const uint8*> (bytes)[1]) << 8) | ((int) static_cast<const uint8*> (bytes)[0]); }
inline int  ByteOrder::bigEndian24Bit (const void* const bytes) noexcept                             { return (((int) static_cast<const int8*> (bytes)[0]) << 16) | (((int) static_cast<const uint8*> (bytes)[1]) << 8) | ((int) static_cast<const uint8*> (bytes)[2]); }
inline void ByteOrder::littleEndian24BitToChars (const int value, void* const destBytes) noexcept    { static_cast<uint8*> (destBytes)[0] = (uint8) value;         static_cast<uint8*> (destBytes)[1] = (uint8) (value >> 8); static_cast<uint8*> (destBytes)[2] = (uint8) (value >> 16); }
inline void ByteOrder::bigEndian24BitToChars (const int value, void* const destBytes) noexcept       { static_cast<uint8*> (destBytes)[0] = (uint8) (value >> 16); static_cast<uint8*> (destBytes)[1] = (uint8) (value >> 8); static_cast<uint8*> (destBytes)[2] = (uint8) value; }

}

#endif // WATER_BYTEORDER_H_INCLUDED
