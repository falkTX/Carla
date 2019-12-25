/*
 * Carla Plugin Host
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_ENGINE_CLIENT_HPP_INCLUDED
#define CARLA_ENGINE_CLIENT_HPP_INCLUDED

#include "CarlaEngine.hpp"
#include "CarlaEngineInternal.hpp"

#include "CarlaStringList.hpp"

CARLA_BACKEND_START_NAMESPACE

class EngineInternalGraph;
class PatchbayGraph;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
// -----------------------------------------------------------------------
// Carla Engine Meta CV port

class CarlaEngineCVSourcePorts2 : public CarlaEngineCVSourcePorts
{
public:
    CarlaEngineCVSourcePorts2() : CarlaEngineCVSourcePorts() {}
    ~CarlaEngineCVSourcePorts2() override {}

    void setGraphAndPlugin(PatchbayGraph* const graph, CarlaPlugin* const plugin) noexcept
    {
        pData->graph = graph;
        pData->plugin = plugin;
    }
};
#endif

// -----------------------------------------------------------------------
// Carla Engine client (Abstract)

struct CarlaEngineClient::ProtectedData {
    const CarlaEngine& engine;

    bool     active;
    uint32_t latency;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineCVSourcePorts2 cvSourcePorts;
    EngineInternalGraph& egraph;
    CarlaPlugin* const plugin;
#endif

    CarlaStringList audioInList;
    CarlaStringList audioOutList;
    CarlaStringList cvInList;
    CarlaStringList cvOutList;
    CarlaStringList eventInList;
    CarlaStringList eventOutList;

    ProtectedData(const CarlaEngine& eng, EngineInternalGraph& eg, CarlaPlugin* const p) noexcept
        :  engine(eng),
           active(false),
           latency(0),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
           cvSourcePorts(),
           egraph(eg),
           plugin(p),
#endif
           audioInList(),
           audioOutList(),
           cvInList(),
           cvOutList(),
           eventInList(),
           eventOutList() {}

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ProtectedData)
#endif
};

// -----------------------------------------------------------------------

class CarlaEngineClient2 : public CarlaEngineClient
{
public:
    CarlaEngineClient2(const CarlaEngine& engine, EngineInternalGraph& egraph, CarlaPlugin* const plugin);
    virtual ~CarlaEngineClient2() override;

protected:
    PatchbayGraph* getPatchbayGraph() const noexcept;
    CarlaPlugin* getPlugin() const noexcept;
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_CLIENT_HPP_INCLUDED
