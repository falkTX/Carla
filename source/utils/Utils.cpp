/*
 * Carla utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.hpp"

// IDE Helper
#ifndef REAL_BUILD
# include "CarlaUtils.hpp"
#endif

#include <cstdarg> // needed for va_*
#include <cstdio>  // needed for *printf
#include <cstdlib> // needed for free
#include <cstring> // needed for str*

#ifdef CARLA_OS_WIN
# include <winsock2.h> // should be included before "windows.h"
# include <windows.h>
#else
# include <unistd.h>
#endif

#ifdef CARLA_UTILS_HPP_INCLUDED
// -----------------------------------------------------------------------
// string print functions

#ifdef DEBUG
void carla_debug(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::fprintf(stdout, "\x1b[30;1m");
    std::vfprintf(stdout, fmt, args);
    std::fprintf(stdout, "\x1b[0m\n");
    va_end(args);
}
#endif

void carla_stdout(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stdout, fmt, args);
    std::fprintf(stdout, "\n");
    va_end(args);
}

void carla_stderr(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "\n");
    va_end(args);
}

void carla_stderr2(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::fprintf(stderr, "\x1b[31m");
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "\x1b[0m\n");
    va_end(args);
}

// -----------------------------------------------------------------------
// carla_*sleep

void carla_sleep(const unsigned int secs)
{
    CARLA_SAFE_ASSERT_RETURN(secs > 0,);

#ifdef CARLA_OS_WIN
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

void carla_msleep(const unsigned int msecs)
{
    CARLA_SAFE_ASSERT_RETURN(msecs > 0,);

#ifdef CARLA_OS_WIN
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

// -----------------------------------------------------------------------
// carla_setenv

void carla_setenv(const char* const key, const char* const value)
{
    CARLA_SAFE_ASSERT_RETURN(key != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

#ifdef CARLA_OS_WIN
    SetEnvironmentVariableA(key, value);
#else
    setenv(key, value, 1);
#endif
}

// -----------------------------------------------------------------------
// carla_strdup

const char* carla_strdup(const char* const strBuf)
{
    CARLA_SAFE_ASSERT(strBuf != nullptr);

    const size_t bufferLen = (strBuf != nullptr) ? std::strlen(strBuf) : 0;
    char* const  buffer    = new char[bufferLen+1];

    if (strBuf != nullptr && bufferLen > 0)
        std::strncpy(buffer, strBuf, bufferLen);

    buffer[bufferLen] = '\0';

    return buffer;
}

const char* carla_strdup_free(char* const strBuf)
{
    const char* const buffer(carla_strdup(strBuf));
    std::free(strBuf);
    return buffer;
}
#endif // CARLA_UTILS_HPP_INCLUDED
