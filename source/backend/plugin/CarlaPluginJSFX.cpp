// SPDX-FileCopyrightText: 2021 Jean Pierre Cimalando
// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// TODO(jsfx) graphics section

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef HAVE_YSFX

#include "CarlaJsfxUtils.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaUtils.hpp"

#include "water/files/File.h"
#include "water/text/StringArray.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

using water::CharPointer_UTF8;
using water::File;

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginJSFX : public CarlaPlugin
{
public:
    CarlaPluginJSFX(CarlaEngine* const engine, const uint id) noexcept
        : CarlaPlugin(engine, id),
          fEffect(nullptr),
          fEffectState(nullptr),
          fUnit(),
          fChunkText(),
          fTransportValues(),
          fMapOfSliderToParameter(ysfx_max_sliders, -1)
    {
        carla_debug("CarlaPluginJSFX::CarlaPluginJSFX(%p, %i)", engine, id);

        carla_zeroStruct(fTransportValues);
    }

    ~CarlaPluginJSFX() noexcept override
    {
        carla_debug("CarlaPluginJSFX::~CarlaPluginJSFX()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        clearBuffers();

        ysfx_state_free(fEffectState);
        ysfx_free(fEffect);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_JSFX;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, CarlaPlugin::getCategory());

        return CarlaJsfxCategories::getFromEffect(fEffect);
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, 0);

        ysfx_real sampleRate = ysfx_get_sample_rate(fEffect);
        ysfx_real latencyInSeconds = ysfx_get_pdc_delay(fEffect);

        //NOTE: `pdc_bot_ch` and `pdc_top_ch` channel range ignored

        int32_t latencyInFrames = water::roundToInt(latencyInSeconds * sampleRate);
        wassert(latencyInFrames >= 0);

        return (uint32_t)latencyInFrames;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        return 1;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        return 1;
    }

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        const uint32_t rindex = static_cast<uint32_t>(pData->param.data[parameterId].rindex);
        return ysfx_slider_get_enum_names(fEffect, rindex, nullptr, 0);;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        ysfx_state_free(fEffectState);
        fEffectState = ysfx_save_state(fEffect);
        CARLA_SAFE_ASSERT_RETURN(fEffectState != nullptr, 0);

        *dataPtr = fEffectState->data;
        return fEffectState->data_size;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        options |= PLUGIN_OPTION_USE_CHUNKS;

        options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
        options |= PLUGIN_OPTION_SEND_PITCHBEND;
        options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
        options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        const uint32_t rindex = static_cast<uint32_t>(pData->param.data[parameterId].rindex);
        return static_cast<float>(ysfx_slider_get_value(fEffect, rindex));
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const uint32_t rindex = static_cast<uint32_t>(pData->param.data[parameterId].rindex);

        if (const char* const name = ysfx_slider_get_name(fEffect, rindex))
        {
            std::snprintf(strBuf, STR_MAX, "%s", name);
            return true;
        }

        return false;
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), 0.0f);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId), 0.0f);
        return (float)scalePointId;
    }

    bool getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);

        const uint32_t rindex = static_cast<uint32_t>(pData->param.data[parameterId].rindex);

        const uint32_t enumCount = ysfx_slider_get_enum_names(fEffect, rindex, nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < enumCount, false);

        if (const char* const name = ysfx_slider_get_enum_name(fEffect, rindex, scalePointId))
        {
            std::snprintf(strBuf, STR_MAX, "%s", name);
            return true;
        }

        return false;
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fUnit.getFileId(), STR_MAX);
        return true;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const uint32_t rindex = static_cast<uint32_t>(pData->param.data[parameterId].rindex);
        ysfx_slider_set_value(fEffect, rindex, value);

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const uint32_t rindex = static_cast<uint32_t>(pData->param.data[parameterId].rindex);
        ysfx_slider_set_value(fEffect, rindex, value);

        CarlaPlugin::setParameterValueRT(parameterId, value, frameOffset, sendCallbackLater);
    }

    void setChunkData(const void* data, std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);

        ysfx_state_t state;
        state.sliders = nullptr;
        state.slider_count = 0;
        state.data = static_cast<uint8_t*>(const_cast<void*>(data));
        state.data_size = dataSize;

        CARLA_SAFE_ASSERT(ysfx_load_state(fEffect, &state));
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        carla_debug("CarlaPluginJSFX::reload()");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        // ---------------------------------------------------------------

        // initialize the block size and sample rate
        // loading the chunk can invoke @slider which makes computations based on these
        ysfx_set_sample_rate(fEffect, pData->engine->getSampleRate());
        ysfx_set_block_size(fEffect, (uint32_t)pData->engine->getBufferSize());
        ysfx_init(fEffect);

        const uint32_t aIns = ysfx_get_num_inputs(fEffect);
        const uint32_t aOuts = ysfx_get_num_outputs(fEffect);

        // perhaps we obtained a latency value from @init
        pData->client->setLatency(getLatencyInFrames());

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
        }

        // count the sliders and establish the mappings between parameter and slider
        uint32_t params = 0;
        uint32_t mapOfParameterToSlider[ysfx_max_sliders];
        for (uint32_t rindex = 0; rindex < ysfx_max_sliders; ++rindex)
        {
            if (ysfx_slider_exists(fEffect, rindex))
            {
                mapOfParameterToSlider[params] = rindex;
                fMapOfSliderToParameter[rindex] = (int32_t)params;
                ++params;
            }
            else
            {
                fMapOfSliderToParameter[rindex] = -1;
            }
        }

        if (params > 0)
        {
            pData->param.createNew(params, false);
        }

        const uint portNameSize = pData->engine->getMaxPortNameSize();
        String portName;

        // Audio Ins
        for (uint32_t j = 0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            const char* const inputName = ysfx_get_input_name(fEffect, j);
            if (inputName && inputName[0])
            {
                portName += inputName;
            }
            else if (aIns > 1)
            {
                portName += "input_";
                portName += String(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j = 0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            const char* const outputName = ysfx_get_input_name(fEffect, j);
            if (outputName && outputName[0])
            {
                portName += outputName;
            }
            else if (aOuts > 1)
            {
                portName += "output_";
                portName += String(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        // Parameters
        for (uint32_t j = 0; j < params; ++j)
        {
            const uint32_t rindex = mapOfParameterToSlider[j];
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].index  = (int32_t)j;
            pData->param.data[j].rindex = (int32_t)rindex;

            ysfx_slider_range_t range = {};
            ysfx_slider_get_range(fEffect, rindex, &range);

            float min = (float)range.min;
            float max = (float)range.max;
            float def = (float)range.def;
            float step = (float)range.inc;
            float stepSmall;
            float stepLarge;

            // only use values as integer if we have a proper range
            const bool isEnum = ysfx_slider_is_enum(fEffect, rindex) &&
                                d_isZero(min) &&
                                max >= 0.0f &&
                                d_isEqual(max + 1.0f, static_cast<float>(ysfx_slider_get_enum_names(fEffect, rindex, nullptr, 0)));

            // NOTE: in case of incomplete slider specification without <min,max,step>;
            //  these are usually output-only sliders.
            if (d_isEqual(min, max))
            {
                // replace with a dummy range
                min = 0.0f;
                max = 1.0f;
            }

            if (min > max)
                std::swap(min, max);

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            pData->param.data[j].hints |= PARAMETER_IS_ENABLED;

            if (isEnum)
            {
                step = 1.0f;
                stepSmall = 1.0f;
                stepLarge = 10.0f;
                pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;
            }
            else
            {
                stepSmall = step/10.0f;
                stepLarge = step*10.0f;
                pData->param.data[j].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
            }

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;
        }

        //if (needsCtrlIn)
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

        //if (needsCtrlOut)
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
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect,);

        ysfx_set_sample_rate(fEffect, pData->engine->getSampleRate());
        ysfx_set_block_size(fEffect, (uint32_t)pData->engine->getBufferSize());
        ysfx_init(fEffect);

        fTransportValues.tempo = 120;
        fTransportValues.playback_state = ysfx_playback_paused;
        fTransportValues.time_position = 0;
        fTransportValues.beat_position = 0;
        fTransportValues.time_signature[0] = 4;
        fTransportValues.time_signature[1] = 4;
    }

    void process(const float* const* const audioIn, float** const audioOut,
                 const float* const* const, float**,
                 const uint32_t frames) override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect,);

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo timeInfo = pData->engine->getTimeInfo();
        const EngineTimeInfoBBT& bbt = timeInfo.bbt;

        fTransportValues.playback_state = timeInfo.playing ?
            ysfx_playback_playing : ysfx_playback_paused;
        fTransportValues.time_position = 1e-6*double(timeInfo.usecs);

        if (bbt.valid)
        {
            const double samplePos  = double(timeInfo.frame);
            const double sampleRate = pData->engine->getSampleRate();
            fTransportValues.tempo = bbt.beatsPerMinute;
            fTransportValues.beat_position = samplePos / (sampleRate * 60 / bbt.beatsPerMinute);
            fTransportValues.time_signature[0] = (uint32_t)bbt.beatsPerBar;
            fTransportValues.time_signature[1] = (uint32_t)bbt.beatType;
        }

        ysfx_set_time_info(fEffect, &fTransportValues);

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin2(); it.valid(); it.next())
                {
                    const ExternalMidiNote& note(it.getValue(kExternalMidiNoteFallback));
                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    uint8_t midiData[3];
                    midiData[0] = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    midiData[1] = note.note;
                    midiData[2] = note.velo;

                    ysfx_midi_event_t event;
                    event.bus = 0;
                    event.offset = 0;
                    event.size = 3;
                    event.data = midiData;
                    ysfx_send_midi(fEffect, &event);
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent = false;
#endif

            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // non-midi
                        if (event.channel == kEngineEventNonMidiChannel)
                        {
                            const uint32_t k = ctrlEvent.param;
                            CARLA_SAFE_ASSERT_CONTINUE(k < pData->param.count);

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                            continue;
                        }

                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
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

                                ctrlEvent.handled = true;
                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.param);
                            midiData[2] = uint8_t(ctrlEvent.normalizedValue*127.0f);

                            ysfx_midi_event_t yevent;
                            yevent.bus = 0;
                            yevent.offset = event.time;
                            yevent.size = 3;
                            yevent.data = midiData;
                            ysfx_send_midi(fEffect, &yevent);
                        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        if (! ctrlEvent.handled)
                            checkForMidiLearn(event);
