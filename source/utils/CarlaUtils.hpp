/*
 * Carla common utils
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_UTILS_HPP_INCLUDED
#define CARLA_UTILS_HPP_INCLUDED

#include "CarlaDefines.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#ifdef CARLA_OS_WIN
# include <winsock2.h>
# include <windows.h>
#else
# include <unistd.h>
#endif

// -----------------------------------------------------------------------
// misc functions

/*
 * Return "true" or "false" according to yesNo.
 */
static inline
const char* bool2str(const bool yesNo) noexcept
{
    return yesNo ? "true" : "false";
}

/*
 * Dummy function.
 */
static inline
void pass() noexcept {}

// -----------------------------------------------------------------------
// string print functions

/*
 * Print a string to stdout with newline (gray color).
 * Does nothing if DEBUG is not defined.
 */
#ifndef DEBUG
# define carla_debug(...)
#else
static inline
void carla_debug(const char* const fmt, ...) noexcept
{
    try {
        va_list args;
        va_start(args, fmt);
        std::fprintf(stdout, "\x1b[30;1m");
        std::vfprintf(stdout, fmt, args);
        std::fprintf(stdout, "\x1b[0m\n");
        va_end(args);
    } catch (...) {}
}
#endif

/*
 * Print a string to stdout with newline.
 */
static inline
void carla_stdout(const char* const fmt, ...) noexcept
{
    try {
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stdout, fmt, args);
        std::fprintf(stdout, "\n");
        va_end(args);
    } catch (...) {}
}

/*
 * Print a string to stderr with newline.
 */
static inline
void carla_stderr(const char* const fmt, ...) noexcept
{
    try {
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        std::fprintf(stderr, "\n");
        va_end(args);
    } catch (...) {}
}

/*
 * Print a string to stderr with newline (red color).
 */
static inline
void carla_stderr2(const char* const fmt, ...) noexcept
{
    try {
        va_list args;
        va_start(args, fmt);
        std::fprintf(stderr, "\x1b[31m");
        std::vfprintf(stderr, fmt, args);
        std::fprintf(stderr, "\x1b[0m\n");
        va_end(args);
    } catch (...) {}
}

// -----------------------------------------------------------------------
// carla_safe_assert*

/*
 * Print a safe assertion error message.
 */
static inline
void carla_safe_assert(const char* const assertion, const char* const file, const int line) noexcept
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

/*
 * Print a safe assertion error message, with 1 extra integer value.
 */
static inline
void carla_safe_assert_int(const char* const assertion, const char* const file, const int line, const int value) noexcept
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, value %i", assertion, file, line, value);
}
static inline
void carla_safe_assert_uint(const char* const assertion, const char* const file, const int line, const uint value) noexcept
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, value %u", assertion, file, line, value);
}

/*
 * Print a safe assertion error message, with 2 extra integer values.
 */
static inline
void carla_safe_assert_int2(const char* const assertion, const char* const file, const int line, const int v1, const int v2) noexcept
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, v1 %i, v2 %i", assertion, file, line, v1, v2);
}
static inline
void carla_safe_assert_uint2(const char* const assertion, const char* const file, const int line, const uint v1, const uint v2) noexcept
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, v1 %u, v2 %u", assertion, file, line, v1, v2);
}

// -----------------------------------------------------------------------
// carla_*sleep

/*
 * Sleep for 'secs' seconds.
 */
static inline
void carla_sleep(const unsigned int secs)
{
    CARLA_SAFE_ASSERT_RETURN(secs > 0,);

#ifdef CARLA_OS_WIN
    ::Sleep(secs * 1000);
#else
    ::sleep(secs);
#endif
}

/*
 * Sleep for 'msecs' milliseconds.
 */
static inline
void carla_msleep(const unsigned int msecs)
{
    CARLA_SAFE_ASSERT_RETURN(msecs > 0,);

#ifdef CARLA_OS_WIN
    ::Sleep(msecs);
#else
    ::usleep(msecs * 1000);
#endif
}

// -----------------------------------------------------------------------
// carla_setenv

/*
 * Set environment variable 'key' to 'value'.
 */
static inline
void carla_setenv(const char* const key, const char* const value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

#ifdef CARLA_OS_WIN
    ::SetEnvironmentVariableA(key, value);
#else
    ::setenv(key, value, 1);
#endif
}

// -----------------------------------------------------------------------
// carla_strdup

/*
 * Custom 'strdup' function.
 * Return value is always valid, and must be freed with "delete[] var".
 */
static inline
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

/*
 * Custom 'strdup' function.
 * Calls "std::free(strBuf)".
 * Return value is always valid, and must be freed with "delete[] var".
 */
static inline
const char* carla_strdup_free(char* const strBuf)
{
    const char* const buffer(carla_strdup(strBuf));
    std::free(strBuf);
    return buffer;
}

// -----------------------------------------------------------------------
// memory functions

/*
 * Fill an array with a fixed value.
 */
template<typename T>
static inline
void carla_fill(T* data, const size_t size, const T v) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);

    if (v == 0)
    {
        std::memset(data, 0, size*sizeof(T));
    }
    else
    {
        for (size_t i=0; i < size; ++i)
            *data++ = v;
    }
}

/*
 * Clear a char array.
 */
static inline
void carla_zeroChar(char* const data, const size_t numChars) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(numChars > 0,);

    std::memset(data, 0, numChars*sizeof(char));
}

/*
 * Clear a float array.
 */
static inline
void carla_zeroFloat(float* const data, const size_t numSamples) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(numSamples > 0,);

    std::memset(data, 0, numSamples*sizeof(float));
}

/*
 * Clear a memory location.
 */
static inline
void carla_zeroMem(void* const memory, const size_t numBytes) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(memory != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(numBytes > 0,);

    std::memset(memory, 0, numBytes);
}

/*
 * Clear a single struct/class.
 */
template <typename T>
static inline
void carla_zeroStruct(T& structure) noexcept
{
    std::memset(&structure, 0, sizeof(T));
}

/*
 * Clear an array of struct/class.
 */
template <typename T>
static inline
void carla_zeroStruct(T* const structure, const size_t count) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(structure != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count > 0,);

    std::memset(structure, 0, count*sizeof(T));
}

/*
 * Copy a single struct/class.
 */
template <typename T>
static inline
void carla_copyStruct(T& struct1, const T& struct2) noexcept
{
    std::memcpy(&struct1, &struct2, sizeof(T));
}

/*
 * Copy an array of struct/class.
 */
template <typename T>
static inline
void carla_copyStruct(T* const struct1, T* const struct2, const size_t count) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(struct1 != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(struct2 != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count > 0,);

    std::memcpy(struct1, struct2, count*sizeof(T));
}

// -----------------------------------------------------------------------

#endif // CARLA_UTILS_HPP_INCLUDED
