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

#ifndef __DISTRHO_UI_QT4_HPP__
#define __DISTRHO_UI_QT4_HPP__

#include "DistrhoUI.hpp"

#include <QtGui/QWidget>

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Qt4 UI

class Qt4UI : public UI,
              public QWidget
{
public:
    Qt4UI();
    virtual ~Qt4UI();

protected:
    // ---------------------------------------------
    // Information

    virtual bool d_resizable() { return false; }
    virtual uint d_minimumWidth() { return 100; }
    virtual uint d_minimumHeight() { return 100; }

    // ---------------------------------------------
    // DSP Callbacks

    virtual void d_parameterChanged(uint32_t index, float value) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_programChanged(uint32_t index) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_stateChanged(const char* key, const char* value) = 0;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    virtual void d_noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity) = 0;
#endif

    // ---------------------------------------------
    // UI Callbacks

    virtual void d_uiIdle();

private:
    friend class UIInternal;

    // ---------------------------------------------
    // Implement resize internally

    void         d_uiResize(unsigned int width, unsigned int height);
    unsigned int d_width() const { return width(); }
    unsigned int d_height() const { return height(); }
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_QT4_HPP__
