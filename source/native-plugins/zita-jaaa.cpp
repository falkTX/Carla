/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaNative.hpp"

#include "CarlaMutex.hpp"
#include "CarlaJuceUtils.hpp"

#include "juce_audio_basics.h"

#include "zita-jaaa/source/audio.cc"
#include "zita-jaaa/source/rngen.cc"
#include "zita-jaaa/source/spectwin.cc"
#include "zita-jaaa/source/styles.cc"

using juce::FloatVectorOperations;
using juce::ScopedPointer;

// -----------------------------------------------------------------------
// fake jack_client_open/close per plugin

static jack_client_t* gLastJackClient = nullptr;

jack_client_t* jack_client_open(const char*, jack_options_t, jack_status_t* status, ...)
{
    if (status != NULL)
        *status = JackNoError;

    return gLastJackClient;
}

int jack_client_close(jack_client_t* client)
{
    carla_zeroStruct(client, 1);
    return 0;
}

// -----------------------------------------------------------------------
// Jaaa Plugin

class JaaaPlugin : public NativePluginClass
{
public:
    enum Parameters {
        kParameterInput = 0,
        kParameterCount
    };

    JaaaPlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fJackClient(),
          fMutex(),
          xrm(),
          itcc(nullptr),
          driver(nullptr),
          display(nullptr),
          rootwin(nullptr),
          mainwin(nullptr),
          xhandler(nullptr),
          leakDetector_JaaaPlugin()
    {
        CARLA_SAFE_ASSERT(host != nullptr);

        int   argc   = 1;
        char* argv[] = { (char*)"jaaa" };
        xrm.init(&argc, argv, (char*)"jaaa", nullptr, 0);

        fParameters[kParameterInput] = 1.0f;
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParameterCount;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterCount, nullptr);

        static NativeParameter param;

        int hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMABLE;

        // reset
        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 0.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 1.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case kParameterInput:
            hints |= NATIVE_PARAMETER_IS_INTEGER;
            param.name = "Input";
            param.ranges.def = 1.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 8.0f;
            break;
        }

        param.hints = static_cast<NativeParameterHints>(hints);

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterCount, 0.0f);

        return fParameters[index];
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterCount,);

        fParameters[index] = value;
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        const CarlaMutexTryLocker cmtl(fMutex);

        if (itcc == nullptr || ! fJackClient.active || ! cmtl.wasLocked())
        {
            const int iframes(static_cast<int>(frames));

            for (int i=0; i<8; ++i)
                FloatVectorOperations::clear(outBuffer[i], iframes);

            return;
        }

        for (int i=0; i<8; ++i)
            fJackClient.portsAudioIn[i]->buffer = inBuffer[i];

        for (int i=0; i<8; ++i)
            fJackClient.portsAudioOut[i]->buffer = outBuffer[i];

        fJackClient.processCallback(frames, fJackClient.processPtr);
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        const CarlaMutexLocker cml(fMutex);

        if (show)
        {
            if (itcc == nullptr)
            {
                carla_zeroStruct(fJackClient);
                gLastJackClient = &fJackClient;

                fJackClient.clientName = getUiName();
                fJackClient.bufferSize = getBufferSize();
                fJackClient.sampleRate = getSampleRate();

                itcc    = new ITC_ctrl();
                driver  = new Audio(itcc, getUiName());
                driver->init_jack(nullptr);

                display = new X_display(nullptr);

                if (display->dpy() == nullptr)
                {
                    driver = nullptr;
                    itcc   = nullptr;
                    return hostUiUnavailable();
                }

                init_styles(display, &xrm);

                rootwin  = new X_rootwin(display);
                mainwin  = new Spectwin(rootwin, &xrm, driver);
                xhandler = new X_handler(display, itcc, EV_X11);

                if (const uintptr_t winId = getUiParentId())
                    XSetTransientForHint(display->dpy(), mainwin->win(), static_cast<Window>(winId));
            }

            xhandler->next_event();
            XFlush(display->dpy());
        }
        else
        {
            xhandler = nullptr;
            mainwin  = nullptr;
            rootwin  = nullptr;
            display  = nullptr;
            driver   = nullptr;
            itcc     = nullptr;
            carla_zeroStruct(fJackClient);
        }
    }

    void uiIdle() override
    {
        if (itcc == nullptr)
            return;

        //for (int i=3; --i>=0;)
        {
            switch (itcc->get_event())
            {
            case EV_TRIG:
                mainwin->handle_trig();
                rootwin->handle_event();
                XFlush(display->dpy());
                break;

            case EV_MESG:
                mainwin->handle_mesg(itcc->get_message());
                rootwin->handle_event();
                XFlush(display->dpy());
                break;

            case EV_X11:
                rootwin->handle_event();
                xhandler->next_event();
                break;
            }
        }

        // check if parameters were updated
        if (mainwin->_input+1 != static_cast<int>(fParameters[kParameterInput]))
        {
            fParameters[kParameterInput] = mainwin->_input+1;
            uiParameterChanged(kParameterInput, fParameters[kParameterInput]);
        }

        // check if UI was closed
        if (! mainwin->running())
        {
            {
                const CarlaMutexLocker cml(fMutex);
                xhandler = nullptr;
                mainwin  = nullptr;
                rootwin  = nullptr;
                display  = nullptr;
                driver   = nullptr;
                itcc     = nullptr;
                carla_zeroStruct(fJackClient);
            }
            uiClosed();
            return;
        }
    }

    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterCount,);

        const CarlaMutexLocker cml(fMutex);

        if (itcc == nullptr)
            return;

        switch (index)
        {
        case kParameterInput:
            mainwin->set_input(static_cast<int>(value)-1);
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        fJackClient.bufferSize = bufferSize;
    }

    void sampleRateChanged(const double sampleRate) override
    {
        fJackClient.sampleRate = sampleRate;
    }

    // -------------------------------------------------------------------

private:
    // Fake jack client
    jack_client_t fJackClient;

    // Mutex just in case
    CarlaMutex fMutex;

    // Zita stuff (core)
    X_resman xrm;
    ScopedPointer<ITC_ctrl>  itcc;
    ScopedPointer<Audio>     driver;
    ScopedPointer<X_display> display;
    ScopedPointer<X_rootwin> rootwin;
    ScopedPointer<Spectwin>  mainwin;
    ScopedPointer<X_handler> xhandler;

    float fParameters[kParameterCount];

    PluginClassEND(JaaaPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JaaaPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor jaaaDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ 8,
    /* audioOuts */ 8,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ JaaaPlugin::kParameterCount,
    /* paramOuts */ 0,
    /* name      */ "Jaaa",
    /* label     */ "jaaa",
    /* maker     */ "Fons Adriaensen",
    /* copyright */ "GPL v2+",
    PluginDescriptorFILL(JaaaPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_zita_jaaa();

CARLA_EXPORT
void carla_register_native_plugin_zita_jaaa()
{
    carla_register_native_plugin(&jaaaDesc);
}

// -----------------------------------------------------------------------
