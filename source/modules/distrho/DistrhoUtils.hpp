/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif

#ifdef __WINE__
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <winsock2.h>
#endif

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

#if defined(DISTRHO_OS_WINDOWS) && defined(_MSC_VER)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#if ! defined(CARLA_MATH_UTILS_HPP_INCLUDED) && ! defined(DISTRHO_PROPER_CPP11_SUPPORT) && ! defined(DISTRHO_OS_MAC)
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

/* --------------------------------------------------------------------------------------------------------------------
 * misc functions */

/**
   @defgroup MiscellaneousFunctions Miscellaneous functions

   @{
 */

/**
   Return a 32-bit number from 4 8-bit numbers.@n
   The return type is a int64_t for better compatibility with plugin formats that use such numbers.
 */
static inline constexpr
int64_t d_cconst(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) noexcept
{
    return (a << 24) | (b << 16) | (c << 8) | (d << 0);
}

/**
   Return a 32-bit number from 4 ASCII characters.
 */
static inline constexpr
uint32_t d_cconst(const char str[4])
{
    return (str[0] << 24) | (str[1] << 16) | (str[2] << 8) | str[3];
}

/**
   Return an hexadecimal representation of a MAJ.MIN.MICRO version number.
 */
static inline constexpr
uint32_t d_version(const uint8_t major, const uint8_t minor, const uint8_t micro) noexcept
{
    return uint32_t(major << 16) | uint32_t(minor << 8) | (micro << 0);
}

/**
   Dummy, no-op function.
 */
static inline
void d_pass() noexcept {}

/** @} */

/* --------------------------------------------------------------------------------------------------------------------
 * string print functions */

/**
   @defgroup StringPrintFunctions String print functions

   @{
 */

/*
 * Internal noexcept-safe fopen function.
 */
static inline
FILE* __d_fopen(const char* const filename, FILE* const fallback) noexcept
{
    if (std::getenv("DPF_CAPTURE_CONSOLE_OUTPUT") == nullptr)
        return fallback;

    FILE* ret = nullptr;

    try {
        ret = std::fopen(filename, "a+");
    } catch (...) {}

    if (ret == nullptr)
        ret = fallback;

    return ret;
}

/**
   Print a string to stdout with newline (gray color).
   Does nothing if DEBUG is not defined.
 */
#ifndef DEBUG
# define d_debug(...)
#else
static inline
void d_debug(const char* const fmt, ...) noexcept
{
    static FILE* const output = __d_fopen("/tmp/dpf.debug.log", stdout);

    try {
        va_list args;
        va_start(args, fmt);

        if (output == stdout)
        {
           #ifdef DISTRHO_OS_MAC
            std::fprintf(output, "\x1b[37;1m[dpf] ");
           #else
            std::fprintf(output, "\x1b[30;1m[dpf] ");
           #endif
            std::vfprintf(output, fmt, args);
            std::fprintf(output, "\x1b[0m\n");
        }
        else
        {
            std::fprintf(output, "[dpf] ");
            std::vfprintf(output, fmt, args);
            std::fprintf(output, "\n");
        }

        std::fflush(output);
        va_end(args);
    } catch (...) {}
}
#endif

/**
   Print a string to stdout with newline.
 */
static inline
void d_stdout(const char* const fmt, ...) noexcept
{
    static FILE* const output = __d_fopen("/tmp/dpf.stdout.log", stdout);

    try {
        va_list args;
        va_start(args, fmt);
        std::fprintf(output, "[dpf] ");
        std::vfprintf(output, fmt, args);
        std::fprintf(output, "\n");
       #ifndef DEBUG
        if (output != stdout)
       #endif
            std::fflush(output);
        va_end(args);
    } catch (...) {}
}

/**
   Print a string to stderr with newline.
 */
static inline
void d_stderr(const char* const fmt, ...) noexcept
{
    static FILE* const output = __d_fopen("/tmp/dpf.stderr.log", stderr);

    try {
        va_list args;
        va_start(args, fmt);
        std::fprintf(output, "[dpf] ");
        std::vfprintf(output, fmt, args);
        std::fprintf(output, "\n");
       #ifndef DEBUG
        if (output != stderr)
       #endif
            std::fflush(output);
        va_end(args);
    } catch (...) {}
}

/**
   Print a string to stderr with newline (red color).
 */
static inline
void d_stderr2(const char* const fmt, ...) noexcept
{
    static FILE* const output = __d_fopen("/tmp/dpf.stderr2.log", stderr);

    try {
        va_list args;
        va_start(args, fmt);

        if (output == stdout)
        {
            std::fprintf(output, "\x1b[31m[dpf] ");
            std::vfprintf(output, fmt, args);
            std::fprintf(output, "\x1b[0m\n");
        }
        else
        {
            std::fprintf(output, "[dpf] ");
            std::vfprintf(output, fmt, args);
            std::fprintf(output, "\n");
        }

        std::fflush(output);
        va_end(args);
    } catch (...) {}
}

