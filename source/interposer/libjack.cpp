/*
 * Carla JACK API for external applications
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

// need to include this first
#include "CarlaDefines.h"

// then define this for bridge utils
#define BUILD_BRIDGE 1

// now include a bunch of stuff
#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaMIDI.h"
#include "CarlaMutex.hpp"
#include "LinkedList.hpp"

#include "AppConfig.h"
#include "juce_core/juce_core.h"

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

#if 0
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>
#include <jack/session.h>
#include <jack/metadata.h>
#endif

// must be last
#include "jackbridge/JackBridge.hpp"

// small check to not hurt myself
#ifdef JACKBRIDGE_DIRECT
# error "Cannot create custom jack server while linking to libjack directly"
#endif

using juce::File;
using juce::MemoryBlock;
using juce::String;
using juce::Time;
using juce::Thread;

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

struct JackPortState {
    char* name;
    char* fullname;
    void* buffer;
    uint index;
    uint flags;

    JackPortState()
        : name(nullptr),
          fullname(nullptr),
          buffer(nullptr),
          index(0),
          flags(0) {}

    JackPortState(const char* const n, const uint i, const uint f)
        : name(strdup(n)),
          fullname(nullptr),
          buffer(nullptr),
          index(i),
          flags(f)
    {
        char strBuf[STR_MAX+1];
        snprintf(strBuf, STR_MAX, "system:%s", n);
        strBuf[STR_MAX] = '\0';

        fullname = strdup(strBuf);
    }

    ~JackPortState()
    {
        free(name);
        free(fullname);
    }
};

struct JackClientState {
    bool activated;

    char* name;

    uint32_t bufferSize;
    double   sampleRate;

    bool playing;
    jack_position_t position;

    LinkedList<JackPortState*> audioIns;
    LinkedList<JackPortState*> audioOuts;
    uint32_t fakeIns, fakeOuts;

    LinkedList<JackPortState> midiIns;
    LinkedList<JackPortState> midiOuts;

    JackProcessCallback process;
    void* processPtr;

    JackShutdownCallback shutdown;
    void* shutdownPtr;

    JackClientState()
        : activated(false),
          name(nullptr),
          bufferSize(0),
          sampleRate(0.0),
          playing(false),
          audioIns(),
          audioOuts(),
          fakeIns(0),
          fakeOuts(0),
          midiIns(),
          midiOuts(),
          process(nullptr),
          processPtr(nullptr),
          shutdown(nullptr),
          shutdownPtr(nullptr)
    {
        carla_zeroStruct(position);
    }

    ~JackClientState()
    {
        free(name);
    }
};

class CarlaJackClient : public Thread
{
public:
    JackClientState fState;

    CarlaJackClient(const char* const audioPoolBaseName, const char* const rtClientBaseName, const char* const nonRtClientBaseName, const char* const nonRtServerBaseName)
        : Thread("CarlaJackClient"),
          fState(),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fBaseNameAudioPool(audioPoolBaseName),
          fBaseNameRtClientControl(rtClientBaseName),
          fBaseNameNonRtClientControl(nonRtClientBaseName),
          fBaseNameNonRtServerControl(nonRtServerBaseName),
          fIsOffline(false),
          fFirstIdle(true),
          fLastPingTime(-1),
          fAudioIns(0),
          fAudioOuts(0)
    {
        carla_debug("CarlaJackClient::CarlaJackClient(\"%s\", \"%s\", \"%s\", \"%s\")", audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);
    }

    ~CarlaJackClient() noexcept override
    {
        carla_debug("CarlaJackClient::~CarlaJackClient()");

        clear();
    }

    bool init(const char* const clientName)
    {
        carla_debug("CarlaJackClient::init(\"%s\")", clientName);

        if (! fShmAudioPool.attachClient(fBaseNameAudioPool))
        {
            carla_stderr("Failed to attach to audio pool shared memory");
            return false;
        }

        if (! fShmRtClientControl.attachClient(fBaseNameRtClientControl))
        {
            clear();
            carla_stderr("Failed to attach to rt client control shared memory");
            return false;
        }

        if (! fShmRtClientControl.mapData())
        {
            clear();
            carla_stderr("Failed to map rt client control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.attachClient(fBaseNameNonRtClientControl))
        {
            clear();
            carla_stderr("Failed to attach to non-rt client control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.mapData())
        {
            clear();
            carla_stderr("Failed to map non-rt control client shared memory");
            return false;
        }

        if (! fShmNonRtServerControl.attachClient(fBaseNameNonRtServerControl))
        {
            clear();
            carla_stderr("Failed to attach to non-rt server control shared memory");
            return false;
        }

        if (! fShmNonRtServerControl.mapData())
        {
            clear();
            carla_stderr("Failed to map non-rt control server shared memory");
            return false;
        }

        PluginBridgeNonRtClientOpcode opcode;

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientNull, opcode);

        const uint32_t shmRtClientDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmRtClientDataSize == sizeof(BridgeRtClientData), shmRtClientDataSize, sizeof(BridgeRtClientData));

        const uint32_t shmNonRtClientDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmNonRtClientDataSize == sizeof(BridgeNonRtClientData), shmNonRtClientDataSize, sizeof(BridgeNonRtClientData));

        const uint32_t shmNonRtServerDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmNonRtServerDataSize == sizeof(BridgeNonRtServerData), shmNonRtServerDataSize, sizeof(BridgeNonRtServerData));

        if (shmRtClientDataSize != sizeof(BridgeRtClientData) || shmNonRtClientDataSize != sizeof(BridgeNonRtClientData) || shmNonRtServerDataSize != sizeof(BridgeNonRtServerData))
        {
            carla_stderr2("CarlaJackClient: data size mismatch");
            return false;
        }

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetBufferSize, opcode);
        fState.bufferSize = fShmNonRtClientControl.readUInt();

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetSampleRate, opcode);
        fState.sampleRate = fShmNonRtClientControl.readDouble();

        if (fState.bufferSize == 0 || carla_isZero(fState.sampleRate))
        {
            carla_stderr2("CarlaJackClient: invalid empty state");
            return false;
        }

        fState.name = strdup(clientName);

        // tell backend we're live
        {
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPong);
            fShmNonRtServerControl.commitWrite();
        }

        startThread(10);

        return true;
    }

    bool close()
    {
        carla_debug("CarlaEnginePlugin::close()");
        fLastPingTime = -1;

        stopThread(5000);
        clear();

        for (LinkedList<JackPortState*>::Itenerator it = fState.audioIns.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        for (LinkedList<JackPortState*>::Itenerator it = fState.audioOuts.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        fState.audioIns.clear();
        fState.audioOuts.clear();

        return true;
    }

    void clear() noexcept
    {
        fShmAudioPool.clear();
        fShmRtClientControl.clear();
        fShmNonRtClientControl.clear();
        fShmNonRtServerControl.clear();
    }

    void activate()
    {
        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
        const bool wasFirstIdle(fFirstIdle);

        if (wasFirstIdle)
        {
            fFirstIdle = false;
            fLastPingTime = Time::currentTimeMillis();
            CARLA_SAFE_ASSERT(fLastPingTime > 0);

            if (fState.audioIns.count() == 0 && fState.audioOuts.count() == 0)
            {
                carla_stderr("Create 2 ins, 2 outs prematurely for the client");
                fState.audioIns.append(new JackPortState("in_1", 0, JackPortIsOutput));
                fState.audioIns.append(new JackPortState("in_2", 1, JackPortIsOutput));
                fState.fakeIns = 2;

                fState.audioOuts.append(new JackPortState("out_1", 0, JackPortIsInput));
                fState.audioOuts.append(new JackPortState("out_2", 1, JackPortIsInput));
                fState.fakeOuts = 2;
            }

            char bufStr[STR_MAX+1];
            uint32_t bufStrSize;

            // kPluginBridgeNonRtServerPluginInfo1
            {
                // uint/category, uint/hints, uint/optionsAvailable, uint/optionsEnabled, long/uniqueId
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPluginInfo1);
                fShmNonRtServerControl.writeUInt(PLUGIN_CATEGORY_NONE);
                fShmNonRtServerControl.writeUInt(0x0);
                fShmNonRtServerControl.writeUInt(0x0);
                fShmNonRtServerControl.writeUInt(0x0);
                fShmNonRtServerControl.writeLong(0);
                fShmNonRtServerControl.commitWrite();
            }

            // kPluginBridgeNonRtServerPluginInfo2
            {
                // uint/size, str[] (realName), uint/size, str[] (label), uint/size, str[] (maker), uint/size, str[] (copyright)
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPluginInfo2);

                carla_zeroChars(bufStr, STR_MAX);
                std::strncpy(bufStr, fState.name, 64);
                bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                carla_zeroChars(bufStr, STR_MAX);

                bufStrSize = 1;
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                fShmNonRtServerControl.commitWrite();
            }

            // kPluginBridgeNonRtServerAudioCount
            {
                const uint32_t aIns  = fState.audioIns.count();
                const uint32_t aOuts = fState.audioOuts.count();

                // uint/ins, uint/outs
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerAudioCount);
                fShmNonRtServerControl.writeUInt(aIns);
                fShmNonRtServerControl.writeUInt(aOuts);
                fShmNonRtServerControl.commitWrite();

                // kPluginBridgeNonRtServerPortName
                for (uint32_t i=0; i<aIns; ++i)
                {
                    const JackPortState* const jport(fState.audioIns.getAt(i, nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                    const char* const portName(jport->name);
                    CARLA_SAFE_ASSERT_CONTINUE(portName != nullptr && portName[0] != '\0');

                    // byte/type, uint/index, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPortName);
                    fShmNonRtServerControl.writeByte(kPluginBridgePortAudioInput);
                    fShmNonRtServerControl.writeUInt(i);

                    bufStrSize = static_cast<uint32_t>(std::strlen(portName));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(portName, bufStrSize);
                }

                // kPluginBridgeNonRtServerPortName
                for (uint32_t i=0; i<aOuts; ++i)
                {
                    const JackPortState* const jport(fState.audioOuts.getAt(i, nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                    const char* const portName(jport->name);
                    CARLA_SAFE_ASSERT_CONTINUE(portName != nullptr && portName[0] != '\0');

                    // byte/type, uint/index, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPortName);
                    fShmNonRtServerControl.writeByte(kPluginBridgePortAudioOutput);
                    fShmNonRtServerControl.writeUInt(i);

                    bufStrSize = static_cast<uint32_t>(std::strlen(portName));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(portName, bufStrSize);
                }
            }

            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // kPluginBridgeNonRtServerMidiCount
            {
                const uint32_t mIns  = fState.midiIns.count();
                const uint32_t mOuts = fState.midiOuts.count();

                // uint/ins, uint/outs
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerMidiCount);
                fShmNonRtServerControl.writeUInt(mIns);
                fShmNonRtServerControl.writeUInt(mOuts);
                fShmNonRtServerControl.commitWrite();
            }

            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // ready!
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerReady);
            fShmNonRtServerControl.commitWrite();
            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // must be static
            fAudioIns = fState.audioIns.count();
            fAudioOuts = fState.audioOuts.count();

            carla_stdout("Carla Jack Client Ready!");
            fLastPingTime = Time::currentTimeMillis();
        }

        fState.activated = true;
    }

    void deactivate()
    {
        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

        fState.activated = false;

        for (LinkedList<JackPortState*>::Itenerator it = fState.audioIns.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        for (LinkedList<JackPortState*>::Itenerator it = fState.audioOuts.begin2(); it.valid(); it.next())
        {
            if (JackPortState* const jport = it.getValue(nullptr))
                delete jport;
        }

        fState.audioIns.clear();
        fState.audioOuts.clear();
    }

    void handleNonRtData()
    {
        for (; fShmNonRtClientControl.isDataAvailableForReading();)
        {
            const PluginBridgeNonRtClientOpcode opcode(fShmNonRtClientControl.readOpcode());

// #ifdef DEBUG
            if (opcode != kPluginBridgeNonRtClientPing)
            {
                static int shownNull = 0;
                if (opcode == kPluginBridgeNonRtClientNull)
                {
                    if (shownNull > 5)
                        continue;
                    ++shownNull;
                }
                carla_stdout("CarlaJackClient::handleNonRtData() - got opcode: %s", PluginBridgeNonRtClientOpcode2str(opcode));
            }
// #endif

            if (opcode != kPluginBridgeNonRtClientNull && opcode != kPluginBridgeNonRtClientPingOnOff && fLastPingTime > 0)
                fLastPingTime = Time::currentTimeMillis();

            switch (opcode)
            {
            case kPluginBridgeNonRtClientNull:
                break;

            case kPluginBridgeNonRtClientPing: {
                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPong);
                fShmNonRtServerControl.commitWrite();
            }   break;

            case kPluginBridgeNonRtClientPingOnOff: {
                const uint32_t onOff(fShmNonRtClientControl.readBool());

                fLastPingTime = onOff ? Time::currentTimeMillis() : -1;
            }   break;

            case kPluginBridgeNonRtClientActivate:
            case kPluginBridgeNonRtClientDeactivate:
                break;

            case kPluginBridgeNonRtClientSetBufferSize:
                fShmNonRtClientControl.readUInt();
                //bufferSizeChanged();
                break;

            case kPluginBridgeNonRtClientSetSampleRate:
                fShmNonRtClientControl.readDouble();
                //sampleRateChanged();
                break;

            case kPluginBridgeNonRtClientSetOffline:
                fIsOffline = true;
                //offlineModeChanged(true);
                break;

            case kPluginBridgeNonRtClientSetOnline:
                fIsOffline = false;
                //offlineModeChanged(false);
                break;

            case kPluginBridgeNonRtClientSetParameterValue:
            case kPluginBridgeNonRtClientSetParameterMidiChannel:
            case kPluginBridgeNonRtClientSetParameterMidiCC:
            case kPluginBridgeNonRtClientSetProgram:
            case kPluginBridgeNonRtClientSetMidiProgram:
            case kPluginBridgeNonRtClientSetCustomData:
            case kPluginBridgeNonRtClientSetChunkDataFile:
                break;

            case kPluginBridgeNonRtClientSetOption:
                fShmNonRtClientControl.readUInt();
                fShmNonRtClientControl.readBool();
                break;

            case kPluginBridgeNonRtClientSetCtrlChannel:
                fShmNonRtClientControl.readShort();
                break;

            case kPluginBridgeNonRtClientPrepareForSave:
                {
                    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSaved);
                    fShmNonRtServerControl.commitWrite();
                }
                break;

            case kPluginBridgeNonRtClientShowUI:
            case kPluginBridgeNonRtClientHideUI:
            case kPluginBridgeNonRtClientUiParameterChange:
            case kPluginBridgeNonRtClientUiProgramChange:
            case kPluginBridgeNonRtClientUiMidiProgramChange:
            case kPluginBridgeNonRtClientUiNoteOn:
            case kPluginBridgeNonRtClientUiNoteOff:
                break;

            case kPluginBridgeNonRtClientQuit:
                signalThreadShouldExit();
                break;
            }
        }
    }

    // -------------------------------------------------------------------

protected:
    void run() override
    {
        carla_stderr("CarlaJackClient run START");

#ifdef __SSE2_MATH__
        // Set FTZ and DAZ flags
        _mm_setcsr(_mm_getcsr() | 0x8040);
#endif

        bool quitReceived = false;

        for (; ! quitReceived && ! threadShouldExit();)
        {
            handleNonRtData();

            const BridgeRtClientControl::WaitHelper helper(fShmRtClientControl);

            if (! helper.ok)
                continue;

            for (; fShmRtClientControl.isDataAvailableForReading();)
            {
                const PluginBridgeRtClientOpcode opcode(fShmRtClientControl.readOpcode());
//#ifdef DEBUG
                if (opcode != kPluginBridgeRtClientProcess && opcode != kPluginBridgeRtClientMidiEvent)
                {
                    carla_stdout("CarlaJackClientRtThread::run() - got opcode: %s", PluginBridgeRtClientOpcode2str(opcode));
                }
//#endif

                switch (opcode)
                {
                case kPluginBridgeRtClientNull:
                    break;

                case kPluginBridgeRtClientSetAudioPool: {
                    if (fShmAudioPool.data != nullptr)
                    {
                        jackbridge_shm_unmap(fShmAudioPool.shm, fShmAudioPool.data);
                        fShmAudioPool.data = nullptr;
                    }
                    const uint64_t poolSize(fShmRtClientControl.readULong());
                    CARLA_SAFE_ASSERT_BREAK(poolSize > 0);
                    fShmAudioPool.data = (float*)jackbridge_shm_map(fShmAudioPool.shm, static_cast<size_t>(poolSize));
                    break;
                }

                case kPluginBridgeRtClientControlEventParameter:
                case kPluginBridgeRtClientControlEventMidiBank:
                case kPluginBridgeRtClientControlEventMidiProgram:
                case kPluginBridgeRtClientControlEventAllSoundOff:
                case kPluginBridgeRtClientControlEventAllNotesOff:
                case kPluginBridgeRtClientMidiEvent:
                    break;

                case kPluginBridgeRtClientProcess: {
                    CARLA_SAFE_ASSERT_BREAK(fShmAudioPool.data != nullptr);

                    const CarlaMutexTryLocker cmtl(fRealtimeThreadMutex);

                    if (cmtl.wasLocked())
                    {
                        float* fdata = fShmAudioPool.data;

                        if (fState.process == nullptr || ! fState.activated)
                        {
                            for (uint32_t i=0; i<fAudioIns; ++i)
                                fdata += fState.bufferSize*fAudioIns;

                            if (fAudioOuts > 0)
                                carla_zeroFloats(fdata, fState.bufferSize*fAudioOuts);
                        }
                        else
                        {
                            const BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

                            uint32_t i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it = fState.audioIns.begin2(); it.valid(); it.next())
                            {
                                CARLA_SAFE_ASSERT_BREAK(i++ < fAudioIns);
                                if (JackPortState* const jport = it.getValue(nullptr))
                                    jport->buffer = fdata;
                                fdata += fState.bufferSize;
                            }

                            i=0;
                            for (LinkedList<JackPortState*>::Itenerator it = fState.audioOuts.begin2(); it.valid(); it.next())
                            {
                                CARLA_SAFE_ASSERT_BREAK(i++ < fAudioOuts);
                                if (JackPortState* const jport = it.getValue(nullptr))
                                    jport->buffer = fdata;
                                fdata += fState.bufferSize;
                            }

                            fState.playing        = bridgeTimeInfo.playing;
                            fState.position.frame = bridgeTimeInfo.frame;
                            fState.position.usecs = bridgeTimeInfo.usecs;

                            if (bridgeTimeInfo.valid & 0x1 /* kValidBBT */)
                            {
                                fState.position.valid = JackPositionBBT;

                                fState.position.bar  = bridgeTimeInfo.bar;
                                fState.position.beat = bridgeTimeInfo.beat;
                                fState.position.tick = bridgeTimeInfo.tick;

                                fState.position.beats_per_bar = bridgeTimeInfo.beatsPerBar;
                                fState.position.beat_type     = bridgeTimeInfo.beatType;

                                fState.position.ticks_per_beat   = bridgeTimeInfo.ticksPerBeat;
                                fState.position.beats_per_minute = bridgeTimeInfo.beatsPerMinute;
                                fState.position.bar_start_tick   = bridgeTimeInfo.barStartTick;
                            }
                            else
                            {
                                fState.position.valid = static_cast<jack_position_bits_t>(0);
                            }

                            fState.process(fState.bufferSize, fState.processPtr);
                        }
                    }
                    else
                    {
                        carla_stderr2("CarlaJackClient: fRealtimeThreadMutex tryLock failed");
                    }

                    carla_zeroBytes(fShmRtClientControl.data->midiOut, kBridgeRtClientDataMidiOutSize);
                    break;
                }

                case kPluginBridgeRtClientQuit:
                    quitReceived = true;
                    signalThreadShouldExit();
                    break;
                }
            }
        }

        carla_stderr("CarlaJackClient run END");

        //callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);

        if (! quitReceived)
        {
            const char* const message("Plugin bridge error, process thread has stopped");
            const std::size_t messageSize(std::strlen(message));

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
            fShmNonRtServerControl.writeUInt(messageSize);
            fShmNonRtServerControl.writeCustomData(message, messageSize);
            fShmNonRtServerControl.commitWrite();
        }

        if (fState.shutdown != nullptr)
            fState.shutdown(fState.shutdownPtr);
    }

