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

#include "CarlaEngineGraph.hpp"
#include "CarlaEngineInternal.hpp"

#include "juce_audio_basics.h"
using juce::FloatVectorOperations;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Rack Graph stuff

static inline
uint getCarlaRackPortIdFromName(const char* const shortname) noexcept
{
    if (std::strcmp(shortname, "AudioIn1") == 0 || std::strcmp(shortname, "audio-in1") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_IN1;
    if (std::strcmp(shortname, "AudioIn2") == 0 || std::strcmp(shortname, "audio-in2") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_IN2;
    if (std::strcmp(shortname, "AudioOut1") == 0 || std::strcmp(shortname, "audio-out1") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_OUT1;
    if (std::strcmp(shortname, "AudioOut2") == 0 || std::strcmp(shortname, "audio-out2") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_OUT2;
    if (std::strcmp(shortname, "MidiIn") == 0 || std::strcmp(shortname, "midi-in") == 0)
        return RACK_GRAPH_CARLA_PORT_MIDI_IN;
    if (std::strcmp(shortname, "MidiOut") == 0 || std::strcmp(shortname, "midi-out") == 0)
        return RACK_GRAPH_CARLA_PORT_MIDI_OUT;

    carla_stderr("CarlaBackend::getCarlaRackPortIdFromName(%s) - invalid short name", shortname);
    return RACK_GRAPH_CARLA_PORT_NULL;
}

// -----------------------------------------------------------------------
// RackGraph MIDI

const char* RackGraph::MIDI::getName(const bool isInput, const uint portId) const noexcept
{
    for (LinkedList<PortNameToId>::Itenerator it = isInput ? ins.begin() : outs.begin(); it.valid(); it.next())
    {
        const PortNameToId& port(it.getValue());

        if (port.port == portId)
            return port.name;
    }

    return nullptr;
}

uint RackGraph::MIDI::getPortId(const bool isInput, const char portName[]) const noexcept
{
    for (LinkedList<PortNameToId>::Itenerator it = isInput ? ins.begin() : outs.begin(); it.valid(); it.next())
    {
        const PortNameToId& port(it.getValue());

        if (std::strcmp(port.name, portName) == 0)
            return port.port;
    }

    return 0;
}

// -----------------------------------------------------------------------
// RackGraph

RackGraph::RackGraph(const uint32_t bufferSize) noexcept
{
    audio.inBuf[0]  = audio.inBuf[1]  = nullptr;
    audio.outBuf[0] = audio.outBuf[1] = nullptr;
    resize(bufferSize);
}

RackGraph::~RackGraph() noexcept
{
    clear();
}

void RackGraph::clear() noexcept
{
    connections.clear();

    audio.mutex.lock();
    audio.connectedIn1.clear();
    audio.connectedIn2.clear();
    audio.connectedOut1.clear();
    audio.connectedOut2.clear();
    audio.mutex.unlock();

    midi.ins.clear();
    midi.outs.clear();
}

void RackGraph::resize(const uint32_t bufferSize) noexcept
{
    if (audio.inBuf[0] != nullptr)
    {
        delete[] audio.inBuf[0];
        audio.inBuf[0] = nullptr;
    }
    if (audio.inBuf[1] != nullptr)
    {
        delete[] audio.inBuf[1];
        audio.inBuf[1] = nullptr;
    }
    if (audio.outBuf[0] != nullptr)
    {
        delete[] audio.outBuf[0];
        audio.outBuf[0] = nullptr;
    }
    if (audio.outBuf[1] != nullptr)
    {
        delete[] audio.outBuf[1];
        audio.outBuf[1] = nullptr;
    }

    try {
        audio.inBuf[0]  = new float[bufferSize];
        audio.inBuf[1]  = new float[bufferSize];
        audio.outBuf[0] = new float[bufferSize];
        audio.outBuf[1] = new float[bufferSize];
    }
    catch(...) {
        if (audio.inBuf[0] != nullptr)
        {
            delete[] audio.inBuf[0];
            audio.inBuf[0] = nullptr;
        }
        if (audio.inBuf[1] != nullptr)
        {
            delete[] audio.inBuf[1];
            audio.inBuf[1] = nullptr;
        }
        if (audio.outBuf[0] != nullptr)
        {
            delete[] audio.outBuf[0];
            audio.outBuf[0] = nullptr;
        }
        if (audio.outBuf[1] != nullptr)
        {
            delete[] audio.outBuf[1];
            audio.outBuf[1] = nullptr;
        }
        return;
    }

    FloatVectorOperations::clear(audio.inBuf[0], bufferSize);
    FloatVectorOperations::clear(audio.inBuf[1], bufferSize);
    FloatVectorOperations::clear(audio.outBuf[0], bufferSize);
    FloatVectorOperations::clear(audio.outBuf[1], bufferSize);
}

bool RackGraph::connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);

    uint otherGroup, otherPort, carlaPort;

    if (groupA == RACK_GRAPH_GROUP_CARLA)
    {
        CARLA_SAFE_ASSERT_RETURN(groupB != RACK_GRAPH_GROUP_CARLA, false);

        carlaPort  = portA;
        otherGroup = groupB;
        otherPort  = portB;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(groupB == RACK_GRAPH_GROUP_CARLA, false);

        carlaPort  = portB;
        otherGroup = groupA;
        otherPort  = portA;
    }

    CARLA_SAFE_ASSERT_RETURN(carlaPort > RACK_GRAPH_CARLA_PORT_NULL && carlaPort < RACK_GRAPH_CARLA_PORT_MAX, false);
    CARLA_SAFE_ASSERT_RETURN(otherGroup > RACK_GRAPH_GROUP_CARLA && otherGroup < RACK_GRAPH_GROUP_MAX, false);

    bool makeConnection = false;

    switch (carlaPort)
    {
    case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == RACK_GRAPH_GROUP_AUDIO_IN, false);
        audio.mutex.lock();
        makeConnection = audio.connectedIn1.append(otherPort);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == RACK_GRAPH_GROUP_AUDIO_IN, false);
        audio.mutex.lock();
        makeConnection = audio.connectedIn2.append(otherPort);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == RACK_GRAPH_GROUP_AUDIO_OUT, false);
        audio.mutex.lock();
        makeConnection = audio.connectedOut1.append(otherPort);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == RACK_GRAPH_GROUP_AUDIO_OUT, false);
        audio.mutex.lock();
        makeConnection = audio.connectedOut2.append(otherPort);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_MIDI_IN:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == RACK_GRAPH_GROUP_MIDI_IN, false);
        if (const char* const portName = midi.getName(true, otherPort))
            makeConnection = engine->connectRackMidiInPort(portName);
        break;

    case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == RACK_GRAPH_GROUP_MIDI_OUT, false);
        if (const char* const portName = midi.getName(false, otherPort))
            makeConnection = engine->connectRackMidiOutPort(portName);
        break;
    }

    if (! makeConnection)
    {
        engine->setLastError("Invalid rack connection");
        return false;
    }

    ConnectionToId connectionToId;
    connectionToId.setData(++connections.lastId, groupA, portA, groupB, portB);

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';
    std::snprintf(strBuf, STR_MAX, "%u:%u:%u:%u", groupA, portA, groupB, portB);

    engine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

    connections.list.append(connectionToId);
    return true;
}

