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

#include "libjack.hpp"
#include <sys/prctl.h>

#include "CarlaThread.hpp"

using juce::FloatVectorOperations;
using juce::Thread;
using juce::Time;

typedef int (*CarlaInterposedCallback)(int, void*);

CARLA_EXPORT
int jack_carla_interposed_action(int, void*)
{
    carla_stderr2("Non-export jack_carla_interposed_action called, this should not happen!!");
    return 0;
}

CARLA_BACKEND_START_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------

class CarlaJackRealtimeThread : public Thread
{
public:
    struct Callback {
        Callback() {}
        virtual ~Callback() {};
        virtual void runRealtimeThread() = 0;
    };

    CarlaJackRealtimeThread(Callback* const callback)
        : Thread("CarlaJackRealtimeThread"),
          fCallback(callback) {}

protected:
    void run() override
    {
        fCallback->runRealtimeThread();
    }

private:
    Callback* const fCallback;
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaJackNonRealtimeThread : public Thread
{
public:
    struct Callback {
        Callback() {}
        virtual ~Callback() {};
        virtual void runNonRealtimeThread() = 0;
    };

    CarlaJackNonRealtimeThread(Callback* const callback)
        : Thread("CarlaJackNonRealtimeThread"),
          fCallback(callback) {}

protected:
    void run() override
    {
        fCallback->runNonRealtimeThread();
    }

private:
    Callback* const fCallback;
};

static int carla_interposed_callback(int, void*);

// ---------------------------------------------------------------------------------------------------------------------

class CarlaJackAppClient : public CarlaJackRealtimeThread::Callback,
                           public CarlaJackNonRealtimeThread::Callback
{
public:
    JackServerState fServer;
    LinkedList<JackClientState*> fClients;

    CarlaJackAppClient()
        : fServer(this),
          fAudioPoolCopy(nullptr),
          fAudioTmpBuf(nullptr),
          fDummyMidiInBuffer(true, "ignored"),
          fDummyMidiOutBuffer(false, "ignored"),
          fMidiInBuffers(nullptr),
          fMidiOutBuffers(nullptr),
          fIsOffline(false),
          fLastPingTime(-1),
          fRealtimeThread(this),
          fNonRealtimeThread(this)
    {
        carla_debug("CarlaJackAppClient::CarlaJackAppClient()");

        const char* const shmIds(std::getenv("CARLA_SHM_IDS"));
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr && std::strlen(shmIds) == 6*4,);

        const char* const libjackSetup(std::getenv("CARLA_LIBJACK_SETUP"));
        CARLA_SAFE_ASSERT_RETURN(libjackSetup != nullptr && std::strlen(libjackSetup) == 5,);

        // make sure we don't get loaded again
        carla_unsetenv("CARLA_SHM_IDS");

        // kill ourselves if main carla dies
        ::prctl(PR_SET_PDEATHSIG, SIGKILL);

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

        fServer.numAudioIns  = libjackSetup[0] - '0';
        fServer.numAudioOuts = libjackSetup[1] - '0';
        fServer.numMidiIns   = libjackSetup[2] - '0';
        fServer.numMidiOuts  = libjackSetup[3] - '0';

        jack_carla_interposed_action(1, (void*)carla_interposed_callback);

        fNonRealtimeThread.startThread();
    }

    ~CarlaJackAppClient() noexcept override
    {
        carla_debug("CarlaJackAppClient::~CarlaJackAppClient()");

        fLastPingTime = -1;

        fNonRealtimeThread.stopThread(5000);

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

    pthread_t getRealtimeThreadId() const noexcept
    {
        return (pthread_t)fRealtimeThread.getThreadId();
    }

    int handleInterposerCallback(const int cb_action, void* const ptr)
    {
        carla_stdout("handleInterposerCallback(%o, %p)", cb_action, ptr);

        switch (cb_action)
        {
        case 1: {
            const CarlaMutexLocker cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
            fShmNonRtServerControl.commitWrite();
            break;
        }
        }

        return 0;
    }

    // -------------------------------------------------------------------

protected:
    void runRealtimeThread() override;
    void runNonRealtimeThread() override;

private:
    bool initSharedMemmory();
    void clearSharedMemory() noexcept;

    bool handleRtData();
    bool handleNonRtData();

    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    float* fAudioPoolCopy;
    float* fAudioTmpBuf;

    JackMidiPortBuffer fDummyMidiInBuffer;
    JackMidiPortBuffer fDummyMidiOutBuffer;
    JackMidiPortBuffer* fMidiInBuffers;
    JackMidiPortBuffer* fMidiOutBuffers;

    char fBaseNameAudioPool[6+1];
    char fBaseNameRtClientControl[6+1];
    char fBaseNameNonRtClientControl[6+1];
    char fBaseNameNonRtServerControl[6+1];

    bool fIsOffline;
    int64_t fLastPingTime;

    CarlaJackRealtimeThread    fRealtimeThread;
    CarlaJackNonRealtimeThread fNonRealtimeThread;

    CarlaMutex fRealtimeThreadMutex;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaJackAppClient)
};

