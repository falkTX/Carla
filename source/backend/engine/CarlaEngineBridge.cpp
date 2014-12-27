/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef BUILD_BRIDGE
# error This file should not be compiled if not building bridge
#endif

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBase64Utils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaMIDI.h"

#include "jackbridge/JackBridge.hpp"

using juce::File;
using juce::MemoryBlock;
using juce::String;

template<typename T>
bool jackbridge_shm_map2(char* shm, T*& value) noexcept
{
    value = (T*)jackbridge_shm_map(shm, sizeof(T));
    return (value != nullptr);
}

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------

struct BridgeAudioPool {
    CarlaString filename;
    float* data;
    char shm[64];

    BridgeAudioPool() noexcept
        : filename(),
          data(nullptr)
    {
        carla_zeroChar(shm, 64);
        jackbridge_shm_init(shm);
    }

    ~BridgeAudioPool() noexcept
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (! jackbridge_shm_is_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        data = nullptr;

        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
    }

    bool attach() noexcept
    {
        // must be invalid right now
        CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

        jackbridge_shm_attach(shm, filename);

        return jackbridge_shm_is_valid(shm);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeAudioPool)
};

// -------------------------------------------------------------------

struct BridgeRtClientControl : public CarlaRingBufferControl<SmallStackBuffer> {
    CarlaString filename;
    BridgeRtClientData* data;
    char shm[64];

    BridgeRtClientControl() noexcept
        : filename(),
          data(nullptr)
    {
        carla_zeroChar(shm, 64);
        jackbridge_shm_init(shm);
    }

    ~BridgeRtClientControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (! jackbridge_shm_is_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        data = nullptr;

        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
    }

    bool attach() noexcept
    {
        // must be invalid right now
        CARLA_SAFE_ASSERT_RETURN(! jackbridge_shm_is_valid(shm), false);

        jackbridge_shm_attach(shm, filename);

        return jackbridge_shm_is_valid(shm);
    }

    bool mapData() noexcept
    {
        CARLA_SAFE_ASSERT(data == nullptr);

        if (jackbridge_shm_map2<BridgeRtClientData>(shm, data))
        {
            CARLA_SAFE_ASSERT(data->midiOut[0] == 0);
            setRingBuffer(&data->ringBuffer, false);
            return true;
        }

        return false;
    }

    bool postClient() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        return jackbridge_sem_post(&data->sem.client);
    }

    bool waitForServer(const uint secs) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        return jackbridge_sem_timedwait(&data->sem.server, secs);
    }

    PluginBridgeRtClientOpcode readOpcode() noexcept
    {
        return static_cast<PluginBridgeRtClientOpcode>(readUInt());
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeRtClientControl)
};

// -------------------------------------------------------------------

struct BridgeNonRtClientControl : public CarlaRingBufferControl<BigStackBuffer> {
    CarlaString filename;
    BridgeNonRtClientData* data;
    char shm[64];

    BridgeNonRtClientControl() noexcept
        : filename(),
          data(nullptr)
    {
        carla_zeroChar(shm, 64);
        jackbridge_shm_init(shm);
    }

    ~BridgeNonRtClientControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    bool attach() noexcept
    {
        jackbridge_shm_attach(shm, filename);

        return jackbridge_shm_is_valid(shm);
    }

    void clear() noexcept
    {
        filename.clear();

        if (! jackbridge_shm_is_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        data = nullptr;

        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
    }

    bool mapData() noexcept
    {
        CARLA_SAFE_ASSERT(data == nullptr);

        if (jackbridge_shm_map2<BridgeNonRtClientData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, false);
            return true;
        }

        return false;
    }

    PluginBridgeNonRtClientOpcode readOpcode() noexcept
    {
        return static_cast<PluginBridgeNonRtClientOpcode>(readUInt());
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeNonRtClientControl)
};

// -------------------------------------------------------------------

struct BridgeNonRtServerControl : public CarlaRingBufferControl<HugeStackBuffer> {
    CarlaMutex mutex;
    CarlaString filename;
    BridgeNonRtServerData* data;
    char shm[64];

