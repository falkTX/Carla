/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifdef HAVE_DGL
# include "src/WidgetPrivateData.hpp"
#endif

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Static data, see DistrhoUIInternal.hpp */

double      d_lastUiSampleRate = 0.0;
void*       d_lastUiDspPtr     = nullptr;
#ifdef HAVE_DGL
Window*     d_lastUiWindow     = nullptr;
#endif
uintptr_t   g_nextWindowId     = 0;
const char* g_nextBundlePath   = nullptr;

/* ------------------------------------------------------------------------------------------------------------
 * UI */

#ifdef HAVE_DGL
UI::UI(uint width, uint height)
    : UIWidget(*d_lastUiWindow),
      pData(new PrivateData())
{
    ((UIWidget*)this)->pData->needsFullViewport = false;

    if (width > 0 && height > 0)
        setSize(width, height);
}
#else
UI::UI(uint width, uint height)
    : UIWidget(width, height),
      pData(new PrivateData()) {}
#endif

UI::~UI()
{
    delete pData;
}

/* ------------------------------------------------------------------------------------------------------------
 * Host state */

double UI::getSampleRate() const noexcept
{
    return pData->sampleRate;
}

void UI::editParameter(uint32_t index, bool started)
{
    pData->editParamCallback(index + pData->parameterOffset, started);
}

void UI::setParameterValue(uint32_t index, float value)
{
    pData->setParamCallback(index + pData->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::setState(const char* key, const char* value)
{
    pData->setStateCallback(key, value);
}
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
void UI::sendNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    pData->sendNoteCallback(channel, note, velocity);
}
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
/* ------------------------------------------------------------------------------------------------------------
 * Direct DSP access */

void* UI::getPluginInstancePointer() const noexcept
{
    return pData->dspPtr;
}
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
/* ------------------------------------------------------------------------------------------------------------
 * External UI helpers */

const char* UI::getNextBundlePath() noexcept
{
    return g_nextBundlePath;
}

# if DISTRHO_PLUGIN_HAS_EMBED_UI
uintptr_t UI::getNextWindowId() noexcept
{
    return g_nextWindowId;
}
# endif
#endif

/* ------------------------------------------------------------------------------------------------------------
 * DSP/Plugin Callbacks (optional) */

void UI::sampleRateChanged(double) {}

#ifdef HAVE_DGL
/* ------------------------------------------------------------------------------------------------------------
 * UI Callbacks (optional) */

void UI::uiFileBrowserSelected(const char*)
{
}

void UI::uiReshape(uint width, uint height)
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
#endif

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
