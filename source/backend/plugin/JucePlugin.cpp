/*
 * Carla Juce Plugin
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#if (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "JucePluginWindow.hpp"

#include "juce_audio_processors.h"

using namespace juce;

CARLA_BACKEND_START_NAMESPACE

class JucePlugin : public CarlaPlugin,
                   public AudioPlayHead,
                   public AudioProcessorListener
{
public:
    JucePlugin(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fInstance(nullptr),
          fAudioBuffer(1, 0)
    {
        carla_debug("JucePlugin::JucePlugin(%p, %i)", engine, id);

        fMidiBuffer.ensureSize(2048);
        fMidiBuffer.clear();
        fPosInfo.resetToDefault();
    }

    ~JucePlugin() override
    {
        carla_debug("JucePlugin::~JucePlugin()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            showCustomUI(false);

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fInstance != nullptr)
        {
            delete fInstance;
            fInstance = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return getPluginTypeFromString(fDesc.pluginFormatName.toRawUTF8());
    }

    PluginCategory getCategory() const noexcept override
    {
        if (fDesc.isInstrument)
            return PLUGIN_CATEGORY_SYNTH;
        return getPluginCategoryFromName(fDesc.category.toRawUTF8());
    }

    int64_t getUniqueId() const noexcept override
    {
        return fDesc.uid;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        *dataPtr = nullptr;

        try {
            fChunk.reset();
            fInstance->getStateInformation(fChunk);
        } CARLA_SAFE_EXCEPTION_RETURN("JucePlugin::getChunkData", 0);

        if (const std::size_t size = fChunk.getSize())
        {
            *dataPtr = fChunk.getData();
            return size;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr, 0x0);

        uint options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        options |= PLUGIN_OPTION_USE_CHUNKS;

        if (fInstance->acceptsMidi())
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr, 0.0f);

        return fInstance->getParameter(static_cast<int>(parameterId));
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fDesc.name.toRawUTF8(), STR_MAX);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fDesc.manufacturerName.toRawUTF8(), STR_MAX);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        getMaker(strBuf);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        std::strncpy(strBuf, fInstance->getName().toRawUTF8(), STR_MAX);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        std::strncpy(strBuf, fInstance->getParameterName(static_cast<int>(parameterId), STR_MAX).toRawUTF8(), STR_MAX);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        std::strncpy(strBuf, fInstance->getParameterText(static_cast<int>(parameterId), STR_MAX).toRawUTF8(), STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        std::strncpy(strBuf, fInstance->getParameterLabel(static_cast<int>(parameterId)).toRawUTF8(), STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        CarlaPlugin::setName(newName);

        if (fWindow != nullptr)
        {
            String uiName(pData->name);
            uiName += " (GUI)";
            fWindow->setName(uiName);
        }
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fInstance->setParameter(static_cast<int>(parameterId), value);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        {
            const ScopedSingleProcessLocker spl(this, true);
            fInstance->setStateInformation(data, dataSize);
        }

#ifdef BUILD_BRIDGE
        const bool sendOsc(false);
#else
        const bool sendOsc(pData->engine->isOscControlRegistered());
#endif
        pData->updateParameterValues(this, sendOsc, true, false);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        if (yesNo)
        {
            if (fWindow == nullptr)
            {
                String uiName(pData->name);
                uiName += " (GUI)";

                fWindow = new JucePluginWindow();
                fWindow->setName(uiName);
            }

            if (AudioProcessorEditor* const editor = fInstance->createEditorIfNeeded())
                fWindow->show(editor);
        }
        else
        {
            if (fWindow != nullptr)
                fWindow->hide();

            if (AudioProcessorEditor* const editor = fInstance->getActiveEditor())
                delete editor;

            fWindow = nullptr;
        }
    }

    void idle() override
    {
        if (fWindow != nullptr)
        {
            if (fWindow->wasClosedByUser())
            {
                showCustomUI(false);
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
            }
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);
        carla_debug("VstPlugin::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        fInstance->refreshParameterList();

        uint32_t aIns, aOuts, mIns, mOuts, params;
        mIns = mOuts = 0;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        aIns   = (fInstance->getNumInputChannels() > 0)  ? static_cast<uint32_t>(fInstance->getNumInputChannels())  : 0;
        aOuts  = (fInstance->getNumOutputChannels() > 0) ? static_cast<uint32_t>(fInstance->getNumOutputChannels()) : 0;
        params = (fInstance->getNumParameters() > 0)     ? static_cast<uint32_t>(fInstance->getNumParameters())     : 0;

        if (fInstance->acceptsMidi())
        {
            mIns = 1;
            needsCtrlIn = true;
        }

        if (fInstance->producesMidi())
        {
            mOuts = 1;
            needsCtrlOut = true;
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

        if (params > 0)
        {
            pData->param.createNew(params, false);
            needsCtrlIn = true;
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
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

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
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

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
            pData->audioOut.ports[j].rindex = j;
        }

        for (uint32_t j=0; j < params; ++j)
        {
            pData->param.data[j].type   = PARAMETER_INPUT;
            pData->param.data[j].index  = static_cast<int32_t>(j);
            pData->param.data[j].rindex = static_cast<int32_t>(j);

            float min, max, def, step, stepSmall, stepLarge;

            // TODO
            //const int numSteps(fInstance->getParameterNumSteps(static_cast<int>(j)));
            {
                min = 0.0f;
                max = 1.0f;
                step = 0.001f;
                stepSmall = 0.0001f;
                stepLarge = 0.1f;
            }

            pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
#ifndef BUILD_BRIDGE
            pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;
#endif

            if (fInstance->isParameterAutomatable(static_cast<int>(j)))
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

            // FIXME?
            def = fInstance->getParameterDefaultValue(static_cast<int>(j));

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;
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

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);
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

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // plugin hints
        pData->hints  = 0x0;
        pData->hints |= PLUGIN_NEEDS_FIXED_BUFFERS;

        if (fDesc.isInstrument)
            pData->hints |= PLUGIN_IS_SYNTH;

        if (fInstance->hasEditor())
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;
            pData->hints |= PLUGIN_NEEDS_SINGLE_THREAD;
        }

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (mOuts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        fInstance->setPlayConfigDetails(static_cast<int>(aIns), static_cast<int>(aOuts), pData->engine->getSampleRate(), static_cast<int>(pData->engine->getBufferSize()));

        bufferSizeChanged(pData->engine->getBufferSize());
        //reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("VstPlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        try {
            fInstance->prepareToPlay(pData->engine->getSampleRate(), static_cast<int>(pData->engine->getBufferSize()));
        } catch(...) {}
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fInstance != nullptr,);

        try {
            fInstance->releaseResources();
        } catch(...) {}
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(outBuffer[i], static_cast<int>(frames));
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            fInstance->reset();
            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input

        fMidiBuffer.clear();

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                for (RtLinkedList<ExternalMidiNote>::Itenerator it = pData->extNotes.data.begin(); it.valid(); it.next())
                {
                    const ExternalMidiNote& note(it.getValue());

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    uint8_t midiEvent[3];
                    midiEvent[0] = static_cast<uint8_t>((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    midiEvent[1] = note.note;
                    midiEvent[2] = note.velo;

                    fMidiBuffer.addEvent(midiEvent, 3, 0);
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE
            bool allNotesOffSent = false;
#endif
            uint32_t numEvents = pData->event.portIn->getEventCount();
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
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
                                break;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                break;
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
                                break;
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
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
                            break;
                        }

                        // check if event is already handled
                        if (k != pData->param.count)
                            break;

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            uint8_t midiData[3];
                            midiData[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + i);
                            midiData[1] = static_cast<uint8_t>(ctrlEvent.param);
                            midiData[2] = uint8_t(ctrlEvent.value*127.0f);

                            fMidiBuffer.addEvent(midiData, 3, static_cast<int>(event.time));
                        }
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (uint32_t k=0; k < pData->midiprog.count; ++k)
                            {
                                if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                {
                                    const int32_t index(static_cast<int32_t>(k));
                                    setMidiProgram(index, false, false, false);
                                    pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, index, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            uint8_t midiData[3];
                            midiData[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + i);
                            midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            midiData[2] = 0;

                            fMidiBuffer.addEvent(midiData, 3, static_cast<int>(event.time));
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

                            uint8_t midiData[3];
                            midiData[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + i);
                            midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            midiData[2] = 0;

                            fMidiBuffer.addEvent(midiData, 3, static_cast<int>(event.time));
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi:
                {
                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));
                    uint8_t channel = event.channel;

                    // Fix bad note-off
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    fMidiBuffer.addEvent(midiEvent.data, midiEvent.size, static_cast<int>(event.time));

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, channel, midiEvent.data[1], 0.0f);

                    break;
                } // case kEngineEventTypeMidi
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());

        fPosInfo.isPlaying = timeInfo.playing;

        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
        {
            const double ppqBar  = double(timeInfo.bbt.bar - 1) * timeInfo.bbt.beatsPerBar;
            const double ppqBeat = double(timeInfo.bbt.beat - 1);
            const double ppqTick = double(timeInfo.bbt.tick) / timeInfo.bbt.ticksPerBeat;

            fPosInfo.bpm = timeInfo.bbt.beatsPerMinute;

            fPosInfo.timeSigNumerator   = static_cast<int>(timeInfo.bbt.beatsPerBar);
            fPosInfo.timeSigDenominator = static_cast<int>(timeInfo.bbt.beatType);

            fPosInfo.timeInSamples = static_cast<int64_t>(timeInfo.frame);
            fPosInfo.timeInSeconds = static_cast<double>(fPosInfo.timeInSamples)/pData->engine->getSampleRate();

            fPosInfo.ppqPosition = ppqBar + ppqBeat + ppqTick;
            fPosInfo.ppqPositionOfLastBarStart = ppqBar;
        }

        // --------------------------------------------------------------------------------------------------------
        // Process

        processSingle(inBuffer, outBuffer, frames);

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            // TODO
        }
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames)
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
                FloatVectorOperations::clear(outBuffer[i], static_cast<int>(frames));
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio in buffers

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            fAudioBuffer.copyFrom(static_cast<int>(i), 0, inBuffer[i], static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fInstance->processBlock(fAudioBuffer, fMidiBuffer);

        // --------------------------------------------------------------------------------------------------------
        // Set audio out buffers

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            FloatVectorOperations::copy(outBuffer[i], fAudioBuffer.getReadPointer(static_cast<int>(i)), static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Midi out

        if (! fMidiBuffer.isEmpty())
        {
            if (pData->event.portOut != nullptr)
            {
                const uint8* midiEventData;
                int midiEventSize, midiEventPosition;

                for (MidiBuffer::Iterator i(fMidiBuffer); i.getNextEvent(midiEventData, midiEventSize, midiEventPosition);)
                {
                    CARLA_SAFE_ASSERT_BREAK(midiEventPosition >= 0 && midiEventPosition < static_cast<int>(frames));
                    CARLA_SAFE_ASSERT_BREAK(midiEventSize > 0);

                    if (! pData->event.portOut->writeMidiEvent(static_cast<uint32_t>(midiEventPosition), static_cast<uint8_t>(midiEventSize), midiEventData))
                        break;
                }
            }

            fMidiBuffer.clear();
        }

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("VstPlugin::bufferSizeChanged(%i)", newBufferSize);

        fAudioBuffer.setSize(static_cast<int>(std::max<uint32_t>(pData->audioIn.count, pData->audioOut.count)), static_cast<int>(newBufferSize));

        if (pData->active)
        {
            deactivate();
            activate();
        }
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("VstPlugin::sampleRateChanged(%g)", newSampleRate);

        if (pData->active)
        {
            deactivate();
            activate();
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    // nothing

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

    void* getNativeHandle() const noexcept override
    {
        return (fInstance != nullptr) ? fInstance->getPlatformSpecificData() : nullptr;
    }

    // -------------------------------------------------------------------

protected:
    void audioProcessorParameterChanged(AudioProcessor*, int index, float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index >= 0,);

        const uint32_t uindex(static_cast<uint32_t>(index));
        const float fixedValue(pData->param.getFixedValue(uindex, value));

        CarlaPlugin::setParameterValue(static_cast<uint32_t>(index), fixedValue, false, true, true);
    }

    void audioProcessorChanged(AudioProcessor*) override
    {
        pData->engine->callback(ENGINE_CALLBACK_UPDATE, pData->id, 0, 0, 0.0f, nullptr);
    }

    void audioProcessorParameterChangeGestureBegin(AudioProcessor*, int) override {}
    void audioProcessorParameterChangeGestureEnd(AudioProcessor*, int) override {}

    bool getCurrentPosition(CurrentPositionInfo& result) override
    {
        carla_copyStruct<CurrentPositionInfo>(result, fPosInfo);
        return true;
    }

    // -------------------------------------------------------------------

public:
    bool init(const char* const filename, const char* const name, /*const char* const label, */const int64_t uniqueId, const char* const format)
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

