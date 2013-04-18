/*
 * Carla common utils
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_UTILS_HPP__
#define __CARLA_UTILS_HPP__

#include "CarlaDefines.hpp"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(CARLA_OS_HAIKU)
# include <kernel/OS.h>
#elif defined(CARLA_OS_LINUX)
# include <sys/prctl.h>
# include <linux/prctl.h>
#endif

// -------------------------------------------------
// misc functions

static inline
const char* bool2str(const bool yesNo)
{
    return yesNo ? "true" : "false";
}

static inline
void pass() {}

// -------------------------------------------------
// string print functions

#ifndef DEBUG
# define carla_debug(...)
#else
static inline
void carla_debug(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
# ifndef CARLA_OS_WIN
    std::fprintf(stdout, "\x1b[30;1m");
# endif
    std::vfprintf(stdout, fmt, args);
# ifndef CARLA_OS_WIN
    std::fprintf(stdout, "\x1b[0m\n");
# else
    std::fprintf(stdout, "\n");
# endif
    va_end(args);
}
#endif

static inline
void carla_stdout(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stdout, fmt, args);
    std::fprintf(stdout, "\n");
    va_end(args);
}

static inline
void carla_stderr(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "\n");
    va_end(args);
}

static inline
void carla_stderr2(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifndef CARLA_OS_WIN
    std::fprintf(stderr, "\x1b[31m");
#endif
    std::vfprintf(stderr, fmt, args);
#ifndef CARLA_OS_WIN
    std::fprintf(stderr, "\x1b[0m\n");
#else
    std::fprintf(stderr, "\n");
#endif
    va_end(args);
}

// -------------------------------------------------
// carla_assert*

static inline
void carla_assert(const char* const assertion, const char* const file, const int line)
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

static inline
void carla_assert_int(const char* const assertion, const char* const file, const int line, const int value)
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, value %i", assertion, file, line, value);
}

static inline
void carla_assert_int2(const char* const assertion, const char* const file, const int line, const int v1, const int v2)
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, v1 %i, v2 %i", assertion, file, line, v1, v2);
}

// -------------------------------------------------
// carla_*sleep

static inline
void carla_sleep(const unsigned int secs)
{
    CARLA_ASSERT(secs > 0);

#ifdef CARLA_OS_WIN
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

static inline
void carla_msleep(const unsigned int msecs)
{
    CARLA_ASSERT(msecs > 0);

#ifdef CARLA_OS_WIN
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

static inline
void carla_usleep(const unsigned int usecs)
{
    CARLA_ASSERT(usecs > 0);

#ifdef CARLA_OS_WIN
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
    CARLA_ASSERT(key != nullptr);
    CARLA_ASSERT(value != nullptr);

#ifdef CARLA_OS_WIN
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
    CARLA_ASSERT(name != nullptr);

#if defined(CARLA_OS_HAIKU)
    if ((thread_id this_thread = find_thread(nullptr)) != B_NAME_NOT_FOUND)
        rename_thread(this_thread, name);
#elif defined(CARLA_OS_LINUX)
    prctl(PR_SET_NAME, name);
#else
    carla_stderr("carla_setprocname(\"%s\") - unsupported on this platform", name);
#endif
}

// -------------------------------------------------
// carla_strdup

static inline
const char* carla_strdup(const char* const strBuf)
{
    CARLA_ASSERT(strBuf != nullptr);

    const size_t bufferLen = (strBuf != nullptr) ? std::strlen(strBuf) : 0;
    char* const  buffer    = new char[bufferLen+1];

    std::strcpy(buffer, strBuf);

    buffer[bufferLen] = '\0';

    return buffer;
}

static inline
const char* carla_strdup_free(char* const strBuf)
{
    const char* const buffer = carla_strdup(strBuf);
    std::free(strBuf);
    return buffer;
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
const T& carla_max(const T& v1, const T& v2, const T& max)
{
    return ((v1 > max || v2 > max) ? max : (v1 > v2 ? v1 : v2));
}

template<typename T>
static inline
const T& carla_fixValue(const T& min, const T& max, const T& value)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

template<typename T>
static inline
void carla_copy(T* dataDst, T* dataSrc, const size_t size)
{
    CARLA_ASSERT(dataDst != nullptr);
    CARLA_ASSERT(dataSrc != nullptr);
    CARLA_ASSERT(size > 0);

    if (dataDst == nullptr || dataSrc == nullptr || size == 0)
        return;

    for (size_t i=0; i < size; ++i)
        *dataDst++ = *dataSrc++;
}

template<typename T>
static inline
void carla_copy(T* dataDst, const T* dataSrc, const size_t size)
{
    CARLA_ASSERT(dataDst != nullptr);
    CARLA_ASSERT(dataSrc != nullptr);
    CARLA_ASSERT(size > 0);

    if (dataDst == nullptr || dataSrc == nullptr || size == 0)
        return;

    for (size_t i=0; i < size; ++i)
        *dataDst++ = *dataSrc++;
}

template<typename T>
static inline
void carla_fill(T* data, const size_t size, const T v)
{
    CARLA_ASSERT(data != nullptr);
    CARLA_ASSERT(size > 0);

    if (data == nullptr || size == 0)
        return;

    for (size_t i=0; i < size; ++i)
        *data++ = v;
}

static inline
void carla_copyDouble(double* const dataDst, double* const dataSrc, const size_t size)
{
    carla_copy<double>(dataDst, dataSrc, size);
}

static inline
void carla_copyFloat(float* const dataDst, float* const dataSrc, const size_t size)
{
    carla_copy<float>(dataDst, dataSrc, size);
}

static inline
void carla_zeroDouble(double* const data, const size_t size)
{
    carla_fill<double>(data, size, 0.0);
}

static inline
void carla_zeroFloat(float* const data, const size_t size)
{
    carla_fill<float>(data, size, 0.0f);
}

#ifdef CARLA_OS_MAC
namespace std {
inline float
  fmin(float __x, float __y)
  { return __builtin_fminf(__x, __y); }
inline float
  fmax(float __x, float __y)
  { return __builtin_fmaxf(__x, __y); }
inline float
  rint(float __x)
  { return __builtin_rintf(__x); }
}
#endif

// -------------------------------------------------
// memory functions

static inline
void carla_zeroMem(void* const memory, const size_t numBytes)
{
    CARLA_ASSERT(memory != nullptr);
    CARLA_ASSERT(numBytes > 0);

    if (memory == nullptr || numBytes == 0)
        return;

    std::memset(memory, 0, numBytes);
}

template <typename T>
static inline
void carla_zeroStruct(T& structure)
{
    std::memset(&structure, 0, sizeof(T));
}

template <typename T>
static inline
void carla_zeroStruct(T* const structure, const size_t count)
{
    CARLA_ASSERT(structure != nullptr);
    CARLA_ASSERT(count >= 1);

    std::memset(structure, 0, sizeof(T)*count);
}

// -------------------------------------------------

#endif // __CARLA_UTILS_HPP__
