/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_UI_HPP_INCLUDED
#define DISTRHO_UI_HPP_INCLUDED

#include "DistrhoUtils.hpp"

#include "dgl/Widget.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// UI

class UI : public DGL::Widget
{
public:
    UI();
    virtual ~UI();

    // -------------------------------------------------------------------
    // Host DSP State

    double d_getSampleRate() const noexcept;
    void   d_editParameter(uint32_t index, bool started);
    void   d_setParameterValue(uint32_t index, float value);
#if DISTRHO_PLUGIN_WANT_STATE
    void   d_setState(const char* key, const char* value);
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    void   d_sendNote(uint8_t channel, uint8_t note, uint8_t velocity);
#endif

    // -------------------------------------------------------------------
    // Host UI State

    void d_uiResize(unsigned int width, unsigned int height);

protected:
    // -------------------------------------------------------------------
    // Basic Information

    virtual const char*  d_getName()   const noexcept { return DISTRHO_PLUGIN_NAME; }
    virtual unsigned int d_getWidth()  const noexcept = 0;
    virtual unsigned int d_getHeight() const noexcept = 0;

    // -------------------------------------------------------------------
    // DSP Callbacks

    virtual void d_parameterChanged(uint32_t index, float value) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_programChanged(uint32_t index) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_stateChanged(const char* key, const char* value) = 0;
#endif

    // -------------------------------------------------------------------
    // UI Callbacks (optional)

    virtual void d_uiIdle() {}

    // -------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class UIExporter;
};

// -----------------------------------------------------------------------
// Create UI, entry point

extern UI* createUI();

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_HPP_INCLUDED
