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

#include "../CarlaBridge.hpp"
#include "CarlaShmUtils.hpp"

#include "jackbridge/jackbridge.h"

#include <ctime>

#ifdef CARLA_OS_WIN
# include <sys/time.h>
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------

class CarlaEngineBridge : public CarlaEngine,
                          public CarlaThread
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
        CARLA_ASSERT(rdwr_readOpcode(&fShmControl.data->ringBuffer) == kPluginBridgeOpcodeBufferSize);
        fBufferSize = rdwr_readInt(&fShmControl.data->ringBuffer);
        carla_stderr("BufferSize: %i", fBufferSize);

        CARLA_ASSERT(rdwr_readOpcode(&fShmControl.data->ringBuffer) == kPluginBridgeOpcodeSampleRate);
        fSampleRate = rdwr_readFloat(&fShmControl.data->ringBuffer);
        carla_stderr("SampleRate: %f", fSampleRate);

        fQuitNow = false;
        fIsRunning = true;

        CarlaThread::start();
        CarlaEngine::init(clientName);
        return true;
    }

    bool close()
    {
        carla_debug("CarlaEnginePlugin::close()");
        CarlaEngine::close();

        fQuitNow = true;
        CarlaThread::stop();

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

        const int timeout = 5000;

        while (! fQuitNow)
        {
            timespec ts_timeout;
#ifdef CARLA_OS_WIN
            timeval now;
            gettimeofday(&now, nullptr);
            ts_timeout.tv_sec = now.tv_sec;
            ts_timeout.tv_nsec = now.tv_usec * 1000;
#else
            clock_gettime(CLOCK_REALTIME, &ts_timeout);
#endif

            time_t seconds = timeout / 1000;
            ts_timeout.tv_sec += seconds;
            ts_timeout.tv_nsec += (timeout - seconds * 1000) * 1000000;
            if (ts_timeout.tv_nsec >= 1000000000) {
                ts_timeout.tv_nsec -= 1000000000;
                ts_timeout.tv_sec++;
            }

            if (linux_sem_timedwait(&fShmControl.data->runServer, &ts_timeout))
            {
                if (errno == ETIMEDOUT)
                {
                    continue;
                }
                else
                {
                    fQuitNow = true;
                    break;
                }
            }

            while (rdwr_dataAvailable(&fShmControl.data->ringBuffer))
            {
                const PluginBridgeOpcode opcode = rdwr_readOpcode(&fShmControl.data->ringBuffer);

                switch (opcode)
                {
                case kPluginBridgeOpcodeNull:
                    break;
                case kPluginBridgeOpcodeReadyWait:
                    fShmAudioPool.data = (float*)carla_shm_map(fShmAudioPool.shm, rdwr_readInt(&fShmControl.data->ringBuffer));
                    break;
                case kPluginBridgeOpcodeBufferSize:
                    bufferSizeChanged(rdwr_readInt(&fShmControl.data->ringBuffer));
                    break;
                case kPluginBridgeOpcodeSampleRate:
                    sampleRateChanged(rdwr_readFloat(&fShmControl.data->ringBuffer));
                    break;
                case kPluginBridgeOpcodeProcess:
                {
                    CARLA_ASSERT(fShmAudioPool.data != nullptr);
                    CarlaPlugin* const plugin(getPluginUnchecked(0));

                    if (plugin != nullptr && plugin->enabled() && plugin->tryLock())
                    {
                        const uint32_t inCount  = plugin->audioInCount();
                        const uint32_t outCount = plugin->audioOutCount();

                        float* inBuffer[inCount];
                        float* outBuffer[outCount];

                        for (uint32_t i=0; i < inCount; i++)
                            inBuffer[i] = fShmAudioPool.data + i*fBufferSize;
                        for (uint32_t i=0; i < outCount; i++)
                            outBuffer[i] = fShmAudioPool.data + (i+inCount)*fBufferSize;

                        plugin->initBuffers();
                        plugin->setActive(true, false, false);
                        plugin->process(inBuffer, outBuffer, fBufferSize);
                        plugin->unlock();
                    }
                    break;
                }
                }
            }

            if (linux_sem_post(&fShmControl.data->runClient) != 0)
                carla_stderr2("Could not post to semaphore");
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
