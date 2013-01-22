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

#ifndef __DISTRHO_UI_QT4_H__
#define __DISTRHO_UI_QT4_H__

#include "src/DistrhoDefines.h"

#ifdef DISTRHO_UI_QT4

#include "DistrhoUI.h"

#include <QtGui/QWidget>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class Qt4UI : public UI,
              public QWidget
{
public:
    Qt4UI();
    virtual ~Qt4UI();

    // ---------------------------------------------

protected:
    // Information
    virtual bool d_resizable() { return false; }
    virtual int  d_minimumWidth() { return 100; }
    virtual int  d_minimumHeight() { return 100; }

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

    // Implement resize internally
    unsigned int d_width() { return width(); }
    unsigned int d_height() { return height(); }

private:
    friend class UIInternal;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_QT4

#endif // __DISTRHO_UI_QT4_H__
