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

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

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
};

struct BridgeControl : public RingBufferControl<StackRingBuffer> {
    CarlaString filename;
    BridgeShmControl* data;
    char shm[32];

    BridgeControl()
        : RingBufferControl(nullptr),
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
            setRingBuffer(&data->ringBuffer, false);
            return true;
        }

        return false;
    }

    PluginBridgeOpcode readOpcode()
    {
        return static_cast<PluginBridgeOpcode>(readInt());
    }
};

// -------------------------------------------------------------------

class CarlaEngineBridge : public CarlaEngine,
                          public CarlaThread
{
public:
    CarlaEngineBridge(const char* const audioBaseName, const char* const controlBaseName)
        : CarlaEngine(),
          CarlaThread("CarlaEngineBridge"),
          fIsRunning(false)
    {
        carla_debug("CarlaEngineBridge::CarlaEngineBridge()");

        fShmAudioPool.filename  = "/carla-bridge_shm_";
        fShmAudioPool.filename += audioBaseName;

        fShmControl.filename    = "/carla-bridge_shc_";
        fShmControl.filename   += controlBaseName;
    }

    ~CarlaEngineBridge() override
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
                carla_stdout("Failed to mmap shared memory file");
                // clear
                fShmControl.clear();
                fShmAudioPool.clear();
                return false;
            }
        }

        // Read values from memory
        PluginBridgeOpcode opcode;

        opcode = fShmControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeOpcodeNull, opcode);
        const uint32_t structSize = fShmControl.readUInt();
        CARLA_SAFE_ASSERT_INT2(structSize == sizeof(BridgeShmControl), structSize, sizeof(BridgeShmControl));
        carla_stderr("Struct Size: %i", structSize);

        opcode = fShmControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeOpcodeSetBufferSize, opcode);
        pData->bufferSize = fShmControl.readUInt();
        carla_stderr("BufferSize: %i", pData->bufferSize);

        opcode = fShmControl.readOpcode();
        CARLA_SAFE_ASSERT_INT(opcode == kPluginBridgeOpcodeSetSampleRate, opcode);
        pData->sampleRate = fShmControl.readFloat();
        carla_stderr("SampleRate: %f", pData->sampleRate);

        CarlaThread::start();
        CarlaEngine::init(clientName);
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEnginePlugin::close()");
        CarlaEngine::close();

        CarlaThread::stop(6000);

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

    // -------------------------------------
    // CarlaThread virtual calls

    void run() override
    {
        fIsRunning = true;

        // TODO - set RT permissions
        carla_debug("CarlaEngineBridge::run()");

        for (; ! shouldExit();)
        {
            if (! jackbridge_sem_timedwait(&fShmControl.data->runServer, 5))
            {
                if (errno == ETIMEDOUT)
                {
                    fIsRunning = false;
                    signalShouldExit();
                    return;
                }
            }

            for (; fShmControl.isDataAvailable();)
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
                    fShmAudioPool.data = (float*)jackbridge_shm_map(fShmAudioPool.shm, size_t(poolSize));
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

                case kPluginBridgeOpcodeSetParameter: {
                    const int32_t index(fShmControl.readInt());
                    const float   value(fShmControl.readFloat());

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->isEnabled())
                    {
                        plugin->setParameterValueByRealIndex(index, value, false, false, false);

                        //if (index >= 0)
                        //    plugin->postponeRtEvent(kPluginPostRtEventParameterChange, index, 0, value);
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

                case kPluginBridgeOpcodeMidiEvent: {
                    const int64_t time(fShmControl.readLong());
                    const int32_t size(fShmControl.readInt());
                    CARLA_SAFE_ASSERT_BREAK(time >= 0);
                    CARLA_SAFE_ASSERT_BREAK(size > 0 && size <= 4);

                    uint8_t data[size];

                    for (int32_t i=0; i < size; ++i)
                        data[i] = static_cast<uint8_t>(fShmControl.readChar());

                    CARLA_SAFE_ASSERT_BREAK(pData->bufEvents.in != nullptr);

                    for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
                    {
                        EngineEvent& event(pData->bufEvents.in[i]);

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

                        plugin->initBuffers();
                        plugin->process(inBuffer, outBuffer, pData->bufferSize);
                        plugin->unlock();
                    }

                    // clear buffer
                    CARLA_SAFE_ASSERT_BREAK(pData->bufEvents.in != nullptr);

                    if (pData->bufEvents.in[0].type != kEngineEventTypeNull)
                        carla_zeroStruct<EngineEvent>(pData->bufEvents.in, kMaxEngineEventInternalCount);
                    break;
                }

                case kPluginBridgeOpcodeQuit:
                    signalShouldExit();
                    fIsRunning = false;
                    break;
                }
            }

            if (! jackbridge_sem_post(&fShmControl.data->runClient))
                carla_stderr2("Could not post to semaphore");
        }

        fIsRunning = false;
    }

private:
    BridgeAudioPool fShmAudioPool;
    BridgeControl   fShmControl;

    volatile bool fIsRunning;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineBridge)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newBridge(const char* const audioBaseName, const char* const controlBaseName)
{
    return new CarlaEngineBridge(audioBaseName, controlBaseName);
}

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------
// Extra stuff for linking purposes

CARLA_BACKEND_START_NAMESPACE

CarlaEngine*       CarlaEngine::newRtAudio(const AudioApi) { return nullptr; }
unsigned int       CarlaEngine::getRtAudioApiCount() { return 0; }
const char*        CarlaEngine::getRtAudioApiName(const unsigned int) { return nullptr; }
const char* const* CarlaEngine::getRtAudioApiDeviceNames(const unsigned int) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getRtAudioDeviceInfo(const unsigned int, const char* const) { return nullptr; }

#ifdef HAVE_JUCE
CarlaEngine*       CarlaEngine::newJuce(const AudioApi) { return nullptr; }
unsigned int       CarlaEngine::getJuceApiCount() { return 0; }
const char*        CarlaEngine::getJuceApiName(const unsigned int) { return nullptr; }
const char* const* CarlaEngine::getJuceApiDeviceNames(const unsigned int) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getJuceDeviceInfo(const unsigned int, const char* const) { return nullptr; }
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
