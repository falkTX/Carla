/*
 * Carla CLAP Plugin
 * Copyright (C) 2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBackendUtils.hpp"
#include "CarlaClapUtils.hpp"
#include "CarlaMathUtils.hpp"

#include "CarlaPluginUI.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# import <Foundation/Foundation.h>
#endif

#include "water/files/File.h"

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

static_assert(kPluginMaxMidiEvents > MAX_MIDI_NOTE, "Enough space for input events");

// --------------------------------------------------------------------------------------------------------------------

struct carla_clap_host : clap_host_t {
    carla_clap_host()
    {
        clap_version = CLAP_VERSION;
        host_data = this;
        name = "Carla";
        vendor = "falkTX";
        url = "https://kx.studio/carla";
        version = CARLA_VERSION_STRING;
        get_extension = carla_get_extension;
        request_restart = carla_request_restart;
        request_process = carla_request_process;
        request_callback = carla_request_callback;
    }

    static const void* carla_get_extension(const clap_host_t*, const char*) { return nullptr; }
    static void carla_request_restart(const clap_host_t*) {}
    static void carla_request_process(const clap_host_t*) {}
    static void carla_request_callback(const clap_host_t*) {}
};

struct carla_clap_input_events : clap_input_events_t {
    union Event {
        clap_event_header_t header;
        clap_event_param_value_t param;
        clap_event_param_gesture_t gesture;
        clap_event_midi_t midi;
        clap_event_midi_sysex_t sysex;
    };

    struct UpdatedParam {
        bool updated;
        double value;
        clap_id clapId;
        void* cookie;

        UpdatedParam()
          : updated(false),
            value(0.f),
            clapId(0),
            cookie(0) {}
    };

    Event* events;
    UpdatedParam* updatedParams;

    uint32_t numEventsAllocated;
    uint32_t numEventsUsed;
    uint32_t numParams;

    carla_clap_input_events()
        : events(nullptr),
          updatedParams(nullptr),
          numEventsAllocated(0),
          numEventsUsed(0),
          numParams(0)
    {
        ctx = this;
        size = carla_size;
        get = carla_get;
    }

    ~carla_clap_input_events()
    {
        delete[] events;
        delete[] updatedParams;
    }

    // called on plugin reload
    // NOTE: clapId and cookie must be separately set outside this function
    void init(const uint32_t paramCount)
    {
        numEventsUsed = 0;
        numParams = paramCount;
        delete[] events;
        delete[] updatedParams;

        if (paramCount != 0)
        {
            numEventsAllocated = paramCount + kPluginMaxMidiEvents;
            events = new Event[numEventsAllocated];
            updatedParams = new UpdatedParam[paramCount];
        }
        else
        {
            numEventsAllocated = 0;
            events = nullptr;
            updatedParams = nullptr;
        }
    }

    // called just before plugin processing
    void prepare()
    {
        uint32_t count = 0;

        for (uint32_t i=0; i<numParams; ++i)
        {
            if (updatedParams[i].updated)
            {
                events[count++].param = {
                    { sizeof(clap_event_param_value_t), 0, 0, CLAP_EVENT_PARAM_VALUE, 0 },
                    updatedParams[i].clapId,
                    updatedParams[i].cookie,
                    -1, -1, -1, -1,
                    updatedParams[i].value
                };
            }
        }

        numEventsUsed = count;
    }

    // called when a parameter is set from non-rt thread
    void setParamValue(const uint32_t index, const double value) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(index < numParams,);

        updatedParams[index].value = value;
        updatedParams[index].updated = true;
    }

    static uint32_t carla_size(const clap_input_events_t* const list) noexcept
    {
        return static_cast<const carla_clap_input_events*>(list->ctx)->numEventsUsed;
    }

    static const clap_event_header_t* carla_get(const clap_input_events_t* const list, const uint32_t index) noexcept
    {
        return &static_cast<const carla_clap_input_events*>(list->ctx)->events[index].header;
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginCLAP : public CarlaPlugin,
                        private CarlaPluginUI::Callback
{
public:
    CarlaPluginCLAP(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fPlugin(nullptr),
          fPluginDescriptor(nullptr),
          fPluginEntry(nullptr),
          fHost(),
          fExtensions(),
          fInputEvents()
    {
        carla_debug("CarlaPluginCLAP::CarlaPluginCLAP(%p, %i)", engine, id);

    }

    ~CarlaPluginCLAP() override
    {
        carla_debug("CarlaPluginCLAP::~CarlaPluginCLAP()");

        // close UI
//         if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
//         {
//             if (! fUI.isEmbed)
//                 showCustomUI(false);
//
//             if (fUI.isOpen)
//             {
//                 fUI.isOpen = false;
//                 dispatcher(effEditClose);
//             }
//         }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

//         CARLA_ASSERT(! fIsProcessing);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fPlugin != nullptr)
        {
            fPlugin->destroy(fPlugin);
            fPlugin = nullptr;
        }

        clearBuffers();

        if (fPluginEntry != nullptr)
        {
            fPluginEntry->deinit();
            fPluginEntry = nullptr;
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_CLAP;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, PLUGIN_CATEGORY_NONE);

        if (fPluginDescriptor->features == nullptr)
            return PLUGIN_CATEGORY_NONE;

        return getPluginCategoryFromClapFeatures(fPluginDescriptor->features);
    }

    /*
    uint32_t getLatencyInFrames() const noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    /*
    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    /*
    uint getOptionsAvailable() const noexcept override
    {
    }
    */

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0.f);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params != nullptr, 0.f);

        const clap_id clapId = pData->param.data[parameterId].rindex;

        double value;
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params->get_value(fPlugin, clapId, &value), 0.f);

        return value;
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->id, STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->vendor, STR_MAX);
        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        return getMaker(strBuf);
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPluginDescriptor != nullptr, false);

        std::strncpy(strBuf, fPluginDescriptor->name, STR_MAX);
        return true;
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const clap_id clapId = pData->param.data[parameterId].rindex;

        clap_param_info_t paramInfo = {};
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params->get_info(fPlugin, clapId, &paramInfo), false);

        std::strncpy(strBuf, paramInfo.name, STR_MAX);
        return true;
    }

    bool getParameterText(const uint32_t parameterId, char* const strBuf) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const clap_id clapId = pData->param.data[parameterId].rindex;

        double value;
        CARLA_SAFE_ASSERT_RETURN(fExtensions.params->get_value(fPlugin, clapId, &value), false);

        return fExtensions.params->value_to_text(fPlugin, clapId, value, strBuf, STR_MAX);
    }

    /*
    bool getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
    }

    bool getParameterGroupName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    /*
    void setName(const char* const newName) override
    {
    }
    */

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue = pData->param.getFixedValue(parameterId, value);

        fInputEvents.setParamValue(parameterId, fixedValue);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue = pData->param.getFixedValue(parameterId, value);

        fInputEvents.setParamValue(parameterId, fixedValue);

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    /*
    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
    }

    void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
    }

    void setProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept override
    {
    }
    */

    // -------------------------------------------------------------------
    // Set ui stuff

    /*
    void setCustomUITitle(const char* const title) noexcept override
    {
    }

    void showCustomUI(const bool yesNo) override
    {
    }

    void* embedCustomUI(void* const ptr) override
    {
    }
    */

    void idle() override
    {
        CarlaPlugin::idle();
    }

    void uiIdle() override
    {
        CarlaPlugin::uiIdle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        carla_debug("CarlaPluginCLAP::reload() - start");

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const clap_plugin_audio_ports_t* audioPortsExt = static_cast<const clap_plugin_audio_ports_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_AUDIO_PORTS));

        const clap_plugin_note_ports_t* notePortsExt = static_cast<const clap_plugin_note_ports_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_NOTE_PORTS));

        const clap_plugin_params_t* paramsExt = static_cast<const clap_plugin_params_t*>(
            fPlugin->get_extension(fPlugin, CLAP_EXT_PARAMS));

        if (audioPortsExt != nullptr && (audioPortsExt->count == nullptr || audioPortsExt->get == nullptr))
            audioPortsExt = nullptr;

        if (notePortsExt != nullptr && (notePortsExt->count == nullptr || notePortsExt->get == nullptr))
            notePortsExt = nullptr;

        if (paramsExt != nullptr && (paramsExt->count == nullptr || paramsExt->get_info == nullptr))
            paramsExt = nullptr;

        fExtensions.params = paramsExt;

        const uint32_t numAudioInputPorts = audioPortsExt != nullptr ? audioPortsExt->count(fPlugin, true) : 0;
        const uint32_t numAudioOutputPorts = audioPortsExt != nullptr ? audioPortsExt->count(fPlugin, false) : 0;
        const uint32_t numNoteInputPorts = notePortsExt != nullptr ? notePortsExt->count(fPlugin, true) : 0;
        const uint32_t numNoteOutputPorts = notePortsExt != nullptr ? notePortsExt->count(fPlugin, true) : 0;
        const uint32_t numParameters = paramsExt != nullptr ? paramsExt->count(fPlugin) : 0;

        uint32_t aIns, aOuts, params;
        aIns = aOuts = params = 0;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        for (uint32_t i=0; i<numAudioInputPorts; ++i)
        {
            clap_audio_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(audioPortsExt->get(fPlugin, i, true, &portInfo));

            aIns += portInfo.channel_count;
        }

        for (uint32_t i=0; i<numAudioOutputPorts; ++i)
        {
            clap_audio_port_info_t portInfo = {};
            CARLA_SAFE_ASSERT_BREAK(audioPortsExt->get(fPlugin, i, false, &portInfo));

            aOuts += portInfo.channel_count;
        }

        for (uint32_t i=0; i<numParameters; ++i)
        {
            clap_param_info_t paramInfo = {};
            CARLA_SAFE_ASSERT_BREAK(paramsExt->get_info(fPlugin, i, &paramInfo));

            if ((paramInfo.flags & (CLAP_PARAM_IS_HIDDEN|CLAP_PARAM_IS_BYPASS)) == 0x0)
                ++params;
        }

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            needsCtrlIn = true;
        }

        if (numNoteInputPorts > 0)
            needsCtrlIn = true;

        if (numNoteOutputPorts > 0)
            needsCtrlOut = true;

        if (params > 0)
        {
            pData->param.createNew(params, false);
            needsCtrlIn = true;
        }

        fInputEvents.init(params);

        const EngineProcessMode processMode = pData->engine->getProccessMode();
        const uint portNameSize = pData->engine->getMaxPortNameSize();
        CarlaString portName;

        // Audio Ins
        for (uint32_t j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aIns > 1)
            {
                portName += "input_";
                portName += CarlaString(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aOuts > 1)
            {
                portName += "output_";
                portName += CarlaString(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        for (uint32_t j=0; j < params; ++j)
        {
            clap_param_info_t paramInfo = {};
            CARLA_SAFE_ASSERT_BREAK(paramsExt->get_info(fPlugin, j, &paramInfo));

            if (paramInfo.flags & (CLAP_PARAM_IS_HIDDEN|CLAP_PARAM_IS_BYPASS))
                continue;

            pData->param.data[j].index  = j;
            pData->param.data[j].rindex = paramInfo.id;

            double min, max, def, step, stepSmall, stepLarge;

            min = paramInfo.min_value;
            max = paramInfo.max_value;
            def = paramInfo.default_value;

            if (min >= max)
                max = min + 0.1;

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            if (paramInfo.flags & CLAP_PARAM_IS_READONLY)
                pData->param.data[j].type = PARAMETER_OUTPUT;
            else
                pData->param.data[j].type = PARAMETER_INPUT;

            if (paramInfo.flags & CLAP_PARAM_IS_STEPPED)
            {
                if (carla_isEqual(max - min, 1.0))
                {
                    step = stepSmall = stepLarge = 1.0;
                    pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else
                {
                    step = 1.0;
                    stepSmall = 1.0;
                    stepLarge = std::min(max - min, 10.0);
                }
                pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
            }
            else
            {
                double range = max - min;
                step = range/100.0;
                stepSmall = range/1000.0;
                stepLarge = range/10.0;
            }

            pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
            pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;

            if (paramInfo.flags & CLAP_PARAM_IS_AUTOMATABLE)
            {
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;

                if ((paramInfo.flags & CLAP_PARAM_IS_STEPPED) == 0x0)
                    pData->param.data[j].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
            }

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;

            fInputEvents.updatedParams[j].clapId = paramInfo.id;
            fInputEvents.updatedParams[j].cookie = paramInfo.cookie;
        }

        if (needsCtrlIn)
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
           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
           #endif
        }

        if (needsCtrlOut)
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

        // plugin hints
        const PluginCategory category = fPluginDescriptor->features != nullptr ? getPluginCategoryFromClapFeatures(fPluginDescriptor->features)
                                                                               : PLUGIN_CATEGORY_NONE;

        pData->hints = 0x0;

        if (category == PLUGIN_CATEGORY_SYNTH)
            pData->hints |= PLUGIN_IS_SYNTH;

       #ifdef CLAP_WINDOW_API_NATIVE
        if (const clap_plugin_gui_t* const guiExt = static_cast<const clap_plugin_gui_t*>(fPlugin->get_extension(fPlugin, CLAP_EXT_GUI)))
        {
            if (guiExt->is_api_supported != nullptr)
            {
                if (guiExt->is_api_supported(fPlugin, CLAP_WINDOW_API_NATIVE, false))
                {
                    pData->hints |= PLUGIN_HAS_CUSTOM_UI;
                    pData->hints |= PLUGIN_HAS_CUSTOM_EMBED_UI;
                }
                else if (guiExt->is_api_supported(fPlugin, CLAP_WINDOW_API_NATIVE, false))
                {
                    pData->hints |= PLUGIN_HAS_CUSTOM_UI;
                }
            }
        }
       #endif

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (numNoteInputPorts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (numNoteOutputPorts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginCLAP::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginCLAP::reloadPrograms(%s)", bool2str(doInit));
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        // FIXME check return status
        fPlugin->activate(fPlugin, pData->engine->getSampleRate(), 1, pData->engine->getBufferSize());
        fPlugin->start_processing(fPlugin);
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        // FIXME check return status
        fPlugin->stop_processing(fPlugin);
        fPlugin->deactivate(fPlugin);
    }

    void process(const float* const* const audioIn,
                 float** const audioOut,
                 const float* const* const cvIn,
                 float** const,
                 const uint32_t frames) override
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

        fInputEvents.prepare();

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (uint8_t i=0, k=fInputEvents.numEventsUsed; i < MAX_MIDI_CHANNELS; ++i)
                {
                    fInputEvents.events[k + i].midi = {
                        { sizeof(clap_event_midi_t), 0, 0, CLAP_EVENT_MIDI, 0 },
                        0, // TODO multi-port MIDI
                        { uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT)), MIDI_CONTROL_ALL_NOTES_OFF, 0 }
                    };
                    fInputEvents.events[k + MAX_MIDI_CHANNELS + i].midi = {
                        { sizeof(clap_event_midi_t), 0, 0, CLAP_EVENT_MIDI, 0 },
                        0, // TODO multi-port MIDI
                        { uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT)), MIDI_CONTROL_ALL_SOUND_OFF, 0 }
                    };
                }

                fInputEvents.numEventsUsed += MAX_MIDI_CHANNELS * 2;
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                for (uint8_t i=0, k=fInputEvents.numEventsUsed; i < MAX_MIDI_NOTE; ++i)
                {
                    fInputEvents.events[k + i].midi = {
                        { sizeof(clap_event_midi_t), 0, 0, CLAP_EVENT_MIDI, 0 },
                        0, // TODO multi-port MIDI
                        { uint8_t(MIDI_STATUS_NOTE_OFF | (pData->ctrlChannel & MIDI_CHANNEL_BIT)), i, 0 }
                    };
                }

                fInputEvents.numEventsUsed += MAX_MIDI_NOTE;
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());

        clap_event_transport_t clapTransport = {
            { sizeof(clap_event_transport_t), 0, 0, CLAP_EVENT_TRANSPORT, 0 },
            0x0, // flags
            0, // song_pos_beats, position in beats
            0, // song_pos_seconds, position in seconds
            0.0, // tempo, in bpm
            0.0, // tempo_inc, tempo increment for each samples and until the next time info event
            0, // loop_start_beats;
            0, // loop_end_beats;
            0, // loop_start_seconds;
            0, // loop_end_seconds;
            0, // bar_start, start pos of the current bar
            0, // bar_number, bar at song pos 0 has the number 0
            0, // tsig_num, time signature numerator
            0, // tsig_denom, time signature denominator
        };

        if (timeInfo.playing)
            clapTransport.flags |= CLAP_TRANSPORT_IS_PLAYING;

        // TODO song_pos_seconds (based on frame and sample rate)

        if (timeInfo.bbt.valid)
        {
            // TODO song_pos_beats

            // Tempo
            clapTransport.tempo  = timeInfo.bbt.beatsPerMinute;
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TEMPO;

            // Bar
            // TODO bar_start
            clapTransport.bar_number = timeInfo.bbt.bar - 1;

            // Time Signature
            clapTransport.tsig_num = static_cast<uint16_t>(timeInfo.bbt.beatsPerBar + 0.5f);
            clapTransport.tsig_denom = static_cast<uint16_t>(timeInfo.bbt.beatType + 0.5f);
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TIME_SIGNATURE;
        }
        else
        {
            // Tempo
            clapTransport.tempo = 120.0;
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TEMPO;

            // Time Signature
            clapTransport.tsig_num = 4;
            clapTransport.tsig_denom = 4;
            clapTransport.flags |= CLAP_TRANSPORT_HAS_TIME_SIGNATURE;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                ExternalMidiNote note = { 0, 0, 0 };

                for (; fInputEvents.numEventsUsed < fInputEvents.numEventsAllocated && ! pData->extNotes.data.isEmpty();)
                {
                    note = pData->extNotes.data.getFirst(note, true);

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    fInputEvents.events[fInputEvents.numEventsUsed++].midi = {
                        { sizeof(clap_event_midi_t), 0, 0, CLAP_EVENT_MIDI, CLAP_EVENT_IS_LIVE },
                        0, // TODO multi-port MIDI
                        {
                            uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT)),
                            note.note,
                            note.velo
                        }
                    };
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, frames - timeOffset, timeOffset, &clapTransport);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, frames, 0, &clapTransport);

        } // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {

        } // End of MIDI Output

        // --------------------------------------------------------------------------------------------------------

