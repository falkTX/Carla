/*
 * Carla Plugin Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifdef WANT_PLUGIN

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"

#include "DistrhoPlugin.hpp"

using DISTRHO::d_cconst;
using DISTRHO::d_string;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------
// Parameters

static const unsigned char paramMap[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F
};

static const unsigned int paramVolume  = 5;
static const unsigned int paramBalance = 6;
static const unsigned int paramPan     = 8;

static const unsigned int paramCount   = sizeof(paramMap);
static const unsigned int programCount = 128;
static const unsigned int stateCount   = MAX_RACK_PLUGINS;

// -----------------------------------------

class CarlaEnginePlugin : public CarlaEngine,
                          public DISTRHO::Plugin
{
public:
    CarlaEnginePlugin()
        : CarlaEngine(),
          DISTRHO::Plugin(paramCount, programCount, stateCount),
          paramBuffers{0.0f},
          prevParamBuffers{0.0f}
    {
        carla_debug("CarlaEnginePlugin::CarlaEnginePlugin()");

        // init parameters
        paramBuffers[paramVolume]  = 100.0f;
        paramBuffers[paramBalance] = 63.5f;
        paramBuffers[paramPan]     = 63.5f;

        prevParamBuffers[paramVolume]  = 100.0f;
        prevParamBuffers[paramBalance] = 63.5f;
        prevParamBuffers[paramPan]     = 63.5f;

        // set-up engine
        fOptions.processMode   = PROCESS_MODE_CONTINUOUS_RACK;
        fOptions.transportMode = TRANSPORT_MODE_INTERNAL;
        fOptions.forceStereo   = true;
        fOptions.preferPluginBridges = false;
        fOptions.preferUiBridges = false;
        init("Carla");
    }

    ~CarlaEnginePlugin() override
    {
        carla_debug("CarlaEnginePlugin::~CarlaEnginePlugin()");

        setAboutToClose();
        removeAllPlugins();
        close();
    }

    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEnginePlugin::init(\"%s\")", clientName);

        fBufferSize = d_bufferSize();
        fSampleRate = d_sampleRate();

        CarlaEngine::init(clientName);
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEnginePlugin::close()");
        CarlaEngine::close();

        return true;
    }

    bool isRunning() const override
    {
        return true;
    }

    bool isOffline() const override
    {
        return false;
    }

    EngineType type() const override
    {
        return kEngineTypePlugin;
    }

protected:
    // ---------------------------------------------
    // DISTRHO Plugin Information

    const char* d_label() const override
    {
        return "Carla";
    }

    const char* d_maker() const override
    {
        return "falkTX";
    }

    const char* d_license() const override
    {
        return "GPL v2+";
    }

    uint32_t d_version() const override
    {
        return 0x0600;
    }

    long d_uniqueId() const override
    {
        return d_cconst('C', 'r', 'l', 'a');
    }

    // ---------------------------------------------
    // DISTRHO Plugin Init

    void d_initParameter(uint32_t index, DISTRHO::Parameter& parameter) override
    {
        if (index >= paramCount)
            return;

        parameter.hints = DISTRHO::PARAMETER_IS_AUTOMABLE;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 127.0f;

        if (index == paramVolume)
            parameter.ranges.def = 100.0f;
        else if (index == paramBalance)
            parameter.ranges.def = 63.5f;
        else if (index == paramPan)
            parameter.ranges.def = 63.5f;

        switch (paramMap[index])
        {
        case 0x01:
            parameter.name = "0x01 Modulation";
            break;
        case 0x02:
            parameter.name = "0x02 Breath";
            break;
        case 0x03:
            parameter.name = "0x03 (Undefined)";
            break;
        case 0x04:
            parameter.name = "0x04 Foot";
            break;
        case 0x05:
            parameter.name = "0x05 Portamento";
            break;
        case 0x07:
            parameter.name = "0x07 Volume";
            break;
        case 0x08:
            parameter.name = "0x08 Balance";
            break;
        case 0x09:
            parameter.name = "0x09 (Undefined)";
            break;
        case 0x0A:
            parameter.name = "0x0A Pan";
            break;
        case 0x0B:
            parameter.name = "0x0B Expression";
            break;
        case 0x0C:
            parameter.name = "0x0C FX Control 1";
            break;
        case 0x0D:
            parameter.name = "0x0D FX Control 2";
            break;
        case 0x0E:
            parameter.name = "0x0E (Undefined)";
            break;
        case 0x0F:
            parameter.name = "0x0F (Undefined)";
            break;
        case 0x10:
            parameter.name = "0x10 General Purpose 1";
            break;
        case 0x11:
            parameter.name = "0x11 General Purpose 2";
            break;
        case 0x12:
            parameter.name = "0x12 General Purpose 3";
            break;
        case 0x13:
            parameter.name = "0x13 General Purpose 4";
            break;
        case 0x14:
            parameter.name = "0x14 (Undefined)";
            break;
        case 0x15:
            parameter.name = "0x15 (Undefined)";
            break;
        case 0x16:
            parameter.name = "0x16 (Undefined)";
            break;
        case 0x17:
            parameter.name = "0x17 (Undefined)";
            break;
        case 0x18:
            parameter.name = "0x18 (Undefined)";
            break;
        case 0x19:
            parameter.name = "0x19 (Undefined)";
            break;
        case 0x1A:
            parameter.name = "0x1A (Undefined)";
            break;
        case 0x1B:
            parameter.name = "0x1B (Undefined)";
            break;
        case 0x1C:
            parameter.name = "0x1C (Undefined)";
            break;
        case 0x1D:
            parameter.name = "0x1D (Undefined)";
            break;
        case 0x1E:
            parameter.name = "0x1E (Undefined)";
            break;
        case 0x1F:
            parameter.name = "0x1F (Undefined)";
            break;
        case 0x46:
            parameter.name = "0x46 Control 1 [Variation]";
            break;
        case 0x47:
            parameter.name = "0x47 Control 2 [Timbre]";
            break;
        case 0x48:
            parameter.name = "0x48 Control 3 [Release]";
            break;
        case 0x49:
            parameter.name = "0x49 Control 4 [Attack]";
            break;
        case 0x4A:
            parameter.name = "0x4A Control 5 [Brightness]";
            break;
        case 0x4B:
            parameter.name = "0x4B Control 6 [Decay]";
            break;
        case 0x4C:
            parameter.name = "0x4C Control 7 [Vib Rate]";
            break;
        case 0x4D:
            parameter.name = "0x4D Control 8 [Vib Depth]";
            break;
        case 0x4E:
            parameter.name = "0x4E Control 9 [Vib Delay]";
            break;
        case 0x4F:
            parameter.name = "0x4F Control 10 [Undefined]";
            break;
        case 0x50:
            parameter.name = "0x50 General Purpose 5";
            break;
        case 0x51:
            parameter.name = "0x51 General Purpose 6";
            break;
        case 0x52:
            parameter.name = "0x52 General Purpose 7";
            break;
        case 0x53:
            parameter.name = "0x53 General Purpose 8";
            break;
        case 0x54:
            parameter.name = "0x54 Portamento Control";
            break;
        case 0x5B:
            parameter.name = "0x5B FX 1 Depth [Reverb]";
            break;
        case 0x5C:
            parameter.name = "0x5C FX 2 Depth [Tremolo]";
            break;
        case 0x5D:
            parameter.name = "0x5D FX 3 Depth [Chorus]";
            break;
        case 0x5E:
            parameter.name = "0x5E FX 4 Depth [Detune]";
            break;
        case 0x5F:
            parameter.name = "0x5F FX 5 Depth [Phaser]";
            break;
        }
    }

    void d_initProgramName(uint32_t index, d_string& programName) override
    {
        programName = "Program #" + d_string(index);
    }

    void d_initStateKey(uint32_t index, d_string& stateKey) override
    {
        stateKey = "Plugin #" + d_string(index);
    }

    // ---------------------------------------------
    // DISTRHO Plugin Internal data

    float d_parameterValue(uint32_t index) override
    {
        if (index >= paramCount)
            return 0.0f;

        return paramBuffers[index];
    }

    void d_setParameterValue(uint32_t index, float value) override
    {
        if (index >= paramCount)
            return;

        paramBuffers[index] = value;
    }

    void d_setProgram(uint32_t index) override
    {
        if (index >= programCount)
            return;
        if (kData->curPluginCount == 0)
            return;

        if (CarlaPlugin* const plugin = getPlugin(0))
        {
            if (plugin->programCount() > 0)
            {
                if (index <= plugin->programCount())
                    plugin->setProgram(index, true, true, false);
            }
            else if (plugin->midiProgramCount())
            {
                if (index <= plugin->midiProgramCount())
                    plugin->setMidiProgram(index, true, true, false);
            }
        }
    }

    void  d_setState(const char* key, const char* value) override
    {
        // TODO
        (void)key;
        (void)value;
    }

    // ---------------------------------------------
    // DISTRHO Plugin Process

    void d_activate() override
    {
        static bool firstTestInit = true;

        if (firstTestInit)
        {
            firstTestInit = false;
            if (! addPlugin(PLUGIN_INTERNAL, nullptr, nullptr, "zynaddsubfx"))
                carla_stderr2("Plugin add zynaddsubfx failed");
            if (! addPlugin(PLUGIN_INTERNAL, nullptr, nullptr, "PingPongPan"))
                carla_stderr2("Plugin add Pan failed");
        }

        for (unsigned int i=0; i < kData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(getPluginUnchecked(i));

            if (plugin != nullptr && plugin->enabled())
                plugin->setActive(true, true, false);
        }

        carla_copyFloat(prevParamBuffers, paramBuffers, paramCount);
    }

    void d_deactivate() override
    {
        for (unsigned int i=0; i < kData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(getPluginUnchecked(i));

            if (plugin != nullptr && plugin->enabled())
                plugin->setActive(false, true, false);
        }
    }

    void d_run(float** inputs, float** outputs, uint32_t frames, uint32_t midiEventCount, const DISTRHO::MidiEvent* midiEvents) override
    {
        if (kData->curPluginCount == 0)
        {
            carla_zeroFloat(outputs[0], frames);
            carla_zeroFloat(outputs[1], frames);
            return;
        }

        // create audio buffers
        float* inBuf[2]  = { inputs[0], inputs[1] };
        float* outBuf[2] = { outputs[0], outputs[1] };

        // initialize input events
        carla_zeroStruct<EngineEvent>(kData->rack.in, RACK_EVENT_COUNT);
        {
            uint32_t engineEventIndex = 0;

            for (unsigned int i=0; i < paramCount && engineEventIndex+midiEventCount < RACK_EVENT_COUNT; ++i)
            {
                if (paramBuffers[i] == prevParamBuffers[i])
                    continue;

                EngineEvent* const engineEvent = &kData->rack.in[engineEventIndex++];
                engineEvent->clear();

                engineEvent->type    = kEngineEventTypeControl;
                engineEvent->time    = 0;
                engineEvent->channel = 0;

                engineEvent->ctrl.type  = kEngineControlEventTypeParameter;
                engineEvent->ctrl.param = paramMap[i];
                engineEvent->ctrl.value = paramBuffers[i]/127.0f;

                prevParamBuffers[i] = paramBuffers[i];
            }

            const DISTRHO::MidiEvent* midiEvent;

            for (uint32_t i=0; i < midiEventCount && engineEventIndex < RACK_EVENT_COUNT; ++i)
            {
                midiEvent = &midiEvents[i];

                if (midiEvent->size > 4)
                    continue;

                EngineEvent* const engineEvent = &kData->rack.in[engineEventIndex++];
                engineEvent->clear();

                engineEvent->type    = kEngineEventTypeMidi;
                engineEvent->time    = midiEvent->frame;
                engineEvent->channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent->buf);

                engineEvent->midi.data[0] = MIDI_GET_STATUS_FROM_DATA(midiEvent->buf);
                engineEvent->midi.data[1] = midiEvent->buf[1];
                engineEvent->midi.data[2] = midiEvent->buf[2];
                engineEvent->midi.data[3] = midiEvent->buf[3];
                engineEvent->midi.size    = midiEvent->size;
            }
        }

        processRack(inBuf, outBuf, frames);
    }

    // ---------------------------------------------
    // Callbacks

    void d_bufferSizeChanged(uint32_t newBufferSize) override
    {
        if (fBufferSize == newBufferSize)
            return;

        fBufferSize = newBufferSize;
        bufferSizeChanged(newBufferSize);
    }

    void d_sampleRateChanged(double newSampleRate) override
    {
        if (fSampleRate == newSampleRate)
            return;

        fSampleRate = newSampleRate;
        sampleRateChanged(newSampleRate);
    }

    // ---------------------------------------------

private:
    float paramBuffers[paramCount];
    float prevParamBuffers[paramCount];
};

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new CarlaBackend::CarlaEnginePlugin();
}

END_NAMESPACE_DISTRHO

// -------------------------------------------------

#include "DistrhoPluginMain.cpp"

#endif // CARLA_ENGINE_PLUGIN
