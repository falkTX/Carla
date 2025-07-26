// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef HAVE_FLUIDSYNTH

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"

#include "water/text/StringArray.h"

#include <fluidsynth.h>

#define FLUID_DEFAULT_POLYPHONY 64

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };

// -------------------------------------------------------------------------------------------------------------------

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

        carla_zeroFloats(fParamBuffers, FluidSynthParametersMax);
        carla_fill<int32_t>(fCurMidiProgs, 0, MAX_MIDI_CHANNELS);

        // create settings
        fSettings = new_fluid_settings();
        CARLA_SAFE_ASSERT_RETURN(fSettings != nullptr,);

        // define settings
        fluid_settings_setint(fSettings, "synth.audio-channels", use16Outs ? 16 : 1);
        fluid_settings_setint(fSettings, "synth.audio-groups", use16Outs ? 16 : 1);
        fluid_settings_setnum(fSettings, "synth.sample-rate", pData->engine->getSampleRate());
        //fluid_settings_setnum(fSettings, "synth.cpu-cores", 2);
        fluid_settings_setint(fSettings, "synth.ladspa.active", 0);
        fluid_settings_setint(fSettings, "synth.lock-memory", 1);
#if FLUIDSYNTH_VERSION_MAJOR < 2
        fluid_settings_setint(fSettings, "synth.parallel-render", 1);
#endif
        fluid_settings_setint(fSettings, "synth.threadsafe-api", 0);
#ifdef DEBUG
        fluid_settings_setint(fSettings, "synth.verbose", 1);
#endif

        // create synth
        fSynth = new_fluid_synth(fSettings);
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);

        initializeFluidDefaultsIfNeeded();

#if FLUIDSYNTH_VERSION_MAJOR < 2
        fluid_synth_set_sample_rate(fSynth, static_cast<float>(pData->engine->getSampleRate()));
