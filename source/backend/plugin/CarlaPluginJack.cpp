/*
 * Carla Plugin JACK
 * Copyright (C) 2016-2017 Filipe Coelho <falktx@falktx.com>
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

#ifdef BUILD_BRIDGE
# error This file should not be used under bridge mode
#endif

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef CARLA_OS_LINUX

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaShmUtils.hpp"
#include "CarlaThread.hpp"

#include "jackbridge/JackBridge.hpp"

#include <ctime>

// -------------------------------------------------------------------------------------------------------------------

using juce::ChildProcess;
using juce::File;
using juce::ScopedPointer;
using juce::String;
using juce::StringArray;
using juce::Time;

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ExternalMidiNote kExternalMidiNoteFallback = { -1, 0, 0 };

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginJackThread : public CarlaThread
{
public:
    CarlaPluginJackThread(CarlaEngine* const engine, CarlaPlugin* const plugin) noexcept
        : CarlaThread("CarlaPluginJackThread"),
          kEngine(engine),
          kPlugin(plugin),
          fShmIds(),
          fProcess() {}

    void setData(const char* const shmIds) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && shmIds[0] != '\0',);
        CARLA_SAFE_ASSERT(! isThreadRunning());

        fShmIds = shmIds;
    }

    uintptr_t getProcessID() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fProcess != nullptr, 0);

        return (uintptr_t)fProcess->getPID();
    }

protected:
    void run()
    {
        if (fProcess == nullptr)
        {
            fProcess = new ChildProcess();
        }
        else if (fProcess->isRunning())
        {
            carla_stderr("CarlaPluginJackThread::run() - already running");
        }

        String name(kPlugin->getName());
        String filename(kPlugin->getFilename());

        if (name.isEmpty())
            name = "(none)";

        CARLA_SAFE_ASSERT_RETURN(filename.isNotEmpty(),);

        StringArray arguments;

        // binary
        arguments.addTokens(filename, true);

        bool started;

        {
            char strBuf[STR_MAX+1];
            strBuf[STR_MAX] = '\0';

            const EngineOptions& options(kEngine->getOptions());
            const ScopedEngineEnvironmentLocker _seel(kEngine);

#ifdef CARLA_OS_LINUX
            const char* const oldPreload(std::getenv("LD_PRELOAD"));

            if (oldPreload != nullptr)
                ::unsetenv("LD_PRELOAD");
#endif

            std::snprintf(strBuf, STR_MAX, P_UINTPTR, options.frontendWinId);
            carla_setenv("CARLA_FRONTEND_WIN_ID", strBuf);
            carla_setenv("CARLA_SHM_IDS", fShmIds.toRawUTF8());
            carla_setenv("LD_LIBRARY_PATH", "/home/falktx/Personal/FOSS/GIT/falkTX/Carla/bin/jack");

            started = fProcess->start(arguments);

#ifdef CARLA_OS_LINUX
            if (oldPreload != nullptr)
                ::setenv("LD_PRELOAD", oldPreload, 1);

            ::unsetenv("LD_LIBRARY_PATH");
#endif
        }

        if (! started)
        {
            carla_stdout("failed!");
            fProcess = nullptr;
            return;
        }

        for (; fProcess->isRunning() && ! shouldThreadExit();)
            carla_msleep(50);

        // we only get here if bridge crashed or thread asked to exit
        if (fProcess->isRunning() && shouldThreadExit())
        {
            fProcess->waitForProcessToFinish(2000);

            if (fProcess->isRunning())
            {
                carla_stdout("CarlaPluginJackThread::run() - bridge refused to close, force kill now");
                fProcess->kill();
            }
            else
            {
                carla_stdout("CarlaPluginJackThread::run() - bridge auto-closed successfully");
            }
        }
        else
        {
            // forced quit, may have crashed
            if (fProcess->getExitCode() != 0 /*|| fProcess->exitStatus() == QProcess::CrashExit*/)
            {
                carla_stderr("CarlaPluginJackThread::run() - bridge crashed");

                CarlaString errorString("Plugin '" + CarlaString(kPlugin->getName()) + "' has crashed!\n"
                                        "Saving now will lose its current settings.\n"
                                        "Please remove this plugin, and not rely on it from this point.");
                kEngine->callback(CarlaBackend::ENGINE_CALLBACK_ERROR, kPlugin->getId(), 0, 0, 0.0f, errorString);
            }
            else
            {
                carla_stderr("CarlaPluginJackThread::run() - bridge closed itself");
            }
        }

        fProcess = nullptr;
    }

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;

    String fShmIds;

    ScopedPointer<ChildProcess> fProcess;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginJackThread)
};

