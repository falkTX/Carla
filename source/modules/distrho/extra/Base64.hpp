/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_BASE64_HPP_INCLUDED
#define DISTRHO_BASE64_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#include <vector>

// --------------------------------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------------------------------
// Helpers

#ifndef DOXYGEN
namespace DistrhoBase64Helpers {

static constexpr const char* const kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline
uint8_t findBase64CharIndex(const char c)
{
    static const uint8_t kBase64CharsLen = static_cast<uint8_t>(std::strlen(kBase64Chars));

    for (uint8_t i=0; i<kBase64CharsLen; ++i)
    {
        if (kBase64Chars[i] == c)
            return i;
    }

    d_stderr2("findBase64CharIndex('%c') - failed", c);
    return 0;
}

static constexpr inline
bool isBase64Char(const char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/';
}

} // namespace DistrhoBase64Helpers
#endif

// --------------------------------------------------------------------------------------------------------------------

static inline
void d_getChunkFromBase64String_impl(std::vector<uint8_t>& vector, const char* const base64string)
{
    vector.clear();
    DISTRHO_SAFE_ASSERT_RETURN(base64string != nullptr,);

    uint i=0, j=0;
    uint charArray3[3], charArray4[4];

    vector.reserve(std::strlen(base64string)*3/4 + 4);

    for (std::size_t l=0, len=std::strlen(base64string); l<len; ++l)
    {
        const char c = base64string[l];

        if (c == '\0' || c == '=')
            break;
        if (c == ' ' || c == '\n')
            continue;

        DISTRHO_SAFE_ASSERT_CONTINUE(DistrhoBase64Helpers::isBase64Char(c));

        charArray4[i++] = static_cast<uint>(c);

        if (i == 4)
        {
            for (i=0; i<4; ++i)
                charArray4[i] = DistrhoBase64Helpers::findBase64CharIndex(static_cast<char>(charArray4[i]));

            charArray3[0] =  (charArray4[0] << 2)        + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) +   charArray4[3];

            for (i=0; i<3; ++i)
                vector.push_back(static_cast<uint8_t>(charArray3[i]));

            i = 0;
        }
    }

    if (i != 0)
    {
        for (j=0; j<i && j<4; ++j)
            charArray4[j] = DistrhoBase64Helpers::findBase64CharIndex(static_cast<char>(charArray4[j]));

        for (j=i; j<4; ++j)
            charArray4[j] = 0;

        charArray3[0] =  (charArray4[0] << 2)        + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
        charArray3[2] = ((charArray4[2] & 0x3) << 6) +   charArray4[3];

        for (j=0; i>0 && j<i-1; j++)
            vector.push_back(static_cast<uint8_t>(charArray3[j]));
    }
}

static inline
std::vector<uint8_t> d_getChunkFromBase64String(const char* const base64string)
{
    std::vector<uint8_t> ret;
    d_getChunkFromBase64String_impl(ret, base64string);
    return ret;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // DISTRHO_BASE64_HPP_INCLUDED
