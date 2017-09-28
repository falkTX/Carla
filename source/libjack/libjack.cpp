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

class CarlaJackAppClient : public juce::Thread
{
public:
    JackServerState fServer;
    LinkedList<JackClientState*> fClients;

    CarlaJackAppClient()
        : Thread("CarlaJackAppClient"),
          fServer(),
          fIsValid(false),
          fIsOffline(false),
          fLastPingTime(-1)
    {
        carla_debug("CarlaJackAppClient::CarlaJackAppClient()");

        const char* const shmIds(std::getenv("CARLA_SHM_IDS"));
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && std::strlen(shmIds) == 6*4,);

        const char* const libjackSetup(std::getenv("CARLA_LIBJACK_SETUP"));
        CARLA_SAFE_ASSERT_RETURN(libjackSetup != nullptr && std::strlen(libjackSetup) == 5,);

        for (int i=4; --i >= 0;) {
            CARLA_SAFE_ASSERT_RETURN(libjackSetup[i] >= '0' && libjackSetup[i] <= '0'+64,);
        }
        CARLA_SAFE_ASSERT_RETURN(libjackSetup[4] >= '0' && libjackSetup[4] < '0'+0x4f,);

        std::memcpy(fBaseNameAudioPool,          shmIds+6*0, 6);
        std::memcpy(fBaseNameRtClientControl,    shmIds+6*1, 6);
        std::memcpy(fBaseNameNonRtClientControl, shmIds+6*2, 6);
        std::memcpy(fBaseNameNonRtServerControl, shmIds+6*3, 6);

        fBaseNameAudioPool[6]          = '\0';
        fBaseNameRtClientControl[6]    = '\0';
        fBaseNameNonRtClientControl[6] = '\0';
        fBaseNameNonRtServerControl[6] = '\0';

        fNumPorts.audioIns  = libjackSetup[0] - '0';
        fNumPorts.audioOuts = libjackSetup[1] - '0';
        fNumPorts.midiIns   = libjackSetup[2] - '0';
        fNumPorts.midiOuts  = libjackSetup[3] - '0';

        startThread(10);
    }

    ~CarlaJackAppClient() noexcept override
    {
        carla_debug("CarlaJackAppClient::~CarlaJackAppClient()");

        fLastPingTime = -1;

        stopThread(5000);
        clear();

        const CarlaMutexLocker cms(fRealtimeThreadMutex);

        for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
        {
            JackClientState* const jclient(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

            delete jclient;
        }

        fClients.clear();
    }

    JackClientState* addClient(const char* const name)
    {
        JackClientState* const jclient(new JackClientState(fServer, name));

        const CarlaMutexLocker cms(fRealtimeThreadMutex);
        fClients.append(jclient);
        return jclient;
    }

    bool removeClient(JackClientState* const jclient)
    {
        {
            const CarlaMutexLocker cms(fRealtimeThreadMutex);
            CARLA_SAFE_ASSERT_RETURN(fClients.removeOne(jclient), false);
        }

        delete jclient;
        return true;
    }

    void clear() noexcept;
    bool isValid() const noexcept;

    void handleNonRtData();

    // -------------------------------------------------------------------

protected:
    void run() override;

private:
    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    char fBaseNameAudioPool[6+1];
    char fBaseNameRtClientControl[6+1];
    char fBaseNameNonRtClientControl[6+1];
    char fBaseNameNonRtServerControl[6+1];

    bool fIsValid;
    bool fIsOffline;
    int64_t fLastPingTime;

    struct NumPorts {
        uint32_t audioIns;
        uint32_t audioOuts;
        uint32_t midiIns;
        uint32_t midiOuts;

        NumPorts()
            : audioIns(0),
              audioOuts(0),
              midiIns(0),
              midiOuts(0) {}
    } fNumPorts;

    CarlaMutex fRealtimeThreadMutex;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaJackAppClient)
};

// --------------------------------------------------------------------------------------------------------------------

void CarlaJackAppClient::clear() noexcept
{
    fShmAudioPool.clear();
    fShmRtClientControl.clear();
    fShmNonRtClientControl.clear();
    fShmNonRtServerControl.clear();
}

