/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef __DISTRHO_UI_H__
#define __DISTRHO_UI_H__

#include "DistrhoUtils.h"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

struct UIPrivateData;

class UI
{
public:
    UI();
    virtual ~UI();

    // ---------------------------------------------

    // Host DSP State
    double d_sampleRate() const;
    void   d_setParameterValue(uint32_t index, float value);
#if DISTRHO_PLUGIN_WANT_STATE
    void   d_setState(const char* key, const char* value);
#endif

    // Host UI State
    void d_uiEditParameter(uint32_t index, bool started);
#if DISTRHO_PLUGIN_IS_SYNTH
    void d_uiSendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity);
#endif
    void d_uiResize(unsigned int width, unsigned int height);

    // ---------------------------------------------

protected:
    // Information
    virtual unsigned int d_width() = 0;
    virtual unsigned int d_height() = 0;

    // DSP Callbacks
    virtual void d_parameterChanged(uint32_t index, float value) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_programChanged(uint32_t index) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_stateChanged(const char* key, const char* value) = 0;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    virtual void d_uiNoteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity);
#endif

    // UI Callbacks
    virtual void d_uiIdle() = 0;

    // ---------------------------------------------

private:
    UIPrivateData* data;
    friend class UIInternal;
#ifdef DISTRHO_UI_QT4
    friend class Qt4UI;
#else
    friend class OpenGLUI;
    friend class OpenGLExtUI;
#endif
};

UI* createUI();

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_H__
