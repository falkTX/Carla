/*
 * Carla Engine Bridge
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"

#include "CarlaBridgeUtils.hpp"
#include "CarlaShmUtils.hpp"

#include "jackbridge/JackBridge.hpp"

#include <cerrno>
#include <ctime>

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------

struct BridgeAudioPool {
    CarlaString filename;
    float* data;
    shm_t shm;

    BridgeAudioPool()
        : data(nullptr)
    {
        carla_shm_init(shm);
    }

    ~BridgeAudioPool()
    {
        // should be cleared by now
        CARLA_ASSERT(data == nullptr);

        clear();
    }

    bool attach()
    {
#ifdef CARLA_OS_WIN
        // TESTING!
        shm = carla_shm_attach_linux((const char*)filename);
#else
        shm = carla_shm_attach((const char*)filename);
#endif

        return carla_is_shm_valid(shm);
    }

    void clear()
    {
        filename.clear();

        data = nullptr;

        if (carla_is_shm_valid(shm))
            carla_shm_close(shm);
    }
};

struct BridgeControl : public RingBufferControl {
    CarlaString filename;
    BridgeShmControl* data;
    shm_t shm;

    BridgeControl()
        : RingBufferControl(nullptr),
          data(nullptr)
    {
        carla_shm_init(shm);
    }

    ~BridgeControl()
    {
        // should be cleared by now
        CARLA_ASSERT(data == nullptr);

        clear();
    }

    bool attach()
    {
#ifdef CARLA_OS_WIN
        // TESTING!
        shm = carla_shm_attach_linux((const char*)filename);
#else
        shm = carla_shm_attach((const char*)filename);
#endif

        return carla_is_shm_valid(shm);
    }

    void clear()
    {
        filename.clear();

        data = nullptr;

        if (carla_is_shm_valid(shm))
            carla_shm_close(shm);
    }

    bool mapData()
    {
        CARLA_ASSERT(data == nullptr);

        if (carla_shm_map<BridgeShmControl>(shm, data))
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
        CARLA_ASSERT_INT(opcode == kPluginBridgeOpcodeSetBufferSize, opcode);
        pData->bufferSize = fShmControl.readInt();
        carla_stderr("BufferSize: %i", pData->bufferSize);

        opcode = fShmControl.readOpcode();
        CARLA_ASSERT_INT(opcode == kPluginBridgeOpcodeSetSampleRate, opcode);
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

        while (! shouldExit())
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

            while (fShmControl.isDataAvailable())
            {
                const PluginBridgeOpcode opcode(fShmControl.readOpcode());

                if (opcode != kPluginBridgeOpcodeProcess)
                    carla_debug("CarlaEngineBridge::run() - got opcode: %s", PluginBridgeOpcode2str(opcode));

                switch (opcode)
                {
                case kPluginBridgeOpcodeNull:
                    break;

                case kPluginBridgeOpcodeSetAudioPool:
                {
                    const long poolSize(fShmControl.readLong());
                    fShmAudioPool.data = (float*)carla_shm_map(fShmAudioPool.shm, poolSize);
                    break;
                }

                case kPluginBridgeOpcodeSetBufferSize:
                {
                    const int bufferSize(fShmControl.readInt());
                    bufferSizeChanged(bufferSize);
                    break;
                }

                case kPluginBridgeOpcodeSetSampleRate:
                {
                    const float sampleRate(fShmControl.readFloat());
                    sampleRateChanged(sampleRate);
                    break;
                }

                case kPluginBridgeOpcodeSetParameter:
                {
                    //const int   index(fShmControl.readInt());
                    //const float value(fShmControl.readFloat());

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->isEnabled())
                    {
                        //plugin->setParameterValueByRealIndex(index, value, false, false, false);
                        //plugin->postponeRtEvent(kPluginPostRtEventParameterChange, index, 0, value);
                    }

                    break;
                }

                case kPluginBridgeOpcodeSetProgram:
                {
                    //const int index(fShmControl.readInt());

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->isEnabled())
                    {
                        //plugin->setProgram(index, false, false, false);
                        //plugin->postponeRtEvent(kPluginPostRtEventProgramChange, index, 0, 0.0f);
                    }

                    break;
                }

                case kPluginBridgeOpcodeSetMidiProgram:
                {
                    //const int index(fShmControl.readInt());

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->isEnabled())
                    {
                        //plugin->setMidiProgram(index, false, false, false);
                        //plugin->postponeRtEvent(kPluginPostRtEventMidiProgramChange, index, 0, 0.0f);
                    }

                    break;
                }

                case kPluginBridgeOpcodeMidiEvent:
                {
                    //uint8_t data[4] = { 0 };
                    //const long time(fShmControl.readLong());
                    //const int  dataSize(fShmControl.readInt());

                    //CARLA_ASSERT_INT(dataSize >= 1 && dataSize <= 4, dataSize);

                    //for (int i=0; i < dataSize && i < 4; ++i)
                    //    data[i] = fShmControl.readChar();

                    //CARLA_ASSERT(pData->bufEvents.in != nullptr);

                    //if (pData->bufEvents.in != nullptr)
                    {
                        // TODO
                    }

                    break;
                }

                case kPluginBridgeOpcodeProcess:
                {
                    CARLA_ASSERT(fShmAudioPool.data != nullptr);
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
                    break;
                }

                case kPluginBridgeOpcodeQuit:
                    signalShouldExit();
                    break;
                }
            }

            if (jackbridge_sem_post(&fShmControl.data->runClient) != 0)
                pass(); //carla_stderr2("Could not post to semaphore");
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
