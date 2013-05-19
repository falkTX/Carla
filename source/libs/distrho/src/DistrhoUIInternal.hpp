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

#ifndef __DISTRHO_UI_INTERNAL_HPP__
#define __DISTRHO_UI_INTERNAL_HPP__

#include "DistrhoDefines.h"

#if defined(DISTRHO_UI_EXTERNAL)
# include "../DistrhoUI.hpp"
// TODO
#elif defined(DISTRHO_UI_OPENGL)
# include "../DistrhoUIOpenGL.hpp"
# include "../dgl/App.hpp"
# include "../dgl/Window.hpp"
#else
# include "../DistrhoUIQt.hpp"
#endif

#include <cassert>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

typedef void (*editParamFunc) (void* ptr, uint32_t index, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t index, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*uiResizeFunc)  (void* ptr, unsigned int width, unsigned int height);

extern double d_lastUiSampleRate;

// -------------------------------------------------

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
    }

    ~PrivateData()
    {
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

// -------------------------------------------------

class UIInternal
{
public:
    UIInternal(void* ptr, intptr_t winId, editParamFunc editParamCall, setParamFunc setParamCall, setStateFunc setStateCall, sendNoteFunc sendNoteCall, uiResizeFunc uiResizeCall)
#ifdef DISTRHO_UI_OPENGL
        : glApp(),
          glWindow(&glApp, winId),
#else
        :
#endif
          kUi(createUI()),
          kData((kUi != nullptr) ? kUi->pData : nullptr)
    {
        assert(kUi != nullptr);

        if (kUi == nullptr)
            return;

#ifdef DISTRHO_UI_OPENGL
        assert(winId != 0);

        if (winId == 0)
            return;
#else
        assert(winId == 0);

        if (winId != 0)
            return;
#endif

        kData->ptr = ptr;
        kData->editParamCallbackFunc = editParamCall;
        kData->setParamCallbackFunc  = setParamCall;
        kData->setStateCallbackFunc  = setStateCall;
        kData->sendNoteCallbackFunc  = sendNoteCall;
        kData->uiResizeCallbackFunc  = uiResizeCall;
    }

    ~UIInternal()
    {
        if (kUi != nullptr)
            delete kUi;
    }

    // ---------------------------------------------

    const char* name() const
    {
        assert(kUi != nullptr);
        return (kUi != nullptr) ? kUi->d_name() : "";
    }

    unsigned int width()
    {
        assert(kUi != nullptr);
        return (kUi != nullptr) ? kUi->d_width() : 0;
    }

    unsigned int height()
    {
        assert(kUi != nullptr);
        return (kUi != nullptr) ? kUi->d_height() : 0;
    }

    // ---------------------------------------------

    void parameterChanged(uint32_t index, float value)
    {
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programChanged(uint32_t index)
    {
      assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_programChanged(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value)
    {
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_stateChanged(key, value);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_noteReceived(onOff, channel, note, velocity);
    }
#endif

    // ---------------------------------------------

    void idle()
    {
#if defined(DISTRHO_UI_EXTERNAL)
        // TODO - idle OSC
#elif defined(DISTRHO_UI_OPENGL)
        glApp.idle();
#else
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_uiIdle();
#endif
    }

#if defined(DISTRHO_UI_EXTERNAL)
    // needed?
#elif defined(DISTRHO_UI_OPENGL)
    intptr_t getWinId()
    {
        return glWindow.getWindowId();
    }
#else
    QtUI* getQtUI() const
    {
        return (QtUI*)kUi;
    }

    bool resizable() const
    {
        return ((QtUI*)kUi)->d_resizable();
    }
#endif

    // ---------------------------------------------

#ifdef DISTRHO_UI_OPENGL
private:
    App    glApp;
    Window glWindow;
#endif

protected:
    UI* const kUi;
    UI::PrivateData* const kData;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_INTERNAL_HPP__
