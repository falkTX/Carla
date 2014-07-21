/*
 * Carla base64 utils, based on http://www.adp-gmbh.ch/cpp/common/base64.html
 * Copyright (C) 2004-2008 René Nyffenegger
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BASE64_UTILS_HPP_INCLUDED
#define CARLA_BASE64_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <cctype>
#include <vector>

// -----------------------------------------------------------------------
// Helpers

namespace CarlaBase64Helpers {

static const char* const kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline
uint8_t findBase64CharIndex(const char c)
{
    static const uint8_t kBase64CharsLen(std::strlen(kBase64Chars));

    for (uint8_t i=0; i<kBase64CharsLen; ++i)
    {
        if (kBase64Chars[i] == c)
            return i;
    }

    return 0;
}

static inline
bool isBase64Char(const char c)
{
    return (std::isalnum(c) || (c == '+') || (c == '/'));
}

}

// -----------------------------------------------------------------------

static inline
std::vector<uint8_t> carla_getChunkFromBase64String(const char* const base64string)
{
    CARLA_SAFE_ASSERT_RETURN(base64string != nullptr, std::vector<uint8_t>());

    uint i=0, j=0;
    uint charArray3[3], charArray4[4];

    std::vector<uint8_t> ret;
    ret.reserve(std::strlen(base64string)*3/4 + 4);

    for (std::size_t l=0, len=std::strlen(base64string); l<len && base64string[l] != '=' && CarlaBase64Helpers::isBase64Char(base64string[l]); ++l)
    {
        charArray4[i++] = static_cast<uint>(base64string[l]);

        if (i == 4)
        {
            for (i=0; i<4; ++i)
                charArray4[i] = CarlaBase64Helpers::findBase64CharIndex(static_cast<char>(charArray4[i]));

            charArray3[0] =  (charArray4[0] << 2)        + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) +   charArray4[3];

            for (i=0; i<3; ++i)
                ret.push_back(static_cast<uint8_t>(charArray3[i]));

            i = 0;
        }
    }

    if (i != 0)
    {
        for (j=i; j<4; ++j)
            charArray4[j] = 0;

        for (j=0; j<4; ++j)
            charArray4[j] = CarlaBase64Helpers::findBase64CharIndex(static_cast<char>(charArray4[j]));

        charArray3[0] =  (charArray4[0] << 2)        + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
        charArray3[2] = ((charArray4[2] & 0x3) << 6) +   charArray4[3];

        for (j=0; i>0 && j<i-1; j++)
            ret.push_back(static_cast<uint8_t>(charArray3[j]));
    }

    return ret;
}

// -----------------------------------------------------------------------

#endif // CARLA_BASE64_UTILS_HPP_INCLUDED
