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

// -----------------------------------------------------------------------
// defines

/* Compatibility with non-clang compilers */
#ifndef __has_feature
# define __has_feature(x) 0
#endif
#ifndef __has_extension
# define __has_extension __has_feature
#endif

/* Check OS */
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
# define CARLA_OS_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define CARLA_OS_WIN32
#elif defined(__APPLE__)
# define CARLA_OS_MAC
#elif defined(__HAIKU__)
# define CARLA_OS_HAIKU
#elif defined(__linux__) || defined(__linux)
# define CARLA_OS_LINUX
#else
# warning Unsupported platform!
#endif

#if defined(CARLA_OS_WIN32) || defined(CARLA_OS_WIN64)
# define CARLA_OS_WIN
#elif ! defined(CARLA_OS_HAIKU)
# define CARLA_OS_UNIX
#endif

/* Check for C++11 support */
#if defined(HAVE_CPP11_SUPPORT)
# define CARLA_PROPER_CPP11_SUPPORT
#elif defined(__cplusplus)
# if __cplusplus >= 201103L || (defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405) || __has_extension(cxx_noexcept)
#  define CARLA_PROPER_CPP11_SUPPORT
#  if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407) || ! __has_extension(cxx_override_control)
#   define override // gcc4.7+ only
#   define final    // gcc4.7+ only
#  endif
# endif
#endif

#if defined(__cplusplus) && !defined(CARLA_PROPER_CPP11_SUPPORT)
# define noexcept throw()
# define override
# define final
# define nullptr (0)
#endif

/* Common includes */
#ifdef __cplusplus
# include <cstddef>
#else
# include <stdbool.h>
# include <stddef.h>
#endif

/* Define CARLA_SAFE_ASSERT* */
#define CARLA_SAFE_ASSERT(cond)               if (cond) pass(); else carla_safe_assert      (#cond, __FILE__, __LINE__);
#define CARLA_SAFE_ASSERT_INT(cond, value)    if (cond) pass(); else carla_safe_assert_int  (#cond, __FILE__, __LINE__, static_cast<int>(value));
#define CARLA_SAFE_ASSERT_INT2(cond, v1, v2)  if (cond) pass(); else carla_safe_assert_int2 (#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2));
#define CARLA_SAFE_ASSERT_UINT(cond, value)   if (cond) pass(); else carla_safe_assert_uint (#cond, __FILE__, __LINE__, static_cast<uint>(value));
#define CARLA_SAFE_ASSERT_UINT2(cond, v1, v2) if (cond) pass(); else carla_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2));

#define CARLA_SAFE_ASSERT_BREAK(cond)         if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); break; }
#define CARLA_SAFE_ASSERT_CONTINUE(cond)      if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); continue; }
#define CARLA_SAFE_ASSERT_RETURN(cond, ret)   if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); return ret; }

// -----------------------------------------------------------------------
// utils

#include <cstdarg>
#include <cstdio>

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
void carla_stdout(const char* const fmt, ...) noexcept
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
void carla_stderr(const char* const fmt, ...) noexcept
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
void carla_stderr2(const char* const fmt, ...) noexcept
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

#endif // CARLA_UTILS_HPP_INCLUDED