/**
   Print a safe assertion error message.
 */
static inline
void d_safe_assert(const char* const assertion, const char* const file, const int line) noexcept
{
    d_stderr2("assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

/**
   Print a safe assertion error message, with 1 extra signed integer value.
 */
static inline
void d_safe_assert_int(const char* const assertion, const char* const file,
                       const int line, const int value) noexcept
{
    d_stderr2("assertion failure: \"%s\" in file %s, line %i, value %i", assertion, file, line, value);
}

/**
   Print a safe assertion error message, with 1 extra unsigned integer value.
 */
static inline
void d_safe_assert_uint(const char* const assertion, const char* const file,
                        const int line, const uint value) noexcept
{
    d_stderr2("assertion failure: \"%s\" in file %s, line %i, value %u", assertion, file, line, value);
}

/**
   Print a safe assertion error message, with 2 extra signed integer values.
 */
static inline
void d_safe_assert_int2(const char* const assertion, const char* const file,
                        const int line, const int v1, const int v2) noexcept
{
    d_stderr2("assertion failure: \"%s\" in file %s, line %i, v1 %i, v2 %i", assertion, file, line, v1, v2);
}

/**
   Print a safe assertion error message, with 2 extra unsigned integer values.
 */
static inline
void d_safe_assert_uint2(const char* const assertion, const char* const file,
                         const int line, const uint v1, const uint v2) noexcept
{
    d_stderr2("assertion failure: \"%s\" in file %s, line %i, v1 %u, v2 %u", assertion, file, line, v1, v2);
}

/**
   Print a safe assertion error message, with a custom error message.
 */
static inline
void d_custom_safe_assert(const char* const message, const char* const assertion, const char* const file,
                          const int line) noexcept
{
    d_stderr2("assertion failure: %s, condition \"%s\" in file %s, line %i", message, assertion, file, line);
}

/**
   Print a safe exception error message.
 */
static inline
void d_safe_exception(const char* const exception, const char* const file, const int line) noexcept
{
    d_stderr2("exception caught: \"%s\" in file %s, line %i", exception, file, line);
}

/** @} */

/* --------------------------------------------------------------------------------------------------------------------
 * math functions */

/**
   @defgroup MathFunctions Math related functions

   @{
 */

/**
   Safely compare two floating point numbers.
   Returns true if they match.
 */
template<typename T>
static inline constexpr
bool d_isEqual(const T& v1, const T& v2)
{
    return std::abs(v1-v2) < std::numeric_limits<T>::epsilon();
}

/**
   Safely compare two floating point numbers.
   Returns true if they don't match.
 */
template<typename T>
static inline constexpr
bool d_isNotEqual(const T& v1, const T& v2)
{
    return std::abs(v1-v2) >= std::numeric_limits<T>::epsilon();
}

/**
   Safely check if a floating point number is zero.
 */
template<typename T>
static inline constexpr
bool d_isZero(const T& value)
{
    return std::abs(value) < std::numeric_limits<T>::epsilon();
}

/**
   Safely check if a floating point number is not zero.
 */
template<typename T>
static inline constexpr
bool d_isNotZero(const T& value)
{
    return std::abs(value) >= std::numeric_limits<T>::epsilon();
}

/**
   Get next power of 2.
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

/**
   Round a floating point number to an integer.
   Fast operation for values known to be 0 or positive.
 */
template<typename T>
static inline constexpr
int32_t d_roundToIntPositive(const T& value)
{
    return static_cast<int32_t>(value + static_cast<T>(0.5));
}

/**
   Round a floating point number to an unsigned integer.
   Fast operation for values known to be 0 or positive.
 */
template<typename T>
static inline constexpr
uint32_t d_roundToUnsignedInt(const T& value)
{
    return static_cast<uint32_t>(value + static_cast<T>(0.5));
}

/**
   Round a floating point number to an integer.
   Fast operation for values known to be 0 or negative.
 */
template<typename T>
static inline constexpr
int32_t d_roundToIntNegative(const T& value)
{
    return static_cast<int32_t>(value - static_cast<T>(0.5));
}

/**
   Round a floating point number to integer.
 */
template<typename T>
static inline constexpr
int32_t d_roundToInt(const T& value)
{
    return value >= 0 ? static_cast<int32_t>(value + static_cast<T>(0.5))
                      : static_cast<int32_t>(value - static_cast<T>(0.5));
}

/** @} */

/* --------------------------------------------------------------------------------------------------------------------
 * other stuff */

#ifndef DONT_SET_USING_DISTRHO_NAMESPACE
/**
   If your code uses a lot of DISTRHO classes, then this will obviously save you a lot of typing,
   but can be disabled by setting DONT_SET_USING_DISTRHO_NAMESPACE.
 */
namespace DISTRHO_NAMESPACE {}
using namespace DISTRHO_NAMESPACE;
#endif

#endif // DISTRHO_UTILS_HPP_INCLUDED