// -------------------------------------------------------------------------------------------------------------------

class CarlaPluginJack : public CarlaPlugin
{
public:
    CarlaPluginJack(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fInitiated(false),
          fInitError(false),
          fTimedOut(false),
          fTimedError(false),
          fProcCanceled(false),
          fProcWaitTime(0),
          fLastPongTime(-1),
          fBridgeThread(engine, this),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fInfo()
    {
        carla_debug("CarlaPluginJack::CarlaPluginJack(%p, %i, %s, %s)", engine, id, BinaryType2Str(btype), PluginType2Str(ptype));

        pData->hints |= PLUGIN_IS_BRIDGE;
    }

    ~CarlaPluginJack() override
    {
        carla_debug("CarlaPluginJack::~CarlaPluginJack()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            pData->transientTryCounter = 0;

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fBridgeThread.isThreadRunning())
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientQuit);
            fShmRtClientControl.commitWrite();

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientQuit);
            fShmNonRtClientControl.commitWrite();

            if (! fTimedOut)
                waitForClient("stopping", 3000);
        }

        fBridgeThread.stopThread(3000);

        fShmNonRtServerControl.clear();
        fShmNonRtClientControl.clear();
        fShmRtClientControl.clear();
        fShmAudioPool.clear();

        clearBuffers();

        fInfo.chunk.clear();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_JACK;
    }

    PluginCategory getCategory() const noexcept override
    {
        return fInfo.category;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        return fInfo.mIns;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        return fInfo.mOuts;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        return fInfo.optionsAvailable;
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fInfo.label, STR_MAX);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        nullStrBuf(strBuf);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        nullStrBuf(strBuf);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fInfo.name, STR_MAX);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setOption(const uint option, const bool yesNo, const bool sendCallback) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetOption);
            fShmNonRtClientControl.writeUInt(option);
            fShmNonRtClientControl.writeBool(yesNo);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setOption(option, yesNo, sendCallback);
    }

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetCtrlChannel);
            fShmNonRtClientControl.writeShort(channel);
            fShmNonRtClientControl.commitWrite();
        }

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    // -------------------------------------------------------------------
    // Set ui stuff

    void idle() override
    {
        if (fBridgeThread.isThreadRunning())
        {
            if (fInitiated && fTimedOut && pData->active)
                setActive(false, true, true);

            {
                const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

                fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientPing);
                fShmNonRtClientControl.commitWrite();
            }

            try {
                handleNonRtData();
            } CARLA_SAFE_EXCEPTION("handleNonRtData");
        }
        else if (fInitiated)
        {
            fTimedOut   = true;
            fTimedError = true;
            fInitiated  = false;
            carla_stderr2("Plugin bridge has been stopped or crashed");
            pData->engine->callback(ENGINE_CALLBACK_PLUGIN_UNAVAILABLE, pData->id, 0, 0, 0.0f,
                                    "Plugin bridge has been stopped or crashed");
        }

        CarlaPlugin::idle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        carla_debug("CarlaPluginJack::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        // cleanup of previous data
        pData->audioIn.clear();
        pData->audioOut.clear();
        pData->cvIn.clear();
        pData->cvOut.clear();
        pData->event.clear();

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        if (fInfo.aIns > 0)
        {
            pData->audioIn.createNew(fInfo.aIns);
        }

        if (fInfo.aOuts > 0)
        {
            pData->audioOut.createNew(fInfo.aOuts);
            needsCtrlIn = true;
        }

        if (fInfo.cvIns > 0)
        {
            pData->cvIn.createNew(fInfo.cvIns);
        }

        if (fInfo.cvOuts > 0)
        {
            pData->cvOut.createNew(fInfo.cvOuts);
        }

        if (fInfo.mIns > 0)
            needsCtrlIn = true;

        if (fInfo.mOuts > 0)
            needsCtrlOut = true;

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (uint32_t j=0; j < fInfo.aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aInNames != nullptr && fInfo.aInNames[j] != nullptr)
            {
                portName += fInfo.aInNames[j];
            }
            else if (fInfo.aIns > 1)
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
        for (uint32_t j=0; j < fInfo.aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aOutNames != nullptr && fInfo.aOutNames[j] != nullptr)
            {
                portName += fInfo.aOutNames[j];
            }
            else if (fInfo.aOuts > 1)
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

        // TODO - MIDI

        // TODO - CV

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        // extra plugin hints
        pData->extraHints = 0x0;

        if (fInfo.mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (fInfo.mOuts > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        if (fInfo.aIns <= 2 && fInfo.aOuts <= 2 && (fInfo.aIns == fInfo.aOuts || fInfo.aIns == 0 || fInfo.aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        carla_debug("CarlaPluginJack::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        if (! fBridgeThread.isThreadRunning())
        {
            CARLA_SAFE_ASSERT_RETURN(restartBridgeThread(),);
        }

        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientActivate);
            fShmNonRtClientControl.commitWrite();
        }

        fTimedOut = false;

        try {
            waitForClient("activate", 2000);
        } CARLA_SAFE_EXCEPTION("activate - waitForClient");
    }

    void deactivate() noexcept override
    {
        if (! fBridgeThread.isThreadRunning())
            return;

        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientDeactivate);
            fShmNonRtClientControl.commitWrite();
        }

        fTimedOut = false;

        try {
            waitForClient("deactivate", 2000);
        } CARLA_SAFE_EXCEPTION("deactivate - waitForClient");
    }

    void process(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (fProcCanceled || fTimedOut || fTimedError || ! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                FloatVectorOperations::clear(cvOut[i], static_cast<int>(frames));
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            // TODO

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input

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

                    uint8_t data1, data2, data3;
                    data1 = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    data2 = note.note;
                    data3 = note.velo;

                    fShmRtClientControl.writeOpcode(kPluginBridgeRtClientMidiEvent);
                    fShmRtClientControl.writeUInt(0); // time
                    fShmRtClientControl.writeByte(0); // port
                    fShmRtClientControl.writeByte(3); // size
                    fShmRtClientControl.writeByte(data1);
                    fShmRtClientControl.writeByte(data2);
                    fShmRtClientControl.writeByte(data3);
                    fShmRtClientControl.commitWrite();
                }

                pData->extNotes.data.clear();
                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;

            for (uint32_t i=0, numEvents=pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
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
                        break;

                    case kEngineControlEventTypeMidiBank:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventMidiBank);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.writeUShort(event.ctrl.param);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventMidiProgram);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.writeUShort(event.ctrl.param);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventAllSoundOff);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.commitWrite();
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientControlEventAllNotesOff);
                            fShmRtClientControl.writeUInt(event.time);
                            fShmRtClientControl.writeByte(event.channel);
                            fShmRtClientControl.commitWrite();
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size == 0 || midiEvent.size >= MAX_MIDI_VALUE)
                        continue;

                    const uint8_t* const midiData(midiEvent.size > EngineMidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data);

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiData));

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

                    fShmRtClientControl.writeOpcode(kPluginBridgeRtClientMidiEvent);
                    fShmRtClientControl.writeUInt(event.time);
                    fShmRtClientControl.writeByte(midiEvent.port);
                    fShmRtClientControl.writeByte(midiEvent.size);

                    fShmRtClientControl.writeByte(uint8_t(midiData[0] | (event.channel & MIDI_CHANNEL_BIT)));

                    for (uint8_t j=1; j < midiEvent.size; ++j)
                        fShmRtClientControl.writeByte(midiData[j]);

                    fShmRtClientControl.commitWrite();

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, event.channel, midiData[1], midiData[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, event.channel, midiData[1], 0.0f);
                } break;
                }
            }

            pData->postRtEvents.trySplice();

        } // End of Event Input

        if (! processSingle(audioIn, audioOut, cvIn, cvOut, frames))
            return;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            uint8_t size;
            uint32_t time;
            const uint8_t* midiData(fShmRtClientControl.data->midiOut);

            for (std::size_t read=0; read<kBridgeRtClientDataMidiOutSize;)
            {
                size = *midiData;

                if (size == 0)
                    break;

                // advance 8 bits (1 byte)
                midiData = midiData + 1;

                // get time as 32bit
                time = *(const uint32_t*)midiData;

                // advance 32 bits (4 bytes)
                midiData = midiData + 4;

                // store midi data advancing as needed
                uint8_t data[size];

                for (uint8_t j=0; j<size; ++j)
                    data[j] = *midiData++;

                pData->event.portOut->writeMidiEvent(time, size, data);

                read += 1U /* size*/ + 4U /* time */ + size;
            }

        } // End of Control and MIDI Output
    }

    bool processSingle(const float** const audioIn, float** const audioOut,
                       const float** const cvIn, float** const cvOut, const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedError, false);
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr, false);
        }
        if (pData->cvIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvIn != nullptr, false);
        }
        if (pData->cvOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvOut != nullptr, false);
        }

        const int iframes(static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], iframes);
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                FloatVectorOperations::clear(cvOut[i], iframes);
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (uint32_t i=0; i < fInfo.aIns; ++i)
            FloatVectorOperations::copy(fShmAudioPool.data + (i * frames), audioIn[i], iframes);

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());
        BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

        bridgeTimeInfo.playing = timeInfo.playing;
        bridgeTimeInfo.frame   = timeInfo.frame;
        bridgeTimeInfo.usecs   = timeInfo.usecs;
        bridgeTimeInfo.valid   = timeInfo.valid;

        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
        {
            bridgeTimeInfo.bar  = timeInfo.bbt.bar;
            bridgeTimeInfo.beat = timeInfo.bbt.beat;
            bridgeTimeInfo.tick = timeInfo.bbt.tick;

            bridgeTimeInfo.beatsPerBar = timeInfo.bbt.beatsPerBar;
            bridgeTimeInfo.beatType    = timeInfo.bbt.beatType;

            bridgeTimeInfo.ticksPerBeat   = timeInfo.bbt.ticksPerBeat;
            bridgeTimeInfo.beatsPerMinute = timeInfo.bbt.beatsPerMinute;
            bridgeTimeInfo.barStartTick   = timeInfo.bbt.barStartTick;
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientProcess);
            fShmRtClientControl.commitWrite();
        }

        waitForClient("process", fProcWaitTime);

        if (fTimedOut)
        {
            pData->singleMutex.unlock();
            return false;
        }

        if (fShmRtClientControl.data->procFlags)
        {
            carla_stdout("PROC Flags active, disabling plugin");
            fInitiated    = false;
            fProcCanceled = true;
        }

        for (uint32_t i=0; i < fInfo.aOuts; ++i)
            FloatVectorOperations::copy(audioOut[i], fShmAudioPool.data + ((i + fInfo.aIns) * frames), iframes);

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
                        bufValue = audioIn[c][k];
                        audioOut[i][k] = (audioOut[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        FloatVectorOperations::copy(oldBufLeft, audioOut[i], iframes);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            audioOut[i][k]  = oldBufLeft[k]     * (1.0f - balRangeL);
                            audioOut[i][k] += audioOut[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            audioOut[i][k]  = audioOut[i][k] * balRangeR;
                            audioOut[i][k] += oldBufLeft[k]   * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                if (doVolume)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        audioOut[i][k] *= pData->postProc.volume;
                }
            }

        } // End of Post-processing

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        resizeAudioPool(newBufferSize);

        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);
            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetBufferSize);
            fShmNonRtClientControl.writeUInt(newBufferSize);
            fShmNonRtClientControl.commitWrite();
        }

        //fProcWaitTime = newBufferSize*1000/pData->engine->getSampleRate();
        fProcWaitTime = 1000;

        waitForClient("buffersize", 1000);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);
            fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetSampleRate);
            fShmNonRtClientControl.writeDouble(newSampleRate);
            fShmNonRtClientControl.commitWrite();
        }

        //fProcWaitTime = pData->engine->getBufferSize()*1000/newSampleRate;
        fProcWaitTime = 1000;

        waitForClient("samplerate", 1000);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        {
            const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);
            fShmNonRtClientControl.writeOpcode(isOffline ? kPluginBridgeNonRtClientSetOffline : kPluginBridgeNonRtClientSetOnline);
            fShmNonRtClientControl.commitWrite();
        }

        waitForClient("offline", 1000);
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // -------------------------------------------------------------------

    void handleNonRtData()
    {
        for (; fShmNonRtServerControl.isDataAvailableForReading();)
        {
            const PluginBridgeNonRtServerOpcode opcode(fShmNonRtServerControl.readOpcode());
//#ifdef DEBUG
            if (opcode != kPluginBridgeNonRtServerPong)
            {
                carla_debug("CarlaPluginJack::handleNonRtData() - got opcode: %s", PluginBridgeNonRtServerOpcode2str(opcode));
            }
//#endif
            if (opcode != kPluginBridgeNonRtServerNull && fLastPongTime > 0)
                fLastPongTime = Time::currentTimeMillis();

            switch (opcode)
            {
            case kPluginBridgeNonRtServerNull:
            case kPluginBridgeNonRtServerPong:
                break;

            case kPluginBridgeNonRtServerPluginInfo1:
                // uint/category, uint/hints, uint/optionsAvailable, uint/optionsEnabled, long/uniqueId
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readLong();
                break;

            case kPluginBridgeNonRtServerPluginInfo2: {
                // uint/size, str[] (realName), uint/size, str[] (label), uint/size, str[] (maker), uint/size, str[] (copyright)

                // realName
                const uint32_t realNameSize(fShmNonRtServerControl.readUInt());
                char realName[realNameSize+1];
                carla_zeroChars(realName, realNameSize+1);
                fShmNonRtServerControl.readCustomData(realName, realNameSize);

                // label
                const uint32_t labelSize(fShmNonRtServerControl.readUInt());
                char label[labelSize+1];
                carla_zeroChars(label, labelSize+1);
                fShmNonRtServerControl.readCustomData(label, labelSize);

                // maker
                const uint32_t makerSize(fShmNonRtServerControl.readUInt());
                char maker[makerSize+1];
                carla_zeroChars(maker, makerSize+1);
                fShmNonRtServerControl.readCustomData(maker, makerSize);

                // copyright
                const uint32_t copyrightSize(fShmNonRtServerControl.readUInt());
                char copyright[copyrightSize+1];
                carla_zeroChars(copyright, copyrightSize+1);
                fShmNonRtServerControl.readCustomData(copyright, copyrightSize);

                fInfo.name  = realName;
                fInfo.label = label;

                if (pData->name == nullptr)
                    pData->name = pData->engine->getUniquePluginName(realName);
            }   break;

            case kPluginBridgeNonRtServerAudioCount:
                // uint/ins, uint/outs
                fInfo.aIns  = fShmNonRtServerControl.readUInt();
                fInfo.aOuts = fShmNonRtServerControl.readUInt();

                CARLA_SAFE_ASSERT(fInfo.aInNames  == nullptr);
                CARLA_SAFE_ASSERT(fInfo.aOutNames == nullptr);

                if (fInfo.aIns > 0)
                {
                    fInfo.aInNames = new const char*[fInfo.aIns];
                    carla_zeroPointers(fInfo.aInNames, fInfo.aIns);
                }

                if (fInfo.aOuts > 0)
                {
                    fInfo.aOutNames = new const char*[fInfo.aOuts];
                    carla_zeroPointers(fInfo.aOutNames, fInfo.aOuts);
                }
                break;

            case kPluginBridgeNonRtServerMidiCount:
                // uint/ins, uint/outs
                fInfo.mIns  = fShmNonRtServerControl.readUInt();
                fInfo.mOuts = fShmNonRtServerControl.readUInt();
                break;

            case kPluginBridgeNonRtServerCvCount:
                // uint/ins, uint/outs
                fInfo.cvIns  = fShmNonRtServerControl.readUInt();
                fInfo.cvOuts = fShmNonRtServerControl.readUInt();
                break;

            case kPluginBridgeNonRtServerParameterCount:
                // uint/count
                fShmNonRtServerControl.readUInt();
                break;

            case kPluginBridgeNonRtServerProgramCount:
                // uint/count
                fShmNonRtServerControl.readUInt();
                break;

            case kPluginBridgeNonRtServerMidiProgramCount:
                // uint/count
                fShmNonRtServerControl.readUInt();
                break;

            case kPluginBridgeNonRtServerPortName: {
                // byte/type, uint/index, uint/size, str[] (name)
                const uint8_t  portType = fShmNonRtServerControl.readByte();
                const uint32_t index    = fShmNonRtServerControl.readUInt();

                // name
                const uint32_t nameSize(fShmNonRtServerControl.readUInt());
                char* const name = new char[nameSize+1];
                carla_zeroChars(name, nameSize+1);
                fShmNonRtServerControl.readCustomData(name, nameSize);

                CARLA_SAFE_ASSERT_BREAK(portType > kPluginBridgePortNull && portType < kPluginBridgePortTypeCount);

                switch (portType)
                {
                case kPluginBridgePortAudioInput:
                    CARLA_SAFE_ASSERT_BREAK(index < fInfo.aIns);
                    fInfo.aInNames[index] = name;
                    break;
                case kPluginBridgePortAudioOutput:
                    CARLA_SAFE_ASSERT_BREAK(index < fInfo.aOuts);
                    fInfo.aOutNames[index] = name;
                    break;
                }

            }   break;

            case kPluginBridgeNonRtServerParameterData1:
                // uint/index, int/rindex, uint/type, uint/hints, int/cc
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readInt();
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readShort();
                break;

            case kPluginBridgeNonRtServerParameterData2:
                // uint/index, uint/size, str[] (name), uint/size, str[] (unit)
                fShmNonRtServerControl.readUInt();
                if (const uint32_t size = fShmNonRtServerControl.readUInt())
                {
                    char name[size];
                    fShmNonRtServerControl.readCustomData(name, size);
                }
                if (const uint32_t size = fShmNonRtServerControl.readUInt())
                {
                    char symbol[size];
                    fShmNonRtServerControl.readCustomData(symbol, size);
                }
                if (const uint32_t size = fShmNonRtServerControl.readUInt())
                {
                    char unit[size];
                    fShmNonRtServerControl.readCustomData(unit, size);
                }
                break;

            case kPluginBridgeNonRtServerParameterRanges:
                // uint/index, float/def, float/min, float/max, float/step, float/stepSmall, float/stepLarge
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readFloat();
                fShmNonRtServerControl.readFloat();
                fShmNonRtServerControl.readFloat();
                fShmNonRtServerControl.readFloat();
                fShmNonRtServerControl.readFloat();
                fShmNonRtServerControl.readFloat();
                break;

            case kPluginBridgeNonRtServerParameterValue:
                // uint/index, float/value
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readFloat();
                break;

            case kPluginBridgeNonRtServerParameterValue2:
                // uint/index, float/value
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readFloat();
                break;

            case kPluginBridgeNonRtServerDefaultValue:
                // uint/index, float/value
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readFloat();
                break;

            case kPluginBridgeNonRtServerCurrentProgram:
                // int/index
                fShmNonRtServerControl.readInt();
                break;

            case kPluginBridgeNonRtServerCurrentMidiProgram: {
                // int/index
                fShmNonRtServerControl.readInt();
            }   break;

            case kPluginBridgeNonRtServerProgramName: {
                // uint/index, uint/size, str[] (name)
                fShmNonRtServerControl.readUInt();
                const uint32_t nameSize(fShmNonRtServerControl.readUInt());
                char name[nameSize];
                fShmNonRtServerControl.readCustomData(name, nameSize);
            }   break;

            case kPluginBridgeNonRtServerMidiProgramData: {
                // uint/index, uint/bank, uint/program, uint/size, str[] (name)
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readUInt();
                fShmNonRtServerControl.readUInt();
                const uint32_t nameSize(fShmNonRtServerControl.readUInt());
                char name[nameSize];
                fShmNonRtServerControl.readCustomData(name, nameSize);
            }   break;

            case kPluginBridgeNonRtServerSetCustomData: {
                // uint/size, str[], uint/size, str[], uint/size, str[]

                // type
                const uint32_t typeSize(fShmNonRtServerControl.readUInt());
                char type[typeSize];
                fShmNonRtServerControl.readCustomData(type, typeSize);

                // key
                const uint32_t keySize(fShmNonRtServerControl.readUInt());
                char key[keySize];
                fShmNonRtServerControl.readCustomData(key, keySize);

                // value
                const uint32_t valueSize(fShmNonRtServerControl.readUInt());
                char value[valueSize];
                fShmNonRtServerControl.readCustomData(value, valueSize);
            }   break;

            case kPluginBridgeNonRtServerSetChunkDataFile: {
                // uint/size, str[] (filename)
                const uint32_t chunkFilePathSize(fShmNonRtServerControl.readUInt());
                char chunkFilePath[chunkFilePathSize];
                fShmNonRtServerControl.readCustomData(chunkFilePath, chunkFilePathSize);
            }   break;

            case kPluginBridgeNonRtServerSetLatency:
                // uint
                fShmNonRtServerControl.readUInt();
                break;

            case kPluginBridgeNonRtServerReady:
                fInitiated = true;
                break;

            case kPluginBridgeNonRtServerSaved:
                break;

            case kPluginBridgeNonRtServerUiClosed:
                carla_stdout("bridge closed cleanly?");
                pData->active = false;

#ifdef HAVE_LIBLO
                if (pData->engine->isOscControlRegistered())
                    pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_ACTIVE, 0.0f);
#endif

                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_ACTIVE, 0, 0.0f, nullptr);

                fBridgeThread.stopThread(1000);
                break;

            case kPluginBridgeNonRtServerError: {
                // error
                const uint32_t errorSize(fShmNonRtServerControl.readUInt());
                char error[errorSize+1];
                carla_zeroChars(error, errorSize+1);
                fShmNonRtServerControl.readCustomData(error, errorSize);

                if (fInitiated)
                {
                    pData->engine->callback(ENGINE_CALLBACK_ERROR, pData->id, 0, 0, 0.0f, error);

                    // just in case
                    pData->engine->setLastError(error);
                    fInitError = true;
                }
                else
                {
                    pData->engine->setLastError(error);
                    fInitError = true;
                    fInitiated = true;
                }
            }   break;
            }
        }
    }

    // -------------------------------------------------------------------

    uintptr_t getUiBridgeProcessId() const noexcept override
    {
        return fBridgeThread.getProcessID();
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        // ---------------------------------------------------------------
        // set info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);

        if (filename != nullptr && filename[0] != '\0')
            pData->filename = carla_strdup(filename);
        else
            pData->filename = carla_strdup("");

        std::srand(static_cast<uint>(std::time(nullptr)));

        // ---------------------------------------------------------------
        // init sem/shm

        if (! fShmAudioPool.initializeServer())
        {
            carla_stderr("Failed to initialize shared memory audio pool");
            return false;
        }

        if (! fShmRtClientControl.initializeServer())
        {
            carla_stderr("Failed to initialize RT client control");
            fShmAudioPool.clear();
            return false;
        }

        if (! fShmNonRtClientControl.initializeServer())
        {
            carla_stderr("Failed to initialize Non-RT client control");
            fShmRtClientControl.clear();
            fShmAudioPool.clear();
            return false;
        }

        if (! fShmNonRtServerControl.initializeServer())
        {
            carla_stderr("Failed to initialize Non-RT server control");
            fShmNonRtClientControl.clear();
            fShmRtClientControl.clear();
            fShmAudioPool.clear();
            return false;
        }

        // ---------------------------------------------------------------

        // init bridge thread
        {
            char shmIdsStr[6*4+1];
            carla_zeroChars(shmIdsStr, 6*4+1);

            std::strncpy(shmIdsStr+6*0, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*1, &fShmRtClientControl.filename[fShmRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*2, &fShmNonRtClientControl.filename[fShmNonRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*3, &fShmNonRtServerControl.filename[fShmNonRtServerControl.filename.length()-6], 6);

            fBridgeThread.setData(shmIdsStr);
        }

        if (! restartBridgeThread())
            return false;

        // ---------------------------------------------------------------
        // register client

        if (pData->name == nullptr)
            pData->name = pData->engine->getUniquePluginName("unknown");

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        return true;
    }

private:
    bool fInitiated;
    bool fInitError;
    bool fTimedOut;
    bool fTimedError;
    bool fProcCanceled;
    uint fProcWaitTime;

    int64_t fLastPongTime;

    CarlaPluginJackThread fBridgeThread;

    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    struct Info {
        uint32_t aIns, aOuts;
        uint32_t cvIns, cvOuts;
        uint32_t mIns, mOuts;
        PluginCategory category;
        uint optionsAvailable;
        CarlaString name;
        CarlaString label;
        const char** aInNames;
        const char** aOutNames;
        std::vector<uint8_t> chunk;

        Info()
            : aIns(0),
              aOuts(0),
              cvIns(0),
              cvOuts(0),
              mIns(0),
              mOuts(0),
              category(PLUGIN_CATEGORY_NONE),
              optionsAvailable(0),
              name(),
              label(),
              aInNames(nullptr),
              aOutNames(nullptr),
              chunk() {}

        CARLA_DECLARE_NON_COPY_STRUCT(Info)
    } fInfo;

    void resizeAudioPool(const uint32_t bufferSize)
    {
        fShmAudioPool.resize(bufferSize, fInfo.aIns+fInfo.aOuts, fInfo.cvIns+fInfo.cvOuts);

        fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
        fShmRtClientControl.writeULong(static_cast<uint64_t>(fShmAudioPool.dataSize));
        fShmRtClientControl.commitWrite();

        waitForClient("resize-pool", 5000);
    }

    void waitForClient(const char* const action, const uint msecs)
    {
        CARLA_SAFE_ASSERT_RETURN(! fTimedOut,);
        CARLA_SAFE_ASSERT_RETURN(! fTimedError,);

        if (fShmRtClientControl.waitForClient(msecs))
            return;

        fTimedOut = true;
        carla_stderr("waitForClient(%s) timed out", action);
    }

    bool restartBridgeThread()
    {
        fInitiated  = false;
        fInitError  = false;
        fTimedError = false;

        // cleanup of previous data
        delete[] fInfo.aInNames;
        fInfo.aInNames = nullptr;

        delete[] fInfo.aOutNames;
        fInfo.aOutNames = nullptr;

        // reset memory
        fProcCanceled = false;
        fShmRtClientControl.data->procFlags = 0;
        carla_zeroStruct(fShmRtClientControl.data->timeInfo);
        carla_zeroBytes(fShmRtClientControl.data->midiOut, kBridgeRtClientDataMidiOutSize);

        fShmRtClientControl.clearData();
        fShmNonRtClientControl.clearData();
        fShmNonRtServerControl.clearData();

        // initial values
        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientNull);
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtServerData)));

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetBufferSize);
        fShmNonRtClientControl.writeUInt(pData->engine->getBufferSize());

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientSetSampleRate);
        fShmNonRtClientControl.writeDouble(pData->engine->getSampleRate());

        fShmNonRtClientControl.commitWrite();

        if (fShmAudioPool.dataSize != 0)
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
            fShmRtClientControl.writeULong(static_cast<uint64_t>(fShmAudioPool.dataSize));
            fShmRtClientControl.commitWrite();
        }
        else
        {
            // testing dummy message
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientNull);
            fShmRtClientControl.commitWrite();
        }

        fBridgeThread.startThread();

        fLastPongTime = Time::currentTimeMillis();
        CARLA_SAFE_ASSERT(fLastPongTime > 0);

        static bool sFirstInit = true;

        int64_t timeoutEnd = 5000;

        if (sFirstInit)
            timeoutEnd *= 2;
        sFirstInit = false;

        const bool needsEngineIdle = pData->engine->getType() != kEngineTypePlugin;

        for (; Time::currentTimeMillis() < fLastPongTime + timeoutEnd && fBridgeThread.isThreadRunning();)
        {
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

            if (needsEngineIdle)
                pData->engine->idle();

            idle();

            if (fInitiated)
                break;
            if (pData->engine->isAboutToClose())
                break;

            carla_msleep(20);
        }

        fLastPongTime = -1;

        if (fInitError || ! fInitiated)
        {
            fBridgeThread.stopThread(6000);

            if (! fInitError)
                pData->engine->setLastError("Timeout while waiting for a response from plugin-bridge\n"
                                            "(or the plugin crashed on initialization?)");

            return false;
        }

        return true;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginJack)
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_OS_LINUX

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newJackApp(const Initializer& init)
{
    carla_debug("CarlaPlugin::newJackApp({%p, \"%s\", \"%s\", \"%s\"}, %s, %s, \"%s\")", init.engine, init.filename, init.name, init.label);

#ifdef CARLA_OS_LINUX
    CarlaPluginJack* const plugin(new CarlaPluginJack(init.engine, init.id));

    if (! plugin->init(init.filename, init.name))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("JACK Application support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
