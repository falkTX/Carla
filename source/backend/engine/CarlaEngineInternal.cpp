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

static
int getCarlaRackPortIdFromName(const char* const shortname) noexcept
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
    return RACK_GRAPH_CARLA_PORT_NULL;
}

// -----------------------------------------------------------------------
// RackGraph

bool RackGraph::connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int portB) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(groupA != groupB, false);
    CARLA_SAFE_ASSERT_RETURN(groupA >= RACK_GRAPH_GROUP_CARLA && groupA < RACK_GRAPH_GROUP_MAX, false);
    CARLA_SAFE_ASSERT_RETURN(groupB >= RACK_GRAPH_GROUP_CARLA && groupB < RACK_GRAPH_GROUP_MAX, false);
    CARLA_SAFE_ASSERT_RETURN(portA >= 0, false);
    CARLA_SAFE_ASSERT_RETURN(portB >= 0, false);

    int carlaPort, otherPort;

    if (groupA == RACK_GRAPH_GROUP_CARLA)
    {
        carlaPort = portA;
        otherPort = portB;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(groupB == RACK_GRAPH_GROUP_CARLA, false);

        carlaPort = portB;
        otherPort = portA;
    }

    bool makeConnection = false;

    switch (carlaPort)
    {
    case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
        connectLock.enter();
        connectedIn1.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
        connectLock.enter();
        connectedIn2.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
        connectLock.enter();
        connectedOut1.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
        connectLock.enter();
        connectedOut2.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_GRAPH_CARLA_PORT_MIDI_IN:
        makeConnection = engine->connectRackMidiInPort(otherPort);
        break;

    case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
        makeConnection = engine->connectRackMidiOutPort(otherPort);
        break;
    }

    if (! makeConnection)
    {
        engine->setLastError("Invalid rack connection");
        return false;
    }

    ConnectionToId connectionToId;
    connectionToId.id     = lastConnectionId;
    connectionToId.groupA = groupA;
    connectionToId.portA  = portA;
    connectionToId.groupB = groupB;
    connectionToId.portB  = portB;

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';
    std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", groupA, portA, groupB, portB);

    engine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, lastConnectionId, 0, 0, 0.0f, strBuf);

    usedConnections.append(connectionToId);
    ++lastConnectionId;

    return true;
}

bool RackGraph::disconnect(CarlaEngine* const engine, const uint connectionId) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(usedConnections.count() > 0, false);

    for (LinkedList<ConnectionToId>::Itenerator it=usedConnections.begin(); it.valid(); it.next())
    {
        const ConnectionToId& connection(it.getValue());

        if (connection.id != connectionId)
            continue;

        int carlaPort, otherPort;

        if (connection.groupA == RACK_GRAPH_GROUP_CARLA)
        {
            carlaPort = connection.portA;
            otherPort = connection.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(connection.groupB == RACK_GRAPH_GROUP_CARLA, false);

            carlaPort = connection.portB;
            otherPort = connection.portA;
        }

        switch (carlaPort)
        {
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
            connectLock.enter();
            connectedIn1.removeAll(otherPort);
            connectLock.leave();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
            connectLock.enter();
            connectedIn2.removeAll(otherPort);
            connectLock.leave();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
            connectLock.enter();
            connectedOut1.removeAll(otherPort);
            connectLock.leave();
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
            connectLock.enter();
            connectedOut2.removeAll(otherPort);
            connectLock.leave();
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_IN:
            engine->disconnectRackMidiInPort(otherPort);
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
            engine->disconnectRackMidiOutPort(otherPort);
            break;

        default:
            engine->setLastError("Invalid connection");
            return false;
        }

        engine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connection.id, 0, 0, 0.0f, nullptr);

        usedConnections.remove(it);
        return true;
    }

    engine->setLastError("Failed to find connection");
    return false;
}

