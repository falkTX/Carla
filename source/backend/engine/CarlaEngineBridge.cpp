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
#include "CarlaBridgeUtils.hpp"
#include "CarlaMIDI.h"

#include "jackbridge/JackBridge.hpp"

#include <cerrno>
#include <ctime>

using juce::File;
using juce::String;

#ifdef JACKBRIDGE_EXPORT
// -------------------------------------------------------------------

bool jackbridge_is_ok() noexcept
{
    return true;
}
#endif

// -------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------

template<typename T>
bool jackbridge_shm_map2(char* shm, T*& value)
{
    value = (T*)jackbridge_shm_map(shm, sizeof(T));
    return (value != nullptr);
}

struct BridgeAudioPool {
    CarlaString filename;
    float* data;
    char shm[32];

    BridgeAudioPool()
        : data(nullptr)
    {
        carla_zeroChar(shm, 32);
        jackbridge_shm_init(shm);
    }

    ~BridgeAudioPool()
    {
        // should be cleared by now
        CARLA_ASSERT(data == nullptr);

        clear();
    }

    bool attach()
    {
        jackbridge_shm_attach(shm, filename);

        return jackbridge_shm_is_valid(shm);
    }

    void clear()
    {
        filename.clear();

        data = nullptr;

        if (jackbridge_shm_is_valid(shm))
            jackbridge_shm_close(shm);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeAudioPool)
};

struct BridgeControl : public CarlaRingBuffer<StackBuffer> {
    CarlaString filename;
    BridgeShmControl* data;
    char shm[32];

    BridgeControl()
        : CarlaRingBuffer<StackBuffer>(),
          data(nullptr)
    {
        carla_zeroChar(shm, 32);
        jackbridge_shm_init(shm);
    }

    ~BridgeControl()
    {
        // should be cleared by now
        CARLA_ASSERT(data == nullptr);

        clear();
    }

    bool attach()
    {
        jackbridge_shm_attach(shm, filename);

        return jackbridge_shm_is_valid(shm);
    }

    void clear()
    {
        filename.clear();

        data = nullptr;

        if (jackbridge_shm_is_valid(shm))
            jackbridge_shm_close(shm);
    }

    bool mapData()
    {
        CARLA_ASSERT(data == nullptr);

        if (jackbridge_shm_map2<BridgeShmControl>(shm, data))
        {
            setRingBuffer(&data->buffer, false);
            return true;
        }

        return false;
    }

    PluginBridgeOpcode readOpcode()
    {
        return static_cast<PluginBridgeOpcode>(readInt());
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeControl)
};

struct BridgeTime {
    CarlaString filename;
    BridgeTimeInfo* info;
    char shm[32];

    BridgeTime()
        : info(nullptr)
    {
        carla_zeroChar(shm, 32);
        jackbridge_shm_init(shm);
    }

    ~BridgeTime()
    {
        // should be cleared by now
        CARLA_ASSERT(info == nullptr);

        clear();
    }

    bool attach()
    {
        jackbridge_shm_attach(shm, filename);

        return jackbridge_shm_is_valid(shm);
    }

    void clear()
    {
        filename.clear();

        info = nullptr;

        if (jackbridge_shm_is_valid(shm))
            jackbridge_shm_close(shm);
    }

