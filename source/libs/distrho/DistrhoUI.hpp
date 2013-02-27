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

#ifndef __DISTRHO_UI_HPP__
#define __DISTRHO_UI_HPP__

#include "DistrhoUtils.hpp"

#include <cstdint>

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// UI

struct UIPrivateData;

class UI
{
public:
    UI();
    virtual ~UI();

    // ---------------------------------------------
    // Host DSP State

    double d_sampleRate() const;
    void   d_editParameter(uint32_t index, bool started);
    void   d_setParameterValue(uint32_t index, float value);
#if DISTRHO_PLUGIN_WANT_STATE
    void   d_setState(const char* key, const char* value);
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    void   d_sendNote(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity);
#endif

    // ---------------------------------------------
    // Host UI State

    void d_uiResize(unsigned int width, unsigned int height);

protected:
    // ---------------------------------------------
    // Information

    virtual const char*  d_name() const { return DISTRHO_PLUGIN_NAME; }
    virtual unsigned int d_width() const = 0;
    virtual unsigned int d_height() const = 0;

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

    virtual void d_uiIdle() = 0;

    // ---------------------------------------------

private:
    UIPrivateData* const pData;
    friend class UIInternal;
};

// -------------------------------------------------

UI* createUI();

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_HPP__