#endif
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if ((pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) != 0)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_BANK_SELECT;
                            midiData[2] = 0;

                            ysfx_midi_event_t yevent;
                            yevent.bus = 0;
                            yevent.offset = event.time;
                            yevent.size = 3;
                            yevent.data = midiData;
                            ysfx_send_midi(fEffect, &yevent);

                            midiData[1] = MIDI_CONTROL_BANK_SELECT__LSB;
                            midiData[2] = uint8_t(ctrlEvent.normalizedValue*127.0f);
                            yevent.bus = 0;
                            yevent.offset = event.time;
                            yevent.size = 3;
                            yevent.data = midiData;
                            ysfx_send_midi(fEffect, &yevent);
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            if (ctrlEvent.param < pData->prog.count)
                            {
                                setProgramRT(ctrlEvent.param, true);
                            }
                        }
                        else if ((pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) != 0)
                        {
                            uint8_t midiData[2];
                            midiData[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.normalizedValue*127.0f);
                            ysfx_midi_event_t yevent;
                            yevent.bus = 0;
                            yevent.offset = event.time;
                            yevent.size = 2;
                            yevent.data = midiData;
                            ysfx_send_midi(fEffect, &yevent);
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            midiData[2] = 0;
                            ysfx_midi_event_t yevent;
                            yevent.bus = 0;
                            yevent.offset = event.time;
                            yevent.size = 3;
                            yevent.data = midiData;
                            ysfx_send_midi(fEffect, &yevent);
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                postponeRtAllNotesOff();
                            }
