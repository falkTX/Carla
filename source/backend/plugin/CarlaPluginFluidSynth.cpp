/*
 * Carla FluidSynth Plugin
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef HAVE_FLUIDSYNTH

#include "CarlaMathUtils.hpp"

#include "juce_core.h"

#include <fluidsynth.h>

#if (FLUIDSYNTH_VERSION_MAJOR >= 1 && FLUIDSYNTH_VERSION_MINOR >= 1 && FLUIDSYNTH_VERSION_MICRO >= 4)
# define FLUIDSYNTH_VERSION_NEW_API
#endif

#define FLUID_DEFAULT_POLYPHONY 64

using juce::String;
using juce::StringArray;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------

class CarlaPluginFluidSynth : public CarlaPlugin
{
public:
    CarlaPluginFluidSynth(CarlaEngine* const engine, const uint id, const bool use16Outs)
        : CarlaPlugin(engine, id),
          kUse16Outs(use16Outs),
          fSettings(nullptr),
          fSynth(nullptr),
          fSynthId(0),
          fAudio16Buffers(nullptr),
          fLabel(nullptr)
    {
        carla_debug("CarlaPluginFluidSynth::CarlaPluginFluidSynth(%p, %i, %s)", engine, id,  bool2str(use16Outs));

        FloatVectorOperations::clear(fParamBuffers, FluidSynthParametersMax);
        carla_fill<int32_t>(fCurMidiProgs, 0, MAX_MIDI_CHANNELS);

        // create settings
        fSettings = new_fluid_settings();
        CARLA_SAFE_ASSERT_RETURN(fSettings != nullptr,);

        // define settings
        fluid_settings_setint(fSettings, "synth.audio-channels", use16Outs ? 16 : 1);
        fluid_settings_setint(fSettings, "synth.audio-groups", use16Outs ? 16 : 1);
        fluid_settings_setnum(fSettings, "synth.sample-rate", pData->engine->getSampleRate());
        //fluid_settings_setnum(fSettings, "synth.cpu-cores", 2);
        fluid_settings_setint(fSettings, "synth.parallel-render", 1);
        fluid_settings_setint(fSettings, "synth.threadsafe-api", 0);

        // create synth
        fSynth = new_fluid_synth(fSettings);
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);

#ifdef FLUIDSYNTH_VERSION_NEW_API
        fluid_synth_set_sample_rate(fSynth, (float)pData->engine->getSampleRate());
#endif

        // set default values
        fluid_synth_set_reverb_on(fSynth, 1);
        fluid_synth_set_reverb(fSynth, FLUID_REVERB_DEFAULT_ROOMSIZE, FLUID_REVERB_DEFAULT_DAMP, FLUID_REVERB_DEFAULT_WIDTH, FLUID_REVERB_DEFAULT_LEVEL);

        fluid_synth_set_chorus_on(fSynth, 1);
        fluid_synth_set_chorus(fSynth, FLUID_CHORUS_DEFAULT_N, FLUID_CHORUS_DEFAULT_LEVEL, FLUID_CHORUS_DEFAULT_SPEED, FLUID_CHORUS_DEFAULT_DEPTH, FLUID_CHORUS_DEFAULT_TYPE);

        fluid_synth_set_polyphony(fSynth, FLUID_DEFAULT_POLYPHONY);
        fluid_synth_set_gain(fSynth, 1.0f);

        for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
            fluid_synth_set_interp_method(fSynth, i, FLUID_INTERP_DEFAULT);
    }

    ~CarlaPluginFluidSynth() override
    {
        carla_debug("CarlaPluginFluidSynth::~CarlaPluginFluidSynth()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fSynth != nullptr)
        {
            delete_fluid_synth(fSynth);
            fSynth = nullptr;
        }

        if (fSettings != nullptr)
        {
            delete_fluid_settings(fSettings);
            fSettings = nullptr;
        }

        if (fLabel != nullptr)
        {
            delete[] fLabel;
            fLabel = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_SF2;
    }

    PluginCategory getCategory() const noexcept override
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        switch (parameterId)
        {
        case FluidSynthChorusType:
            return 2;
        case FluidSynthInterpolation:
            return 4;
        default:
            return 0;
        }
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        options |= PLUGIN_OPTION_SEND_PITCHBEND;
        options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId), 0.0f);

        switch (parameterId)
        {
        case FluidSynthChorusType:
            switch (scalePointId)
            {
            case 0:
                return FLUID_CHORUS_MOD_SINE;
            case 1:
                return FLUID_CHORUS_MOD_TRIANGLE;
            default:
                return FLUID_CHORUS_DEFAULT_TYPE;
            }
        case FluidSynthInterpolation:
            switch (scalePointId)
            {
            case 0:
                return FLUID_INTERP_NONE;
            case 1:
                return FLUID_INTERP_LINEAR;
            case 2:
                return FLUID_INTERP_4THORDER;
            case 3:
                return FLUID_INTERP_7THORDER;
            default:
                return FLUID_INTERP_DEFAULT;
            }
        default:
            return 0.0f;
        }
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        if (fLabel != nullptr)
        {
            std::strncpy(strBuf, fLabel, STR_MAX);
            return;
        }

        CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "FluidSynth SF2 engine", STR_MAX);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "GNU GPL v2+", STR_MAX);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        getLabel(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        switch (parameterId)
        {
        case FluidSynthReverbOnOff:
            std::strncpy(strBuf, "Reverb On/Off", STR_MAX);
            return;
        case FluidSynthReverbRoomSize:
            std::strncpy(strBuf, "Reverb Room Size", STR_MAX);
            return;
        case FluidSynthReverbDamp:
            std::strncpy(strBuf, "Reverb Damp", STR_MAX);
            return;
        case FluidSynthReverbLevel:
            std::strncpy(strBuf, "Reverb Level", STR_MAX);
            return;
        case FluidSynthReverbWidth:
            std::strncpy(strBuf, "Reverb Width", STR_MAX);
            return;
        case FluidSynthChorusOnOff:
            std::strncpy(strBuf, "Chorus On/Off", STR_MAX);
            return;
        case FluidSynthChorusNr:
            std::strncpy(strBuf, "Chorus Voice Count", STR_MAX);
            return;
        case FluidSynthChorusLevel:
            std::strncpy(strBuf, "Chorus Level", STR_MAX);
            return;
        case FluidSynthChorusSpeedHz:
            std::strncpy(strBuf, "Chorus Speed", STR_MAX);
            return;
        case FluidSynthChorusDepthMs:
            std::strncpy(strBuf, "Chorus Depth", STR_MAX);
            return;
        case FluidSynthChorusType:
            std::strncpy(strBuf, "Chorus Type", STR_MAX);
            return;
        case FluidSynthPolyphony:
            std::strncpy(strBuf, "Polyphony", STR_MAX);
            return;
        case FluidSynthInterpolation:
            std::strncpy(strBuf, "Interpolation", STR_MAX);
            return;
        case FluidSynthVoiceCount:
            std::strncpy(strBuf, "Voice Count", STR_MAX);
            return;
        }

        CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        switch (parameterId)
        {
        case FluidSynthChorusSpeedHz:
            std::strncpy(strBuf, "Hz", STR_MAX);
            return;
        case FluidSynthChorusDepthMs:
            std::strncpy(strBuf, "ms", STR_MAX);
            return;
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId),);

        switch (parameterId)
        {
        case FluidSynthChorusType:
            switch (scalePointId)
            {
            case 0:
                std::strncpy(strBuf, "Sine wave", STR_MAX);
                return;
            case 1:
                std::strncpy(strBuf, "Triangle wave", STR_MAX);
                return;
            }
        case FluidSynthInterpolation:
            switch (scalePointId)
            {
            case 0:
                std::strncpy(strBuf, "None", STR_MAX);
                return;
            case 1:
                std::strncpy(strBuf, "Straight-line", STR_MAX);
                return;
            case 2:
                std::strncpy(strBuf, "Fourth-order", STR_MAX);
                return;
            case 3:
                std::strncpy(strBuf, "Seventh-order", STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() override
    {
        char strBuf[STR_MAX+1];
        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i", fCurMidiProgs[0],  fCurMidiProgs[1],  fCurMidiProgs[2],  fCurMidiProgs[3],
                                                                                          fCurMidiProgs[4],  fCurMidiProgs[5],  fCurMidiProgs[6],  fCurMidiProgs[7],
                                                                                          fCurMidiProgs[8],  fCurMidiProgs[9],  fCurMidiProgs[10], fCurMidiProgs[11],
                                                                                          fCurMidiProgs[12], fCurMidiProgs[13], fCurMidiProgs[14], fCurMidiProgs[15]);

        CarlaPlugin::setCustomData(CUSTOM_DATA_TYPE_STRING, "midiPrograms", strBuf, false);
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        if (channel >= 0 && channel < MAX_MIDI_CHANNELS)
            pData->midiprog.current = fCurMidiProgs[channel];

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        {
            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            switch (parameterId)
            {
            case FluidSynthReverbOnOff:
                try {
                    fluid_synth_set_reverb_on(fSynth, (fixedValue > 0.5f) ? 1 : 0);
                } catch(...) {}
                break;

            case FluidSynthReverbRoomSize:
            case FluidSynthReverbDamp:
            case FluidSynthReverbLevel:
            case FluidSynthReverbWidth:
                try {
                    fluid_synth_set_reverb(fSynth, fParamBuffers[FluidSynthReverbRoomSize], fParamBuffers[FluidSynthReverbDamp], fParamBuffers[FluidSynthReverbWidth], fParamBuffers[FluidSynthReverbLevel]);
                } catch(...) {}
                break;

            case FluidSynthChorusOnOff:
                try {
                    fluid_synth_set_chorus_on(fSynth, (value > 0.5f) ? 1 : 0);
                } catch(...) {}
                break;

            case FluidSynthChorusNr:
            case FluidSynthChorusLevel:
            case FluidSynthChorusSpeedHz:
            case FluidSynthChorusDepthMs:
            case FluidSynthChorusType:
                try {
                    fluid_synth_set_chorus(fSynth, (int)fParamBuffers[FluidSynthChorusNr], fParamBuffers[FluidSynthChorusLevel], fParamBuffers[FluidSynthChorusSpeedHz], fParamBuffers[FluidSynthChorusDepthMs], (int)fParamBuffers[FluidSynthChorusType]);
                } catch(...) {}
                break;

            case FluidSynthPolyphony:
                try {
                    fluid_synth_set_polyphony(fSynth, (int)value);
                } catch(...) {}
                break;

            case FluidSynthInterpolation:
                for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
                {
                    try {
                        fluid_synth_set_interp_method(fSynth, i, (int)value);
                    }
                    catch(...) {
                        break;
                    }
                }
                break;

            default:
                break;
            }
        }

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);
        carla_debug("CarlaPluginFluidSynth::setCustomData(%s, \"%s\", \"%s\", %s)", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) != 0)
            return carla_stderr2("CarlaPluginFluidSynth::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (std::strcmp(key, "midiPrograms") != 0)
            return carla_stderr2("CarlaPluginFluidSynth::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        StringArray midiProgramList(StringArray::fromTokens(value, ":", ""));

        if (midiProgramList.size() == MAX_MIDI_CHANNELS)
        {
            uint8_t channel = 0;
            for (String *it=midiProgramList.begin(), *end=midiProgramList.end(); it != end; ++it)
            {
                const int index(it->getIntValue());

                if (index >= 0 && index < static_cast<int>(pData->midiprog.count))
                {
                    const uint32_t bank    = pData->midiprog.data[index].bank;
                    const uint32_t program = pData->midiprog.data[index].program;

                    fluid_synth_program_select(fSynth, channel, fSynthId, bank, program);
                    fCurMidiProgs[channel] = index;

                    if (pData->ctrlChannel == static_cast<int32_t>(channel))
                    {
                        pData->midiprog.current = index;
                        pData->engine->callback(ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED, pData->id, index, 0, 0.0f, nullptr);
                    }
                }

                ++channel;
            }
            CARLA_SAFE_ASSERT(channel == MAX_MIDI_CHANNELS);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        if (index >= 0 && pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
        {
            const uint32_t bank    = pData->midiprog.data[index].bank;
            const uint32_t program = pData->midiprog.data[index].program;

            //const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            try {
                fluid_synth_program_select(fSynth, pData->ctrlChannel, fSynthId, bank, program);
            } catch(...) {}

            fCurMidiProgs[pData->ctrlChannel] = index;
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    // nothing

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);
        carla_debug("CarlaPluginFluidSynth::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        uint32_t aOuts, params;
        aOuts  = kUse16Outs ? 32 : 2;
        params = FluidSynthParametersMax;

        pData->audioOut.createNew(aOuts);
        pData->param.createNew(params, false);

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // ---------------------------------------
        // Audio Outputs

        if (kUse16Outs)
        {
            for (uint32_t i=0; i < 32; ++i)
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                portName += "out-";

                if ((i+2)/2 < 9)
                    portName += "0";

                portName += CarlaString((i+2)/2);

                if (i % 2 == 0)
                    portName += "L";
                else
                    portName += "R";

                portName.truncate(portNameSize);

                pData->audioOut.ports[i].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, i);
                pData->audioOut.ports[i].rindex = i;
            }

            fAudio16Buffers = new float*[aOuts];

            for (uint32_t i=0; i < aOuts; ++i)
                fAudio16Buffers[i] = nullptr;
        }
        else
        {
            // out-left
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "out-left";
            portName.truncate(portNameSize);

            pData->audioOut.ports[0].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 0);
            pData->audioOut.ports[0].rindex = 0;

            // out-right
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "out-right";
            portName.truncate(portNameSize);

            pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 1);
            pData->audioOut.ports[1].rindex = 1;
        }

        // ---------------------------------------
        // Event Input

        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
        }

        // ---------------------------------------
        // Event Output

        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        // ---------------------------------------
        // Parameters

        {
            int j;

            // ----------------------
            j = FluidSynthReverbOnOff;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMABLE*/ | PARAMETER_IS_BOOLEAN;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = 1.0f;
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 1.0f;

            // ----------------------
            j = FluidSynthReverbRoomSize;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.2f;
            pData->param.ranges[j].def = FLUID_REVERB_DEFAULT_ROOMSIZE;
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthReverbDamp;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = FLUID_REVERB_DEFAULT_DAMP;
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthReverbLevel;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.data[j].midiCC = MIDI_CONTROL_REVERB_SEND_LEVEL;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = FLUID_REVERB_DEFAULT_LEVEL;
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthReverbWidth;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 10.0f; // should be 100, but that sounds too much
            pData->param.ranges[j].def = FLUID_REVERB_DEFAULT_WIDTH;
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthChorusOnOff;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_BOOLEAN;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = 1.0f;
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 1.0f;

            // ----------------------
            j = FluidSynthChorusNr;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 99.0f;
            pData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_N;
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 10.0f;

            // ----------------------
            j = FluidSynthChorusLevel;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 10.0f;
            pData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_LEVEL;
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthChorusSpeedHz;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.29f;
            pData->param.ranges[j].max = 5.0f;
            pData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_SPEED;
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthChorusDepthMs;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = float(2048.0 * 1000.0 / pData->engine->getSampleRate()); // FIXME?
            pData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_DEPTH;
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthChorusType;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER | PARAMETER_USES_SCALEPOINTS;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = FLUID_CHORUS_MOD_SINE;
            pData->param.ranges[j].max = FLUID_CHORUS_MOD_TRIANGLE;
            pData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_TYPE;
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 1.0f;

            // ----------------------
            j = FluidSynthPolyphony;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 1.0f;
            pData->param.ranges[j].max = 512.0f; // max theoric is 65535
            pData->param.ranges[j].def = (float)fluid_synth_get_polyphony(fSynth);
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 10.0f;

            // ----------------------
            j = FluidSynthInterpolation;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER | PARAMETER_USES_SCALEPOINTS;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = FLUID_INTERP_NONE;
            pData->param.ranges[j].max = FLUID_INTERP_HIGHEST;
            pData->param.ranges[j].def = FLUID_INTERP_DEFAULT;
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 1.0f;

            // ----------------------
            j = FluidSynthVoiceCount;
            pData->param.data[j].type   = PARAMETER_OUTPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 65535.0f;
            pData->param.ranges[j].def = 0.0f;
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 1.0f;

            for (j=0; j<FluidSynthParametersMax; ++j)
                fParamBuffers[j] = pData->param.ranges[j].def;
        }

        // ---------------------------------------

        // plugin hints
        pData->hints  = 0x0;
        pData->hints |= PLUGIN_IS_SYNTH;
        pData->hints |= PLUGIN_CAN_VOLUME;
        pData->hints |= PLUGIN_USES_MULTI_PROGS;

        if (! kUse16Outs)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints  = 0x0;
        pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (! kUse16Outs)
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginFluidSynth::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginFluidSynth::reloadPrograms(%s)", bool2str(doInit));

        // save drum info in case we have one program for it
        bool hasDrums = false;
        uint32_t drumIndex, drumProg;
        drumIndex = drumProg = 0;

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;

        if (fluid_sfont_t* const f_sfont = fluid_synth_get_sfont_by_id(fSynth, fSynthId))
        {
            fluid_preset_t f_preset;

            // initial check to know how many midi-programs we have
            f_sfont->iteration_start(f_sfont);
            for (; f_sfont->iteration_next(f_sfont, &f_preset);)
                ++count;

            // sound kits must always have at least 1 midi-program
            CARLA_SAFE_ASSERT_RETURN(count > 0,);

            pData->midiprog.createNew(count);

            // Update data
            int tmp;
            uint32_t i = 0;
            f_sfont->iteration_start(f_sfont);

            for (; f_sfont->iteration_next(f_sfont, &f_preset);)
            {
                CARLA_SAFE_ASSERT_BREAK(i < count);

                tmp = f_preset.get_banknum(&f_preset);
                pData->midiprog.data[i].bank = (tmp >= 0) ? static_cast<uint32_t>(tmp) : 0;

                tmp = f_preset.get_num(&f_preset);
                pData->midiprog.data[i].program = (tmp >= 0) ? static_cast<uint32_t>(tmp) : 0;

                pData->midiprog.data[i].name = carla_strdup(f_preset.get_name(&f_preset));

                if (pData->midiprog.data[i].bank == 128 && ! hasDrums)
                {
                    hasDrums  = true;
                    drumIndex = i;
                    drumProg  = pData->midiprog.data[i].program;
                }

                ++i;
            }
        }
        else
        {
            // failing means 0 midi-programs, it shouldn't happen!
            carla_safe_assert("fluid_sfont_t* const f_sfont = fluid_synth_get_sfont_by_id(fSynth, fSynthId)", __FILE__, __LINE__);
            return;
        }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        // Update OSC Names
        if (pData->engine->isOscControlRegistered() && pData->id < pData->engine->getCurrentPluginCount())
        {
            pData->engine->oscSend_control_set_midi_program_count(pData->id, count);

            for (uint32_t i=0; i < count; ++i)
                pData->engine->oscSend_control_set_midi_program_data(pData->id, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
        }
#endif

        if (doInit)
        {
            fluid_synth_program_reset(fSynth);

            // select first program, or 128 for ch10
            for (int i=0; i < MAX_MIDI_CHANNELS && i != 9; ++i)
            {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                fluid_synth_set_channel_type(fSynth, i, CHANNEL_TYPE_MELODIC);
#endif
                fluid_synth_program_select(fSynth, i, fSynthId, pData->midiprog.data[0].bank, pData->midiprog.data[0].program);

                fCurMidiProgs[i] = 0;
            }

            if (hasDrums)
            {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                fluid_synth_set_channel_type(fSynth, 9, CHANNEL_TYPE_DRUM);
#endif
                fluid_synth_program_select(fSynth, 9, fSynthId, 128, drumProg);

                fCurMidiProgs[9] = static_cast<int32_t>(drumIndex);
            }
            else
            {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                fluid_synth_set_channel_type(fSynth, 9, CHANNEL_TYPE_MELODIC);
#endif
                fluid_synth_program_select(fSynth, 9, fSynthId, pData->midiprog.data[0].bank, pData->midiprog.data[0].program);

                fCurMidiProgs[9] = 0;
            }

            pData->midiprog.current = 0;
        }
        else
        {
            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(const float** const, float** const audioOut, const float** const, float** const, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
                {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                    fluid_synth_all_notes_off(fSynth, i);
                    fluid_synth_all_sounds_off(fSynth, i);
#else
                    fluid_synth_cc(fSynth, i, MIDI_CONTROL_ALL_SOUND_OFF, 0);
                    fluid_synth_cc(fSynth, i, MIDI_CONTROL_ALL_NOTES_OFF, 0);
#endif
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                for (int i=0; i < MAX_MIDI_NOTE; ++i)
                    fluid_synth_noteoff(fSynth, pData->ctrlChannel, i);
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin2(); it.valid(); it.next())
                {
                    const ExternalMidiNote& note(it.getValue());

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    if (note.velo > 0)
                        fluid_synth_noteon(fSynth, note.channel, note.note, note.velo);
                    else
                        fluid_synth_noteoff(fSynth,note.channel, note.note);
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE
            bool allNotesOffSent = false;
#endif
            uint32_t timeOffset = 0;

            uint32_t nextBankIds[MAX_MIDI_CHANNELS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0 };

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0 && pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
                nextBankIds[pData->ctrlChannel] = pData->midiprog.data[pData->midiprog.current].bank;

            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (event.time > timeOffset)
                {
                    if (processSingle(audioOut, event.time - timeOffset, timeOffset))
                    {
                        timeOffset = event.time;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0 && pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
                            nextBankIds[pData->ctrlChannel] = pData->midiprog.data[pData->midiprog.current].bank;
                    }
                }

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl:
                {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
                    {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.value/0.5f - 1.0f;

                                if (value < 0.0f)
                                {
                                    left  = -1.0f;
                                    right = (value*2.0f)+1.0f;
                                }
                                else if (value > 0.0f)
                                {
                                    left  = (value*2.0f)-1.0f;
                                    right = 1.0f;
                                }
                                else
                                {
                                    left  = -1.0f;
                                    right = 1.0f;
                                }

                                setBalanceLeft(left, false, false);
                                setBalanceRight(right, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            }
                        }
#endif
                        // Control plugin parameters
                        for (uint32_t k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].hints != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (pData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? pData->param.ranges[k].min : pData->param.ranges[k].max;
                            }
                            else
                            {
                                value = pData->param.ranges[k].getUnnormalizedValue(ctrlEvent.value);

                                if (pData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_CONTROL)
                        {
                            fluid_synth_cc(fSynth, event.channel, ctrlEvent.param, int(ctrlEvent.value*127.0f));
                        }
                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel < MAX_MIDI_CHANNELS && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankIds[event.channel] = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel < MAX_MIDI_CHANNELS && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t bankId(nextBankIds[event.channel]);
                            const uint32_t progId(ctrlEvent.param);

                            // TODO int32_t midiprog.find(bank, prog)
                            for (uint32_t k=0; k < pData->midiprog.count; ++k)
                            {
                                if (pData->midiprog.data[k].bank == bankId && pData->midiprog.data[k].program == progId)
                                {
                                    fluid_synth_program_select(fSynth, event.channel, fSynthId, bankId, progId);
                                    fCurMidiProgs[event.channel] = static_cast<int32_t>(k);

                                    if (event.channel == pData->ctrlChannel)
                                        pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, static_cast<int32_t>(k), 0, 0.0f);

                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                            fluid_synth_all_sounds_off(fSynth, event.channel);
#else
                            fluid_synth_cc(fSynth, event.channel, MIDI_CONTROL_ALL_SOUND_OFF, 0);
#endif
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

#ifdef FLUIDSYNTH_VERSION_NEW_API
                            fluid_synth_all_notes_off(fSynth, event.channel);
#else
                            fluid_synth_cc(fSynth, event.channel, MIDI_CONTROL_ALL_NOTES_OFF, 0);
#endif
                        }
                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > EngineMidiEvent::kDataSize)
                        continue;

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    switch (status)
                    {
                    case MIDI_STATUS_NOTE_OFF: {
                        const uint8_t note = midiEvent.data[1];

                        fluid_synth_noteoff(fSynth, event.channel, note);

                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, event.channel, note, 0.0f);
                        break;
                    }

                    case MIDI_STATUS_NOTE_ON: {
                        const uint8_t note = midiEvent.data[1];
                        const uint8_t velo = midiEvent.data[2];

                        fluid_synth_noteon(fSynth, event.channel, note, velo);

                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, event.channel, note, velo);
                        break;
                    }

                    case MIDI_STATUS_POLYPHONIC_AFTERTOUCH:
                        if (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
                        {
                            //const uint8_t note     = midiEvent.data[1];
                            //const uint8_t pressure = midiEvent.data[2];

                            // not in fluidsynth API
                        }
                        break;

                    case MIDI_STATUS_CONTROL_CHANGE:
                        if (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
                        {
                            const uint8_t control = midiEvent.data[1];
                            const uint8_t value   = midiEvent.data[2];

                            fluid_synth_cc(fSynth, event.channel, control, value);
                        }
                        break;

                    case MIDI_STATUS_CHANNEL_PRESSURE:
                        if (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
                        {
                            const uint8_t pressure = midiEvent.data[1];

                            fluid_synth_channel_pressure(fSynth, event.channel, pressure);;
                        }
                        break;

                    case MIDI_STATUS_PITCH_WHEEL_CONTROL:
                        if (pData->options & PLUGIN_OPTION_SEND_PITCHBEND)
                        {
                            const uint8_t lsb = midiEvent.data[1];
                            const uint8_t msb = midiEvent.data[2];
                            const int   value = ((msb << 7) | lsb);

                            fluid_synth_pitch_bend(fSynth, event.channel, value);
                        }
                        break;

                    default:
                        continue;
                        break;
                    } // switch (status)
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioOut, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Control Output

        {
            uint32_t k = FluidSynthVoiceCount;
            fParamBuffers[k] = float(fluid_synth_get_active_voice_count(fSynth));
            pData->param.ranges[k].fixValue(fParamBuffers[k]);

            if (pData->param.data[k].midiCC > 0)
            {
                float value(pData->param.ranges[k].getNormalizedValue(fParamBuffers[k]));
                pData->event.portOut->writeControlEvent(0, pData->param.data[k].midiChannel, kEngineControlEventTypeParameter, static_cast<uint16_t>(pData->param.data[k].midiCC), value);
            }

        } // End of Control Output
#endif
    }

    bool processSingle(float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Fill plugin buffers and Run plugin

        if (kUse16Outs)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(fAudio16Buffers[i], static_cast<int>(frames));

            // FIXME use '32' or '16' instead of outs
            fluid_synth_process(fSynth, static_cast<int>(frames), 0, nullptr, static_cast<int>(pData->audioOut.count), fAudio16Buffers);
        }
        else
            fluid_synth_write_float(fSynth, static_cast<int>(frames), outBuffer[0] + timeOffset, 0, 1, outBuffer[1] + timeOffset, 0, 1);

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (volume and balance)

        {
            // note - balance not possible with kUse16Outs, so we can safely skip fAudioOutBuffers
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && carla_isNotEqual(pData->postProc.volume, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));

            float oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        FloatVectorOperations::copy(oldBufLeft, outBuffer[i]+timeOffset, static_cast<int>(frames));

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (i % 2 == 0)
                        {
                            // left
                            outBuffer[i][k+timeOffset]  = oldBufLeft[k]                * (1.0f - balRangeL);
                            outBuffer[i][k+timeOffset] += outBuffer[i+1][k+timeOffset] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k+timeOffset]  = outBuffer[i][k+timeOffset] * balRangeR;
                            outBuffer[i][k+timeOffset] += oldBufLeft[k]              * balRangeL;
                        }
                    }
                }

                // Volume
                if (kUse16Outs)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudio16Buffers[i][k] * pData->postProc.volume;
                }
                else if (doVolume)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing
#else
        if (kUse16Outs)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = fAudio16Buffers[i][k];
            }
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        if (! kUse16Outs)
            return;

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudio16Buffers[i] != nullptr)
                delete[] fAudio16Buffers[i];
            fAudio16Buffers[i] = new float[newBufferSize];
        }
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_SAFE_ASSERT_RETURN(fSettings != nullptr,);

        fluid_settings_setnum(fSettings, "synth.sample-rate", newSampleRate);

