/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DGL_BASE_HPP__
#define __DGL_BASE_HPP__

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define DGL_OS_WINDOWS 1
#elif defined(__APPLE__)
# define DGL_OS_MAC     1
#elif defined(__HAIKU__)
# define DGL_OS_HAIKU   1
#elif defined(__linux__)
# define DGL_OS_LINUX   1
#endif

#define DGL_NAMESPACE DGL

#define START_NAMESPACE_DGL namespace DGL_NAMESPACE {
#define END_NAMESPACE_DGL }
#define USE_NAMESPACE_DGL using namespace DGL_NAMESPACE;

#if DGL_OS_WINDOWS
# include <windows.h>
#else
# include <unistd.h>
#endif

#if DGL_OS_MAC
# include <OpenGL/glu.h>
#else
# include <GL/glu.h>
#endif

#if defined(GL_BGR_EXT) && ! defined(GL_BGR)
# define GL_BGR GL_BGR_EXT
#endif

#if defined(GL_BGRA_EXT) && ! defined(GL_BGRA)
# define GL_BGRA GL_BGRA_EXT
#endif

// -------------------------------------------------

START_NAMESPACE_DGL

enum Char {
    DGL_CHAR_BACKSPACE = 0x08,
    DGL_CHAR_ESCAPE    = 0x1B,
    DGL_CHAR_DELETE    = 0x7F
};

enum Key {
    DGL_KEY_F1 = 1,
    DGL_KEY_F2,
    DGL_KEY_F3,
    DGL_KEY_F4,
    DGL_KEY_F5,
    DGL_KEY_F6,
    DGL_KEY_F7,
    DGL_KEY_F8,
    DGL_KEY_F9,
    DGL_KEY_F10,
    DGL_KEY_F11,
    DGL_KEY_F12,
    DGL_KEY_LEFT,
    DGL_KEY_UP,
    DGL_KEY_RIGHT,
    DGL_KEY_DOWN,
    DGL_KEY_PAGE_UP,
    DGL_KEY_PAGE_DOWN,
    DGL_KEY_HOME,
    DGL_KEY_END,
    DGL_KEY_INSERT,
    DGL_KEY_SHIFT,
    DGL_KEY_CTRL,
    DGL_KEY_ALT,
    DGL_KEY_SUPER
};

enum Modifier {
    DGL_MODIFIER_SHIFT = 1 << 0, /**< Shift key */
    DGL_MODIFIER_CTRL  = 1 << 1, /**< Control key */
    DGL_MODIFIER_ALT   = 1 << 2, /**< Alt/Option key */
    DGL_MODIFIER_SUPER = 1 << 3  /**< Mod4/Command/Windows key */
};

END_NAMESPACE_DGL

// -------------------------------------------------

static inline
void dgl_sleep(unsigned int secs)
{
#ifdef DGL_OS_WINDOWS
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

static inline
void dgl_msleep(unsigned int msecs)
{
#ifdef DGL_OS_WINDOWS
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

#endif // __DGL_BASE_HPP__