// ---------------------------------------------------------------------------------------------------------------------

bool CarlaJackAppClient::initSharedMemmory()
{
    if (! fShmAudioPool.attachClient(fBaseNameAudioPool))
    {
        carla_stderr("Failed to attach to audio pool shared memory");
        return false;
    }

    if (! fShmRtClientControl.attachClient(fBaseNameRtClientControl))
    {
        clearSharedMemory();
        carla_stderr("Failed to attach to rt client control shared memory");
        return false;
    }

    if (! fShmRtClientControl.mapData())
    {
        clearSharedMemory();
        carla_stderr("Failed to map rt client control shared memory");
        return false;
    }

    if (! fShmNonRtClientControl.attachClient(fBaseNameNonRtClientControl))
    {
        clearSharedMemory();
        carla_stderr("Failed to attach to non-rt client control shared memory");
        return false;
    }

    if (! fShmNonRtClientControl.mapData())
    {
        clearSharedMemory();
        carla_stderr("Failed to map non-rt control client shared memory");
        return false;
    }

    if (! fShmNonRtServerControl.attachClient(fBaseNameNonRtServerControl))
    {
        clearSharedMemory();
        carla_stderr("Failed to attach to non-rt server control shared memory");
        return false;
    }

    if (! fShmNonRtServerControl.mapData())
    {
        clearSharedMemory();
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
        carla_stderr2("CarlaJackAppClient: data size mismatch");
        return false;
    }

    opcode = fShmNonRtClientControl.readOpcode();
    CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientInitialSetup, opcode);
    fServer.bufferSize = fShmNonRtClientControl.readUInt();
    fServer.sampleRate = fShmNonRtClientControl.readDouble();

    if (fServer.bufferSize == 0 || carla_isZero(fServer.sampleRate))
    {
        carla_stderr2("CarlaJackAppClient: invalid empty state");
        return false;
    }

    fAudioTmpBuf = new float[fServer.bufferSize];
    FloatVectorOperations::clear(fAudioTmpBuf, fServer.bufferSize);

    // tell backend we're live
    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

    fLastPingTime = Time::currentTimeMillis();
    CARLA_SAFE_ASSERT(fLastPingTime > 0);

    // ready!
    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerReady);
    fShmNonRtServerControl.commitWrite();
    fShmNonRtServerControl.waitIfDataIsReachingLimit();

    return true;
}

void CarlaJackAppClient::clearSharedMemory() noexcept
{
    const CarlaMutexLocker cml(fRealtimeThreadMutex);

    if (fAudioPoolCopy != nullptr)
    {
        delete[] fAudioPoolCopy;
        fAudioPoolCopy = nullptr;
    }

    if (fAudioTmpBuf != nullptr)
    {
        delete[] fAudioTmpBuf;
        fAudioTmpBuf = nullptr;
    }

    if (fMidiInBuffers != nullptr)
    {
        delete[] fMidiInBuffers;
        fMidiInBuffers = nullptr;
    }

    if (fMidiOutBuffers != nullptr)
    {
        delete[] fMidiOutBuffers;
        fMidiOutBuffers = nullptr;
    }

    fShmAudioPool.clear();
    fShmRtClientControl.clear();
    fShmNonRtClientControl.clear();
    fShmNonRtServerControl.clear();
}

