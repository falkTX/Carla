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

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"
#include "CarlaMIDI.h"

#include "CarlaMathUtils.hpp"

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// Rack Patchbay stuff

static uint getCarlaRackPortIdFromName(const char* const shortname) noexcept
{
    if (std::strcmp(shortname, "AudioIn1") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_IN1;
    if (std::strcmp(shortname, "AudioIn2") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_IN2;
    if (std::strcmp(shortname, "AudioOut1") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_OUT1;
    if (std::strcmp(shortname, "AudioOut2") == 0)
        return RACK_GRAPH_CARLA_PORT_AUDIO_OUT2;
    if (std::strcmp(shortname, "MidiIn") == 0)
        return RACK_GRAPH_CARLA_PORT_MIDI_IN;
    if (std::strcmp(shortname, "MidiOut") == 0)
        return RACK_GRAPH_CARLA_PORT_MIDI_OUT;

    carla_stderr("CarlaBackend::getCarlaRackPortIdFromName(%s) - invalid short name", shortname);
    return RACK_GRAPH_CARLA_PORT_NULL;
}

// -----------------------------------------------------------------------
// RackGraph

const char* RackGraph::MIDI::getName(const bool isInput, const uint index) const noexcept
{
    for (LinkedList<PortNameToId>::Itenerator it = isInput ? ins.begin() : outs.begin(); it.valid(); it.next())
    {
        const PortNameToId& port(it.getValue());

        if (port.port == index)
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

bool RackGraph::connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);

    uint carlaPort;
    GroupPort otherPort;

    if (groupA == RACK_GRAPH_GROUP_CARLA)
    {
        CARLA_SAFE_ASSERT_RETURN(groupB != RACK_GRAPH_GROUP_CARLA, false);

        carlaPort       = portA;
        otherPort.group = groupB;
        otherPort.port  = portB;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(groupB == RACK_GRAPH_GROUP_CARLA, false);

        carlaPort       = portB;
        otherPort.group = groupA;
        otherPort.port  = portA;
    }

    CARLA_SAFE_ASSERT_RETURN(carlaPort > RACK_GRAPH_CARLA_PORT_NULL && carlaPort < RACK_GRAPH_CARLA_PORT_MAX, false);
    CARLA_SAFE_ASSERT_RETURN(otherPort.group > RACK_GRAPH_GROUP_CARLA && otherPort.group < RACK_GRAPH_GROUP_MAX, false);

    bool makeConnection = false;

    switch (carlaPort)
    {
    case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
        CARLA_SAFE_ASSERT_RETURN(otherPort.group == RACK_GRAPH_GROUP_AUDIO_OUT, false);
        audio.mutex.lock();
        makeConnection = audio.connectedIn1.append(otherPort.port);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
        CARLA_SAFE_ASSERT_RETURN(otherPort.group == RACK_GRAPH_GROUP_AUDIO_OUT, false);
        audio.mutex.lock();
        makeConnection = audio.connectedIn2.append(otherPort.port);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
        CARLA_SAFE_ASSERT_RETURN(otherPort.group == RACK_GRAPH_GROUP_AUDIO_IN, false);
        audio.mutex.lock();
        makeConnection = audio.connectedOut1.append(otherPort.port);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
        CARLA_SAFE_ASSERT_RETURN(otherPort.group == RACK_GRAPH_GROUP_AUDIO_IN, false);
        audio.mutex.lock();
        makeConnection = audio.connectedOut2.append(otherPort.port);
        audio.mutex.unlock();
        break;

    case RACK_GRAPH_CARLA_PORT_MIDI_IN:
        CARLA_SAFE_ASSERT_RETURN(otherPort.group == RACK_GRAPH_GROUP_MIDI_OUT, false);
        if (const char* const portName = midi.getName(true, otherPort.port))
            makeConnection = engine->connectRackMidiInPort(portName);
        break;

    case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
        CARLA_SAFE_ASSERT_RETURN(otherPort.group == RACK_GRAPH_GROUP_MIDI_IN, false);
        if (const char* const portName = midi.getName(false, otherPort.port))
            makeConnection = engine->connectRackMidiOutPort(portName);
        break;
    }

    if (! makeConnection)
    {
        engine->setLastError("Invalid rack connection");
        return false;
    }

    ConnectionToId connectionToId = { ++connections.lastId, groupA, portA, groupB, portB };

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

        uint carlaPort;
        GroupPort otherPort;

        if (connection.groupA == RACK_GRAPH_GROUP_CARLA)
        {
            CARLA_SAFE_ASSERT_RETURN(connection.groupB != RACK_GRAPH_GROUP_CARLA, false);

            carlaPort       = connection.portA;
            otherPort.group = connection.groupB;
            otherPort.port  = connection.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(connection.groupB == RACK_GRAPH_GROUP_CARLA, false);

            carlaPort       = connection.portB;
            otherPort.group = connection.groupA;
            otherPort.port  = connection.portA;
        }

        CARLA_SAFE_ASSERT_RETURN(carlaPort > RACK_GRAPH_CARLA_PORT_NULL && carlaPort < RACK_GRAPH_CARLA_PORT_MAX, false);
        CARLA_SAFE_ASSERT_RETURN(otherPort.group > RACK_GRAPH_GROUP_CARLA && otherPort.group < RACK_GRAPH_GROUP_MAX, false);

        bool makeDisconnection = false;

        switch (carlaPort)
        {
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
            audio.mutex.lock();
            makeDisconnection = audio.connectedIn1.removeOne(otherPort.port);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
            audio.mutex.lock();
            makeDisconnection = audio.connectedIn2.removeOne(otherPort.port);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
            audio.mutex.lock();
            makeDisconnection = audio.connectedOut1.removeOne(otherPort.port);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
            audio.mutex.lock();
            makeDisconnection = audio.connectedOut2.removeOne(otherPort.port);
            audio.mutex.unlock();
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_IN:
            if (const char* const portName = midi.getName(true, otherPort.port))
                makeDisconnection = engine->disconnectRackMidiInPort(portName);
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
            if (const char* const portName = midi.getName(false, otherPort.port))
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

        uint carlaPort;
        GroupPort otherPort;

        if (connection.groupA == RACK_GRAPH_GROUP_CARLA)
        {
            CARLA_SAFE_ASSERT_CONTINUE(connection.groupB != RACK_GRAPH_GROUP_CARLA);

            carlaPort       = connection.portA;
            otherPort.group = connection.groupB;
            otherPort.port  = connection.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_CONTINUE(connection.groupB == RACK_GRAPH_GROUP_CARLA);

            carlaPort       = connection.portB;
            otherPort.group = connection.groupA;
            otherPort.port  = connection.portA;
        }

        CARLA_SAFE_ASSERT_CONTINUE(carlaPort > RACK_GRAPH_CARLA_PORT_NULL && carlaPort < RACK_GRAPH_CARLA_PORT_MAX);
        CARLA_SAFE_ASSERT_CONTINUE(otherPort.group > RACK_GRAPH_GROUP_CARLA && otherPort.group < RACK_GRAPH_GROUP_MAX);

        switch (carlaPort)
        {
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
            std::snprintf(strBuf, STR_MAX, "AudioIn:%i", otherPort.port+1);
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup((carlaPort == RACK_GRAPH_CARLA_PORT_AUDIO_IN1) ? "Carla:AudioIn1" : "Carla:AudioIn2"));
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
            std::snprintf(strBuf, STR_MAX, "AudioOut:%i", otherPort.port+1);
            connList.append(carla_strdup((carlaPort == RACK_GRAPH_CARLA_PORT_AUDIO_OUT1) ? "Carla:AudioOut1" : "Carla:AudioOut2"));
            connList.append(carla_strdup(strBuf));
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_IN:
            std::snprintf(strBuf, STR_MAX, "MidiIn:%s", midi.getName(true, otherPort.port));
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup("Carla:MidiIn"));
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
            std::snprintf(strBuf, STR_MAX, "MidiOut:%s", midi.getName(false, otherPort.port));
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
#endif

// -----------------------------------------------------------------------
// CarlaEngine::ProtectedData

CarlaEngine::ProtectedData::ProtectedData(CarlaEngine* const engine) noexcept
    : osc(engine),
      thread(engine),
      oscData(nullptr),
      callback(nullptr),
      callbackPtr(nullptr),
      fileCallback(nullptr),
      fileCallbackPtr(nullptr),
      hints(0x0),
      bufferSize(0),
      sampleRate(0.0),
      aboutToClose(false),
      curPluginCount(0),
      maxPluginNumber(0),
      nextPluginId(0),
      plugins(nullptr) {}

CarlaEngine::ProtectedData::~ProtectedData() noexcept
{
    CARLA_SAFE_ASSERT(curPluginCount == 0);
    CARLA_SAFE_ASSERT(maxPluginNumber == 0);
    CARLA_SAFE_ASSERT(nextPluginId == 0);
    CARLA_SAFE_ASSERT(plugins == nullptr);
}

// -----------------------------------------------------------------------

void CarlaEngine::ProtectedData::doPluginRemove() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(curPluginCount > 0,);
    CARLA_SAFE_ASSERT_RETURN(nextAction.pluginId < curPluginCount,);
    --curPluginCount;

    // move all plugins 1 spot backwards
    for (uint i=nextAction.pluginId; i < curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(plugins[i+1].plugin);

        CARLA_SAFE_ASSERT_BREAK(plugin != nullptr);

        plugin->setId(i);

        plugins[i].plugin      = plugin;
        plugins[i].insPeak[0]  = 0.0f;
        plugins[i].insPeak[1]  = 0.0f;
        plugins[i].outsPeak[0] = 0.0f;
        plugins[i].outsPeak[1] = 0.0f;
    }

    const uint id(curPluginCount);

    // reset last plugin (now removed)
    plugins[id].plugin      = nullptr;
    plugins[id].insPeak[0]  = 0.0f;
    plugins[id].insPeak[1]  = 0.0f;
    plugins[id].outsPeak[0] = 0.0f;
    plugins[id].outsPeak[1] = 0.0f;
}

void CarlaEngine::ProtectedData::doPluginsSwitch() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(curPluginCount >= 2,);

    const uint idA(nextAction.pluginId);
    const uint idB(nextAction.value);

    CARLA_SAFE_ASSERT_RETURN(idA < curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(idB < curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(plugins[idA].plugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugins[idB].plugin != nullptr,);

#if 0
    std::swap(plugins[idA].plugin, plugins[idB].plugin);
#else
    CarlaPlugin* const tmp(plugins[idA].plugin);

    plugins[idA].plugin = plugins[idB].plugin;
    plugins[idB].plugin = tmp;
#endif
}

void CarlaEngine::ProtectedData::doNextPluginAction(const bool unlock) noexcept
{
    switch (nextAction.opcode)
    {
    case kEnginePostActionNull:
        break;
    case kEnginePostActionZeroCount:
        curPluginCount = 0;
        break;
    case kEnginePostActionRemovePlugin:
        doPluginRemove();
        break;
    case kEnginePostActionSwitchPlugins:
        doPluginsSwitch();
        break;
    }

    nextAction.opcode   = kEnginePostActionNull;
    nextAction.pluginId = 0;
    nextAction.value    = 0;

    if (unlock)
    {
        nextAction.mutex.tryLock();
        nextAction.mutex.unlock();
    }
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
void CarlaEngine::ProtectedData::processRack(const float* inBufReal[2], float* outBuf[2], const uint32_t frames, const bool isOffline)
{
    CARLA_SAFE_ASSERT_RETURN(events.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(events.out != nullptr,);

    // safe copy
    float inBuf0[frames];
    float inBuf1[frames];
    float* inBuf[2] = { inBuf0, inBuf1 };

    // initialize audio inputs
    FLOAT_COPY(inBuf0, inBufReal[0], frames);
    FLOAT_COPY(inBuf1, inBufReal[1], frames);

    // initialize audio outputs (zero)
    FLOAT_CLEAR(outBuf[0], frames);
    FLOAT_CLEAR(outBuf[1], frames);

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
            FLOAT_COPY(inBuf0, outBuf[0], frames);
            FLOAT_COPY(inBuf1, outBuf[1], frames);

            // initialize audio outputs (zero)
            FLOAT_CLEAR(outBuf[0], frames);
            FLOAT_CLEAR(outBuf[1], frames);

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
            FLOAT_ADD(outBuf[0], inBuf0, frames);
            FLOAT_ADD(outBuf[1], inBuf1, frames);
        }

        // set peaks
        {
            EnginePluginData& pluginData(plugins[i]);

#ifdef HAVE_JUCE
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
#else
            float peak1, peak2;

            if (oldAudioInCount > 0)
            {
                peak1 = peak2 = 0.0f;

                for (uint32_t k=0; k < frames; ++k)
                {
                    peak1  = carla_max<float>(peak1, std::fabs(inBuf0[k]), 1.0f);
                    peak2  = carla_max<float>(peak2, std::fabs(inBuf1[k]), 1.0f);
                }

                pluginData.insPeak[0] = peak1;
                pluginData.insPeak[1] = peak2;
            }
            else
            {
                pluginData.insPeak[0] = 0.0f;
                pluginData.insPeak[1] = 0.0f;
            }

            if (plugin->getAudioOutCount() > 0)
            {
                peak1 = peak2 = 0.0f;

                for (uint32_t k=0; k < frames; ++k)
                {
                    peak1 = carla_max<float>(peak1, std::fabs(outBuf[0][k]), 1.0f);
                    peak2 = carla_max<float>(peak2, std::fabs(outBuf[1][k]), 1.0f);
                }

                pluginData.outsPeak[0] = peak1;
                pluginData.outsPeak[1] = peak2;
            }
            else
            {
                pluginData.outsPeak[0] = 0.0f;
                pluginData.outsPeak[1] = 0.0f;
            }
#endif
        }

        processed = true;
    }
}

void CarlaEngine::ProtectedData::processRackFull(const float* const* const inBuf, const uint32_t inCount, float* const* const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline)
{
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
                FLOAT_COPY(audio.inBuf[0], inBuf[port], nframes);
                noConnections = false;
            }
            else
            {
                FLOAT_ADD(audio.inBuf[0], inBuf[port], nframes);
            }
        }

        if (noConnections)
            FLOAT_CLEAR(audio.inBuf[0], nframes);

        noConnections = true;

        for (LinkedList<uint>::Itenerator it = rackAudio.connectedIn2.begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < inCount);

            if (noConnections)
            {
                FLOAT_COPY(audio.inBuf[1], inBuf[port], nframes);
                noConnections = false;
            }
            else
            {
                FLOAT_ADD(audio.inBuf[1], inBuf[port], nframes);
            }
        }

        if (noConnections)
            FLOAT_CLEAR(audio.inBuf[1], nframes);
    }
    else
    {
        FLOAT_CLEAR(audio.inBuf[0], nframes);
        FLOAT_CLEAR(audio.inBuf[1], nframes);
    }

    FLOAT_CLEAR(audio.outBuf[0], nframes);
    FLOAT_CLEAR(audio.outBuf[1], nframes);

    // process
    processRack(const_cast<const float**>(audio.inBuf), audio.outBuf, nframes, isOffline);

    // connect output buffers
    if (rackAudio.connectedOut1.count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = rackAudio.connectedOut1.begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < outCount);

            FLOAT_ADD(outBuf[port], audio.outBuf[0], nframes);
        }
    }

    if (rackAudio.connectedOut2.count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = rackAudio.connectedOut2.begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < outCount);

            FLOAT_ADD(outBuf[port], audio.outBuf[1], nframes);
        }
    }
}
#endif

// -----------------------------------------------------------------------
// ScopedActionLock

ScopedActionLock::ScopedActionLock(CarlaEngine::ProtectedData* const data, const EnginePostAction action, const uint pluginId, const uint value, const bool lockWait) noexcept
    : fData(data)
{
    fData->nextAction.mutex.lock();

    CARLA_SAFE_ASSERT_RETURN(fData->nextAction.opcode == kEnginePostActionNull,);

    fData->nextAction.opcode   = action;
    fData->nextAction.pluginId = pluginId;
    fData->nextAction.value    = value;

    if (lockWait)
    {
        // block wait for unlock on processing side
        carla_stdout("ScopedPluginAction(%i) - blocking START", pluginId);
        fData->nextAction.mutex.lock();
        carla_stdout("ScopedPluginAction(%i) - blocking DONE", pluginId);
    }
    else
    {
        fData->doNextPluginAction(false);
    }
}

ScopedActionLock::~ScopedActionLock() noexcept
{
    fData->nextAction.mutex.unlock();
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
