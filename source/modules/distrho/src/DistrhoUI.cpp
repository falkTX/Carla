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

#include "DistrhoUIInternal.hpp"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Static data, see DistrhoUIInternal.hpp */

double  d_lastUiSampleRate = 0.0;
void*   d_lastUiDspPtr = nullptr;
Window* d_lastUiWindow = nullptr;

/* ------------------------------------------------------------------------------------------------------------
 * UI */

UI::UI()
    : UIWidget(*d_lastUiWindow),
      pData(new PrivateData())
{
    UIWidget::setNeedsFullViewport(true);
}

UI::~UI()
{
    delete pData;
}

/* ------------------------------------------------------------------------------------------------------------
 * Host state */

double UI::d_getSampleRate() const noexcept
{
    return pData->sampleRate;
}

void UI::d_editParameter(const uint32_t index, const bool started)
{
    pData->editParamCallback(index + pData->parameterOffset, started);
}

void UI::d_setParameterValue(const uint32_t index, const float value)
{
    pData->setParamCallback(index + pData->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::d_setState(const char* const key, const char* const value)
{
    pData->setStateCallback(key, value);
}
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
void UI::d_sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
{
    pData->sendNoteCallback(channel, note, velocity);
}
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
/* ------------------------------------------------------------------------------------------------------------
 * Direct DSP access */

void* UI::d_getPluginInstancePointer() const noexcept
{
    return pData->dspPtr;
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * DSP/Plugin Callbacks (optional) */

void UI::d_sampleRateChanged(double) {}

/* ------------------------------------------------------------------------------------------------------------
 * UI Callbacks (optional) */

void UI::d_uiFileBrowserSelected(const char*)
{
}

void UI::d_uiReshape(uint width, uint height)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(width), static_cast<GLdouble>(height), 0.0, 0.0, 1.0);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* ------------------------------------------------------------------------------------------------------------
 * UI Resize Handling, internal */

void UI::onResize(const ResizeEvent& ev)
{
    pData->setSizeCallback(ev.size.getWidth(), ev.size.getHeight());
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
