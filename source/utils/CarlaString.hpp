/*
 * Carla String
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_STRING_HPP_INCLUDED
#define CARLA_STRING_HPP_INCLUDED

#include "CarlaMathUtils.hpp"
#include "CarlaScopeUtils.hpp"

#include <algorithm>

// -----------------------------------------------------------------------
// CarlaString class

class CARLA_API CarlaString
{
public:
    // -------------------------------------------------------------------
    // constructors (no explicit conversions allowed)

    /*
     * Empty string.
     */
    explicit CarlaString() noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false) {}

    /*
     * Simple character.
     */
    explicit CarlaString(const char c) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char ch[2];
        ch[0] = c;
        ch[1] = '\0';

        _dup(ch);
    }

    /*
     * Simple char string.
     */
    explicit CarlaString(char* const strBuf, const bool reallocData = true) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        if (reallocData || strBuf == nullptr)
        {
            _dup(strBuf);
        }
        else
        {
            fBuffer      = strBuf;
            fBufferLen   = std::strlen(strBuf);
            fBufferAlloc = true;
        }
    }

    /*
     * Simple const char string.
     */
    explicit CarlaString(const char* const strBuf) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        _dup(strBuf);
    }

    /*
     * Integer.
     */
    explicit CarlaString(const int value) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%d", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Unsigned integer, possibly in hexadecimal.
     */
    explicit CarlaString(const unsigned int value, const bool hexadecimal = false) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%x" : "%u", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long integer.
     */
    explicit CarlaString(const long value) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%ld", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long unsigned integer, possibly hexadecimal.
     */
    explicit CarlaString(const unsigned long value, const bool hexadecimal = false) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%lx" : "%lu", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long long integer.
     */
    explicit CarlaString(const long long value) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, "%lld", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Long long unsigned integer, possibly hexadecimal.
     */
    explicit CarlaString(const unsigned long long value, const bool hexadecimal = false) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%llx" : "%llu", value);
        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Single-precision floating point number.
     */
    explicit CarlaString(const float value) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];

        {
            const CarlaScopedLocale csl;
            std::snprintf(strBuf, 0xff, "%.12g", static_cast<double>(value));
        }

        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    /*
     * Double-precision floating point number.
     */
    explicit CarlaString(const double value) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        char strBuf[0xff+1];

        {
            const CarlaScopedLocale csl;
            std::snprintf(strBuf, 0xff, "%.24g", value);
        }

        strBuf[0xff] = '\0';

        _dup(strBuf);
    }

    // -------------------------------------------------------------------
    // non-explicit constructor

    /*
     * Create string from another string.
     */
    CarlaString(const CarlaString& str) noexcept
        : fBuffer(_null()),
          fBufferLen(0),
          fBufferAlloc(false)
    {
        _dup(str.fBuffer);
    }

    // -------------------------------------------------------------------
    // destructor

    /*
     * Destructor.
     */
    ~CarlaString() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        if (fBufferAlloc)
            std::free(fBuffer);

        fBuffer      = nullptr;
        fBufferLen   = 0;
        fBufferAlloc = false;
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
     * Check if the string contains a specific character, case-sensitive.
     */
    bool contains(const char c) const noexcept
    {
        for (std::size_t i=0; i<fBufferLen; ++i)
        {
            if (fBuffer[i] == c)
                return true;
        }

        return false;
    }

    /*
     * Check if the string contains another string, optionally ignoring case.
     */
    bool contains(const char* const strBuf, const bool ignoreCase = false) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(strBuf != nullptr, false);

        if (ignoreCase)
        {
#ifdef __USE_GNU
            return (strcasestr(fBuffer, strBuf) != nullptr);
#else
            CarlaString tmp1(fBuffer), tmp2(strBuf);

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
        CARLA_SAFE_ASSERT_RETURN(pos < fBufferLen, false);

        return (fBuffer[pos] >= '0' && fBuffer[pos] <= '9');
    }

    /*
     * Check if the string starts with the character 'c'.
     */
    bool startsWith(const char c) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(c != '\0', false);

        return (fBufferLen > 0 && fBuffer[0] == c);
    }

    /*
     * Check if the string starts with the string 'prefix'.
     */
    bool startsWith(const char* const prefix) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(prefix != nullptr, false);

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
        CARLA_SAFE_ASSERT_RETURN(c != '\0', false);

        return (fBufferLen > 0 && fBuffer[fBufferLen-1] == c);
    }

    /*
     * Check if the string ends with the string 'suffix'.
     */
    bool endsWith(const char* const suffix) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(suffix != nullptr, false);

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
            const ssize_t ret = subStrBuf - fBuffer;

            if (ret < 0)
            {
                // should never happen!
                carla_safe_assert_int("ret >= 0", __FILE__, __LINE__, int(ret));

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
    CarlaString& replace(const char before, const char after) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(before != '\0' && after != '\0', *this);

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
    CarlaString& truncate(const std::size_t n) noexcept
    {
        if (n >= fBufferLen)
            return *this;

        fBuffer[n] = '\0';
        fBufferLen = n;

        return *this;
    }

    /*
     * Convert all non-basic characters to '_'.
     */
    CarlaString& toBasic() noexcept
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
     * Convert all ascii characters to lowercase.
     */
    CarlaString& toLower() noexcept
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
     * Convert all ascii characters to uppercase.
     */
    CarlaString& toUpper() noexcept
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

    /*
     * Return a duplicate string buffer.
     * May throw.
     */
    const char* dup() const
    {
        return carla_strdup(fBuffer);
    }

    /*
     * Return a duplicate string buffer or null.
     */
    const char* dupSafe() const noexcept
    {
        return carla_strdup_safe(fBuffer);
    }

    /*
     * Release the buffer pointer while clearing this string.
     * This allows to keep a pointer to the buffer after this object is deleted.
     */
    char* releaseBufferPointer() noexcept
    {
        char* ret = fBufferLen > 0 ? fBuffer : nullptr;
        fBuffer = _null();
        fBufferLen = 0;
        fBufferAlloc = false;
        return ret;
    }

    // -------------------------------------------------------------------
    // base64 stuff, based on http://www.adp-gmbh.ch/cpp/common/base64.html
    // Copyright (C) 2004-2008 René Nyffenegger

    static CarlaString asBase64(const void* const data, const std::size_t dataSize)
    {
        static const char* const kBase64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        static constexpr const std::size_t kTmpBufSize = 65536U;

        const uchar* bytesToEncode((const uchar*)data);

        uint i=0, j=0;
        uint charArray3[3], charArray4[4];

        char strBuf[kTmpBufSize+1];
        strBuf[kTmpBufSize] = '\0';
        std::size_t strBufIndex = 0;

        CarlaString ret;

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

        carla_safe_assert("pos < fBufferLen", __FILE__, __LINE__);

        static char fallback;
        fallback = '\0';
        return fallback;
    }

    char& operator[](const std::size_t pos) noexcept
    {
        if (pos < fBufferLen)
            return fBuffer[pos];

        carla_safe_assert("pos < fBufferLen", __FILE__, __LINE__);

        static char fallback;
        fallback = '\0';
        return fallback;
    }

    bool operator==(const char* const strBuf) const noexcept
    {
        return (strBuf != nullptr && std::strcmp(fBuffer, strBuf) == 0);
    }

    bool operator==(const CarlaString& str) const noexcept
    {
        return operator==(str.fBuffer);
    }

    bool operator!=(const char* const strBuf) const noexcept
    {
        return !operator==(strBuf);
    }

    bool operator!=(const CarlaString& str) const noexcept
    {
        return !operator==(str.fBuffer);
    }

    CarlaString& operator=(const char* const strBuf) noexcept
    {
        _dup(strBuf);

        return *this;
    }

    CarlaString& operator=(const CarlaString& str) noexcept
    {
        _dup(str.fBuffer);

        return *this;
    }

    CarlaString& operator+=(const char* const strBuf) noexcept
    {
        if (strBuf == nullptr || strBuf[0] == '\0')
            return *this;

        const std::size_t strBufLen = std::strlen(strBuf);

        // for empty strings, we can just take the appended string as our entire data
        if (isEmpty())
        {
            _dup(strBuf, strBufLen);
            return *this;
        }

        // we have some data ourselves, reallocate to add the new stuff
        char* const newBuf = (char*)realloc(fBuffer, fBufferLen + strBufLen + 1);
        CARLA_SAFE_ASSERT_RETURN(newBuf != nullptr, *this);

        std::memcpy(newBuf + fBufferLen, strBuf, strBufLen + 1);

        fBuffer = newBuf;
        fBufferLen += strBufLen;

        return *this;
    }

    CarlaString& operator+=(const CarlaString& str) noexcept
    {
        return operator+=(str.fBuffer);
    }

    CarlaString operator+(const char* const strBuf) noexcept
    {
        if (strBuf == nullptr || strBuf[0] == '\0')
            return *this;
        if (isEmpty())
            return CarlaString(strBuf);

        const std::size_t strBufLen = std::strlen(strBuf);
        const std::size_t newBufSize = fBufferLen + strBufLen;
        char* const newBuf = (char*)std::malloc(newBufSize + 1);
        CARLA_SAFE_ASSERT_RETURN(newBuf != nullptr, CarlaString());

        std::memcpy(newBuf, fBuffer, fBufferLen);
        std::memcpy(newBuf + fBufferLen, strBuf, strBufLen + 1);

        return CarlaString(newBuf, false);
    }

    CarlaString operator+(const CarlaString& str) noexcept
    {
        return operator+(str.fBuffer);
    }

    // needed for std::map compatibility
    bool operator<(const CarlaString& str) const noexcept
    {
        return std::strcmp(fBuffer, str.fBuffer) < 0;
    }

    // -------------------------------------------------------------------

private:
    char*       fBuffer;      // the actual string buffer
    std::size_t fBufferLen;   // string length
    bool        fBufferAlloc; // wherever the buffer is allocated, not using _null()

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

            if (fBufferAlloc)
                std::free(fBuffer);

            fBufferLen = (size > 0) ? size : std::strlen(strBuf);
            fBuffer    = (char*)std::malloc(fBufferLen+1);

            if (fBuffer == nullptr)
            {
                fBuffer      = _null();
                fBufferLen   = 0;
                fBufferAlloc = false;
                return;
            }

            fBufferAlloc = true;

            std::strcpy(fBuffer, strBuf);
            fBuffer[fBufferLen] = '\0';
        }
        else
        {
            CARLA_SAFE_ASSERT_UINT(size == 0, static_cast<uint>(size));

            // don't recreate null string
            if (! fBufferAlloc)
                return;

            CARLA_SAFE_ASSERT(fBuffer != nullptr);
            std::free(fBuffer);

            fBuffer      = _null();
            fBufferLen   = 0;
            fBufferAlloc = false;
        }
    }

    CARLA_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

