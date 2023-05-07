/*
 * Carla common defines
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_DEFINES_H_INCLUDED
#define CARLA_DEFINES_H_INCLUDED

/* Compatibility with non-clang compilers */
#ifndef __has_feature
# define __has_feature(x) 0
#endif
#ifndef __has_extension
# define __has_extension __has_feature
#endif

/* Set Version */
#define CARLA_VERSION_HEX    0x020591
#define CARLA_VERSION_STRING "2.6.0-alpha1"
#define CARLA_VERSION_STRMIN "2.6"

/* Check OS */
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(_M_ARM64)
# define CARLA_OS_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_M_ARM)
# define CARLA_OS_WIN32
#elif defined(__APPLE__)
# define CARLA_OS_MAC
#elif defined(__HAIKU__)
# define CARLA_OS_HAIKU
#elif defined(__linux__) || defined(__linux)
# define CARLA_OS_LINUX
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# define CARLA_OS_BSD
#elif defined(__GNU__)
# define CARLA_OS_GNU_HURD
#elif defined(__EMSCRIPTEN__)
# define CARLA_OS_WASM
#else
# warning Unsupported platform!
#endif

#if defined(CARLA_OS_WIN32) || defined(CARLA_OS_WIN64)
# define CARLA_OS_WIN
#elif defined(CARLA_OS_BSD) || defined(CARLA_OS_GNU_HURD) || defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC)
# define CARLA_OS_UNIX
#endif

#if defined(__LP64__) || defined(_LP64) || defined(__arm64__) || defined(__aarch64__) || defined(CARLA_OS_WIN64)
# define CARLA_OS_64BIT
#endif

/* Check for C++11 support */
#if defined(HAVE_CPP11_SUPPORT)
# if HAVE_CPP11_SUPPORT
#   define CARLA_PROPER_CPP11_SUPPORT
# endif
#elif defined(__cplusplus)
# if __cplusplus >= 201103L || (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405 && defined(__GXX_EXPERIMENTAL_CXX0X__)) || __has_extension(cxx_noexcept) || defined(_MSC_VER)
#  define CARLA_PROPER_CPP11_SUPPORT
#  if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407 && ! defined(__clang__)) || (defined(__clang__) && ! __has_extension(cxx_override_control))
#   define override /* gcc4.7+ only */
#   define final    /* gcc4.7+ only */
#  endif
# endif
#endif

#if defined(__cplusplus) && ! defined(CARLA_PROPER_CPP11_SUPPORT)
# define constexpr
# define noexcept throw()
# define override
# define final
# define nullptr NULL
#endif

/* Common includes */
#ifdef __cplusplus
# include <cstddef>
#else
# include <stdbool.h>
# include <stddef.h>
# define constexpr /* note: constexpr is coming to C soon, but no compilers support it yet */
#endif

/* Define various string format types */
#if defined(CARLA_OS_WIN64)
# define P_INT64   "%lli"
# define P_UINT64  "%llu"
# define P_INTPTR  "%lli"
# define P_UINTPTR "%llx"
# define P_SIZE    "%llu"
# define P_SSIZE   "%lli"
#elif defined(CARLA_OS_WIN32)
# define P_INT64   "%lli"
# define P_UINT64  "%llu"
# define P_INTPTR  "%i"
# define P_UINTPTR "%x"
# define P_SIZE    "%u"
# define P_SSIZE   "%i"
#elif defined(CARLA_OS_WASM)
# define P_INT64   "%lli"
# define P_UINT64  "%llu"
# define P_INTPTR  "%li"
# define P_UINTPTR "%lx"
# define P_SIZE    "%lu"
# define P_SSIZE   "%li"
#elif defined(CARLA_OS_MAC)
# define P_INT64   "%lli"
# define P_UINT64  "%llu"
# define P_INTPTR  "%li"
# define P_UINTPTR "%lx"
# define P_SIZE    "%lu"
# define P_SSIZE   "%li"
#elif (defined(__WORDSIZE)     && __WORDSIZE     == 64) || \
      (defined(__SIZE_WIDTH__) && __SIZE_WIDTH__ == 64) || \
      (defined(CARLA_OS_HAIKU) && defined(CARLA_OS_64BIT))
# define P_INT64   "%li"
# define P_UINT64  "%lu"
# define P_INTPTR  "%li"
# define P_UINTPTR "%lx"
# define P_SIZE    "%lu"
# define P_SSIZE   "%li"
#else
# define P_INT64   "%lli"
# define P_UINT64  "%llu"
# define P_INTPTR  "%i"
# define P_UINTPTR "%x"
# define P_SIZE    "%u"
# define P_SSIZE   "%i"
#endif

/* Define BINARY_NATIVE */
#if defined(CARLA_OS_WIN)
# ifdef CARLA_OS_WIN64
#  define BINARY_NATIVE BINARY_WIN64
# else
#  define BINARY_NATIVE BINARY_WIN32
# endif
#else
# ifdef CARLA_OS_64BIT
#  define BINARY_NATIVE BINARY_POSIX64
# else
#  define BINARY_NATIVE BINARY_POSIX32
# endif
#endif

