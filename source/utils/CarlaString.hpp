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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_STRING_HPP__
#define __CARLA_STRING_HPP__

#include "CarlaJuceUtils.hpp"

// -------------------------------------------------
// CarlaString class

class CarlaString
{
public:
    // ---------------------------------------------
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
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%d", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const unsigned int value, const bool hexadecimal = false)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%x" : "%u", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const long int value)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%ld", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const unsigned long int value, const bool hexadecimal = false)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%lx" : "%lu", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const float value)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%f", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const double value)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%g", value);

        _init();
        _dup(strBuf);
    }

    // ---------------------------------------------
    // non-explicit constructor

    CarlaString(const CarlaString& str)
    {
        _init();
        _dup(str.buffer);
    }

    // ---------------------------------------------
    // destructor

    ~CarlaString()
    {
        CARLA_ASSERT(buffer != nullptr);

        delete[] buffer;
        buffer = nullptr;
    }

    // ---------------------------------------------
    // public methods

    size_t length() const
    {
        return bufferLen;
    }

    bool isEmpty() const
    {
        return (bufferLen == 0);
    }

    bool isNotEmpty() const
    {
        return (bufferLen != 0);
    }

#ifdef __USE_GNU
    bool contains(const char* const strBuf, const bool ignoreCase = false) const
    {
        if (strBuf == nullptr)
            return false;

        if (ignoreCase)
            return (strcasestr(buffer, strBuf) != nullptr);
        else
            return (std::strstr(buffer, strBuf) != nullptr);
    }

    bool contains(const CarlaString& str, const bool ignoreCase = false) const
    {
        return contains(str.buffer, ignoreCase);
    }
#else
    bool contains(const char* const strBuf) const
    {
        if (strBuf == nullptr)
            return false;

        return (std::strstr(buffer, strBuf) != nullptr);
    }

    bool contains(const CarlaString& str) const
    {
        return contains(str.buffer);
    }
#endif

    bool isDigit(const size_t pos) const
    {
        if (pos >= bufferLen)
            return false;

        return (buffer[pos] >= '0' && buffer[pos] <= '9');
    }

    bool startsWith(const char* const prefix) const
    {
        if (prefix == nullptr)
            return false;

        const size_t prefixLen(std::strlen(prefix));

        if (bufferLen < prefixLen)
            return false;

        return (std::strncmp(buffer + (bufferLen-prefixLen), prefix, prefixLen) == 0);
    }

    bool endsWith(const char* const suffix) const
    {
        if (suffix == nullptr)
            return false;

        const size_t suffixLen(std::strlen(suffix));

        if (bufferLen < suffixLen)
            return false;

        return (std::strncmp(buffer + (bufferLen-suffixLen), suffix, suffixLen) == 0);
    }

    void clear()
    {
        truncate(0);
    }

    size_t find(const char c) const
    {
        for (size_t i=0; i < bufferLen; ++i)
        {
            if (buffer[i] == c)
                return i;
        }

        return 0;
    }

    size_t rfind(const char c) const
    {
        for (size_t i=bufferLen; i > 0; --i)
        {
            if (buffer[i-1] == c)
                return i-1;
        }

        return 0;
    }

    size_t rfind(const char* const strBuf) const
    {
        if (strBuf == nullptr || strBuf[0] == '\0')
            return bufferLen;

        size_t ret = bufferLen+1;
        const char* tmpBuf = buffer;

        for (size_t i=0; i < bufferLen; ++i)
        {
            if (std::strstr(tmpBuf, strBuf) == nullptr)
                break;

            --ret;
            ++tmpBuf;
        }

        return (ret > bufferLen) ? bufferLen : bufferLen-ret;
    }

    void replace(const char before, const char after)
    {
        if (after == '\0')
            return;

        for (size_t i=0; i < bufferLen; ++i)
        {
            if (buffer[i] == before)
                buffer[i] = after;
            else if (buffer[i] == '\0')
                break;
        }
    }

    void truncate(const size_t n)
    {
        if (n >= bufferLen)
            return;

        for (size_t i=n; i < bufferLen; ++i)
            buffer[i] = '\0';

        bufferLen = n;
    }

    void toBasic()
    {
        for (size_t i=0; i < bufferLen; ++i)
        {
            if (buffer[i] >= '0' && buffer[i] <= '9')
                continue;
            if (buffer[i] >= 'A' && buffer[i] <= 'Z')
                continue;
            if (buffer[i] >= 'a' && buffer[i] <= 'z')
                continue;
            if (buffer[i] == '_')
                continue;

            buffer[i] = '_';
        }
    }

    void toLower()
    {
        static const char kCharDiff = 'a' - 'A';

        for (size_t i=0; i < bufferLen; ++i)
        {
            if (buffer[i] >= 'A' && buffer[i] <= 'Z')
                buffer[i] += kCharDiff;
        }
    }

    void toUpper()
    {
        static const char kCharDiff = 'a' - 'A';

        for (size_t i=0; i < bufferLen; ++i)
        {
            if (buffer[i] >= 'a' && buffer[i] <= 'z')
                buffer[i] -= kCharDiff;
        }
    }

    // ---------------------------------------------
    // public operators

    operator const char*() const
    {
        return buffer;
    }

    char& operator[](const size_t pos)
    {
        return buffer[pos];
    }

    bool operator==(const char* const strBuf) const
    {
        return (strBuf != nullptr && std::strcmp(buffer, strBuf) == 0);
    }

    bool operator==(const CarlaString& str) const
    {
        return operator==(str.buffer);
    }

    bool operator!=(const char* const strBuf) const
    {
        return !operator==(strBuf);
    }

    bool operator!=(const CarlaString& str) const
    {
        return !operator==(str.buffer);
    }

    CarlaString& operator=(const char* const strBuf)
    {
        _dup(strBuf);

        return *this;
    }

    CarlaString& operator=(const CarlaString& str)
    {
        return operator=(str.buffer);
    }

    CarlaString& operator+=(const char* const strBuf)
    {
        const size_t newBufSize = bufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, buffer);
        std::strcat(newBuf, strBuf);

        _dup(newBuf, newBufSize-1);

        return *this;
    }

    CarlaString& operator+=(const CarlaString& str)
    {
        return operator+=(str.buffer);
    }

    CarlaString operator+(const char* const strBuf)
    {
        const size_t newBufSize = bufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, buffer);
        std::strcat(newBuf, strBuf);

        return CarlaString(newBuf);
    }

    CarlaString operator+(const CarlaString& str)
    {
        return operator+(str.buffer);
    }

    // ---------------------------------------------

