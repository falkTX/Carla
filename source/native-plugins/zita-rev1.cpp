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

#include "zita-rev1/jclient.cc"
#include "zita-rev1/pareq.cc"
#include "zita-rev1/reverb.cc"

using juce::FloatVectorOperations;
using juce::ScopedPointer;

using namespace REV1;

// -----------------------------------------------------------------------
// REV1 Plugin

class REV1Plugin : public NativePluginAndUiClass
{
public:
    enum Parameters {
        kParameterDELAY,
        kParameterXOVER,
        kParameterRTLOW,
        kParameterRTMID,
        kParameterFDAMP,
        kParameterEQ1FR,
        kParameterEQ1GN,
        kParameterEQ2FR,
        kParameterEQ2GN,
        kParameterOPMIXorRGXYZ,
        kParameterNROTARY
    };

    REV1Plugin(const NativeHostDescriptor* const host, const bool isAmbisonic)
        : NativePluginAndUiClass(host, "rev1-ui"),
          kIsAmbisonic(isAmbisonic),
          kNumInputs(2),
          kNumOutputs(isAmbisonic ? 4 : 2),
          fJackClient(),
          jclient(nullptr)
    {
        CARLA_SAFE_ASSERT(host != nullptr);

        carla_zeroStruct(fJackClient);

        fJackClient.bufferSize = getBufferSize();
        fJackClient.sampleRate = getSampleRate();

        // set initial values
        fParameters[kParameterDELAY] = 0.04f;
        fParameters[kParameterXOVER] = 200.0f;
        fParameters[kParameterRTLOW] = 3.0f;
        fParameters[kParameterRTMID] = 2.0f;
        fParameters[kParameterFDAMP] = 6.0e3;
        fParameters[kParameterEQ1FR] = 160.0f;
        fParameters[kParameterEQ1GN] = 0.0f;
        fParameters[kParameterEQ2FR] = 2.5e3;
        fParameters[kParameterEQ2GN] = 0.0f;

        if (isAmbisonic)
            fParameters[kParameterOPMIXorRGXYZ] = 0.0f;
        else
            fParameters[kParameterOPMIXorRGXYZ] = 0.5f;

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
        case kParameterDELAY:
            param.name = "Delay";
            param.ranges.def = 0.04f;
            param.ranges.min = 0.02f;
            param.ranges.max = 0.100f;
            break;
        case kParameterXOVER:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "Crossover";
            param.ranges.def = 200.0f;
            param.ranges.min = 50.0f;
            param.ranges.max = 1000.0f;
            break;
        case kParameterRTLOW:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "RT60 Low";
            param.ranges.def = 3.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 8.0f;
            break;
        case kParameterRTMID:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "RT60 Mid";
            param.ranges.def = 2.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 8.0f;
            break;
        case kParameterFDAMP:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "HF Damping";
            param.ranges.def = 6.0e3;
            param.ranges.min = 1.5e3;
            param.ranges.max = 24.0e3;
            break;
        case kParameterEQ1FR:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "Eq1 Frequency";
            param.ranges.def = 160.0f;
            param.ranges.min = 40.0f;
            param.ranges.max = 2.5e3;
            break;
        case kParameterEQ1GN:
            param.name = "Eq1 Gain";
            param.ranges.def = 0.0f;
            param.ranges.min = -15.0;
            param.ranges.max = 15.0f;
            break;
        case kParameterEQ2FR:
            hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
            param.name = "Eq2 Frequency";
            param.ranges.def = 2.5e3;
            param.ranges.min = 160.0;
            param.ranges.max = 10e3;
            break;
        case kParameterEQ2GN:
            param.name = "Eq2 Gain";
            param.ranges.def = 0.0f;
            param.ranges.min = -15.0;
            param.ranges.max = 15.0f;
            break;
        case kParameterOPMIXorRGXYZ:
            if (kIsAmbisonic)
            {
                param.name = "XYZ gain";
                param.ranges.def = 0.0f;
                param.ranges.min = -9.0f;
                param.ranges.max = 9.0f;
            }
            else
            {
                param.name = "Dry/wet mix";
                param.ranges.def = 0.5f;
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
            }
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

        Reverb* const reverb(jclient->reverb());

        fParameters[index] = value;

        switch (index)
        {
        case kParameterDELAY:
            reverb->set_delay(value);
            break;
        case kParameterXOVER:
            reverb->set_xover(value);
            break;
        case kParameterRTLOW:
            reverb->set_rtlow(value);
            break;
        case kParameterRTMID:
            reverb->set_rtmid(value);
            break;
        case kParameterFDAMP:
            reverb->set_fdamp(value);
            break;
        case kParameterEQ1FR:
            reverb->set_eq1(value, fParameters[kParameterEQ1GN]);
            break;
        case kParameterEQ1GN:
            reverb->set_eq1(fParameters[kParameterEQ1FR], value);
            break;
        case kParameterEQ2FR:
            reverb->set_eq2(value, fParameters[kParameterEQ2GN]);
            break;
        case kParameterEQ2GN:
            reverb->set_eq2(fParameters[kParameterEQ2FR], value);
            break;
        case kParameterOPMIXorRGXYZ:
            if (kIsAmbisonic)
                reverb->set_rgxyz(value);
            else
                reverb->set_opmix(value);
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
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (isPipeRunning())
            {
                const CarlaMutexLocker cml(getPipeLock());
                writeMessage("focus\n", 6);
                flushMessages();
                return;
            }

            carla_stdout("Trying to start UI using \"%s\"", getExtUiPath());

            CarlaExternalUI::setData(getExtUiPath(), kIsAmbisonic ? "true" : "false", getUiName());

            if (! CarlaExternalUI::startPipeServer(true))
            {
                uiClosed();
                hostUiUnavailable();
            }
        }
        else
        {
            CarlaExternalUI::stopPipeServer(2000);
        }
    }

