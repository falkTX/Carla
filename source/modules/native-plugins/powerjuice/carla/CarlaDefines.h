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

/* Set Version */
#define CARLA_VERSION_HEX    0x01093
#define CARLA_VERSION_STRING "1.9.3 (2.0-beta1)"

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

/* Define CARLA_SAFE_EXCEPTION */
#define CARLA_SAFE_EXCEPTION(msg)             catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); }

#define CARLA_SAFE_EXCEPTION_BREAK(msg)       catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); break; }
#define CARLA_SAFE_EXCEPTION_CONTINUE(msg)    catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); continue; }
#define CARLA_SAFE_EXCEPTION_RETURN(msg, ret) catch(...) { carla_safe_exception(msg, __FILE__, __LINE__); return ret; }

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
