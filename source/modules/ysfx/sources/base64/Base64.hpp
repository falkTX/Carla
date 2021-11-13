/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2022 Jean Pierre Cimalando <jp-dev@inbox.ru>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once
#include <cstdint>
#include <cstring>
#include <cassert>
#include <array>
#include <vector>

// -----------------------------------------------------------------------
// base64 stuff, based on http://www.adp-gmbh.ch/cpp/common/base64.html

/*
   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/

// -----------------------------------------------------------------------
// Helpers

#ifndef DOXYGEN
namespace DistrhoBase64Helpers {

static
std::array<int8_t, 256> createCharIndexTable()
{
    std::array<int8_t, 256> table;
    table.fill(-1);
    int8_t index = 0;
    for (uint8_t c = 'A'; c <= 'Z'; ++c) table[c] = index++;
    for (uint8_t c = 'a'; c <= 'z'; ++c) table[c] = index++;
    for (uint8_t c = '0'; c <= '9'; ++c) table[c] = index++;
    table['+'] = index++;
    table['/'] = index++;
    return table;
}

static const std::array<int8_t, 256> kCharIndexTable = createCharIndexTable();

} // namespace DistrhoBase64Helpers
#endif

// -----------------------------------------------------------------------

static inline
std::vector<uint8_t> d_getChunkFromBase64String(const char* const base64string, std::size_t base64stringLen = ~std::size_t(0))
{
    std::vector<uint8_t> ret;

    if (! base64string)
        return ret;

    uint32_t i=0, j=0;
    uint32_t charArray3[3], charArray4[4];

    if (base64stringLen == ~std::size_t(0))
        base64stringLen = std::strlen(base64string);

    ret.reserve(base64stringLen*3/4 + 4);

    for (std::size_t l=0; l<base64stringLen; ++l)
    {
        const unsigned char c = base64string[l];

        if (c == '\0' || c == '=')
            break;
        if (DistrhoBase64Helpers::kCharIndexTable[c] == -1)
            continue;

        charArray4[i++] = static_cast<uint32_t>(c);

        if (i == 4)
        {
            for (i=0; i<4; ++i)
                charArray4[i] = static_cast<uint8_t>(DistrhoBase64Helpers::kCharIndexTable[charArray4[i]]);

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
        for (j=0; j<i && j<4; ++j)
            charArray4[j] = static_cast<uint8_t>(DistrhoBase64Helpers::kCharIndexTable[charArray4[j]]);

        for (j=i; j<4; ++j)
            charArray4[j] = 0;

        charArray3[0] =  (charArray4[0] << 2)        + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
        charArray3[2] = ((charArray4[2] & 0x3) << 6) +   charArray4[3];

        for (j=0; i>0 && j<i-1; j++)
            ret.push_back(static_cast<uint8_t>(charArray3[j]));
    }

    return ret;
}
