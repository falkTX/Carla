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

#ifndef DISTRHO_UI_INTERNAL_HPP_INCLUDED
#define DISTRHO_UI_INTERNAL_HPP_INCLUDED

#include "../DistrhoUI.hpp"

#include "../dgl/App.hpp"
#include "../dgl/Window.hpp"
#include <lo/lo_osc_types.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Static data, see DistrhoUI.cpp

extern double d_lastUiSampleRate;

// -----------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc) (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*uiResizeFunc)  (void* ptr, unsigned int width, unsigned int height);

// -----------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DSP
    double   sampleRate;
    uint32_t parameterOffset;

    // Callbacks
    editParamFunc editParamCallbackFunc;
    setParamFunc  setParamCallbackFunc;
    setStateFunc  setStateCallbackFunc;
    sendNoteFunc  sendNoteCallbackFunc;
    uiResizeFunc  uiResizeCallbackFunc;
    void*         ptr;

    PrivateData() noexcept
        : sampleRate(d_lastUiSampleRate),
          parameterOffset(0),
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          uiResizeCallbackFunc(nullptr),
          ptr(nullptr)
    {
        assert(sampleRate != 0.0);

#if defined(DISTRHO_PLUGIN_TARGET_DSSI) || defined(DISTRHO_PLUGIN_TARGET_LV2)
        parameterOffset += DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;
# if DISTRHO_PLUGIN_WANT_LATENCY
        parameterOffset += 1;
# endif
#endif
#ifdef DISTRHO_PLUGIN_TARGET_LV2
# if (DISTRHO_PLUGIN_IS_SYNTH || DISTRHO_PLUGIN_WANT_TIMEPOS || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
#  if DISTRHO_PLUGIN_WANT_STATE
        parameterOffset += 1;
#  endif
# endif
#endif
    }

    void editParamCallback(const uint32_t rindex, const bool started)
    {
        if (editParamCallbackFunc != nullptr)
            editParamCallbackFunc(ptr, rindex, started);
    }

    void setParamCallback(const uint32_t rindex, const float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(ptr, rindex, value);
    }

    void setStateCallback(const char* const key, const char* const value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(ptr, key, value);
    }

    void sendNoteCallback(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(ptr, channel, note, velocity);
    }

    void uiResizeCallback(const unsigned int width, const unsigned int height)
    {
        if (uiResizeCallbackFunc != nullptr)
            uiResizeCallbackFunc(ptr, width, height);
    }
};

// -----------------------------------------------------------------------
// UI exporter class

class UIExporter
{
public:
    UIExporter(void* const ptr, const intptr_t winId,
               const editParamFunc editParamCall, const setParamFunc setParamCall, const setStateFunc setStateCall, const sendNoteFunc sendNoteCall, const uiResizeFunc uiResizeCall)
        : glApp(),
          glWindow(glApp, winId),
          fUi(createUI()),
          fData((fUi != nullptr) ? fUi->pData : nullptr)
    {
        assert(fUi != nullptr);

        if (fUi == nullptr)
            return;

        fData->ptr = ptr;
        fData->editParamCallbackFunc = editParamCall;
        fData->setParamCallbackFunc  = setParamCall;
        fData->setStateCallbackFunc  = setStateCall;
        fData->sendNoteCallbackFunc  = sendNoteCall;
        fData->uiResizeCallbackFunc  = uiResizeCall;

        glWindow.setSize(fUi->d_getWidth(), fUi->d_getHeight());
        glWindow.setResizable(false);
    }

    ~UIExporter()
    {
        delete fUi;
    }

    // -------------------------------------------------------------------

    const char* getName() const noexcept
    {
        return (fUi != nullptr) ? fUi->d_getName() : "";
    }

    unsigned int getWidth() const noexcept
    {
        return (fUi != nullptr) ? fUi->d_getWidth() : 0;
    }

    unsigned int getHeight() const noexcept
    {
        return (fUi != nullptr) ? fUi->d_getHeight() : 0;
    }

    // -------------------------------------------------------------------

    uint32_t getParameterOffset() const noexcept
    {
        return (fData != nullptr) ? fData->parameterOffset : 0;
    }

    // -------------------------------------------------------------------

    void parameterChanged(const uint32_t index, const float value)
    {
        if (fUi != nullptr)
            fUi->d_parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programChanged(const uint32_t index)
    {
        if (fUi != nullptr)
            fUi->d_programChanged(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* const key, const char* const value)
    {
        if (fUi != nullptr)
            fUi->d_stateChanged(key, value);
    }
#endif

    // -------------------------------------------------------------------

    bool idle()
    {
        if (fUi != nullptr)
            fUi->d_uiIdle();

        glApp.idle();

        return ! glApp.isQuiting();
    }

    void quit()
    {
        glWindow.close();
        glApp.quit();
    }

    void setSize(const unsigned int width, const unsigned int height)
    {
        glWindow.setSize(width, height);
    }

    void setTitle(const char* const uiTitle)
    {
        glWindow.setTitle(uiTitle);
    }

    void setVisible(const bool yesNo)
    {
        glWindow.setVisible(yesNo);
    }

private:
    // -------------------------------------------------------------------
    // DGL Application and Window for this plugin

    DGL::App    glApp;
    DGL::Window glWindow;

    // -------------------------------------------------------------------
    // private members accessed by DistrhoPlugin class

    UI* const fUi;
    UI::PrivateData* const fData;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_INTERNAL_HPP_INCLUDED
