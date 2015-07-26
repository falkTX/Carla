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

#include "zita-at1/jclient.cc"
#include "zita-at1/retuner.cc"

using juce::roundToIntAccurate;
using juce::FloatVectorOperations;
using juce::ScopedPointer;

using namespace AT1;

// -----------------------------------------------------------------------
// AT1 Plugin

class AT1Plugin : public NativePluginAndUiClass
{
public:
    enum Parameters {
        // rotary knobs
        kParameterR_TUNE,
        kParameterR_FILT,
        kParameterR_BIAS,
        kParameterR_CORR,
        kParameterR_OFFS,
        // knob count
        kParameterNROTARY,
        // midi channel
        kParameterM_CHANNEL = kParameterNROTARY,
        // final count
        kNumParameters
    };

    AT1Plugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "at1-ui"),
          fJackClient(),
          jclient(nullptr),
          notemask(0xfff)
    {
        CARLA_SAFE_ASSERT(host != nullptr);

        carla_zeroStruct(fJackClient);

        fJackClient.bufferSize = getBufferSize();
        fJackClient.sampleRate = getSampleRate();

        // set initial values
        fParameters[kParameterR_TUNE] = 440.0f;
        fParameters[kParameterR_FILT] = 0.1f;
        fParameters[kParameterR_BIAS] = 0.5f;
        fParameters[kParameterR_CORR] = 1.0f;
        fParameters[kParameterR_OFFS] = 0.0f;
        fParameters[kParameterM_CHANNEL] = 0.0f;

        _recreateZitaClient();
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kNumParameters;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kNumParameters, nullptr);

        static NativeParameter param;
        static NativeParameterScalePoint scalePoints[17];

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
        case kParameterR_TUNE:
            param.name = "Tuning";
            param.ranges.def = 440.0f;
            param.ranges.min = 400.0f;
            param.ranges.max = 480.0f;
            break;
        case kParameterR_FILT:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "Filter";
            param.ranges.def = 0.1f;
            param.ranges.min = 0.02f;
            param.ranges.max = 0.5f;
            break;
        case kParameterR_BIAS:
            param.name = "Bias";
            param.ranges.def = 0.5f;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case kParameterR_CORR:
            param.name = "Correction";
            param.ranges.def = 1.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case kParameterR_OFFS:
            param.name = "Offset";
            param.ranges.def = 0.0f;
            param.ranges.min = -2.0f;
            param.ranges.max = 2.0f;
            break;
        case kParameterM_CHANNEL:
            hints |= NATIVE_PARAMETER_USES_SCALEPOINTS;
            param.name = "MIDI Channel";
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 16.0f;
            param.scalePointCount = 17;
            param.scalePoints     = scalePoints;
            scalePoints[ 0].value = 0.0f;
            scalePoints[ 0].label = "Omni";
            scalePoints[ 1].value = 1.0f;
            scalePoints[ 1].label = "1";
            scalePoints[ 2].value = 2.0f;
            scalePoints[ 2].label = "2";
            scalePoints[ 3].value = 3.0f;
            scalePoints[ 3].label = "3";
            scalePoints[ 4].value = 4.0f;
            scalePoints[ 4].label = "4";
            scalePoints[ 5].value = 5.0f;
            scalePoints[ 5].label = "5";
            scalePoints[ 6].value = 6.0f;
            scalePoints[ 6].label = "6";
            scalePoints[ 7].value = 7.0f;
            scalePoints[ 7].label = "7";
            scalePoints[ 8].value = 8.0f;
            scalePoints[ 8].label = "8";
            scalePoints[ 9].value = 9.0f;
            scalePoints[ 9].label = "9";
            scalePoints[10].value = 10.0f;
            scalePoints[10].label = "10";
            scalePoints[11].value = 11.0f;
            scalePoints[11].label = "11";
            scalePoints[12].value = 12.0f;
            scalePoints[12].label = "12";
            scalePoints[13].value = 13.0f;
            scalePoints[13].label = "13";
            scalePoints[14].value = 14.0f;
            scalePoints[14].label = "14";
            scalePoints[15].value = 15.0f;
            scalePoints[15].label = "15";
            scalePoints[16].value = 16.0f;
            scalePoints[16].label = "16";
            break;
        }

        param.hints = static_cast<NativeParameterHints>(hints);

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kNumParameters, 0.0f);

        return fParameters[index];
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kNumParameters,);

        fParameters[index] = value;

        Retuner* const retuner(jclient->retuner());

        switch (index)
        {
        case kParameterR_TUNE:
            retuner->set_refpitch(value);
            break;
        case kParameterR_FILT:
            retuner->set_corrfilt(value);
            break;
        case kParameterR_BIAS:
            retuner->set_notebias(value);
            break;
        case kParameterR_CORR:
            retuner->set_corrgain(value);
            break;
        case kParameterR_OFFS:
            retuner->set_corroffs(value);
            break;
        case kParameterM_CHANNEL:
            jclient->set_midichan(value-1.0f);
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames,
                 const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        if (! fJackClient.active)
        {
            FloatVectorOperations::clear(outBuffer[0], static_cast<int>(frames));
            return;
        }

        fJackClient.portsAudioIn [0].buffer.audio = inBuffer [0];
        fJackClient.portsAudioOut[0].buffer.audio = outBuffer[0];

        fJackClient.portsMidiIn[0].buffer.midi.count  = midiEventCount;
        fJackClient.portsMidiIn[0].buffer.midi.events = const_cast<NativeMidiEvent*>(midiEvents);

        fJackClient.processCallback(frames, fJackClient.processPtr);
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        NativePluginAndUiClass::uiShow(show);

        if (show && isPipeRunning())
        {
            char tmpBuf[0xff+1];
            tmpBuf[0xff] = '\0';

            const CarlaMutexLocker cml(getPipeLock());

            writeMessage("zita-mask\n", 10);

            std::snprintf(tmpBuf, 0xff, "%i\n", notemask);
            writeMessage(tmpBuf);

            flushMessages();
        }
    }

    void uiIdle() override
    {
        NativePluginAndUiClass::uiIdle();

        if (! isPipeRunning())
            return;

        char tmpBuf[0xff+1];
        tmpBuf[0xff] = '\0';

        const CarlaMutexLocker cmlc(fClientMutex);
        const CarlaMutexLocker cmlp(getPipeLock());
        const ScopedLocale csl;

        Retuner* const retuner(jclient->retuner());

        writeMessage("zita-data\n", 10);

        std::snprintf(tmpBuf, 0xff, "%f\n", retuner->get_error());
        writeMessage(tmpBuf);

        std::snprintf(tmpBuf, 0xff, "%i\n", retuner->get_noteset());
        writeMessage(tmpBuf);

        std::snprintf(tmpBuf, 0xff, "%i\n", jclient->get_midiset());
        writeMessage(tmpBuf);

        flushMessages();
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        char tmpBuf[0xff+1];
        tmpBuf[0xff] = '\0';
        std::snprintf(tmpBuf, 0xff, "%i", notemask);

        return strdup(tmpBuf);
    }

    void setState(const char* const data) override
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr && data[0] != '\0',);

        notemask = std::atoi(data);

        const CarlaMutexLocker cml(fClientMutex);
        jclient->set_notemask(notemask);
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
    // Pipe Server calls

    bool msgReceived(const char* const msg) noexcept override
    {
        if (NativePluginAndUiClass::msgReceived(msg))
            return true;

        if (std::strcmp(msg, "zita-mask") == 0)
        {
            int mask;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(mask), true);

            const CarlaMutexLocker cml(fClientMutex);

            try {
                if (mask < 0)
                {
                    jclient->clr_midimask();
                }
                else
                {
                    notemask = static_cast<uint>(mask);
                    jclient->set_notemask(mask);
                }
            } CARLA_SAFE_EXCEPTION("msgReceived, zita-mask");

            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

private:
    // Fake jack client
    jack_client_t fJackClient;

    // Zita stuff (core)
    ScopedPointer<Jclient> jclient;
    uint notemask;

    // Parameters
    float fParameters[kNumParameters];

    // mutex to make sure jclient is always valid
    CarlaMutex fClientMutex;

    void _recreateZitaClient()
    {
        const CarlaMutexLocker cml(fClientMutex);

        jclient = new Jclient(&fJackClient);
        jclient->set_notemask(notemask);
        jclient->set_midichan(roundToIntAccurate(fParameters[kParameterM_CHANNEL])-1);

        Retuner* const retuner(jclient->retuner());
        retuner->set_refpitch(fParameters[kParameterR_TUNE]);
        retuner->set_corrfilt(fParameters[kParameterR_FILT]);
        retuner->set_notebias(fParameters[kParameterR_BIAS]);
        retuner->set_corrgain(fParameters[kParameterR_CORR]);
        retuner->set_corroffs(fParameters[kParameterR_OFFS]);
    }

    PluginClassEND(AT1Plugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AT1Plugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor at1Desc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_USES_STATE),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 1,
    /* audioOuts */ 1,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ AT1Plugin::kNumParameters,
    /* paramOuts */ 0,
    /* name      */ "AT1",
    /* label     */ "at1",
    /* maker     */ "falkTX, Fons Adriaensen",
    /* copyright */ "GPL v2+",
    PluginDescriptorFILL(AT1Plugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_zita_at1();

CARLA_EXPORT
void carla_register_native_plugin_zita_at1()
{
    carla_register_native_plugin(&at1Desc);
}

// -----------------------------------------------------------------------