private:
    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    CarlaString fBaseNameAudioPool;
    CarlaString fBaseNameRtClientControl;
    CarlaString fBaseNameNonRtClientControl;
    CarlaString fBaseNameNonRtServerControl;

    bool fIsOffline;
    bool fFirstIdle;
    int64_t fLastPingTime;

    uint32_t fAudioIns;
    uint32_t fAudioOuts;

    CarlaMutex fRealtimeThreadMutex;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaJackClient)
};

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_USE_NAMESPACE

CarlaJackClient* global_client = nullptr;

CARLA_EXPORT
jack_client_t* jack_client_open(const char* client_name, jack_options_t /*options*/, jack_status_t* status, ...)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    if (global_client != nullptr)
        return (jack_client_t*)global_client;

    const char* const shmIds(std::getenv("CARLA_SHM_IDS"));

    if (shmIds == nullptr || std::strlen(shmIds) != 6*4)
    {
        if (status != nullptr)
            *status = JackFailure;
        return nullptr;
    }

    char audioPoolBaseName[6+1];
    char rtClientBaseName[6+1];
    char nonRtClientBaseName[6+1];
    char nonRtServerBaseName[6+1];

    std::memcpy(audioPoolBaseName,   shmIds+6*0, 6);
    std::memcpy(rtClientBaseName,    shmIds+6*1, 6);
    std::memcpy(nonRtClientBaseName, shmIds+6*2, 6);
    std::memcpy(nonRtServerBaseName, shmIds+6*3, 6);

    audioPoolBaseName[6]   = '\0';
    rtClientBaseName[6]    = '\0';
    nonRtClientBaseName[6] = '\0';
    nonRtServerBaseName[6] = '\0';

    CarlaJackClient* const client = new CarlaJackClient(audioPoolBaseName, rtClientBaseName,
                                                        nonRtClientBaseName, nonRtServerBaseName);

    if (! client->init(client_name))
    {
        if (status != nullptr)
            *status = JackServerError;
        return nullptr;
    }

    global_client = client;
    return (jack_client_t*)client;
}