    bool mapData()
    {
        CARLA_ASSERT(info == nullptr);

        return jackbridge_shm_map2<BridgeTimeInfo>(shm, info);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(BridgeTime)
};

// -------------------------------------------------------------------

class CarlaEngineBridge : public CarlaEngine,
                          public CarlaThread
{
public:
    CarlaEngineBridge(const char* const audioBaseName, const char* const controlBaseName, const char* const timeBaseName)
        : CarlaEngine(),
          CarlaThread("CarlaEngineBridge"),
          fIsRunning(false),
          fNextUIState(-1)
    {
        carla_stdout("CarlaEngineBridge::CarlaEngineBridge(%s, %s, %s)", audioBaseName, controlBaseName, timeBaseName);

        fShmAudioPool.filename  = "/carla-bridge_shm_";
        fShmAudioPool.filename += audioBaseName;

        fShmControl.filename  = "/carla-bridge_shc_";
        fShmControl.filename += controlBaseName;

        fShmTime.filename  = "/carla-bridge_sht_";
        fShmTime.filename += timeBaseName;
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

        // SHM Audio Pool
        {
            if (! fShmAudioPool.attach())
            {
                carla_stdout("Failed to open or create shared memory file #1");
                return false;
            }
        }

        // SHM Control
        {
            if (! fShmControl.attach())
            {
                carla_stdout("Failed to open or create shared memory file #2");
                // clear
                fShmAudioPool.clear();
                return false;
            }

            if (! fShmControl.mapData())
            {
                carla_stdout("Failed to map shared memory file #2");
                // clear
                fShmControl.clear();
                fShmAudioPool.clear();
                return false;
            }
        }

        // SHM Transport
        {
            if (! fShmTime.attach())
            {
                carla_stdout("Failed to open or create shared memory file #3");
                // clear
                fShmControl.clear();
                fShmAudioPool.clear();
                return false;
            }

            if (! fShmTime.mapData())
            {
                carla_stdout("Failed to map shared memory file #3");
                // clear
                fShmTime.clear();
                fShmControl.clear();
                fShmAudioPool.clear();
                return false;
            }
        }

        // Read values from memory
        PluginBridgeOpcode opcode;

        opcode = fShmControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeOpcodeNull, opcode);

        const uint32_t stackBufferSize = fShmControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(stackBufferSize == sizeof(StackBuffer), stackBufferSize, sizeof(StackBuffer));

        const uint32_t shmStructSize = fShmControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(shmStructSize == sizeof(BridgeShmControl), shmStructSize, sizeof(BridgeShmControl));

        const uint32_t timeStructSize = fShmControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(timeStructSize == sizeof(BridgeTimeInfo), timeStructSize, sizeof(BridgeTimeInfo));

        opcode = fShmControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeOpcodeSetBufferSize, opcode);
        pData->bufferSize = fShmControl.readUInt();

