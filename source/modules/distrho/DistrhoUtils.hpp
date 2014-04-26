/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_UTILS_HPP_INCLUDED
#define DISTRHO_UTILS_HPP_INCLUDED

#include "src/DistrhoDefines.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#ifdef DISTRHO_OS_WINDOWS
# include <winsock2.h>
# include <windows.h>
#else
# include <unistd.h>
#endif

#if defined(DISTRHO_OS_MAC) && ! defined(CARLA_OS_MAC)
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
inline float
  round(float __x)
  { return __builtin_roundf(__x); }
}
#endif

// -----------------------------------------------------------------------
// misc functions

static inline
long d_cconst(const int a, const int b, const int c, const int d) noexcept
{
    return (a << 24) | (b << 16) | (c << 8) | (d << 0);
}

static inline
void d_pass() noexcept {}

// -----------------------------------------------------------------------
// string print functions

#ifndef DEBUG
# define d_debug(...)
#else
static inline
void d_debug(const char* const fmt, ...) noexcept
{
    try {
        ::va_list args;
        ::va_start(args, fmt);
        std::fprintf(stdout, "\x1b[30;1m");
        std::vfprintf(stdout, fmt, args);
        std::fprintf(stdout, "\x1b[0m\n");
        ::va_end(args);
    } catch (...) {}
}
#endif

static inline
void d_stdout(const char* const fmt, ...) noexcept
{
    try {
        ::va_list args;
        ::va_start(args, fmt);
        std::vfprintf(stdout, fmt, args);
        std::fprintf(stdout, "\n");
        ::va_end(args);
    } catch (...) {}
}

static inline
void d_stderr(const char* const fmt, ...) noexcept
{
    try {
        ::va_list args;
        ::va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        std::fprintf(stderr, "\n");
        ::va_end(args);
    } catch (...) {}
}

static inline
void d_stderr2(const char* const fmt, ...) noexcept
{
    try {
        ::va_list args;
        ::va_start(args, fmt);
        std::fprintf(stderr, "\x1b[31m");
        std::vfprintf(stderr, fmt, args);
        std::fprintf(stderr, "\x1b[0m\n");
        ::va_end(args);
    } catch (...) {}
}

static inline
void d_safe_assert(const char* const assertion, const char* const file, const int line) noexcept
{
    d_stderr2("assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

static inline
void d_safe_exception(const char* const exception, const char* const file, const int line) noexcept
{
    d_stderr2("exception caught: \"%s\" in file %s, line %i", exception, file, line);
}

// -----------------------------------------------------------------------
// d_*sleep

static inline
void d_sleep(const uint secs)
{
    DISTRHO_SAFE_ASSERT_RETURN(secs > 0,);

    try {
#ifdef DISTRHO_OS_WINDOWS
        ::Sleep(secs * 1000);
#else
        ::sleep(secs);
#endif
    } DISTRHO_SAFE_EXCEPTION("carla_sleep");
}

static inline
void d_msleep(const uint msecs)
{
    DISTRHO_SAFE_ASSERT_RETURN(msecs > 0,);

    try {
#ifdef DISTRHO_OS_WINDOWS
        ::Sleep(msecs);
#else
        ::usleep(msecs * 1000);
#endif
    } DISTRHO_SAFE_EXCEPTION("carla_msleep");
}

// -----------------------------------------------------------------------

#endif // DISTRHO_UTILS_HPP_INCLUDED
