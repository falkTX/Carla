/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_STRING_HPP_INCLUDED
#define DISTRHO_STRING_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#include <algorithm>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// String class

class String
{
public:
    // -------------------------------------------------------------------
    // constructors (no explicit conversions allowed)

    /*
     * Empty string.
     */
    explicit String() noexcept
        : fBuffer(_null()),
          fBufferLen(0) {}

    /*
     * Simple character.
     */
    explicit String(const char c) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char ch[2];
        ch[0] = c;
        ch[1] = '\0';

        _dup(ch);
    }

    /*
     * Simple char string.
     */
    explicit String(char* const strBuf, const bool copyData = true) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        if (copyData || strBuf == nullptr)
        {
            _dup(strBuf);
        }
        else
        {
            fBuffer    = strBuf;
            fBufferLen = std::strlen(strBuf);
        }

    }

    /*
     * Simple const char string.
     */
    explicit String(const char* const strBuf) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        _dup(strBuf);
    }

    /*
     * Integer.
     */
    explicit String(const int value) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%d", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Unsigned integer, possibly in hexadecimal.
     */
    explicit String(const unsigned int value, const bool hexadecimal = false) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%x" : "%u", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long integer.
     */
    explicit String(const long value) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%ld", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long unsigned integer, possibly hexadecimal.
     */
    explicit String(const unsigned long value, const bool hexadecimal = false) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%lx" : "%lu", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long long integer.
     */
    explicit String(const long long value) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%lld", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long long unsigned integer, possibly hexadecimal.
     */
    explicit String(const unsigned long long value, const bool hexadecimal = false) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%llx" : "%llu", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Single-precision floating point number.
     */
    explicit String(const float value) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%f", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Double-precision floating point number.
     */
    explicit String(const double value) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%g", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    // -------------------------------------------------------------------
    // non-explicit constructor

    /*
     * Create string from another string.
     */
    String(const String& str) noexcept
        : fBuffer(_null()),
          fBufferLen(0)
    {
        _dup(str.fBuffer);
    }

    // -------------------------------------------------------------------
    // destructor

    /*
     * Destructor.
     */
    ~String() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        if (fBuffer == _null())
            return;

        std::free(fBuffer);

        fBuffer    = nullptr;
        fBufferLen = 0;
    }

    // -------------------------------------------------------------------
    // public methods

    /*
     * Get length of the string.
     */
    std::size_t length() const noexcept
    {
        return fBufferLen;
    }

    /*
     * Check if the string is empty.
     */
    bool isEmpty() const noexcept
    {
        return (fBufferLen == 0);
    }

    /*
     * Check if the string is not empty.
     */
    bool isNotEmpty() const noexcept
    {
        return (fBufferLen != 0);
    }

    /*
     * Check if the string contains another string, optionally ignoring case.
     */
    bool contains(const char* const strBuf, const bool ignoreCase = false) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(strBuf != nullptr, false);

        if (ignoreCase)
        {
#ifdef __USE_GNU
            return (strcasestr(fBuffer, strBuf) != nullptr);
#else
            String tmp1(fBuffer), tmp2(strBuf);

            // memory allocation failed or empty string(s)
            if (tmp1.fBuffer == _null() || tmp2.fBuffer == _null())
                return false;

            tmp1.toLower();
            tmp2.toLower();
            return (std::strstr(tmp1, tmp2) != nullptr);
#endif
        }

        return (std::strstr(fBuffer, strBuf) != nullptr);
    }

    /*
     * Check if character at 'pos' is a digit.
     */
    bool isDigit(const std::size_t pos) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(pos < fBufferLen, false);

        return (fBuffer[pos] >= '0' && fBuffer[pos] <= '9');
    }

    /*
     * Check if the string starts with the character 'c'.
     */
    bool startsWith(const char c) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(c != '\0', false);

        return (fBufferLen > 0 && fBuffer[0] == c);
    }

    /*
     * Check if the string starts with the string 'prefix'.
     */
    bool startsWith(const char* const prefix) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(prefix != nullptr, false);

        const std::size_t prefixLen(std::strlen(prefix));

        if (fBufferLen < prefixLen)
            return false;

        return (std::strncmp(fBuffer, prefix, prefixLen) == 0);
    }

    /*
     * Check if the string ends with the character 'c'.
     */
    bool endsWith(const char c) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(c != '\0', false);

        return (fBufferLen > 0 && fBuffer[fBufferLen-1] == c);
    }

    /*
     * Check if the string ends with the string 'suffix'.
     */
    bool endsWith(const char* const suffix) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(suffix != nullptr, false);

        const std::size_t suffixLen(std::strlen(suffix));

        if (fBufferLen < suffixLen)
            return false;

        return (std::strncmp(fBuffer + (fBufferLen-suffixLen), suffix, suffixLen) == 0);
    }

    /*
     * Find the first occurrence of character 'c' in the string.
     * Returns "length()" if the character is not found.
     */
    std::size_t find(const char c, bool* const found = nullptr) const noexcept
    {
        if (fBufferLen == 0 || c == '\0')
        {
            if (found != nullptr)
                *found = false;
            return fBufferLen;
        }

        for (std::size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] == c)
            {
                if (found != nullptr)
                    *found = true;
                return i;
            }
        }

        if (found != nullptr)
            *found = false;
        return fBufferLen;
    }

    /*
     * Find the first occurrence of string 'strBuf' in the string.
     * Returns "length()" if the string is not found.
     */
    std::size_t find(const char* const strBuf, bool* const found = nullptr) const noexcept
    {
        if (fBufferLen == 0 || strBuf == nullptr || strBuf[0] == '\0')
        {
            if (found != nullptr)
                *found = false;
            return fBufferLen;
        }

        if (char* const subStrBuf = std::strstr(fBuffer, strBuf))
        {
            const ssize_t ret(subStrBuf - fBuffer);

            if (ret < 0)
            {
                // should never happen!
                d_safe_assert("ret >= 0", __FILE__, __LINE__);

                if (found != nullptr)
                    *found = false;
                return fBufferLen;
            }

            if (found != nullptr)
                *found = true;
            return static_cast<std::size_t>(ret);
        }

        if (found != nullptr)
            *found = false;
        return fBufferLen;
    }

    /*
     * Find the last occurrence of character 'c' in the string.
     * Returns "length()" if the character is not found.
     */
    std::size_t rfind(const char c, bool* const found = nullptr) const noexcept
    {
        if (fBufferLen == 0 || c == '\0')
        {
            if (found != nullptr)
                *found = false;
            return fBufferLen;
        }

        for (std::size_t i=fBufferLen; i > 0; --i)
        {
            if (fBuffer[i-1] == c)
            {
                if (found != nullptr)
                    *found = true;
                return i-1;
            }
        }

        if (found != nullptr)
            *found = false;
        return fBufferLen;
    }

    /*
     * Find the last occurrence of string 'strBuf' in the string.
     * Returns "length()" if the string is not found.
     */
    std::size_t rfind(const char* const strBuf, bool* const found = nullptr) const noexcept
    {
        if (found != nullptr)
            *found = false;

        if (fBufferLen == 0 || strBuf == nullptr || strBuf[0] == '\0')
            return fBufferLen;

        const std::size_t strBufLen(std::strlen(strBuf));

        std::size_t ret = fBufferLen;
        const char* tmpBuf = fBuffer;

        for (std::size_t i=0; i < fBufferLen; ++i)
        {
            if (std::strstr(tmpBuf+1, strBuf) == nullptr && std::strncmp(tmpBuf, strBuf, strBufLen) == 0)
            {
                if (found != nullptr)
                    *found = true;
                break;
            }

            --ret;
            ++tmpBuf;
        }

        return fBufferLen-ret;
    }

    /*
     * Clear the string.
     */
    void clear() noexcept
    {
        truncate(0);
    }

    /*
     * Replace all occurrences of character 'before' with character 'after'.
     */
    String& replace(const char before, const char after) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(before != '\0' && after != '\0', *this);

        for (std::size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] == before)
                fBuffer[i] = after;
        }

        return *this;
    }

    /*
     * Truncate the string to size 'n'.
     */
    String& truncate(const std::size_t n) noexcept
    {
        if (n >= fBufferLen)
            return *this;

        for (std::size_t i=n; i < fBufferLen; ++i)
            fBuffer[i] = '\0';

        fBufferLen = n;

        return *this;
    }

    /*
     * Convert all non-basic characters to '_'.
     */
    String& toBasic() noexcept
    {
        for (std::size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= '0' && fBuffer[i] <= '9')
                continue;
            if (fBuffer[i] >= 'A' && fBuffer[i] <= 'Z')
                continue;
            if (fBuffer[i] >= 'a' && fBuffer[i] <= 'z')
                continue;
            if (fBuffer[i] == '_')
                continue;

            fBuffer[i] = '_';
        }

        return *this;
    }

    /*
     * Convert to all ascii characters to lowercase.
     */
    String& toLower() noexcept
    {
        static const char kCharDiff('a' - 'A');

        for (std::size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'A' && fBuffer[i] <= 'Z')
                fBuffer[i] = static_cast<char>(fBuffer[i] + kCharDiff);
        }

        return *this;
    }

    /*
     * Convert to all ascii characters to uppercase.
     */
    String& toUpper() noexcept
    {
        static const char kCharDiff('a' - 'A');

        for (std::size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'a' && fBuffer[i] <= 'z')
                fBuffer[i] = static_cast<char>(fBuffer[i] - kCharDiff);
        }

        return *this;
    }

    /*
     * Direct access to the string buffer (read-only).
     */
    const char* buffer() const noexcept
    {
        return fBuffer;
    }

    // -------------------------------------------------------------------
    // base64 stuff, based on http://www.adp-gmbh.ch/cpp/common/base64.html
    // Copyright (C) 2004-2008 René Nyffenegger

    static String asBase64(const void* const data, const std::size_t dataSize)
    {
        static const char* const kBase64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        const std::size_t kTmpBufSize = std::min(d_nextPowerOf2(dataSize/3), 65536U);

        const uchar* bytesToEncode((const uchar*)data);

        uint i=0, j=0;
        uint charArray3[3], charArray4[4];

        char strBuf[kTmpBufSize+1];
        strBuf[kTmpBufSize] = '\0';
        std::size_t strBufIndex = 0;

        String ret;

        for (std::size_t s=0; s<dataSize; ++s)
        {
            charArray3[i++] = *(bytesToEncode++);

            if (i == 3)
            {
                charArray4[0] =  (charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
                charArray4[3] =   charArray3[2] & 0x3f;

                for (i=0; i<4; ++i)
                    strBuf[strBufIndex++] = kBase64Chars[charArray4[i]];

                if (strBufIndex >= kTmpBufSize-7)
                {
                    strBuf[strBufIndex] = '\0';
                    strBufIndex = 0;
                    ret += strBuf;
                }

                i = 0;
            }
        }

        if (i != 0)
        {
            for (j=i; j<3; ++j)
              charArray3[j] = '\0';

            charArray4[0] =  (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] =   charArray3[2] & 0x3f;

            for (j=0; j<4 && i<3 && j<i+1; ++j)
                strBuf[strBufIndex++] = kBase64Chars[charArray4[j]];

            for (; i++ < 3;)
                strBuf[strBufIndex++] = '=';
        }

        if (strBufIndex != 0)
        {
            strBuf[strBufIndex] = '\0';
            ret += strBuf;
        }

        return ret;
    }

    // -------------------------------------------------------------------
    // public operators

    operator const char*() const noexcept
    {
        return fBuffer;
    }

    char operator[](const std::size_t pos) const noexcept
    {
        if (pos < fBufferLen)
            return fBuffer[pos];

        d_safe_assert("pos < fBufferLen", __FILE__, __LINE__);

        static char fallback;
        fallback = '\0';
        return fallback;
    }

    char& operator[](const std::size_t pos) noexcept
    {
        if (pos < fBufferLen)
            return fBuffer[pos];

        d_safe_assert("pos < fBufferLen", __FILE__, __LINE__);

        static char fallback;
        fallback = '\0';
        return fallback;
    }

    bool operator==(const char* const strBuf) const noexcept
    {
        return (strBuf != nullptr && std::strcmp(fBuffer, strBuf) == 0);
    }

    bool operator==(const String& str) const noexcept
    {
        return operator==(str.fBuffer);
    }

    bool operator!=(const char* const strBuf) const noexcept
    {
        return !operator==(strBuf);
    }

    bool operator!=(const String& str) const noexcept
    {
        return !operator==(str.fBuffer);
    }

    String& operator=(const char* const strBuf) noexcept
    {
        _dup(strBuf);

        return *this;
    }

    String& operator=(const String& str) noexcept
    {
        _dup(str.fBuffer);

        return *this;
    }

    String& operator+=(const char* const strBuf) noexcept
    {
        if (strBuf == nullptr)
            return *this;

        const std::size_t newBufSize = fBufferLen + std::strlen(strBuf) + 1;
        char              newBuf[newBufSize];

        std::strcpy(newBuf, fBuffer);
        std::strcat(newBuf, strBuf);

        _dup(newBuf, newBufSize-1);

        return *this;
    }

    String& operator+=(const String& str) noexcept
    {
        return operator+=(str.fBuffer);
    }

    String operator+(const char* const strBuf) noexcept
    {
        const std::size_t newBufSize = fBufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char              newBuf[newBufSize];

        std::strcpy(newBuf, fBuffer);

        if (strBuf != nullptr)
            std::strcat(newBuf, strBuf);

        return String(newBuf);
    }

    String operator+(const String& str) noexcept
    {
        return operator+(str.fBuffer);
    }

    // -------------------------------------------------------------------

private:
    char*       fBuffer;    // the actual string buffer
    std::size_t fBufferLen; // string length

    /*
     * Static null string.
     * Prevents allocation for new and/or empty strings.
     */
    static char* _null() noexcept
    {
        static char sNull = '\0';
        return &sNull;
    }

    /*
     * Helper function.
     * Called whenever the string needs to be allocated.
     *
     * Notes:
     * - Allocates string only if 'strBuf' is not null and new string contents are different
     * - If 'strBuf' is null, 'size' must be 0
     */
    void _dup(const char* const strBuf, const std::size_t size = 0) noexcept
    {
        if (strBuf != nullptr)
        {
            // don't recreate string if contents match
            if (std::strcmp(fBuffer, strBuf) == 0)
                return;

            if (fBuffer != _null())
                std::free(fBuffer);

            fBufferLen = (size > 0) ? size : std::strlen(strBuf);
            fBuffer    = (char*)std::malloc(fBufferLen+1);

            if (fBuffer == nullptr)
            {
                fBuffer    = _null();
                fBufferLen = 0;
                return;
            }

            std::strcpy(fBuffer, strBuf);

            fBuffer[fBufferLen] = '\0';
        }
        else
        {
            DISTRHO_SAFE_ASSERT(size == 0);

            // don't recreate null string
            if (fBuffer == _null())
                return;

            DISTRHO_SAFE_ASSERT(fBuffer != nullptr);
            std::free(fBuffer);

            fBuffer    = _null();
            fBufferLen = 0;
        }
    }

    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

static inline
String operator+(const String& strBefore, const char* const strBufAfter) noexcept
{
    const char* const strBufBefore = strBefore.buffer();
    const std::size_t newBufSize   = strBefore.length() + ((strBufAfter != nullptr) ? std::strlen(strBufAfter) : 0) + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return String(newBuf);
}

static inline
String operator+(const char* const strBufBefore, const String& strAfter) noexcept
{
    const char* const strBufAfter = strAfter.buffer();
    const std::size_t newBufSize  = ((strBufBefore != nullptr) ? std::strlen(strBufBefore) : 0) + strAfter.length() + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return String(newBuf);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_STRING_HPP_INCLUDED