/* Define unlikely */
#ifdef __GNUC__
# define unlikely(x) __builtin_expect(x,0)
#else
# define unlikely(x) x
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
#define CARLA_SAFE_ASSERT(cond)               if (unlikely(!(cond))) carla_safe_assert      (#cond, __FILE__, __LINE__);
#define CARLA_SAFE_ASSERT_INT(cond, value)    if (unlikely(!(cond))) carla_safe_assert_int  (#cond, __FILE__, __LINE__, static_cast<int>(value));
#define CARLA_SAFE_ASSERT_INT2(cond, v1, v2)  if (unlikely(!(cond))) carla_safe_assert_int2 (#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2));
#define CARLA_SAFE_ASSERT_UINT(cond, value)   if (unlikely(!(cond))) carla_safe_assert_uint (#cond, __FILE__, __LINE__, static_cast<uint>(value));
#define CARLA_SAFE_ASSERT_UINT2(cond, v1, v2) if (unlikely(!(cond))) carla_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2));

#define CARLA_SAFE_ASSERT_BREAK(cond)         if (unlikely(!(cond))) { carla_safe_assert(#cond, __FILE__, __LINE__); break; }
#define CARLA_SAFE_ASSERT_CONTINUE(cond)      if (unlikely(!(cond))) { carla_safe_assert(#cond, __FILE__, __LINE__); continue; }
#define CARLA_SAFE_ASSERT_RETURN(cond, ret)   if (unlikely(!(cond))) { carla_safe_assert(#cond, __FILE__, __LINE__); return ret; }

#define CARLA_CUSTOM_SAFE_ASSERT(msg, cond)             if (unlikely(!(cond))) carla_custom_safe_assert(msg, #cond, __FILE__, __LINE__);
#define CARLA_CUSTOM_SAFE_ASSERT_BREAK(msg, cond)       if (unlikely(!(cond))) { carla_custom_safe_assert(msg, #cond, __FILE__, __LINE__); break; }
#define CARLA_CUSTOM_SAFE_ASSERT_CONTINUE(msg, cond)    if (unlikely(!(cond))) { carla_custom_safe_assert(msg, #cond, __FILE__, __LINE__); continue; }
#define CARLA_CUSTOM_SAFE_ASSERT_RETURN(msg, cond, ret) if (unlikely(!(cond))) { carla_custom_safe_assert(msg, #cond, __FILE__, __LINE__); return ret; }

#define CARLA_CUSTOM_SAFE_ASSERT_ONCE_BREAK(msg, cond)       if (unlikely(!(cond))) { static bool _p; if (!_p) { _p = true; carla_custom_safe_assert(msg, #cond, __FILE__, __LINE__); } break; }
#define CARLA_CUSTOM_SAFE_ASSERT_ONCE_CONTINUE(msg, cond)    if (unlikely(!(cond))) { static bool _p; if (!_p) { _p = true; carla_custom_safe_assert(msg, #cond, __FILE__, __LINE__); } continue; }
#define CARLA_CUSTOM_SAFE_ASSERT_ONCE_RETURN(msg, cond, ret) if (unlikely(!(cond))) { static bool _p; if (!_p) { _p = true; carla_custom_safe_assert(msg, #cond, __FILE__, __LINE__); } return ret; }

#define CARLA_SAFE_ASSERT_INT_BREAK(cond, value)       if (unlikely(!(cond))) { carla_safe_assert_int(#cond, __FILE__, __LINE__, static_cast<int>(value)); break; }
#define CARLA_SAFE_ASSERT_INT_CONTINUE(cond, value)    if (unlikely(!(cond))) { carla_safe_assert_int(#cond, __FILE__, __LINE__, static_cast<int>(value)); continue; }
#define CARLA_SAFE_ASSERT_INT_RETURN(cond, value, ret) if (unlikely(!(cond))) { carla_safe_assert_int(#cond, __FILE__, __LINE__, static_cast<int>(value)); return ret; }

#define CARLA_SAFE_ASSERT_INT2_BREAK(cond, v1, v2)        if (unlikely(!(cond))) { carla_safe_assert_int2(#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2)); break; }
#define CARLA_SAFE_ASSERT_INT2_CONTINUE(cond, v1, v2)     if (unlikely(!(cond))) { carla_safe_assert_int2(#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2)); continue; }
#define CARLA_SAFE_ASSERT_INT2_RETURN(cond, v1, v2, ret)  if (unlikely(!(cond))) { carla_safe_assert_int2(#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2)); return ret; }

#define CARLA_SAFE_ASSERT_UINT_BREAK(cond, value)       if (unlikely(!(cond))) { carla_safe_assert_uint(#cond, __FILE__, __LINE__, static_cast<uint>(value); break; }
#define CARLA_SAFE_ASSERT_UINT_CONTINUE(cond, value)    if (unlikely(!(cond))) { carla_safe_assert_uint(#cond, __FILE__, __LINE__, static_cast<uint>(value)); continue; }
#define CARLA_SAFE_ASSERT_UINT_RETURN(cond, value, ret) if (unlikely(!(cond))) { carla_safe_assert_uint(#cond, __FILE__, __LINE__, static_cast<uint>(value)); return ret; }

