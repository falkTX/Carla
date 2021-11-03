/*
 * Carla JSFX Plugin
 * Copyright (C) 2021 Filipe Coelho <falktx@falktx.com>
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

// TODO(jsfx) graphics section

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"
#include "CarlaJsfxUtils.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaScopeUtils.hpp"
#include "CarlaUtils.hpp"

#include "water/files/File.h"
#include "water/text/StringArray.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

using water::CharPointer_UTF8;
using water::File;
using water::String;
using water::StringArray;

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginJSFX : public CarlaPlugin
{
public:
    CarlaPluginJSFX(CarlaEngine* const engine, const uint id) noexcept
        : CarlaPlugin(engine, id)
    {
        carla_debug("CarlaPluginJSFX::CarlaPluginJSFX(%p, %i)", engine, id);
    }

    ~CarlaPluginJSFX()
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

        if (fEffect != nullptr)
        {
            delete fEffect;
            fEffect = nullptr;
        }

        if (fFileAPI != nullptr)
        {
            delete fFileAPI;
            fFileAPI = nullptr;
        }

        if (fPathLibrary != nullptr)
        {
            delete fPathLibrary;
            fPathLibrary = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_JSFX;
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

        const JsusFx_Slider& slider = fEffect->sliders[pData->param.data[parameterId].rindex];
        return (uint32_t)slider.enumNames.size();
    }

    // -------------------------------------------------------------------
    // Information (current data)

    std::size_t getChunkData(void** dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        JsusFxSerializationData fxData;
        CarlaJsusFxSerializer serializer(fxData);
        CARLA_SAFE_ASSERT_RETURN(fEffect->serialize(serializer, true), 0);

        fChunkText = serializer.convertDataToString(fxData);

        *dataPtr = (void*)fChunkText.toRawUTF8();
        return (std::size_t)fChunkText.getNumBytesAsUTF8();
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

        const JsusFx_Slider& slider = fEffect->sliders[pData->param.data[parameterId].rindex];
        return slider.getValue();
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const JsusFx_Slider& slider = fEffect->sliders[pData->param.data[parameterId].rindex];
        std::snprintf(strBuf, STR_MAX, "%s", slider.desc);

        return true;
    }


    bool getParameterText(const uint32_t parameterId, char* const strBuf) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        const JsusFx_Slider& slider = fEffect->sliders[pData->param.data[parameterId].rindex];
        float value = getParameterValue(parameterId);

        int intValue = (int)value;
        if (intValue >= 0 && (size_t)intValue < slider.enumNames.size())
            std::snprintf(strBuf, STR_MAX, "%s", slider.enumNames[(size_t)intValue].c_str());
        else
            std::snprintf(strBuf, STR_MAX, "%.12g", value);

        return true;
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

        const JsusFx_Slider& slider = fEffect->sliders[pData->param.data[parameterId].rindex];
        CARLA_SAFE_ASSERT_RETURN(scalePointId < slider.enumNames.size(), false);

        std::snprintf(strBuf, STR_MAX, "%s", slider.enumNames[(size_t)scalePointId].c_str());
        return true;
    }

    bool getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fUnit.getFileId().toRawUTF8(), STR_MAX);
        return true;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        int32_t sliderIndex = pData->param.data[parameterId].rindex;
        fEffect->moveSlider(sliderIndex, value);

        const JsusFx_Slider& slider = fEffect->sliders[sliderIndex];
        float newValue = slider.getValue();
        CarlaPlugin::setParameterValue(parameterId, newValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t parameterId, const float value, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fEffect != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        int32_t sliderIndex = pData->param.data[parameterId].rindex;
        fEffect->moveSlider(sliderIndex, value);

        const JsusFx_Slider& slider = fEffect->sliders[sliderIndex];
        float newValue = slider.getValue();
        CarlaPlugin::setParameterValueRT(parameterId, newValue, sendCallbackLater);
    }

    void setChunkData(const void* data, std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);

        water::String dataText(water::CharPointer_UTF8((const char*)data), dataSize);

        JsusFxSerializationData fxData;
        CARLA_SAFE_ASSERT_RETURN(CarlaJsusFxSerializer::convertStringToData(dataText, fxData),);

        CarlaJsusFxSerializer serializer(fxData);
        CARLA_SAFE_ASSERT_RETURN(fEffect->serialize(serializer, false),);
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
        fEffect->prepare(pData->engine->getSampleRate(), pData->engine->getBufferSize());

        // NOTE: count can be -1 in case of "none"
        uint32_t aIns = (fEffect->numInputs == -1) ? 0 : (uint32_t)fEffect->numInputs;
        uint32_t aOuts = (fEffect->numOutputs == -1) ? 0 : (uint32_t)fEffect->numOutputs;

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
        }

        uint32_t params = 0;
        uint32_t mapOfParameterToSlider[JsusFx::kMaxSliders];
        for (uint32_t sliderIndex = 0; sliderIndex < JsusFx::kMaxSliders; ++sliderIndex)
        {
            JsusFx_Slider &slider = fEffect->sliders[sliderIndex];
            if (slider.exists)
            {
                mapOfParameterToSlider[params] = sliderIndex;
                ++params;
            }
        }

        if (params > 0)
        {
            pData->param.createNew(params, false);
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (uint32_t j = 0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (j < fEffect->inputNames.size() && !fEffect->inputNames[j].empty())
            {
                portName += fEffect->inputNames[j].c_str();
            }
            else if (aIns > 1)
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
        for (uint32_t j = 0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (j < fEffect->outputNames.size() && !fEffect->outputNames[j].empty())
            {
                portName += fEffect->outputNames[j].c_str();
            }
            else if (aOuts > 1)
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

        // Parameters
        for (uint32_t j = 0; j < params; ++j)
        {
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].index  = (int32_t)j;
            pData->param.data[j].rindex = (int32_t)mapOfParameterToSlider[j];

            const JsusFx_Slider &slider = fEffect->sliders[mapOfParameterToSlider[j]];
            float min = slider.min;
            float max = slider.max;
            float def = slider.def;
            float step = slider.inc;

            // only use values as integer if we have a proper range
            bool isEnum = slider.isEnum && min == 0.0f && max >= 0.0f &&
                max + 1.0f == (float)slider.enumNames.size();

            // NOTE: in case of incomplete slider specification without <min,max,step>;
            //  these are usually output-only sliders.
            if (min == max)
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

            float stepSmall;
            float stepLarge;
            if (isEnum)
            {
                step = 1.0f;
                stepSmall = 1.0f;
                stepLarge = 10.0f;
            }
            else
            {
                stepSmall = step/10.0f;
                stepLarge = step*10.0f;
            }

            pData->param.data[j].hints |= PARAMETER_IS_ENABLED;

            if (isEnum)
            {
                pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;
                pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;
            }
            else
            {
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

        const double sampleRate = pData->engine->getSampleRate();
        const uint32_t bufferSize = pData->engine->getBufferSize();
        fEffect->prepare((int)sampleRate, (int)bufferSize);

        fTransportValues = TransportValues();
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

        fTransportValues.playbackState = timeInfo.playing ?
            JsusFx::kPlaybackState_Playing : JsusFx::kPlaybackState_Paused;
        fTransportValues.playbackPositionInSeconds = 1e-6*double(timeInfo.usecs);

        if (bbt.valid)
        {
            const double samplePos  = double(timeInfo.frame);
            const double sampleRate = pData->engine->getSampleRate();
            fTransportValues.tempo = bbt.beatsPerMinute;
            fTransportValues.beatPosition = samplePos / (sampleRate * 60 / bbt.beatsPerMinute);
            fTransportValues.timeSignatureNumerator = int(bbt.beatsPerBar);
            fTransportValues.timeSignatureDenumerator = int(bbt.beatType);
        }

        fEffect->setTransportValues(
            fTransportValues.tempo,
            fTransportValues.playbackState,
            fTransportValues.playbackPositionInSeconds,
            fTransportValues.beatPosition,
            fTransportValues.timeSignatureNumerator,
            fTransportValues.timeSignatureDenumerator);

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

                    uint8_t midiEvent[3];
                    midiEvent[0] = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    midiEvent[1] = note.note;
                    midiEvent[2] = note.velo;

                    fEffect->addInputEvent(0, midiEvent, 3);
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
                            setParameterValueRT(k, value, true);
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
                            setParameterValueRT(k, value, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.param);
                            midiData[2] = uint8_t(ctrlEvent.normalizedValue*127.0f);

                            fEffect->addInputEvent(static_cast<int>(event.time), midiData, 3);
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
                            fEffect->addInputEvent(static_cast<int>(event.time), midiData, 3);

                            midiData[1] = MIDI_CONTROL_BANK_SELECT__LSB;
                            midiData[2] = uint8_t(ctrlEvent.normalizedValue*127.0f);
                            fEffect->addInputEvent(static_cast<int>(event.time), midiData, 3);
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
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = uint8_t(ctrlEvent.normalizedValue*127.0f);
                            fEffect->addInputEvent(static_cast<int>(event.time), midiData, 2);
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            uint8_t midiData[3];
                            midiData[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            midiData[2] = 0;

                            fEffect->addInputEvent(static_cast<int>(event.time), midiData, 3);
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

                            fEffect->addInputEvent(static_cast<int>(event.time), midiData, 3);
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    const uint8_t* const midiData(midiEvent.size > EngineMidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data);

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiData));

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
                    if (status == MIDI_STATUS_NOTE_ON && midiData[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    // put back channel in data
                    uint8_t midiData2[midiEvent.size];
                    midiData2[0] = uint8_t(status | (event.channel & MIDI_CHANNEL_BIT));
                    std::memcpy(midiData2+1, midiData+1, static_cast<std::size_t>(midiEvent.size-1));

                    fEffect->addInputEvent(static_cast<int>(event.time), midiData2, midiEvent.size);

                    if (status == MIDI_STATUS_NOTE_ON)
                    {
                        pData->postponeNoteOnRtEvent(true, event.channel, midiData[1], midiData[2]);
                    }
                    else if (status == MIDI_STATUS_NOTE_OFF)
                    {
                        pData->postponeNoteOffRtEvent(true, event.channel, midiData[1]);
                    }
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        fEffect->process((const float**)audioIn, audioOut, (int)frames, fEffect->numInputs, fEffect->numOutputs);

        // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            size_t iterPos = 0;
            int samplePosition;
            const uint8_t *data;
            int numBytes;

            while (fEffect->iterateOutputEvents(iterPos, samplePosition, data, numBytes))
            {
                CARLA_SAFE_ASSERT_BREAK(samplePosition >= 0);
                CARLA_SAFE_ASSERT_BREAK(samplePosition < static_cast<int>(frames));
                CARLA_SAFE_ASSERT_BREAK(numBytes > 0);
                CARLA_SAFE_ASSERT_CONTINUE(numBytes <= 0xff);

                if (! pData->event.portOut->writeMidiEvent(static_cast<uint32_t>(samplePosition),
                                                           static_cast<uint8_t>(numBytes),
                                                           data))
                    break;
            }

        } // End of MIDI Output
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
            StringArray splitPaths;

            if (const char* paths = pData->engine->getOptions().pathJSFX)
                splitPaths = StringArray::fromTokens(CharPointer_UTF8(paths), CARLA_OS_SPLIT_STR, "");

            File file;
            if (filename && filename[0] != '\0')
                file = File(CharPointer_UTF8(filename));

            if (file.isNotNull() && file.existsAsFile())
            {
                // find which engine search path we're in, and use this as the root
                for (int i = 0; i < splitPaths.size() && !fUnit; ++i)
                {
                    const File currentPath(splitPaths[i]);
                    if (file.isAChildOf(currentPath))
                        fUnit = CarlaJsfxUnit(currentPath, file);
                }

                // if not found in engine search paths, use parent directory as the root
                if (!fUnit)
                    fUnit = CarlaJsfxUnit(file.getParentDirectory(), file);
            }
            else if (label && label[0] != '\0')
            {
                // search a matching file in plugin paths
                for (int i = 0; i < splitPaths.size() && !fUnit; ++i)
                {
                    const File currentPath(splitPaths[i]);
                    const File currentFile = currentPath.getChildFile(CharPointer_UTF8(label));
                    const CarlaJsfxUnit currentUnit(currentPath, currentFile);
                    if (currentUnit.getFilePath().existsAsFile())
                        fUnit = currentUnit;
                }
            }
        }

        if (!fUnit)
        {
            pData->engine->setLastError("Cannot locate the JSFX plugin");
            return false;
        }

        // ---------------------------------------------------------------

        fPathLibrary = new CarlaJsusFxPathLibrary(fUnit);
        fFileAPI = new CarlaJsusFxFileAPI;
        fEffect = new CarlaJsusFx(*fPathLibrary);
        fEffect->fileAPI = fFileAPI;
        fFileAPI->init(fEffect->m_vm);

        // ---------------------------------------------------------------
        // get info

        const water::String filePath = fUnit.getFilePath().getFullPathName();

        {
            const CarlaScopedLocale csl;

            // TODO(jsfx) uncomment when implementing these features
            const int compileFlags = 0
                //| JsusFx::kCompileFlag_CompileGraphicsSection
                | JsusFx::kCompileFlag_CompileSerializeSection
                ;

            if (!fEffect->compile(*fPathLibrary, filePath.toRawUTF8(), compileFlags))
            {
                pData->engine->setLastError("Failed to compile JSFX");
                return false;
            }
        }

        // jsusfx lacks validity checks currently,
        // ensure we have at least the description (required)
        if (fEffect->desc[0] == '\0')
        {
            pData->engine->setLastError("The JSFX header is invalid");
            return false;
        }

        if (name != nullptr && name[0] != '\0')
        {
            pData->name = pData->engine->getUniquePluginName(name);
        }
        else
        {
            pData->name = carla_strdup(fEffect->desc);
        }

        pData->filename = carla_strdup(filePath.toRawUTF8());

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
    CarlaJsusFxPathLibrary* fPathLibrary = nullptr;
    CarlaJsusFxFileAPI* fFileAPI = nullptr;
    CarlaJsusFx* fEffect = nullptr;
    CarlaJsfxUnit fUnit;
    water::String fChunkText;

    struct TransportValues
    {
        double tempo = 120;
        JsusFx::PlaybackState playbackState = JsusFx::kPlaybackState_Paused;
        double playbackPositionInSeconds = 0;
        double beatPosition = 0;
        int timeSignatureNumerator = 4;
        int timeSignatureDenumerator = 4;
    };

    TransportValues fTransportValues;
};

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newJSFX(const Initializer& init)
{
    carla_debug("CarlaPlugin::newJSFX({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.name, init.label, init.uniqueId);

    JsusFx::init();

    std::shared_ptr<CarlaPluginJSFX> plugin(new CarlaPluginJSFX(init.engine, init.id));

    if (!plugin->initJSFX(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