#endif

        // set default values
        fluid_synth_set_reverb_on(fSynth, 1);
        fluid_synth_set_reverb(fSynth,
                               sFluidDefaults[FluidSynthReverbRoomSize],
                               sFluidDefaults[FluidSynthReverbDamp],
                               sFluidDefaults[FluidSynthReverbWidth],
                               sFluidDefaults[FluidSynthReverbLevel]);

        fluid_synth_set_chorus_on(fSynth, 1);
        fluid_synth_set_chorus(fSynth,
                               static_cast<int>(sFluidDefaults[FluidSynthChorusNr] + 0.5f),
                               sFluidDefaults[FluidSynthChorusLevel],
                               sFluidDefaults[FluidSynthChorusSpeedHz],
                               sFluidDefaults[FluidSynthChorusDepthMs],
                               static_cast<int>(sFluidDefaults[FluidSynthChorusType] + 0.5f));

        fluid_synth_set_polyphony(fSynth, FLUID_DEFAULT_POLYPHONY);
        fluid_synth_set_gain(fSynth, 1.0f);

        for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
            fluid_synth_set_interp_method(fSynth, i, static_cast<int>(sFluidDefaults[FluidSynthInterpolation] + 0.5f));
    }

    ~CarlaPluginFluidSynth() override
    {
        carla_debug("CarlaPluginFluidSynth::~CarlaPluginFluidSynth()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

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
        options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
#if FLUIDSYNTH_VERSION_MAJOR >= 2
        options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
#endif

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
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
                return sFluidDefaults[FluidSynthChorusType];
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
                return sFluidDefaults[FluidSynthInterpolation];
            }
        default:
            return 0.0f;
        }
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        if (fLabel != nullptr)
        {
            std::strncpy(strBuf, fLabel, STR_MAX);
            return true;
        }

        return CarlaPlugin::getLabel(strBuf);
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "FluidSynth SF2 engine", STR_MAX);
        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "GNU GPL v2+", STR_MAX);
        return true;
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        return getLabel(strBuf);
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        switch (parameterId)
        {
        case FluidSynthReverbOnOff:
            std::strncpy(strBuf, "Reverb On/Off", STR_MAX);
            return true;
        case FluidSynthReverbRoomSize:
            std::strncpy(strBuf, "Reverb Room Size", STR_MAX);
            return true;
        case FluidSynthReverbDamp:
            std::strncpy(strBuf, "Reverb Damp", STR_MAX);
            return true;
        case FluidSynthReverbLevel:
            std::strncpy(strBuf, "Reverb Level", STR_MAX);
            return true;
        case FluidSynthReverbWidth:
            std::strncpy(strBuf, "Reverb Width", STR_MAX);
            return true;
        case FluidSynthChorusOnOff:
            std::strncpy(strBuf, "Chorus On/Off", STR_MAX);
            return true;
        case FluidSynthChorusNr:
            std::strncpy(strBuf, "Chorus Voice Count", STR_MAX);
            return true;
        case FluidSynthChorusLevel:
            std::strncpy(strBuf, "Chorus Level", STR_MAX);
            return true;
        case FluidSynthChorusSpeedHz:
            std::strncpy(strBuf, "Chorus Speed", STR_MAX);
            return true;
        case FluidSynthChorusDepthMs:
            std::strncpy(strBuf, "Chorus Depth", STR_MAX);
            return true;
        case FluidSynthChorusType:
            std::strncpy(strBuf, "Chorus Type", STR_MAX);
            return true;
        case FluidSynthPolyphony:
            std::strncpy(strBuf, "Polyphony", STR_MAX);
            return true;
        case FluidSynthInterpolation:
            std::strncpy(strBuf, "Interpolation", STR_MAX);
            return true;
        case FluidSynthVoiceCount:
            std::strncpy(strBuf, "Voice Count", STR_MAX);
            return true;
        }

        return CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    bool getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        switch (parameterId)
        {
        case FluidSynthChorusSpeedHz:
            std::strncpy(strBuf, "Hz", STR_MAX);
            return true;
        case FluidSynthChorusDepthMs:
            std::strncpy(strBuf, "ms", STR_MAX);
            return true;
        }

        return CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    bool getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId), false);

        switch (parameterId)
        {
        case FluidSynthChorusType:
            switch (scalePointId)
            {
            case 0:
                std::strncpy(strBuf, "Sine wave", STR_MAX);
                return true;
            case 1:
                std::strncpy(strBuf, "Triangle wave", STR_MAX);
                return true;
            }
            break;
        case FluidSynthInterpolation:
            switch (scalePointId)
            {
            case 0:
                std::strncpy(strBuf, "None", STR_MAX);
                return true;
            case 1:
                std::strncpy(strBuf, "Straight-line", STR_MAX);
                return true;
            case 2:
                std::strncpy(strBuf, "Fourth-order", STR_MAX);
                return true;
            case 3:
                std::strncpy(strBuf, "Seventh-order", STR_MAX);
                return true;
            }
            break;
        }

        return CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave(bool) override
    {
        char strBuf[STR_MAX+1];
        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i",
                      fCurMidiProgs[0],  fCurMidiProgs[1],  fCurMidiProgs[2],  fCurMidiProgs[3],
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
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback,);

        float fixedValue;

        {
            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));
            fixedValue = setParameterValueInFluidSynth(parameterId, value);

        }

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        const float fixedValue = setParameterValueInFluidSynth(parameterId, value);

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    float setParameterValueInFluidSynth(const uint32_t parameterId, const float value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, value);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        switch (parameterId)
        {
        case FluidSynthReverbOnOff:
            try {
                fluid_synth_set_reverb_on(fSynth, (fixedValue > 0.5f) ? 1 : 0);
            } CARLA_SAFE_EXCEPTION("fluid_synth_set_reverb_on")
            break;

        case FluidSynthReverbRoomSize:
        case FluidSynthReverbDamp:
        case FluidSynthReverbLevel:
        case FluidSynthReverbWidth:
            try {
                fluid_synth_set_reverb(fSynth,
                                       fParamBuffers[FluidSynthReverbRoomSize],
                                       fParamBuffers[FluidSynthReverbDamp],
                                       fParamBuffers[FluidSynthReverbWidth],
                                       fParamBuffers[FluidSynthReverbLevel]);
            } CARLA_SAFE_EXCEPTION("fluid_synth_set_reverb")
            break;

        case FluidSynthChorusOnOff:
            try {
                fluid_synth_set_chorus_on(fSynth, (value > 0.5f) ? 1 : 0);
            } CARLA_SAFE_EXCEPTION("fluid_synth_set_chorus_on")
            break;

        case FluidSynthChorusNr:
        case FluidSynthChorusLevel:
        case FluidSynthChorusSpeedHz:
        case FluidSynthChorusDepthMs:
        case FluidSynthChorusType:
            try {
                fluid_synth_set_chorus(fSynth,
                                       static_cast<int>(fParamBuffers[FluidSynthChorusNr] + 0.5f),
                                       fParamBuffers[FluidSynthChorusLevel],
                                       fParamBuffers[FluidSynthChorusSpeedHz],
                                       fParamBuffers[FluidSynthChorusDepthMs],
                                       static_cast<int>(fParamBuffers[FluidSynthChorusType] + 0.5f));
            } CARLA_SAFE_EXCEPTION("fluid_synth_set_chorus")
            break;

        case FluidSynthPolyphony:
            try {
                fluid_synth_set_polyphony(fSynth, static_cast<int>(value + 0.5f));
            } CARLA_SAFE_EXCEPTION("fluid_synth_set_polyphony")
            break;

        case FluidSynthInterpolation:
            for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
            {
                try {
                    fluid_synth_set_interp_method(fSynth, i, static_cast<int>(value + 0.5f));
                } CARLA_SAFE_EXCEPTION_BREAK("fluid_synth_set_interp_method")
            }
            break;

        default:
            break;
        }

        return fixedValue;
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

        water::StringArray midiProgramList(water::StringArray::fromTokens(value, ":", ""));

        if (midiProgramList.size() == MAX_MIDI_CHANNELS)
        {
            uint8_t channel = 0;
            for (water::String *it=midiProgramList.begin(), *end=midiProgramList.end(); it != end; ++it)
            {
                const int index(it->getIntValue());

                if (index >= 0 && index < static_cast<int>(pData->midiprog.count))
                {
                    const uint32_t bank    = pData->midiprog.data[index].bank;
                    const uint32_t program = pData->midiprog.data[index].program;

#if FLUIDSYNTH_VERSION_MAJOR >= 2
                    fluid_synth_program_select(fSynth,
                                               static_cast<int>(channel),
                                               fSynthId,
                                               static_cast<int>(bank),
                                               static_cast<int>(program));
#else
                    fluid_synth_program_select(fSynth, channel, fSynthId, bank, program);
#endif
                    fCurMidiProgs[channel] = index;

                    if (pData->ctrlChannel == static_cast<int32_t>(channel))
                    {
                        pData->midiprog.current = index;
                        pData->engine->callback(true, true,
                                                ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED,
                                                pData->id,
                                                index,
                                                0, 0, 0.0f, nullptr);
                    }
                }

                ++channel;
            }
            CARLA_SAFE_ASSERT(channel == MAX_MIDI_CHANNELS);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback || doingInit,);

        if (index >= 0 && pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
        {
            const uint32_t bank    = pData->midiprog.data[index].bank;
            const uint32_t program = pData->midiprog.data[index].program;

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            try {
#if FLUIDSYNTH_VERSION_MAJOR >= 2
                fluid_synth_program_select(fSynth, pData->ctrlChannel, fSynthId,
                                           static_cast<int>(bank), static_cast<int>(program));
#else
                fluid_synth_program_select(fSynth, pData->ctrlChannel, fSynthId, bank, program);
#endif
            } CARLA_SAFE_EXCEPTION("fluid_synth_program_select")

            fCurMidiProgs[pData->ctrlChannel] = index;
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    // FIXME: this is never used
    void setMidiProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(uindex < pData->midiprog.count,);

        if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
        {
            const uint32_t bank    = pData->midiprog.data[uindex].bank;
            const uint32_t program = pData->midiprog.data[uindex].program;

            try {
#if FLUIDSYNTH_VERSION_MAJOR >= 2
                fluid_synth_program_select(fSynth, pData->ctrlChannel, fSynthId,
                                           static_cast<int>(bank), static_cast<int>(program));
#else
                fluid_synth_program_select(fSynth, pData->ctrlChannel, fSynthId, bank, program);
#endif
            } CARLA_SAFE_EXCEPTION("fluid_synth_program_select")

            fCurMidiProgs[pData->ctrlChannel] = static_cast<int32_t>(uindex);
        }

        CarlaPlugin::setMidiProgramRT(uindex, sendCallbackLater);
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
        String portName;

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

                portName += String((i+2)/2);

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
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMATABLE*/ | PARAMETER_IS_BOOLEAN;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = sFluidDefaults[j];
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 1.0f;

            // ----------------------
            j = FluidSynthReverbRoomSize;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMATABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = sFluidDefaults[j];
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthReverbDamp;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMATABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = sFluidDefaults[j];
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthReverbLevel;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMATABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.data[j].mappedControlIndex = MIDI_CONTROL_REVERB_SEND_LEVEL;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 1.0f;
            pData->param.ranges[j].def = sFluidDefaults[j];
            pData->param.ranges[j].step = 0.01f;
            pData->param.ranges[j].stepSmall = 0.0001f;
            pData->param.ranges[j].stepLarge = 0.1f;

            // ----------------------
            j = FluidSynthReverbWidth;
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED /*| PARAMETER_IS_AUTOMATABLE*/;
            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = j;
            pData->param.ranges[j].min = 0.0f;
            pData->param.ranges[j].max = 10.0f; // should be 100, but that sounds too much
            pData->param.ranges[j].def = sFluidDefaults[j];
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
            pData->param.ranges[j].def = sFluidDefaults[j];
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
            pData->param.ranges[j].def = sFluidDefaults[j];
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
            pData->param.ranges[j].def = sFluidDefaults[j];
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
            pData->param.ranges[j].def = sFluidDefaults[j];
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
#if FLUIDSYNTH_VERSION_MAJOR >= 2
            pData->param.ranges[j].max = 256.0f;
#else
            pData->param.ranges[j].max = float(2048.0 * 1000.0 / pData->engine->getSampleRate()); // FIXME?
#endif
            pData->param.ranges[j].def = sFluidDefaults[j];
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
            pData->param.ranges[j].def = sFluidDefaults[j];
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
            pData->param.ranges[j].def = sFluidDefaults[j];
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
            pData->param.ranges[j].def = sFluidDefaults[j];
            pData->param.ranges[j].step = 1.0f;
            pData->param.ranges[j].stepSmall = 1.0f;
            pData->param.ranges[j].stepLarge = 1.0f;

            // ----------------------
            j = FluidSynthVoiceCount;
            pData->param.data[j].type   = PARAMETER_OUTPUT;
            pData->param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMATABLE | PARAMETER_IS_INTEGER;
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
#if FLUIDSYNTH_VERSION_MAJOR >= 2
            fluid_preset_t* f_preset;

            // initial check to know how many midi-programs we have
            fluid_sfont_iteration_start(f_sfont);
            for (; fluid_sfont_iteration_next(f_sfont);)
                ++count;

            // sound kits must always have at least 1 midi-program
            CARLA_SAFE_ASSERT_RETURN(count > 0,);

            pData->midiprog.createNew(count);

            // Update data
            int tmp;
            uint32_t i = 0;
            fluid_sfont_iteration_start(f_sfont);

            for (; (f_preset = fluid_sfont_iteration_next(f_sfont));)
            {
                CARLA_SAFE_ASSERT_BREAK(i < count);

                tmp = fluid_preset_get_banknum(f_preset);
                pData->midiprog.data[i].bank = (tmp >= 0) ? static_cast<uint32_t>(tmp) : 0;

                tmp = fluid_preset_get_num(f_preset);
                pData->midiprog.data[i].program = (tmp >= 0) ? static_cast<uint32_t>(tmp) : 0;

                pData->midiprog.data[i].name = carla_strdup(fluid_preset_get_name(f_preset));
#else
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
#endif

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

        if (doInit)
        {
            fluid_synth_program_reset(fSynth);

            // select first program, or 128 for ch10
            for (int i=0; i < MAX_MIDI_CHANNELS && i != 9; ++i)
            {
                fluid_synth_set_channel_type(fSynth, i, CHANNEL_TYPE_MELODIC);
#if FLUIDSYNTH_VERSION_MAJOR >= 2
                fluid_synth_program_select(fSynth, i, fSynthId,
                                           static_cast<int>(pData->midiprog.data[0].bank),
                                           static_cast<int>(pData->midiprog.data[0].program));
#else
                fluid_synth_program_select(fSynth, i, fSynthId,
                                           pData->midiprog.data[0].bank, pData->midiprog.data[0].program);
#endif
                fCurMidiProgs[i] = 0;
            }

            if (hasDrums)
            {
                fluid_synth_set_channel_type(fSynth, 9, CHANNEL_TYPE_DRUM);
#if FLUIDSYNTH_VERSION_MAJOR >= 2
                fluid_synth_program_select(fSynth, 9, fSynthId, 128, static_cast<int>(drumProg));
#else
                fluid_synth_program_select(fSynth, 9, fSynthId, 128, drumProg);
#endif
                fCurMidiProgs[9] = static_cast<int32_t>(drumIndex);
            }
            else
            {
                fluid_synth_set_channel_type(fSynth, 9, CHANNEL_TYPE_MELODIC);
#if FLUIDSYNTH_VERSION_MAJOR >= 2
                fluid_synth_program_select(fSynth, 9, fSynthId,
                                           static_cast<int>(pData->midiprog.data[0].bank),
                                           static_cast<int>(pData->midiprog.data[0].program));
#else
                fluid_synth_program_select(fSynth, 9, fSynthId,
                                           pData->midiprog.data[0].bank, pData->midiprog.data[0].program);
#endif
                fCurMidiProgs[9] = 0;
            }

            pData->midiprog.current = 0;
        }
        else
        {
            pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(const float* const* const, float** const audioOut,
                 const float* const*, float**, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
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
                    fluid_synth_all_notes_off(fSynth, i);
                    fluid_synth_all_sounds_off(fSynth, i);
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
                    const ExternalMidiNote& note(it.getValue(kExternalMidiNoteFallback));
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

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < timeOffset)
                {
                    carla_stderr2("Timing error, eventTime:%u < timeOffset:%u for '%s'",
                                  eventTime, timeOffset, pData->name);
                    eventTime = timeOffset;
                }
                else if (eventTime > timeOffset)
                {
                    if (processSingle(audioOut, eventTime - timeOffset, timeOffset))
                    {
                        timeOffset = eventTime;

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
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.normalizedValue/0.5f - 1.0f;

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

                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        for (uint32_t k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].hints != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMATABLE) == 0)
                                continue;

                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            fluid_synth_cc(fSynth, event.channel, ctrlEvent.param, int(ctrlEvent.normalizedValue*127.0f + 0.5f));
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
#if FLUIDSYNTH_VERSION_MAJOR >= 2
                                    fluid_synth_program_select(fSynth, event.channel, fSynthId,
                                                               static_cast<int>(bankId), static_cast<int>(progId));
#else
                                    fluid_synth_program_select(fSynth, event.channel, fSynthId, bankId, progId);
#endif
                                    fCurMidiProgs[event.channel] = static_cast<int32_t>(k);

                                    if (event.channel == pData->ctrlChannel)
                                    {
                                        pData->postponeMidiProgramChangeRtEvent(true, k);
                                    }

                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                            fluid_synth_all_sounds_off(fSynth, event.channel);
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                postponeRtAllNotesOff();
                            }
#endif

                            fluid_synth_all_notes_off(fSynth, event.channel);
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
                    case MIDI_STATUS_NOTE_OFF:
                        if ((pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES) == 0x0)
                        {
                            const uint8_t note = midiEvent.data[1];

                            fluid_synth_noteoff(fSynth, event.channel, note);

                            pData->postponeNoteOffRtEvent(true, event.channel, note);
                        }
                        break;

                    case MIDI_STATUS_NOTE_ON:
                        if ((pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES) == 0x0)
                        {
                            const uint8_t note = midiEvent.data[1];
                            const uint8_t velo = midiEvent.data[2];

                            fluid_synth_noteon(fSynth, event.channel, note, velo);

                            pData->postponeNoteOnRtEvent(true, event.channel, note, velo);
                        }
                        break;

#if FLUIDSYNTH_VERSION_MAJOR >= 2
                    case MIDI_STATUS_POLYPHONIC_AFTERTOUCH:
                        if (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
                        {
                            const uint8_t note     = midiEvent.data[1];
                            const uint8_t pressure = midiEvent.data[2];

                            fluid_synth_key_pressure(fSynth, event.channel, note, pressure);
                        }
                        break;
#endif

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

            if (pData->param.data[k].mappedControlIndex > 0)
            {
                float value(pData->param.ranges[k].getNormalizedValue(fParamBuffers[k]));
                pData->event.portOut->writeControlEvent(0,
                                                        pData->param.data[k].midiChannel,
                                                        kEngineControlEventTypeParameter,
                                                        static_cast<uint16_t>(pData->param.data[k].mappedControlIndex),
                                                        -1,
                                                        value);
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

#ifndef STOAT_TEST_BUILD
        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else
#endif
        if (! pData->singleMutex.tryLock())
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
                carla_zeroFloats(fAudio16Buffers[i], frames);

            // FIXME use '32' or '16' instead of outs
            fluid_synth_process(fSynth, static_cast<int>(frames),
                                0, nullptr,
                                static_cast<int>(pData->audioOut.count), fAudio16Buffers);
        }
        else
        {
            fluid_synth_write_float(fSynth, static_cast<int>(frames),
                                    outBuffer[0] + timeOffset, 0, 1,
                                    outBuffer[1] + timeOffset, 0, 1);
        }

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (volume and balance)

        {
            // note - balance not possible with kUse16Outs, so we can safely skip fAudioOutBuffers
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && carla_isNotEqual(pData->postProc.volume, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));

            float* const oldBufLeft = pData->postProc.extraBuffer;

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        carla_copyFloats(oldBufLeft, outBuffer[i]+timeOffset, frames);

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
        if (kUse16Outs)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                if (fAudio16Buffers[i] != nullptr)
                    delete[] fAudio16Buffers[i];
                fAudio16Buffers[i] = new float[newBufferSize];
            }
        }

        CarlaPlugin::bufferSizeChanged(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_SAFE_ASSERT_RETURN(fSettings != nullptr,);
        fluid_settings_setnum(fSettings, "synth.sample-rate", newSampleRate);

#if FLUIDSYNTH_VERSION_MAJOR < 2
        CARLA_SAFE_ASSERT_RETURN(fSynth != nullptr,);
        fluid_synth_set_sample_rate(fSynth, static_cast<float>(newSampleRate));
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

    bool init(const CarlaPluginPtr plugin,
              const char* const filename, const char* const name, const char* const label, const uint options)
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

        const int synthId = fluid_synth_sfload(fSynth, filename, 0);

        if (synthId < 0)
        {
            pData->engine->setLastError("Failed to load SoundFont file");
            return false;
        }

#if FLUIDSYNTH_VERSION_MAJOR >= 2
        fSynthId = synthId;
#else
        fSynthId = static_cast<uint>(synthId);
#endif


        // ---------------------------------------------------------------
        // get info

        String label2(label);

        if (kUse16Outs && ! label2.endsWith(" (16 outs)"))
            label2 += " (16 outs)";

        fLabel          = carla_strdup(label2);
        pData->filename = carla_strdup(filename);

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(label);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // set options

        pData->options = 0x0;

        if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
            pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
            pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
            pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
        if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
            pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
            pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
#if FLUIDSYNTH_VERSION_MAJOR >= 2
        if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
#endif

        return true;
    }

private:
    void initializeFluidDefaultsIfNeeded()
    {
        if (sFluidDefaultsStored)
            return;

        sFluidDefaultsStored = true;

        // reverb defaults
        sFluidDefaults[FluidSynthReverbOnOff] = 1.0f;
#if FLUIDSYNTH_VERSION_MAJOR >= 2
        double reverbVal;

        reverbVal = 0.2;
        fluid_settings_getnum_default(fSettings, "synth.reverb.room-size", &reverbVal);
        sFluidDefaults[FluidSynthReverbRoomSize] = static_cast<float>(reverbVal);

        reverbVal = 0.0;
        fluid_settings_getnum_default(fSettings, "synth.reverb.damp", &reverbVal);
        sFluidDefaults[FluidSynthReverbDamp] = static_cast<float>(reverbVal);

        reverbVal = 0.9;
        fluid_settings_getnum_default(fSettings, "synth.reverb.level", &reverbVal);
        sFluidDefaults[FluidSynthReverbLevel] = static_cast<float>(reverbVal);

        reverbVal = 0.5;
        fluid_settings_getnum_default(fSettings, "synth.reverb.width", &reverbVal);
        sFluidDefaults[FluidSynthReverbWidth] = static_cast<float>(reverbVal);
#else
        sFluidDefaults[FluidSynthReverbRoomSize] = FLUID_REVERB_DEFAULT_ROOMSIZE;
        sFluidDefaults[FluidSynthReverbDamp] = FLUID_REVERB_DEFAULT_DAMP;
        sFluidDefaults[FluidSynthReverbLevel] = FLUID_REVERB_DEFAULT_LEVEL;
        sFluidDefaults[FluidSynthReverbWidth] = FLUID_REVERB_DEFAULT_WIDTH;
#endif

        // chorus defaults
        sFluidDefaults[FluidSynthChorusOnOff] = 1.0f;
#if FLUIDSYNTH_VERSION_MAJOR >= 2
        double chorusVal;

        chorusVal = 3.0;
        fluid_settings_getnum_default(fSettings, "synth.chorus.nr", &chorusVal);
        sFluidDefaults[FluidSynthChorusNr] = static_cast<float>(chorusVal);

        chorusVal = 2.0;
        fluid_settings_getnum_default(fSettings, "synth.chorus.level", &chorusVal);
        sFluidDefaults[FluidSynthChorusLevel] = static_cast<float>(chorusVal);

        chorusVal = 0.3;
        fluid_settings_getnum_default(fSettings, "synth.chorus.speed", &chorusVal);
        sFluidDefaults[FluidSynthChorusSpeedHz] = static_cast<float>(chorusVal);

        chorusVal = 8.0;
        fluid_settings_getnum_default(fSettings, "synth.chorus.depth", &chorusVal);
        sFluidDefaults[FluidSynthChorusDepthMs] = static_cast<float>(chorusVal);

        // There is no settings for chorus default type
        sFluidDefaults[FluidSynthChorusType] = static_cast<float>(fluid_synth_get_chorus_type(fSynth));
#else
        sFluidDefaults[FluidSynthChorusNr] = FLUID_CHORUS_DEFAULT_N;
        sFluidDefaults[FluidSynthChorusLevel] = FLUID_CHORUS_DEFAULT_LEVEL;
        sFluidDefaults[FluidSynthChorusSpeedHz] = FLUID_CHORUS_DEFAULT_SPEED;
        sFluidDefaults[FluidSynthChorusDepthMs] = FLUID_CHORUS_DEFAULT_DEPTH;
        sFluidDefaults[FluidSynthChorusType] = FLUID_CHORUS_DEFAULT_TYPE;
#endif

        // misc. defaults
        sFluidDefaults[FluidSynthInterpolation] = FLUID_INTERP_DEFAULT;
        sFluidDefaults[FluidSynthPolyphony] = FLUID_DEFAULT_POLYPHONY;
    }

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
#if FLUIDSYNTH_VERSION_MAJOR >= 2
    int fSynthId;
#else
    uint fSynthId;
#endif

    float** fAudio16Buffers;
    float   fParamBuffers[FluidSynthParametersMax];

    static bool sFluidDefaultsStored;
    static float sFluidDefaults[FluidSynthParametersMax];

    int32_t fCurMidiProgs[MAX_MIDI_CHANNELS];

    const char* fLabel;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginFluidSynth)
};

