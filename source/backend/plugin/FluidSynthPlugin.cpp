/*
 * Carla FluidSynth Plugin
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginInternal.hpp"

#ifdef WANT_FLUIDSYNTH

#include <fluidsynth.h>

#include <QtCore/QStringList>

#define FLUIDSYNTH_VERSION_NEW_API (FLUIDSYNTH_VERSION_MAJOR >= 1 && FLUIDSYNTH_VERSION_MINOR >= 1 && FLUIDSYNTH_VERSION_MICRO >= 4)

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

#define FLUID_DEFAULT_POLYPHONY 64

class FluidSynthPlugin : public CarlaPlugin
{
public:
    FluidSynthPlugin(CarlaEngine* const engine, const unsigned int id, const bool use16Outs)
        : CarlaPlugin(engine, id),
          kUses16Outs(use16Outs),
          fSettings(nullptr),
          fSynth(nullptr),
          fSynthId(-1),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fAudio16Buffers(nullptr),
          fParamBuffers{0.0f},
          fCurMidiProgs{0}
#else
          fAudio16Buffers(nullptr)
#endif
    {
        carla_debug("FluidSynthPlugin::FluidSynthPlugin(%p, %i, %s)", engine, id,  bool2str(use16Outs));

#ifndef CARLA_PROPER_CPP11_SUPPORT
        carla_zeroFloat(fParamBuffers, FluidSynthParametersMax);
        carla_fill<int32_t>(fCurMidiProgs, MAX_MIDI_CHANNELS, 0);
#endif

        // create settings
        fSettings = new_fluid_settings();
        CARLA_ASSERT(fSettings != nullptr);

        // define settings
        fluid_settings_setint(fSettings, "synth.audio-channels", use16Outs ? 16 : 1);
        fluid_settings_setint(fSettings, "synth.audio-groups", use16Outs ? 16 : 1);
        fluid_settings_setnum(fSettings, "synth.sample-rate", kData->engine->getSampleRate());
        fluid_settings_setint(fSettings, "synth.threadsafe-api ", 0);

        // create synth
        fSynth = new_fluid_synth(fSettings);
        CARLA_ASSERT(fSynth != nullptr);

#ifdef FLUIDSYNTH_VERSION_NEW_API
        fluid_synth_set_sample_rate(fSynth, kData->engine->getSampleRate());
#endif

        // set default values
        fluid_synth_set_reverb_on(fSynth, 0);
        fluid_synth_set_reverb(fSynth, FLUID_REVERB_DEFAULT_ROOMSIZE, FLUID_REVERB_DEFAULT_DAMP, FLUID_REVERB_DEFAULT_WIDTH, FLUID_REVERB_DEFAULT_LEVEL);

        fluid_synth_set_chorus_on(fSynth, 0);
        fluid_synth_set_chorus(fSynth, FLUID_CHORUS_DEFAULT_N, FLUID_CHORUS_DEFAULT_LEVEL, FLUID_CHORUS_DEFAULT_SPEED, FLUID_CHORUS_DEFAULT_DEPTH, FLUID_CHORUS_DEFAULT_TYPE);

        fluid_synth_set_polyphony(fSynth, FLUID_DEFAULT_POLYPHONY);
        fluid_synth_set_gain(fSynth, 1.0f);

        for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
            fluid_synth_set_interp_method(fSynth, i, FLUID_INTERP_DEFAULT);
    }

    ~FluidSynthPlugin() override
    {
        carla_debug("FluidSynthPlugin::~FluidSynthPlugin()");

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        if (kData->active)
        {
            deactivate();
            kData->active = false;
        }

        delete_fluid_synth(fSynth);
        delete_fluid_settings(fSettings);

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const override
    {
        return PLUGIN_SF2;
    }

    PluginCategory category() override
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t parameterScalePointCount(const uint32_t parameterId) const override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

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

    unsigned int availableOptions() override
    {
        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        options |= PLUGIN_OPTION_SEND_PITCHBEND;
        options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        return options;
    }

    float getParameterValue(const uint32_t parameterId) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

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

    void getLabel(char* const strBuf) override
    {
        if (fLabel.isNotEmpty())
            std::strncpy(strBuf, (const char*)fLabel, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) override
    {
        std::strncpy(strBuf, "FluidSynth SF2 engine", STR_MAX);
    }

    void getCopyright(char* const strBuf) override
    {
        std::strncpy(strBuf, "GNU GPL v2+", STR_MAX);
    }

    void getRealName(char* const strBuf) override
    {
        getLabel(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        switch (parameterId)
        {
        case FluidSynthReverbOnOff:
            std::strncpy(strBuf, "Reverb On/Off", STR_MAX);
            break;
        case FluidSynthReverbRoomSize:
            std::strncpy(strBuf, "Reverb Room Size", STR_MAX);
            break;
        case FluidSynthReverbDamp:
            std::strncpy(strBuf, "Reverb Damp", STR_MAX);
            break;
        case FluidSynthReverbLevel:
            std::strncpy(strBuf, "Reverb Level", STR_MAX);
            break;
        case FluidSynthReverbWidth:
            std::strncpy(strBuf, "Reverb Width", STR_MAX);
            break;
        case FluidSynthChorusOnOff:
            std::strncpy(strBuf, "Chorus On/Off", STR_MAX);
            break;
        case FluidSynthChorusNr:
            std::strncpy(strBuf, "Chorus Voice Count", STR_MAX);
            break;
        case FluidSynthChorusLevel:
            std::strncpy(strBuf, "Chorus Level", STR_MAX);
            break;
        case FluidSynthChorusSpeedHz:
            std::strncpy(strBuf, "Chorus Speed", STR_MAX);
            break;
        case FluidSynthChorusDepthMs:
            std::strncpy(strBuf, "Chorus Depth", STR_MAX);
            break;
        case FluidSynthChorusType:
            std::strncpy(strBuf, "Chorus Type", STR_MAX);
            break;
        case FluidSynthPolyphony:
            std::strncpy(strBuf, "Polyphony", STR_MAX);
            break;
        case FluidSynthInterpolation:
            std::strncpy(strBuf, "Interpolation", STR_MAX);
            break;
        case FluidSynthVoiceCount:
            std::strncpy(strBuf, "Voice Count", STR_MAX);
            break;
        default:
            CarlaPlugin::getParameterName(parameterId, strBuf);
            break;
        }
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        switch (parameterId)
        {
        case FluidSynthChorusSpeedHz:
            std::strncpy(strBuf, "Hz", STR_MAX);
            break;
        case FluidSynthChorusDepthMs:
            std::strncpy(strBuf, "ms", STR_MAX);
            break;
        default:
            CarlaPlugin::getParameterUnit(parameterId, strBuf);
            break;
        }
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

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
        CARLA_ASSERT(fSynth != nullptr);

        char strBuf[STR_MAX+1];
        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i", fCurMidiProgs[0],  fCurMidiProgs[1],  fCurMidiProgs[2],  fCurMidiProgs[3],
                                                                                          fCurMidiProgs[4],  fCurMidiProgs[5],  fCurMidiProgs[6],  fCurMidiProgs[7],
                                                                                          fCurMidiProgs[8],  fCurMidiProgs[9],  fCurMidiProgs[10], fCurMidiProgs[11],
                                                                                          fCurMidiProgs[12], fCurMidiProgs[13], fCurMidiProgs[14], fCurMidiProgs[15]);

        CarlaPlugin::setCustomData(CUSTOM_DATA_STRING, "midiPrograms", strBuf, false);
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) override
    {
        if (channel < MAX_MIDI_CHANNELS)
            kData->midiprog.current = fCurMidiProgs[channel];

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue(kData->param.fixValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        {
            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            switch (parameterId)
            {
            case FluidSynthReverbOnOff:
                fluid_synth_set_reverb_on(fSynth, (fixedValue > 0.5f) ? 1 : 0);
                break;

            case FluidSynthReverbRoomSize:
            case FluidSynthReverbDamp:
            case FluidSynthReverbLevel:
            case FluidSynthReverbWidth:
                fluid_synth_set_reverb(fSynth, fParamBuffers[FluidSynthReverbRoomSize], fParamBuffers[FluidSynthReverbDamp], fParamBuffers[FluidSynthReverbWidth], fParamBuffers[FluidSynthReverbLevel]);
                break;

            case FluidSynthChorusOnOff:
                fluid_synth_set_chorus_on(fSynth, (value > 0.5f) ? 1 : 0);
                break;

            case FluidSynthChorusNr:
            case FluidSynthChorusLevel:
            case FluidSynthChorusSpeedHz:
            case FluidSynthChorusDepthMs:
            case FluidSynthChorusType:
                fluid_synth_set_chorus(fSynth, fParamBuffers[FluidSynthChorusNr], fParamBuffers[FluidSynthChorusLevel], fParamBuffers[FluidSynthChorusSpeedHz], fParamBuffers[FluidSynthChorusDepthMs], fParamBuffers[FluidSynthChorusType]);
                break;

            case FluidSynthPolyphony:
                fluid_synth_set_polyphony(fSynth, value);
                break;

            case FluidSynthInterpolation:
                for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
                    fluid_synth_set_interp_method(fSynth, i, value);
                break;

            default:
                break;
            }
        }

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_ASSERT(fSynth != nullptr);
        CARLA_ASSERT(type != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        carla_debug("DssiPlugin::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (type == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_STRING) != 0)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (key == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (std::strcmp(key, "midiPrograms") != 0)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (value == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        QStringList midiProgramList(QString(value).split(":", QString::SkipEmptyParts));

        if (midiProgramList.count() == MAX_MIDI_CHANNELS)
        {
            uint i = 0;
            foreach (const QString& midiProg, midiProgramList)
            {
                bool ok;
                uint index = midiProg.toUInt(&ok);

                if (ok && index < kData->midiprog.count)
                {
                    const uint32_t bank    = kData->midiprog.data[index].bank;
                    const uint32_t program = kData->midiprog.data[index].program;

                    fluid_synth_program_select(fSynth, i, fSynthId, bank, program);
                    fCurMidiProgs[i] = index;

                    if (kData->ctrlChannel == static_cast<int32_t>(i))
                    {
                        kData->midiprog.current = index;
                        kData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fId, index, 0, 0.0f, nullptr);
                    }
                }

                ++i;
            }
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(fSynth != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        if (kData->ctrlChannel < 0 || kData->ctrlChannel >= MAX_MIDI_CHANNELS)
            return;

        if (index >= 0)
        {
            const uint32_t bank    = kData->midiprog.data[index].bank;
            const uint32_t program = kData->midiprog.data[index].program;

            //const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));
            fluid_synth_program_select(fSynth, kData->ctrlChannel, fSynthId, bank, program);
            fCurMidiProgs[kData->ctrlChannel] = index;
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    // nothing

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("FluidSynthPlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fSynth != nullptr);

        if (kData->engine == nullptr)
            return;
        if (fSynth == nullptr)
            return;

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (kData->active)
            deactivate();

        clearBuffers();

        uint32_t aOuts, params, j;
        aOuts  = kUses16Outs ? 32 : 2;
        params = FluidSynthParametersMax;

        kData->audioOut.createNew(aOuts);
        kData->param.createNew(params);

        const int   portNameSize = kData->engine->maxPortNameSize();
        CarlaString portName;

        // ---------------------------------------
        // Audio Outputs

        if (kUses16Outs)
        {
            for (j=0; j < 32; ++j)
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fName;
                    portName += ":";
                }

                portName += "out-";

                if ((j+2)/2 < 9)
                    portName += "0";

                portName += CarlaString((j+2)/2);

                if (j % 2 == 0)
                    portName += "L";
                else
                    portName += "R";

                portName.truncate(portNameSize);

                kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                kData->audioOut.ports[j].rindex = j;
            }

            fAudio16Buffers = new float*[aOuts];

            for (j=0; j < aOuts; ++j)
                fAudio16Buffers[j] = nullptr;
        }
        else
        {
            // out-left
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "out-left";
            portName.truncate(portNameSize);

            kData->audioOut.ports[0].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[0].rindex = 0;

            // out-right
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "out-right";
            portName.truncate(portNameSize);

            kData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[1].rindex = 1;
        }

        // ---------------------------------------
        // Event Input

        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        // ---------------------------------------
        // Event Output

        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // ----------------------
        j = FluidSynthReverbOnOff;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 1.0f;
        kData->param.ranges[j].def = 0.0f; // off
        kData->param.ranges[j].step = 1.0f;
        kData->param.ranges[j].stepSmall = 1.0f;
        kData->param.ranges[j].stepLarge = 1.0f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthReverbRoomSize;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 1.2f;
        kData->param.ranges[j].def = FLUID_REVERB_DEFAULT_ROOMSIZE;
        kData->param.ranges[j].step = 0.01f;
        kData->param.ranges[j].stepSmall = 0.0001f;
        kData->param.ranges[j].stepLarge = 0.1f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthReverbDamp;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 1.0f;
        kData->param.ranges[j].def = FLUID_REVERB_DEFAULT_DAMP;
        kData->param.ranges[j].step = 0.01f;
        kData->param.ranges[j].stepSmall = 0.0001f;
        kData->param.ranges[j].stepLarge = 0.1f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthReverbLevel;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = MIDI_CONTROL_REVERB_SEND_LEVEL;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 1.0f;
        kData->param.ranges[j].def = FLUID_REVERB_DEFAULT_LEVEL;
        kData->param.ranges[j].step = 0.01f;
        kData->param.ranges[j].stepSmall = 0.0001f;
        kData->param.ranges[j].stepLarge = 0.1f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthReverbWidth;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 10.0f; // should be 100, but that sounds too much
        kData->param.ranges[j].def = FLUID_REVERB_DEFAULT_WIDTH;
        kData->param.ranges[j].step = 0.01f;
        kData->param.ranges[j].stepSmall = 0.0001f;
        kData->param.ranges[j].stepLarge = 0.1f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusOnOff;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_BOOLEAN;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 1.0f;
        kData->param.ranges[j].def = 0.0f; // off
        kData->param.ranges[j].step = 1.0f;
        kData->param.ranges[j].stepSmall = 1.0f;
        kData->param.ranges[j].stepLarge = 1.0f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusNr;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 99.0f;
        kData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_N;
        kData->param.ranges[j].step = 1.0f;
        kData->param.ranges[j].stepSmall = 1.0f;
        kData->param.ranges[j].stepLarge = 10.0f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusLevel;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = 0; //MIDI_CONTROL_CHORUS_SEND_LEVEL;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 10.0f;
        kData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_LEVEL;
        kData->param.ranges[j].step = 0.01f;
        kData->param.ranges[j].stepSmall = 0.0001f;
        kData->param.ranges[j].stepLarge = 0.1f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusSpeedHz;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.29f;
        kData->param.ranges[j].max = 5.0f;
        kData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_SPEED;
        kData->param.ranges[j].step = 0.01f;
        kData->param.ranges[j].stepSmall = 0.0001f;
        kData->param.ranges[j].stepLarge = 0.1f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusDepthMs;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 2048000.0 / kData->engine->getSampleRate();
        kData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_DEPTH;
        kData->param.ranges[j].step = 0.01f;
        kData->param.ranges[j].stepSmall = 0.0001f;
        kData->param.ranges[j].stepLarge = 0.1f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusType;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER | PARAMETER_USES_SCALEPOINTS;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = FLUID_CHORUS_MOD_SINE;
        kData->param.ranges[j].max = FLUID_CHORUS_MOD_TRIANGLE;
        kData->param.ranges[j].def = FLUID_CHORUS_DEFAULT_TYPE;
        kData->param.ranges[j].step = 1.0f;
        kData->param.ranges[j].stepSmall = 1.0f;
        kData->param.ranges[j].stepLarge = 1.0f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthPolyphony;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 1.0f;
        kData->param.ranges[j].max = 512.0f; // max theoric is 65535
        kData->param.ranges[j].def = fluid_synth_get_polyphony(fSynth);
        kData->param.ranges[j].step = 1.0f;
        kData->param.ranges[j].stepSmall = 1.0f;
        kData->param.ranges[j].stepLarge = 10.0f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthInterpolation;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_INPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER | PARAMETER_USES_SCALEPOINTS;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = FLUID_INTERP_NONE;
        kData->param.ranges[j].max = FLUID_INTERP_HIGHEST;
        kData->param.ranges[j].def = FLUID_INTERP_DEFAULT;
        kData->param.ranges[j].step = 1.0f;
        kData->param.ranges[j].stepSmall = 1.0f;
        kData->param.ranges[j].stepLarge = 1.0f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ----------------------
        j = FluidSynthVoiceCount;
        kData->param.data[j].index  = j;
        kData->param.data[j].rindex = j;
        kData->param.data[j].type   = PARAMETER_OUTPUT;
        kData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
        kData->param.data[j].midiChannel = 0;
        kData->param.data[j].midiCC = -1;
        kData->param.ranges[j].min = 0.0f;
        kData->param.ranges[j].max = 65535.0f;
        kData->param.ranges[j].def = 0.0f;
        kData->param.ranges[j].step = 1.0f;
        kData->param.ranges[j].stepSmall = 1.0f;
        kData->param.ranges[j].stepLarge = 1.0f;
        fParamBuffers[j] = kData->param.ranges[j].def;

        // ---------------------------------------

        // plugin hints
        fHints  = 0x0;
        fHints |= PLUGIN_IS_SYNTH;
        fHints |= PLUGIN_CAN_VOLUME;
        fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        kData->extraHints  = 0x0;
        kData->extraHints |= PLUGIN_HINT_HAS_MIDI_IN;
        kData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        if (kData->active)
            activate();

        carla_debug("FluidSynthPlugin::reload() - end");
    }

    void reloadPrograms(const bool init) override
    {
        carla_debug("FluidSynthPlugin::reloadPrograms(%s)", bool2str(init));

        // Delete old programs
        kData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        fluid_sfont_t* f_sfont;
        fluid_preset_t f_preset;

        bool hasDrums = false;
        uint32_t drumIndex, drumProg;

        f_sfont = fluid_synth_get_sfont_by_id(fSynth, fSynthId);

        // initial check to know how much midi-programs we have
        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
            count += 1;

        // soundfonts must always have at least 1 midi-program
        CARLA_ASSERT(count > 0);

        if (count == 0)
            return;

        kData->midiprog.createNew(count);

        // Update data
        uint32_t i = 0;
        f_sfont->iteration_start(f_sfont);

        while (f_sfont->iteration_next(f_sfont, &f_preset))
        {
            CARLA_ASSERT(i < kData->midiprog.count);
            kData->midiprog.data[i].bank    = f_preset.get_banknum(&f_preset);
            kData->midiprog.data[i].program = f_preset.get_num(&f_preset);
            kData->midiprog.data[i].name    = carla_strdup(f_preset.get_name(&f_preset));

            if (kData->midiprog.data[i].bank == 128 && ! hasDrums)
            {
                hasDrums  = true;
                drumIndex = i;
                drumProg  = kData->midiprog.data[i].program;
            }

            ++i;
        }

        //f_sfont->free(f_sfont);

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_midi_program_count(fId, count);

            for (i=0; i < count; ++i)
                kData->engine->osc_send_control_set_midi_program_data(fId, i, kData->midiprog.data[i].bank, kData->midiprog.data[i].program, kData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            fluid_synth_program_reset(fSynth);

            // select first program, or 128 for ch10
            for (i=0; i < MAX_MIDI_CHANNELS && i != 9; ++i)
            {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                fluid_synth_set_channel_type(fSynth, i, CHANNEL_TYPE_MELODIC);
#endif
                fluid_synth_program_select(fSynth, i, fSynthId, kData->midiprog.data[0].bank, kData->midiprog.data[0].program);

                fCurMidiProgs[i] = 0;
            }

            if (hasDrums)
            {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                fluid_synth_set_channel_type(fSynth, 9, CHANNEL_TYPE_DRUM);
#endif
                fluid_synth_program_select(fSynth, 9, fSynthId, 128, drumProg);

                fCurMidiProgs[9] = drumIndex;
            }
            else
            {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                fluid_synth_set_channel_type(fSynth, 9, CHANNEL_TYPE_MELODIC);
#endif
                fluid_synth_program_select(fSynth, 9, fSynthId, kData->midiprog.data[0].bank, kData->midiprog.data[0].program);

                fCurMidiProgs[9] = 0;
            }

            kData->midiprog.current = 0;
        }
        else
        {
            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const, float** const outBuffer, const uint32_t frames) override
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (kData->needsReset)
        {
            if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (int c=0; c < MAX_MIDI_CHANNELS; ++c)
                {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                    fluid_synth_all_notes_off(fSynth, c);
                    fluid_synth_all_sounds_off(fSynth, c);
#else
                    fluid_synth_cc(fSynth, c, MIDI_CONTROL_ALL_SOUND_OFF, 0);
                    fluid_synth_cc(fSynth, c, MIDI_CONTROL_ALL_NOTES_OFF, 0);
#endif
                }
            }
            else if (kData->ctrlChannel >= 0 && kData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                for (k=0; k < MAX_MIDI_NOTE; ++k)
                    fluid_synth_noteoff(fSynth, kData->ctrlChannel, k);
            }

            kData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                while (! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(kData->extNotes.data.getFirst(true));

                    CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    if (note.velo > 0)
                        fluid_synth_noteon(fSynth, note.channel, note.note, note.velo);
                    else
                        fluid_synth_noteoff(fSynth,note.channel, note.note);
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;

            uint32_t time, nEvents = kData->event.portIn->getEventCount();
            uint32_t timeOffset = 0;

            uint32_t nextBankIds[MAX_MIDI_CHANNELS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0 };

            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0 && kData->ctrlChannel >= 0 && kData->ctrlChannel < MAX_MIDI_CHANNELS)
                nextBankIds[kData->ctrlChannel] = kData->midiprog.data[kData->midiprog.current].bank;

            for (i=0; i < nEvents; ++i)
            {
                const EngineEvent& event(kData->event.portIn->getEvent(i));

                time = event.time;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset)
                {
                    if (processSingle(outBuffer, time - timeOffset, timeOffset))
                    {
                        timeOffset = time;

                        if (kData->midiprog.current >= 0 && kData->midiprog.count > 0 && kData->ctrlChannel >= 0 && kData->ctrlChannel < 16)
                            nextBankIds[kData->ctrlChannel] = kData->midiprog.data[kData->midiprog.current].bank;
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
                        if (event.channel == kData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) > 0)
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
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            }
                        }
#endif

                        // Control plugin parameters
                        for (k=0; k < kData->param.count; ++k)
                        {
                            if (kData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (kData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (kData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((kData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (kData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                value = kData->param.ranges[i].unnormalizeValue(ctrlEvent.value);

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        if ((fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            fluid_synth_cc(fSynth, event.channel, ctrlEvent.param, ctrlEvent.value*127.0f);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel < MAX_MIDI_CHANNELS && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankIds[event.channel] = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel < MAX_MIDI_CHANNELS && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t bankId(nextBankIds[event.channel]);
                            const uint32_t progId(ctrlEvent.param);

                            for (k=0; k < kData->midiprog.count; ++k)
                            {
                                if (kData->midiprog.data[k].bank == bankId && kData->midiprog.data[k].program == progId)
                                {
                                    fluid_synth_program_select(fSynth, event.channel, fSynthId, bankId, progId);
                                    fCurMidiProgs[event.channel] = k;

                                    if (event.channel == kData->ctrlChannel)
                                        postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);

                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                            fluid_synth_all_sounds_off(fSynth, event.channel);
#else
                            fluid_synth_cc(fSynth, event.channel, MIDI_CONTROL_ALL_SOUND_OFF, 0);
#endif
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == kData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

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

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;

                    // Fix bad note-off
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status -= 0x10;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        const uint8_t note = midiEvent.data[1];

                        fluid_synth_noteoff(fSynth, channel, note);

                        postponeRtEvent(kPluginPostRtEventNoteOff, channel, note, 0.0f);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        const uint8_t note = midiEvent.data[1];
                        const uint8_t velo = midiEvent.data[2];

                        fluid_synth_noteon(fSynth, channel, note, velo);

                        postponeRtEvent(kPluginPostRtEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) != 0)
                    {
                        //const uint8_t note     = midiEvent.data[1];
                        //const uint8_t pressure = midiEvent.data[2];

                        // TODO, not in fluidsynth API
                    }
                    else if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0)
                    {
                        const uint8_t control = midiEvent.data[1];
                        const uint8_t value   = midiEvent.data[2];

                        fluid_synth_cc(fSynth, channel, control, value);
                    }
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) != 0)
                    {
                        const uint8_t pressure = midiEvent.data[1];

                        fluid_synth_channel_pressure(fSynth, channel, pressure);;
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status) && (fOptions & PLUGIN_OPTION_SEND_PITCHBEND) != 0)
                    {
                        const uint8_t lsb = midiEvent.data[1];
                        const uint8_t msb = midiEvent.data[2];

                        fluid_synth_pitch_bend(fSynth, channel, (msb << 7) | lsb);
                    }

                    break;
                }
                }
            }

            kData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(outBuffer, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        {
            k = FluidSynthVoiceCount;
            fParamBuffers[k] = fluid_synth_get_active_voice_count(fSynth);
            kData->param.ranges[k].fixValue(fParamBuffers[k]);

            if (kData->param.data[k].midiCC > 0)
            {
                float value(kData->param.ranges[k].normalizeValue(fParamBuffers[k]));
                kData->event.portOut->writeControlEvent(0, kData->param.data[k].midiChannel, kEngineControlEventTypeParameter, kData->param.data[k].midiCC, value);
            }

        } // End of Control Output
    }

    bool processSingle(float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_ASSERT(outBuffer != nullptr);
        CARLA_ASSERT(frames > 0);

        if (outBuffer == nullptr)
            return false;
        if (frames == 0)
            return false;

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (kData->engine->isOffline())
        {
            kData->singleMutex.lock();
        }
        else if (! kData->singleMutex.tryLock())
        {
            for (i=0; i < kData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Fill plugin buffers and Run plugin

        if (kUses16Outs)
        {
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(fAudio16Buffers[i], frames);

            fluid_synth_process(fSynth, frames, 0, nullptr, kData->audioOut.count, fAudio16Buffers);
        }
        else
            fluid_synth_write_float(fSynth, frames, outBuffer[0] + timeOffset, 0, 1, outBuffer[1] + timeOffset, 0, 1);

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (volume and balance)

        {
            // note - balance not possible with kUses16Outs, so we can safely skip fAudioOutBuffers
            const bool doVolume  = (fHints & PLUGIN_CAN_VOLUME) > 0 && kData->postProc.volume != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) > 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            float oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; ++i)
            {
                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        carla_copyFloat(oldBufLeft, outBuffer[i]+timeOffset, frames);

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; ++k)
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
                if (kUses16Outs)
                {
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudio16Buffers[i][k] * kData->postProc.volume;
                }
                else if (doVolume)
                {
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] *= kData->postProc.volume;
                }
            }

        } // End of Post-processing
#else
        if (kUses16Outs)
        {
            for (i=0; i < kData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = fAudio16Buffers[i][k];
            }
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        kData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        if (! kUses16Outs)
            return;

        for (uint32_t i=0; i < kData->audioOut.count; ++i)
        {
            if (fAudio16Buffers[i] != nullptr)
                delete[] fAudio16Buffers[i];
            fAudio16Buffers[i] = new float[newBufferSize];
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() override
    {
        carla_debug("FluidSynthPlugin::clearBuffers() - start");

        if (fAudio16Buffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioOut.count; ++i)
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

        carla_debug("FluidSynthPlugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------

    const void* getExtraStuff() override
    {
        return kUses16Outs ? (const void*)0x1 : nullptr;
    }

    bool init(const char* const filename, const char* const name, const char* const label)
    {
        CARLA_ASSERT(fSynth != nullptr);
        CARLA_ASSERT(filename != nullptr);
        CARLA_ASSERT(label != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (fSynth == nullptr)
        {
            kData->engine->setLastError("null synth");
            return false;
        }

        if (filename == nullptr)
        {
            kData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr)
        {
            kData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // open soundfont

        fSynthId = fluid_synth_sfload(fSynth, filename, 0);

        if (fSynthId < 0)
        {
            kData->engine->setLastError("Failed to load SoundFont file");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        fFilename = filename;
        fLabel    = label;

        if (name != nullptr)
            fName = kData->engine->getUniquePluginName(name);
        else
            fName = kData->engine->getUniquePluginName(label);

        // ---------------------------------------------------------------
        // register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            fOptions = 0x0;

            fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
            fOptions |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            fOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
            fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

            // load settings
            kData->idStr  = "SF2/";
            kData->idStr += label;
            fOptions = kData->loadSettings(fOptions, availableOptions());
        }

        return true;
    }

private:
    enum FluidSynthInputParameters {
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

    const bool kUses16Outs;

    CarlaString fLabel;

    fluid_settings_t* fSettings;
    fluid_synth_t* fSynth;
    int fSynthId;

    float** fAudio16Buffers;
    float   fParamBuffers[FluidSynthParametersMax];

    int32_t fCurMidiProgs[MAX_MIDI_CHANNELS];

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FluidSynthPlugin)
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_FLUIDSYNTH
# warning fluidsynth not available (no SF2 support)
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newSF2(const Initializer& init, const bool use16Outs)
{
    carla_debug("CarlaPlugin::newSF2({%p, \"%s\", \"%s\", \"%s\"}, %s)", init.engine, init.filename, init.name, init.label, bool2str(use16Outs));

#ifdef WANT_FLUIDSYNTH
    if (! fluid_is_soundfont(init.filename))
    {
        init.engine->setLastError("Requested file is not a valid SoundFont");
        return nullptr;
    }

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && use16Outs)
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo modules, please choose the 2-channel only SoundFont version");
        return nullptr;
    }

    FluidSynthPlugin* const plugin(new FluidSynthPlugin(init.engine, init.id,  use16Outs));

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    return plugin;
#else
    init.engine->setLastError("fluidsynth support not available");
    return nullptr;

    // unused
    (void)use16Outs;
#endif
}

CARLA_BACKEND_END_NAMESPACE
