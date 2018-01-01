/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
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

#include <cmath>
#include <limits>

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#if defined(DISTRHO_OS_MAC) && ! defined(CARLA_OS_MAC) && ! defined(DISTRHO_PROPER_CPP11_SUPPORT)
namespace std {
inline float fmin(float __x, float __y)
  { return __builtin_fminf(__x, __y); }
inline float fmax(float __x, float __y)
  { return __builtin_fmaxf(__x, __y); }
inline float rint(float __x)
  { return __builtin_rintf(__x); }
inline float round(float __x)
  { return __builtin_roundf(__x); }
}
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

// -----------------------------------------------------------------------
// misc functions

/*
 * Return a 64-bit number from 4 8-bit numbers.
 */
static inline
int64_t d_cconst(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) noexcept
{
    return (a << 24) | (b << 16) | (c << 8) | (d << 0);
}

/*
 * Return an hexadecimal representation of a MAJ.MIN.MICRO version number.
 */
static inline
uint32_t d_version(const uint8_t major, const uint8_t minor, const uint8_t micro) noexcept
{
    return uint32_t(major << 16) | uint32_t(minor << 8) | (micro << 0);
}

/*
 * Dummy function.
 */
static inline
void d_pass() noexcept {}

// -----------------------------------------------------------------------
// string print functions

/*
 * Print a string to stdout with newline (gray color).
 * Does nothing if DEBUG is not defined.
 */
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

/*
 * Print a string to stdout with newline.
 */
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

/*
 * Print a string to stderr with newline.
 */
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

/*
 * Print a string to stderr with newline (red color).
 */
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

/*
 * Print a safe assertion error message.
 */
static inline
void d_safe_assert(const char* const assertion, const char* const file, const int line) noexcept
{
    d_stderr2("assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

/*
 * Print a safe exception error message.
 */
static inline
void d_safe_exception(const char* const exception, const char* const file, const int line) noexcept
{
    d_stderr2("exception caught: \"%s\" in file %s, line %i", exception, file, line);
}

// -----------------------------------------------------------------------
// math functions

/*
 * Safely compare two floating point numbers.
 * Returns true if they match.
 */
template<typename T>
static inline
bool d_isEqual(const T& v1, const T& v2)
{
    return std::abs(v1-v2) < std::numeric_limits<T>::epsilon();
}

/*
 * Safely compare two floating point numbers.
 * Returns true if they don't match.
 */
template<typename T>
static inline
bool d_isNotEqual(const T& v1, const T& v2)
{
    return std::abs(v1-v2) >= std::numeric_limits<T>::epsilon();
}

/*
 * Safely check if a floating point number is zero.
 */
template<typename T>
static inline
bool d_isZero(const T& value)
{
    return std::abs(value) < std::numeric_limits<T>::epsilon();
}

/*
 * Safely check if a floating point number is not zero.
 */
template<typename T>
static inline
bool d_isNotZero(const T& value)
{
    return std::abs(value) >= std::numeric_limits<T>::epsilon();
}

/*
 * Get next power of 2.
 */
static inline
uint32_t d_nextPowerOf2(uint32_t size) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0, 0);

    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --size;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return ++size;
}

// -----------------------------------------------------------------------

#ifndef DONT_SET_USING_DISTRHO_NAMESPACE
  // If your code uses a lot of DISTRHO classes, then this will obviously save you
  // a lot of typing, but can be disabled by setting DONT_SET_USING_DISTRHO_NAMESPACE.
  namespace DISTRHO_NAMESPACE {}
  using namespace DISTRHO_NAMESPACE;
#endif

// -----------------------------------------------------------------------

#endif // DISTRHO_UTILS_HPP_INCLUDED
