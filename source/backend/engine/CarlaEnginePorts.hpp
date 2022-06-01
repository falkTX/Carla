/*
 * Carla Plugin Host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_ENGINE_PORTS_HPP_INCLUDED
#define CARLA_ENGINE_PORTS_HPP_INCLUDED

#include "CarlaEngine.hpp"
#include "CarlaEngineInternal.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Carla Engine Meta CV Port Protected Data

struct CarlaEngineEventCV {
    CarlaEngineCVPort* cvPort;
    uint32_t indexOffset;
    float previousValue;
};

struct CarlaEngineCVSourcePorts::ProtectedData {
    CarlaRecursiveMutex rmutex;
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    PatchbayGraph* graph;
    CarlaPluginPtr plugin;
#endif
    water::Array<CarlaEngineEventCV> cvs;

    ProtectedData() noexcept
        : rmutex(),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
          graph(nullptr),
          plugin(nullptr),
#endif
          cvs() {}

    ~ProtectedData()
    {
        CARLA_SAFE_ASSERT(cvs.size() == 0);
    }

    void cleanup()
    {
        const CarlaRecursiveMutexLocker crml(rmutex);

        for (int i = cvs.size(); --i >= 0;)
            delete cvs[i].cvPort;

        cvs.clear();
    }

    CARLA_DECLARE_NON_COPYABLE(ProtectedData)
};

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
// -----------------------------------------------------------------------
// Carla Engine Meta CV Port

class CarlaEngineCVSourcePortsForStandalone : public CarlaEngineCVSourcePorts
{
public:
    CarlaEngineCVSourcePortsForStandalone() : CarlaEngineCVSourcePorts() {}
    ~CarlaEngineCVSourcePortsForStandalone() override {}

    inline void resetGraphAndPlugin() noexcept
    {
        pData->graph = nullptr;
        pData->plugin.reset();
    }

    inline void setGraphAndPlugin(PatchbayGraph* const graph, const CarlaPluginPtr plugin) noexcept
    {
        pData->graph = graph;
        pData->plugin = plugin;
    }
};
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_PORTS_HPP_INCLUDED
