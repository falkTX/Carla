/*
 * Carla common defines
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __CARLA_DEFINES_HPP__
#define __CARLA_DEFINES_HPP__

#include <QtCore/Qt>

// If the compiler can't do C++11 lambdas, it most likely doesn't know about nullptr either
#ifndef Q_COMPILER_LAMBDA
# define nullptr (0)
#endif

// Common includes
#ifdef Q_OS_WIN
# include <winsock2.h>
# include <windows.h>
#else
# include <unistd.h>
# ifndef __cdecl
#  define __cdecl
# endif
#endif

// Define various string format types, needed for qDebug/Warning/Critical sections
#if defined(Q_OS_WIN64)
# define P_INT64   "%I64i"
# define P_INTPTR  "%I64i"
# define P_UINTPTR "%I64x"
# define P_SIZE    "%I64u"
#elif defined(Q_OS_WIN32)
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
#if defined(Q_OS_HAIKU) || defined(Q_OS_UNIX)
# ifdef __LP64__
#  define BINARY_NATIVE BINARY_POSIX64
# else
#  define BINARY_NATIVE BINARY_POSIX32
# endif
#elif defined(Q_OS_WIN)
# ifdef Q_OS_WIN64
#  define BINARY_NATIVE BINARY_WIN64
# else
#  define BINARY_NATIVE BINARY_WIN32
# endif
#else
# warning Unknown binary native
# define BINARY_NATIVE BINARY_OTHER
#endif

// Define CARLA_ASSERT*
#ifdef NDEBUG
# define CARLA_ASSERT(cond) ((!(cond)) ? carla_assert(#cond, __FILE__, __LINE__) : qt_noop())
# define CARLA_ASSERT_INT(cond, value) ((!(cond)) ? carla_assert_int(#cond, __FILE__, __LINE__, value) : qt_noop())
#else
# define CARLA_ASSERT Q_ASSERT
# define CARLA_ASSERT_INT(cond, value) Q_ASSERT(cond)
#endif

// Define CARLA_EXPORT
#ifdef BUILD_BRIDGE
# define CARLA_EXPORT extern "C"
#else
# if defined(Q_OS_WIN) && ! defined(__WINE__)
#  define CARLA_EXPORT extern "C" __declspec (dllexport)
# else
#  define CARLA_EXPORT extern "C" __attribute__ ((visibility("default")))
# endif
#endif

#endif // __CARLA_DEFINES_HPP__
