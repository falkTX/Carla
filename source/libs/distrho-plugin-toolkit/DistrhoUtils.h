/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the license see the GPL.txt file
 */

#ifndef __DISTRHO_UTILS_H__
#define __DISTRHO_UTILS_H__

#include "src/DistrhoMacros.h"

#if DISTRHO_OS_WINDOWS
# include <windows.h>
#else
# include <unistd.h>
#endif

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// -------------------------------------------------

inline
long d_cconst(int a, int b, int c, int d)
{
    return (a << 24) | (b << 16) | (c << 8) | (d << 0);
}

inline
void d_sleep(unsigned int secs)
{
#if DISTRHO_OS_WINDOWS
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

inline
void d_msleep(unsigned int msecs)
{
#if DISTRHO_OS_WINDOWS
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

inline
void d_usleep(unsigned int usecs)
{
#if DISTRHO_OS_WINDOWS
    Sleep(usecs / 1000);
#else
    usleep(usecs);
#endif
}

inline
void d_setenv(const char* key, const char* value)
{
#if DISTRHO_OS_WINDOWS
    SetEnvironmentVariableA(key, value);
#else
    setenv(key, value, 1);
#endif
}

// -------------------------------------------------

// TODO - import new stuff from Carla, when ready

class d_string
{
public:
    d_string()
    {
        buffer = strdup("");
    }

    d_string(const char* strBuf)
    {
        buffer = strdup(strBuf ? strBuf : "");
    }

    d_string(const d_string& str)
    {
        buffer = strdup(str.buffer);
    }

    d_string(int value)
    {
        size_t strBufSize = abs(value/10) + 3;
        char   strBuf[strBufSize];
        snprintf(strBuf, strBufSize, "%d", value);

        buffer = strdup(strBuf);
    }

    d_string(unsigned int value)
    {
        size_t strBufSize = value/10 + 2;
        char   strBuf[strBufSize];
        snprintf(strBuf, strBufSize, "%u", value);

        buffer = strdup(strBuf);
    }

    d_string(float value)
    {
        char strBuf[255];
        snprintf(strBuf, 255, "%f", value);

        buffer = strdup(strBuf);
    }

    ~d_string()
    {
        free(buffer);
    }

    size_t length() const
    {
        return strlen(buffer);
    }

    bool isEmpty() const
    {
        return (*buffer == 0);
    }

    // ---------------------------------------------

    operator const char*() const
    {
        return buffer;
    }

    bool operator==(const char* strBuf) const
    {
        return (strcmp(buffer, strBuf) == 0);
    }

    bool operator==(const d_string& str) const
    {
        return operator==(str.buffer);
    }

    bool operator!=(const char* strBuf) const
    {
        return !operator==(strBuf);
    }

    bool operator!=(const d_string& str) const
    {
        return !operator==(str.buffer);
    }

    d_string& operator=(const char* strBuf)
    {
        free(buffer);

        buffer = strdup(strBuf);

        return *this;
    }

    d_string& operator=(const d_string& str)
    {
        return operator=(str.buffer);
    }

    d_string& operator+=(const char* strBuf)
    {
        size_t newBufSize = strlen(buffer) + strlen(strBuf) + 1;
        char   newBuf[newBufSize];

        strcpy(newBuf, buffer);
        strcat(newBuf, strBuf);
        free(buffer);

        buffer = strdup(newBuf);

        return *this;
    }

    d_string& operator+=(const d_string& str)
    {
        return operator+=(str.buffer);
    }

    d_string operator+(const char* strBuf)
    {
        size_t newBufSize = strlen(buffer) + strlen(strBuf) + 1;
        char   newBuf[newBufSize];

        strcpy(newBuf, buffer);
        strcat(newBuf, strBuf);

        return d_string(newBuf);
    }

    d_string operator+(const d_string& str)
    {
        return operator+(str.buffer);
    }

private:
    char* buffer;
};

static inline
d_string operator+(const char* strBufBefore, const d_string& strAfter)
{
    const char* strBufAfter = (const char*)strAfter;
    size_t newBufSize = strlen(strBufBefore) + strlen(strBufAfter) + 1;
    char   newBuf[newBufSize];

    strcpy(newBuf, strBufBefore);
    strcat(newBuf, strBufAfter);

    return d_string(newBuf);
}

// -------------------------------------------------

#endif // __DISTRHO_UTILS_H__
