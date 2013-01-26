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

#ifndef __DISTRHO_UI_OPENGL_H__
#define __DISTRHO_UI_OPENGL_H__

#include "src/DistrhoMacros.h"

#ifdef DISTRHO_UI_OPENGL

#include "DistrhoUI.h"

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

START_NAMESPACE_DISTRHO

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

// -------------------------------------------------

class OpenGLUI : public UI
{
public:
    OpenGLUI();
    virtual ~OpenGLUI();

    // ---------------------------------------------

    // Host UI State (OpenGL)
    int  d_uiGetModifiers();
    void d_uiIgnoreKeyRepeat(bool ignore);
    void d_uiRepaint();

    // ---------------------------------------------

protected:
    // Information
    virtual int d_width() = 0;
    virtual int d_height() = 0;

    // DSP Callbacks
    virtual void d_parameterChanged(uint32_t index, float value) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_programChanged(uint32_t index) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_stateChanged(const char* key, const char* value) = 0;
#endif

    // UI Callbacks
    virtual void d_uiIdle();
    virtual void d_onInit() = 0;
    virtual void d_onDisplay() = 0;
    virtual void d_onKeyboard(bool press, uint32_t key) = 0;
    virtual void d_onMotion(int x, int y) = 0;
    virtual void d_onMouse(int button, bool press, int x, int y) = 0;
    virtual void d_onReshape(int width, int height) = 0;
    virtual void d_onScroll(float dx, float dy) = 0;
    virtual void d_onSpecial(bool press, Key key) = 0;
    virtual void d_onClose() = 0;

private:
    friend class UIInternal;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_OPENGL

#endif // __DISTRHO_UI_OPENGL_H__
