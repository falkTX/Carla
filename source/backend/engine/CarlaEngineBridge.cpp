/*
 * Carla Plugin Host
 * Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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
using juce::Time;

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
        carla_zeroChars(shm, 64);
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
        carla_zeroChars(shm, 64);
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

        if (data != nullptr)
            unmapData();

        if (! jackbridge_shm_is_valid(shm))
            return;

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

    void unmapData() noexcept
    {
        data = nullptr;
        setRingBuffer(nullptr, false);
    }

    void postClient() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        jackbridge_sem_post(&data->sem.client);
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
        carla_zeroChars(shm, 64);
        jackbridge_shm_init(shm);
    }

    ~BridgeNonRtClientControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (data != nullptr)
            unmapData();

        if (! jackbridge_shm_is_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

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

        if (jackbridge_shm_map2<BridgeNonRtClientData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, false);
            return true;
        }

        return false;
    }

    void unmapData() noexcept
    {
        data = nullptr;
        setRingBuffer(nullptr, false);
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
        carla_zeroChars(shm, 64);
        jackbridge_shm_init(shm);
    }

    ~BridgeNonRtServerControl() noexcept override
    {
        // should be cleared by now
        CARLA_SAFE_ASSERT(data == nullptr);

        clear();
    }

    void clear() noexcept
    {
        filename.clear();

        if (data != nullptr)
            unmapData();

        if (! jackbridge_shm_is_valid(shm))
        {
            CARLA_SAFE_ASSERT(data == nullptr);
            return;
        }

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

        if (jackbridge_shm_map2<BridgeNonRtServerData>(shm, data))
        {
            setRingBuffer(&data->ringBuffer, false);
            return true;
        }

        return false;
    }

    void unmapData() noexcept
    {
        data = nullptr;
        setRingBuffer(nullptr, false);
    }

    void writeOpcode(const PluginBridgeNonRtServerOpcode opcode) noexcept
    {
        writeUInt(static_cast<uint32_t>(opcode));
    }

    void waitIfDataIsReachingLimit() noexcept
    {
        if (getAvailableDataSize() < HugeStackBuffer::size/4)
            return;

        for (int i=50; --i >= 0;)
        {
            if (getAvailableDataSize() >= HugeStackBuffer::size*3/4)
            {
                //carla_stdout("Client waitIfDataIsReachingLimit() reached and waited successfully");
                writeOpcode(kPluginBridgeNonRtServerPong);
                commitWrite();
                return;
            }
            carla_msleep(20);
        }

        carla_stderr("Client waitIfDataIsReachingLimit() reached and failed");
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
          fIsOffline(false),
          fFirstIdle(true),
          fLastPingTime(-1)
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

        clear();
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
            carla_stdout("Failed to attach to rt client control shared memory");
            return false;
        }

        if (! fShmRtClientControl.mapData())
        {
            clear();
            carla_stdout("Failed to map rt client control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.attach())
        {
            clear();
            carla_stdout("Failed to attach to non-rt client control shared memory");
            return false;
        }

        if (! fShmNonRtClientControl.mapData())
        {
            clear();
            carla_stdout("Failed to map non-rt control client shared memory");
            return false;
        }

        if (! fShmNonRtServerControl.attach())
        {
            clear();
            carla_stdout("Failed to attach to non-rt server control shared memory");
            return false;
        }

        if (! fShmNonRtServerControl.mapData())
        {
            clear();
            carla_stdout("Failed to map non-rt control server shared memory");
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

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetBufferSize, opcode);
        pData->bufferSize = fShmNonRtClientControl.readUInt();

        opcode = fShmNonRtClientControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeNonRtClientSetSampleRate, opcode);
        pData->sampleRate = fShmNonRtClientControl.readDouble();

        carla_stdout("Carla Client Info:");
        carla_stdout("  BufferSize: %i", pData->bufferSize);
        carla_stdout("  SampleRate: %g", pData->sampleRate);
        carla_stdout("  sizeof(BridgeRtClientData):    %i/" P_SIZE, shmRtClientDataSize,    sizeof(BridgeRtClientData));
        carla_stdout("  sizeof(BridgeNonRtClientData): %i/" P_SIZE, shmNonRtClientDataSize, sizeof(BridgeNonRtClientData));
        carla_stdout("  sizeof(BridgeNonRtServerData): %i/" P_SIZE, shmNonRtServerDataSize, sizeof(BridgeNonRtServerData));

        if (shmRtClientDataSize != sizeof(BridgeRtClientData) || shmNonRtClientDataSize != sizeof(BridgeNonRtClientData)  || shmNonRtServerDataSize != sizeof(BridgeNonRtServerData))
            return false;

        // tell backend we're live
        {
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPong);
            fShmNonRtServerControl.commitWrite();
        }

        startThread();

        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEnginePlugin::close()");
        fLastPingTime = -1;

        CarlaEngine::close();

        stopThread(5000);
        clear();

        return true;
    }

    bool isRunning() const noexcept override
    {
        return isThreadRunning() || ! fFirstIdle;
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
        CarlaPlugin* const plugin(pData->plugins[0].plugin);
        CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

        const bool wasFirstIdle(fFirstIdle);

        if (fFirstIdle)
        {
            fFirstIdle = false;
            fLastPingTime = Time::currentTimeMillis();
            CARLA_SAFE_ASSERT(fLastPingTime > 0);

            char bufStr[STR_MAX+1];
            uint32_t bufStrSize;

            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            // kPluginBridgeNonRtServerPluginInfo1
            {
                // uint/category, uint/hints, uint/optionsAvailable, uint/optionsEnabled, long/uniqueId
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPluginInfo1);
                fShmNonRtServerControl.writeUInt(plugin->getCategory());
                fShmNonRtServerControl.writeUInt(plugin->getHints());
                fShmNonRtServerControl.writeUInt(plugin->getOptionsAvailable());
                fShmNonRtServerControl.writeUInt(plugin->getOptionsEnabled());
                fShmNonRtServerControl.writeLong(plugin->getUniqueId());
                fShmNonRtServerControl.commitWrite();
            }

            // kPluginBridgeNonRtServerPluginInfo2
            {
                // uint/size, str[] (realName), uint/size, str[] (label), uint/size, str[] (maker), uint/size, str[] (copyright)
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerPluginInfo2);

                carla_zeroChars(bufStr, STR_MAX);
                plugin->getRealName(bufStr);
                bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                carla_zeroChars(bufStr, STR_MAX);
                plugin->getLabel(bufStr);
                bufStrSize = carla_fixedValue(1U, 256U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                carla_zeroChars(bufStr, STR_MAX);
                plugin->getMaker(bufStr);
                bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                carla_zeroChars(bufStr, STR_MAX);
                plugin->getCopyright(bufStr);
                bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                fShmNonRtServerControl.writeUInt(bufStrSize);
                fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                fShmNonRtServerControl.commitWrite();
            }

            // kPluginBridgeNonRtServerAudioCount
            {
                // uint/ins, uint/outs
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerAudioCount);
                fShmNonRtServerControl.writeUInt(plugin->getAudioInCount());
                fShmNonRtServerControl.writeUInt(plugin->getAudioOutCount());
                fShmNonRtServerControl.commitWrite();
            }

            // kPluginBridgeNonRtServerMidiCount
            {
                // uint/ins, uint/outs
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerMidiCount);
                fShmNonRtServerControl.writeUInt(plugin->getMidiInCount());
                fShmNonRtServerControl.writeUInt(plugin->getMidiOutCount());
                fShmNonRtServerControl.commitWrite();
            }

            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            // kPluginBridgeNonRtServerParameter*
            if (const uint32_t count = plugin->getParameterCount())
            {
                // uint/count
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterCount);
                fShmNonRtServerControl.writeUInt(count);
                fShmNonRtServerControl.commitWrite();

                for (uint32_t i=0; i<count; ++i)
                {
                    const ParameterData& paramData(plugin->getParameterData(i));

                    if (paramData.type != PARAMETER_INPUT && paramData.type != PARAMETER_OUTPUT)
                        continue;
                    if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
                        continue;

                    // kPluginBridgeNonRtServerParameterData1
                    {
                        // uint/index, int/rindex, uint/type, uint/hints, short/cc
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterData1);
                        fShmNonRtServerControl.writeUInt(i);
                        fShmNonRtServerControl.writeInt(paramData.rindex);
                        fShmNonRtServerControl.writeUInt(paramData.type);
                        fShmNonRtServerControl.writeUInt(paramData.hints);
                        fShmNonRtServerControl.writeShort(paramData.midiCC);
                        fShmNonRtServerControl.commitWrite();
                    }

                    // kPluginBridgeNonRtServerParameterData2
                    {
                        // uint/index, uint/size, str[] (name), uint/size, str[] (symbol), uint/size, str[] (unit)
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterData2);
                        fShmNonRtServerControl.writeUInt(i);

                        carla_zeroChars(bufStr, STR_MAX);
                        plugin->getParameterName(i, bufStr);
                        bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(bufStr)));
                        fShmNonRtServerControl.writeUInt(bufStrSize);
                        fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                        carla_zeroChars(bufStr, STR_MAX);
                        plugin->getParameterSymbol(i, bufStr);
                        bufStrSize = carla_fixedValue(1U, 64U, static_cast<uint32_t>(std::strlen(bufStr)));
                        fShmNonRtServerControl.writeUInt(bufStrSize);
                        fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                        carla_zeroChars(bufStr, STR_MAX);
                        plugin->getParameterUnit(i, bufStr);
                        bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(bufStr)));
                        fShmNonRtServerControl.writeUInt(bufStrSize);
                        fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                        fShmNonRtServerControl.commitWrite();
                    }

                    // kPluginBridgeNonRtServerParameterRanges
                    {
                        const ParameterRanges& paramRanges(plugin->getParameterRanges(i));

                        // uint/index, float/def, float/min, float/max, float/step, float/stepSmall, float/stepLarge
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterRanges);
                        fShmNonRtServerControl.writeUInt(i);
                        fShmNonRtServerControl.writeFloat(paramRanges.def);
                        fShmNonRtServerControl.writeFloat(paramRanges.min);
                        fShmNonRtServerControl.writeFloat(paramRanges.max);
                        fShmNonRtServerControl.writeFloat(paramRanges.step);
                        fShmNonRtServerControl.writeFloat(paramRanges.stepSmall);
                        fShmNonRtServerControl.writeFloat(paramRanges.stepLarge);
                        fShmNonRtServerControl.commitWrite();
                    }

                    // kPluginBridgeNonRtServerParameterValue2
                    {
                        // uint/index float/value (used for init/output parameters only, don't resend values)
                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterValue2);
                        fShmNonRtServerControl.writeUInt(i);
                        fShmNonRtServerControl.writeFloat(plugin->getParameterValue(i));
                        fShmNonRtServerControl.commitWrite();
                    }

                    fShmNonRtServerControl.waitIfDataIsReachingLimit();
                }
            }

            // kPluginBridgeNonRtServerProgram*
            if (const uint32_t count = plugin->getProgramCount())
            {
                // uint/count
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerProgramCount);
                fShmNonRtServerControl.writeUInt(count);
                fShmNonRtServerControl.commitWrite();

                for (uint32_t i=0; i < count; ++i)
                {
                    // uint/index, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerProgramName);
                    fShmNonRtServerControl.writeUInt(i);

                    carla_zeroChars(bufStr, STR_MAX);
                    plugin->getProgramName(i, bufStr);
                    bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(bufStr)));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(bufStr, bufStrSize);

                    fShmNonRtServerControl.commitWrite();
                    fShmNonRtServerControl.waitIfDataIsReachingLimit();
                }
            }

            // kPluginBridgeNonRtServerMidiProgram*
            if (const uint32_t count = plugin->getMidiProgramCount())
            {
                // uint/count
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerMidiProgramCount);
                fShmNonRtServerControl.writeUInt(count);
                fShmNonRtServerControl.commitWrite();

                for (uint32_t i=0; i < count; ++i)
                {
                    const MidiProgramData& mpData(plugin->getMidiProgramData(i));
                    CARLA_SAFE_ASSERT_CONTINUE(mpData.name != nullptr);

                    // uint/index, uint/bank, uint/program, uint/size, str[] (name)
                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerMidiProgramData);
                    fShmNonRtServerControl.writeUInt(i);
                    fShmNonRtServerControl.writeUInt(mpData.bank);
                    fShmNonRtServerControl.writeUInt(mpData.program);

                    bufStrSize = carla_fixedValue(1U, 32U, static_cast<uint32_t>(std::strlen(mpData.name)));
                    fShmNonRtServerControl.writeUInt(bufStrSize);
                    fShmNonRtServerControl.writeCustomData(mpData.name, bufStrSize);

                    fShmNonRtServerControl.commitWrite();
                    fShmNonRtServerControl.waitIfDataIsReachingLimit();
                }
            }

            // ready!
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerReady);
            fShmNonRtServerControl.commitWrite();
            fShmNonRtServerControl.waitIfDataIsReachingLimit();

            carla_stdout("Carla Client Ready!");
            fLastPingTime = Time::currentTimeMillis();
        }

        // send parameter outputs
        if (const uint32_t count = plugin->getParameterCount())
        {
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

            for (uint32_t i=0; i < count; ++i)
            {
                if (! plugin->isParameterOutput(i))
                    continue;

                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterValue2);
                fShmNonRtServerControl.writeUInt(i);
                fShmNonRtServerControl.writeFloat(plugin->getParameterValue(i));

                // parameter outputs are not that important, we can skip some
                if (! fShmNonRtServerControl.commitWrite())
                    break;
            }
        }

        CarlaEngine::idle();

        try {
            handleNonRtData();
        } CARLA_SAFE_EXCEPTION("handleNonRtData");

        if (fLastPingTime > 0 && Time::currentTimeMillis() > fLastPingTime + 30000 && ! wasFirstIdle)
        {
            carla_stderr("Did not receive ping message from server for 30 secs, closing...");
            callback(ENGINE_CALLBACK_QUIT, 0, 0, 0, 0.0f, nullptr);
        }
    }

    void callback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr) noexcept override
    {
        CarlaEngine::callback(action, pluginId, value1, value2, value3, valueStr);

        if (fLastPingTime < 0)
            return;

        switch (action)
        {
        // uint/index float/value
        case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= 0);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerParameterValue);
            fShmNonRtServerControl.writeUInt(static_cast<uint>(value1));
            fShmNonRtServerControl.writeFloat(value3);
            fShmNonRtServerControl.commitWrite();
        }   break;

        // uint/index float/value
        case ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= 0);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerDefaultValue);
            fShmNonRtServerControl.writeUInt(static_cast<uint>(value1));
            fShmNonRtServerControl.writeFloat(value3);
            fShmNonRtServerControl.commitWrite();
        }   break;

        // int/index
        case ENGINE_CALLBACK_PROGRAM_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= -1);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerCurrentProgram);
            fShmNonRtServerControl.writeInt(value1);
            fShmNonRtServerControl.commitWrite();
        }   break;

        // int/index
        case ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED: {
            CARLA_SAFE_ASSERT_BREAK(value1 >= -1);
            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerCurrentMidiProgram);
            fShmNonRtServerControl.writeInt(value1);
            fShmNonRtServerControl.commitWrite();
        }   break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            if (value1 != 1)
            {
                const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);
                fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerUiClosed);
                fShmNonRtServerControl.commitWrite();
            }
            break;

        default:
            break;
        }
    }

    // -------------------------------------------------------------------

    void clear() noexcept
    {
        fShmAudioPool.clear();
        fShmRtClientControl.clear();
        fShmNonRtClientControl.clear();
        fShmNonRtServerControl.clear();
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

                carla_stdout("Carla bridge client side, OnOff ping checks => %s", bool2str(onOff));
            }   break;

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
                carla_zeroChars(typeStr, typeSize+1);
                fShmNonRtClientControl.readCustomData(typeStr, typeSize);

                // key
                const uint32_t keySize(fShmNonRtClientControl.readUInt());
                char keyStr[keySize+1];
                carla_zeroChars(keyStr, keySize+1);
                fShmNonRtClientControl.readCustomData(keyStr, keySize);

                // value
                const uint32_t valueSize(fShmNonRtClientControl.readUInt());
                char valueStr[valueSize+1];
                carla_zeroChars(valueStr, valueSize+1);
                fShmNonRtClientControl.readCustomData(valueStr, valueSize);

                if (plugin != nullptr && plugin->isEnabled())
                    plugin->setCustomData(typeStr, keyStr, valueStr, true);
                break;
            }

            case kPluginBridgeNonRtClientSetChunkDataFile: {
                const uint32_t size(fShmNonRtClientControl.readUInt());
                CARLA_SAFE_ASSERT_BREAK(size > 0);

                char chunkFilePathTry[size+1];
                carla_zeroChars(chunkFilePathTry, size+1);
                fShmNonRtClientControl.readCustomData(chunkFilePathTry, size);

                CARLA_SAFE_ASSERT_BREAK(chunkFilePathTry[0] != '\0');
                if (plugin == nullptr || ! plugin->isEnabled()) break;

                carla_stdout("Carla bridge client side setChunkData 001");

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
                carla_stdout("Carla bridge client side setChunkData 002");

                plugin->setChunkData(chunk.data(), chunk.size());
                carla_stdout("Carla bridge client side setChunkData done");
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

                for (uint32_t i=0, count=plugin->getCustomDataCount(); i<count; ++i)
                {
                    const CustomData& cdata(plugin->getCustomData(i));

                    if (std::strcmp(cdata.type, CUSTOM_DATA_TYPE_STRING) == 0 && std::strcmp(cdata.key, "CarlaLoadLv2StateNow") == 0 && std::strcmp(cdata.value, "true") == 0)
                        continue;

                    const uint32_t typeLen(static_cast<uint32_t>(std::strlen(cdata.type)));
                    const uint32_t keyLen(static_cast<uint32_t>(std::strlen(cdata.key)));
                    const uint32_t valueLen(static_cast<uint32_t>(std::strlen(cdata.value)));

                    {
                        const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                        fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetCustomData);

                        fShmNonRtServerControl.writeUInt(typeLen);
                        fShmNonRtServerControl.writeCustomData(cdata.type, typeLen);

                        fShmNonRtServerControl.writeUInt(keyLen);
                        fShmNonRtServerControl.writeCustomData(cdata.key, keyLen);

                        fShmNonRtServerControl.writeUInt(valueLen);
                        fShmNonRtServerControl.writeCustomData(cdata.value, valueLen);

                        fShmNonRtServerControl.commitWrite();
                        fShmNonRtServerControl.waitIfDataIsReachingLimit();
                    }
                }

                if (plugin->getOptionsEnabled() & PLUGIN_OPTION_USE_CHUNKS)
                {
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
                        {
                            const uint32_t ulength(static_cast<uint32_t>(filePath.length()));

                            const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                            fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSetChunkDataFile);
                            fShmNonRtServerControl.writeUInt(ulength);
                            fShmNonRtServerControl.writeCustomData(filePath.toRawUTF8(), ulength);
                            fShmNonRtServerControl.commitWrite();
                        }
                    }
                }

                {
                    const CarlaMutexLocker _cml(fShmNonRtServerControl.mutex);

                    fShmNonRtServerControl.writeOpcode(kPluginBridgeNonRtServerSaved);
                    fShmNonRtServerControl.commitWrite();
                }
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
        bool timedOut = false;
        bool quitReceived = false;

        for (; ! shouldThreadExit();)
        {
            if (! fShmRtClientControl.waitForServer(5))
            {
                // Give engine 1 more change to catch up.
                if (! timedOut)
                {
                    carla_stderr2("Bridge timed-out, giving it one more chance");
                    timedOut = true;
                    continue;
                }

                carla_stderr2("Bridge timed-out, final post...");
                fShmRtClientControl.postClient();
                carla_stderr2("Bridge timed-out, done.");
                signalThreadShouldExit();
                break;
            }

            timedOut = false;

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
                }   break;

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
                }   break;

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

                        float* fdata = fShmAudioPool.data;

                        for (uint32_t i=0; i < audioInCount; ++i, fdata += pData->bufferSize)
                            audioIn[i] = fdata;
                        for (uint32_t i=0; i < audioOutCount; ++i, fdata += pData->bufferSize)
                            audioOut[i] = fdata;

                        for (uint32_t i=0; i < cvInCount; ++i, fdata += pData->bufferSize)
                            cvIn[i] = fdata;
                        for (uint32_t i=0; i < cvOutCount; ++i, fdata += pData->bufferSize)
                            cvOut[i] = fdata;

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

                    uint8_t* midiData(fShmRtClientControl.data->midiOut);
                    carla_zeroBytes(midiData, kBridgeRtClientDataMidiOutSize);
                    std::size_t curMidiDataPos = 0;

                    if (pData->events.in[0].type != kEngineEventTypeNull)
                        carla_zeroStructs(pData->events.in,  kMaxEngineEventInternalCount);

                    if (pData->events.out[0].type != kEngineEventTypeNull)
                    {
                        for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
                        {
                            const EngineEvent& event(pData->events.out[i]);

                            if (event.type == kEngineEventTypeNull)
                                break;

                            if (event.type == kEngineEventTypeControl)
                            {
                                uint8_t size;
                                uint8_t data[3];
                                event.ctrl.convertToMidiData(event.channel, size, data);
                                CARLA_SAFE_ASSERT_CONTINUE(size > 0 && size <= 3);

                                if (curMidiDataPos + 1U /* size*/ + 4U /* time */ + size >= kBridgeRtClientDataMidiOutSize)
                                    break;

                                // set size
                                *midiData++ = size;

                                // set time
                                *(uint32_t*)midiData = event.time;
                                midiData = midiData + 4;

                                // set data
                                for (uint8_t j=0; j<size; ++j)
                                    *midiData++ = data[j];

                                curMidiDataPos += 1U /* size*/ + 4U /* time */ + size;
                            }
                            else if (event.type == kEngineEventTypeMidi)
                            {
                                const EngineMidiEvent& _midiEvent(event.midi);

                                if (curMidiDataPos + 1 /* size*/ + 4 /* time */ + _midiEvent.size >= kBridgeRtClientDataMidiOutSize)
                                    break;

                                const uint8_t* const _midiData(_midiEvent.dataExt != nullptr ? _midiEvent.dataExt : _midiEvent.data);

                                // set size
                                *midiData++ = _midiEvent.size;

                                // set time
                                *(uint32_t*)midiData = event.time;
                                midiData = midiData + 4;

                                // set data
                                *midiData++ = uint8_t(_midiData[0] | (event.channel & MIDI_CHANNEL_BIT));

                                for (uint8_t j=1; j<_midiEvent.size; ++j)
                                    *midiData++ = _midiData[j];

                                curMidiDataPos += 1U /* size*/ + 4U /* time */ + _midiEvent.size;
                            }
                        }

                        carla_zeroStructs(pData->events.out, kMaxEngineEventInternalCount);
                    }

                }   break;

                case kPluginBridgeRtClientQuit: {
                    quitReceived = true;
                    signalThreadShouldExit();
                }   break;
                }
            }

            fShmRtClientControl.postClient();
        }

        callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);

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
    }

    // called from process thread above
    EngineEvent* getNextFreeInputEvent() const noexcept
    {
        for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
        {
            EngineEvent* const event(&pData->events.in[i]);

            if (event->type == kEngineEventTypeNull)
                return event;
        }
        return nullptr;
    }

    // -------------------------------------------------------------------

private:
    BridgeAudioPool          fShmAudioPool;
    BridgeRtClientControl    fShmRtClientControl;
    BridgeNonRtClientControl fShmNonRtClientControl;
    BridgeNonRtServerControl fShmNonRtServerControl;

    bool fIsOffline;
    bool fFirstIdle;
    int64_t fLastPingTime;

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
