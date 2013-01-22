/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoDefines.h"

#ifdef DISTRHO_UI_OPENGL

#include "DistrhoUIInternal.h"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

OpenGLUI::OpenGLUI()
    : UI()
{
}

OpenGLUI::~OpenGLUI()
{
}

// -------------------------------------------------
// Host UI State

int OpenGLUI::d_uiGetModifiers()
{
    if (data && data->widget)
        return puglGetModifiers(data->widget);
    return 0;
}

void OpenGLUI::d_uiIgnoreKeyRepeat(bool ignore)
{
    if (data && data->widget)
        puglIgnoreKeyRepeat(data->widget, ignore);
}

void OpenGLUI::d_uiRepaint()
{
    if (data && data->widget)
        puglPostRedisplay(data->widget);
}

// -------------------------------------------------
// UI Callbacks

void OpenGLUI::d_uiIdle()
{
    if (data && data->widget)
        puglProcessEvents(data->widget);
}

// -------------------------------------------------

#if DISTRHO_OS_WINDOWS
# include "pugl/pugl_win.cpp"
#elif DISTRHO_OS_MAC
# include "pugl/pugl_osx.m"
#elif DISTRHO_OS_LINUX
# include "pugl/pugl_x11.c"
#else
# error Unsupported platform!
#endif

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_OPENGL
