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

#include "CarlaNativeExtUI.hpp"
#include "CarlaJuceUtils.hpp"

#include "juce_audio_basics.h"

#include "zita-bls1/hp3filt.cc"
#include "zita-bls1/jclient.cc"
#include "zita-bls1/lfshelf2.cc"
#include "zita-bls1/shuffler.cc"

using juce::FloatVectorOperations;
using juce::ScopedPointer;

using namespace BLS1;

// -----------------------------------------------------------------------
// BLS1 Plugin

class BLS1Plugin : public NativePluginAndUiClass
{
public:
    static const uint32_t kNumInputs  = 2;
    static const uint32_t kNumOutputs = 2;

    enum Parameters {
        kParameterINPBAL,
        kParameterHPFILT,
        kParameterSHGAIN,
        kParameterSHFREQ,
        kParameterLFFREQ,
        kParameterLFGAIN,
        kParameterNROTARY
    };

    BLS1Plugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "bls1-ui"),
          fJackClient(),
          jclient(nullptr)
    {
        CARLA_SAFE_ASSERT(host != nullptr);

        carla_zeroStruct(fJackClient);

        fJackClient.bufferSize = getBufferSize();
        fJackClient.sampleRate = getSampleRate();

        // set initial values
        fParameters[kParameterINPBAL] = 0.0f;
        fParameters[kParameterHPFILT] = 40.0f;
        fParameters[kParameterSHGAIN] = 15.0f;
        fParameters[kParameterSHFREQ] = 5e2f;
        fParameters[kParameterLFFREQ] = 80.0f;
        fParameters[kParameterLFGAIN] = 0.0f;

        _recreateZitaClient();
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParameterNROTARY;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterNROTARY, nullptr);

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
        case kParameterINPBAL:
            param.name = "Input balance";
            //param.unit = "dB";
            param.ranges.def = 0.0f;
            param.ranges.min = -3.0f;
            param.ranges.max = 3.0f;
            break;
        case kParameterHPFILT:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "Highpass filter";
            param.ranges.def = 40.0f;
            param.ranges.min = 10.0f;
            param.ranges.max = 320.0f;
            break;
        case kParameterSHGAIN:
            param.name = "Shuffler gain";
            param.ranges.def = 15.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 24.0f;
            break;
        case kParameterSHFREQ:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "Shuffler frequency";
            param.ranges.def = 5e2f;
            param.ranges.min = 125.0f;
            param.ranges.max = 2e3f;
            break;
        case kParameterLFFREQ:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "LF shelf filter frequency";
            param.ranges.def = 80.0f;
            param.ranges.min = 20.0f;
            param.ranges.max = 320.0f;
            break;
        case kParameterLFGAIN:
            param.name = "LF shelf filter gain";
            param.ranges.def = 0.0f;
            param.ranges.min = -9.0f;
            param.ranges.max = 9.0f;
            break;
        }

        param.hints = static_cast<NativeParameterHints>(hints);

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterNROTARY, 0.0f);

        return fParameters[index];
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterNROTARY,);

        fParameters[index] = value;

        switch (index)
        {
        case kParameterINPBAL:
            jclient->set_inpbal(value);
            break;
        case kParameterHPFILT:
            jclient->set_hpfilt(value);
            break;
        case kParameterSHGAIN:
            jclient->shuffler()->prepare(value, fParameters[kParameterSHFREQ]);
            break;
        case kParameterSHFREQ:
            jclient->shuffler()->prepare(fParameters[kParameterSHGAIN], value);
            break;
        case kParameterLFFREQ:
            jclient->set_loshelf(fParameters[kParameterLFGAIN], value);
            break;
        case kParameterLFGAIN:
            jclient->set_loshelf(value, fParameters[kParameterLFFREQ]);
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        if (! fJackClient.active)
        {
            const int iframes(static_cast<int>(frames));

            for (uint32_t i=0; i<kNumInputs; ++i)
                FloatVectorOperations::clear(outBuffer[i], iframes);

            return;
        }

        for (uint32_t i=0; i<kNumInputs; ++i)
            fJackClient.portsAudioIn[i].buffer.audio = inBuffer[i];

        for (uint32_t i=0; i<kNumOutputs; ++i)
            fJackClient.portsAudioOut[i].buffer.audio = outBuffer[i];

        fJackClient.processCallback(frames, fJackClient.processPtr);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        fJackClient.bufferSize = bufferSize;
        // _recreateZitaClient(); // FIXME
    }

    void sampleRateChanged(const double sampleRate) override
    {
        fJackClient.sampleRate = sampleRate;
        // _recreateZitaClient(); // FIXME
    }

    // -------------------------------------------------------------------

private:
    // Fake jack client
    jack_client_t fJackClient;

    // Zita stuff (core)
    ScopedPointer<Jclient> jclient;

    // Parameters
    float fParameters[kParameterNROTARY];

    void _recreateZitaClient()
    {
        jclient = new Jclient(&fJackClient);
        jclient->set_inpbal(fParameters[kParameterINPBAL]);
        jclient->set_hpfilt(fParameters[kParameterHPFILT]);
        jclient->shuffler()->prepare(fParameters[kParameterSHGAIN], fParameters[kParameterSHFREQ]);
        jclient->set_loshelf(fParameters[kParameterLFGAIN], fParameters[kParameterLFFREQ]);
    }

    PluginClassEND(BLS1Plugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BLS1Plugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor bls1Desc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_FILTER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ BLS1Plugin::kNumInputs,
    /* audioOuts */ BLS1Plugin::kNumOutputs,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ BLS1Plugin::kParameterNROTARY,
    /* paramOuts */ 0,
    /* name      */ "BLS1",
    /* label     */ "bls1",
    /* maker     */ "falkTX, Fons Adriaensen",
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
