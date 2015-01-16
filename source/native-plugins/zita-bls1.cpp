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

#include "zita-bls1/source/png2img.cc"
#include "zita-bls1/source/guiclass.cc"
#include "zita-bls1/source/hp3filt.cc"
#include "zita-bls1/source/jclient.cc"
#include "zita-bls1/source/lfshelf2.cc"
#include "zita-bls1/source/mainwin.cc"
#include "zita-bls1/source/rotary.cc"
#include "zita-bls1/source/shuffler.cc"
#include "zita-bls1/source/styles.cc"

using juce::FloatVectorOperations;
using juce::ScopedPointer;

// -----------------------------------------------------------------------
// BLS1 Plugin

class BLS1Plugin : public NativePluginClass
{
public:
#if 0
    enum Parameters {
        kParameterInput = 0,
        kParameterCount
    };
#endif

    BLS1Plugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fJackClient(),
          fMutex(),
          xresman(),
          jclient(nullptr),
          display(nullptr),
          rootwin(nullptr),
          mainwin(nullptr),
          handler(nullptr),
          leakDetector_BLS1Plugin()
    {
        CARLA_SAFE_ASSERT(host != nullptr);

        carla_zeroStruct(fJackClient);
        gLastJackClient = &fJackClient;

        fJackClient.clientName = "bls1";
        fJackClient.bufferSize = getBufferSize();
        fJackClient.sampleRate = getSampleRate();

        int   argc   = 1;
        char* argv[] = { (char*)"bls1" };
        xresman.init(&argc, argv, (char*)"bls1", nullptr, 0);

        jclient = new Jclient(xresman.rname(), nullptr);

        //fParameters[kParameterInput] = 1.0f;
    }

#if 0
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
#endif

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        if (! fJackClient.active)
        {
            const int iframes(static_cast<int>(frames));

            for (int i=0; i<2; ++i)
                FloatVectorOperations::clear(outBuffer[i], iframes);

            return;
        }

        for (int i=0; i<2; ++i)
            fJackClient.portsAudioIn[i]->buffer = inBuffer[i];

        for (int i=0; i<2; ++i)
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
            if (display == nullptr)
            {
                display = new X_display(nullptr);

                if (display->dpy() == nullptr)
                    return hostUiUnavailable();

                styles_init(display, &xresman);

                rootwin  = new X_rootwin(display);
                mainwin  = new Mainwin(rootwin, &xresman, 0, 0, jclient);
                rootwin->handle_event();

                handler = new X_handler(display, mainwin, EV_X11);

                if (const uintptr_t winId = getUiParentId())
                    XSetTransientForHint(display->dpy(), mainwin->win(), static_cast<Window>(winId));
            }

            handler->next_event();
            XFlush(display->dpy());
        }
        else
        {
            handler = nullptr;
            mainwin  = nullptr;
            rootwin  = nullptr;
            display  = nullptr;
        }
    }

    void uiIdle() override
    {
        if (display == nullptr)
            return;

        int ev;

        for (; (ev = mainwin->process()) == EV_X11;)
        {
            rootwin->handle_event();
            handler->next_event();
        }

#if 0
        // check if parameters were updated
        if (mainwin->_input+1 != static_cast<int>(fParameters[kParameterInput]))
        {
            fParameters[kParameterInput] = mainwin->_input+1;
            uiParameterChanged(kParameterInput, fParameters[kParameterInput]);
        }
#endif

        if (ev == EV_EXIT)
        {
            handler = nullptr;
            mainwin = nullptr;
            rootwin = nullptr;
            display = nullptr;
            uiClosed();
        }
    }

#if 0
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
#endif

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
    X_resman xresman;
    ScopedPointer<Jclient>   jclient;
    ScopedPointer<X_display> display;
    ScopedPointer<X_rootwin> rootwin;
    ScopedPointer<Mainwin>   mainwin;
    ScopedPointer<X_handler> handler;

    //float fParameters[kParameterCount];

    PluginClassEND(BLS1Plugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BLS1Plugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor bls1Desc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 0, //JaaaPlugin::kParameterCount,
    /* paramOuts */ 0,
    /* name      */ "BLS1",
    /* label     */ "bls1",
    /* maker     */ "Fons Adriaensen",
    /* copyright */ "GPL v2+",
    PluginDescriptorFILL(BLS1Plugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_zita_bls1();

CARLA_EXPORT
void carla_register_native_plugin_zita_bls1()
{
    carla_register_native_plugin(&bls1Desc);
}

// -----------------------------------------------------------------------