    BridgeNonRtServerControl() noexcept
        :  mutex(),
          filename(),
          data(nullptr)
    {
        carla_zeroChar(shm, 64);
        jackbridge_shm_init(shm);
    }

    ~BridgeNonRtServerControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    bool attach() noexcept
    {
        jackbridge_shm_attach(shm, filename);

        return jackbridge_shm_is_valid(shm);
    }

    void clear() noexcept
    {
        filename.clear();

        if (! jackbridge_shm_is_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

        data = nullptr;

        jackbridge_shm_close(shm);
        jackbridge_shm_init(shm);
    }

    bool mapData() noexcept
    {
        CARLA_SAFE_ASSERT(data == nullptr);

        if (jackbridge_shm_map2<BridgeNonRtServerData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, false);
            return true;
        }

        return false;
    }

    void writeOpcode(const PluginBridgeNonRtServerOpcode opcode) noexcept
    {
        writeUInt(static_cast<uint32_t>(opcode));
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeNonRtServerControl)
};

// -------------------------------------------------------------------

class CarlaEngineBridge : public CarlaEngine,
                          public CarlaThread
{
public:
    CarlaEngineBridge(const char* const audioPoolBaseName, const char* const rtClientBaseName, const char* const nonRtClientBaseName, const char* const nonRtServerBaseName)
        : CarlaEngine(),
          CarlaThread("CarlaEngineBridge"),
          fShmAudioPool(),
          fShmRtClientControl(),
          fShmNonRtClientControl(),
          fShmNonRtServerControl(),
          fIsRunning(false),
          fIsOffline(false),
          leakDetector_CarlaEngineBridge()
    {
        carla_stdout("CarlaEngineBridge::CarlaEngineBridge(\"%s\", \"%s\", \"%s\", \"%s\")", audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);

        fShmAudioPool.filename  = PLUGIN_BRIDGE_NAMEPREFIX_AUDIO_POOL;
        fShmAudioPool.filename += audioPoolBaseName;

        fShmRtClientControl.filename  = PLUGIN_BRIDGE_NAMEPREFIX_RT_CLIENT;
        fShmRtClientControl.filename += rtClientBaseName;

        fShmNonRtClientControl.filename  = PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_CLIENT;
        fShmNonRtClientControl.filename += nonRtClientBaseName;

        fShmNonRtServerControl.filename  = PLUGIN_BRIDGE_NAMEPREFIX_NON_RT_SERVER;
        fShmNonRtServerControl.filename += nonRtServerBaseName;
    }

    ~CarlaEngineBridge() noexcept override
    {
        carla_debug("CarlaEngineBridge::~CarlaEngineBridge()");
    }

    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineBridge::init(\"%s\")", clientName);

        if (! pData->init(clientName))
        {
            setLastError("Failed to init internal data");
            return false;
        }

        if (! fShmAudioPool.attach())
        {
            carla_stdout("Failed to attach to audio pool shared memory");
            return false;
        }

        if (! fShmRtClientControl.attach())
        {
            clear();
            carla_stdout("Failed to attach to rt control shared memory");
            return false;
        }

        if (! fShmRtClientControl.mapData())
        {
            clear();
            carla_stdout("Failed to map rt control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.attach())
        {
            clear();
            carla_stdout("Failed to attach to non-rt control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.mapData())
        {
            clear();
            carla_stdout("Failed to map non-rt control shared memory");
            return false;
        }

        PluginBridgeNonRtClientOpcode opcode;

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientNull, opcode);

        const uint32_t shmRtDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmRtDataSize == sizeof(BridgeRtClientData), shmRtDataSize, sizeof(BridgeRtClientData));

        const uint32_t shmNonRtDataSize = fShmNonRtClientControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmNonRtDataSize == sizeof(BridgeNonRtClientData), shmNonRtDataSize, sizeof(BridgeNonRtClientData));

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetBufferSize, opcode);
        pData->bufferSize = fShmNonRtClientControl.readUInt();

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetSampleRate, opcode);
        pData->sampleRate = fShmNonRtClientControl.readDouble();

        carla_stdout("Carla Client Info:");
        carla_stdout("  BufferSize: %i", pData->bufferSize);
        carla_stdout("  SampleRate: %g", pData->sampleRate);
        carla_stdout("  sizeof(BridgeRtData):    %i/" P_SIZE, shmRtDataSize,    sizeof(BridgeRtClientData));
        carla_stdout("  sizeof(BridgeNonRtData): %i/" P_SIZE, shmNonRtDataSize, sizeof(BridgeNonRtClientData));

        if (shmRtDataSize != sizeof(BridgeRtClientData) || shmNonRtDataSize != sizeof(BridgeNonRtClientData))
            return false;

        startThread();

        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEnginePlugin::close()");
        CarlaEngine::close();

        stopThread(5000);
        clear();

        return true;
    }

    bool isRunning() const noexcept override
    {
        return isThreadRunning();
    }

    bool isOffline() const noexcept override
    {
        return fIsOffline;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypeBridge;
    }

    const char* getCurrentDriverName() const noexcept
    {
        return "Bridge";
    }

    void idle() noexcept override
    {
        CarlaEngine::idle();

        // TODO - send output parameters to server

        try {
            handleNonRtData();
        } CARLA_SAFE_EXCEPTION("handleNonRtData");
    }

    // -------------------------------------------------------------------

    void clear() noexcept
    {
        fShmAudioPool.clear();
        fShmRtClientControl.clear();
        fShmNonRtClientControl.clear();
    }

    void handleNonRtData()
    {
        for (; fShmNonRtClientControl.isDataAvailableForReading();)
        {
            const PluginBridgeNonRtClientOpcode opcode(fShmNonRtClientControl.readOpcode());
            CarlaPlugin* const plugin(pData->plugins[0].plugin);

#ifdef DEBUG
            if (opcode != kPluginBridgeNonRtClientPing) {
                carla_debug("CarlaEngineBridge::handleNonRtData() - got opcode: %s", PluginBridgeNonRtClientOpcode2str(opcode));
            }
#endif

            switch (opcode)
            {
            case kPluginBridgeNonRtClientNull:
                break;

            case kPluginBridgeNonRtClientPing:
                //oscSend_bridge_pong();
                break;

            case kPluginBridgeNonRtClientActivate:
                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setActive(true, false, false);
                break;

            case kPluginBridgeNonRtClientDeactivate:
                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setActive(false, false, false);
                break;

            case kPluginBridgeNonRtClientSetBufferSize: {
                const uint32_t bufferSize(fShmNonRtClientControl.readUInt());
                pData->bufferSize = bufferSize;
                bufferSizeChanged(bufferSize);
                break;
            }

            case kPluginBridgeNonRtClientSetSampleRate: {
                const double sampleRate(fShmNonRtClientControl.readDouble());
                pData->sampleRate = sampleRate;
                sampleRateChanged(sampleRate);
                break;
            }

            case kPluginBridgeNonRtClientSetOffline:
                fIsOffline = true;
                offlineModeChanged(true);
                break;

            case kPluginBridgeNonRtClientSetOnline:
                fIsOffline = false;
                offlineModeChanged(false);
                break;

            case kPluginBridgeNonRtClientSetParameterValue: {
                const uint32_t index(fShmNonRtClientControl.readUInt());
                const float    value(fShmNonRtClientControl.readFloat());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setParameterValue(index, value, false, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetParameterMidiChannel: {
                const uint32_t index(fShmNonRtClientControl.readUInt());
                const uint8_t  channel(fShmNonRtClientControl.readByte());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setParameterMidiChannel(index, channel, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetParameterMidiCC: {
                const uint32_t index(fShmNonRtClientControl.readUInt());
                const int16_t  cc(fShmNonRtClientControl.readShort());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setParameterMidiCC(index, cc, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetProgram: {
                const int32_t index(fShmNonRtClientControl.readInt());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setProgram(index, false, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetMidiProgram: {
                const int32_t index(fShmNonRtClientControl.readInt());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setMidiProgram(index, false, false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetCustomData: {
                // type
                const uint32_t typeSize(fShmNonRtClientControl.readUInt());
                char typeStr[typeSize+1];
                carla_zeroChar(typeStr, typeSize+1);
                fShmNonRtClientControl.readCustomData(typeStr, typeSize);

                // key
                const uint32_t keySize(fShmNonRtClientControl.readUInt());
                char keyStr[keySize+1];
                carla_zeroChar(keyStr, keySize+1);
                fShmNonRtClientControl.readCustomData(keyStr, keySize);

                // value
                const uint32_t valueSize(fShmNonRtClientControl.readUInt());
                char valueStr[valueSize+1];
                carla_zeroChar(valueStr, valueSize+1);
                fShmNonRtClientControl.readCustomData(valueStr, valueSize);

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setCustomData(typeStr, keyStr, valueStr, true);
                break;
            }

            case kPluginBridgeNonRtClientSetChunkDataFile: {
                const uint32_t size(fShmNonRtClientControl.readUInt());
                CARLA_SAFE_ASSERT_BREAK(size > 0);

                char chunkFilePathTry[size+1];
                carla_zeroChar(chunkFilePathTry, size+1);
                fShmNonRtClientControl.readCustomData(chunkFilePathTry, size);

                CARLA_SAFE_ASSERT_BREAK(chunkFilePathTry[0] != '\0');
                if (plugin == nullptr || ! plugin->isEnabled()) break;

                String chunkFilePath(chunkFilePathTry);

#ifdef CARLA_OS_WIN
                // check if running under Wine
                if (chunkFilePath.startsWith("/"))
                    chunkFilePath = chunkFilePath.replaceSection(0, 1, "Z:\\").replace("/", "\\");
#endif

                File chunkFile(chunkFilePath);
                CARLA_SAFE_ASSERT_BREAK(chunkFile.existsAsFile());

                String chunkDataBase64(chunkFile.loadFileAsString());
                chunkFile.deleteFile();
                CARLA_SAFE_ASSERT_BREAK(chunkDataBase64.isNotEmpty());

                std::vector<uint8_t> chunk(carla_getChunkFromBase64String(chunkDataBase64.toRawUTF8()));
                plugin->setChunkData(chunk.data(), chunk.size());
                break;
            }

            case kPluginBridgeNonRtClientSetCtrlChannel: {
                const int16_t channel(fShmNonRtClientControl.readShort());
                CARLA_SAFE_ASSERT_BREAK(channel >= -1 && channel < MAX_MIDI_CHANNELS);

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setCtrlChannel(static_cast<int8_t>(channel), false, false);
                break;
            }

            case kPluginBridgeNonRtClientSetOption: {
                const uint32_t option(fShmNonRtClientControl.readUInt());
                const bool     yesNo(fShmNonRtClientControl.readBool());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setOption(option, yesNo, false);
                break;
            }

            case kPluginBridgeNonRtClientPrepareForSave: {
                if (plugin == nullptr || ! plugin->isEnabled()) break;

                plugin->prepareForSave();

                //for (uint32_t i=0, count=plugin->getCustomDataCount(); i<count; ++i)
                {
                    //const CustomData& cdata(plugin->getCustomData(i));
                    //oscSend_bridge_set_custom_data(cdata.type, cdata.key, cdata.value);
                }

                if (plugin->getOptionsEnabled() & PLUGIN_OPTION_USE_CHUNKS)
                {
                    /*
                    void* data = nullptr;
                    if (const std::size_t dataSize = plugin->getChunkData(&data))
                    {
                        CARLA_SAFE_ASSERT_BREAK(data != nullptr);

                        CarlaString dataBase64 = CarlaString::asBase64(data, dataSize);
                        CARLA_SAFE_ASSERT_BREAK(dataBase64.length() > 0);

                        String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

                        filePath += CARLA_OS_SEP_STR;
                        filePath += ".CarlaChunk_";
                        filePath += fShmNonRtClientControl.filename.buffer() + 24;

                        if (File(filePath).replaceWithText(dataBase64.buffer()))
                            oscSend_bridge_set_chunk_data_file(filePath.toRawUTF8());
                    }
                    */
                }

                //oscSend_bridge_configure(CARLA_BRIDGE_MSG_SAVED, "");
                break;
            }

            case kPluginBridgeNonRtClientShowUI:
                if (plugin != nullptr && plugin->isEnabled())
                    plugin->showCustomUI(true);
                break;

            case kPluginBridgeNonRtClientHideUI:
                if (plugin != nullptr && plugin->isEnabled())
                    plugin->showCustomUI(false);
                break;

            case kPluginBridgeNonRtClientUiParameterChange: {
                const uint32_t index(fShmNonRtClientControl.readUInt());
                const float    value(fShmNonRtClientControl.readFloat());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->uiParameterChange(index, value);
                break;
            }

            case kPluginBridgeNonRtClientUiProgramChange: {
                const uint32_t index(fShmNonRtClientControl.readUInt());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->uiProgramChange(index);
                break;
            }

            case kPluginBridgeNonRtClientUiMidiProgramChange: {
                const uint32_t index(fShmNonRtClientControl.readUInt());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->uiMidiProgramChange(index);
                break;
            }

            case kPluginBridgeNonRtClientUiNoteOn: {
                const uint8_t chnl(fShmNonRtClientControl.readByte());
                const uint8_t note(fShmNonRtClientControl.readByte());
                const uint8_t velo(fShmNonRtClientControl.readByte());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->uiNoteOn(chnl, note, velo);
                break;
            }

            case kPluginBridgeNonRtClientUiNoteOff: {
                const uint8_t chnl(fShmNonRtClientControl.readByte());
                const uint8_t note(fShmNonRtClientControl.readByte());

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->uiNoteOff(chnl, note);
                break;
            }

            case kPluginBridgeNonRtClientQuit:
                signalThreadShouldExit();
                callback(ENGINE_CALLBACK_QUIT, 0, 0, 0, 0.0f, nullptr);
                break;
            }
        }
    }

    // -------------------------------------------------------------------

protected:
    void run() override
    {
        for (; ! shouldThreadExit();)
        {
            if (! fShmRtClientControl.waitForServer(5))
            {
                carla_stderr2("Bridge timed-out, final post...");
                fShmRtClientControl.postClient();
                carla_stderr2("Bridge timed-out, done.");
                signalThreadShouldExit();
                break;
            }

            for (; fShmRtClientControl.isDataAvailableForReading();)
            {
                const PluginBridgeRtClientOpcode opcode(fShmRtClientControl.readOpcode());
                CarlaPlugin* const plugin(pData->plugins[0].plugin);

#ifdef DEBUG
                if (opcode != kPluginBridgeRtClientProcess && opcode != kPluginBridgeRtClientMidiEvent) {
                    carla_debug("CarlaEngineBridgeRtThread::run() - got opcode: %s", PluginBridgeRtClientOpcode2str(opcode));
                }
#endif

                switch (opcode)
                {
                case kPluginBridgeRtClientNull:
                    break;

                case kPluginBridgeRtClientSetAudioPool: {
                    const uint64_t poolSize(fShmRtClientControl.readULong());
                    CARLA_SAFE_ASSERT_BREAK(poolSize > 0);
                    fShmAudioPool.data = (float*)jackbridge_shm_map(fShmAudioPool.shm, static_cast<size_t>(poolSize));
                    break;
                }

                case kPluginBridgeRtClientControlEventParameter: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());
                    const uint16_t param(fShmRtClientControl.readUShort());
                    const float    value(fShmRtClientControl.readFloat());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type    = kEngineEventTypeControl;
                        event->time    = time;
                        event->channel = channel;
                        event->ctrl.type  = kEngineControlEventTypeParameter;
                        event->ctrl.param = param;
                        event->ctrl.value = value;
                    }
                    break;
                }

                case kPluginBridgeRtClientControlEventMidiBank: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());
                    const uint16_t index(fShmRtClientControl.readUShort());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type    = kEngineEventTypeControl;
                        event->time    = time;
                        event->channel = channel;
                        event->ctrl.type  = kEngineControlEventTypeMidiBank;
                        event->ctrl.param = index;
                        event->ctrl.value = 0.0f;
                    }
                    break;
                }

                case kPluginBridgeRtClientControlEventMidiProgram: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());
                    const uint16_t index(fShmRtClientControl.readUShort());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type    = kEngineEventTypeControl;
                        event->time    = time;
                        event->channel = channel;
                        event->ctrl.type  = kEngineControlEventTypeMidiProgram;
                        event->ctrl.param = index;
                        event->ctrl.value = 0.0f;
                    }
                    break;
                }

                case kPluginBridgeRtClientControlEventAllSoundOff: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type    = kEngineEventTypeControl;
                        event->time    = time;
                        event->channel = channel;
                        event->ctrl.type  = kEngineControlEventTypeAllSoundOff;
                        event->ctrl.param = 0;
                        event->ctrl.value = 0.0f;
                    }
                }

                case kPluginBridgeRtClientControlEventAllNotesOff: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  channel(fShmRtClientControl.readByte());

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type    = kEngineEventTypeControl;
                        event->time    = time;
                        event->channel = channel;
                        event->ctrl.type  = kEngineControlEventTypeAllNotesOff;
                        event->ctrl.param = 0;
                        event->ctrl.value = 0.0f;
                    }
                }

                case kPluginBridgeRtClientMidiEvent: {
                    const uint32_t time(fShmRtClientControl.readUInt());
                    const uint8_t  port(fShmRtClientControl.readByte());
                    const uint8_t  size(fShmRtClientControl.readByte());
                    CARLA_SAFE_ASSERT_BREAK(size > 0);

                    uint8_t data[size];

                    for (uint8_t i=0; i<size; ++i)
                        data[i] = fShmRtClientControl.readByte();

                    if (EngineEvent* const event = getNextFreeInputEvent())
                    {
                        event->type    = kEngineEventTypeMidi;
                        event->time    = time;
                        event->channel = MIDI_GET_CHANNEL_FROM_DATA(data);

                        event->midi.port = port;
                        event->midi.size = size;

                        if (size > EngineMidiEvent::kDataSize)
                        {
                            event->midi.dataExt = data;
                            std::memset(event->midi.data, 0, sizeof(uint8_t)*EngineMidiEvent::kDataSize);
                        }
                        else
                        {
                            event->midi.data[0] = MIDI_GET_STATUS_FROM_DATA(data);

                            uint8_t i=1;
                            for (; i < size; ++i)
                                event->midi.data[i] = data[i];
                            for (; i < EngineMidiEvent::kDataSize; ++i)
                                event->midi.data[i] = 0;

                            event->midi.dataExt = nullptr;
                        }
                    }
                    break;
                }

                case kPluginBridgeRtClientProcess: {
                    CARLA_SAFE_ASSERT_BREAK(fShmAudioPool.data != nullptr);

                    if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock(false))
                    {
                        const BridgeTimeInfo& bridgeTimeInfo(fShmRtClientControl.data->timeInfo);

                        const uint32_t audioInCount(plugin->getAudioInCount());
                        const uint32_t audioOutCount(plugin->getAudioOutCount());
                        const uint32_t cvInCount(plugin->getCVInCount());
                        const uint32_t cvOutCount(plugin->getCVOutCount());

                        const float* audioIn[audioInCount];
                        /* */ float* audioOut[audioOutCount];
                        const float* cvIn[cvInCount];
                        /* */ float* cvOut[cvOutCount];

                        for (uint32_t i=0; i < audioInCount; ++i)
                            audioIn[i] = fShmAudioPool.data + i*pData->bufferSize;
                        for (uint32_t i=0; i < audioOutCount; ++i)
                            audioOut[i] = fShmAudioPool.data + (i+audioInCount)*pData->bufferSize;

                        for (uint32_t i=0; i < cvInCount; ++i)
                            cvIn[i] = fShmAudioPool.data + i*pData->bufferSize;
                        for (uint32_t i=0; i < cvOutCount; ++i)
                            cvOut[i] = fShmAudioPool.data + (i+cvInCount)*pData->bufferSize;

                        EngineTimeInfo& timeInfo(pData->timeInfo);

                        timeInfo.playing = bridgeTimeInfo.playing;
                        timeInfo.frame   = bridgeTimeInfo.frame;
                        timeInfo.usecs   = bridgeTimeInfo.usecs;
                        timeInfo.valid   = bridgeTimeInfo.valid;

                        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
                        {
                            timeInfo.bbt.bar  = bridgeTimeInfo.bar;
                            timeInfo.bbt.beat = bridgeTimeInfo.beat;
                            timeInfo.bbt.tick = bridgeTimeInfo.tick;

                            timeInfo.bbt.beatsPerBar = bridgeTimeInfo.beatsPerBar;
                            timeInfo.bbt.beatType    = bridgeTimeInfo.beatType;

                            timeInfo.bbt.ticksPerBeat   = bridgeTimeInfo.ticksPerBeat;
                            timeInfo.bbt.beatsPerMinute = bridgeTimeInfo.beatsPerMinute;
                            timeInfo.bbt.barStartTick   = bridgeTimeInfo.barStartTick;
                        }

                        plugin->initBuffers();
                        plugin->process(audioIn, audioOut, cvIn, cvOut, pData->bufferSize);
                        plugin->unlock();
                    }

                    // clear buffer
                    CARLA_SAFE_ASSERT_BREAK(pData->events.in != nullptr);

                    if (pData->events.in[0].type != kEngineEventTypeNull)
                        carla_zeroStruct<EngineEvent>(pData->events.in, kMaxEngineEventInternalCount);

                    break;
                }

                case kPluginBridgeRtClientQuit:
                    signalThreadShouldExit();
                    break;
                }
            }

            if (! fShmRtClientControl.postClient())
                carla_stderr2("Could not post to client rt semaphore");
        }

        fIsRunning = false;
        callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);
    }

    // called from process thread above
    EngineEvent* getNextFreeInputEvent() const noexcept
    {
        for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
        {
            EngineEvent& event(pData->events.in[i]);

            if (event.type == kEngineEventTypeNull)
                return &event;
        }
        return nullptr;
    }

    // -------------------------------------------------------------------