static inline
CarlaString operator+(const CarlaString& strBefore, const char* const strBufAfter) noexcept
{
    if (strBufAfter == nullptr || strBufAfter[0] == '\0')
        return strBefore;
    if (strBefore.isEmpty())
        return CarlaString(strBufAfter);

    const std::size_t strBeforeLen = strBefore.length();
    const std::size_t strBufAfterLen = std::strlen(strBufAfter);
    const std::size_t newBufSize = strBeforeLen + strBufAfterLen;
    char* const newBuf = (char*)std::malloc(newBufSize + 1);
    CARLA_SAFE_ASSERT_RETURN(newBuf != nullptr, CarlaString());

    std::memcpy(newBuf, strBefore.buffer(), strBeforeLen);
    std::memcpy(newBuf + strBeforeLen, strBufAfter, strBufAfterLen + 1);

    return CarlaString(newBuf, false);
}

static inline
CarlaString operator+(const char* const strBufBefore, const CarlaString& strAfter) noexcept
{
    if (strAfter.isEmpty())
        return CarlaString(strBufBefore);
    if (strBufBefore == nullptr || strBufBefore[0] == '\0')
        return strAfter;

    const std::size_t strBufBeforeLen = std::strlen(strBufBefore);
    const std::size_t strAfterLen = strAfter.length();
    const std::size_t newBufSize = strBufBeforeLen + strAfterLen;
    char* const newBuf = (char*)std::malloc(newBufSize + 1);
    CARLA_SAFE_ASSERT_RETURN(newBuf != nullptr, CarlaString());

    std::memcpy(newBuf, strBufBefore, strBufBeforeLen);
    std::memcpy(newBuf + strBufBeforeLen, strAfter.buffer(), strAfterLen + 1);

    return CarlaString(newBuf, false);
}

// -----------------------------------------------------------------------

#endif // CARLA_STRING_HPP_INCLUDED