#ifdef BUILD_BRIDGE_ALTERNATIVE_ARCH
        return;

        // unused
        (void)cvIn;
#endif
    }

    bool processSingle(const float* const* const inBuffer,
                       float** const outBuffer,
                       const uint32_t frames,
                       const uint32_t timeOffset,
                       clap_event_transport_t* const transport)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
        }

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
        // Run plugin

        const clap_audio_buffer_const_t inBuffers[1] = {
            {
                inBuffer, // data32
                nullptr, // data64
                1, // channel_count
                0, // latency
                0  // constant_mask;
            }
        };
        clap_audio_buffer_t outBuffers[1] = {
            {
                outBuffer, // data32
                nullptr, // data64
                1, // channel_count
                0, // latency
                0  // constant_mask;
            }
        };
        const clap_process_t process = {
            0, // steady_time
            frames,
            transport,
            static_cast<const clap_audio_buffer_t*>(static_cast<const void*>(inBuffers)), // audio_inputs
            outBuffers, // audio_outputs
            1, // audio_inputs_count
            1, // audio_outputs_count
            &fInputEvents, // in_events
            nullptr  // out_events
        };

        fPlugin->process(fPlugin, &process);

        fInputEvents.numEventsUsed = 0;

        // TODO update transport

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doVolume  = (pData->hints & PLUGIN_CAN_VOLUME) != 0 && carla_isNotEqual(pData->postProc.volume, 1.0f);
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    const uint32_t c = isMono ? 0 : i;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue = inBuffer[c][k];
                        outBuffer[i][k] = (outBuffer[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        carla_copyFloats(oldBufLeft, outBuffer[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            outBuffer[i][k]  = oldBufLeft[k]     * (1.0f - balRangeL);
                            outBuffer[i][k] += outBuffer[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k] * balRangeR;
                            outBuffer[i][k] += oldBufLeft[k]   * balRangeL;
                        }
                    }
                }

                // Volume
                if (doVolume)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing
       #endif // BUILD_BRIDGE_ALTERNATIVE_ARCH

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginCLAP::bufferSizeChanged(%i)", newBufferSize);

        if (pData->active)
            deactivate();

        if (pData->active)
            activate();
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginCLAP::sampleRateChanged(%g)", newSampleRate);

        if (pData->active)
            deactivate();

        if (pData->active)
            activate();
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginCLAP::clearBuffers() - start");

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginCLAP::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
    void handlePluginUIClosed() override
    {
        // CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginCLAP::handlePluginUIClosed()");

        showCustomUI(false);
        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_UI_STATE_CHANGED,
                                pData->id,
                                0,
                                0, 0, 0.0f, nullptr);
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        // CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginCLAP::handlePluginUIResized(%u, %u)", width, height);
    }

    // -------------------------------------------------------------------

