/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DISTRHO_UTILS_HPP__
#define __DISTRHO_UTILS_HPP__

#include "src/DistrhoDefines.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef DISTRHO_OS_WINDOWS
# include <windows.h>
#else
# include <unistd.h>
#endif

START_NAMESPACE_DISTRHO

// -------------------------------------------------

static inline
long d_cconst(int a, int b, int c, int d)
{
    return (a << 24) | (b << 16) | (c << 8) | (d << 0);
}

// -------------------------------------------------

static inline
void d_sleep(unsigned int secs)
{
#ifdef DISTRHO_OS_WINDOWS
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

static inline
void d_msleep(unsigned int msecs)
{
#ifdef DISTRHO_OS_WINDOWS
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

static inline
void d_usleep(unsigned int usecs)
{
#ifdef DISTRHO_OS_WINDOWS
    Sleep(usecs / 1000);
#else
    usleep(usecs);
#endif
}

// -------------------------------------------------

static inline
void d_setenv(const char* key, const char* value)
{
#if DISTRHO_OS_WINDOWS
    SetEnvironmentVariableA(key, value);
#else
    setenv(key, value, 1);
#endif
}

// -------------------------------------------------

class d_string
{
public:
    // ---------------------------------------------
    // constructors (no explicit conversions allowed)

    explicit d_string()
    {
        _init();
        _dup(nullptr);
    }

    explicit d_string(char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit d_string(const char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit d_string(const int value)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%d", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const unsigned int value, const bool hexadecimal = false)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%x" : "%u", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const long int value)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%ld", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const unsigned long int value, const bool hexadecimal = false)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%lx" : "%lu", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const float value)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%f", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const double value)
    {
        char strBuf[0xff] = { '\0' };
        std::snprintf(strBuf, 0xff, "%g", value);

        _init();
        _dup(strBuf);
    }

    // ---------------------------------------------
    // non-explicit constructor

    d_string(const d_string& str)
    {
        _init();
        _dup(str.buffer);
    }

    // ---------------------------------------------
    // deconstructor

    ~d_string()
    {
        assert(buffer != nullptr);

        delete[] buffer;
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

    bool contains(const d_string& str, const bool ignoreCase = false) const
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

    bool contains(const d_string& str) const
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

    bool operator==(const d_string& str) const
    {
        return operator==(str.buffer);
    }

    bool operator!=(const char* const strBuf) const
    {
        return !operator==(strBuf);
    }

    bool operator!=(const d_string& str) const
    {
        return !operator==(str.buffer);
    }

    d_string& operator=(const char* const strBuf)
    {
        _dup(strBuf);

        return *this;
    }

    d_string& operator=(const d_string& str)
    {
        return operator=(str.buffer);
    }

    d_string& operator+=(const char* const strBuf)
    {
        const size_t newBufSize = bufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, buffer);
        std::strcat(newBuf, strBuf);

        _dup(newBuf, newBufSize-1);

        return *this;
    }

    d_string& operator+=(const d_string& str)
    {
        return operator+=(str.buffer);
    }

    d_string operator+(const char* const strBuf)
    {
        const size_t newBufSize = bufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, buffer);
        std::strcat(newBuf, strBuf);

        return d_string(newBuf);
    }

    d_string operator+(const d_string& str)
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
                    assert(buffer != nullptr);
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
            assert(size == 0);

            // don't recreate null string
            if (firstInit || bufferLen != 0)
            {
                if (! firstInit)
                {
                    assert(buffer != nullptr);
                    delete[] buffer;
                }

                bufferLen = 0;
                buffer    = new char[1];
                buffer[0] = '\0';

                firstInit = false;
            }
        }
    }
};

// -------------------------------------------------

static inline
d_string operator+(const d_string& strBefore, const char* const strBufAfter)
{
    const char* const strBufBefore = (const char*)strBefore;
    const size_t newBufSize = strBefore.length() + ((strBufAfter != nullptr) ? std::strlen(strBufAfter) : 0) + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return d_string(newBuf);
}

static inline
d_string operator+(const char* const strBufBefore, const d_string& strAfter)
{
    const char* const strBufAfter = (const char*)strAfter;
    const size_t newBufSize = ((strBufBefore != nullptr) ? std::strlen(strBufBefore) : 0) + strAfter.length() + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return d_string(newBuf);
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __UTILS_HPP__
