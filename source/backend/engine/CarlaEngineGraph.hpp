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

#ifndef CARLA_ENGINE_GRAPH_HPP_INCLUDED
#define CARLA_ENGINE_GRAPH_HPP_INCLUDED

#include "CarlaEngine.hpp"
#include "CarlaMutex.hpp"
#include "CarlaPatchbayUtils.hpp"
#include "CarlaStringList.hpp"
#include "CarlaRunner.hpp"

#include "water/processors/AudioProcessorGraph.h"
#include "water/text/StringArray.h"

using water::AudioProcessorGraph;
using water::AudioSampleBuffer;
using water::MidiBuffer;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

struct PatchbayPosition {
    bool active;
    int x1, y1, x2, y2;
};

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
    const char* getName(bool isInput, uint portId) const noexcept;
    uint getPortIdFromName(bool isInput, const char name[], bool* ok = nullptr) const noexcept;
    uint getPortIdFromIdentifier(bool isInput, const char identifier[], bool* ok = nullptr) const noexcept;
    ExternalGraphPorts() noexcept;
    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(ExternalGraphPorts)
};

struct ExternalGraph {
    PatchbayConnectionList connections;
    ExternalGraphPorts audioPorts, midiPorts;
    PatchbayPosition positions[kExternalGraphGroupMax];
    mutable CharStringListPtr retCon;
    ExternalGraph(CarlaEngine* engine) noexcept;

    void clear() noexcept;

    bool connect(bool sendHost, bool sendOSC,
                 uint groupA, uint portA, uint groupB, uint portB) noexcept;
    bool disconnect(bool sendHost, bool sendOSC,
                    uint connectionId) noexcept;
    void setGroupPos(bool sendHost, bool sendOSC,
                     uint groupId, int x1, int y1, int x2, int y2);
    void refresh(bool sendHost, bool sendOSC,
                 const char* deviceName);

    const char* const* getConnections() const noexcept;
    bool getGroupFromName(const char* groupName, uint& groupId) const noexcept;
    bool getGroupAndPortIdFromFullName(const char* fullPortName, uint& groupId, uint& portId) const noexcept;

    CarlaEngine* const kEngine;
    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(ExternalGraph)
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
        float* unusedBuf;
        Buffers() noexcept;
        ~Buffers() noexcept;
        void setBufferSize(uint32_t bufferSize, bool createBuffers) noexcept;
        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPYABLE(Buffers)
    } audioBuffers;

    RackGraph(CarlaEngine* engine, uint32_t inputs, uint32_t outputs) noexcept;
    ~RackGraph() noexcept;

    void setBufferSize(uint32_t bufferSize) noexcept;
    void setOffline(bool offline) noexcept;

    bool connect(uint groupA, uint portA, uint groupB, uint portB) noexcept;
    bool disconnect(uint connectionId) noexcept;
    void refresh(bool sendHost, bool sendOsc, bool ignored, const char* deviceName);

    const char* const* getConnections() const noexcept;
    bool getGroupAndPortIdFromFullName(const char* fullPortName, uint& groupId, uint& portId) const noexcept;

    // the base, where plugins run
    void process(CarlaEngine::ProtectedData* data, const float* inBuf[2], float* outBuf[2], uint32_t frames);

    // extended, will call process() in the middle
    void processHelper(CarlaEngine::ProtectedData* data, const float* const* inBuf, float* const* outBuf, uint32_t frames);

    CarlaEngine* const kEngine;
    CARLA_DECLARE_NON_COPYABLE(RackGraph)
};

// -----------------------------------------------------------------------
// PatchbayGraph

class PatchbayGraph : private CarlaRunner {
public:
    PatchbayConnectionList connections;
    AudioProcessorGraph graph;
    AudioSampleBuffer audioBuffer;
    AudioSampleBuffer cvInBuffer;
    AudioSampleBuffer cvOutBuffer;
    MidiBuffer midiBuffer;
    const uint32_t numAudioIns;
    const uint32_t numAudioOuts;
    const uint32_t numCVIns;
    const uint32_t numCVOuts;
    mutable CharStringListPtr retCon;
    bool usingExternalHost;
    bool usingExternalOSC;

    ExternalGraph extGraph;

    PatchbayGraph(CarlaEngine* engine,
                  uint32_t audioIns, uint32_t audioOuts,
                  uint32_t cvIns, uint32_t cvOuts,
                  bool withMidiIn, bool withMidiOut);
    ~PatchbayGraph();

    void setBufferSize(uint32_t bufferSize);
    void setSampleRate(double sampleRate);
    void setOffline(bool offline);

    void addPlugin(CarlaPluginPtr plugin);
    void replacePlugin(CarlaPluginPtr oldPlugin, CarlaPluginPtr newPlugin);
    void renamePlugin(CarlaPluginPtr plugin, const char* newName);
    void switchPlugins(CarlaPluginPtr pluginA, CarlaPluginPtr pluginB);
    void reconfigureForCV(CarlaPluginPtr plugin, const uint portIndex, bool added);
    void reconfigurePlugin(CarlaPluginPtr plugin, bool portsAdded);
    void removePlugin(CarlaPluginPtr plugin);
    void removeAllPlugins(bool aboutToClose);

    bool connect(bool external, uint groupA, uint portA, uint groupB, uint portB);
    bool disconnect(bool external, uint connectionId);
    void disconnectInternalGroup(uint groupId) noexcept;
    void setGroupPos(bool sendHost, bool sendOsc, bool external, uint groupId, int x1, int y1, int x2, int y2);
    void refresh(bool sendHost, bool sendOsc, bool external, const char* deviceName);

    const char* const* getConnections(bool external) const;
    const CarlaEngine::PatchbayPosition* getPositions(bool external, uint& count) const;
    bool getGroupFromName(bool external, const char* groupName, uint& groupId) const;
    bool getGroupAndPortIdFromFullName(bool external, const char* fullPortName, uint& groupId, uint& portId) const;

    void process(CarlaEngine::ProtectedData* data,
                 const float* const* inBuf,
                 float* const* outBuf,
                 uint32_t frames);

private:
    bool run() override;

    CarlaEngine* const kEngine;
    CARLA_DECLARE_NON_COPYABLE(PatchbayGraph)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_GRAPH_HPP_INCLUDED