CARLA_EXPORT
jack_client_t* jack_client_new(const char* client_name)
{
    return jack_client_open(client_name, JackNullOption, nullptr);
}

CARLA_EXPORT
int jack_client_close(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    JackClientState& jstate(jclient->fState);

    if (jstate.activated)
    {
        jclient->deactivate();
    }

    return 0;

    jclient->close();
    delete jclient;

    return 0;
}

CARLA_EXPORT
jack_port_t* jack_port_register(jack_client_t* client, const char* port_name, const char* port_type,
                                unsigned long flags, unsigned long /*buffer_size*/)
{
    carla_stdout("CarlaJackClient :: %s | %s %s %lu", __FUNCTION__, port_name, port_type, flags);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    JackClientState& jstate(jclient->fState);
    const bool isActivated(jstate.activated);

    //CARLA_SAFE_ASSERT(! isActivated);

    CARLA_SAFE_ASSERT_RETURN(port_name != nullptr && port_name[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(port_type != nullptr && port_type[0] != '\0', nullptr);

    if (std::strcmp(port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
    {
        uint32_t index;

        /**/ if (flags & JackPortIsInput)
        {
            if (isActivated)
            {
                CARLA_SAFE_ASSERT_RETURN(jstate.fakeIns > 0, nullptr);
                jstate.fakeIns -= 1;
                index = jstate.audioIns.count() - jstate.fakeIns - 1;
            }
            else
            {
                index = jstate.audioIns.count();
                jstate.audioIns.append(new JackPortState(port_name, index, flags));
            }

            return (jack_port_t*)jstate.audioIns.getAt(index, nullptr);
        }
        else if (flags & JackPortIsOutput)
        {
            if (isActivated)
            {
                CARLA_SAFE_ASSERT_RETURN(jstate.fakeOuts > 0, nullptr);
                jstate.fakeOuts -= 1;
                index = jstate.audioOuts.count() - jstate.fakeOuts - 1;
            }
            else
            {
                index = jstate.audioOuts.count();
                jstate.audioOuts.append(new JackPortState(port_name, index, flags));
            }

            return (jack_port_t*)jstate.audioOuts.getAt(index, nullptr);
        }

        carla_stderr2("Invalid port flags '%x'", flags);
        return nullptr;
    }

    carla_stderr2("Invalid port type '%s'", port_type);
    return nullptr;
}

CARLA_EXPORT
int jack_port_unregister(jack_client_t* client, jack_port_t* port)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 1);

    JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    if (jport->flags & JackPortIsOutput)
    {
        CARLA_SAFE_ASSERT_RETURN(jstate.audioIns.removeOne(jport), 1);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(jstate.audioOuts.removeOne(jport), 1);
    }

    return 0;
}

CARLA_EXPORT
void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t)
{
    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->buffer;
}

CARLA_EXPORT
int jack_set_process_callback(jack_client_t* client, JackProcessCallback callback, void* arg)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    jstate.process = callback;
    jstate.processPtr = arg;

    return 0;
}