private:
    char*  buffer;
    size_t bufferLen;
    bool   firstInit;

    void _init()
    {
        buffer    = nullptr;
        bufferLen = 0;
        firstInit = true;
    }

    // allocate string strBuf if not null
    // size > 0 only if strBuf is valid
    void _dup(const char* const strBuf, const size_t size = 0)
    {
        if (strBuf != nullptr)
        {
            // don't recreate string if contents match
            if (firstInit || std::strcmp(buffer, strBuf) != 0)
            {
                if (! firstInit)
                {
                    CARLA_ASSERT(buffer != nullptr);
                    delete[] buffer;
                }

                bufferLen = (size > 0) ? size : std::strlen(strBuf);
                buffer    = new char[bufferLen+1];

                std::strcpy(buffer, strBuf);

                buffer[bufferLen] = '\0';

                firstInit = false;
            }
        }
        else
        {
            CARLA_ASSERT(size == 0);

            // don't recreate null string
            if (firstInit || bufferLen != 0)
            {
                if (! firstInit)
                {
                    CARLA_ASSERT(buffer != nullptr);
                    delete[] buffer;
                }

                bufferLen = 0;
                buffer    = new char[1];
                buffer[0] = '\0';

                firstInit = false;
            }
        }
    }

    CARLA_LEAK_DETECTOR(CarlaString)
    CARLA_PREVENT_HEAP_ALLOCATION
};

static inline
CarlaString operator+(const CarlaString& strBefore, const char* const strBufAfter)
{
    const char* const strBufBefore = (const char*)strBefore;
    const size_t newBufSize = strBefore.length() + ((strBufAfter != nullptr) ? std::strlen(strBufAfter) : 0) + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return CarlaString(newBuf);
}

static inline
CarlaString operator+(const char* const strBufBefore, const CarlaString& strAfter)
{
    const char* const strBufAfter = (const char*)strAfter;
    const size_t newBufSize = ((strBufBefore != nullptr) ? std::strlen(strBufBefore) : 0) + strAfter.length() + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return CarlaString(newBuf);
}

// -------------------------------------------------

#endif // __CARLA_UTILS_HPP__