bool CarlaJackAppClient::handleRtData()
{
    const BridgeRtClientControl::WaitHelper helper(fShmRtClientControl);

    if (! helper.ok)
        return false;

    bool ret = false;

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
            const CarlaMutexLocker cml(fRealtimeThreadMutex);

            if (fShmAudioPool.data != nullptr)
            {
                jackbridge_shm_unmap(fShmAudioPool.shm, fShmAudioPool.data);
                fShmAudioPool.data = nullptr;
            }
            if (fAudioPoolCopy != nullptr)
            {
                delete[] fAudioPoolCopy;
                fAudioPoolCopy = nullptr;
            }
            const uint64_t poolSize(fShmRtClientControl.readULong());
            CARLA_SAFE_ASSERT_BREAK(poolSize > 0);
            fShmAudioPool.data = (float*)jackbridge_shm_map(fShmAudioPool.shm, static_cast<size_t>(poolSize));
            fAudioPoolCopy = new float[poolSize];
            break;
        }

        case kPluginBridgeRtClientSetBufferSize:
            if (const uint32_t newBufferSize = fShmRtClientControl.readUInt())
            {
                if (fServer.bufferSize != newBufferSize)
                {
                    const CarlaMutexLocker cml(fRealtimeThreadMutex);

                    fServer.bufferSize = newBufferSize;

                    for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
                    {
                        JackClientState* const jclient(it.getValue(nullptr));
                        CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                        jclient->bufferSizeCb(fServer.bufferSize, jclient->bufferSizeCbPtr);
                    }

                    delete[] fAudioTmpBuf;
                    fAudioTmpBuf = new float[fServer.bufferSize];
                    FloatVectorOperations::clear(fAudioTmpBuf, fServer.bufferSize);
                }
            }
            break;

        case kPluginBridgeRtClientSetSampleRate:
            if (const double newSampleRate = fShmRtClientControl.readDouble())
            {
                if (fServer.sampleRate != newSampleRate)
                {
                    const CarlaMutexLocker cml(fRealtimeThreadMutex);

                    fServer.sampleRate = newSampleRate;

                    for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
                    {
                        JackClientState* const jclient(it.getValue(nullptr));
                        CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                        jclient->sampleRateCb(fServer.sampleRate, jclient->sampleRateCbPtr);
                    }
                }
            }
            break;

        case kPluginBridgeRtClientSetOnline:
            // TODO inform changes
            fIsOffline = fShmRtClientControl.readBool();
            //offlineModeChanged(fIsOffline);
            break;

        case kPluginBridgeRtClientControlEventParameter:
        case kPluginBridgeRtClientControlEventMidiBank:
        case kPluginBridgeRtClientControlEventMidiProgram:
        case kPluginBridgeRtClientControlEventAllSoundOff:
        case kPluginBridgeRtClientControlEventAllNotesOff:
            break;

        case kPluginBridgeRtClientMidiEvent: {
            const uint32_t time(fShmRtClientControl.readUInt());
            const uint8_t  port(fShmRtClientControl.readByte());
            const uint8_t  size(fShmRtClientControl.readByte());
            CARLA_SAFE_ASSERT_BREAK(size > 0);

            if (port >= fServer.numMidiIns || size > JackMidiPortBuffer::kMaxEventSize || ! fRealtimeThreadMutex.tryLock())
            {
                for (uint8_t i=0; i<size; ++i)
                    fShmRtClientControl.readByte();
                break;
            }

            JackMidiPortBuffer& midiPortBuf(fMidiInBuffers[port]);

            if (midiPortBuf.count < JackMidiPortBuffer::kMaxEventCount &&
                midiPortBuf.bufferPoolPos + size < JackMidiPortBuffer::kBufferPoolSize)
            {
                jack_midi_event_t& ev(midiPortBuf.events[midiPortBuf.count++]);

                ev.time   = time;
                ev.size   = size;
                ev.buffer = midiPortBuf.bufferPool + midiPortBuf.bufferPoolPos;
                midiPortBuf.bufferPoolPos += size;

                for (uint8_t i=0; i<size; ++i)
                    ev.buffer[i] = fShmRtClientControl.readByte();
            }

            fRealtimeThreadMutex.unlock(true);
            break;
        }

        case kPluginBridgeRtClientProcess: {
            // FIXME - lock if offline
            const CarlaMutexTryLocker cmtl(fRealtimeThreadMutex);

            if (cmtl.wasLocked())
            {
                CARLA_SAFE_ASSERT_BREAK(fShmAudioPool.data != nullptr);

                // location to start of audio outputs (shm buffer)
                float* const fdataRealOuts = fShmAudioPool.data+(fServer.bufferSize*fServer.numAudioIns);

                if (! fClients.isEmpty())
                {
                    // save tranport for all clients
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

                    int numClientOutputsProcessed = 0;

                    // now go through each client
                    for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
                    {
                        JackClientState* const jclient(it.getValue(nullptr));
                        CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                        // FIXME - lock if offline
                        const CarlaMutexTryLocker cmtl2(jclient->mutex);

                        // check if we can process
                        if (cmtl2.wasNotLocked() || jclient->processCb == nullptr || ! jclient->activated)
                        {
                            if (fServer.numAudioOuts > 0)
                                FloatVectorOperations::clear(fdataRealOuts, fServer.bufferSize*fServer.numAudioOuts);

                            if (jclient->deactivated)
                                fShmRtClientControl.data->procFlags = 1;
                        }
                        else
                        {
                            uint8_t i;
                            // direct access to shm buffer, used only for inputs
                            float* fdataReal = fShmAudioPool.data;
                            // safe temp location for output, mixed down to shm buffer later on
                            float* fdataCopy = fAudioPoolCopy;
                            // wherever we're using fAudioTmpBuf
                            bool needsTmpBufClear = false;

                            // set audio inputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it = jclient->audioIns.begin2(); it.valid(); it.next())
                            {
                                JackPortState* const jport = it.getValue(nullptr);
                                CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                                if (i++ < fServer.numAudioIns)
                                {
                                    jport->buffer = fdataReal;
                                    fdataReal += fServer.bufferSize;
                                    fdataCopy += fServer.bufferSize;
                                }
                                else
                                {
                                    jport->buffer = fAudioTmpBuf;
                                    needsTmpBufClear = true;
                                }
                            }
                            // FIXME one single "if"
                            for (; i++ < fServer.numAudioIns;)
                            {
                                fdataReal += fServer.bufferSize;
                                fdataCopy += fServer.bufferSize;
                            }

                            // location to start of audio outputs
                            float* const fdataCopyOuts = fdataCopy;

                            // set audio ouputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it = jclient->audioOuts.begin2(); it.valid(); it.next())
                            {
                                JackPortState* const jport = it.getValue(nullptr);
                                CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                                if (i++ < fServer.numAudioOuts)
                                {
                                    jport->buffer = fdataCopy;
                                    fdataCopy += fServer.bufferSize;
                                }
                                else
                                {
                                    jport->buffer = fAudioTmpBuf;
                                    needsTmpBufClear = true;
                                }
                            }
                            // FIXME one single "if"
                            for (; i++ < fServer.numAudioOuts;)
                            {
                                FloatVectorOperations::clear(fdataCopy, fServer.bufferSize);
                                fdataCopy += fServer.bufferSize;
                            }

                            // set midi inputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it = jclient->midiIns.begin2(); it.valid(); it.next())
                            {
                                JackPortState* const jport = it.getValue(nullptr);
                                CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                                if (i++ < fServer.numMidiIns)
                                    jport->buffer = &fMidiInBuffers[i-1];
                                else
                                    jport->buffer = &fDummyMidiInBuffer;
                            }

                            // set midi outputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it = jclient->midiOuts.begin2(); it.valid(); it.next())
                            {
                                JackPortState* const jport = it.getValue(nullptr);
                                CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                                if (i++ < fServer.numMidiOuts)
                                    jport->buffer = &fMidiOutBuffers[i-1];
                                else
                                    jport->buffer = &fDummyMidiOutBuffer;
                            }

                            if (needsTmpBufClear)
                                FloatVectorOperations::clear(fAudioTmpBuf, fServer.bufferSize);

                            jclient->processCb(fServer.bufferSize, jclient->processCbPtr);

                            if (fServer.numAudioOuts > 0)
                            {
                                if (++numClientOutputsProcessed == 1)
                                {
                                    // first client, we can copy stuff over
                                    FloatVectorOperations::copy(fdataRealOuts, fdataCopyOuts,
                                                                fServer.bufferSize*fServer.numAudioOuts);
                                }
                                else
                                {
                                    // subsequent clients, add data (then divide by number of clients later on)
                                    FloatVectorOperations::add(fdataRealOuts, fdataCopyOuts,
                                                               fServer.bufferSize*fServer.numAudioOuts);
                                }
                            }
                        }
                    }

                    if (numClientOutputsProcessed > 1)
                    {
                        // more than 1 client active, need to divide buffers
                        FloatVectorOperations::multiply(fdataRealOuts,
                                                        1.0f/static_cast<float>(numClientOutputsProcessed),
                                                        fServer.bufferSize*fServer.numAudioOuts);
                    }
                }
                // fClients.isEmpty()
                else if (fServer.numAudioOuts > 0)
                {
                    FloatVectorOperations::clear(fdataRealOuts, fServer.bufferSize*fServer.numAudioOuts);
                }

                for (uint8_t i=0; i<fServer.numMidiIns; ++i)
                {
                    fMidiInBuffers[i].count = 0;
                    fMidiInBuffers[i].bufferPoolPos = 0;
                }

                if (fServer.numMidiOuts > 0)
                {
                    uint8_t* midiData(fShmRtClientControl.data->midiOut);
                    carla_zeroBytes(midiData, kBridgeRtClientDataMidiOutSize);
                    std::size_t curMidiDataPos = 0;

                    for (uint8_t i=0; i<fServer.numMidiOuts; ++i)
                    {
                        JackMidiPortBuffer& midiPortBuf(fMidiOutBuffers[i]);

                        for (uint16_t j=0; j<midiPortBuf.count; ++j)
                        {
                            jack_midi_event_t& jmevent(midiPortBuf.events[j]);

                            if (curMidiDataPos + 6U /* time, port and size */ + jmevent.size >= kBridgeRtClientDataMidiOutSize)
                                break;

                            // set time
                            *(uint32_t*)midiData = jmevent.time;
                            midiData += 4;

                            // set port
                            *midiData++ = i;

                            // set size
                            *midiData++ = static_cast<uint8_t>(jmevent.size);

                            // set data
                            std::memcpy(midiData, jmevent.buffer, jmevent.size);
                            midiData += jmevent.size;

                            curMidiDataPos += 6U /* time, port and size */ + jmevent.size;
                        }
                    }
                }
            }
            else
            {
                carla_stderr2("CarlaJackAppClient: fRealtimeThreadMutex tryLock failed");
            }
            break;
        }

        case kPluginBridgeRtClientQuit:
            ret = true;
            break;
        }

    //#ifdef DEBUG
        if (opcode != kPluginBridgeRtClientProcess && opcode != kPluginBridgeRtClientMidiEvent)
        {
            carla_stdout("CarlaJackAppClientRtThread::run() - opcode %s done", PluginBridgeRtClientOpcode2str(opcode));
        }
    //#endif
    }

    return ret;
}