    // -------------------------------------------------------------------

private:
    const bool     kIsAmbisonic;
    const uint32_t kNumInputs;
    const uint32_t kNumOutputs;

    // Fake jack client
    jack_client_t fJackClient;

    // Zita stuff (core)
    ScopedPointer<Jclient> jclient;

    // Parameters
    float fParameters[kParameterNROTARY];

    void _recreateZitaClient()
    {
        jclient = new Jclient(&fJackClient, kIsAmbisonic);

        Reverb* const reverb(jclient->reverb());

        reverb->set_delay(fParameters[kParameterDELAY]);
        reverb->set_xover(fParameters[kParameterXOVER]);
        reverb->set_rtlow(fParameters[kParameterRTLOW]);
        reverb->set_rtmid(fParameters[kParameterRTMID]);
        reverb->set_fdamp(fParameters[kParameterFDAMP]);

        if (kIsAmbisonic)
        {
            reverb->set_opmix(0.5);
            reverb->set_rgxyz(fParameters[kParameterOPMIXorRGXYZ]);
        }
        else
        {
            reverb->set_opmix(fParameters[kParameterOPMIXorRGXYZ]);
            reverb->set_rgxyz(0.0);
        }

        reverb->set_eq1(fParameters[kParameterEQ1FR], fParameters[kParameterEQ1GN]);
        reverb->set_eq2(fParameters[kParameterEQ2FR], fParameters[kParameterEQ2GN]);
    }

public:
    static NativePluginHandle _instantiateAmbisonic(const NativeHostDescriptor* host)
    {
        return (host != nullptr) ? new REV1Plugin(host, true) : nullptr;
    }

    static NativePluginHandle _instantiateStereo(const NativeHostDescriptor* host)
    {
        return (host != nullptr) ? new REV1Plugin(host, false) : nullptr;
    }

    static void _cleanup(NativePluginHandle handle)
    {
        delete (REV1Plugin*)handle;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(REV1Plugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor rev1AmbisonicDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 4,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ REV1Plugin::kParameterNROTARY,
    /* paramOuts */ 0,
    /* name      */ "REV1 (Ambisonic)",
    /* label     */ "rev1-ambisonic",
    /* maker     */ "falkTX, Fons Adriaensen",
    /* copyright */ "GPL v2+",
    REV1Plugin::_instantiateAmbisonic,
    REV1Plugin::_cleanup,
    REV1Plugin::_get_parameter_count,
    REV1Plugin::_get_parameter_info,
    REV1Plugin::_get_parameter_value,
    REV1Plugin::_get_midi_program_count,
    REV1Plugin::_get_midi_program_info,
    REV1Plugin::_set_parameter_value,
    REV1Plugin::_set_midi_program,
    REV1Plugin::_set_custom_data,
    REV1Plugin::_ui_show,
    REV1Plugin::_ui_idle,
    REV1Plugin::_ui_set_parameter_value,
    REV1Plugin::_ui_set_midi_program,
    REV1Plugin::_ui_set_custom_data,
    REV1Plugin::_activate,
    REV1Plugin::_deactivate,
    REV1Plugin::_process,
    REV1Plugin::_get_state,
    REV1Plugin::_set_state,
    REV1Plugin::_dispatcher
};

static const NativePluginDescriptor rev1StereoDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ REV1Plugin::kParameterNROTARY,
    /* paramOuts */ 0,
    /* name      */ "REV1 (Stereo)",
    /* label     */ "rev1-stereo",
    /* maker     */ "falkTX, Fons Adriaensen",
    /* copyright */ "GPL v2+",
    REV1Plugin::_instantiateStereo,
    REV1Plugin::_cleanup,
    REV1Plugin::_get_parameter_count,
    REV1Plugin::_get_parameter_info,
    REV1Plugin::_get_parameter_value,
    REV1Plugin::_get_midi_program_count,
    REV1Plugin::_get_midi_program_info,
    REV1Plugin::_set_parameter_value,
    REV1Plugin::_set_midi_program,
    REV1Plugin::_set_custom_data,
    REV1Plugin::_ui_show,
    REV1Plugin::_ui_idle,
    REV1Plugin::_ui_set_parameter_value,
    REV1Plugin::_ui_set_midi_program,
    REV1Plugin::_ui_set_custom_data,
    REV1Plugin::_activate,
    REV1Plugin::_deactivate,
    REV1Plugin::_process,
    REV1Plugin::_get_state,
    REV1Plugin::_set_state,
    REV1Plugin::_dispatcher
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_zita_rev1();

CARLA_EXPORT
void carla_register_native_plugin_zita_rev1()
{
    carla_register_native_plugin(&rev1AmbisonicDesc);
    carla_register_native_plugin(&rev1StereoDesc);
}

// -----------------------------------------------------------------------
