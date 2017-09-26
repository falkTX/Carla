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
#include "libjack.hpp"

using juce::File;
using juce::MemoryBlock;
using juce::String;
using juce::Time;
using juce::Thread;

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CarlaJackClient::CarlaJackClient()
    : Thread("CarlaJackClient"),
      fState(),
      fIsValid(false),
      fIsOffline(false),
      fFirstIdle(true),
      fLastPingTime(-1),
      fAudioIns(0),
      fAudioOuts(0)
{
    carla_debug("CarlaJackClient::CarlaJackClient(\"%s\", \"%s\", \"%s\", \"%s\")", audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);

    const char* const shmIds(std::getenv("CARLA_SHM_IDS"));
    CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && std::strlen(shmIds) == 6*4,);

    std::memcpy(fBaseNameAudioPool,          shmIds+6*0, 6);
    std::memcpy(fBaseNameRtClientControl,    shmIds+6*1, 6);
    std::memcpy(fBaseNameNonRtClientControl, shmIds+6*2, 6);
    std::memcpy(fBaseNameNonRtServerControl, shmIds+6*3, 6);

    fBaseNameAudioPool[6]          = '\0';
    fBaseNameRtClientControl[6]    = '\0';
    fBaseNameNonRtClientControl[6] = '\0';
    fBaseNameNonRtServerControl[6] = '\0';

    startThread(10);
}

CarlaJackClient::~CarlaJackClient() noexcept
{
    carla_debug("CarlaJackClient::~CarlaJackClient()");

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
}

bool CarlaJackClient::initIfNeeded(const char* const clientName)
{
    carla_debug("CarlaJackClient::initIfNeeded(\"%s\")", clientName);

    if (fState.name == nullptr)
        fState.name = strdup(clientName);

    return fIsValid;
}

void CarlaJackClient::clear() noexcept
{
    fShmAudioPool.clear();
    fShmRtClientControl.clear();
    fShmNonRtClientControl.clear();
    fShmNonRtServerControl.clear();
}

bool CarlaJackClient::isValid() const noexcept
{
    return fIsValid;
}

void CarlaJackClient::activate()
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
            fState.audioIns.append(new JackPortState(fState.name, "in_1", 0, JackPortIsOutput, false));
            fState.audioIns.append(new JackPortState(fState.name, "in_2", 1, JackPortIsOutput, false));
            fState.fakeIns = 2;

            fState.audioOuts.append(new JackPortState(fState.name, "out_1", 0, JackPortIsInput, false));
            fState.audioOuts.append(new JackPortState(fState.name, "out_2", 1, JackPortIsInput, false));
            fState.fakeOuts = 2;

            fState.prematurelyActivated = true;
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

void CarlaJackClient::deactivate()
{
    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
    fShmNonRtServerControl.commitWrite();

    fState.activated = false;
    fState.prematurelyActivated = false;

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

void CarlaJackClient::handleNonRtData()
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

void CarlaJackClient::run()
{
    carla_stderr("CarlaJackClient run START");

    if (! fShmAudioPool.attachClient(fBaseNameAudioPool))
    {
        carla_stderr("Failed to attach to audio pool shared memory");
        return;
    }

    if (! fShmRtClientControl.attachClient(fBaseNameRtClientControl))
    {
        clear();
        carla_stderr("Failed to attach to rt client control shared memory");
        return;
    }

    if (! fShmRtClientControl.mapData())
    {
        clear();
        carla_stderr("Failed to map rt client control shared memory");
        return;
    }

    if (! fShmNonRtClientControl.attachClient(fBaseNameNonRtClientControl))
    {
        clear();
        carla_stderr("Failed to attach to non-rt client control shared memory");
        return;
    }

    if (! fShmNonRtClientControl.mapData())
    {
        clear();
        carla_stderr("Failed to map non-rt control client shared memory");
        return;
    }

    if (! fShmNonRtServerControl.attachClient(fBaseNameNonRtServerControl))
    {
        clear();
        carla_stderr("Failed to attach to non-rt server control shared memory");
        return;
    }

    if (! fShmNonRtServerControl.mapData())
    {
        clear();
        carla_stderr("Failed to map non-rt control server shared memory");
        return;
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
        return;
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
        return;
    }

    // tell backend we're live
    {
        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPong);
        fShmNonRtServerControl.commitWrite();
    }

    fIsValid = true;

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

                        if (! fState.activated)
                        {
                            fShmRtClientControl.data->procFlags = 1;
                        }
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

    //callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);

    if (quitReceived)
    {
        carla_stderr("CarlaJackClient run END - quit by carla");

        ::kill(::getpid(), SIGTERM);
    }
    else
    {
        const char* const message("Plugin bridge error, process thread has stopped");
        const std::size_t messageSize(std::strlen(message));

        bool activated;

        {
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            activated = fState.activated;

            if (activated)
            {
                carla_stderr("CarlaJackClient run END - quit error");

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
                fShmNonRtServerControl.writeUInt(messageSize);
                fShmNonRtServerControl.writeCustomData(message, messageSize);
                fShmNonRtServerControl.commitWrite();
            }
            else
            {
                carla_stderr("CarlaJackClient run END - quit itself");
            }
        }

        if (activated && fState.shutdown != nullptr)
            fState.shutdown(fState.shutdownPtr);
    }
}

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_USE_NAMESPACE

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
int jack_client_real_time_priority(jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return -1;
}

// --------------------------------------------------------------------------------------------------------------------

#include "jackbridge/JackBridge2.cpp"
#include "CarlaBridgeUtils.cpp"

// --------------------------------------------------------------------------------------------------------------------