bool CarlaJackAppClient::handleNonRtData()
{
    bool ret = false;

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

        case kPluginBridgeNonRtClientInitialSetup:
            // should never happen!!
            fShmNonRtServerControl.readUInt();
            fShmNonRtServerControl.readDouble();
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
            jack_carla_interposed_action(2, nullptr);
            break;

        case kPluginBridgeNonRtClientHideUI:
            jack_carla_interposed_action(3, nullptr);
            break;

        case kPluginBridgeNonRtClientUiParameterChange:
        case kPluginBridgeNonRtClientUiProgramChange:
        case kPluginBridgeNonRtClientUiMidiProgramChange:
        case kPluginBridgeNonRtClientUiNoteOn:
        case kPluginBridgeNonRtClientUiNoteOff:
            break;

        case kPluginBridgeNonRtClientQuit:
            ret = true;
            break;
        }

        if (opcode != kPluginBridgeNonRtClientPing)
        {
            static int shownNull = 0;
            if (opcode == kPluginBridgeNonRtClientNull)
            {
                if (shownNull > 5)
                    continue;
                ++shownNull;
            }
            carla_stdout("CarlaJackAppClient::handleNonRtData() - opcode %s handled", PluginBridgeNonRtClientOpcode2str(opcode));
        }
    }

    return ret;
}