private:
    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    bool fIsRunning;
    bool fIsOffline;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineBridge)
};

// -----------------------------------------------------------------------

CarlaEngine* CarlaEngine::newBridge(const char* const audioPoolBaseName, const char* const rtClientBaseName, const char* const nonRtClientBaseName, const char* const nonRtServerBaseName)
{
    return new CarlaEngineBridge(audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);
}

// -----------------------------------------------------------------------

#ifdef BRIDGE_PLUGIN
CarlaPlugin* CarlaPlugin::newNative(const CarlaPlugin::Initializer&)              { return nullptr; }
CarlaPlugin* CarlaPlugin::newFileGIG(const CarlaPlugin::Initializer&, const bool) { return nullptr; }
CarlaPlugin* CarlaPlugin::newFileSF2(const CarlaPlugin::Initializer&, const bool) { return nullptr; }
CarlaPlugin* CarlaPlugin::newFileSFZ(const CarlaPlugin::Initializer&)             { return nullptr; }
#endif

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------

#if defined(CARLA_OS_WIN) && ! defined(__WINE__)
extern "C" __declspec (dllexport)
#else
extern "C" __attribute__ ((visibility("default")))
#endif
void carla_register_native_plugin_carla();
void carla_register_native_plugin_carla(){}

// -----------------------------------------------------------------------
