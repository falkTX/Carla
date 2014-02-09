/*
 * Carla String
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaJuceUtils.hpp"

#ifdef CARLA_OS_HAIKU
namespace std {
	using ::snprintf;
}
#endif

// -----------------------------------------------------------------------
// CarlaString class

class CarlaString
{
public:
    // -------------------------------------------------------------------
    // constructors (no explicit conversions allowed)

    /*
     * Empty string.
     */
    explicit CarlaString()
    {
        _init();
        _dup(nullptr);
    }

    /*
     * Simple character.
     */
    explicit CarlaString(const char c)
    {
        char ch[2];
        ch[0] = c;
        ch[1] = '\0';

        _init();
        _dup(ch);
    }

    /*
     * Simple char string.
     */
    explicit CarlaString(char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    /*
     * Simple const char string.
     */
    explicit CarlaString(const char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    /*
     * Integer.
     */
    explicit CarlaString(const int value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%d", value);

        _init();
        _dup(strBuf);
    }

    /*
     * Unsigned integer, possibly in hexadecimal.
     */
    explicit CarlaString(const unsigned int value, const bool hexadecimal = false)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%x" : "%u", value);

        _init();
        _dup(strBuf);
    }

    /*
     * Long integer.
     */
    explicit CarlaString(const long int value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%ld", value);

        _init();
        _dup(strBuf);
    }

    /*
     * Long unsigned integer, possibly hexadecimal.
     */
    explicit CarlaString(const unsigned long int value, const bool hexadecimal = false)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%lx" : "%lu", value);

        _init();
        _dup(strBuf);
    }

    /*
     * Long long integer.
     */
    explicit CarlaString(const long long int value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%lld", value);

        _init();
        _dup(strBuf);
    }

    /*
     * Long long unsigned integer, possibly hexadecimal.
     */
    explicit CarlaString(const unsigned long long int value, const bool hexadecimal = false)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%llx" : "%llu", value);

        _init();
        _dup(strBuf);
    }

    /*
     * Single-precision floating point number.
     */
    explicit CarlaString(const float value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%f", value);

        _init();
        _dup(strBuf);
    }

    /*
     * Double-precision floating point number.
     */
    explicit CarlaString(const double value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%g", value);

        _init();
        _dup(strBuf);
    }

    // -------------------------------------------------------------------
    // non-explicit constructor

    /*
     * Create string from another string.
     */
    CarlaString(const CarlaString& str)
    {
        _init();
        _dup(str.fBuffer);
    }

    // -------------------------------------------------------------------
    // destructor

    /*
     * Destructor.
     */
    ~CarlaString()
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        delete[] fBuffer;
        fBuffer = nullptr;
    }

    // -------------------------------------------------------------------
    // public methods

    /*
     * Get length of the string.
     */
    size_t length() const noexcept
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
    bool contains(const char* const strBuf, const bool ignoreCase = false) const
    {
        CARLA_SAFE_ASSERT_RETURN(strBuf != nullptr, false);

        if (ignoreCase)
        {
#ifdef __USE_GNU
            return (strcasestr(fBuffer, strBuf) != nullptr);
#else
            CarlaString tmp1(fBuffer), tmp2(strBuf);
            tmp1.toLower();
            tmp2.toLower();
            return (std::strstr((const char*)tmp1, (const char*)tmp2) != nullptr);
#endif
        }

        return (std::strstr(fBuffer, strBuf) != nullptr);
    }

    /*
     * Overloaded function.
     */
    bool contains(const CarlaString& str, const bool ignoreCase = false) const
    {
        return contains(str.fBuffer, ignoreCase);
    }

    /*
     * Check if character at 'pos' is a digit.
     */
    bool isDigit(const size_t pos) const noexcept
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

        const size_t prefixLen(std::strlen(prefix));

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

        const size_t suffixLen(std::strlen(suffix));

        if (fBufferLen < suffixLen)
            return false;

        return (std::strncmp(fBuffer + (fBufferLen-suffixLen), suffix, suffixLen) == 0);
    }

    /*
     * Find the first occurrence of character 'c' in the string.
     * Returns "length()" if the character is not found.
     */
    size_t find(const char c, bool* const found = nullptr) const noexcept
    {
        if (fBufferLen == 0 || c == '\0')
        {
            if (found != nullptr)
                *found = false;
            return fBufferLen;
        }

        for (size_t i=0; i < fBufferLen; ++i)
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
    size_t find(const char* const strBuf, bool* const found = nullptr) const noexcept
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
                carla_safe_assert_int("ret >= 0", __FILE__, __LINE__, int(ret));

                if (found != nullptr)
                    *found = false;
                return fBufferLen;
            }

            if (found != nullptr)
                *found = true;
            return static_cast<size_t>(ret);
        }

        if (found != nullptr)
            *found = false;
        return fBufferLen;
    }

    /*
     * Find the last occurrence of character 'c' in the string.
     * Returns "length()" if the character is not found.
     */
    size_t rfind(const char c, bool* const found = nullptr) const noexcept
    {
        if (fBufferLen == 0 || c == '\0')
        {
            if (found != nullptr)
                *found = false;
            return fBufferLen;
        }

        for (size_t i=fBufferLen; i > 0; --i)
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
    size_t rfind(const char* const strBuf, bool* const found = nullptr) const noexcept
    {
        if (found != nullptr)
            *found = false;

        if (fBufferLen == 0 || strBuf == nullptr || strBuf[0] == '\0')
            return fBufferLen;

        const size_t strBufLen(std::strlen(strBuf));

        size_t ret = fBufferLen;
        const char* tmpBuf = fBuffer;

        for (size_t i=0; i < fBufferLen; ++i)
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
    void replace(const char before, const char after) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(before != '\0' && after != '\0',);

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] == before)
                fBuffer[i] = after;
            else if (fBuffer[i] == '\0')
                break;
        }
    }

    /*
     * Truncate the string to size 'n'.
     */
    void truncate(const size_t n) noexcept
    {
        if (n >= fBufferLen)
            return;

        for (size_t i=n; i < fBufferLen; ++i)
            fBuffer[i] = '\0';

        fBufferLen = n;
    }

    /*
     * Convert all non-basic characters to '_'.
     */
    void toBasic() noexcept
    {
        for (size_t i=0; i < fBufferLen; ++i)
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
    }

    /*
     * Convert to all ascii characters to lowercase.
     */
    void toLower() noexcept
    {
        static const char kCharDiff('a' - 'A');

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'A' && fBuffer[i] <= 'Z')
                fBuffer[i] = static_cast<char>(fBuffer[i] + kCharDiff);
        }
    }

    /*
     * Convert to all ascii characters to uppercase.
     */
    void toUpper() noexcept
    {
        static const char kCharDiff('a' - 'A');

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'a' && fBuffer[i] <= 'z')
                fBuffer[i] = static_cast<char>(fBuffer[i] - kCharDiff);
        }
    }

    /*
     * Return a duplicate string buffer.
     */
    const char* dup() const
    {
        return carla_strdup(fBuffer);
    }

    /*
     * Direct access to the string buffer.
     */
    const char* getBuffer() const noexcept
    {
        return fBuffer;
    }

    // -------------------------------------------------------------------
    // public operators

    operator const char*() const noexcept
    {
        return fBuffer;
    }

    char& operator[](const size_t pos) const noexcept
    {
        if (pos < fBufferLen)
            return fBuffer[pos];

        static char rfallback;
        rfallback = '\0';
        carla_safe_assert("pos < fBufferLen", __FILE__, __LINE__);
        return rfallback;
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

    CarlaString& operator=(const char* const strBuf)
    {
        _dup(strBuf);

        return *this;
    }

    CarlaString& operator=(const CarlaString& str)
    {
        return operator=(str.fBuffer);
    }

    CarlaString& operator+=(const char* const strBuf)
    {
        if (strBuf == nullptr)
            return *this;

        const size_t newBufSize = fBufferLen + std::strlen(strBuf) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, fBuffer);
        std::strcat(newBuf, strBuf);

        _dup(newBuf, newBufSize-1);

        return *this;
    }

    CarlaString& operator+=(const CarlaString& str)
    {
        return operator+=(str.fBuffer);
    }

    CarlaString operator+(const char* const strBuf)
    {
        const size_t newBufSize = fBufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, fBuffer);

        if (strBuf != nullptr)
            std::strcat(newBuf, strBuf);

        return CarlaString(newBuf);
    }

    CarlaString operator+(const CarlaString& str)
    {
        return operator+(str.fBuffer);
    }

    // -------------------------------------------------------------------

