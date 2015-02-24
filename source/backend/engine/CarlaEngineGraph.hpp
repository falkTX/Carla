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

#include "juce_audio_processors.h"
using juce::AudioProcessorGraph;
using juce::AudioSampleBuffer;
using juce::MidiBuffer;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// External Graph stuff

enum ExternalGraphGroupIds {
    kExternalGraphGroupNull     = 0,
    kExternalGraphGroupCarla    = 1,
    kExternalGraphGroupAudioIn  = 2,
    kExternalGraphGroupAudioOut = 3,
    kExternalGraphGroupMidiIn   = 4,
    kExternalGraphGroupMidiOut  = 5,
    kExternalGraphGroupMax      = 6
};

enum ExternalGraphCarlaPortIds {
    kExternalGraphCarlaPortNull      = 0,
    kExternalGraphCarlaPortAudioIn1  = 1,
    kExternalGraphCarlaPortAudioIn2  = 2,
    kExternalGraphCarlaPortAudioOut1 = 3,
    kExternalGraphCarlaPortAudioOut2 = 4,
    kExternalGraphCarlaPortMidiIn    = 5,
    kExternalGraphCarlaPortMidiOut   = 6,
    kExternalGraphCarlaPortMax       = 7
};

enum ExternalGraphConnectionType {
    kExternalGraphConnectionNull       = 0,
    kExternalGraphConnectionAudioIn1   = 1,
    kExternalGraphConnectionAudioIn2   = 2,
    kExternalGraphConnectionAudioOut1  = 3,
    kExternalGraphConnectionAudioOut2  = 4,
    kExternalGraphConnectionMidiInput  = 5,
    kExternalGraphConnectionMidiOutput = 6
};

struct ExternalGraphPorts {
    LinkedList<PortNameToId> ins;
    LinkedList<PortNameToId> outs;
    const char* getName(const bool isInput, const uint portId) const noexcept;
    uint getPortId(const bool isInput, const char portName[], bool* const ok = nullptr) const noexcept;
    ExternalGraphPorts() noexcept;
    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ExternalGraphPorts)
};

struct ExternalGraph {
    PatchbayConnectionList connections;
    ExternalGraphPorts audioPorts, midiPorts;
    mutable CharStringListPtr retCon;
    ExternalGraph(CarlaEngine* const engine) noexcept;

    void clear() noexcept;
    bool connect(const uint groupA, const uint portA, const uint groupB, const uint portB, const bool sendCallback) noexcept;
    bool disconnect(const uint connectionId) noexcept;
    void refresh(const char* const deviceName);

    const char* const* getConnections() const noexcept;
    bool getGroupAndPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const noexcept;

    CarlaEngine* const kEngine;
    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ExternalGraph)
};

// -----------------------------------------------------------------------
// RackGraph

struct RackGraph {
    ExternalGraph extGraph;
    const uint32_t inputs;
    const uint32_t outputs;
    bool isOffline;

    struct Buffers {
        CarlaRecursiveMutex mutex;
        LinkedList<uint> connectedIn1;
        LinkedList<uint> connectedIn2;
        LinkedList<uint> connectedOut1;
        LinkedList<uint> connectedOut2;
        float* inBuf[2];
        float* inBufTmp[2];
        float* outBuf[2];
        Buffers() noexcept;
        ~Buffers() noexcept;
        void setBufferSize(const uint32_t bufferSize, const bool createBuffers) noexcept;
        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(Buffers)
    } audioBuffers;

    RackGraph(CarlaEngine* const engine, const uint32_t inputs, const uint32_t outputs) noexcept;
    ~RackGraph() noexcept;

    void setBufferSize(const uint32_t bufferSize) noexcept;
    void setOffline(const bool offline) noexcept;

    bool connect(const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept;
    bool disconnect(const uint connectionId) noexcept;
    void refresh(const char* const deviceName);

    const char* const* getConnections() const noexcept;
    bool getGroupAndPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const noexcept;

    // the base, where plugins run
    void process(CarlaEngine::ProtectedData* const data, const float* inBufReal[2], float* outBuf[2], const uint32_t frames);

    // extended, will call process() in the middle
    void processHelper(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const uint32_t frames);

    CarlaEngine* const kEngine;
    CARLA_DECLARE_NON_COPY_CLASS(RackGraph)
};

// -----------------------------------------------------------------------
// PatchbayGraph

struct PatchbayGraph {
    PatchbayConnectionList connections;
    AudioProcessorGraph graph;
    AudioSampleBuffer audioBuffer;
    MidiBuffer midiBuffer;
    const uint32_t inputs;
    const uint32_t outputs;
    mutable CharStringListPtr retCon;
    bool usingExternal;

    ExternalGraph extGraph;

    PatchbayGraph(CarlaEngine* const engine, const uint32_t inputs, const uint32_t outputs);
    ~PatchbayGraph();

    void setBufferSize(const uint32_t bufferSize);
    void setSampleRate(const double sampleRate);
    void setOffline(const bool offline);

    void addPlugin(CarlaPlugin* const plugin);
    void replacePlugin(CarlaPlugin* const oldPlugin, CarlaPlugin* const newPlugin);
    void removePlugin(CarlaPlugin* const plugin);
    void removeAllPlugins();

    bool connect(const bool external, const uint groupA, const uint portA, const uint groupB, const uint portB, const bool sendCallback);
    bool disconnect(const uint connectionId);
    void disconnectInternalGroup(const uint groupId) noexcept;
    void refresh(const char* const deviceName);

    const char* const* getConnections(const bool external) const;
    bool getGroupAndPortIdFromFullName(const bool external, const char* const fullPortName, uint& groupId, uint& portId) const;

    void process(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const int frames);

    CarlaEngine* const kEngine;
    CARLA_DECLARE_NON_COPY_CLASS(PatchbayGraph)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_GRAPH_HPP_INCLUDED
