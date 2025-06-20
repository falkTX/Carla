/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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
# define DISTRHO_API
# define DISTRHO_PLUGIN_EXPORT extern "C" __declspec (dllexport)
# define DISTRHO_OS_WINDOWS 1
# define DISTRHO_DLL_EXTENSION "dll"
#else
# define DISTRHO_API
# define DISTRHO_PLUGIN_EXPORT extern "C" __attribute__ ((visibility("default")))
# if defined(__APPLE__)
#  define DISTRHO_OS_MAC 1
#  define DISTRHO_DLL_EXTENSION "dylib"
# elif defined(__HAIKU__)
#  define DISTRHO_OS_HAIKU 1
# elif defined(__linux__) || defined(__linux)
#  define DISTRHO_OS_LINUX 1
# elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#  define DISTRHO_OS_BSD 1
# elif defined(__GNU__)
#  define DISTRHO_OS_GNU_HURD 1
# elif defined(__EMSCRIPTEN__)
#  define DISTRHO_OS_WASM 1
# endif
#endif

#ifndef DISTRHO_DLL_EXTENSION
# define DISTRHO_DLL_EXTENSION "so"
#endif

/* Check for C++11 support */
#if defined(HAVE_CPP11_SUPPORT)
# if HAVE_CPP11_SUPPORT
#  define DISTRHO_PROPER_CPP11_SUPPORT
# endif
#elif __cplusplus >= 201103L || (defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405) || __has_extension(cxx_noexcept) || (defined(_MSC_VER) && _MSVC_LANG >= 201103L)
# define DISTRHO_PROPER_CPP11_SUPPORT
# if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407 && ! defined(__clang__)) || (defined(__clang__) && ! __has_extension(cxx_override_control))
#  define override /* gcc4.7+ only */
#  define final    /* gcc4.7+ only */
# endif
#endif

#ifndef DISTRHO_PROPER_CPP11_SUPPORT
# define constexpr
# define noexcept throw()
# define override
# define final
# define nullptr NULL
#endif

/* Define unlikely */
#ifdef __GNUC__
# define unlikely(x) __builtin_expect(x,0)
#else
# define unlikely(x) x
#endif

/* Define DISTRHO_DEPRECATED */
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 480
# define DISTRHO_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
# define DISTRHO_DEPRECATED [[deprecated]] /* Note: __declspec(deprecated) it not applicable to enum members */
#else
# define DISTRHO_DEPRECATED
#endif

/* Define DISTRHO_DEPRECATED_BY */
#if defined(__clang__) && (__clang_major__ * 100 + __clang_minor__) >= 502
# define DISTRHO_DEPRECATED_BY(other) __attribute__((deprecated("", other)))
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 408
# define DISTRHO_DEPRECATED_BY(other) __attribute__((deprecated("Use " other)))
#else
# define DISTRHO_DEPRECATED_BY(other) DISTRHO_DEPRECATED
#endif

/* Define DISTRHO_SAFE_ASSERT* */
#define DISTRHO_SAFE_ASSERT(cond)               if (unlikely(!(cond))) d_safe_assert      (#cond, __FILE__, __LINE__);
#define DISTRHO_SAFE_ASSERT_INT(cond, value)    if (unlikely(!(cond))) d_safe_assert_int  (#cond, __FILE__, __LINE__, static_cast<int>(value));
#define DISTRHO_SAFE_ASSERT_INT2(cond, v1, v2)  if (unlikely(!(cond))) d_safe_assert_int2 (#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2));
#define DISTRHO_SAFE_ASSERT_UINT(cond, value)   if (unlikely(!(cond))) d_safe_assert_uint (#cond, __FILE__, __LINE__, static_cast<uint>(value));
#define DISTRHO_SAFE_ASSERT_UINT2(cond, v1, v2) if (unlikely(!(cond))) d_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2));

#define DISTRHO_SAFE_ASSERT_BREAK(cond)         if (unlikely(!(cond))) { d_safe_assert(#cond, __FILE__, __LINE__); break; }
#define DISTRHO_SAFE_ASSERT_CONTINUE(cond)      if (unlikely(!(cond))) { d_safe_assert(#cond, __FILE__, __LINE__); continue; }
#define DISTRHO_SAFE_ASSERT_RETURN(cond, ret)   if (unlikely(!(cond))) { d_safe_assert(#cond, __FILE__, __LINE__); return ret; }

