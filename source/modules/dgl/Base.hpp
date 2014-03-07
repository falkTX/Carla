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

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define DGL_OS_WINDOWS 1
#elif defined(__APPLE__)
# define DGL_OS_MAC     1
#elif defined(__HAIKU__)
# define DGL_OS_HAIKU   1
#elif defined(__linux__)
# define DGL_OS_LINUX   1
#endif

#if defined(HAVE_CPP11_SUPPORT)
# define PROPER_CPP11_SUPPORT
#elif defined(__GNUC__) && (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
# if  (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
#  define PROPER_CPP11_SUPPORT
#  if  (__GNUC__ * 100 + __GNUC_MINOR__) < 407
#   define override // gcc4.7+ only
#  endif
# endif
#endif

#ifndef PROPER_CPP11_SUPPORT
# ifndef __clang__
#  define noexcept throw()
# endif
# define override
# define nullptr (0)
#endif

#ifndef DGL_NAMESPACE
# define DGL_NAMESPACE DGL
#endif

#define START_NAMESPACE_DGL namespace DGL_NAMESPACE {
#define END_NAMESPACE_DGL }
#define USE_NAMESPACE_DGL using namespace DGL_NAMESPACE;

#ifdef DGL_OS_MAC
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif

#if defined(GL_BGR_EXT) && ! defined(GL_BGR)
# define GL_BGR GL_BGR_EXT
#endif

#if defined(GL_BGRA_EXT) && ! defined(GL_BGRA)
# define GL_BGRA GL_BGRA_EXT
#endif

#ifndef GL_CLAMP_TO_BORDER
# define GL_CLAMP_TO_BORDER 0x812D
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/*
 * Convenience symbols for ASCII control characters.
 */
enum Char {
    CHAR_BACKSPACE = 0x08,
    CHAR_ESCAPE    = 0x1B,
    CHAR_DELETE    = 0x7F
};

/*
 * Special (non-Unicode) keyboard keys.
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

/*
 * Keyboard modifier flags.
 */
enum Modifier {
    MODIFIER_SHIFT = 1 << 0, /**< Shift key */
    MODIFIER_CTRL  = 1 << 1, /**< Control key */
    MODIFIER_ALT   = 1 << 2, /**< Alt/Option key */
    MODIFIER_SUPER = 1 << 3  /**< Mod4/Command/Windows key */
};

/*
 * Cross-platform sleep function.
 */
void sleep(unsigned int secs);

/*
 * Cross-platform msleep function.
 */
void msleep(unsigned int msecs);

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_BASE_HPP_INCLUDED