bool CarlaJackAppClient::isValid() const noexcept
{
    return fIsValid;
}

void CarlaJackAppClient::handleNonRtData()
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
            carla_stdout("CarlaJackAppClient::handleNonRtData() - got opcode: %s", PluginBridgeNonRtClientOpcode2str(opcode));
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

void CarlaJackAppClient::run()
{
    carla_stderr("CarlaJackAppClient run START");

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
        carla_stderr2("CarlaJackAppClient: data size mismatch");
        return;
    }

    opcode = fShmNonRtClientControl.readOpcode();
    CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetBufferSize, opcode);
    fServer.bufferSize = fShmNonRtClientControl.readUInt();

    opcode = fShmNonRtClientControl.readOpcode();
    CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetSampleRate, opcode);
    fServer.sampleRate = fShmNonRtClientControl.readDouble();

    if (fServer.bufferSize == 0 || carla_isZero(fServer.sampleRate))
    {
        carla_stderr2("CarlaJackAppClient: invalid empty state");
        return;
    }

    // tell backend we're live
    {
        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

        fLastPingTime = Time::currentTimeMillis();
        CARLA_SAFE_ASSERT(fLastPingTime > 0);

        // ready!
        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerReady);
        fShmNonRtServerControl.commitWrite();
        fShmNonRtServerControl.waitIfDataIsReachingLimit();
    }

    fIsValid = true;
    fLastPingTime = Time::currentTimeMillis();
    carla_stdout("Carla Jack Client Ready!");

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
                carla_stdout("CarlaJackAppClientRtThread::run() - got opcode: %s", PluginBridgeRtClientOpcode2str(opcode));
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

                    if (! fClients.isEmpty())
                    {
                        // tranport for all clients
                        const BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

                        fServer.playing        = bridgeTimeInfo.playing;
                        fServer.position.frame = bridgeTimeInfo.frame;
                        fServer.position.usecs = bridgeTimeInfo.usecs;

                        if (bridgeTimeInfo.valid & 0x1 /* kValidBBT */)
                        {
                            fServer.position.valid = JackPositionBBT;

                            fServer.position.bar  = bridgeTimeInfo.bar;
                            fServer.position.beat = bridgeTimeInfo.beat;
                            fServer.position.tick = bridgeTimeInfo.tick;

                            fServer.position.beats_per_bar = bridgeTimeInfo.beatsPerBar;
                            fServer.position.beat_type     = bridgeTimeInfo.beatType;

                            fServer.position.ticks_per_beat   = bridgeTimeInfo.ticksPerBeat;
                            fServer.position.beats_per_minute = bridgeTimeInfo.beatsPerMinute;
                            fServer.position.bar_start_tick   = bridgeTimeInfo.barStartTick;
                        }
                        else
                        {
                            fServer.position.valid = static_cast<jack_position_bits_t>(0);
                        }

                        for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
                        {
                            JackClientState* const jclient(it.getValue(nullptr));
                            CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                            const CarlaMutexTryLocker cmtl2(jclient->mutex);

                            if (cmtl2.wasNotLocked() || jclient->processCb == nullptr || ! jclient->activated)
                            {
                                if (fNumPorts.audioIns > 0)
                                    fdata += fServer.bufferSize*fNumPorts.audioIns;

                                if (fNumPorts.audioOuts > 0)
                                    carla_zeroFloats(fdata, fServer.bufferSize*fNumPorts.audioOuts);

                                if (jclient->deactivated)
                                {
                                    fShmRtClientControl.data->procFlags = 1;
                                }
                            }
                            else
                            {
                                uint32_t i;

                                i = 0;
                                for (LinkedList<JackPortState*>::Itenerator it = jclient->audioIns.begin2(); it.valid(); it.next())
                                {
                                    CARLA_SAFE_ASSERT_BREAK(i++ < fNumPorts.audioIns);
                                    if (JackPortState* const jport = it.getValue(nullptr))
                                        jport->buffer = fdata;
                                    fdata += fServer.bufferSize;
                                }
                                for (; i++ < fNumPorts.audioIns;)
                                    fdata += fServer.bufferSize;

                                i = 0;
                                for (LinkedList<JackPortState*>::Itenerator it = jclient->audioOuts.begin2(); it.valid(); it.next())
                                {
                                    CARLA_SAFE_ASSERT_BREAK(i++ < fNumPorts.audioOuts);
                                    if (JackPortState* const jport = it.getValue(nullptr))
                                        jport->buffer = fdata;
                                    fdata += fServer.bufferSize;
                                }
                                for (; i++ < fNumPorts.audioOuts;)
                                {
                                    carla_zeroFloats(fdata, fServer.bufferSize);
                                    fdata += fServer.bufferSize;
                                }

                                jclient->processCb(fServer.bufferSize, jclient->processCbPtr);
                            }
                        }
                    }
                    else
                    {
                        if (fNumPorts.audioIns > 0)
                            fdata += fServer.bufferSize*fNumPorts.audioIns;

                        if (fNumPorts.audioOuts > 0)
                            carla_zeroFloats(fdata, fServer.bufferSize*fNumPorts.audioOuts);
                    }
                }
                else
                {
                    carla_stderr2("CarlaJackAppClient: fRealtimeThreadMutex tryLock failed");
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
        carla_stderr("CarlaJackAppClient run END - quit by carla");

        ::kill(::getpid(), SIGTERM);
    }
    else
    {
        const char* const message("Plugin bridge error, process thread has stopped");
        const std::size_t messageSize(std::strlen(message));

        bool activated;

        {
            const CarlaMutexLocker cms(fRealtimeThreadMutex);

            if (fClients.isEmpty())
            {
                activated = false;
            }
            else if (JackClientState* const jclient = fClients.getLast(nullptr))
            {
                const CarlaMutexLocker cms(jclient->mutex);
                activated = jclient->activated;
            }
            else
            {
                activated = true;
            }
        }

        if (activated)
        {
            carla_stderr("CarlaJackAppClient run END - quit error");

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
            fShmNonRtServerControl.writeUInt(messageSize);
            fShmNonRtServerControl.writeCustomData(message, messageSize);
            fShmNonRtServerControl.commitWrite();
        }
        else
        {
            carla_stderr("CarlaJackAppClient run END - quit itself");

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
            fShmNonRtServerControl.commitWrite();
        }

        /*
        if (activated)
        {
            // TODO infoShutdown
            if (fClient.shutdownCb != nullptr)
                fClient.shutdownCb(fClient.shutdownCbPtr);
        }
        */
    }
}