#define DISTRHO_CUSTOM_SAFE_ASSERT(msg, cond)             if (unlikely(!(cond)))   d_custom_safe_assert(msg, #cond, __FILE__, __LINE__);
#define DISTRHO_CUSTOM_SAFE_ASSERT_BREAK(msg, cond)       if (unlikely(!(cond))) { d_custom_safe_assert(msg, #cond, __FILE__, __LINE__); break; }
#define DISTRHO_CUSTOM_SAFE_ASSERT_CONTINUE(msg, cond)    if (unlikely(!(cond))) { d_custom_safe_assert(msg, #cond, __FILE__, __LINE__); continue; }
#define DISTRHO_CUSTOM_SAFE_ASSERT_RETURN(msg, cond, ret) if (unlikely(!(cond))) { d_custom_safe_assert(msg, #cond, __FILE__, __LINE__); return ret; }

#define DISTRHO_CUSTOM_SAFE_ASSERT_ONCE_BREAK(msg, cond)       if (unlikely(!(cond))) { static bool _p; if (!_p) { _p = true; d_custom_safe_assert(msg, #cond, __FILE__, __LINE__); } break; }
#define DISTRHO_CUSTOM_SAFE_ASSERT_ONCE_CONTINUE(msg, cond)    if (unlikely(!(cond))) { static bool _p; if (!_p) { _p = true; d_custom_safe_assert(msg, #cond, __FILE__, __LINE__); } continue; }
#define DISTRHO_CUSTOM_SAFE_ASSERT_ONCE_RETURN(msg, cond, ret) if (unlikely(!(cond))) { static bool _p; if (!_p) { _p = true; d_custom_safe_assert(msg, #cond, __FILE__, __LINE__); } return ret; }

#define DISTRHO_SAFE_ASSERT_INT_BREAK(cond, value)       if (unlikely(!(cond))) { d_safe_assert_int(#cond, __FILE__, __LINE__, static_cast<int>(value)); break; }
#define DISTRHO_SAFE_ASSERT_INT_CONTINUE(cond, value)    if (unlikely(!(cond))) { d_safe_assert_int(#cond, __FILE__, __LINE__, static_cast<int>(value)); continue; }
#define DISTRHO_SAFE_ASSERT_INT_RETURN(cond, value, ret) if (unlikely(!(cond))) { d_safe_assert_int(#cond, __FILE__, __LINE__, static_cast<int>(value)); return ret; }

#define DISTRHO_SAFE_ASSERT_INT2_BREAK(cond, v1, v2)        if (unlikely(!(cond))) { d_safe_assert_int2(#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2)); break; }
#define DISTRHO_SAFE_ASSERT_INT2_CONTINUE(cond, v1, v2)     if (unlikely(!(cond))) { d_safe_assert_int2(#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2)); continue; }
#define DISTRHO_SAFE_ASSERT_INT2_RETURN(cond, v1, v2, ret)  if (unlikely(!(cond))) { d_safe_assert_int2(#cond, __FILE__, __LINE__, static_cast<int>(v1), static_cast<int>(v2)); return ret; }

#define DISTRHO_SAFE_ASSERT_UINT_BREAK(cond, value)       if (unlikely(!(cond))) { d_safe_assert_uint(#cond, __FILE__, __LINE__, static_cast<uint>(value)); break; }
#define DISTRHO_SAFE_ASSERT_UINT_CONTINUE(cond, value)    if (unlikely(!(cond))) { d_safe_assert_uint(#cond, __FILE__, __LINE__, static_cast<uint>(value)); continue; }
#define DISTRHO_SAFE_ASSERT_UINT_RETURN(cond, value, ret) if (unlikely(!(cond))) { d_safe_assert_uint(#cond, __FILE__, __LINE__, static_cast<uint>(value)); return ret; }

