/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_DEFINES_H_INCLUDED
#define DISTRHO_DEFINES_H_INCLUDED

/* Compatibility with non-clang compilers */
#ifndef __has_feature
# define __has_feature(x) 0
#endif
#ifndef __has_extension
# define __has_extension __has_feature
#endif

/* Check OS */
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define DISTRHO_PLUGIN_EXPORT extern "C" __declspec (dllexport)
# define DISTRHO_OS_WINDOWS    1
# define DISTRHO_DLL_EXTENSION "dll"
#else
# define DISTRHO_PLUGIN_EXPORT extern "C" __attribute__ ((visibility("default")))
# if defined(__APPLE__)
#  define DISTRHO_OS_MAC        1
#  define DISTRHO_DLL_EXTENSION "dylib"
# elif defined(__HAIKU__)
#  define DISTRHO_OS_HAIKU      1
# elif defined(__linux__) || defined(__linux)
#  define DISTRHO_OS_LINUX      1
#  define DISTRHO_OS_LINUX_FULL 1
# elif defined(__FreeBSD__) || defined(__GNU__)
#  define DISTRHO_OS_LINUX      1
# endif
#endif

#if defined(DISTRHO_OS_LINUX) || defined(DISTRHO_OS_MAC)
# define DISTRHO_OS_UNIX
#endif

#ifndef DISTRHO_DLL_EXTENSION
# define DISTRHO_DLL_EXTENSION "so"
#endif

/* Check for C++11 support */
#if defined(HAVE_CPP11_SUPPORT)
# if HAVE_CPP11_SUPPORT
#  define DISTRHO_PROPER_CPP11_SUPPORT
# endif
#elif __cplusplus >= 201103L || (defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405) || __has_extension(cxx_noexcept)
# define DISTRHO_PROPER_CPP11_SUPPORT
# if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407 && ! defined(__clang__)) || (defined(__clang__) && ! __has_extension(cxx_override_control))
#  define override // gcc4.7+ only
#  define final    // gcc4.7+ only
# endif
#endif

#ifndef DISTRHO_PROPER_CPP11_SUPPORT
# define noexcept throw()
# define override
# define final
# define nullptr NULL
#endif

/* Define DISTRHO_SAFE_ASSERT* */
#define DISTRHO_SAFE_ASSERT(cond)               if (! (cond))   d_safe_assert(#cond, __FILE__, __LINE__);
#define DISTRHO_SAFE_ASSERT_BREAK(cond)         if (! (cond)) { d_safe_assert(#cond, __FILE__, __LINE__); break; }
#define DISTRHO_SAFE_ASSERT_CONTINUE(cond)      if (! (cond)) { d_safe_assert(#cond, __FILE__, __LINE__); continue; }
#define DISTRHO_SAFE_ASSERT_RETURN(cond, ret)   if (! (cond)) { d_safe_assert(#cond, __FILE__, __LINE__); return ret; }

/* Define DISTRHO_SAFE_EXCEPTION */
#define DISTRHO_SAFE_EXCEPTION(msg)             catch(...) { d_safe_exception(msg, __FILE__, __LINE__); }
#define DISTRHO_SAFE_EXCEPTION_BREAK(msg)       catch(...) { d_safe_exception(msg, __FILE__, __LINE__); break; }
#define DISTRHO_SAFE_EXCEPTION_CONTINUE(msg)    catch(...) { d_safe_exception(msg, __FILE__, __LINE__); continue; }
#define DISTRHO_SAFE_EXCEPTION_RETURN(msg, ret) catch(...) { d_safe_exception(msg, __FILE__, __LINE__); return ret; }

/* Define DISTRHO_DECLARE_NON_COPY_CLASS */
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# define DISTRHO_DECLARE_NON_COPY_CLASS(ClassName) \
private:                                           \
    ClassName(ClassName&) = delete;                \
    ClassName(const ClassName&) = delete;          \
    ClassName& operator=(ClassName&) = delete  ;   \
    ClassName& operator=(const ClassName&) = delete;
#else
# define DISTRHO_DECLARE_NON_COPY_CLASS(ClassName) \
private:                                           \
    ClassName(ClassName&);                         \
    ClassName(const ClassName&);                   \
    ClassName& operator=(ClassName&);              \
    ClassName& operator=(const ClassName&);
#endif

/* Define DISTRHO_DECLARE_NON_COPY_STRUCT */
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# define DISTRHO_DECLARE_NON_COPY_STRUCT(StructName) \
    StructName(StructName&) = delete;                \
    StructName(const StructName&) = delete;          \
    StructName& operator=(StructName&) = delete;     \
    StructName& operator=(const StructName&) = delete;
#else
# define DISTRHO_DECLARE_NON_COPY_STRUCT(StructName)
#endif

/* Define DISTRHO_PREVENT_HEAP_ALLOCATION */
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# define DISTRHO_PREVENT_HEAP_ALLOCATION        \
private:                                        \
    static void* operator new(size_t) = delete; \
    static void operator delete(void*) = delete;
#else
# define DISTRHO_PREVENT_HEAP_ALLOCATION \
private:                                 \
    static void* operator new(size_t);   \
    static void operator delete(void*);
#endif

/* Define namespace */
#ifndef DISTRHO_NAMESPACE
# define DISTRHO_NAMESPACE DISTRHO
#endif
#define START_NAMESPACE_DISTRHO namespace DISTRHO_NAMESPACE {
#define END_NAMESPACE_DISTRHO }
#define USE_NAMESPACE_DISTRHO using namespace DISTRHO_NAMESPACE;

/* Useful typedefs */
typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef unsigned long int ulong;

#endif // DISTRHO_DEFINES_H_INCLUDED
