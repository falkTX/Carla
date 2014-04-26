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

START_NAMESPACE_DGL
extern Window* dgl_lastUiParent;
END_NAMESPACE_DGL

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Static data, see DistrhoUIInternal.hpp

double d_lastUiSampleRate = 0.0;

// -----------------------------------------------------------------------
// UI

UI::UI()
    : DGL::Widget(*DGL::dgl_lastUiParent),
      pData(new PrivateData())
{
    DISTRHO_SAFE_ASSERT(DGL::dgl_lastUiParent != nullptr);

    DGL::dgl_lastUiParent = nullptr;
}

UI::~UI()
{
    delete pData;
}

// -----------------------------------------------------------------------
// Host DSP State

double UI::d_getSampleRate() const noexcept
{
    return pData->sampleRate;
}

void UI::d_editParameter(uint32_t index, bool started)
{
    pData->editParamCallback(index + pData->parameterOffset, started);
}

void UI::d_setParameterValue(uint32_t index, float value)
{
    pData->setParamCallback(index + pData->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::d_setState(const char* key, const char* value)
{
    pData->setStateCallback(key, value);
}
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
void UI::d_sendNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    pData->sendNoteCallback(channel, note, velocity);
}
#endif

// -----------------------------------------------------------------------
// Host UI State

void UI::d_uiResize(uint width, uint height)
{
    pData->uiResizeCallback(width, height);
}

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
// -----------------------------------------------------------------------
// Direct DSP access

void* UI::d_getPluginInstancePointer() const noexcept
{
    return pData->dspPtr;
}
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