// --------------------------------------------------------------------------------------------------------------------

static CarlaJackAppClient gClient;

CARLA_EXPORT
jack_client_t* jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
    if (status != nullptr)
        *status = JackNameNotUnique;

    if (options & JackUseExactName)
        return nullptr;

    if (JackClientState* const client = gClient.addClient(client_name))
        return (jack_client_t*)client;

    if (status != nullptr)
        *status = JackServerError;

    return nullptr;
}

CARLA_EXPORT
jack_client_t* jack_client_new(const char* client_name)
{
    return jack_client_open(client_name, JackNullOption, nullptr);
}

CARLA_EXPORT
int jack_client_close(jack_client_t* client)
{
    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    gClient.removeClient(jclient);
    return 0;
}

CARLA_EXPORT
pthread_t jack_client_thread_id(jack_client_t* client)
{
    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    CarlaJackAppClient* const jackAppPtr = jclient->server.jackAppPtr;
    CARLA_SAFE_ASSERT_RETURN(jackAppPtr != nullptr, 0);

    return (pthread_t)jackAppPtr->getThreadId();
}

CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

#include "jackbridge/JackBridge2.cpp"
#include "CarlaBridgeUtils.cpp"

// --------------------------------------------------------------------------------------------------------------------
// TODO

CARLA_BACKEND_USE_NAMESPACE

CARLA_EXPORT
int jack_client_real_time_priority(jack_client_t*)
{
    carla_stdout("CarlaJackAppClient :: %s", __FUNCTION__);

    return -1;
}

// --------------------------------------------------------------------------------------------------------------------
