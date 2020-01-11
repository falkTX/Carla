/*
 * Carla Tests
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "../backend/engine/CarlaEngineInternal.hpp"

#include "CarlaPlugin.hpp"

#include "CarlaHost.h"
#include "CarlaUtils.hpp"

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

class CarlaEngineDummy : public CarlaEngine
{
public:
    CarlaEngineDummy()
        : CarlaEngine(),
          fIsRunning(false)
    {
    }

    bool init(const char* const clientName) override
    {
        fIsRunning = true;
        pData->bufferSize = 512;
        pData->sampleRate = 44100.0;
        return CarlaEngine::init(clientName);
    }

    bool close() override
    {
        fIsRunning = false;
        return CarlaEngine::close();
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
        return kEngineTypePlugin;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "Dummy";
    }

private:
    bool fIsRunning;
};

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------

CARLA_BACKEND_USE_NAMESPACE

#define TEST_NAME "TestName"

void testEngine(CarlaEngine* const eng)
{
    assert(eng->getMaxClientNameSize() != 0);
    assert(eng->getMaxPortNameSize() != 0);
    assert(eng->getCurrentPluginCount() == 0);
    assert(eng->getMaxPluginNumber() == 0);
    assert(! eng->isRunning());
    assert(eng->getPlugin(0) == nullptr);

    const char* name1 = eng->getUniquePluginName(TEST_NAME);
    const char* name2 = eng->getUniquePluginName(TEST_NAME);
    const char* name3 = eng->getUniquePluginName(TEST_NAME);

    assert(name1 != nullptr);
    assert(name2 != nullptr);
    assert(name3 != nullptr);
    assert(std::strcmp(name1, TEST_NAME) == 0);
    assert(std::strcmp(name2, TEST_NAME) == 0);
    assert(std::strcmp(name3, TEST_NAME) == 0);
    delete[] name1;
    delete[] name2;
    delete[] name3;

    eng->init("test");

    assert(eng->getCurrentPluginCount() == 0);
    assert(eng->getMaxPluginNumber() != 0);
    assert(eng->isRunning());
    assert(eng->getPlugin(0) == nullptr);

    name1 = eng->getUniquePluginName(TEST_NAME);
    name2 = eng->getUniquePluginName(TEST_NAME);
    name3 = eng->getUniquePluginName(TEST_NAME);

    assert(name1 != nullptr);
    assert(name2 != nullptr);
    assert(name3 != nullptr);
    assert(std::strcmp(name1, TEST_NAME) == 0);
    assert(std::strcmp(name2, TEST_NAME) == 0);
    assert(std::strcmp(name3, TEST_NAME) == 0);
    delete[] name1;
    delete[] name2;
    delete[] name3;

    eng->close();

    // test quick init & close 3 times
    eng->init("test1");
    eng->close();
    eng->init("test2");
    eng->close();
    eng->init("test3");
    eng->close();

    // leave it open
    eng->init("test");

    // add as much plugins as possible
    for (;;)
    {
        if (! eng->addPlugin(PLUGIN_INTERNAL, nullptr, TEST_NAME, "bypass"))
            break;
    }
    assert(eng->getCurrentPluginCount() != 0);
    assert(eng->getCurrentPluginCount() == eng->getMaxPluginNumber());
    assert(eng->getCurrentPluginCount() == MAX_DEFAULT_PLUGINS);

    eng->close();
}

int main()
{
    assert(CarlaEngine::getDriverCount() != 0);

#if 0
    for (uint i=0, count=CarlaEngine::getDriverCount(); i < count && i < 4; ++i)
    {
        if (i == 1 || i == 2) continue;
        carla_stdout("driver %i/%i: %s", i+1, count, CarlaEngine::getDriverName(i));

        if (const char* const* const devNames = CarlaEngine::getDriverDeviceNames(i))
        {
            for (uint j=0; devNames[j] != nullptr; ++j)
            {
                CarlaEngine::getDriverDeviceInfo(i, devNames[j]);
            }
        }
    }
#endif

    assert(CarlaPlugin::getNativePluginCount() != 0);

    for (size_t i=0, count=CarlaPlugin::getNativePluginCount(); i < count; ++i)
    {
        assert(CarlaPlugin::getNativePluginDescriptor(i) != nullptr);
    }

    CarlaEngineDummy e;
    testEngine(&e);

//     if (CarlaEngine* const eng = CarlaEngine::newDriverByName("PulseAudio"))
//     {
//         testEngine(eng);
//         delete eng;
//     }

    return 0;
}
