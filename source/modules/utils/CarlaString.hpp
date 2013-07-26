/*
 * Carla String
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_STRING_HPP_INCLUDED
#define CARLA_STRING_HPP_INCLUDED

#include "CarlaJuceUtils.hpp"

// -----------------------------------------------------------------------
// CarlaString class

class CarlaString
{
public:
    // -------------------------------------------------------------------
    // constructors (no explicit conversions allowed)

    explicit CarlaString()
    {
        _init();
        _dup(nullptr);
    }

    explicit CarlaString(char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const int value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%d", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const unsigned int value, const bool hexadecimal = false)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%x" : "%u", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const long int value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%ld", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const unsigned long int value, const bool hexadecimal = false)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%lx" : "%lu", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const float value)
    {
        char strBuf[0xff+1];
        carla_zeroChar(strBuf, 0xff+1);
        std::snprintf(strBuf, 0xff, "%f", value);

        _init();
        _dup(strBuf);
    }

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

    CarlaString(const CarlaString& str)
    {
        _init();
        _dup(str.fBuffer);
    }

    // -------------------------------------------------------------------
    // destructor

    ~CarlaString()
    {
        CARLA_ASSERT(fBuffer != nullptr);

        delete[] fBuffer;
        fBuffer = nullptr;
    }

    // -------------------------------------------------------------------
    // public methods

    size_t length() const noexcept
    {
        return fBufferLen;
    }

    bool isEmpty() const noexcept
    {
        return (fBufferLen == 0);
    }

    bool isNotEmpty() const noexcept
    {
        return (fBufferLen != 0);
    }

#ifdef __USE_GNU
    bool contains(const char* const strBuf, const bool ignoreCase = false) const
    {
        if (strBuf == nullptr)
            return false;

        if (ignoreCase)
            return (strcasestr(fBuffer, strBuf) != nullptr);
        else
            return (std::strstr(fBuffer, strBuf) != nullptr);
    }

    bool contains(const CarlaString& str, const bool ignoreCase = false) const
    {
        return contains(str.fBuffer, ignoreCase);
    }
#else
    bool contains(const char* const strBuf) const
    {
        if (strBuf == nullptr)
            return false;

        return (std::strstr(fBuffer, strBuf) != nullptr);
    }

    bool contains(const CarlaString& str) const
    {
        return contains(str.fBuffer);
    }
#endif

    bool isDigit(const size_t pos) const noexcept
    {
        if (pos >= fBufferLen)
            return false;

        return (fBuffer[pos] >= '0' && fBuffer[pos] <= '9');
    }

    bool startsWith(const char* const prefix) const
    {
        if (prefix == nullptr)
            return false;

        const size_t prefixLen(std::strlen(prefix));

        if (fBufferLen < prefixLen)
            return false;

        return (std::strncmp(fBuffer + (fBufferLen-prefixLen), prefix, prefixLen) == 0);
    }

    bool endsWith(const char* const suffix) const
    {
        if (suffix == nullptr)
            return false;

        const size_t suffixLen(std::strlen(suffix));

        if (fBufferLen < suffixLen)
            return false;

        return (std::strncmp(fBuffer + (fBufferLen-suffixLen), suffix, suffixLen) == 0);
    }

    void clear() noexcept
    {
        truncate(0);
    }

    size_t find(const char c) const noexcept
    {
        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] == c)
                return i;
        }

        return 0;
    }

    size_t rfind(const char c) const noexcept
    {
        for (size_t i=fBufferLen; i > 0; --i)
        {
            if (fBuffer[i-1] == c)
                return i-1;
        }

        return 0;
    }

    size_t rfind(const char* const strBuf) const
    {
        if (strBuf == nullptr || strBuf[0] == '\0')
            return fBufferLen;

        size_t ret = fBufferLen+1;
        const char* tmpBuf = fBuffer;

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (std::strstr(tmpBuf, strBuf) == nullptr)
                break;

            --ret;
            ++tmpBuf;
        }

        return (ret > fBufferLen) ? fBufferLen : fBufferLen-ret;
    }

    void replace(const char before, const char after) noexcept
    {
        if (after == '\0')
            return;

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] == before)
                fBuffer[i] = after;
            else if (fBuffer[i] == '\0')
                break;
        }
    }

    void truncate(const size_t n) noexcept
    {
        if (n >= fBufferLen)
            return;

        for (size_t i=n; i < fBufferLen; ++i)
            fBuffer[i] = '\0';

        fBufferLen = n;
    }

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

    void toLower() noexcept
    {
#ifndef BUILD_ANSI_TEST
        // Using '+=' temporarily converts char into int
        static const char kCharDiff('a' - 'A');

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'A' && fBuffer[i] <= 'Z')
                fBuffer[i] += kCharDiff;
        }
#endif
    }

    void toUpper() noexcept
    {
#ifndef BUILD_ANSI_TEST
        // Using '-=' temporarily converts char into int
        static const char kCharDiff('a' - 'A');

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'a' && fBuffer[i] <= 'z')
                fBuffer[i] -= kCharDiff;
        }
#endif
    }

    // -------------------------------------------------------------------
    // public operators

    operator const char*() const noexcept
    {
        return fBuffer;
    }

    char& operator[](const size_t pos) const noexcept
    {
        return fBuffer[pos];
    }

    bool operator==(const char* const strBuf) const
    {
        return (strBuf != nullptr && std::strcmp(fBuffer, strBuf) == 0);
    }

    bool operator==(const CarlaString& str) const
    {
        return operator==(str.fBuffer);
    }

    bool operator!=(const char* const strBuf) const
    {
        return !operator==(strBuf);
    }

    bool operator!=(const CarlaString& str) const
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
        const size_t newBufSize = fBufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
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
        std::strcat(newBuf, strBuf);

        return CarlaString(newBuf);
    }

    CarlaString operator+(const CarlaString& str)
    {
        return operator+(str.fBuffer);
    }

    // -------------------------------------------------------------------

private:
    char*  fBuffer;
    size_t fBufferLen;
    bool   fFirstInit;

    void _init() noexcept
    {
        fBuffer    = nullptr;
        fBufferLen = 0;
        fFirstInit = true;
    }

    // allocate string strBuf if not null
    // size > 0 only if strBuf is valid
    void _dup(const char* const strBuf, const size_t size = 0)
    {
        if (strBuf != nullptr)
        {
            // don't recreate string if contents match
            if (fFirstInit || std::strcmp(fBuffer, strBuf) != 0)
            {
                if (! fFirstInit)
                {
                    CARLA_ASSERT(fBuffer != nullptr);
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
            CARLA_ASSERT(size == 0);

            // don't recreate null string
            if (fFirstInit || fBufferLen != 0)
            {
                if (! fFirstInit)
                {
                    CARLA_ASSERT(fBuffer != nullptr);
                    delete[] fBuffer;
                }

                fBufferLen = 0;
                fBuffer    = new char[1];
                fBuffer[0] = '\0';

                fFirstInit = false;
            }
        }
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_LEAK_DETECTOR(CarlaString)
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
