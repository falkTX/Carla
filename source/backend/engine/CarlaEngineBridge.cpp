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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifdef BUILD_BRIDGE

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

class CarlaEngineBridge : public CarlaEngine,
                          public QThread
{
public:
    CarlaEngineBridge(const char* const audioBaseName, const char* const controlBaseName)
        : CarlaEngine(),
          fIsRunning(false),
          fQuitNow(false)
    {
        carla_debug("CarlaEngineBridge::CarlaEngineBridge()");

        fShmAudioPool.filename  = "/carla-bridge_shm_";
        fShmAudioPool.filename += audioBaseName;

        fShmControl.filename    = "/carla-bridge_shc_";
        fShmControl.filename   += controlBaseName;
    }

    ~CarlaEngineBridge()
    {
        carla_debug("CarlaEngineBridge::~CarlaEngineBridge()");
    }

    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName)
    {
        carla_debug("CarlaEngineBridge::init(\"%s\")", clientName);

        // SHM Audio Pool
        {
#ifdef CARLA_OS_WIN
            // TESTING!
            fShmAudioPool.shm = carla_shm_attach_linux((const char*)fShmAudioPool.filename);
#else
            fShmAudioPool.shm = carla_shm_attach((const char*)fShmAudioPool.filename);
#endif

            if (! carla_is_shm_valid(fShmAudioPool.shm))
            {
                _cleanup();
                carla_stdout("Failed to open or create shared memory file #1");
                return false;
            }
        }

        // SHM Control
        {
#ifdef CARLA_OS_WIN
            // TESTING!
            fShmControl.shm = carla_shm_attach_linux((const char*)fShmControl.filename);
#else
            fShmControl.shm = carla_shm_attach((const char*)fShmControl.filename);
#endif

            if (! carla_is_shm_valid(fShmControl.shm))
            {
                _cleanup();
                carla_stdout("Failed to open or create shared memory file #2");
                return false;
            }

            if (! carla_shm_map<BridgeShmControl>(fShmControl.shm, fShmControl.data))
            {
                _cleanup();
                carla_stdout("Failed to mmap shared memory file");
                return false;
            }
        }

        // Read values from memory
        PluginBridgeOpcode opcode;

        opcode = rdwr_readOpcode(&fShmControl.data->ringBuffer);
        CARLA_ASSERT(opcode == kPluginBridgeOpcodeSetBufferSize);
        fBufferSize = rdwr_readInt(&fShmControl.data->ringBuffer);
        carla_stderr("BufferSize: %i", fBufferSize);

        opcode = rdwr_readOpcode(&fShmControl.data->ringBuffer);
        CARLA_ASSERT(opcode == kPluginBridgeOpcodeSetSampleRate);
        fSampleRate = rdwr_readFloat(&fShmControl.data->ringBuffer);
        carla_stderr("SampleRate: %f", fSampleRate);

        fQuitNow = false;
        fIsRunning = true;

        QThread::start(QThread::TimeCriticalPriority);
        CarlaEngine::init(clientName);
        return true;
    }

    bool close()
    {
        carla_debug("CarlaEnginePlugin::close()");
        CarlaEngine::close();

        fQuitNow = true;
        QThread::wait();

        _cleanup();

        return true;
    }

    bool isRunning() const
    {
        return fIsRunning;
    }

    bool isOffline() const
    {
        return false;
    }

    EngineType type() const
    {
        return kEngineTypeBridge;
    }

    // -------------------------------------
    // CarlaThread virtual calls

    void run()
    {
        // TODO - set RT permissions

        while (! fQuitNow)
        {
            if (! jackbridge_sem_timedwait(&fShmControl.data->runServer, 5))
            {
                if (errno != ETIMEDOUT)
                    continue;

                fQuitNow = true;
                return;
            }

            while (rdwr_dataAvailable(&fShmControl.data->ringBuffer))
            {
                const PluginBridgeOpcode opcode(rdwr_readOpcode(&fShmControl.data->ringBuffer));

                switch (opcode)
                {
                case kPluginBridgeOpcodeNull:
                    break;

                case kPluginBridgeOpcodeSetAudioPool:
                {
                    const int poolSize(rdwr_readInt(&fShmControl.data->ringBuffer));
                    fShmAudioPool.data = (float*)carla_shm_map(fShmAudioPool.shm, poolSize);
                    break;
                }

                case kPluginBridgeOpcodeSetBufferSize:
                {
                    const int bufferSize(rdwr_readInt(&fShmControl.data->ringBuffer));
                    bufferSizeChanged(bufferSize);
                    break;
                }

                case kPluginBridgeOpcodeSetSampleRate:
                {
                    const float sampleRate(rdwr_readFloat(&fShmControl.data->ringBuffer));
                    sampleRateChanged(sampleRate);
                    break;
                }

                case kPluginBridgeOpcodeSetParameter:
                {
                    const int   index(rdwr_readInt(&fShmControl.data->ringBuffer));
                    const float value(rdwr_readFloat(&fShmControl.data->ringBuffer));

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->enabled())
                    {
                        plugin->setParameterValueByRealIndex(index, value, false, false, false);
                        plugin->postponeRtEvent(kPluginPostRtEventParameterChange, index, 0, value);
                    }

                    break;
                }

                case kPluginBridgeOpcodeSetProgram:
                {
                    const int index(rdwr_readInt(&fShmControl.data->ringBuffer));

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->enabled())
                    {
                        plugin->setProgram(index, false, false, false);
                        plugin->postponeRtEvent(kPluginPostRtEventProgramChange, index, 0, 0.0f);
                    }

                    break;
                }

                case kPluginBridgeOpcodeSetMidiProgram:
                {
                    const int index(rdwr_readInt(&fShmControl.data->ringBuffer));

                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->enabled())
                    {
                        plugin->setMidiProgram(index, false, false, false);
                        plugin->postponeRtEvent(kPluginPostRtEventMidiProgramChange, index, 0, 0.0f);
                    }

                    break;
                }

                case kPluginBridgeOpcodeMidiEvent:
                {
                    uint8_t data[4] = { 0 };
                    const long time(rdwr_readLong(&fShmControl.data->ringBuffer));
                    const int  dataSize(rdwr_readInt(&fShmControl.data->ringBuffer));

                    CARLA_ASSERT_INT(dataSize >= 1 && dataSize <= 4, dataSize);

                    for (int i=0; i < dataSize && i < 4; ++i)
                        data[i] = rdwr_readChar(&fShmControl.data->ringBuffer);

                    CARLA_ASSERT(kData->bufEvents.in != nullptr);

                    if (kData->bufEvents.in != nullptr)
                    {
                        // TODO
                    }

                    break;
                }

                case kPluginBridgeOpcodeProcess:
                {
                    CARLA_ASSERT(fShmAudioPool.data != nullptr);
                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->enabled() && plugin->tryLock())
                    {
                        const uint32_t inCount(plugin->audioInCount());
                        const uint32_t outCount(plugin->audioOutCount());

                        float* inBuffer[inCount];
                        float* outBuffer[outCount];

                        for (uint32_t i=0; i < inCount; ++i)
                            inBuffer[i] = fShmAudioPool.data + i*fBufferSize;
                        for (uint32_t i=0; i < outCount; ++i)
                            outBuffer[i] = fShmAudioPool.data + (i+inCount)*fBufferSize;

                        plugin->initBuffers();
                        plugin->process(inBuffer, outBuffer, fBufferSize);
                        plugin->unlock();
                    }
                    break;
                }

                case kPluginBridgeOpcodeQuit:
                    fQuitNow = true;
                    break;
                }
            }

            if (jackbridge_sem_post(&fShmControl.data->runClient) != 0)
                pass(); //carla_stderr2("Could not post to semaphore");
        }

        fIsRunning = false;
    }

