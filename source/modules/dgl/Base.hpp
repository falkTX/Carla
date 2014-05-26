/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "../distrho/extra/d_leakdetector.hpp"
#include "../distrho/extra/d_scopedpointer.hpp"

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
# include "OpenGL/gl.h"
#else
# define GL_GLEXT_PROTOTYPES
# include "GL/gl.h"
# include "GL/glext.h"
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
    CHAR_BACKSPACE = 0x08,
    CHAR_ESCAPE    = 0x1B,
    CHAR_DELETE    = 0x7F
};

/**
   Special (non-Unicode) keyboard keys.
 */
enum Key {
    KEY_F1 = 1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_LEFT,
    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_INSERT,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_ALT,
    KEY_SUPER
};

/**
   Keyboard modifier flags.
 */
enum Modifier {
    MODIFIER_SHIFT = 1 << 0, /**< Shift key */
    MODIFIER_CTRL  = 1 << 1, /**< Control key */
    MODIFIER_ALT   = 1 << 2, /**< Alt/Option key */
    MODIFIER_SUPER = 1 << 3  /**< Mod4/Command/Windows key */
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

#endif // DGL_BASE_HPP_INCLUDED
