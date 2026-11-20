// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "libjack.hpp"

#include "CarlaThread.hpp"
#include "CarlaJuceUtils.hpp"

#include "extra/Time.hpp"

#include <signal.h>
#include <sys/time.h>

// ---------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------

static int carla_interposed_callback(int, void*);

// ---------------------------------------------------------------------------------------------------------------------

class CarlaJackRealtimeThread : public CarlaThread
{
public:
    struct Callback {
        Callback() {}
        virtual ~Callback() {};
        virtual void runRealtimeThread() = 0;
    };

    CarlaJackRealtimeThread(Callback* const callback)
        : CarlaThread("CarlaJackRealtimeThread"),
          fCallback(callback) {}

protected:
    void run() override
    {
        fCallback->runRealtimeThread();
    }

private:
    Callback* const fCallback;

    CARLA_DECLARE_NON_COPYABLE(CarlaJackRealtimeThread)
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaJackNonRealtimeThread : public CarlaThread
{
public:
    struct Callback {
        Callback() {}
        virtual ~Callback() {};
        virtual void runNonRealtimeThread() = 0;
    };

    CarlaJackNonRealtimeThread(Callback* const callback)
        : CarlaThread("CarlaJackNonRealtimeThread"),
          fCallback(callback) {}

protected:
    void run() override
    {
        fCallback->runNonRealtimeThread();
    }

private:
    Callback* const fCallback;

    CARLA_DECLARE_NON_COPYABLE(CarlaJackNonRealtimeThread)
};

// ---------------------------------------------------------------------------------------------------------------------

class CarlaJackAppClient : public CarlaJackRealtimeThread::Callback,
                           public CarlaJackNonRealtimeThread::Callback
{
public:
    JackServerState fServer;
    LinkedList<JackClientState*> fClients;
    LinkedList<JackClientState*> fNewClients;

    CarlaJackAppClient()
        : fServer(this),
          fClients(),
          fNewClients(),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fAudioPoolCopy(nullptr),
          fAudioTmpBuf(nullptr),
          fDummyMidiInBuffer(true),
          fDummyMidiOutBuffer(false),
          fMidiInBuffers(nullptr),
          fMidiOutBuffers(nullptr),
          fInitialized(false),
          fIsOffline(false),
          fIsReady(false),
          fLastPingTime(-1),
          fSessionManager(0),
          fSetupHints(0),
          fRealtimeThread(this),
          fNonRealtimeThread(this),
          fRealtimeThreadMutex()
#ifdef DEBUG
          ,leakDetector_CarlaJackAppClient()
#endif
    {
        carla_debug("CarlaJackAppClient::CarlaJackAppClient() START");

        const char* const shmIds(std::getenv("CARLA_SHM_IDS"));
        CARLA_SAFE_ASSERT_INT2_RETURN(shmIds != nullptr && std::strlen(shmIds) == 6*4,
                                      shmIds != nullptr ? static_cast<int>(std::strlen(shmIds)) : -1, 6*4,);

        const char* const libjackSetup(std::getenv("CARLA_LIBJACK_SETUP"));
        CARLA_SAFE_ASSERT_INT_RETURN(libjackSetup != nullptr && std::strlen(libjackSetup) >= 6,
                                     libjackSetup != nullptr ? static_cast<int>(std::strlen(libjackSetup)) : -1,);

        // make sure we don't get loaded again
        carla_unsetenv("CARLA_SHM_IDS");

        // kill ourselves if main carla dies
        carla_terminateProcessOnParentExit(true);

        for (int i=4; --i >= 0;) {
            CARLA_SAFE_ASSERT_RETURN(libjackSetup[i] >= '0' && libjackSetup[i] <= '0'+64,);
        }
        CARLA_SAFE_ASSERT_RETURN(libjackSetup[4] >= '0' && libjackSetup[4] <= '0' + LIBJACK_SESSION_MANAGER_NSM,);
        CARLA_SAFE_ASSERT_RETURN(libjackSetup[5] >= '0' && libjackSetup[5] < '0'+0x4f,);

        std::memcpy(fBaseNameAudioPool,          shmIds+6*0, 6);
        std::memcpy(fBaseNameRtClientControl,    shmIds+6*1, 6);
        std::memcpy(fBaseNameNonRtClientControl, shmIds+6*2, 6);
        std::memcpy(fBaseNameNonRtServerControl, shmIds+6*3, 6);

        fBaseNameAudioPool[6]          = '\0';
        fBaseNameRtClientControl[6]    = '\0';
        fBaseNameNonRtClientControl[6] = '\0';
        fBaseNameNonRtServerControl[6] = '\0';

        fServer.numAudioIns  = static_cast<uint8_t>(libjackSetup[0] - '0');
        fServer.numAudioOuts = static_cast<uint8_t>(libjackSetup[1] - '0');
        fServer.numMidiIns   = static_cast<uint8_t>(libjackSetup[2] - '0');
        fServer.numMidiOuts  = static_cast<uint8_t>(libjackSetup[3] - '0');

        fSessionManager = static_cast<uint>(libjackSetup[4] - '0');
        fSetupHints     = static_cast<uint>(libjackSetup[5] - '0');

        if (fSetupHints & LIBJACK_FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN)
            fServer.numMidiOuts = 16;

        jack_carla_interposed_action(LIBJACK_INTERPOSER_ACTION_SET_HINTS_AND_CALLBACK,
                                     fSetupHints,
                                     (void*)carla_interposed_callback);

        jack_carla_interposed_action(LIBJACK_INTERPOSER_ACTION_SET_SESSION_MANAGER,
                                     fSessionManager,
                                     nullptr);

        fNonRealtimeThread.startThread(false);

        fInitialized = true;
        carla_debug("CarlaJackAppClient::CarlaJackAppClient() DONE");
    }

    ~CarlaJackAppClient() noexcept override
    {
        carla_debug("CarlaJackAppClient::~CarlaJackAppClient() START");

        fLastPingTime = -1;

        fNonRealtimeThread.stopThread(5000);

        {
            const CarlaMutexLocker cms(fRealtimeThreadMutex);

            for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
            {
                JackClientState* const jclient(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                delete jclient;
            }

            fClients.clear();
            fNewClients.clear();
        }

        clearSharedMemory();

        carla_debug("CarlaJackAppClient::~CarlaJackAppClient() DONE");
    }

    JackClientState* createClient(const char* const name)
    {
        if (! fInitialized)
            return nullptr;

        while (fNonRealtimeThread.isThreadRunning() && ! fIsReady)
            d_msleep(100);

        return new JackClientState(fServer, name);
    }

    void destroyClient(JackClientState* const jclient)
    {
        {
            const CarlaMutexLocker cms(fRealtimeThreadMutex);
            fClients.removeOne(jclient);
        }

        delete jclient;
    }

    bool activateClient(JackClientState* const jclient)
    {
        const CarlaMutexLocker cms(fRealtimeThreadMutex);

        if (! fClients.append(jclient))
            return false;
        if (! fNewClients.append(jclient))
        {
            fClients.removeOne(jclient);
            return false;
        }

        jclient->activated = true;
        jclient->deactivated = false;
        return true;
    }

    bool deactivateClient(JackClientState* const jclient)
    {
        const CarlaMutexLocker cms(fRealtimeThreadMutex);

        if (! fClients.removeOne(jclient))
            return false;

        jclient->activated = false;
        jclient->deactivated = true;
        return true;
    }

    const char* getClientNameFromUUID(const jack_uuid_t uuid) const noexcept
    {
        for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
        {
            JackClientState* const jclient(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

            if (jclient->uuid == uuid)
                return jclient->name;
        }

        return nullptr;
    }

    jack_uuid_t getUUIDForClientName(const char* const name) const noexcept
    {
        for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
        {
            JackClientState* const jclient(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

            if (std::strcmp(jclient->name, name) == 0)
                return jclient->uuid;
        }

        return JACK_UUID_EMPTY_INITIALIZER;
    }

    pthread_t getRealtimeThreadId() const noexcept
    {
        return (pthread_t)fRealtimeThread.getThreadId();
    }

    int handleInterposerCallback(const int cb_action, void* const ptr)
    {
        carla_debug("handleInterposerCallback(%o, %p)", cb_action, ptr);

        switch (cb_action)
        {
        case LIBJACK_INTERPOSER_CALLBACK_UI_HIDE: {
            const CarlaMutexLocker cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
            fShmNonRtServerControl.commitWrite();
            break;
        }
        }

        return 0;

        // maybe unused
        (void)ptr;
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

    JackMidiPortBufferDummy fDummyMidiInBuffer;
    JackMidiPortBufferDummy fDummyMidiOutBuffer;
    JackMidiPortBufferOnStack* fMidiInBuffers;
    JackMidiPortBufferOnStack* fMidiOutBuffers;

    char fBaseNameAudioPool[6+1];
    char fBaseNameRtClientControl[6+1];
    char fBaseNameNonRtClientControl[6+1];
    char fBaseNameNonRtServerControl[6+1];

    bool fInitialized;
    bool fIsOffline;
    volatile bool fIsReady;
    int64_t fLastPingTime;

    uint fSessionManager;
    uint fSetupHints;

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
    CARLA_SAFE_ASSERT_RETURN(opcode == kPluginBridgeNonRtClientVersion, false);

    const uint32_t apiVersion = fShmNonRtClientControl.readUInt();
    CARLA_SAFE_ASSERT_RETURN(apiVersion == CARLA_PLUGIN_BRIDGE_API_VERSION_CURRENT, false);

    const uint32_t shmRtClientDataSize = fShmNonRtClientControl.readUInt();
    CARLA_SAFE_ASSERT_INT2(shmRtClientDataSize == sizeof(BridgeRtClientData), shmRtClientDataSize, sizeof(BridgeRtClientData));

    const uint32_t shmNonRtClientDataSize = fShmNonRtClientControl.readUInt();
    CARLA_SAFE_ASSERT_INT2(shmNonRtClientDataSize == sizeof(BridgeNonRtClientData), shmNonRtClientDataSize, sizeof(BridgeNonRtClientData));

    const uint32_t shmNonRtServerDataSize = fShmNonRtClientControl.readUInt();
    CARLA_SAFE_ASSERT_INT2(shmNonRtServerDataSize == sizeof(BridgeNonRtServerData), shmNonRtServerDataSize, sizeof(BridgeNonRtServerData));

    if (shmRtClientDataSize != sizeof(BridgeRtClientData) ||
        shmNonRtClientDataSize != sizeof(BridgeNonRtClientData) ||
        shmNonRtServerDataSize != sizeof(BridgeNonRtServerData))
    {
        carla_stderr2("CarlaJackAppClient: data size mismatch");
        return false;
    }

    opcode = fShmNonRtClientControl.readOpcode();
    CARLA_SAFE_ASSERT_RETURN(opcode == kPluginBridgeNonRtClientInitialSetup, false);

    fServer.bufferSize = fShmNonRtClientControl.readUInt();
    fServer.sampleRate = fShmNonRtClientControl.readDouble();

    if (fServer.bufferSize == 0 || carla_isZero(fServer.sampleRate))
    {
        carla_stderr2("CarlaJackAppClient: invalid empty state");
        return false;
    }

    fAudioTmpBuf = new float[fServer.bufferSize];
    carla_zeroFloats(fAudioTmpBuf, fServer.bufferSize);

    fLastPingTime = d_gettime_ms();
    CARLA_SAFE_ASSERT(fLastPingTime > 0);

    {
        // tell backend we're live
        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

        // ready!
        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerReady);
        fShmNonRtServerControl.commitWrite();
        fShmNonRtServerControl.waitIfDataIsReachingLimit();
    }

    fIsReady = true;
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
    if (fNewClients.count() != 0)
    {
        for (LinkedList<JackClientState*>::Itenerator it = fNewClients.begin2(); it.valid(); it.next())
        {
            JackClientState* const jclient(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

            if (jclient->threadInitCb != nullptr)
                jclient->threadInitCb(jclient->threadInitCbPtr);
        }

        fNewClients.clear();
    }

    const BridgeRtClientControl::WaitHelper helper(fShmRtClientControl);

    if (! helper.ok)
        return false;

    bool ret = false;

    for (; fShmRtClientControl.isDataAvailableForReading();)
    {
        const PluginBridgeRtClientOpcode opcode(fShmRtClientControl.readOpcode());
#ifdef DEBUG
        if (opcode != kPluginBridgeRtClientProcess && opcode != kPluginBridgeRtClientMidiEvent) {
            carla_debug("CarlaJackAppClientRtThread::run() - got opcode: %s", PluginBridgeRtClientOpcode2str(opcode));
        }
#endif

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

                        if (jclient->bufferSizeCb != nullptr)
                            jclient->bufferSizeCb(fServer.bufferSize, jclient->bufferSizeCbPtr);
                    }

                    delete[] fAudioTmpBuf;
                    fAudioTmpBuf = new float[fServer.bufferSize];
                    carla_zeroFloats(fAudioTmpBuf, fServer.bufferSize);
                }
            }
            break;

        case kPluginBridgeRtClientSetSampleRate: {
            const double newSampleRate = fShmRtClientControl.readDouble();

            if (carla_isNotZero(newSampleRate) && carla_isNotEqual(fServer.sampleRate, newSampleRate))
            {
                const CarlaMutexLocker cml(fRealtimeThreadMutex);

                fServer.sampleRate = newSampleRate;

                for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
                {
                    JackClientState* const jclient(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                    if (jclient->sampleRateCb != nullptr)
                        jclient->sampleRateCb(static_cast<uint32_t>(fServer.sampleRate), jclient->sampleRateCbPtr);
                }
            }
        }   break;

        case kPluginBridgeRtClientSetOnline: {
            const bool offline = fShmRtClientControl.readBool();

            if (fIsOffline != offline)
            {
                const CarlaMutexLocker cml(fRealtimeThreadMutex);

                fIsOffline = offline;

                for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
                {
                    JackClientState* const jclient(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                    if (jclient->freewheelCb != nullptr)
                        jclient->freewheelCb(offline ? 1 : 0, jclient->freewheelCbPtr);
                }
            }
        }   break;

        case kPluginBridgeRtClientControlEventParameter:
            break;

        case kPluginBridgeRtClientControlEventMidiBank: {
            const uint32_t time    = fShmRtClientControl.readUInt();
            const uint8_t  channel = fShmRtClientControl.readByte();
            const uint16_t value   = fShmRtClientControl.readUShort();

            if (fServer.numMidiIns > 0 && fRealtimeThreadMutex.tryLock())
            {
                JackMidiPortBufferOnStack& midiPortBuf(fMidiInBuffers[0]);

                if (midiPortBuf.count+1U < JackMidiPortBufferBase::kMaxEventCount &&
                    midiPortBuf.bufferPoolPos + 6U < JackMidiPortBufferBase::kBufferPoolSize)
                {
                    jack_midi_event_t& ev1(midiPortBuf.events[midiPortBuf.count++]);
                    ev1.time   = time;
                    ev1.size   = 3;
                    ev1.buffer = midiPortBuf.bufferPool + midiPortBuf.bufferPoolPos;
                    ev1.buffer[0] = jack_midi_data_t(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
                    ev1.buffer[1] = MIDI_CONTROL_BANK_SELECT;
                    ev1.buffer[2] = 0;
                    midiPortBuf.bufferPoolPos += 3;

                    jack_midi_event_t& ev2(midiPortBuf.events[midiPortBuf.count++]);
                    ev2.time   = time;
                    ev2.size   = 3;
                    ev2.buffer = midiPortBuf.bufferPool + midiPortBuf.bufferPoolPos;
                    ev2.buffer[0] = jack_midi_data_t(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
                    ev2.buffer[1] = MIDI_CONTROL_BANK_SELECT__LSB;
                    ev2.buffer[2] = jack_midi_data_t(value);
                    midiPortBuf.bufferPoolPos += 3;
                }

                fRealtimeThreadMutex.unlock(true);
            }
            break;
        }

        case kPluginBridgeRtClientControlEventMidiProgram: {
            const uint32_t time    = fShmRtClientControl.readUInt();
            const uint8_t  channel = fShmRtClientControl.readByte();
            const uint16_t value   = fShmRtClientControl.readUShort();

            if (fServer.numMidiIns > 0 && fRealtimeThreadMutex.tryLock())
            {
                JackMidiPortBufferOnStack& midiPortBuf(fMidiInBuffers[0]);

                if (midiPortBuf.count < JackMidiPortBufferBase::kMaxEventCount &&
                    midiPortBuf.bufferPoolPos + 2U < JackMidiPortBufferBase::kBufferPoolSize)
                {
                    jack_midi_event_t& ev(midiPortBuf.events[midiPortBuf.count++]);

                    ev.time   = time;
                    ev.size   = 2;
                    ev.buffer = midiPortBuf.bufferPool + midiPortBuf.bufferPoolPos;
                    ev.buffer[0] = jack_midi_data_t(MIDI_STATUS_PROGRAM_CHANGE | (channel & MIDI_CHANNEL_BIT));
                    ev.buffer[1] = jack_midi_data_t(value);
                    midiPortBuf.bufferPoolPos += 2;
                }

                fRealtimeThreadMutex.unlock(true);
            }
            break;
        }

        case kPluginBridgeRtClientControlEventAllSoundOff: {
            const uint32_t time    = fShmRtClientControl.readUInt();
            const uint8_t  channel = fShmRtClientControl.readByte();

            if (fServer.numMidiIns > 0 && fRealtimeThreadMutex.tryLock())
            {
                JackMidiPortBufferOnStack& midiPortBuf(fMidiInBuffers[0]);

                if (midiPortBuf.count < JackMidiPortBufferBase::kMaxEventCount &&
                    midiPortBuf.bufferPoolPos + 3U < JackMidiPortBufferBase::kBufferPoolSize)
                {
                    jack_midi_event_t& ev(midiPortBuf.events[midiPortBuf.count++]);

                    ev.time   = time;
                    ev.size   = 3;
                    ev.buffer = midiPortBuf.bufferPool + midiPortBuf.bufferPoolPos;
                    ev.buffer[0] = jack_midi_data_t(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
                    ev.buffer[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                    ev.buffer[2] = 0;
                    midiPortBuf.bufferPoolPos += 3;
                }

                fRealtimeThreadMutex.unlock(true);
            }
            break;
        }

        case kPluginBridgeRtClientControlEventAllNotesOff: {
            const uint32_t time    = fShmRtClientControl.readUInt();
            const uint8_t  channel = fShmRtClientControl.readByte();

            if (fServer.numMidiIns > 0 && fRealtimeThreadMutex.tryLock())
            {
                JackMidiPortBufferOnStack& midiPortBuf(fMidiInBuffers[0]);

                if (midiPortBuf.count < JackMidiPortBufferBase::kMaxEventCount &&
                    midiPortBuf.bufferPoolPos + 3U < JackMidiPortBufferBase::kBufferPoolSize)
                {
                    jack_midi_event_t& ev(midiPortBuf.events[midiPortBuf.count++]);

                    ev.time   = time;
                    ev.size   = 3;
                    ev.buffer = midiPortBuf.bufferPool + midiPortBuf.bufferPoolPos;
                    ev.buffer[0] = jack_midi_data_t(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
                    ev.buffer[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                    ev.buffer[2] = 0;
                    midiPortBuf.bufferPoolPos += 3;
                }

                fRealtimeThreadMutex.unlock(true);
            }
            break;
        }

        case kPluginBridgeRtClientMidiEvent: {
            const uint32_t time = fShmRtClientControl.readUInt();
            const uint8_t  port = fShmRtClientControl.readByte();
            const uint8_t  size = fShmRtClientControl.readByte();
            CARLA_SAFE_ASSERT_BREAK(size > 0);

            if (port >= fServer.numMidiIns || size > JackMidiPortBufferBase::kMaxEventSize || ! fRealtimeThreadMutex.tryLock())
            {
                for (uint8_t i=0; i<size; ++i)
                    fShmRtClientControl.readByte();
                break;
            }

            JackMidiPortBufferOnStack& midiPortBuf(fMidiInBuffers[port]);

            if (midiPortBuf.count < JackMidiPortBufferBase::kMaxEventCount &&
                midiPortBuf.bufferPoolPos + size < JackMidiPortBufferBase::kBufferPoolSize)
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
            const uint32_t frames(fShmRtClientControl.readUInt());
            CARLA_SAFE_ASSERT_UINT2_BREAK(frames == fServer.bufferSize, frames, fServer.bufferSize);

            // TODO tell client of xrun in case buffersize does not match

            const CarlaMutexTryLocker cmtl(fRealtimeThreadMutex, fIsOffline);

            if (cmtl.wasLocked())
            {
                CARLA_SAFE_ASSERT_BREAK(fShmAudioPool.data != nullptr);

                // mixdown is default, do buffer addition (for multiple clients) if requested
                const bool doBufferAddition  = fSetupHints & LIBJACK_FLAG_AUDIO_BUFFERS_ADDITION;
                // mixdown midi outputs based on channel if requested
                const bool doMidiChanMixdown = fSetupHints & LIBJACK_FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN;

                // location to start of audio outputs (shm buffer)
                float* const fdataRealOuts = fShmAudioPool.data+(fServer.bufferSize*fServer.numAudioIns);

                if (doBufferAddition && fServer.numAudioOuts > 0)
                    carla_zeroFloats(fdataRealOuts, fServer.bufferSize*fServer.numAudioOuts);

                if (! fClients.isEmpty())
                {
                    // save transport for all clients
                    const BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

                    const bool transportChanged = fServer.playing != bridgeTimeInfo.playing ||
                                                  (bridgeTimeInfo.playing && fServer.position.frame + frames != bridgeTimeInfo.frame);

                    fServer.playing        = bridgeTimeInfo.playing;
                    fServer.position.frame = static_cast<jack_nframes_t>(bridgeTimeInfo.frame);
                    fServer.position.usecs = bridgeTimeInfo.usecs;

                    fServer.position.frame_rate = static_cast<jack_nframes_t>(fServer.sampleRate);

                    if (bridgeTimeInfo.validFlags & kPluginBridgeTimeInfoValidBBT)
                    {
                        fServer.position.bar  = bridgeTimeInfo.bar;
                        fServer.position.beat = bridgeTimeInfo.beat;
                        fServer.position.tick = static_cast<int32_t>(bridgeTimeInfo.tick + 0.5);

                        fServer.position.beats_per_bar = bridgeTimeInfo.beatsPerBar;
                        fServer.position.beat_type     = bridgeTimeInfo.beatType;

                        fServer.position.ticks_per_beat   = bridgeTimeInfo.ticksPerBeat;
                        fServer.position.beats_per_minute = bridgeTimeInfo.beatsPerMinute;
                        fServer.position.bar_start_tick   = bridgeTimeInfo.barStartTick;

#ifdef JACK_TICK_DOUBLE
                        fServer.position.tick_double = bridgeTimeInfo.tick;
                        fServer.position.valid = static_cast<jack_position_bits_t>(JackPositionBBT|JackTickDouble);
#else
                        fServer.position.valid = JackPositionBBT;
#endif
                    }
                    else
                    {
                        fServer.position.valid = static_cast<jack_position_bits_t>(0x0);
                    }

                    int numClientOutputsProcessed = 0;

                    // now go through each client
                    for (LinkedList<JackClientState*>::Itenerator it = fClients.begin2(); it.valid(); it.next())
                    {
                        JackClientState* const jclient(it.getValue(nullptr));
                        CARLA_SAFE_ASSERT_CONTINUE(jclient != nullptr);

                        const CarlaMutexTryLocker cmtl2(jclient->mutex, fIsOffline);

                        // check if we can process
                        if (cmtl2.wasNotLocked() || jclient->processCb == nullptr || ! jclient->activated)
                        {
                            if (fServer.numAudioOuts > 0)
                                carla_zeroFloats(fdataRealOuts, fServer.bufferSize*fServer.numAudioOuts);

                            if (jclient->deactivated)
                                fShmRtClientControl.data->procFlags = 1;
                        }
                        else
                        {
                            // report transport sync changes if needed
                            if (transportChanged && jclient->syncCb != nullptr)
                            {
                                jclient->syncCb(fServer.playing ? JackTransportRolling : JackTransportStopped,
                                                &fServer.position,
                                                jclient->syncCbPtr);
                            }

                            uint8_t i;
                            // direct access to shm buffer, used only for inputs
                            float* fdataReal = fShmAudioPool.data;
                            // safe temp location for output, mixed down to shm buffer later on
                            float* fdataCopy = fAudioPoolCopy;
                            // wherever we're using fAudioTmpBuf
                            bool needsTmpBufClear = false;

                            // set audio inputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it2 = jclient->audioIns.begin2(); it2.valid(); it2.next())
                            {
                                JackPortState* const jport = it2.getValue(nullptr);
                                CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                                if (i++ < fServer.numAudioIns)
                                {
                                    if (numClientOutputsProcessed == 0 || ! doBufferAddition)
                                        jport->buffer = fdataReal;
                                    else
                                        jport->buffer = fdataRealOuts + (i*fServer.bufferSize);

                                    fdataReal += fServer.bufferSize;
                                    fdataCopy += fServer.bufferSize;
                                }
                                else
                                {
                                    jport->buffer = fAudioTmpBuf;
                                    needsTmpBufClear = true;
                                }
                            }
                            if (i < fServer.numAudioIns)
                            {
                                const std::size_t remainingBufferSize = fServer.bufferSize * static_cast<uint8_t>(fServer.numAudioIns - i);
                                //fdataReal += remainingBufferSize;
                                fdataCopy += remainingBufferSize;
                            }

                            // location to start of audio outputs
                            float* const fdataCopyOuts = fdataCopy;

                            // set audio outputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it2 = jclient->audioOuts.begin2(); it2.valid(); it2.next())
                            {
                                JackPortState* const jport = it2.getValue(nullptr);
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
                            if (i < fServer.numAudioOuts)
                            {
                                const std::size_t remainingBufferSize = fServer.bufferSize * static_cast<uint8_t>(fServer.numAudioOuts - i);
                                carla_zeroFloats(fdataCopy, remainingBufferSize);
                                //fdataCopy += remainingBufferSize;
                            }

                            // set midi inputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it2 = jclient->midiIns.begin2(); it2.valid(); it2.next())
                            {
                                JackPortState* const jport = it2.getValue(nullptr);
                                CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                                if (i++ < fServer.numMidiIns)
                                    jport->buffer = &fMidiInBuffers[i-1];
                                else
                                    jport->buffer = &fDummyMidiInBuffer;
                            }

                            // set midi outputs
                            i = 0;
                            for (LinkedList<JackPortState*>::Itenerator it2 = jclient->midiOuts.begin2(); it2.valid(); it2.next())
                            {
                                JackPortState* const jport = it2.getValue(nullptr);
                                CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);

                                if (i++ < fServer.numMidiOuts)
                                    jport->buffer = &fMidiOutBuffers[i-1];
                                else
                                    jport->buffer = &fDummyMidiOutBuffer;
                            }

                            if (needsTmpBufClear)
                                carla_zeroFloats(fAudioTmpBuf, fServer.bufferSize);

                            jclient->processCb(fServer.bufferSize, jclient->processCbPtr);

                            if (fServer.numAudioOuts > 0)
                            {
                                if (++numClientOutputsProcessed == 1)
                                {
                                    // first client, we can copy stuff over
                                    carla_copyFloats(fdataRealOuts, fdataCopyOuts,
                                                     fServer.bufferSize*fServer.numAudioOuts);
                                }
                                else
                                {
                                    // subsequent clients, add data (then divide by number of clients later on)
                                    carla_add(fdataRealOuts, fdataCopyOuts,
                                              fServer.bufferSize*fServer.numAudioOuts);

                                    if (doBufferAddition)
                                    {
                                        // for more than 1 client addition, we need to divide buffers now
                                        carla_multiply(fdataRealOuts,
                                                       1.0f/static_cast<float>(numClientOutputsProcessed),
                                                       fServer.bufferSize*fServer.numAudioOuts);
                                    }
                                }

                                if (jclient->audioOuts.count() == 1 && fServer.numAudioOuts > 1)
                                {
                                    for (uint8_t j=1; j<fServer.numAudioOuts; ++j)
                                    {
                                        carla_copyFloats(fdataRealOuts+(fServer.bufferSize*j),
                                                         fdataCopyOuts,
                                                         fServer.bufferSize);
                                    }
                                }
                            }
                        }
                    }

                    if (numClientOutputsProcessed > 1 && ! doBufferAddition)
                    {
                        // more than 1 client active, need to divide buffers
                        carla_multiply(fdataRealOuts,
                                       1.0f/static_cast<float>(numClientOutputsProcessed),
                                       fServer.bufferSize*fServer.numAudioOuts);
                    }
                }
                // fClients.isEmpty()
                else if (fServer.numAudioOuts > 0)
                {
                    carla_zeroFloats(fdataRealOuts, fServer.bufferSize*fServer.numAudioOuts);
                }

                for (uint8_t i=0; i<fServer.numMidiIns; ++i)
                {
                    fMidiInBuffers[i].count = 0;
                    fMidiInBuffers[i].bufferPoolPos = 0;
                }

                if (fServer.numMidiOuts > 0)
                {
                    uint8_t* midiData = fShmRtClientControl.data->midiOut;
                    carla_zeroBytes(midiData, kBridgeBaseMidiOutHeaderSize);
                    std::size_t curMidiDataPos = 0;

                    for (uint8_t i=0; i<fServer.numMidiOuts; ++i)
                    {
                        JackMidiPortBufferOnStack& midiPortBuf(fMidiOutBuffers[i]);

                        for (uint16_t j=0; j<midiPortBuf.count; ++j)
                        {
                            jack_midi_event_t& jmevent(midiPortBuf.events[j]);

                            if (curMidiDataPos + kBridgeBaseMidiOutHeaderSize + jmevent.size >= kBridgeRtClientDataMidiOutSize)
                                break;

                            if (doMidiChanMixdown && MIDI_IS_CHANNEL_MESSAGE(jmevent.buffer[0]))
                                jmevent.buffer[0] = static_cast<jack_midi_data_t>(
                                                    (jmevent.buffer[0] & MIDI_STATUS_BIT) | (i & MIDI_CHANNEL_BIT));

                            // set time
                            *(uint32_t*)midiData = jmevent.time;
                            midiData += 4;

                            // set port
                            *midiData++ = doMidiChanMixdown ? 0 : i;

                            // set size
                            *midiData++ = static_cast<uint8_t>(jmevent.size);

                            // set data
                            std::memcpy(midiData, jmevent.buffer, jmevent.size);
                            midiData += jmevent.size;

                            curMidiDataPos += kBridgeBaseMidiOutHeaderSize + jmevent.size;
                        }
                    }

                    // make last event null, so server stops when reaching it
                    if (curMidiDataPos != 0 &&
                        curMidiDataPos + kBridgeBaseMidiOutHeaderSize < kBridgeRtClientDataMidiOutSize)
                    {
                        carla_zeroBytes(midiData, kBridgeBaseMidiOutHeaderSize);

                        // sort events in case of mixdown
                        if (doMidiChanMixdown)
                        {
                            uint32_t time;
                            uint8_t size, *midiDataPtr;
                            uint8_t tmp[kBridgeBaseMidiOutHeaderSize + JackMidiPortBufferBase::kMaxEventSize];
                            bool wasSorted = true;

                            for (; wasSorted;)
                            {
                                midiDataPtr = fShmRtClientControl.data->midiOut;
                                uint8_t* prevData = midiDataPtr;
                                uint32_t prevTime = *(uint32_t*)midiDataPtr;
                                uint8_t prevSize = *(midiDataPtr + 5);
                                wasSorted = false;

                                for (;;)
                                {
                                    time = *(uint32_t*)midiDataPtr;
                                    size = *(midiDataPtr + 5); // time and port

                                    if (size == 0)
                                        break;

                                    if (prevTime > time)
                                    {
                                        // copy previous data to a temporary place
                                        std::memcpy(tmp, prevData, kBridgeBaseMidiOutHeaderSize + prevSize);
                                        // override previous data with new one (shifting left)
                                        std::memcpy(prevData, midiDataPtr, kBridgeBaseMidiOutHeaderSize + size);
                                        // override new data with old one
                                        std::memcpy(midiDataPtr, tmp, kBridgeBaseMidiOutHeaderSize + prevSize);
                                        // done swapping, flag it
                                        wasSorted = true;
                                    }

                                    prevTime = time;
                                    prevSize = size;
                                    prevData = midiDataPtr;
                                    midiDataPtr += 6 + size;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                carla_stderr2("CarlaJackAppClient: fRealtimeThreadMutex tryLock failed");
            }
            fServer.monotonic_frame += frames;
            break;
        }

        case kPluginBridgeRtClientQuit:
            ret = true;
            break;
        }

#ifdef DEBUG
        if (opcode != kPluginBridgeRtClientProcess && opcode != kPluginBridgeRtClientMidiEvent) {
            carla_debug("CarlaJackAppClientRtThread::run() - opcode %s done", PluginBridgeRtClientOpcode2str(opcode));
        }
#endif
    }

    return ret;
}

bool CarlaJackAppClient::handleNonRtData()
{
    bool ret = false;

    for (; fShmNonRtClientControl.isDataAvailableForReading();)
    {
        const PluginBridgeNonRtClientOpcode opcode(fShmNonRtClientControl.readOpcode());
#ifdef DEBUG
        if (opcode != kPluginBridgeNonRtClientPing) {
            carla_debug("CarlaJackAppClient::handleNonRtData() - got opcode: %s", PluginBridgeNonRtClientOpcode2str(opcode));
        }
#endif

        if (opcode != kPluginBridgeNonRtClientNull && opcode != kPluginBridgeNonRtClientPingOnOff && fLastPingTime > 0)
            fLastPingTime = d_gettime_ms();

        switch (opcode)
        {
        case kPluginBridgeNonRtClientNull:
            break;

        case kPluginBridgeNonRtClientVersion: {
            const uint apiVersion = fShmNonRtServerControl.readUInt();
            CARLA_SAFE_ASSERT_UINT2(apiVersion == CARLA_PLUGIN_BRIDGE_API_VERSION_CURRENT,
                                    apiVersion, CARLA_PLUGIN_BRIDGE_API_VERSION_CURRENT);
        }   break;

        case kPluginBridgeNonRtClientPing: {
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPong);
            fShmNonRtServerControl.commitWrite();
        }   break;

        case kPluginBridgeNonRtClientPingOnOff: {
            const uint32_t onOff(fShmNonRtClientControl.readBool());

            fLastPingTime = onOff ? d_gettime_ms() : -1;
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
        case kPluginBridgeNonRtClientSetParameterMappedControlIndex:
        case kPluginBridgeNonRtClientSetParameterMappedRange:
        case kPluginBridgeNonRtClientSetProgram:
        case kPluginBridgeNonRtClientSetMidiProgram:
        case kPluginBridgeNonRtClientSetCustomData:
        case kPluginBridgeNonRtClientSetChunkDataFile:
        case kPluginBridgeNonRtClientSetWindowTitle:
            break;

        case kPluginBridgeNonRtClientSetOption:
            fShmNonRtClientControl.readUInt();
            fShmNonRtClientControl.readBool();
            break;

        case kPluginBridgeNonRtClientSetOptions:
            fShmNonRtClientControl.readUInt();
            break;

        case kPluginBridgeNonRtClientSetCtrlChannel:
            fShmNonRtClientControl.readShort();
            break;

        case kPluginBridgeNonRtClientGetParameterText:
            fShmNonRtClientControl.readUInt();
            break;

        case kPluginBridgeNonRtClientPrepareForSave:
            {
                if (fSessionManager == LIBJACK_SESSION_MANAGER_AUTO && std::getenv("NSM_URL") == nullptr)
                {
                    struct sigaction sig;
                    carla_zeroStruct(sig);

                    sigaction(SIGUSR1, nullptr, &sig);

                    if (sig.sa_handler != nullptr)
                        fSessionManager = LIBJACK_SESSION_MANAGER_LADISH;
                }

                if (fSessionManager == LIBJACK_SESSION_MANAGER_LADISH)
                    ::kill(::getpid(), SIGUSR1);

                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSaved);
                fShmNonRtServerControl.commitWrite();
            }
            break;

        case kPluginBridgeNonRtClientRestoreLV2State:
            break;

        case kPluginBridgeNonRtClientShowUI:
            if (jack_carla_interposed_action(LIBJACK_INTERPOSER_ACTION_SHOW_HIDE_GUI, 1, nullptr) == 1337)
            {
                // failed, LD_PRELOAD did not work?
                const char* const message("Cannot show UI, LD_PRELOAD not working?");
                const std::size_t messageSize(std::strlen(message));

                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
                fShmNonRtServerControl.commitWrite();

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
                fShmNonRtServerControl.writeUInt(messageSize);
                fShmNonRtServerControl.writeCustomData(message, messageSize);
                fShmNonRtServerControl.commitWrite();
            }
            break;

        case kPluginBridgeNonRtClientHideUI:
            jack_carla_interposed_action(LIBJACK_INTERPOSER_ACTION_SHOW_HIDE_GUI, 0, nullptr);
            break;

        case kPluginBridgeNonRtClientUiParameterChange:
        case kPluginBridgeNonRtClientUiProgramChange:
        case kPluginBridgeNonRtClientUiMidiProgramChange:
        case kPluginBridgeNonRtClientUiNoteOn:
        case kPluginBridgeNonRtClientUiNoteOff:
            break;

        case kPluginBridgeNonRtClientEmbedUI:
            fShmNonRtClientControl.readULong();
            break;

        case kPluginBridgeNonRtClientQuit:
            ret = true;
            break;

        case kPluginBridgeNonRtClientReload:
            break;
        }

#ifdef DEBUG
        if (opcode != kPluginBridgeNonRtClientPing) {
            carla_debug("CarlaJackAppClient::handleNonRtData() - opcode %s handled", PluginBridgeNonRtClientOpcode2str(opcode));
        }
#endif
    }

    return ret;
}

void CarlaJackAppClient::runRealtimeThread()
{
    carla_debug("CarlaJackAppClient runRealtimeThread START");

#ifdef __SSE2_MATH__
    // Set FTZ and DAZ flags
    _mm_setcsr(_mm_getcsr() | 0x8040);
#endif

    bool quitReceived = false;

    for (; ! fRealtimeThread.shouldThreadExit();)
    {
        if (handleRtData())
        {
            quitReceived = true;
            break;
        }
    }

    fNonRealtimeThread.signalThreadShouldExit();

    carla_debug("CarlaJackAppClient runRealtimeThread FINISHED");

    // TODO
    return; (void)quitReceived;
}

void CarlaJackAppClient::runNonRealtimeThread()
{
    carla_debug("CarlaJackAppClient runNonRealtimeThread START");
    CARLA_SAFE_ASSERT_RETURN(initSharedMemmory(),);

    if (fServer.numMidiIns > 0)
    {
        fMidiInBuffers = new JackMidiPortBufferOnStack[fServer.numMidiIns];

        for (uint8_t i=0; i<fServer.numMidiIns; ++i)
            fMidiInBuffers[i].isInput = true;
    }

    if (fServer.numMidiOuts > 0)
    {
        fMidiOutBuffers = new JackMidiPortBufferOnStack[fServer.numMidiOuts];

        for (uint8_t i=0; i<fServer.numMidiOuts; ++i)
            fMidiOutBuffers[i].isInput = false;
    }

    fRealtimeThread.startThread(true);

    fLastPingTime = d_gettime_ms();
    carla_stdout("Carla Jack Client Ready!");

    bool quitReceived = false,
         timedOut = false;

    for (; ! fNonRealtimeThread.shouldThreadExit();)
    {
        d_msleep(50);

        try {
            quitReceived = handleNonRtData();
        } CARLA_SAFE_EXCEPTION("handleNonRtData");

        if (quitReceived)
            break;

        /*
        if (fLastPingTime > 0 && d_gettime_ms() > fLastPingTime + 30000)
        {
            carla_stderr("Did not receive ping message from server for 30 secs, closing...");

            timedOut = true;
            fRealtimeThread.signalThreadShouldExit();
            break;
        }
        */
    }

    //callback(true, true, ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);

    if (quitReceived)
    {
        ::kill(::getpid(), SIGTERM);
    }
    else if (timedOut)
    {
        // TODO send shutdown?
        carla_stderr("CarlaJackAppClient error: runNonRealtimeThread ended with time out");
        ::kill(::getpid(), SIGTERM);
    }
    else
    {
        bool activated;

        {
            const CarlaMutexLocker cms(fRealtimeThreadMutex);

            if (fClients.isEmpty())
            {
                activated = false;
            }
            else if (JackClientState* const jclient = fClients.getLast(nullptr))
            {
                const CarlaMutexLocker cms2(jclient->mutex);
                activated = jclient->activated;
            }
            else
            {
                activated = true;
            }
        }

        if (activated)
        {
            carla_stderr("CarlaJackAppClient error: runNonRealtimeThread ended while client is activated");

            const char* const message("Plugin bridge error, process thread has stopped");
            const std::size_t messageSize(std::strlen(message));

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerError);
            fShmNonRtServerControl.writeUInt(messageSize);
            fShmNonRtServerControl.writeCustomData(message, messageSize);
            fShmNonRtServerControl.commitWrite();
        }
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

    carla_debug("CarlaJackAppClient runNonRealtimeThread FINISHED");
}

CARLA_BACKEND_END_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------

using CARLA_BACKEND_NAMESPACE::CarlaJackAppClient;
using CARLA_BACKEND_NAMESPACE::JackClientState;

static CarlaJackAppClient gClient;

CARLA_BACKEND_START_NAMESPACE

static int carla_interposed_callback(int cb_action, void* ptr)
{
    return gClient.handleInterposerCallback(cb_action, ptr);
}

CARLA_BACKEND_END_NAMESPACE

// ---------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
jack_client_t* jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
    carla_debug("%s(%s, 0x%x, %p)", __FUNCTION__, client_name, options, status);

    if (JackClientState* const client = gClient.createClient(client_name))
    {
        if (status != nullptr)
            *status = static_cast<JackStatus>(0x0);

        return (jack_client_t*)client;
    }

    if (status != nullptr)
        *status = JackServerError;

    return nullptr;

    // unused
    (void)options;
}

CARLA_PLUGIN_EXPORT
jack_client_t* jack_client_new(const char* client_name)
{
    return jack_client_open(client_name, JackNullOption, nullptr);
}

CARLA_PLUGIN_EXPORT
int jack_client_close(jack_client_t* client)
{
    carla_debug("%s(%p)", __FUNCTION__, client);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    gClient.destroyClient(jclient);
    return 0;
}

CARLA_PLUGIN_EXPORT
int jack_activate(jack_client_t* client)
{
    carla_debug("%s(%p)", __FUNCTION__, client);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    return gClient.activateClient(jclient) ? 0 : 1;
}

CARLA_PLUGIN_EXPORT
int jack_deactivate(jack_client_t* client)
{
    carla_debug("%s(%p)", __FUNCTION__, client);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    return gClient.deactivateClient(jclient) ? 0 : 1;
}

// ---------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
char* jack_get_client_name_by_uuid(jack_client_t* const client, const char* const uuidstr)
{
    carla_debug("%s(%p, %s)", __FUNCTION__, client, uuidstr);

    jack_uuid_t uuid = JACK_UUID_EMPTY_INITIALIZER;
    CARLA_SAFE_ASSERT_RETURN(jack_uuid_parse(uuidstr, &uuid) == 0, nullptr);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    const char* clientName;

    if (jclient->server.uuid == uuid)
        return strdup("system");

    if (jclient->uuid == uuid)
    {
        clientName = jclient->name;
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr, nullptr);
    }
    else
    {
        CarlaJackAppClient* const jackAppPtr = jclient->server.jackAppPtr;
        CARLA_SAFE_ASSERT_RETURN(jackAppPtr != nullptr && jackAppPtr == &gClient, nullptr);

        clientName = jackAppPtr->getClientNameFromUUID(uuid);
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr, nullptr);
    }

    return strdup(clientName);
}

CARLA_PLUGIN_EXPORT
char* jack_get_uuid_for_client_name(jack_client_t* client, const char* name)
{
    carla_debug("%s(%p, %s)", __FUNCTION__, client, name);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    if (std::strcmp(name, "system") == 0)
    {
        char* const uuidstr = static_cast<char*>(std::malloc(JACK_UUID_STRING_SIZE));
        CARLA_SAFE_ASSERT_RETURN(uuidstr != nullptr, nullptr);

        jack_uuid_unparse(jclient->server.uuid, uuidstr);
        return uuidstr;
    }
    else
    {
        CarlaJackAppClient* const jackAppPtr = jclient->server.jackAppPtr;
        CARLA_SAFE_ASSERT_RETURN(jackAppPtr != nullptr && jackAppPtr == &gClient, nullptr);

        const jack_uuid_t uuid = jackAppPtr->getUUIDForClientName(name);
        CARLA_SAFE_ASSERT_RETURN(uuid != JACK_UUID_EMPTY_INITIALIZER, nullptr);

        char* const uuidstr = static_cast<char*>(std::malloc(JACK_UUID_STRING_SIZE));
        CARLA_SAFE_ASSERT_RETURN(uuidstr != nullptr, nullptr);

        jack_uuid_unparse(jclient->uuid, uuidstr);
        return uuidstr;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
pthread_t jack_client_thread_id(jack_client_t* client)
{
    carla_debug("%s(%p)", __FUNCTION__, client);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    CarlaJackAppClient* const jackAppPtr = jclient->server.jackAppPtr;
    CARLA_SAFE_ASSERT_RETURN(jackAppPtr != nullptr && jackAppPtr == &gClient, 0);

    return jackAppPtr->getRealtimeThreadId();
}

// ---------------------------------------------------------------------------------------------------------------------

#include "jackbridge/JackBridge2.cpp"
#include "CarlaBridgeUtils.cpp"

// ---------------------------------------------------------------------------------------------------------------------
// TODO

CARLA_BACKEND_USE_NAMESPACE

CARLA_PLUGIN_EXPORT
int jack_client_real_time_priority(jack_client_t* client)
{
    carla_debug("%s(%p)", __FUNCTION__, client);

    // code as used by water
    const int minPriority = sched_get_priority_min(SCHED_RR);
    const int maxPriority = sched_get_priority_max(SCHED_RR);
    return ((maxPriority - minPriority) * 9) / 10 + minPriority;

    // unused
    (void)client;
}

int jack_client_create_thread(jack_client_t* client, pthread_t* thread, int priority,
                               int realtime, void *(*start_routine)(void*), void* arg)
{
    carla_stderr2("%s(%p, %p, %i, %i, %p, %p)", __FUNCTION__, client, thread, priority, realtime, start_routine, arg);
    return ENOSYS;
}

typedef void (*JackSessionCallback)(jack_session_event_t*, void*);

CARLA_PLUGIN_EXPORT
int jack_set_session_callback(jack_client_t* client, JackSessionCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