private:
    struct BridgeAudioPool {
        CarlaString filename;
        float* data;
        shm_t shm;

        BridgeAudioPool()
            : data(nullptr)
        {
            carla_shm_init(shm);
        }
    } fShmAudioPool;

    struct BridgeControl {
        CarlaString filename;
        BridgeShmControl* data;
        shm_t shm;

        BridgeControl()
            : data(nullptr)
        {
            carla_shm_init(shm);
        }

    } fShmControl;

    bool fIsRunning;
    bool fQuitNow;

    void _cleanup()
    {
        if (fShmAudioPool.filename.isNotEmpty())
            fShmAudioPool.filename.clear();

        if (fShmControl.filename.isNotEmpty())
            fShmControl.filename.clear();

        fShmAudioPool.data = nullptr;
        fShmControl.data = nullptr;

        if (carla_is_shm_valid(fShmAudioPool.shm))
            carla_shm_close(fShmAudioPool.shm);

        if (carla_is_shm_valid(fShmControl.shm))
            carla_shm_close(fShmControl.shm);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineBridge)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newBridge(const char* const audioBaseName, const char* const controlBaseName)
{
    return new CarlaEngineBridge(audioBaseName, controlBaseName);
}

CARLA_BACKEND_END_NAMESPACE

#endif // BUILD_BRIDGE
