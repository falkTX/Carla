/*
 * Carla Plugin Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBridge.hpp"
#include "CarlaShmUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------

class CarlaEngineBridge : public CarlaEngine
{
public:
    CarlaEngineBridge(const char* const audioBaseName, const char* const controlBaseName)
        : CarlaEngine()
    {
        carla_debug("CarlaEngineBridge::CarlaEngineBridge()");

        fShmAudioPool.filename = "/carla-bridge_shm_" + audioBaseName;
        fShmControl.filename   = "/carla-bridge_shc_" + controlBaseName;
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

        char tmpFileBase[60];

        // SHM Audio Pool
        {
            fShmAudioPool.shm = carla_shm_attach((const char*)fShmAudioPool.filename);

            if (! carla_is_shm_valid(fShmAudioPool.shm))
            {
                _cleanup();
                carla_stdout("Failed to open or create shared memory file #1");
                return false;
            }
        }

        // SHM Control
        {
            fShmControl.shm = carla_shm_attach((const char*)fShmControl.filename);

            if (! carla_is_shm_valid(fShmControl.shm))
            {
                _cleanup();
                carla_stdout("Failed to open or create shared memory file #2");
                return false;
            }

            if (! carla_shm_map<ShmControl>(fShmControl.shm, fShmControl.data))
            {
                _cleanup();
                carla_stdout("Failed to mmap shared memory file");
                return false;
            }
        }

        CarlaEngine::init(fName);
        return true;
    }

    bool close()
    {
        carla_debug("CarlaEnginePlugin::close()");
        CarlaEngine::close();

        _cleanup();

        return true;
    }

    bool isRunning() const
    {
        return true;
    }

    bool isOffline() const
    {
        return false;
    }

    EngineType type() const
    {
        return kEngineTypeBridge;
    }

private:
    struct BridgeAudioPool {
        CarlaString filename;
        float* data;
        size_t size;
        shm_t shm;

        BridgeAudioPool()
            : data(nullptr),
              size(0)
        {
            carla_shm_init(shm);
        }
    } fShmAudioPool;

    struct BridgeControl {
        CarlaString filename;
        ShmControl* data;
        shm_t shm;

        BridgeControl()
            : data(nullptr)
        {
            carla_shm_init(shm);
        }

    } fShmControl;

    void _cleanup()
    {
        if (fShmAudioPool.filename.isNotEmpty())
            fShmAudioPool.filename.clear();

        if (fShmControl.filename.isNotEmpty())
            fShmControl.filename.clear();

        // delete data
        fShmAudioPool.data = nullptr;
        fShmAudioPool.size = 0;

        // and again
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
