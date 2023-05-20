/*
 * Carla sha1 utils
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_SHA1_UTILS_HPP_INCLUDED
#define CARLA_SHA1_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ ==  __ORDER_BIG_ENDIAN__))
# define CARLA_SHA1_BIG_ENDIAN
#endif

/*!
 * Simple, single-use SHA1 class.
 * Must be discarded after use.
 *
 * Based on libcrypt by Wei Dai and other contributors (originally in the public domain)
 */
class CarlaSha1 {
    static constexpr const size_t BLOCK_LENGTH = 64;

    union {
        uint8_t u8[BLOCK_LENGTH];
        uint32_t u32[BLOCK_LENGTH/sizeof(uint32_t)];
    } buffer;
    uint32_t state[5];
    uint32_t byteCount;
    uint8_t bufferOffset;
    char resultstr[41];

    static_assert(sizeof(buffer.u8) == sizeof(buffer.u32), "valid size");

public:
    /*
     * Constructor.
     */
    CarlaSha1() noexcept
        : byteCount(0),
          bufferOffset(0)
    {
        state[0] = 0x67452301;
        state[1] = 0xefcdab89;
        state[2] = 0x98badcfe;
        state[3] = 0x10325476;
        state[4] = 0xc3d2e1f0;
    }

    /*
     * Write a single byte of data.
     */
    void writeByte(const uint8_t data) noexcept
    {
        ++byteCount;
        _addUncounted(data);
    }

    /*
     * Write a custom blob of data.
     */
    void write(const void* const data, size_t len) noexcept
    {
        const uint8_t* u8data = static_cast<const uint8_t*>(data);

        while (len--)
            writeByte(*u8data++);
    }

    /*
     * Return hash result as byte array (20 characters).
     * @note must be called only once!
     */
    const uint8_t* resultAsHash() noexcept
    {
        // Pad to complete the last block
        _pad();

       #ifndef CARLA_SHA1_BIG_ENDIAN
        // Swap byte order back
        for (int i=0; i<5; ++i)
        {
            state[i] = ((state[i] << 24) & 0xff000000)
                     | ((state[i] <<  8) & 0x00ff0000)
                     | ((state[i] >>  8) & 0x0000ff00)
                     | ((state[i] >> 24) & 0x000000ff);
        }
       #endif

        return static_cast<uint8_t*>(static_cast<void*>(state));
    }

    /*
     * Return hash result as null-terminated string.
     * @note must be called only once!
     */
    const char* resultAsString() noexcept
    {
        const uint8_t* const hash = resultAsHash();

        for (int i=0; i<20; ++i)
            std::snprintf(resultstr + (i * 2), 3, "%02x", hash[i]);

        resultstr[40] = '\0';

        return resultstr;
    }

private:
    void _addUncounted(const uint8_t data) noexcept
    {
       #ifdef CARLA_SHA1_BIG_ENDIAN
        buffer.u8[bufferOffset] = data;
       #else
        buffer.u8[bufferOffset ^ 3] = data;
       #endif
        if (++bufferOffset == BLOCK_LENGTH)
        {
            bufferOffset = 0;
            _hashBlock();
        }
    }

    void _hashBlock() noexcept
    {
        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t t;

        for (uint8_t i=0; i<80; ++i)
        {
            if (i >= 16)
            {
                t = buffer.u32[(i + 13) & 15]
                  ^ buffer.u32[(i + 8) & 15] 
                  ^ buffer.u32[(i + 2) & 15]
                  ^ buffer.u32[i & 15];
                buffer.u32[i & 15] = _rol32(t, 1);
            }
            if (i < 20)
            {
                t = (d ^ (b & (c ^ d))) + 0x5a827999;
            }
            else if (i < 40)
            {
                t = (b ^ c ^ d) + 0x6ed9eba1;
            }
            else if (i < 60)
            {
                t = ((b & c) | (d & (b | c))) + 0x8f1bbcdc;
            }
            else
            {
                t = (b ^ c ^ d) + 0xca62c1d6;
            }
            t += _rol32(a, 5) + e + buffer.u32[i & 15];
            e = d;
            d = c;
            c = _rol32(b, 30);
            b = a;
            a = t;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
    }

    // Implement SHA-1 padding (fips180-2 §5.1.1)
    void _pad() noexcept
    {
        // Pad with 0x80 followed by 0x00 until the end of the block
        _addUncounted(0x80);
        while (bufferOffset != 56) _addUncounted(0x00);

        // Append length in the last 8 bytes
        _addUncounted(0); // We're only using 32 bit lengths
        _addUncounted(0); // But SHA-1 supports 64 bit lengths
        _addUncounted(0); // So zero pad the top bits
        _addUncounted(byteCount >> 29); // Shifting to multiply by 8
        _addUncounted(byteCount >> 21); // as SHA-1 supports bitstreams as well as
        _addUncounted(byteCount >> 13); // byte.
        _addUncounted(byteCount >> 5);
        _addUncounted(byteCount << 3);
    }

    static uint32_t _rol32(const uint32_t number, const uint8_t bits) noexcept
    {
        return (number << bits) | (number >> (32 - bits));
    }
};

#endif // CARLA_SHA1_UTILS_HPP_INCLUDED