bool RackGraph::disconnect(CarlaEngine* const engine, const uint connectionId) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(connections.list.count() > 0, false);

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin(); it.valid(); it.next())
    {
        const ConnectionToId& connection(it.getValue());

        if (connection.id != connectionId)
            continue;

        uint otherGroup, otherPort, carlaPort;

        if (connection.groupA == RACK_GRAPH_GROUP_CARLA)
        {
            CARLA_SAFE_ASSERT_RETURN(connection.groupB != RACK_GRAPH_GROUP_CARLA, false);

            carlaPort  = connection.portA;
            otherGroup = connection.groupB;
            otherPort  = connection.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(connection.groupB == RACK_GRAPH_GROUP_CARLA, false);

            carlaPort  = connection.portB;
            otherGroup = connection.groupA;
            otherPort  = connection.portA;
        }

        CARLA_SAFE_ASSERT_RETURN(carlaPort > RACK_GRAPH_CARLA_PORT_NULL && carlaPort < RACK_GRAPH_CARLA_PORT_MAX, false);
        CARLA_SAFE_ASSERT_RETURN(otherGroup > RACK_GRAPH_GROUP_CARLA && otherGroup < RACK_GRAPH_GROUP_MAX, false);

        bool makeDisconnection = false;

        switch (carlaPort)
        {
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
            audio.mutex.lock();
            makeDisconnection = audio.connectedIn1.removeOne(otherPort);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
            audio.mutex.lock();
            makeDisconnection = audio.connectedIn2.removeOne(otherPort);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
            audio.mutex.lock();
            makeDisconnection = audio.connectedOut1.removeOne(otherPort);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
            audio.mutex.lock();
            makeDisconnection = audio.connectedOut2.removeOne(otherPort);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_IN:
            if (const char* const portName = midi.getName(true, otherPort))
                makeDisconnection = engine->disconnectRackMidiInPort(portName);
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
            if (const char* const portName = midi.getName(false, otherPort))
                makeDisconnection = engine->disconnectRackMidiOutPort(portName);
            break;
        }

        if (! makeDisconnection)
        {
            engine->setLastError("Invalid rack connection");
            return false;
        }

        engine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connection.id, 0, 0, 0.0f, nullptr);

        connections.list.remove(it);
        return true;
    }

    engine->setLastError("Failed to find connection");
    return false;
}

