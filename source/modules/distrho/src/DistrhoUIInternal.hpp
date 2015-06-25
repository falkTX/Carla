/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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
#include "../../dgl/Application.hpp"
#include "../../dgl/Window.hpp"

using DGL::Application;
using DGL::IdleCallback;
using DGL::Window;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Static data, see DistrhoUI.cpp

extern double  d_lastUiSampleRate;
extern void*   d_lastUiDspPtr;
extern Window* d_lastUiWindow;

// -----------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc) (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*setSizeFunc)   (void* ptr, uint width, uint height);

// -----------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DSP
    double   sampleRate;
    uint32_t parameterOffset;
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    void*    dspPtr;
#endif

    // Callbacks
    editParamFunc editParamCallbackFunc;
    setParamFunc  setParamCallbackFunc;
    setStateFunc  setStateCallbackFunc;
    sendNoteFunc  sendNoteCallbackFunc;
    setSizeFunc   setSizeCallbackFunc;
    void*         ptr;

    PrivateData() noexcept
        : sampleRate(d_lastUiSampleRate),
          parameterOffset(0),
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
          dspPtr(d_lastUiDspPtr),
#endif
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          setSizeCallbackFunc(nullptr),
          ptr(nullptr)
    {
        DISTRHO_SAFE_ASSERT(d_isNotZero(sampleRate));

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

    void setSizeCallback(const uint width, const uint height)
    {
        if (setSizeCallbackFunc != nullptr)
            setSizeCallbackFunc(ptr, width, height);
    }
};

// -----------------------------------------------------------------------
// Plugin Window, needed to take care of resize properly

static inline
UI* createUiWrapper(void* const dspPtr, Window* const window)
{
    d_lastUiDspPtr = dspPtr;
    d_lastUiWindow = window;
    UI* const ret  = createUI();
    d_lastUiDspPtr = nullptr;
    d_lastUiWindow = nullptr;
    return ret;
}

class UIExporterWindow : public Window
{
public:
    UIExporterWindow(Application& app, const intptr_t winId, void* const dspPtr)
        : Window(app, winId),
          fUI(createUiWrapper(dspPtr, this)),
          fIsReady(false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        // set window size
        setResizable(false);
        setSize(fUI->getWidth(), fUI->getHeight());
    }

    ~UIExporterWindow()
    {
        delete fUI;
    }

    UI* getUI() const noexcept
    {
        return fUI;
    }

    bool isReady() const noexcept
    {
        return fIsReady;
    }

protected:
    // custom window reshape
    void onReshape(uint width, uint height) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->uiReshape(width, height);
        fIsReady = true;
    }

    // custom file-browser selected
    void fileBrowserSelected(const char* filename) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->uiFileBrowserSelected(filename);
    }

private:
    UI* const fUI;
    bool fIsReady;
};

// -----------------------------------------------------------------------
// UI exporter class

class UIExporter
{
public:
    UIExporter(void* const ptr, const intptr_t winId,
               const editParamFunc editParamCall, const setParamFunc setParamCall, const setStateFunc setStateCall, const sendNoteFunc sendNoteCall, const setSizeFunc setSizeCall,
               void* const dspPtr = nullptr)
        : glApp(),
          glWindow(glApp, winId, dspPtr),
          fChangingSize(false),
          fUI(glWindow.getUI()),
          fData((fUI != nullptr) ? fUI->pData : nullptr)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        fData->ptr = ptr;
        fData->editParamCallbackFunc = editParamCall;
        fData->setParamCallbackFunc  = setParamCall;
        fData->setStateCallbackFunc  = setStateCall;
        fData->sendNoteCallbackFunc  = sendNoteCall;
        fData->setSizeCallbackFunc   = setSizeCall;
    }

    // -------------------------------------------------------------------

    uint getWidth() const noexcept
    {
        return glWindow.getWidth();
    }

    uint getHeight() const noexcept
    {
        return glWindow.getHeight();
    }

    bool isVisible() const noexcept
    {
        return glWindow.isVisible();
    }

    // -------------------------------------------------------------------

    intptr_t getWindowId() const noexcept
    {
        return glWindow.getWindowId();
    }

    // -------------------------------------------------------------------

    uint32_t getParameterOffset() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->parameterOffset;
    }

    // -------------------------------------------------------------------

    void parameterChanged(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(const uint32_t index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->programLoaded(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

        fUI->stateChanged(key, value);
    }
#endif

    // -------------------------------------------------------------------

    void exec(IdleCallback* const cb)
    {
        DISTRHO_SAFE_ASSERT_RETURN(cb != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        glWindow.addIdleCallback(cb);
        glWindow.setVisible(true);
        glApp.exec();
    }

    void exec_idle()
    {
        if (glWindow.isReady())
            fUI->uiIdle();
    }

    bool idle()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, false);

        glApp.idle();

        if (glWindow.isReady())
            fUI->uiIdle();

        return ! glApp.isQuiting();
    }

    void quit()
    {
        glWindow.close();
        glApp.quit();
    }

    // -------------------------------------------------------------------

    void setWindowSize(const uint width, const uint height, const bool updateUI = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(! fChangingSize,);

        fChangingSize = true;

        if (updateUI)
            fUI->setSize(width, height);

        glWindow.setSize(width, height);

        fChangingSize = false;
    }

    void setWindowTitle(const char* const uiTitle)
    {
        glWindow.setTitle(uiTitle);
    }

    void setWindowTransientWinId(const uintptr_t winId)
    {
        glWindow.setTransientWinId(winId);
    }

    bool setWindowVisible(const bool yesNo)
    {
        glWindow.setVisible(yesNo);

        return ! glApp.isQuiting();
    }

    // -------------------------------------------------------------------

    void setSampleRate(const double sampleRate, const bool doCallback = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT(sampleRate > 0.0);

        if (d_isEqual(fData->sampleRate, sampleRate))
            return;

        fData->sampleRate = sampleRate;

        if (doCallback)
            fUI->sampleRateChanged(sampleRate);
    }

private:
    // -------------------------------------------------------------------
    // DGL Application and Window for this widget

    Application      glApp;
    UIExporterWindow glWindow;

    // prevent recursion
    bool fChangingSize;

    // -------------------------------------------------------------------
    // Widget and DistrhoUI data

    UI* const fUI;
    UI::PrivateData* const fData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIExporter)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_INTERNAL_HPP_INCLUDED