void CarlaJackAppClient::runRealtimeThread()
{
    carla_stderr("CarlaJackAppClient runRealtimeThread START");

#ifdef __SSE2_MATH__
    // Set FTZ and DAZ flags
    _mm_setcsr(_mm_getcsr() | 0x8040);
#endif

    bool quitReceived = false;

    for (; ! fRealtimeThread.threadShouldExit();)
    {
        if (handleRtData())
        {
            quitReceived = true;
            break;
        }
    }

    fNonRealtimeThread.signalThreadShouldExit();

    carla_stderr("CarlaJackAppClient runRealtimeThread FINISHED");
}

void CarlaJackAppClient::runNonRealtimeThread()
{
    carla_stderr("CarlaJackAppClient runNonRealtimeThread START");

    if (! initSharedMemmory())
        return;

    if (fServer.numMidiIns > 0)
    {
        fMidiInBuffers = new JackMidiPortBuffer[fServer.numMidiIns];

        for (uint8_t i=0; i<fServer.numMidiIns; ++i)
            fMidiInBuffers[i].isInput = true;
    }

    if (fServer.numMidiOuts > 0)
    {
        fMidiOutBuffers = new JackMidiPortBuffer[fServer.numMidiOuts];

        for (uint8_t i=0; i<fServer.numMidiOuts; ++i)
            fMidiOutBuffers[i].isInput = false;
    }

    fRealtimeThread.startThread(Thread::realtimeAudioPriority);

    fLastPingTime = Time::currentTimeMillis();
    carla_stdout("Carla Jack Client Ready!");

    bool quitReceived = false,
         timedOut = false;

    for (; ! fNonRealtimeThread.threadShouldExit();)
    {
        carla_msleep(50);

        try {
            quitReceived = handleNonRtData();
        } CARLA_SAFE_EXCEPTION("handleNonRtData");

        if (quitReceived)
            break;

        /*
        if (fLastPingTime > 0 && Time::currentTimeMillis() > fLastPingTime + 30000)
        {
            carla_stderr("Did not receive ping message from server for 30 secs, closing...");

            timedOut = true;
            fRealtimeThread.signalThreadShouldExit();
            break;
        }
        */
    }

    //callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);

    if (quitReceived)
    {
        carla_stderr("CarlaJackAppClient runNonRealtimeThread END - quit by carla");

        ::kill(::getpid(), SIGTERM);
    }
    else if (timedOut)
    {
        // TODO send shutdown?
        carla_stderr("CarlaJackAppClient runNonRealtimeThread END - timedOut");

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
            carla_stderr("CarlaJackAppClient runNonRealtimeThread END - quit error");

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
            fShmNonRtServerControl.writeUInt(messageSize);
            fShmNonRtServerControl.writeCustomData(message, messageSize);
            fShmNonRtServerControl.commitWrite();
        }
        else
        {
            carla_stderr("CarlaJackAppClient runNonRealtimeThread END - quit itself");

            //const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            //fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
            //fShmNonRtServerControl.commitWrite();
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

    if (fRealtimeThread.isThreadRunning())
    {
        fRealtimeThread.signalThreadShouldExit();

        const CarlaMutexLocker cml(fRealtimeThreadMutex);

        if (fShmRtClientControl.data != nullptr)
            fShmRtClientControl.data->procFlags = 1;
    }

    clearSharedMemory();

    fRealtimeThread.stopThread(5000);

    carla_stderr("CarlaJackAppClient runNonRealtimeThread FINISHED");
}

// ---------------------------------------------------------------------------------------------------------------------

static CarlaJackAppClient gClient;

static int carla_interposed_callback(int cb_action, void* ptr)
{
    return gClient.handleInterposerCallback(cb_action, ptr);
}

// ---------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
jack_client_t* jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
    carla_stdout("%s(%s, 0x%x, %p)", __FUNCTION__, client_name, options, status);

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
    carla_stdout("%s(%p)", __FUNCTION__, client);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    gClient.removeClient(jclient);
    return 0;
}