const char* const* RackGraph::getConnections() const
{
    if (connections.list.count() == 0)
        return nullptr;

    LinkedList<const char*> connList;
    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin(); it.valid(); it.next())
    {
        const ConnectionToId& connection(it.getValue());

        uint otherGroup, otherPort, carlaPort;

        if (connection.groupA == RACK_GRAPH_GROUP_CARLA)
        {
            CARLA_SAFE_ASSERT_CONTINUE(connection.groupB != RACK_GRAPH_GROUP_CARLA);

            carlaPort  = connection.portA;
            otherGroup = connection.groupB;
            otherPort  = connection.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_CONTINUE(connection.groupB == RACK_GRAPH_GROUP_CARLA);

            carlaPort  = connection.portB;
            otherGroup = connection.groupA;
            otherPort  = connection.portA;
        }

        CARLA_SAFE_ASSERT_CONTINUE(carlaPort > RACK_GRAPH_CARLA_PORT_NULL && carlaPort < RACK_GRAPH_CARLA_PORT_MAX);
        CARLA_SAFE_ASSERT_CONTINUE(otherGroup > RACK_GRAPH_GROUP_CARLA && otherGroup < RACK_GRAPH_GROUP_MAX);

        switch (carlaPort)
        {
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
            std::snprintf(strBuf, STR_MAX, "AudioIn:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup((carlaPort == RACK_GRAPH_CARLA_PORT_AUDIO_IN1) ? "Carla:AudioIn1" : "Carla:AudioIn2"));
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
            std::snprintf(strBuf, STR_MAX, "AudioOut:%i", otherPort+1);
            connList.append(carla_strdup((carlaPort == RACK_GRAPH_CARLA_PORT_AUDIO_OUT1) ? "Carla:AudioOut1" : "Carla:AudioOut2"));
            connList.append(carla_strdup(strBuf));
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_IN:
            std::snprintf(strBuf, STR_MAX, "MidiIn:%s", midi.getName(true, otherPort));
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup("Carla:MidiIn"));
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
            std::snprintf(strBuf, STR_MAX, "MidiOut:%s", midi.getName(false, otherPort));
            connList.append(carla_strdup("Carla:MidiOut"));
            connList.append(carla_strdup(strBuf));
            break;
        }
    }

    const size_t connCount(connList.count());

    if (connCount == 0)
        return nullptr;

    const char** const retConns = new const char*[connCount+1];

    for (size_t i=0; i < connCount; ++i)
    {
        retConns[i] = connList.getAt(i, nullptr);

        if (retConns[i] == nullptr)
            retConns[i] = carla_strdup("(unknown)");
    }

    retConns[connCount] = nullptr;
    connList.clear();

    return retConns;
}