const char* const* RackGraph::getConnections() const
{
    if (usedConnections.count() == 0)
        return nullptr;

    LinkedList<const char*> connList;
    char strBuf[STR_MAX+1];

    for (LinkedList<ConnectionToId>::Itenerator it=usedConnections.begin(); it.valid(); it.next())
    {
        const ConnectionToId& connection(it.getValue());

        CARLA_SAFE_ASSERT_CONTINUE(connection.groupA != connection.groupB);
        CARLA_SAFE_ASSERT_CONTINUE(connection.groupA >= RACK_GRAPH_GROUP_CARLA && connection.groupA < RACK_GRAPH_GROUP_MAX);
        CARLA_SAFE_ASSERT_CONTINUE(connection.groupB >= RACK_GRAPH_GROUP_CARLA && connection.groupB < RACK_GRAPH_GROUP_MAX);
        CARLA_SAFE_ASSERT_CONTINUE(connection.portA >= 0);
        CARLA_SAFE_ASSERT_CONTINUE(connection.portB >= 0);

        int carlaPort, otherPort;

        if (connection.groupA == RACK_GRAPH_GROUP_CARLA)
        {
            carlaPort = connection.portA;
            otherPort = connection.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_CONTINUE(connection.groupB == RACK_GRAPH_GROUP_CARLA);

            carlaPort = connection.portB;
            otherPort = connection.portA;
        }

        switch (carlaPort)
        {
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN1:
        case RACK_GRAPH_CARLA_PORT_AUDIO_IN2:
            std::sprintf(strBuf, "AudioIn:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup((carlaPort == RACK_GRAPH_CARLA_PORT_AUDIO_IN1) ? "Carla:AudioIn1" : "Carla:AudioIn2"));
            break;

        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT1:
        case RACK_GRAPH_CARLA_PORT_AUDIO_OUT2:
            connList.append(carla_strdup((carlaPort == RACK_GRAPH_CARLA_PORT_AUDIO_OUT1) ? "Carla:AudioOut1" : "Carla:AudioOut2"));
            std::sprintf(strBuf, "AudioOut:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_IN:
            std::sprintf(strBuf, "MidiIn:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup("Carla:MidiIn"));
            break;

        case RACK_GRAPH_CARLA_PORT_MIDI_OUT:
            connList.append(carla_strdup("Carla:MidiOut"));
            std::sprintf(strBuf, "MidiOut:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            break;
        }
    }

    const size_t connCount(connList.count());

    if (connCount == 0)
        return nullptr;

    const char** const retConns = new const char*[connCount+1];

    for (size_t i=0; i < connCount; ++i)
        retConns[i] = connList.getAt(i);

    retConns[connCount] = nullptr;
    connList.clear();

    return retConns;
}

bool RackGraph::getPortIdFromName(const char* const portName, int& groupId, int& portId)
{
    if (std::strncmp(portName, "Carla:", 6) == 0)
    {
        groupId = RACK_GRAPH_GROUP_CARLA;
        portId  = getCarlaRackPortIdFromName(portName+6);
    }
    else if (std::strncmp(portName, "AudioIn:", 8) == 0)
    {
        groupId = RACK_GRAPH_GROUP_AUDIO_IN;
        portId  = std::atoi(portName+8) - 1;
    }
    else if (std::strncmp(portName, "AudioOut:", 9) == 0)
    {
        groupId = RACK_GRAPH_GROUP_AUDIO_OUT;
        portId  = std::atoi(portName+9) - 1;
    }
    else if (std::strncmp(portName, "MidiIn:", 7) == 0)
    {
        groupId = RACK_GRAPH_GROUP_MIDI_IN;
        portId  = std::atoi(portName+7) - 1;
    }
    else if (std::strncmp(portName, "MidiOut:", 8) == 0)
    {
        groupId = RACK_GRAPH_GROUP_MIDI_OUT;
        portId  = std::atoi(portName+8) - 1;
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

bool PatchbayGraph::connect(CarlaEngine* const engine, const int /*groupA*/, const int /*portA*/, const int /*groupB*/, const int /*portB*/) noexcept
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

bool PatchbayGraph::getPortIdFromName(const char* const /*portName*/, int& /*groupId*/, int& /*portId*/)
{
    return false;
}
#endif

// -----------------------------------------------------------------------
// CarlaEngineProtectedData

CarlaEngineProtectedData::CarlaEngineProtectedData(CarlaEngine* const engine) noexcept
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

CarlaEngineProtectedData::~CarlaEngineProtectedData() noexcept
{
    CARLA_SAFE_ASSERT(curPluginCount == 0);
    CARLA_SAFE_ASSERT(maxPluginNumber == 0);
    CARLA_SAFE_ASSERT(nextPluginId == 0);
    CARLA_SAFE_ASSERT(plugins == nullptr);
}

// -----------------------------------------------------------------------

void CarlaEngineProtectedData::doPluginRemove() noexcept
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

void CarlaEngineProtectedData::doPluginsSwitch() noexcept
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

void CarlaEngineProtectedData::doNextPluginAction(const bool unlock) noexcept
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
        nextAction.mutex.unlock();
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
void CarlaEngineProtectedData::processRack(const float* inBufReal[2], float* outBuf[2], const uint32_t frames, const bool isOffline)
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

void CarlaEngineProtectedData::processRackFull(const float* const* const inBuf, const uint32_t inCount, float* const* const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline)
{
    const CarlaCriticalSectionScope _cs(graph.rack->connectLock);

    if (inBuf != nullptr && inCount > 0)
    {
        bool noConnections = true;

        // connect input buffers
        for (LinkedList<int>::Itenerator it = graph.rack->connectedIn1.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(inCount));

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

        for (LinkedList<int>::Itenerator it = graph.rack->connectedIn2.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(inCount));

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
    if (graph.rack->connectedOut1.count() != 0)
    {
        for (LinkedList<int>::Itenerator it = graph.rack->connectedOut1.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(outCount));

            FLOAT_ADD(outBuf[port], audio.outBuf[0], nframes);
        }
    }

    if (graph.rack->connectedOut2.count() != 0)
    {
        for (LinkedList<int>::Itenerator it = graph.rack->connectedOut2.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(outCount));

            FLOAT_ADD(outBuf[port], audio.outBuf[1], nframes);
        }
    }
}
#endif

// -----------------------------------------------------------------------
// ScopedActionLock

ScopedActionLock::ScopedActionLock(CarlaEngineProtectedData* const data, const EnginePostAction action, const uint pluginId, const uint value, const bool lockWait) noexcept
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
