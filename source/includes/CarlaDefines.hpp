/*
 * Carla common defines
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

#ifndef __CARLA_DEFINES_HPP__
#define __CARLA_DEFINES_HPP__

// Check OS
#if defined(__APPLE__)
# define CARLA_OS_MAC
#elif defined(__HAIKU__)
# define CARLA_OS_HAIKU
#elif defined(__linux__) || defined(__linux)
# define CARLA_OS_LINUX
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
# define CARLA_OS_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define CARLA_OS_WIN32
#else
# warning Unsupported platform!
#endif

#if defined(CARLA_OS_WIN32) || defined(CARLA_OS_WIN64)
# define CARLA_OS_WIN
#elif ! defined(CARLA_OS_HAIKU)
# define CARLA_OS_UNIX
#endif

// Check for C++11 support
#if defined(HAVE_CPP11_SUPPORT)
# define CARLA_PROPER_CPP11_SUPPORT
# define IS_CPP11 1
#elif defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__)
# if  (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
#  define CARLA_PROPER_CPP11_SUPPORT
# endif
#endif

#ifndef CARLA_PROPER_CPP11_SUPPORT
# define noexcept
# define nullptr (0)
#endif

// Common includes
#ifdef CARLA_OS_WIN
# include <winsock2.h>
# include <windows.h>
#else
# include <unistd.h>
# ifndef __cdecl
#  define __cdecl
# endif
#endif

// Define various string format types
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
#elif __WORDSIZE == 64
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

// Define BINARY_NATIVE
#if defined(CARLA_OS_HAIKU) || defined(CARLA_OS_UNIX)
# ifdef __LP64__
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

// Define CARLA_ASSERT*
#define CARLA_SAFE_ASSERT(cond) ((!(cond)) ? carla_assert(#cond, __FILE__, __LINE__) : pass())
#define CARLA_SAFE_ASSERT_INT(cond, value) ((!(cond)) ? carla_assert_int(#cond, __FILE__, __LINE__, value) : pass())
#define CARLA_SAFE_ASSERT_INT2(cond, v1, v2) ((!(cond)) ? carla_assert_int2(#cond, __FILE__, __LINE__, v1, v2) : pass())

#ifdef NDEBUG
# define CARLA_ASSERT CARLA_SAFE_ASSERT
# define CARLA_ASSERT_INT CARLA_SAFE_ASSERT_INT
# define CARLA_ASSERT_INT2 CARLA_SAFE_ASSERT_INT2
#else
# define CARLA_ASSERT(cond) assert(cond)
# define CARLA_ASSERT_INT(cond, value) assert(cond)
# define CARLA_ASSERT_INT2(cond, v1, v2) assert(cond)
#endif

// Define CARLA_EXPORT
#ifdef BUILD_BRIDGE
# define CARLA_EXPORT extern "C"
#else
# if defined(CARLA_OS_WIN) && ! defined(__WINE__)
#  define CARLA_EXPORT extern "C" __declspec (dllexport)
# else
#  define CARLA_EXPORT extern "C" __attribute__ ((visibility("default")))
# endif
#endif

// Define OS_SEP
#ifdef CARLA_OS_WIN
# define OS_SEP '\\'
#else
# define OS_SEP '/'
#endif

#endif // __CARLA_DEFINES_HPP__
