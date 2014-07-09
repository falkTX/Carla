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

#ifndef CARLA_ENGINE_GRAPH_HPP_INCLUDED
#define CARLA_ENGINE_GRAPH_HPP_INCLUDED

#include "CarlaEngine.hpp"
#include "CarlaMutex.hpp"
#include "CarlaPatchbayUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Rack Graph stuff

enum RackGraphGroupIds {
    RACK_GRAPH_GROUP_CARLA     = 0,
    RACK_GRAPH_GROUP_AUDIO_IN  = 1,
    RACK_GRAPH_GROUP_AUDIO_OUT = 2,
    RACK_GRAPH_GROUP_MIDI_IN   = 3,
    RACK_GRAPH_GROUP_MIDI_OUT  = 4,
    RACK_GRAPH_GROUP_MAX       = 5
};

enum RackGraphCarlaPortIds {
    RACK_GRAPH_CARLA_PORT_NULL       = 0,
    RACK_GRAPH_CARLA_PORT_AUDIO_IN1  = 1,
    RACK_GRAPH_CARLA_PORT_AUDIO_IN2  = 2,
    RACK_GRAPH_CARLA_PORT_AUDIO_OUT1 = 3,
    RACK_GRAPH_CARLA_PORT_AUDIO_OUT2 = 4,
    RACK_GRAPH_CARLA_PORT_MIDI_IN    = 5,
    RACK_GRAPH_CARLA_PORT_MIDI_OUT   = 6,
    RACK_GRAPH_CARLA_PORT_MAX        = 7
};

// -----------------------------------------------------------------------
// InternalGraph

struct InternalGraph {
    virtual ~InternalGraph() noexcept {}
    virtual void clear() noexcept = 0;
    virtual bool connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept = 0;
    virtual bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept = 0;
    virtual const char* const* getConnections() const = 0;
    virtual bool getPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const = 0;
};

// -----------------------------------------------------------------------
// RackGraph

struct RackGraph : InternalGraph {
    PatchbayConnectionList connections;

    struct Audio {
        CarlaRecursiveMutex mutex;
        LinkedList<uint> connectedIn1;
        LinkedList<uint> connectedIn2;
        LinkedList<uint> connectedOut1;
        LinkedList<uint> connectedOut2;
        float* inBuf[2];
        float* outBuf[2];
    } audio;

    struct MIDI {
        LinkedList<PortNameToId> ins;
        LinkedList<PortNameToId> outs;
        const char* getName(const bool isInput, const uint portId) const noexcept;
        uint getPortId(const bool isInput, const char portName[]) const noexcept;
    } midi;

    RackGraph(const uint32_t bufferSize) noexcept;
    ~RackGraph() noexcept override;
    void clear() noexcept override;
    void resize(const uint32_t bufferSize) noexcept;
    bool connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept override;
    bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept override;
    const char* const* getConnections() const override;
    bool getPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const override;
};

// -----------------------------------------------------------------------
// PatchbayGraph

struct PatchbayGraph : InternalGraph {
    // TODO

    PatchbayGraph() noexcept;
    ~PatchbayGraph() noexcept override;
    void clear() noexcept override;
    bool connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept override;
    bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept override;
    const char* const* getConnections() const override;
    bool getPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const override;
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_GRAPH_HPP_INCLUDED