#if 0
        if (label == nullptr || label[0] == '\0')
        {
            pData->engine->setLastError("null label");
            return false;
        }
#endif

        if (format == nullptr || format[0] == '\0')
        {
            pData->engine->setLastError("null format");
            return false;
        }

        // ---------------------------------------------------------------
        // fix path for wine usage

        String jfilename(filename);

#ifdef CARLA_OS_WIN
        if (jfilename.startsWith("/"))
        {
            jfilename.replace("/", "\\");
            jfilename = "Z:" + jfilename;
        }
#endif

        //fDesc.name = fDesc.descriptiveName = label;
        fDesc.uid = static_cast<int>(uniqueId);
        fDesc.fileOrIdentifier = jfilename;
        fDesc.pluginFormatName = format;

        fFormatManager.addDefaultFormats();

        String error;
        fInstance = fFormatManager.createPluginInstance(fDesc, 44100, 512, error);

        if (fInstance == nullptr)
        {
            pData->engine->setLastError(error.toRawUTF8());
            return false;
        }

        fInstance->fillInPluginDescription(fDesc);
        fInstance->setPlayHead(this);
        fInstance->addListener(this);

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName(fInstance->getName().toRawUTF8());

        pData->filename = carla_strdup(filename);

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
        pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
        pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        pData->options |= PLUGIN_OPTION_USE_CHUNKS;

        if (fInstance->acceptsMidi())
        {
            pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
            pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return true;
    }

private:
    PluginDescription    fDesc;
    AudioPluginInstance* fInstance;
    AudioPluginFormatManager fFormatManager;

    AudioSampleBuffer   fAudioBuffer;
    MidiBuffer          fMidiBuffer;
    CurrentPositionInfo fPosInfo;
    MemoryBlock         fChunk;

    const char* fUniqueId;

    ScopedPointer<JucePluginWindow> fWindow;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JucePlugin)
};

CARLA_BACKEND_END_NAMESPACE

#endif // defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newJuce(const Initializer& init, const char* const format)
{
    carla_debug("CarlaPlugin::newJuce({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "}, %s)", init.engine, init.filename, init.name, init.label, init.uniqueId, format);

#if (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
    JucePlugin* const plugin(new JucePlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, /*init.label,*/ init.uniqueId, format))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
    {
        init.engine->setLastError("Carla's rack mode can only work with Stereo VST3 plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("Juce plugin not available");
    return nullptr;
    (void)format;
#endif
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