bool RackGraph::getPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const
{
    CARLA_SAFE_ASSERT_RETURN(fullPortName != nullptr && fullPortName[0] != '\0', false);

    int portTest;

    if (std::strncmp(fullPortName, "Carla:", 6) == 0)
    {
        groupId = RACK_GRAPH_GROUP_CARLA;
        portId  = getCarlaRackPortIdFromName(fullPortName+6);
        CARLA_SAFE_ASSERT_RETURN(portId > RACK_GRAPH_CARLA_PORT_NULL && portId < RACK_GRAPH_CARLA_PORT_MAX, false);
    }
    else if (std::strncmp(fullPortName, "AudioIn:", 8) == 0)
    {
        groupId  = RACK_GRAPH_GROUP_AUDIO_IN;
        portTest = std::atoi(fullPortName+8) - 1;
        CARLA_SAFE_ASSERT_RETURN(portTest >= 0, false);
        portId   = static_cast<uint>(portTest);
    }
    else if (std::strncmp(fullPortName, "AudioOut:", 9) == 0)
    {
        groupId  = RACK_GRAPH_GROUP_AUDIO_OUT;
        portTest = std::atoi(fullPortName+9) - 1;
        CARLA_SAFE_ASSERT_RETURN(portTest >= 0, false);
        portId   = static_cast<uint>(portTest);
    }
    else if (std::strncmp(fullPortName, "MidiIn:", 7) == 0)
    {
        groupId = RACK_GRAPH_GROUP_MIDI_IN;
        //portId  = std::atoi(fullPortName+7) - 1;
    }
    else if (std::strncmp(fullPortName, "MidiOut:", 8) == 0)
    {
        groupId = RACK_GRAPH_GROUP_MIDI_OUT;
        //portId  = std::atoi(fullPortName+8) - 1;
    }
    else
    {
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------
// PatchbayGraph

// TODO

PatchbayGraph::PatchbayGraph() noexcept
{
}

PatchbayGraph::~PatchbayGraph() noexcept
{
    clear();
}

void PatchbayGraph::clear() noexcept
{
}

bool PatchbayGraph::connect(CarlaEngine* const engine, const uint /*groupA*/, const uint /*portA*/, const uint /*groupB*/, const uint /*portB*/) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);

    return false;
}

bool PatchbayGraph::disconnect(CarlaEngine* const engine, const uint /*connectionId*/) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);

    return false;
}

const char* const* PatchbayGraph::getConnections() const
{
    return nullptr;
}

bool PatchbayGraph::getPortIdFromFullName(const char* const /*fillPortName*/, uint& /*groupId*/, uint& /*portId*/) const
{
    return false;
}

// -----------------------------------------------------------------------
// InternalGraph

EngineInternalGraph::EngineInternalGraph() noexcept
    : isRack(true),
      isReady(false),
      graph(nullptr) {}

EngineInternalGraph::~EngineInternalGraph() noexcept
{
    CARLA_SAFE_ASSERT(graph == nullptr);
}

void EngineInternalGraph::create(const uint32_t bufferSize)
{
    CARLA_SAFE_ASSERT_RETURN(graph == nullptr,);

    if (isRack)
        graph = new RackGraph(bufferSize);
    else
        graph = new PatchbayGraph();

    //isReady = true;
}

void EngineInternalGraph::resize(const uint32_t bufferSize) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr,);

    if (isRack)
      ((RackGraph*)graph)->resize(bufferSize);
}

void EngineInternalGraph::clear() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr,);

    delete graph;
    graph = nullptr;
}

