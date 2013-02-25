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

#include "../src/DistrhoDefines.h"

#if DISTRHO_OS_MAC
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

enum Char {
    CHAR_BACKSPACE = 0x08,
    CHAR_ESCAPE    = 0x1B,
    CHAR_DELETE    = 0x7F
};

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

enum Modifier {
    MODIFIER_SHIFT = 1 << 0, /**< Shift key */
    MODIFIER_CTRL  = 1 << 1, /**< Control key */
    MODIFIER_ALT   = 1 << 2, /**< Alt/Option key */
    MODIFIER_SUPER = 1 << 3  /**< Mod4/Command/Windows key */
};

#endif // __DGL_BASE_HPP__
