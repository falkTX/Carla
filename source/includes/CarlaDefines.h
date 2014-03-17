/*
 * Carla common defines
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

#ifndef CARLA_DEFINES_HPP_INCLUDED
#define CARLA_DEFINES_HPP_INCLUDED

/* IDE Helper */
#ifndef REAL_BUILD
# include "config.h"
#endif

/* Set Version */
#define CARLA_VERSION_HEX    0x01093
#define CARLA_VERSION_STRING "1.9.3 (2.0-beta1)"

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
#elif defined(__GNUC__) && defined(__cplusplus)
# if (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
#  define CARLA_PROPER_CPP11_SUPPORT
#  if (__GNUC__ * 100 + __GNUC_MINOR__) < 407
#   define override // gcc4.7+ only
#   define final    // gcc4.7+ only
#  endif
# endif
#endif

#if defined(__cplusplus) && !defined(CARLA_PROPER_CPP11_SUPPORT)
# ifndef __clang__
#  define noexcept throw()
# else
#  define noexcept
# endif
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

/* Define various string format types */
#if defined(CARLA_OS_WIN64)
# define P_INT64   "%I64i"
# define P_INTPTR  "%I64i"
# define P_UINTPTR "%I64x"
# define P_SIZE    "%I64u"
#elif defined(CARLA_OS_WIN32)
# define P_INT64   "%I64i"
# define P_INTPTR  "%i"
# define P_UINTPTR "%x"
# define P_SIZE    "%u"
#elif defined(__WORDSIZE) && __WORDSIZE == 64
# define P_INT64   "%li"
# define P_INTPTR  "%li"
# define P_UINTPTR "%lx"
# define P_SIZE    "%lu"
#else
# define P_INT64   "%lli"
# define P_INTPTR  "%i"
# define P_UINTPTR "%x"
# define P_SIZE    "%u"
#endif

/* Define BINARY_NATIVE */
#if defined(CARLA_OS_HAIKU) || defined(CARLA_OS_UNIX)
# if defined (__LP64__) || defined (_LP64)
#  define BINARY_NATIVE BINARY_POSIX64
# else
#  define BINARY_NATIVE BINARY_POSIX32
# endif
#elif defined(CARLA_OS_WIN)
# ifdef CARLA_OS_WIN64
#  define BINARY_NATIVE BINARY_WIN64
# else
#  define BINARY_NATIVE BINARY_WIN32
# endif
#else
# warning Unknown binary native
# define BINARY_NATIVE BINARY_OTHER
#endif

/* Define CARLA_ASSERT* */
#if defined(CARLA_NO_ASSERTS)
# define CARLA_ASSERT(cond)
# define CARLA_ASSERT_INT(cond, value)
# define CARLA_ASSERT_INT2(cond, v1, v2)
#elif defined(NDEBUG)
# define CARLA_ASSERT      CARLA_SAFE_ASSERT
# define CARLA_ASSERT_INT  CARLA_SAFE_ASSERT_INT
# define CARLA_ASSERT_INT2 CARLA_SAFE_ASSERT_INT2
#else
# define CARLA_ASSERT(cond)              assert(cond)
# define CARLA_ASSERT_INT(cond, value)   assert(cond)
# define CARLA_ASSERT_INT2(cond, v1, v2) assert(cond)
#endif

/* Define CARLA_SAFE_ASSERT* */
#define CARLA_SAFE_ASSERT(cond)               if (cond) pass(); else carla_safe_assert      (#cond, __FILE__, __LINE__);
#define CARLA_SAFE_ASSERT_INT(cond, value)    if (cond) pass(); else carla_safe_assert_int  (#cond, __FILE__, __LINE__, static_cast<int>(value));
#define CARLA_SAFE_ASSERT_INT2(cond, v1, v2)  if (cond) pass(); else carla_safe_assert_int2 (#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2));
#define CARLA_SAFE_ASSERT_UINT(cond, value)   if (cond) pass(); else carla_safe_assert_uint (#cond, __FILE__, __LINE__, static_cast<uint>(value));
#define CARLA_SAFE_ASSERT_UINT2(cond, v1, v2) if (cond) pass(); else carla_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2));

#define CARLA_SAFE_ASSERT_BREAK(cond)        if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); break; }
#define CARLA_SAFE_ASSERT_CONTINUE(cond)     if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); continue; }
#define CARLA_SAFE_ASSERT_RETURN(cond, ret)  if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); return ret; }

/* Define CARLA_DECLARE_NON_COPY_CLASS */
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define CARLA_DECLARE_NON_COPY_CLASS(ClassName) \
private:                                         \
    ClassName(ClassName&) = delete;              \
    ClassName(const ClassName&) = delete;        \
    ClassName& operator=(ClassName&) = delete;   \
    ClassName& operator=(const ClassName&) = delete;
#else
# define CARLA_DECLARE_NON_COPY_CLASS(ClassName) \
private:                                         \
    ClassName(ClassName&);                       \
    ClassName(const ClassName&);                 \
    ClassName& operator=(ClassName&);            \
    ClassName& operator=(const ClassName&);
#endif

/* Define CARLA_DECLARE_NON_COPY_STRUCT */
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define CARLA_DECLARE_NON_COPY_STRUCT(StructName) \
    StructName(StructName&) = delete;              \
    StructName(const StructName&) = delete;        \
    StructName& operator=(StructName&) = delete;   \
    StructName& operator=(const StructName&) = delete;
#else
# define CARLA_DECLARE_NON_COPY_STRUCT(StructName)
#endif

/* Define CARLA_PREVENT_HEAP_ALLOCATION */
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define CARLA_PREVENT_HEAP_ALLOCATION \
private:                               \
    static void* operator new(size_t) = delete; \
    static void operator delete(void*) = delete;
#else
# define CARLA_PREVENT_HEAP_ALLOCATION \
private:                               \
    static void* operator new(size_t); \
    static void operator delete(void*);
#endif

/* Define CARLA_CATCH_MSG */
#define CARLA_CATCH_MSG(msg, after) \
    catch(...) {                                                     \
        carla_stderr2("Caught exception: " msg); \
        after                                                        \
    }

/* Define EXTERN_C */
#ifdef __cplusplus
# define EXTERN_C extern "C"
#else
# define EXTERN_C
#endif

/* Define CARLA_EXPORT */
#ifdef BUILD_BRIDGE
# define CARLA_EXPORT EXTERN_C
#else
# if defined(CARLA_OS_WIN) && ! defined(__WINE__)
#  define CARLA_EXPORT EXTERN_C __declspec (dllexport)
# else
#  define CARLA_EXPORT EXTERN_C __attribute__ ((visibility("default")))
# endif
#endif

/* Define PRE/POST_PACKED_STRUCTURE */
#if defined(__GNUC__)
# define PRE_PACKED_STRUCTURE
# define POST_PACKED_STRUCTURE __attribute__((__packed__))
#elif defined(_MSC_VER)
# define PRE_PACKED_STRUCTURE1 __pragma(pack(push,1))
# define PRE_PACKED_STRUCTURE    PRE_PACKED_STRUCTURE1
# define POST_PACKED_STRUCTURE ;__pragma(pack(pop))
#else
# define PRE_PACKED_STRUCTURE
# define POST_PACKED_STRUCTURE
#endif

/* Define OS_SEP */
#ifdef CARLA_OS_WIN
# define OS_SEP     '\\'
# define OS_SEP_STR "\\"
#else
# define OS_SEP     '/'
# define OS_SEP_STR "/"
#endif

/* Useful typedefs */
typedef unsigned char uchar;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;

#endif /* CARLA_DEFINES_HPP_INCLUDED */
