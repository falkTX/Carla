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

#if defined(DISTRHO_UI_EXTERNAL)
# include "../DistrhoUIExternal.hpp"
#elif defined(DISTRHO_UI_OPENGL)
# include "../DistrhoUIOpenGL.hpp"
# include "../dgl/App.hpp"
# include "../dgl/Window.hpp"
#elif defined(DISTRHO_UI_QT)
# include "../DistrhoUIQt.hpp"
#else
# error Invalid UI type
#endif

#include <cassert>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

typedef void (*editParamFunc) (void* ptr, uint32_t index, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t index, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*uiResizeFunc)  (void* ptr, unsigned int width, unsigned int height);

extern double d_lastUiSampleRate;

// -----------------------------------------------------------------------

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

    PrivateData()
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
    }

    void editParamCallback(uint32_t index, bool started)
    {
        if (editParamCallbackFunc != nullptr)
            editParamCallbackFunc(ptr, index, started);
    }

    void setParamCallback(uint32_t rindex, float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(ptr, rindex, value);
    }

    void setStateCallback(const char* key, const char* value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(ptr, key, value);
    }

    void sendNoteCallback(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(ptr, onOff, channel, note, velocity);
    }

    void uiResizeCallback(unsigned int width, unsigned int height)
    {
        if (uiResizeCallbackFunc != nullptr)
            uiResizeCallbackFunc(ptr, width, height);
    }
};

// -----------------------------------------------------------------------

class UIInternal
{
public:
    UIInternal(void* ptr, intptr_t winId, editParamFunc editParamCall, setParamFunc setParamCall, setStateFunc setStateCall, sendNoteFunc sendNoteCall, uiResizeFunc uiResizeCall)
#ifdef DISTRHO_UI_OPENGL
        : glApp(),
          glWindow(&glApp, winId),
          fUi(createUI()),
#else
        : fUi(createUI()),
#endif
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

#ifndef DISTRHO_UI_OPENGL
        assert(winId == 0);
        return;

        // unused
        (void)winId;
#endif
    }

    ~UIInternal()
    {
        if (fUi != nullptr)
            delete fUi;
    }

    // -------------------------------------------------------------------

    const char* getName() const
    {
        assert(fUi != nullptr);
        return (fUi != nullptr) ? fUi->d_getName() : "";
    }

    unsigned int getWidth() const
    {
        assert(fUi != nullptr);
        return (fUi != nullptr) ? fUi->d_getWidth() : 0;
    }

    unsigned int getHeight() const
    {
        assert(fUi != nullptr);
        return (fUi != nullptr) ? fUi->d_getHeight() : 0;
    }

    // -------------------------------------------------------------------

    void parameterChanged(uint32_t index, float value)
    {
        assert(fUi != nullptr);

        if (fUi != nullptr)
            fUi->d_parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programChanged(uint32_t index)
    {
        assert(fUi != nullptr);

        if (fUi != nullptr)
            fUi->d_programChanged(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value)
    {
        assert(fUi != nullptr);

        if (fUi != nullptr)
            fUi->d_stateChanged(key, value);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        assert(fUi != nullptr);

        if (fUi != nullptr)
            fUi->d_noteReceived(onOff, channel, note, velocity);
    }
#endif

    // -------------------------------------------------------------------

    void idle()
    {
        assert(fUi != nullptr);

        if (fUi != nullptr)
            fUi->d_uiIdle();

#ifdef DISTRHO_UI_OPENGL
        glApp.idle();
#endif
    }

#if defined(DISTRHO_UI_EXTERNAL)
    const char* getExternalFilename() const
    {
        return ((ExternalUI*)fUi)->d_getExternalFilename();
    }
#elif defined(DISTRHO_UI_OPENGL)
    DGL::App& getApp()
    {
        return glApp;
    }

    DGL::Window& getWindow()
    {
        return glWindow;
    }

    /*intptr_t getWindowId() const
    {
        return glWindow.getWindowId();
    }*/

    /*void fixWindowSize()
    {
        assert(fUi != nullptr);
        glWindow.setSize(fUi->d_getWidth(), fUi->d_getHeight());
    }*/
#elif defined(DISTRHO_UI_QT)
    /*QtUI* getQtUI() const
    {
        return (QtUI*)fUi;
    }*/

    bool isResizable() const
    {
        return ((QtUI*)fUi)->d_isResizable();
    }
#endif

    // -------------------------------------------------------------------

#ifdef DISTRHO_UI_OPENGL
private:
    DGL::App    glApp;
    DGL::Window glWindow;
#endif

protected:
    UI* const fUi;
    UI::PrivateData* const fData;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_INTERNAL_HPP_INCLUDED