// -----------------------------------------------------------------------
// CarlaEngine Patchbay stuff

bool CarlaEngine::patchbayConnect(const uint groupA, const uint portA, const uint groupB, const uint portB)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.graph != nullptr, false);
    carla_debug("CarlaEngine::patchbayConnect(%u, %u, %u, %u)", groupA, portA, groupB, portB);

    return pData->graph.graph->connect(this, groupA, portA, groupB, portB);
}

bool CarlaEngine::patchbayDisconnect(const uint connectionId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.graph != nullptr, false);
    carla_debug("CarlaEngine::patchbayDisconnect(%u)", connectionId);

    return pData->graph.graph->disconnect(this, connectionId);
}

bool CarlaEngine::patchbayRefresh()
{
    setLastError("Unsupported operation");
    return false;
}

// -----------------------------------------------------------------------

const char* const* CarlaEngine::getPatchbayConnections() const
{
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady, nullptr);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.graph != nullptr, nullptr);
    carla_debug("CarlaEngine::getPatchbayConnections()");

    return pData->graph.graph->getConnections();
}

void CarlaEngine::restorePatchbayConnection(const char* const connSource, const char* const connTarget)
{
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady,);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.graph != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(connSource != nullptr && connSource[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(connTarget != nullptr && connTarget[0] != '\0',);
    carla_debug("CarlaEngine::restorePatchbayConnection(\"%s\", \"%s\")", connSource, connTarget);

    uint groupA, portA;
    uint groupB, portB;

    if (! pData->graph.graph->getPortIdFromFullName(connSource, groupA, portA))
        return;
    if (! pData->graph.graph->getPortIdFromFullName(connTarget, groupB, portB))
        return;

    patchbayConnect(groupA, portA, groupB, portB);
}


void CarlaEngine::ProtectedData::processRack(const float* inBufReal[2], float* outBuf[2], const uint32_t frames, const bool isOffline)
{
    CARLA_SAFE_ASSERT_RETURN(events.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(events.out != nullptr,);

#if 0
    // safe copy
    float inBuf0[frames];
    float inBuf1[frames];
    float* inBuf[2] = { inBuf0, inBuf1 };

    // initialize audio inputs
    FloatVectorOperations::copy(inBuf0, inBufReal[0], frames);
    FloatVectorOperations::copy(inBuf1, inBufReal[1], frames);

    // initialize audio outputs (zero)
    FloatVectorOperations::clear(outBuf[0], frames);
    FloatVectorOperations::clear(outBuf[1], frames);

    // initialize event outputs (zero)
    carla_zeroStruct<EngineEvent>(events.out, kMaxEngineEventInternalCount);

    bool processed = false;

    uint32_t oldAudioInCount = 0;
    uint32_t oldMidiOutCount = 0;

    // process plugins
    for (uint i=0; i < curPluginCount; ++i)
    {
        CarlaPlugin* const plugin = plugins[i].plugin;

        if (plugin == nullptr || ! plugin->isEnabled() || ! plugin->tryLock(isOffline))
            continue;

        if (processed)
        {
            // initialize audio inputs (from previous outputs)
            FloatVectorOperations::copy(inBuf0, outBuf[0], frames);
            FloatVectorOperations::copy(inBuf1, outBuf[1], frames);

            // initialize audio outputs (zero)
            FloatVectorOperations::clear(outBuf[0], frames);
            FloatVectorOperations::clear(outBuf[1], frames);

            // if plugin has no midi out, add previous events
            if (oldMidiOutCount == 0 && events.in[0].type != kEngineEventTypeNull)
            {
                if (events.out[0].type != kEngineEventTypeNull)
                {
                    // TODO: carefully add to input, sorted events
                }
                // else nothing needed
            }
            else
            {
                // initialize event inputs from previous outputs
                carla_copyStruct<EngineEvent>(events.in, events.out, kMaxEngineEventInternalCount);

                // initialize event outputs (zero)
                carla_zeroStruct<EngineEvent>(events.out, kMaxEngineEventInternalCount);
            }
        }

        oldAudioInCount = plugin->getAudioInCount();
        oldMidiOutCount = plugin->getMidiOutCount();

        // process
        plugin->initBuffers();
        plugin->process(inBuf, outBuf, frames);
        plugin->unlock();

        // if plugin has no audio inputs, add input buffer
        if (oldAudioInCount == 0)
        {
            FloatVectorOperations::add(outBuf[0], inBuf0, frames);
            FloatVectorOperations::add(outBuf[1], inBuf1, frames);
        }

        // set peaks
        {
            EnginePluginData& pluginData(plugins[i]);

            juce::Range<float> range;

            if (oldAudioInCount > 0)
            {
                range = FloatVectorOperations::findMinAndMax(inBuf0, static_cast<int>(frames));
                pluginData.insPeak[0] = carla_max<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);

                range = FloatVectorOperations::findMinAndMax(inBuf1, static_cast<int>(frames));
                pluginData.insPeak[1] = carla_max<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);
            }
            else
            {
                pluginData.insPeak[0] = 0.0f;
                pluginData.insPeak[1] = 0.0f;
            }

            if (plugin->getAudioOutCount() > 0)
            {
                range = FloatVectorOperations::findMinAndMax(outBuf[0], static_cast<int>(frames));
                pluginData.outsPeak[0] = carla_max<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);

                range = FloatVectorOperations::findMinAndMax(outBuf[1], static_cast<int>(frames));
                pluginData.outsPeak[1] = carla_max<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);
            }
            else
            {
                pluginData.outsPeak[0] = 0.0f;
                pluginData.outsPeak[1] = 0.0f;
            }
        }

        processed = true;
    }
