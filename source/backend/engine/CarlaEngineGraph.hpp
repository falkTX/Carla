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
#include "CarlaStringList.hpp"

#if 0
# include "juce_audio_processors.h"
using juce::AudioProcessorGraph;
using juce::AudioSampleBuffer;
using juce::MidiBuffer;
#else
# include "juce_audio_basics.h"
#endif

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
// RackGraph

struct RackGraph {
    PatchbayConnectionList connections;
    const uint32_t inputs;
    const uint32_t outputs;
    bool isOffline;
    mutable CharStringListPtr retCon;

    struct Audio {
        CarlaRecursiveMutex mutex;
        LinkedList<uint> connectedIn1;
        LinkedList<uint> connectedIn2;
        LinkedList<uint> connectedOut1;
        LinkedList<uint> connectedOut2;
        float* inBuf[2];
        float* inBufTmp[2];
        float* outBuf[2];
        // c++ compat stuff
        Audio() noexcept;
        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(Audio)
    } audio;

    struct MIDI {
        LinkedList<PortNameToId> ins;
        LinkedList<PortNameToId> outs;
        const char* getName(const bool isInput, const uint portId) const noexcept;
        uint getPortId(const bool isInput, const char portName[], bool* const ok = nullptr) const noexcept;
        MIDI() noexcept;
        // c++ compat stuff
        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(MIDI)
    } midi;

    RackGraph(const uint32_t bufferSize, const uint32_t inputs, const uint32_t outputs) noexcept;
    ~RackGraph() noexcept;

    void setBufferSize(const uint32_t bufferSize) noexcept;
    void setOffline(const bool offline) noexcept;

    bool connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept;
    bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept;
    void clearConnections() noexcept;

    const char* const* getConnections() const noexcept;
    bool getGroupAndPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const noexcept;

    // the base, where plugins run
    void process(CarlaEngine::ProtectedData* const data, const float* inBufReal[2], float* outBuf[2], const uint32_t frames);

    // extended, will call process() in the middle
    void processHelper(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const uint32_t frames);
};

#if 0
// -----------------------------------------------------------------------
// PatchbayGraph

struct PatchbayGraph  {
    AudioProcessorGraph graph;
    AudioSampleBuffer audioBuffer;
    MidiBuffer midiBuffer;
    const int inputs;
    const int outputs;
    //CharStringListPtr retCon;

    PatchbayGraph(const int bufferSize, const double sampleRate, const uint32_t inputs, const uint32_t outputs);
    ~PatchbayGraph();

    void setBufferSize(const int bufferSize);
    void setSampleRate(const double sampleRate);
    void setOffline(const bool offline);

    void addPlugin(CarlaPlugin* const plugin);
    void replacePlugin(CarlaPlugin* const oldPlugin, CarlaPlugin* const newPlugin);
    void removePlugin(CarlaPlugin* const plugin);
    void removeAllPlugins();

    //bool connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept;
    //bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept;

    //const char* const* getConnections() const noexcept;
    //bool getPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const noexcept;

    void process(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const int frames);
};
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_GRAPH_HPP_INCLUDED
