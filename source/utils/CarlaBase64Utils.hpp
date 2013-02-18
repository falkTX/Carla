/*
 * Carla base64 utils imported from qemu source code
 * Copyright (C) 2009 Michael Brown <mbrown@fensystems.co.uk>
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_BASE64_UTILS_HPP__
#define __CARLA_BASE64_UTILS_HPP__

#include "CarlaUtils.hpp"

#include <cctype>
#include <cstdint>

static const char kBase64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Calculate length of base64-encoded data
 *
 * @v   rawLen        Raw data length
 * @ret encodedLen    Encoded string length (excluding NUL)
 */
static inline
size_t carla_base64_encoded_len(const size_t rawLen)
{
    return (((rawLen + 3 - 1) / 3) * 4);
}

/**
 * Calculate maximum length of base64-decoded string
 *
 * @v encoded         Encoded string
 * @v maxRawLen       Maximum length of raw data
 *
 * Note that the exact length of the raw data cannot be known until
 * the string is decoded.
 */
static inline
size_t carla_base64_decoded_max_len(const char* const encoded)
{
    return (((std::strlen(encoded) + 4 - 1) / 4) * 3);
}

/**
 * Base64-encode data
 *
 * @v raw             Raw data
 * @v len             Length of raw data
 * @v encoded         Buffer for encoded string
 *
 * The buffer must be the correct length for the encoded string.
 * Use something like
 *
 *     char buf[carla_base64_encoded_len(len) + 1];
 *
 * (the +1 is for the terminating NUL) to provide a buffer of the correct size.
 */
static inline
void carla_base64_encode(const uint8_t* const raw, const size_t len, char* const encoded)
{
    const uint8_t* rawBytes = (const uint8_t*)raw;
    uint8_t*   encodedBytes = (uint8_t*)encoded;
    size_t     rawBitLen    = 8*len;
    unsigned int bit;
    unsigned int tmp;

    for (bit = 0; bit < rawBitLen; bit += 6)
    {
        tmp = (rawBytes[bit/8] << (bit % 8)) | (rawBytes[bit/8 + 1] >> (8 - (bit % 8)));
        tmp = (tmp >> 2) & 0x3f;

        *(encodedBytes++) = kBase64[tmp];
    }

    for (; (bit % 8) != 0; bit += 6)
        *(encodedBytes++) = '=';

    *(encodedBytes++) = '\0';

    carla_debug("Base64-encoded to \"%s\":\n", encoded);
    CARLA_ASSERT(std::strlen(encoded) == carla_base64_encoded_len(len));
}

/**
 * Base64-decode string
 *
 * @v encoded         Encoded string
 * @v raw             Raw data
 * @ret len           Length of raw data, or 0
 *
 * The buffer must be large enough to contain the decoded data.
 * Use something like
 *
 *     char buf[carla_base64_decoded_max_len(encoded)];
 *
 * to provide a buffer of the correct size.
 */
static inline
unsigned int carla_base64_decode(const char* const encoded, uint8_t* const raw)
{
    const uint8_t* encodedBytes = (const uint8_t*)encoded;
    uint8_t* rawBytes = (uint8_t*)raw;
    uint8_t encodedByte;
    unsigned int bit = 0;
    unsigned int padCount = 0;

    /* Zero the raw data */
    std::memset(raw, 0, carla_base64_decoded_max_len(encoded));

    /* Decode string */
    while ((encodedByte = *(encodedBytes++)) > 0)
    {
        /* Ignore whitespace characters */
        if (std::isspace(encodedByte))
            continue;

        /* Process pad characters */
        if (encodedByte == '=')
        {
            if (padCount >= 2)
            {
                carla_debug("Base64-encoded string \"%s\" has too many pad characters", encoded);
                return 0;
            }

            padCount++;
            bit -= 2; /* unused_bits = ( 2 * padCount ) */
            continue;
        }

        if (padCount)
        {
            carla_debug("Base64-encoded string \"%s\" has invalid pad sequence", encoded);
            return 0;
        }

        /* Process normal characters */
        const char* match = std::strchr(kBase64, encodedByte);

        if (match == nullptr)
        {
            carla_debug("Base64-encoded string \"%s\" contains invalid character '%c'", encoded, encodedByte);
            return 0;
        }

        int decoded = match - kBase64;

        /* Add to raw data */
        decoded <<= 2;
        rawBytes[bit / 8]     |= (decoded >> (bit % 8));
        rawBytes[bit / 8 + 1] |= (decoded << (8 - (bit % 8)));
        bit += 6;
    }

    /* Check that we decoded a whole number of bytes */
    if ((bit % 8) != 0)
    {
        carla_debug("Base64-encoded string \"%s\" has invalid bit length %d", encoded, bit);
        return 0;
    }

    unsigned int len = bit/8;

    carla_debug("Base64-decoded \"%s\" to: \"%s\"\n", encoded, raw);
    CARLA_ASSERT(len <= carla_base64_decoded_max_len(encoded));

    /* Return length in bytes */
    return len;
}

#endif // __CARLA_BASE64_UTILS_HPP__