bool CarlaPluginFluidSynth::sFluidDefaultsStored = false;
float CarlaPluginFluidSynth::sFluidDefaults[FluidSynthParametersMax] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_FLUIDSYNTH

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newFluidSynth(const Initializer& init, PluginType ptype, bool use16Outs)
{
    carla_debug("CarlaPlugin::newFluidSynth({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "}, %s)",
                init.engine, init.filename, init.name, init.label, init.uniqueId, bool2str(use16Outs));

#ifdef HAVE_FLUIDSYNTH
    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        use16Outs = false;

    if (ptype == PLUGIN_SF2 && ! fluid_is_soundfont(init.filename))
    {
        init.engine->setLastError("Requested file is not a valid SoundFont");
        return nullptr;
    }
#ifndef HAVE_FLUIDSYNTH_INSTPATCH
    if (ptype == PLUGIN_DLS)
    {
        init.engine->setLastError("DLS file support not available");
        return nullptr;
    }
    if (ptype == PLUGIN_GIG)
    {
        init.engine->setLastError("GIG file support not available");
        return nullptr;
    }
#endif

    std::shared_ptr<CarlaPluginFluidSynth> plugin(new CarlaPluginFluidSynth(init.engine, init.id,  use16Outs));

    if (! plugin->init(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
#else
    init.engine->setLastError("fluidsynth support not available");
    return nullptr;

    // unused
    (void)ptype;
    (void)use16Outs;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
