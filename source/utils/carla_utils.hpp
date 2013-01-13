/*
 * Carla common utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __CARLA_UTILS_HPP__
#define __CARLA_UTILS_HPP__

#include "carla_defines.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(Q_OS_HAIKU)
# include <kernel/OS.h>
#elif defined(Q_OS_LINUX)
# include <sys/prctl.h>
# include <linux/prctl.h>
#endif

// -------------------------------------------------
// carla_assert*

static inline
void carla_assert(const char* const assertion, const char* const file, const int line)
{
    qCritical("Carla assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

static inline
void carla_assert_int(const char* const assertion, const char* const file, const int line, const int value)
{
    qCritical("Carla assertion failure: \"%s\" in file %s, line %i, value %i", assertion, file, line, value);
}

// -------------------------------------------------
// carla_*sleep (carla_usleep not possible in Windows)

static inline
void carla_sleep(const int secs)
{
    CARLA_ASSERT(secs > 0);

#ifdef Q_OS_WIN
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

static inline
void carla_msleep(const int msecs)
{
    CARLA_ASSERT(msecs > 0);

#ifdef Q_OS_WIN
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

static inline
void carla_usleep(const int usecs)
{
    CARLA_ASSERT(usecs > 0);

#ifdef Q_OS_WIN
    Sleep(usecs / 1000);
#else
    usleep(usecs);
#endif
}

// -------------------------------------------------
// carla_setenv

static inline
void carla_setenv(const char* const key, const char* const value)
{
    CARLA_ASSERT(key);
    CARLA_ASSERT(value);

#ifdef Q_OS_WIN
    SetEnvironmentVariableA(key, value);
#else
    setenv(key, value, 1);
#endif
}

// -------------------------------------------------
// carla_setprocname (not available on all platforms)

static inline
void carla_setprocname(const char* const name)
{
    CARLA_ASSERT(name);

#if defined(Q_OS_HAIKU)
    if ((thread_id this_thread = find_thread(nullptr)) != B_NAME_NOT_FOUND)
        rename_thread(this_thread, name);
#elif defined(Q_OS_LINUX)
    prctl(PR_SET_NAME, name);
#else
    qWarning("carla_setprocname(\"%s\") - unsupported on this platform", name);
#endif
}

// -------------------------------------------------
// math functions

template<typename T>
static inline
const T& carla_min(const T& v1, const T& v2, const T& min)
{
    return ((v1 < min || v2 < min) ? min : (v1 < v2 ? v1 : v2));
}

template<typename T>
static inline
void carla_fill(T* data, const unsigned int size, const T v)
{
    CARLA_ASSERT(data);
    CARLA_ASSERT(size > 0);

    for (unsigned int i=0; i < size; i++)
        *data++ = v;
}

void carla_zeroDouble(double* data, const unsigned size)
{
    carla_fill<double>(data, size, 0.0);
}

void carla_zeroFloat(float* data, const unsigned size)
{
    carla_fill<float>(data, size, 0.0f);
}

// -------------------------------------------------
// other misc functions

static inline
const char* bool2str(const bool yesNo)
{
    return yesNo ? "true" : "false";
}

static inline
void pass() {}

// -------------------------------------------------
// CarlaString class

class CarlaString
{
public:
    // ---------------------------------------------
    // constructors (no explicit conversions allowed)

    explicit CarlaString()
    {
        buffer = ::strdup("");
    }

    explicit CarlaString(char* const strBuf)
    {
        buffer = ::strdup(strBuf ? strBuf : "");
    }

    explicit CarlaString(const char* const strBuf)
    {
        buffer = ::strdup(strBuf ? strBuf : "");
    }

    explicit CarlaString(const int value)
    {
        const size_t strBufSize = ::abs(value/10) + 3;
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, "%d", value);

        buffer = ::strdup(strBuf);
    }

    explicit CarlaString(const unsigned int value, const bool hexadecimal = false)
    {
        const size_t strBufSize = value/10 + 2 + (hexadecimal ? 2 : 0);
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, hexadecimal ? "%u" : "0x%x", value);

        buffer = ::strdup(strBuf);
    }

    explicit CarlaString(const long int value)
    {
        const size_t strBufSize = ::labs(value/10) + 3;
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, "%ld", value);

        buffer = ::strdup(strBuf);
    }

    explicit CarlaString(const unsigned long int value, const bool hexadecimal = false)
    {
        const size_t strBufSize = value/10 + 2 + (hexadecimal ? 2 : 0);
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, hexadecimal ? "%lu" : "0x%lx", value);

        buffer = ::strdup(strBuf);
    }

    explicit CarlaString(const float value)
    {
        char strBuf[0xff];
        ::snprintf(strBuf, 0xff, "%f", value);

        buffer = ::strdup(strBuf);
    }

    explicit CarlaString(const double value)
    {
        char strBuf[0xff];
        ::snprintf(strBuf, 0xff, "%g", value);

        buffer = ::strdup(strBuf);
    }

    // ---------------------------------------------
    // non-explicit constructor

    CarlaString(const CarlaString& str)
    {
        buffer = ::strdup(str.buffer);
    }

    // ---------------------------------------------
    // deconstructor

    ~CarlaString()
    {
        CARLA_ASSERT(buffer);
        ::free(buffer);
    }

    // ---------------------------------------------
    // public methods

    size_t length() const
    {
        return ::strlen(buffer);
    }

    bool isEmpty() const
    {
        return (*buffer == 0);
    }

    bool isNotEmpty() const
    {
        return (*buffer != 0);
    }

    bool contains(const char* const strBuf) const
    {
        if (! strBuf)
            return false;
        if (*strBuf == 0)
            return false;

        size_t thisLen = ::strlen(buffer);
        size_t thatLen = ::strlen(strBuf)-1;

        for (size_t i=0, j=0; i < thisLen; i++)
        {
            if (buffer[i] == strBuf[j])
                j++;
            else
                j = 0;

            if (j == thatLen)
                return true;
        }

        return false;
    }

    bool contains(const CarlaString& str) const
    {
        return contains(str.buffer);
    }

    bool isDigit(const size_t pos) const
    {
        if (pos >= length())
            return false;

        return (buffer[pos] >= '0' && buffer[pos] <= '9');
    }

    void clear()
    {
        truncate(0);
    }

    void replace(const char before, const char after)
    {
        for (size_t i=0, len = ::strlen(buffer); i < len; i++)
        {
            if (buffer[i] == before)
                buffer[i] = after;
        }
    }

    void truncate(const unsigned int n)
    {
        for (size_t i=n, len = ::strlen(buffer); i < len; i++)
            buffer[i] = 0;
    }

    void toBasic()
    {
        for (size_t i=0, len = ::strlen(buffer); i < len; i++)
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
        for (size_t i=0, len = ::strlen(buffer); i < len; i++)
        {
            if (buffer[i] >= 'A' && buffer[i] <= 'Z')
                buffer[i] += 32;
        }
    }

    void toUpper()
    {
        for (size_t i=0, len = ::strlen(buffer); i < len; i++)
        {
            if (buffer[i] >= 'a' && buffer[i] <= 'z')
                buffer[i] -= 32;
        }
    }

    // ---------------------------------------------
    // public operators

    operator const char*() const
    {
        return buffer;
    }

    char& operator[](const unsigned int pos)
    {
        return buffer[pos];
    }

    bool operator==(const char* const strBuf) const
    {
        return (strBuf && ::strcmp(buffer, strBuf) == 0);
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
        ::free(buffer);

        buffer = ::strdup(strBuf ? strBuf : "");

        return *this;
    }

    CarlaString& operator=(const CarlaString& str)
    {
        return operator=(str.buffer);
    }

    CarlaString& operator+=(const char* const strBuf)
    {
        const size_t newBufSize = ::strlen(buffer) + (strBuf ? ::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        ::strcpy(newBuf, buffer);
        ::strcat(newBuf, strBuf);
        ::free(buffer);

        buffer = ::strdup(newBuf);

        return *this;
    }

    CarlaString& operator+=(const CarlaString& str)
    {
        return operator+=(str.buffer);
    }

    CarlaString operator+(const char* const strBuf)
    {
        const size_t newBufSize = ::strlen(buffer) + (strBuf ? ::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        ::strcpy(newBuf, buffer);
        ::strcat(newBuf, strBuf);

        return CarlaString(newBuf);
    }

    CarlaString operator+(const CarlaString& str)
    {
        return operator+(str.buffer);
    }

    // ---------------------------------------------

private:
    char* buffer;
};

static inline
CarlaString operator+(const char* const strBufBefore, const CarlaString& strAfter)
{
    const char* const strBufAfter = (const char*)strAfter;
    const size_t newBufSize = (strBufBefore ? ::strlen(strBufBefore) : 0) + ::strlen(strBufAfter) + 1;
    char         newBuf[newBufSize];

    ::strcpy(newBuf, strBufBefore);
    ::strcat(newBuf, strBufAfter);

    return CarlaString(newBuf);
}

// -------------------------------------------------

#endif // __CARLA_UTILS_HPP__