        opcode = fShmControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeOpcodeSetSampleRate, opcode);
        pData->sampleRate = fShmControl.readFloat();

        carla_stdout("Carla Client Info:");
        carla_stdout("  BufferSize: %i", pData->bufferSize);
        carla_stdout("  SampleRate: %f", pData->sampleRate);
        carla_stdout("  sizeof(StackBuffer):      %i/" P_SIZE, stackBufferSize, sizeof(StackBuffer));
        carla_stdout("  sizeof(BridgeShmControl): %i/" P_SIZE, shmStructSize,   sizeof(BridgeShmControl));
        carla_stdout("  sizeof(BridgeTimeInfo):   %i/" P_SIZE, timeStructSize,  sizeof(BridgeTimeInfo));

        CarlaThread::startThread();
        CarlaEngine::init(clientName);
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEnginePlugin::close()");
        CarlaEngine::close();

        CarlaThread::stopThread(6000);

        fShmTime.clear();
        fShmControl.clear();
        fShmAudioPool.clear();

        return true;
    }

    bool isRunning() const noexcept override
    {
        return fIsRunning;
    }

    bool isOffline() const noexcept override
    {
        return false;
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

        if (fNextUIState == -1 || ! fIsRunning)
            return;

        try {
            carla_show_custom_ui(0, bool(fNextUIState));
        } CARLA_SAFE_EXCEPTION("bridge show_custom_ui");

        fNextUIState = -1;
    }

    // -------------------------------------
    // CarlaThread virtual calls

    void run() override
    {
        fIsRunning = true;

        // TODO - set RT permissions
        carla_debug("CarlaEngineBridge::run()");

        for (; ! shouldThreadExit();)
        {
            if (! jackbridge_sem_timedwait(&fShmControl.data->runServer, 5))
            {
                if (errno == ETIMEDOUT)
                {
                    fIsRunning = false;
                    signalThreadShouldExit();
                    return;
                }
            }

            for (; fShmControl.isDataAvailableForReading();)
            {
                const PluginBridgeOpcode opcode(fShmControl.readOpcode());

                if (opcode != kPluginBridgeOpcodeProcess) {
                    carla_debug("CarlaEngineBridge::run() - got opcode: %s", PluginBridgeOpcode2str(opcode));
                }

                switch (opcode)
                {
                case kPluginBridgeOpcodeNull:
                    break;

                case kPluginBridgeOpcodeSetAudioPool: {
                    const int64_t poolSize(fShmControl.readLong());
                    CARLA_SAFE_ASSERT_BREAK(poolSize > 0);
                    fShmAudioPool.data = (float*)jackbridge_shm_map(fShmAudioPool.shm, static_cast<size_t>(poolSize));
                    break;
                }

                case kPluginBridgeOpcodeSetBufferSize: {
                    const uint32_t bufferSize(fShmControl.readUInt());
                    bufferSizeChanged(bufferSize);
                    break;
                }

                case kPluginBridgeOpcodeSetSampleRate: {
                    const float sampleRate(fShmControl.readFloat());
                    sampleRateChanged(sampleRate);
                    break;
                }

                case kPluginBridgeOpcodeSetParameterRt:
                case kPluginBridgeOpcodeSetParameterNonRt:{
                    const int32_t index(fShmControl.readInt());
                    const float   value(fShmControl.readFloat());

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->isEnabled())
                    {
                        if (index == PARAMETER_ACTIVE)
                        {
                            plugin->setActive((value > 0.0f), false, false);
                            break;
                        }

                        CARLA_SAFE_ASSERT_BREAK(index >= 0);

                        plugin->setParameterValue(static_cast<uint32_t>(index), value, (opcode == kPluginBridgeOpcodeSetParameterNonRt), false, false);
                    }
                    break;
                }

                case kPluginBridgeOpcodeSetProgram: {
                    const int32_t index(fShmControl.readInt());
                    CARLA_SAFE_ASSERT_BREAK(index >= 0);

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->isEnabled())
                    {
                        plugin->setProgram(index, false, false, false);
                        //plugin->postponeRtEvent(kPluginPostRtEventProgramChange, index, 0, 0.0f);
                    }
                    break;
                }

                case kPluginBridgeOpcodeSetMidiProgram: {
                    const int32_t index(fShmControl.readInt());
                    CARLA_SAFE_ASSERT_BREAK(index >= 0);

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->isEnabled())
                    {
                        plugin->setMidiProgram(index, false, false, false);
                        //plugin->postponeRtEvent(kPluginPostRtEventMidiProgramChange, index, 0, 0.0f);
                    }

                    break;
                }

                case kPluginBridgeOpcodeSetChunkFile: {
                    const uint32_t size(fShmControl.readUInt());
                    CARLA_SAFE_ASSERT_BREAK(size > 0);

                    char chunkFilePathTry[size+1];
                    carla_zeroChar(chunkFilePathTry, size+1);
                    fShmControl.readCustomData(chunkFilePathTry, size);

                    CARLA_SAFE_ASSERT_BREAK(chunkFilePathTry[0] != '\0');

                    String chunkFilePath(chunkFilePathTry);
#ifdef CARLA_OS_WIN
                    if (chunkFilePath.startsWith("/"))
                    {
                        // running under Wine, posix host
                        chunkFilePath = chunkFilePath.replaceSection(0, 1, "Z:\\");
                        chunkFilePath = chunkFilePath.replace("/", "\\");
                    }
#endif

                    File chunkFile(chunkFilePath);
                    CARLA_SAFE_ASSERT_BREAK(chunkFile.existsAsFile());

                    String chunkData(chunkFile.loadFileAsString());
                    chunkFile.deleteFile();
                    CARLA_SAFE_ASSERT_BREAK(chunkData.isNotEmpty());

                    carla_set_chunk_data(0, chunkData.toRawUTF8());
                    carla_stdout("chunk sent, size:%i", chunkData.length());
                    break;
                }

                case kPluginBridgeOpcodePrepareForSave: {
                    carla_prepare_for_save(0);

                    for (uint32_t i=0, count=carla_get_custom_data_count(0); i<count; ++i)
                    {
                        const CarlaBackend::CustomData* const cdata(carla_get_custom_data(0, i));
                        CARLA_SAFE_ASSERT_CONTINUE(cdata != nullptr);

                        oscSend_bridge_set_custom_data(cdata->type, cdata->key, cdata->value);
                    }

                    //if (fPlugin->getOptionsEnabled() & CarlaBackend::PLUGIN_OPTION_USE_CHUNKS)
                    {
                        if (const char* const chunkData = carla_get_chunk_data(0))
                        {
                            String filePath(File::getSpecialLocation(File::tempDirectory).getFullPathName());

                            filePath += OS_SEP_STR;
                            filePath += ".CarlaChunk_";
                            filePath += fShmAudioPool.filename.buffer() + 18;

                            if (File(filePath).replaceWithText(chunkData))
                                oscSend_bridge_set_chunk_data(filePath.toRawUTF8());
                        }
                    }

                    carla_stdout("-----------------------------------------------------, got prepare for save");

                    oscSend_bridge_configure(CARLA_BRIDGE_MSG_SAVED, "");
                    break;
                }

                case kPluginBridgeOpcodeMidiEvent: {
                    const int64_t time(fShmControl.readLong());
                    const int32_t size(fShmControl.readInt());
                    CARLA_SAFE_ASSERT_BREAK(time >= 0);
                    CARLA_SAFE_ASSERT_BREAK(size > 0 && size <= 4);

                    uint8_t data[size];

                    for (int32_t i=0; i < size; ++i)
                        data[i] = fShmControl.readByte();

                    CARLA_SAFE_ASSERT_BREAK(pData->events.in != nullptr);

                    for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
                    {
                        EngineEvent& event(pData->events.in[i]);

                        if (event.type != kEngineEventTypeNull)
                            continue;

                        event.fillFromMidiData(static_cast<uint8_t>(size), data);
                        break;
                    }
                    break;
                }

                case kPluginBridgeOpcodeProcess: {
                    CARLA_SAFE_ASSERT_BREAK(fShmAudioPool.data != nullptr);
                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    BridgeTimeInfo* const bridgeInfo(fShmTime.info);
                    CARLA_SAFE_ASSERT_BREAK(bridgeInfo != nullptr);

                    if (plugin != nullptr && plugin->isEnabled() && plugin->tryLock(true)) // FIXME - always lock?
                    {
                        const uint32_t inCount(plugin->getAudioInCount());
                        const uint32_t outCount(plugin->getAudioOutCount());

                        float* inBuffer[inCount];
                        float* outBuffer[outCount];

                        for (uint32_t i=0; i < inCount; ++i)
                            inBuffer[i] = fShmAudioPool.data + i*pData->bufferSize;
                        for (uint32_t i=0; i < outCount; ++i)
                            outBuffer[i] = fShmAudioPool.data + (i+inCount)*pData->bufferSize;

                        EngineTimeInfo& timeInfo(pData->timeInfo);

                        timeInfo.playing = bridgeInfo->playing;
                        timeInfo.frame   = bridgeInfo->frame;
                        timeInfo.usecs   = bridgeInfo->usecs;
                        timeInfo.valid   = bridgeInfo->valid;

                        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
                        {
                            timeInfo.bbt.bar  = bridgeInfo->bar;
                            timeInfo.bbt.beat = bridgeInfo->beat;
                            timeInfo.bbt.tick = bridgeInfo->tick;

                            timeInfo.bbt.beatsPerBar = bridgeInfo->beatsPerBar;
                            timeInfo.bbt.beatType    = bridgeInfo->beatType;

                            timeInfo.bbt.ticksPerBeat   = bridgeInfo->ticksPerBeat;
                            timeInfo.bbt.beatsPerMinute = bridgeInfo->beatsPerMinute;
                            timeInfo.bbt.barStartTick   = bridgeInfo->barStartTick;
                        }

                        plugin->initBuffers();
                        plugin->process(inBuffer, outBuffer, pData->bufferSize);
                        plugin->unlock();
                    }

                    // clear buffer
                    CARLA_SAFE_ASSERT_BREAK(pData->events.in != nullptr);

                    if (pData->events.in[0].type != kEngineEventTypeNull)
                        carla_zeroStruct<EngineEvent>(pData->events.in, kMaxEngineEventInternalCount);
                    break;
                }

                case kPluginBridgeOpcodeShowUI:
                    carla_stdout("-----------------------------------------------------, got SHOW UI");
                    fNextUIState = 1;
                    break;

                case kPluginBridgeOpcodeHideUI:
                    carla_stdout("-----------------------------------------------------, got HIDE UI");
                    fNextUIState = 0;
                    break;

                case kPluginBridgeOpcodeQuit:
                    signalThreadShouldExit();
                    fIsRunning = false;
                    break;

                default:
                    carla_stderr2("Unhandled Plugin opcode %i", opcode);
                    break;
                }
            }

            if (! jackbridge_sem_post(&fShmControl.data->runClient))
                carla_stderr2("Could not post to semaphore");
        }

        fIsRunning = false;

        callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);
    }

private:
    BridgeAudioPool fShmAudioPool;
    BridgeControl   fShmControl;
    BridgeTime      fShmTime;

    volatile bool fIsRunning;
    volatile int  fNextUIState;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineBridge)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newBridge(const char* const audioBaseName, const char* const controlBaseName, const char* const timeBaseName)
{
    return new CarlaEngineBridge(audioBaseName, controlBaseName, timeBaseName);
}

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