#define DISTRHO_SAFE_ASSERT_UINT2_BREAK(cond, v1, v2)       if (unlikely(!(cond))) { d_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2)); break; }
#define DISTRHO_SAFE_ASSERT_UINT2_CONTINUE(cond, v1, v2)    if (unlikely(!(cond))) { d_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2)); continue; }
#define DISTRHO_SAFE_ASSERT_UINT2_RETURN(cond, v1, v2, ret) if (unlikely(!(cond))) { d_safe_assert_uint2(#cond, __FILE__, __LINE__, static_cast<uint>(v1), static_cast<uint>(v2)); return ret; }

/* Define DISTRHO_SAFE_EXCEPTION */
#define DISTRHO_SAFE_EXCEPTION(msg)             catch(...) { d_safe_exception(msg, __FILE__, __LINE__); }
#define DISTRHO_SAFE_EXCEPTION_BREAK(msg)       catch(...) { d_safe_exception(msg, __FILE__, __LINE__); break; }
#define DISTRHO_SAFE_EXCEPTION_CONTINUE(msg)    catch(...) { d_safe_exception(msg, __FILE__, __LINE__); continue; }
#define DISTRHO_SAFE_EXCEPTION_RETURN(msg, ret) catch(...) { d_safe_exception(msg, __FILE__, __LINE__); return ret; }

/* Define DISTRHO_DECLARE_NON_COPYABLE */
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# define DISTRHO_DECLARE_NON_COPYABLE(ClassName) \
private:                                         \
    ClassName(ClassName&) = delete;              \
    ClassName(const ClassName&) = delete;        \
    ClassName& operator=(ClassName&) = delete;   \
    ClassName& operator=(const ClassName&) = delete;
#else
# define DISTRHO_DECLARE_NON_COPYABLE(ClassName) \
private:                                         \
    ClassName(ClassName&);                       \
    ClassName(const ClassName&);                 \
    ClassName& operator=(ClassName&);            \
    ClassName& operator=(const ClassName&);
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

/* Define DISTRHO_PREVENT_VIRTUAL_HEAP_ALLOCATION */
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# define DISTRHO_PREVENT_VIRTUAL_HEAP_ALLOCATION \
private:                                         \
    static void* operator new(std::size_t) = delete;
#else
# define DISTRHO_PREVENT_VIRTUAL_HEAP_ALLOCATION \
private:                                         \
    static void* operator new(std::size_t);
#endif

/* Define namespace */
#ifndef DISTRHO_NAMESPACE
# define DISTRHO_NAMESPACE DISTRHO
#endif
#define START_NAMESPACE_DISTRHO namespace DISTRHO_NAMESPACE {
#define END_NAMESPACE_DISTRHO }
#define USE_NAMESPACE_DISTRHO using namespace DISTRHO_NAMESPACE;

/* Define DISTRHO_OS_SEP and DISTRHO_OS_SPLIT */
#ifdef DISTRHO_OS_WINDOWS
# define DISTRHO_OS_SEP       '\\'
# define DISTRHO_OS_SEP_STR   "\\"
# define DISTRHO_OS_SPLIT     ';'
# define DISTRHO_OS_SPLIT_STR ";"
#else
# define DISTRHO_OS_SEP       '/'
# define DISTRHO_OS_SEP_STR   "/"
# define DISTRHO_OS_SPLIT     ':'
# define DISTRHO_OS_SPLIT_STR ":"
#endif

/* MSVC warnings */
#ifdef _MSC_VER
# define strdup _strdup
# pragma warning(disable:4244) /* possible loss of data */
#endif

/* Useful macros */
#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))
#define STRINGIFY2(s) #s
#define STRINGIFY(s) STRINGIFY2(s)

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
#define CPP_AGGREGATE_INIT(ClassName) ClassName
#else
#define CPP_AGGREGATE_INIT(ClassName) (ClassName)
#endif

/* Useful typedefs */
typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef unsigned long int ulong;
typedef unsigned long long int ulonglong;

/* Deprecated macros */
#define DISTRHO_DECLARE_NON_COPY_CLASS(ClassName) DISTRHO_DECLARE_NON_COPYABLE(ClassName)
#define DISTRHO_DECLARE_NON_COPY_STRUCT(StructName) DISTRHO_DECLARE_NON_COPYABLE(StructName)
#define DISTRHO_MACRO_AS_STRING(MACRO) STRINGIFY2(MACRO)

#endif // DISTRHO_DEFINES_H_INCLUDED