#ifdef FLUIDSYNTH_VERSION_NEW_API
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);

        fluid_synth_set_sample_rate(fSynth, float(newSampleRate));
#endif
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginFluidSynth::clearBuffers() - start");

        if (fAudio16Buffers != nullptr)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                if (fAudio16Buffers[i] != nullptr)
                {
                    delete[] fAudio16Buffers[i];
                    fAudio16Buffers[i] = nullptr;
                }
            }

            delete[] fAudio16Buffers;
            fAudio16Buffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginFluidSynth::clearBuffers() - end");
    }

    // -------------------------------------------------------------------

    const void* getExtraStuff() const noexcept override
    {
        static const char xtrue[]  = "true";
        static const char xfalse[] = "false";
        return kUse16Outs ? xtrue : xfalse;
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (fSynth == nullptr)
        {
            pData->engine->setLastError("null synth");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr || label[0] == '\0')
        {
            pData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // open soundfont

        const int synthId(fluid_synth_sfload(fSynth, filename, 0));

        if (synthId < 0)
        {
            pData->engine->setLastError("Failed to load SoundFont file");
            return false;
        }

        fSynthId = static_cast<uint>(synthId);

        // ---------------------------------------------------------------
        // get info

        CarlaString label2(label);

        if (kUse16Outs && ! label2.endsWith(" (16 outs)"))
            label2 += " (16 outs)";

        fLabel          = label2.dup();
        pData->filename = carla_strdup(filename);

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(label);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // set default options

        pData->options  = 0x0;
        pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
        pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        return true;
    }

private:
    enum FluidSynthParameters {
        FluidSynthReverbOnOff    = 0,
        FluidSynthReverbRoomSize = 1,
        FluidSynthReverbDamp     = 2,
        FluidSynthReverbLevel    = 3,
        FluidSynthReverbWidth    = 4,
        FluidSynthChorusOnOff    = 5,
        FluidSynthChorusNr       = 6,
        FluidSynthChorusLevel    = 7,
        FluidSynthChorusSpeedHz  = 8,
        FluidSynthChorusDepthMs  = 9,
        FluidSynthChorusType     = 10,
        FluidSynthPolyphony      = 11,
        FluidSynthInterpolation  = 12,
        FluidSynthVoiceCount     = 13,
        FluidSynthParametersMax  = 14
    };

    const bool kUse16Outs;

    fluid_settings_t* fSettings;
    fluid_synth_t*    fSynth;
    uint              fSynthId;

    float** fAudio16Buffers;
    float   fParamBuffers[FluidSynthParametersMax];

    int32_t fCurMidiProgs[MAX_MIDI_CHANNELS];

    const char* fLabel;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginFluidSynth)
};

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_FLUIDSYNTH

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newFluidSynth(const Initializer& init, const bool use16Outs)
{
    carla_debug("CarlaPlugin::newFluidSynth({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "}, %s)", init.engine, init.filename, init.name, init.label, init.uniqueId, bool2str(use16Outs));

#ifdef HAVE_FLUIDSYNTH
    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && use16Outs)
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo modules, please choose the 2-channel only SoundFont version");
        return nullptr;
    }

    if (! fluid_is_soundfont(init.filename))
    {
        init.engine->setLastError("Requested file is not a valid SoundFont");
        return nullptr;
    }

    CarlaPluginFluidSynth* const plugin(new CarlaPluginFluidSynth(init.engine, init.id,  use16Outs));

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("fluidsynth support not available");
    return nullptr;

    // unused
    (void)use16Outs;
#endif
}

CarlaPlugin* CarlaPlugin::newFileSF2(const Initializer& init, const bool use16Outs)
{
    carla_debug("CarlaPlugin::newFileSF2({%p, \"%s\", \"%s\", \"%s\"}, %s)", init.engine, init.filename, init.name, init.label, bool2str(use16Outs));
#ifdef HAVE_FLUIDSYNTH
    return newFluidSynth(init, use16Outs);
#else
    init.engine->setLastError("SF2 support not available");
    return nullptr;

    // unused
    (void)use16Outs;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