CARLA_EXPORT
void jack_on_shutdown(jack_client_t* client, JackShutdownCallback callback, void* arg)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr,);

    JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated,);

    jstate.shutdown = callback;
    jstate.shutdownPtr = arg;
}

CARLA_EXPORT
int jack_activate(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

#if 0
    // needed for pulseaudio
    static bool skipFirstActivate = true;
    if (skipFirstActivate) {
        skipFirstActivate = false;
        return 0;
    }
#endif

    jclient->activate();
    return 0;
}

CARLA_EXPORT
int jack_deactivate(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(jstate.activated, 1);

    jclient->deactivate();
    return 0;
}

CARLA_EXPORT
char* jack_get_client_name(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    const JackClientState& jstate(jclient->fState);

    return jstate.name;
}

CARLA_EXPORT
jack_nframes_t jack_get_buffer_size(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackClientState& jstate(jclient->fState);

    return jstate.bufferSize;
}

CARLA_EXPORT
jack_nframes_t jack_get_sample_rate(jack_client_t* client)
{
    carla_debug("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackClientState& jstate(jclient->fState);

    return jstate.sampleRate;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
void jack_set_error_function(void (*func)(const char *))
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    (void)func;
}

CARLA_EXPORT
int jack_set_sync_callback(jack_client_t* client, JackSyncCallback /*callback*/, void* /*arg*/)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    // TODO

    return 0;
}