#endif

                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            midiData[2] = 0;

                            ysfx_midi_event_t yevent;
                            yevent.bus = 0;
                            yevent.offset = event.time;
                            yevent.size = 3;
                            yevent.data = midiData;
                            ysfx_send_midi(fEffect, &yevent);
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > EngineMidiEvent::kDataSize)
                        continue;

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    if ((status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON) && (pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        continue;
                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    // put back channel in data
                    uint8_t midiData2[EngineMidiEvent::kDataSize];
                    midiData2[0] = uint8_t(status | (event.channel & MIDI_CHANNEL_BIT));
                    std::memcpy(midiData2 + 1, midiEvent.data + 1, static_cast<std::size_t>(midiEvent.size - 1));

                    ysfx_midi_event_t yevent;
                    yevent.bus = midiEvent.port;
                    yevent.offset = event.time;
                    yevent.size = midiEvent.size;
                    yevent.data = midiData2;
                    ysfx_send_midi(fEffect, &yevent);

                    if (status == MIDI_STATUS_NOTE_ON)
                    {
                        pData->postponeNoteOnRtEvent(true, event.channel, midiEvent.data[1], midiEvent.data[2]);
                    }
                    else if (status == MIDI_STATUS_NOTE_OFF)
                    {
                        pData->postponeNoteOffRtEvent(true, event.channel, midiEvent.data[1]);
                    }
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        const uint32_t numInputs = ysfx_get_num_inputs(fEffect);
        const uint32_t numOutputs = ysfx_get_num_outputs(fEffect);
        ysfx_process_float(fEffect, audioIn, audioOut, numInputs, numOutputs, frames);

        // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            ysfx_midi_event_t event;

            while (ysfx_receive_midi(fEffect, &event))
            {
                CARLA_SAFE_ASSERT_BREAK(event.offset < frames);
                CARLA_SAFE_ASSERT_BREAK(event.size > 0);
                CARLA_SAFE_ASSERT_CONTINUE(event.size <= 0xff);

                if (! pData->event.portOut->writeMidiEvent(event.offset,
                                                           static_cast<uint8_t>(event.size),
                                                           event.data))
                    break;
            }

        } // End of MIDI Output

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        {
            uint64_t changes = ysfx_fetch_slider_changes(fEffect);
            uint64_t automations = ysfx_fetch_slider_automations(fEffect);

            if ((changes|automations) != 0)
            {
                for (uint32_t rindex = 0; rindex < ysfx_max_sliders; ++rindex)
                {
                    uint64_t mask = (uint64_t)1 << rindex;

                    //TODO: automations and changes are handled identically
                    // refer to `sliderchange` vs `slider_automate`

                    if (((changes|automations) & mask) != 0)
                    {
                        int32_t parameterIndex = fMapOfSliderToParameter[rindex];
                        CARLA_SAFE_ASSERT_CONTINUE(parameterIndex != -1);

                        const float newValue = static_cast<float>(ysfx_slider_get_value(fEffect, (uint32_t)parameterIndex));
                        setParameterValueRT((uint32_t)parameterIndex, newValue, 0, true);
                    }
                }
            }

            //TODO: slider visibility changes, if this feature can be supported
        }
    }

    // -------------------------------------------------------------------

    bool initJSFX(const CarlaPluginPtr plugin,
                  const char* const filename, const char* name, const char* const label, const uint options)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if ((filename == nullptr || filename[0] == '\0') &&
            (label == nullptr || label[0] == '\0'))
        {
            pData->engine->setLastError("null filename and label");
            return false;
        }

        // ---------------------------------------------------------------

        fUnit = CarlaJsfxUnit();

        {
            water::StringArray splitPaths;

            if (const char* paths = pData->engine->getOptions().pathJSFX)
                splitPaths = water::StringArray::fromTokens(CharPointer_UTF8(paths), CARLA_OS_SPLIT_STR, "");

            File file;
            if (filename && filename[0] != '\0')
                file = File(CharPointer_UTF8(filename));

            if (file.isNotNull() && file.existsAsFile())
            {
                // find which engine search path we're in, and use this as the root
                for (int i = 0; i < splitPaths.size() && !fUnit; ++i)
                {
                    const File currentPath(splitPaths[i].toRawUTF8());
                    if (file.isAChildOf(currentPath))
                        fUnit = CarlaJsfxUnit(currentPath, file);
                }

                // if not found in engine search paths, use parent directory as the root
                if (! fUnit)
                    fUnit = CarlaJsfxUnit(file.getParentDirectory(), file);
            }
            else if (label && label[0] != '\0')
            {
                // search a matching file in plugin paths
                for (int i = 0; i < splitPaths.size() && !fUnit; ++i)
                {
                    const File currentPath(splitPaths[i].toRawUTF8());
                    const File currentFile = currentPath.getChildFile(CharPointer_UTF8(label));
                    const CarlaJsfxUnit currentUnit(currentPath, currentFile);
                    if (File(currentUnit.getFilePath()).existsAsFile())
                        fUnit = currentUnit;
                }
            }
        }

        if (! fUnit)
        {
            pData->engine->setLastError("Cannot locate the JSFX plugin");
            return false;
        }

        // ---------------------------------------------------------------

        ysfx_config_u config(ysfx_config_new());
        CARLA_SAFE_ASSERT_RETURN(config != nullptr, false);

        const String rootPath = fUnit.getRootPath();
        const String filePath = fUnit.getFilePath();

        ysfx_register_builtin_audio_formats(config.get());
        ysfx_set_import_root(config.get(), rootPath);
        ysfx_guess_file_roots(config.get(), filePath);
        ysfx_set_log_reporter(config.get(), &CarlaJsfxLogging::logAll);
        ysfx_set_user_data(config.get(), (intptr_t)this);

        fEffect = ysfx_new(config.get());
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);

        // ---------------------------------------------------------------
        // get info

        {
            if (! ysfx_load_file(fEffect, filePath, 0))
            {
                pData->engine->setLastError("Failed to load JSFX");
                return false;
            }

            // TODO(jsfx) adapt when implementing these features
            const int compileFlags = 0
                //| ysfx_compile_no_serialize
                | ysfx_compile_no_gfx
                ;

            if (! ysfx_compile(fEffect, compileFlags))
            {
                pData->engine->setLastError("Failed to compile JSFX");
                return false;
            }
        }

        if (name != nullptr && name[0] != '\0')
        {
            pData->name = pData->engine->getUniquePluginName(name);
        }
        else
        {
            pData->name = carla_strdup(ysfx_get_name(fEffect));
        }

        pData->filename = carla_strdup(filePath);

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

        if (isPluginOptionEnabled(options, PLUGIN_OPTION_USE_CHUNKS))
            pData->options |= PLUGIN_OPTION_USE_CHUNKS;

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
        if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;

        return true;
    }

private:
    ysfx_t* fEffect;
    ysfx_state_t* fEffectState;
    CarlaJsfxUnit fUnit;
    water::String fChunkText;
    ysfx_time_info_t fTransportValues;
    std::vector<int32_t> fMapOfSliderToParameter;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginJSFX)
};

CARLA_BACKEND_END_NAMESPACE

#endif // HAVE_YSFX

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newJSFX(const Initializer& init)
{
    carla_debug("CarlaPlugin::newJSFX({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.name, init.label, init.uniqueId);

#ifdef HAVE_YSFX
    std::shared_ptr<CarlaPluginJSFX> plugin(new CarlaPluginJSFX(init.engine, init.id));

    if (! plugin->initJSFX(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
#else
    init.engine->setLastError("JSFX support not available");
    return nullptr;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
