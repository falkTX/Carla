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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#ifdef CARLA_OS_LINUX

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaShmUtils.hpp"
#include "CarlaThread.hpp"

#include "water/misc/Time.h"
#include "water/text/StringArray.h"
#include "water/threads/ChildProcess.h"

#include "jackbridge/JackBridge.hpp"

#include <ctime>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------

using water::ChildProcess;
using water::File;
using water::String;
using water::StringArray;
using water::Time;

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
          fNumPorts(),
          fProcess() {}

    void setData(const char* const shmIds, const char* const numPorts) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && shmIds[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(numPorts != nullptr && numPorts[0] != '\0',);
        CARLA_SAFE_ASSERT(! isThreadRunning());

        fShmIds   = shmIds;
        fNumPorts = numPorts;
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
            const EngineOptions& options(kEngine->getOptions());

            char winIdStr[STR_MAX+1];
            std::snprintf(winIdStr, STR_MAX, P_UINTPTR, options.frontendWinId);
            winIdStr[STR_MAX] = '\0';

            CarlaString libjackdir(options.binaryDir);
            libjackdir += "/jack";

            CarlaString ldpreload;
#ifdef HAVE_X11
            ldpreload = (CarlaString(options.binaryDir)
                      + "/libcarla_interposer-jack-x11.so");
#endif

            const ScopedEngineEnvironmentLocker _seel(kEngine);

            const ScopedEnvVar sev2("LD_LIBRARY_PATH", libjackdir.buffer());
            const ScopedEnvVar sev1("LD_PRELOAD", ldpreload.isNotEmpty() ? ldpreload.buffer() : nullptr);

            if (kPlugin->getHints() & PLUGIN_HAS_CUSTOM_UI)
                carla_setenv("CARLA_FRONTEND_WIN_ID", winIdStr);
            else
                carla_unsetenv("CARLA_FRONTEND_WIN_ID");

            carla_setenv("CARLA_LIBJACK_SETUP", fNumPorts.buffer());
            carla_setenv("CARLA_SHM_IDS", fShmIds.buffer());

            started = fProcess->start(arguments);
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
                carla_stdout("CarlaPluginJackThread::run() - application refused to close, force kill now");
                fProcess->kill();
            }
        }
        else
        {
            // forced quit, may have crashed
            if (fProcess->getExitCode() != 0 /*|| fProcess->exitStatus() == QProcess::CrashExit*/)
            {
                carla_stderr("CarlaPluginJackThread::run() - application crashed");

                CarlaString errorString("Plugin '" + CarlaString(kPlugin->getName()) + "' has crashed!\n"
                                        "Saving now will lose its current settings.\n"
                                        "Please remove this plugin, and not rely on it from this point.");
                kEngine->callback(CarlaBackend::ENGINE_CALLBACK_ERROR, kPlugin->getId(), 0, 0, 0.0f, errorString);
            }
        }

        fProcess = nullptr;
    }

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;

    CarlaString fShmIds;
    CarlaString fNumPorts;

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
          fLastPingTime(-1),
          fBridgeThread(engine, this),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fInfo()
    {
        carla_debug("CarlaPluginJack::CarlaPluginJack(%p, %i)", engine, id);

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
        return PLUGIN_CATEGORY_NONE;
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
        std::strncpy(strBuf, fInfo.setupLabel, STR_MAX);
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
        // FIXME
        std::strncpy(strBuf, "Carla's libjack", STR_MAX);
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

    void showCustomUI(const bool yesNo) override
    {
        if (yesNo && ! fBridgeThread.isThreadRunning()) {
            CARLA_SAFE_ASSERT_RETURN(restartBridgeThread(),);
        }

        const CarlaMutexLocker _cml(fShmNonRtClientControl.mutex);

        fShmNonRtClientControl.writeOpcode(yesNo ? kPluginBridgeNonRtClientShowUI : kPluginBridgeNonRtClientHideUI);
        fShmNonRtClientControl.commitWrite();
    }

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

            if (fLastPingTime > 0 && Time::currentTimeMillis() > fLastPingTime + 30000)
            {
                carla_stderr("Did not receive ping message from server for 30 secs, closing...");
                // TODO
                //threadShouldExit();
                //callback(ENGINE_CALLBACK_QUIT, 0, 0, 0, 0.0f, nullptr);
            }
        }
        else if (fInitiated)
        {
            fTimedOut   = true;
            fTimedError = true;
            fInitiated  = false;
            handleProcessStopped();
        }
        else if (fProcCanceled)
        {
            handleProcessStopped();
            fProcCanceled = false;
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

        if (fInfo.mIns > 0)
            needsCtrlIn = true;

        if (fInfo.mOuts > 0)
            needsCtrlOut = true;

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (uint8_t j=0; j < fInfo.aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aIns > 1)
            {
                portName += "audio_in_";
                portName += CarlaString(j+1);
            }
            else
            {
                portName += "audio_in";
            }

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint8_t j=0; j < fInfo.aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fInfo.aOuts > 1)
            {
                portName += "audio_out_";
                portName += CarlaString(j+1);
            }
            else
            {
                portName += "audio_out";
            }

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

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

    void process(const float** const audioIn, float** const audioOut, const float**, float**, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (fProcCanceled || fTimedOut || fTimedError || ! pData->active)
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

#ifndef BUILD_BRIDGE
            bool allNotesOffSent = false;
#endif
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
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

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

        if (! processSingle(audioIn, audioOut, frames))
            return;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            uint32_t time;
            uint8_t port, size;
            const uint8_t* midiData(fShmRtClientControl.data->midiOut);

            for (std::size_t read=0; read<kBridgeRtClientDataMidiOutSize-kBridgeBaseMidiOutHeaderSize;)
            {
                // get time
                time = *(const uint32_t*)midiData;
                midiData += 4;

                // get port and size
                port = *midiData++;
                size = *midiData++;

                if (size == 0)
                    break;

                // store midi data advancing as needed
                uint8_t data[size];

                for (uint8_t j=0; j<size; ++j)
                    data[j] = *midiData++;

                pData->event.portOut->writeMidiEvent(time, size, data);

                read += kBridgeBaseMidiOutHeaderSize + size;
            }

            // TODO
            (void)port;

        } // End of Control and MIDI Output
    }

    bool processSingle(const float** const audioIn, float** const audioOut, const uint32_t frames)
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

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (uint32_t i=0; i < fInfo.aIns; ++i)
            carla_copyFloats(fShmAudioPool.data + (i * frames), audioIn[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());
        BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

        bridgeTimeInfo.playing    = timeInfo.playing;
        bridgeTimeInfo.frame      = timeInfo.frame;
        bridgeTimeInfo.usecs      = timeInfo.usecs;
        bridgeTimeInfo.validFlags = timeInfo.bbt.valid ? kPluginBridgeTimeInfoValidBBT : 0x0;

        if (timeInfo.bbt.valid)
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
            fInitiated    = false;
            fProcCanceled = true;
        }

        for (uint32_t i=0; i < fInfo.aOuts; ++i)
            carla_copyFloats(audioOut[i], fShmAudioPool.data + ((i + fInfo.aIns) * frames), frames);