CARLA_EXPORT
int jack_set_timebase_callback(jack_client_t* client, int, JackTimebaseCallback, void*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    // TODO

    return EBUSY;
}

CARLA_EXPORT
int jack_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback /*callback*/, void* /*arg*/)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    // TODO

    return 0;
}

CARLA_EXPORT
int jack_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback /*callback*/, void* /*arg*/)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    // TODO

    return 0;
}

CARLA_EXPORT
int jack_set_client_registration_callback(jack_client_t* client, JackClientRegistrationCallback, void*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);


    return 0;
}

CARLA_EXPORT
int jack_set_port_registration_callback(jack_client_t* client, JackPortRegistrationCallback, void*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    return 0;
}

CARLA_EXPORT
int jack_set_port_connect_callback(jack_client_t* client, JackPortConnectCallback, void*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    return 0;
}

CARLA_EXPORT
int jack_set_graph_order_callback(jack_client_t* client, JackGraphOrderCallback, void*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    return 0;
}

CARLA_EXPORT
int jack_is_realtime(jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 0;
}

CARLA_EXPORT
int jack_transport_locate(jack_client_t*, jack_nframes_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 1;
}

CARLA_EXPORT
jack_transport_state_t jack_transport_query(const jack_client_t* client, jack_position_t* pos)
{
    carla_debug("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, JackTransportStopped);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(jstate.activated, JackTransportStopped);

    if (pos != nullptr)
        std::memcpy(pos, &jstate.position, sizeof(jack_position_t));

    return jstate.playing ? JackTransportRolling : JackTransportStopped;
}

CARLA_EXPORT
int jack_transport_reposition(jack_client_t*, const jack_position_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return EINVAL;
}

CARLA_EXPORT
void jack_transport_start(jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

}

CARLA_EXPORT
void jack_transport_stop (jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

}

CARLA_EXPORT
pthread_t jack_client_thread_id(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(jstate.activated, 0);

    return (pthread_t)jclient->getThreadId();
}

CARLA_EXPORT
jack_nframes_t jack_frame_time(const jack_client_t* client)
{
    carla_debug("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(jstate.activated, 0);

    return jstate.position.usecs;
}

CARLA_EXPORT
void jack_free(void* ptr)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    free(ptr);
}

CARLA_EXPORT
jack_nframes_t jack_midi_get_event_count(void*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 0;
}


CARLA_EXPORT
int jack_midi_event_get(jack_midi_event_t*, void*, uint32_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return ENODATA;
}

CARLA_EXPORT
const char* jack_port_short_name(const jack_port_t* port)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->name;
}

