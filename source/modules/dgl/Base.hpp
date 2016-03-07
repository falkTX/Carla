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

#ifndef DGL_BASE_HPP_INCLUDED
#define DGL_BASE_HPP_INCLUDED

#include "../distrho/extra/LeakDetector.hpp"
#include "../distrho/extra/ScopedPointer.hpp"

// -----------------------------------------------------------------------
// Define namespace

#ifndef DGL_NAMESPACE
# define DGL_NAMESPACE DGL
#endif

#define START_NAMESPACE_DGL namespace DGL_NAMESPACE {
#define END_NAMESPACE_DGL }
#define USE_NAMESPACE_DGL using namespace DGL_NAMESPACE;

#ifdef DISTRHO_OS_WINDOWS
// -----------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code

#ifndef APIENTRY
# define APIENTRY __stdcall
#endif // APIENTRY

/* We need WINGDIAPI defined */
#ifndef WINGDIAPI
# if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__POCC__)
#  define WINGDIAPI __declspec(dllimport)
# elif defined(__LCC__)
#  define WINGDIAPI __stdcall
# else
#  define WINGDIAPI extern
# endif
# define DGL_WINGDIAPI_DEFINED
#endif // WINGDIAPI

/* Some <GL/glu.h> files also need CALLBACK defined */
#ifndef CALLBACK
# if defined(_MSC_VER)
#  if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#   define CALLBACK __stdcall
#  else
#   define CALLBACK
#  endif
# else
#  define CALLBACK __stdcall
# endif
# define DGL_CALLBACK_DEFINED
#endif // CALLBACK

/* Most GL/glu.h variants on Windows need wchar_t */
#include <cstddef>

#endif // DISTRHO_OS_WINDOWS

// -----------------------------------------------------------------------
// OpenGL includes

#ifdef DISTRHO_OS_MAC
# include <OpenGL/gl.h>
#else
# ifndef DISTRHO_OS_WINDOWS
#  define GL_GLEXT_PROTOTYPES
# endif
# include <GL/gl.h>
# include <GL/glext.h>
#endif

// -----------------------------------------------------------------------
// Missing OpenGL defines

#if defined(GL_BGR_EXT) && ! defined(GL_BGR)
# define GL_BGR GL_BGR_EXT
#endif

#if defined(GL_BGRA_EXT) && ! defined(GL_BGRA)
# define GL_BGRA GL_BGRA_EXT
#endif

#ifndef GL_CLAMP_TO_BORDER
# define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifdef DISTRHO_OS_WINDOWS
// -----------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code

#ifdef DGL_WINGDIAPI_DEFINED
# undef WINGDIAPI
# undef DGL_WINGDIAPI_DEFINED
#endif

#ifdef DGL_CALLBACK_DEFINED
# undef CALLBACK
# undef DGL_CALLBACK_DEFINED
#endif

#endif // DISTRHO_OS_WINDOWS

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Base DGL enums

/**
   Convenience symbols for ASCII control characters.
 */
enum Char {
    kCharBackspace = 0x08,
    kCharEscape    = 0x1B,
    kCharDelete    = 0x7F
};

/**
   Keyboard modifier flags.
 */
enum Modifier {
    kModifierShift   = 1 << 0, /**< Shift key */
    kModifierControl = 1 << 1, /**< Control key */
    kModifierAlt     = 1 << 2, /**< Alt/Option key */
    kModifierSuper   = 1 << 3  /**< Mod4/Command/Windows key */
};

/**
   Special (non-Unicode) keyboard keys.
 */
enum Key {
    kKeyF1 = 1,
    kKeyF2,
    kKeyF3,
    kKeyF4,
    kKeyF5,
    kKeyF6,
    kKeyF7,
    kKeyF8,
    kKeyF9,
    kKeyF10,
    kKeyF11,
    kKeyF12,
    kKeyLeft,
    kKeyUp,
    kKeyRight,
    kKeyDown,
    kKeyPageUp,
    kKeyPageDown,
    kKeyHome,
    kKeyEnd,
    kKeyInsert,
    kKeyShift,
    kKeyControl,
    kKeyAlt,
    kKeySuper
};

// -----------------------------------------------------------------------
// Base DGL classes

/**
   Idle callback.
 */
class IdleCallback
{
public:
    virtual ~IdleCallback() {}
    virtual void idleCallback() = 0;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#ifndef DONT_SET_USING_DGL_NAMESPACE
  // If your code uses a lot of DGL classes, then this will obviously save you
  // a lot of typing, but can be disabled by setting DONT_SET_USING_DGL_NAMESPACE.
  using namespace DGL_NAMESPACE;
#endif

// -----------------------------------------------------------------------

#endif // DGL_BASE_HPP_INCLUDED