#endif
}

void CarlaEngine::ProtectedData::processRackFull(const float* const* const inBuf, const uint32_t inCount, float* const* const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline)
{
#if 0
    RackGraph::Audio& rackAudio(graph.rack->audio);

    const CarlaMutexLocker _cml(rackAudio.mutex);

    if (inBuf != nullptr && inCount > 0)
    {
        bool noConnections = true;

        // connect input buffers
        for (LinkedList<uint>::Itenerator it = rackAudio.connectedIn1.begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < inCount);

            if (noConnections)
            {
                FloatVectorOperations::copy(audio.inBuf[0], inBuf[port], nframes);
                noConnections = false;
            }
            else
            {
                FloatVectorOperations::add(audio.inBuf[0], inBuf[port], nframes);
            }
        }

        if (noConnections)
            FloatVectorOperations::clear(audio.inBuf[0], nframes);

        noConnections = true;

        for (LinkedList<uint>::Itenerator it = rackAudio.connectedIn2.begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < inCount);

            if (noConnections)
            {
                FloatVectorOperations::copy(audio.inBuf[1], inBuf[port], nframes);
                noConnections = false;
            }
            else
            {
                FloatVectorOperations::add(audio.inBuf[1], inBuf[port], nframes);
            }
        }

        if (noConnections)
            FloatVectorOperations::clear(audio.inBuf[1], nframes);
    }
    else
    {
        FloatVectorOperations::clear(audio.inBuf[0], nframes);
        FloatVectorOperations::clear(audio.inBuf[1], nframes);
    }

    FloatVectorOperations::clear(audio.outBuf[0], nframes);
    FloatVectorOperations::clear(audio.outBuf[1], nframes);

    // process
    processRack(const_cast<const float**>(audio.inBuf), audio.outBuf, nframes, isOffline);

    // connect output buffers
    if (rackAudio.connectedOut1.count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = rackAudio.connectedOut1.begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < outCount);

            FloatVectorOperations::add(outBuf[port], audio.outBuf[0], nframes);
        }
    }

    if (rackAudio.connectedOut2.count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = rackAudio.connectedOut2.begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < outCount);

            FloatVectorOperations::add(outBuf[port], audio.outBuf[1], nframes);
        }
    }
#endif
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