CARLA_EXPORT
const char* jack_port_name(const jack_port_t* port)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->fullname;
}

CARLA_EXPORT
int jack_port_flags(const jack_port_t* port)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->flags;
}

CARLA_EXPORT
const char* jack_port_type(const jack_port_t* port)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    // TODO

    return JACK_DEFAULT_AUDIO_TYPE;
}

CARLA_EXPORT
int jack_client_real_time_priority(jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return -1;
}

CARLA_EXPORT
int jack_connect(jack_client_t*, const char*, const char*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 0;
}

CARLA_EXPORT
int jack_disconnect(jack_client_t*, const char*, const char*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 0;
}

CARLA_EXPORT
const char** jack_get_ports(jack_client_t*, const char* a, const char* b, unsigned long flags)
{
    carla_stdout("CarlaJackClient :: %s | %s %s %li", __FUNCTION__, a, b, flags);

    static const char* capture_1  = "system:capture_1";
    static const char* capture_2  = "system:capture_2";
    static const char* playback_1 = "system:playback_1";
    static const char* playback_2 = "system:playback_2";

    if (flags == 0 || (flags & (JackPortIsInput|JackPortIsOutput)) == (JackPortIsInput|JackPortIsOutput))
    {
        if (const char** const ret = (const char**)calloc(5, sizeof(const char*)))
        {
            ret[0] = capture_1;
            ret[1] = capture_2;
            ret[2] = playback_1;
            ret[3] = playback_2;
            ret[4] = nullptr;
            return ret;
        }
    }

    if (flags & JackPortIsInput)
    {
        if (const char** const ret = (const char**)calloc(3, sizeof(const char*)))
        {
            ret[0] = playback_1;
            ret[1] = playback_2;
            ret[2] = nullptr;
            return ret;
        }
    }

    if (flags & JackPortIsOutput)
    {
        if (const char** const ret = (const char**)calloc(3, sizeof(const char*)))
        {
            ret[0] = capture_1;
            ret[1] = capture_2;
            ret[2] = nullptr;
            return ret;
        }
    }

    return nullptr;
}