private:
    char*  fBuffer;    // the actual string buffer
    size_t fBufferLen; // string length
    bool   fFirstInit; // true when first initiated

    /*
     * Shared init function.
     * Called on all constructors.
     */
    void _init() noexcept
    {
        fBuffer    = nullptr;
        fBufferLen = 0;
        fFirstInit = true;
    }

    /*
     * Helper function.
     * Called whenever the string needs to be allocated.
     *
     * Notes:
     * - Allocates string only if first initiated, or if 'strBuf' is not null and new string contents are different
     * - If 'strBuf' is null 'size' must be 0
     */
    void _dup(const char* const strBuf, const size_t size = 0)
    {
        if (strBuf != nullptr)
        {
            // don't recreate string if contents match
            if (fFirstInit || std::strcmp(fBuffer, strBuf) != 0)
            {
                if (! fFirstInit)
                {
                    CARLA_SAFE_ASSERT(fBuffer != nullptr);
                    delete[] fBuffer;
                }

                fBufferLen = (size > 0) ? size : std::strlen(strBuf);
                fBuffer    = new char[fBufferLen+1];

                std::strcpy(fBuffer, strBuf);

                fBuffer[fBufferLen] = '\0';

                fFirstInit = false;
            }
        }
        else
        {
            CARLA_SAFE_ASSERT(size == 0);

            // don't recreate null string
            if (fFirstInit || fBufferLen != 0)
            {
                if (! fFirstInit)
                {
                    CARLA_SAFE_ASSERT(fBuffer != nullptr);
                    delete[] fBuffer;
                }

                fBufferLen = 0;
                fBuffer    = new char[1];
                fBuffer[0] = '\0';

                fFirstInit = false;
            }
        }
    }

    CARLA_LEAK_DETECTOR(CarlaString)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

static inline
CarlaString operator+(const CarlaString& strBefore, const char* const strBufAfter)
{
    const char* const strBufBefore = (const char*)strBefore;
    const size_t      newBufSize   = strBefore.length() + ((strBufAfter != nullptr) ? std::strlen(strBufAfter) : 0) + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return CarlaString(newBuf);
}

static inline
CarlaString operator+(const char* const strBufBefore, const CarlaString& strAfter)
{
    const char* const strBufAfter = (const char*)strAfter;
    const size_t      newBufSize  = ((strBufBefore != nullptr) ? std::strlen(strBufBefore) : 0) + strAfter.length() + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return CarlaString(newBuf);
}

// -----------------------------------------------------------------------

#endif // CARLA_STRING_HPP_INCLUDED
