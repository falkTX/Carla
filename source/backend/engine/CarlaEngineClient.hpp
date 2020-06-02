/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaEnginePorts.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaStringList.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Carla Engine Client Protected Data

struct CarlaEngineClient::ProtectedData {
    const CarlaEngine& engine;

    bool     active;
    uint32_t latency;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaEngineCVSourcePortsForStandalone cvSourcePorts;
    EngineInternalGraph& egraph;
    CarlaPluginPtr plugin;
#endif

    CarlaStringList audioInList;
    CarlaStringList audioOutList;
    CarlaStringList cvInList;
    CarlaStringList cvOutList;
    CarlaStringList eventInList;
    CarlaStringList eventOutList;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    ProtectedData(const CarlaEngine& eng, EngineInternalGraph& eg, CarlaPluginPtr p) noexcept;
#else
    ProtectedData(const CarlaEngine& eng) noexcept;
#endif
    ~ProtectedData();

    void addAudioPortName(bool isInput, const char* name);
    void addCVPortName(bool isInput, const char* name);
    void addEventPortName(bool isInput, const char* name);
    void clearPorts();
    const char* getUniquePortName(const char* name);

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ProtectedData)
#endif
};

// -----------------------------------------------------------------------
// Carla Engine Client

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
class CarlaEngineClientForStandalone : public CarlaEngineClient
{
public:
    CarlaEngineClientForStandalone(const CarlaEngine& engine,
                                   EngineInternalGraph& egraph,
                                   const CarlaPluginPtr plugin)
        : CarlaEngineClient(new ProtectedData(engine, egraph, plugin)) {}

    ~CarlaEngineClientForStandalone() noexcept override
    {
        carla_debug("CarlaEngineClientForStandalone::~CarlaEngineClientForStandalone()");
        delete pData;
    }

protected:
    inline PatchbayGraph* getPatchbayGraphOrNull() const noexcept
    {
        return pData->egraph.getPatchbayGraphOrNull();
    }

    inline CarlaPluginPtr getPlugin() const noexcept
    {
        return pData->plugin;
    }

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineClientForStandalone)
};
typedef CarlaEngineClientForStandalone CarlaEngineClientForSubclassing;
#else
class CarlaEngineClientForBridge : public CarlaEngineClient
{
public:
    CarlaEngineClientForBridge(const CarlaEngine& engine)
        : CarlaEngineClient(new ProtectedData(engine)) {}

    ~CarlaEngineClientForBridge() override
    {
        carla_debug("CarlaEngineClientForBridge::~CarlaEngineClientForBridge()");
        delete pData;
    }

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineClientForBridge)
};
typedef CarlaEngineClientForBridge CarlaEngineClientForSubclassing;
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_CLIENT_HPP_INCLUDED