CARLA_EXPORT
jack_port_t* jack_port_by_name(jack_client_t*, const char*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return nullptr;
}

CARLA_EXPORT
jack_port_t* jack_port_by_id(jack_client_t*, jack_port_id_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return nullptr;
}

CARLA_EXPORT
void jack_set_info_function(void (*func)(const char *))
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    (void)func;
}

CARLA_EXPORT
int jack_set_freewheel(jack_client_t*, int)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 1;
}

CARLA_EXPORT
int jack_set_buffer_size(jack_client_t*, jack_nframes_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 1;
}

CARLA_EXPORT
int jack_engine_takeover_timebase(jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return ENOSYS;
}

CARLA_EXPORT
float jack_cpu_load(jack_client_t*)
{
    return 0.0f;
}

CARLA_EXPORT
int jack_client_name_size(void)
{
    return STR_MAX;
}

CARLA_EXPORT
void jack_port_get_latency_range(jack_port_t*, jack_latency_callback_mode_t, jack_latency_range_t* range)
{
    range->min = range->max = 0;
}

CARLA_EXPORT
jack_nframes_t jack_port_get_latency(jack_port_t*)
{
    return 0;
}

CARLA_EXPORT
int jack_set_xrun_callback(jack_client_t*, JackXRunCallback, void*)
{
    return 0;
}

CARLA_EXPORT
int jack_set_thread_init_callback(jack_client_t*, JackThreadInitCallback, void*)
{
    return 0;
}

CARLA_EXPORT
int jack_port_name_size(void)
{
    return STR_MAX;
}

CARLA_EXPORT
const char* JACK_METADATA_PRETTY_NAME;

CARLA_EXPORT
const char* JACK_METADATA_PRETTY_NAME = "http://jackaudio.org/metadata/pretty-name";

// jack_ringbuffer_create
// jack_port_connected
// jack_port_is_mine
// jack_port_set_name
// jack_port_get_all_connections
// jack_port_uuid

// --------------------------------------------------------------------------------------------------------------------

#include "jackbridge/JackBridge2.cpp"
#include "CarlaBridgeUtils.cpp"

// --------------------------------------------------------------------------------------------------------------------