public:
    bool init(const CarlaPluginPtr plugin,
              const char* const filename, const char* const name, const char* const id, const uint options)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        if (id == nullptr || id[0] == '\0')
        {
            pData->engine->setLastError("null label/id");
            return false;
        }

        // ---------------------------------------------------------------

        const clap_plugin_entry_t* entry;

       #ifdef CARLA_OS_MAC
        if (!water::File(filename).existsAsFile())
        {
            if (! fBundleLoader.load(filename))
            {
                pData->engine->setLastError("Failed to load CLAP bundle executable");
                return false;
            }

            entry = fBundleLoader.getSymbol<const clap_plugin_entry_t*>(CFSTR("clap_entry"));
        }
        else
       #endif
        {
            if (! pData->libOpen(filename))
            {
                pData->engine->setLastError(pData->libError(filename));
                return false;
            }

            entry = pData->libSymbol<const clap_plugin_entry_t*>("clap_entry");
        }

        if (entry == nullptr)
        {
            pData->engine->setLastError("Could not find the CLAP entry in the plugin library");
            return false;
        }

        if (entry->init == nullptr || entry->deinit == nullptr || entry->get_factory == nullptr)
        {
            pData->engine->setLastError("CLAP factory entries are null");
            return false;
        }

        if (!clap_version_is_compatible(entry->clap_version))
        {
            pData->engine->setLastError("Incompatible CLAP plugin");
            return false;
        }

        // ---------------------------------------------------------------

        const water::String pluginPath(water::File(filename).getParentDirectory().getFullPathName());

        if (!entry->init(pluginPath.toRawUTF8()))
        {
            pData->engine->setLastError("Plugin entry failed to initialize");
            return false;
        }

        fPluginEntry = entry;

        // ---------------------------------------------------------------

        const clap_plugin_factory_t* const factory = static_cast<const clap_plugin_factory_t*>(
            entry->get_factory(CLAP_PLUGIN_FACTORY_ID));

        if (factory == nullptr
            || factory->get_plugin_count == nullptr
            || factory->get_plugin_descriptor == nullptr
            || factory->create_plugin == nullptr)
        {
            pData->engine->setLastError("Plugin is missing factory methods");
            return false;
        }

        // ---------------------------------------------------------------

        if (const uint32_t count = factory->get_plugin_count(factory))
        {
            for (uint32_t i=0; i<count; ++i)
            {
                const clap_plugin_descriptor_t* const desc = factory->get_plugin_descriptor(factory, i);
                CARLA_SAFE_ASSERT_CONTINUE(desc != nullptr);
                CARLA_SAFE_ASSERT_CONTINUE(desc->id != nullptr);

                if (std::strcmp(desc->id, id) == 0)
                {
                    fPluginDescriptor = desc;
                    break;
                }
            }
        }
        else
        {
            pData->engine->setLastError("Plugin library contains no plugins");
            return false;
        }

        if (fPluginDescriptor == nullptr)
        {
            pData->engine->setLastError("Plugin library does not contain the requested plugin");
            return false;
        }

        // ---------------------------------------------------------------

        fPlugin = factory->create_plugin(factory, &fHost, fPluginDescriptor->id);

        if (fPlugin == nullptr)
        {
            pData->engine->setLastError("Failed to create CLAP plugin instance");
            return false;
        }

        if (!fPlugin->init(fPlugin))
        {
            pData->engine->setLastError("Failed to initialize CLAP plugin instance");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        pData->name = pData->engine->getUniquePluginName(name != nullptr && name[0] != '\0'
                                                         ? name
                                                         : fPluginDescriptor->name);
        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // set default options

        pData->options = 0x0;

//         if (fEffect->initialDelay > 0 || hasMidiOutput() || isPluginOptionEnabled(options, PLUGIN_OPTION_FIXED_BUFFERS))
//             pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
//
//         if (fEffect->flags & effFlagsProgramChunks)
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_USE_CHUNKS))
//                 pData->options |= PLUGIN_OPTION_USE_CHUNKS;
//
//         if (hasMidiInput())
//         {
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
//                 pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
//                 pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
//                 pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
//                 pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
//                 pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PROGRAM_CHANGES))
//                 pData->options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
//             if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
//                 pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
//         }
//
//         if (fEffect->numPrograms > 1 && (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) == 0)
//             if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
//                 pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        return true;
    }

private:
    const clap_plugin_t* fPlugin;
    const clap_plugin_descriptor_t* fPluginDescriptor;
    const clap_plugin_entry_t* fPluginEntry;
    const carla_clap_host fHost;

    struct Extensions {
        const clap_plugin_params_t* params;

        Extensions()
            : params(nullptr) {}

        CARLA_DECLARE_NON_COPYABLE(Extensions)
    } fExtensions;

    carla_clap_input_events fInputEvents;

   #ifdef CARLA_OS_MAC
    BundleLoader fBundleLoader;
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newCLAP(const Initializer& init)
{
    carla_debug("CarlaPlugin::newCLAP({%p, \"%s\", \"%s\", \"%s\"})",
                init.engine, init.filename, init.name, init.label);

    std::shared_ptr<CarlaPluginCLAP> plugin(new CarlaPluginCLAP(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