CARLA_EXPORT
pthread_t jack_client_thread_id(jack_client_t* client)
{
    carla_stdout("%s(%p)", __FUNCTION__, client);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    CarlaJackAppClient* const jackAppPtr = jclient->server.jackAppPtr;
    CARLA_SAFE_ASSERT_RETURN(jackAppPtr != nullptr && jackAppPtr == &gClient, 0);

    return jackAppPtr->getRealtimeThreadId();
}

CARLA_BACKEND_END_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------

#include "jackbridge/JackBridge2.cpp"
#include "CarlaBridgeUtils.cpp"

// ---------------------------------------------------------------------------------------------------------------------
// TODO

CARLA_BACKEND_USE_NAMESPACE

CARLA_EXPORT
int jack_client_real_time_priority(jack_client_t* client)
{
    carla_stdout("%s(%p)", __FUNCTION__, client);

    // code as used by juce
    const int minPriority = sched_get_priority_min(SCHED_RR);
    const int maxPriority = sched_get_priority_max(SCHED_RR);
    return ((maxPriority - minPriority) * 9) / 10 + minPriority;
}

typedef void (*JackSessionCallback)(jack_session_event_t*, void*);

CARLA_EXPORT
int jack_set_session_callback(jack_client_t* client, JackSessionCallback callback, void* arg)
{
    carla_stdout("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