#define CARLA_SAFE_ASSERT_UINT2_BREAK(cond, v1, v2)       if (unlikely(!(cond))) { carla_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2)); break; }
#define CARLA_SAFE_ASSERT_UINT2_CONTINUE(cond, v1, v2)    if (unlikely(!(cond))) { carla_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2)); continue; }
#define CARLA_SAFE_ASSERT_UINT2_RETURN(cond, v1, v2, ret) if (unlikely(!(cond))) { carla_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2)); return ret; }

#if defined(__GNUC__) && defined(CARLA_PROPER_CPP11_SUPPORT) && ! defined(__clang__)
# define CARLA_CATCH_UNWIND catch (abi::__forced_unwind&) { throw; }
# if __GNUC__ >= 6
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wterminate"
# endif
#else
# define CARLA_CATCH_UNWIND
#endif

/* Define CARLA_SAFE_EXCEPTION */
#define CARLA_SAFE_EXCEPTION(msg)             CARLA_CATCH_UNWIND catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); }
#define CARLA_SAFE_EXCEPTION_BREAK(msg)       CARLA_CATCH_UNWIND catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); break; }
#define CARLA_SAFE_EXCEPTION_CONTINUE(msg)    CARLA_CATCH_UNWIND catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); continue; }
#define CARLA_SAFE_EXCEPTION_RETURN(msg, ret) CARLA_CATCH_UNWIND catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); return ret; }

/* Define CARLA_DECLARE_NON_COPYABLE */
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define CARLA_DECLARE_NON_COPYABLE(ClassName) \
private:                                         \
    ClassName(ClassName&) = delete;              \
    ClassName(const ClassName&) = delete;        \
    ClassName& operator=(ClassName&) = delete;   \
    ClassName& operator=(const ClassName&) = delete;
#else
# define CARLA_DECLARE_NON_COPYABLE(ClassName) \
private:                                         \
    ClassName(ClassName&);                       \
    ClassName(const ClassName&);                 \
    ClassName& operator=(ClassName&);            \
    ClassName& operator=(const ClassName&);
#endif

/* Define CARLA_PREVENT_HEAP_ALLOCATION */
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define CARLA_PREVENT_HEAP_ALLOCATION \
private:                               \
    static void* operator new(std::size_t) = delete; \
    static void operator delete(void*) = delete;
#else
# define CARLA_PREVENT_HEAP_ALLOCATION \
private:                               \
    static void* operator new(std::size_t); \
    static void operator delete(void*);
#endif

/* Define CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION */
#ifdef CARLA_PROPER_CPP11_SUPPORT
# define CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION  \
private:                                        \
    static void* operator new(std::size_t) = delete;
#else
# define CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION  \
private:                                        \
    static void* operator new(std::size_t);
#endif

/* CARLA_VISIBLE_SYMBOL */
#if defined(CARLA_OS_WIN) && ! defined(__WINE__)
# ifdef BUILDING_CARLA
#  define CARLA_VISIBLE_SYMBOL __declspec (dllexport)
# else
#  define CARLA_VISIBLE_SYMBOL __declspec (dllimport)
# endif
#else
# define CARLA_VISIBLE_SYMBOL __attribute__ ((visibility("default")))
#endif

/* Define CARLA_API */
#if defined(BUILD_BRIDGE) || defined(STATIC_PLUGIN_TARGET)
# define CARLA_API
#else
# define CARLA_API CARLA_VISIBLE_SYMBOL
#endif

/* Define CARLA_EXTERN_C */
#ifdef __cplusplus
# define CARLA_EXTERN_C extern "C"
#else
# define CARLA_EXTERN_C
#endif

/* Define CARLA_*_EXPORT */
#ifdef BUILD_BRIDGE
# define CARLA_API_EXPORT CARLA_EXTERN_C
# define CARLA_PLUGIN_EXPORT CARLA_EXTERN_C
#else
# define CARLA_API_EXPORT CARLA_EXTERN_C CARLA_API
# define CARLA_PLUGIN_EXPORT CARLA_EXTERN_C CARLA_VISIBLE_SYMBOL
#endif

/* Define CARLA_OS_SEP */
#ifdef CARLA_OS_WIN
# define CARLA_OS_SEP       '\\'
# define CARLA_OS_SEP_STR   "\\"
# define CARLA_OS_SPLIT     ';'
# define CARLA_OS_SPLIT_STR ";"
#else
# define CARLA_OS_SEP       '/'
# define CARLA_OS_SEP_STR   "/"
# define CARLA_OS_SPLIT     ':'
# define CARLA_OS_SPLIT_STR ":"
#endif

/* Useful typedefs */
typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef unsigned long int ulong;
typedef unsigned long long int ulonglong;
#ifdef _MSC_VER
# include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#endif /* CARLA_DEFINES_H_INCLUDED */