#ifndef BUILD_BRIDGE
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
                        carla_copyFloats(oldBufLeft, audioOut[i], frames);
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
#endif
        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        resizeAudioPool(newBufferSize);

        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetBufferSize);
            fShmRtClientControl.writeUInt(newBufferSize);
            fShmRtClientControl.commitWrite();
        }

        //fProcWaitTime = newBufferSize*1000/pData->engine->getSampleRate();
        fProcWaitTime = 1000;

        waitForClient("buffersize", 1000);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetSampleRate);
            fShmRtClientControl.writeDouble(newSampleRate);
            fShmRtClientControl.commitWrite();
        }

        //fProcWaitTime = pData->engine->getBufferSize()*1000/newSampleRate;
        fProcWaitTime = 1000;

        waitForClient("samplerate", 1000);
    }

    void offlineModeChanged(const bool isOffline) override
    {
        {
            fShmRtClientControl.writeOpcode(kPluginBridgeRtClientSetOnline);
            fShmRtClientControl.writeBool(isOffline);
            fShmRtClientControl.commitWrite();
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
            if (opcode != kPluginBridgeNonRtServerNull && fLastPingTime > 0)
                fLastPingTime = Time::currentTimeMillis();

            switch (opcode)
            {
            case kPluginBridgeNonRtServerNull:
            case kPluginBridgeNonRtServerPong:
            case kPluginBridgeNonRtServerPluginInfo1:
            case kPluginBridgeNonRtServerPluginInfo2:
            case kPluginBridgeNonRtServerAudioCount:
            case kPluginBridgeNonRtServerMidiCount:
            case kPluginBridgeNonRtServerCvCount:
            case kPluginBridgeNonRtServerParameterCount:
            case kPluginBridgeNonRtServerProgramCount:
            case kPluginBridgeNonRtServerMidiProgramCount:
            case kPluginBridgeNonRtServerPortName:
            case kPluginBridgeNonRtServerParameterData1:
            case kPluginBridgeNonRtServerParameterData2:
            case kPluginBridgeNonRtServerParameterRanges:
            case kPluginBridgeNonRtServerParameterValue:
            case kPluginBridgeNonRtServerParameterValue2:
            case kPluginBridgeNonRtServerDefaultValue:
            case kPluginBridgeNonRtServerCurrentProgram:
            case kPluginBridgeNonRtServerCurrentMidiProgram:
            case kPluginBridgeNonRtServerProgramName:
            case kPluginBridgeNonRtServerMidiProgramData:
            case kPluginBridgeNonRtServerSetCustomData:
                break;

            case kPluginBridgeNonRtServerSetChunkDataFile: {
                // uint/size, str[] (filename)
                const uint32_t chunkFilePathSize(fShmNonRtServerControl.readUInt());
                char chunkFilePath[chunkFilePathSize];
                fShmNonRtServerControl.readCustomData(chunkFilePath, chunkFilePathSize);
            }   break;

            case kPluginBridgeNonRtServerSetLatency:
                break;

            case kPluginBridgeNonRtServerReady:
                fInitiated = true;
                break;

            case kPluginBridgeNonRtServerSaved:
                break;

            case kPluginBridgeNonRtServerUiClosed:
                carla_stdout("got kPluginBridgeNonRtServerUiClosed, bridge closed cleanly?");
                pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
                //fBridgeThread.signalThreadShouldExit();
                //handleProcessStopped();
                //fBridgeThread.stopThread(5000);
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
        // check setup

        if (std::strlen(label) != 6)
        {
            pData->engine->setLastError("invalid application setup received");
            return false;
        }

        for (int i=4; --i >= 0;) {
            CARLA_SAFE_ASSERT_RETURN(label[i] >= '0' && label[i] <= '0'+64, false);
        }
        for (int i=6; --i >= 4;) {
            CARLA_SAFE_ASSERT_RETURN(label[i] >= '0' && label[i] < '0'+0x4f, false);
        }

        fInfo.aIns   = static_cast<uint8_t>(label[0] - '0');
        fInfo.aOuts  = static_cast<uint8_t>(label[1] - '0');
        fInfo.mIns   = static_cast<uint8_t>(carla_minPositive(label[2] - '0', 1));
        fInfo.mOuts  = static_cast<uint8_t>(carla_minPositive(label[3] - '0', 1));

        fInfo.setupLabel = label;

        const int setupHints = label[5] - '0';

        // ---------------------------------------------------------------
        // set info

        pData->filename = carla_strdup(filename);

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName("Jack Application");

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
        // setup hints and options

        // FIXME dryWet broken
        pData->hints  = PLUGIN_IS_BRIDGE | PLUGIN_OPTION_FIXED_BUFFERS;
#ifndef BUILD_BRIDGE
        pData->hints |= /*PLUGIN_CAN_DRYWET |*/ PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE;
#endif
        //fInfo.optionsAvailable = optionAv;

        if (setupHints & 0x1)
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;

        // ---------------------------------------------------------------
        // init bridge thread

        {
            char shmIdsStr[6*4+1];
            carla_zeroChars(shmIdsStr, 6*4+1);

            std::strncpy(shmIdsStr+6*0, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*1, &fShmRtClientControl.filename[fShmRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*2, &fShmNonRtClientControl.filename[fShmNonRtClientControl.filename.length()-6], 6);
            std::strncpy(shmIdsStr+6*3, &fShmNonRtServerControl.filename[fShmNonRtServerControl.filename.length()-6], 6);

            fBridgeThread.setData(shmIdsStr, label);
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

    int64_t fLastPingTime;

    CarlaPluginJackThread fBridgeThread;

    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    struct Info {
        uint8_t aIns, aOuts;
        uint8_t mIns, mOuts;
        uint optionsAvailable;
        CarlaString setupLabel;
        std::vector<uint8_t> chunk;

        Info()
            : aIns(0),
              aOuts(0),
              mIns(0),
              mOuts(0),
              optionsAvailable(0),
              setupLabel(),
              chunk() {}

        CARLA_DECLARE_NON_COPY_STRUCT(Info)
    } fInfo;

    void handleProcessStopped() noexcept
    {
        const bool wasActive = pData->active;
        pData->active = false;

        if (wasActive)
        {
#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
            if (pData->engine->isOscControlRegistered())
                pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_ACTIVE, 0.0f);
            pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_ACTIVE, 0, 0.0f, nullptr);
#endif
        }

        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
            pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
    }

    void resizeAudioPool(const uint32_t bufferSize)
    {
        fShmAudioPool.resize(bufferSize, static_cast<uint32_t>(fInfo.aIns+fInfo.aOuts), 0);

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

        // reset memory
        fProcCanceled = false;
        fShmRtClientControl.data->procFlags = 0;
        carla_zeroStruct(fShmRtClientControl.data->timeInfo);
        carla_zeroBytes(fShmRtClientControl.data->midiOut, kBridgeRtClientDataMidiOutSize);

        fShmRtClientControl.clearData();
        fShmNonRtClientControl.clearData();
        fShmNonRtServerControl.clearData();

        // initial values
        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientVersion);
        fShmNonRtClientControl.writeUInt(CARLA_PLUGIN_BRIDGE_API_VERSION);

        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtClientData)));
        fShmNonRtClientControl.writeUInt(static_cast<uint32_t>(sizeof(BridgeNonRtServerData)));

        fShmNonRtClientControl.writeOpcode(kPluginBridgeNonRtClientInitialSetup);
        fShmNonRtClientControl.writeUInt(pData->engine->getBufferSize());
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

        fLastPingTime = Time::currentTimeMillis();
        CARLA_SAFE_ASSERT(fLastPingTime > 0);

        static bool sFirstInit = true;

        int64_t timeoutEnd = 5000;

        if (sFirstInit)
            timeoutEnd *= 2;
        sFirstInit = false;

        const bool needsEngineIdle = pData->engine->getType() != kEngineTypePlugin;

        for (; Time::currentTimeMillis() < fLastPingTime + timeoutEnd && fBridgeThread.isThreadRunning();)
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

        fLastPingTime = -1;

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

    if (! plugin->init(init.filename, init.name, init.label))
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
