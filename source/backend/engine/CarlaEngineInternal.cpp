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

// -----------------------------------------------------------------------
// Rack Patchbay stuff

enum RackPatchbayGroupIds {
    RACK_PATCHBAY_GROUP_CARLA = 0,
    RACK_PATCHBAY_GROUP_AUDIO = 1,
    RACK_PATCHBAY_GROUP_MIDI  = 2,
    RACK_PATCHBAY_GROUP_MAX   = 3
};

enum RackPatchbayCarlaPortIds {
    RACK_PATCHBAY_CARLA_PORT_NULL       = 0,
    RACK_PATCHBAY_CARLA_PORT_AUDIO_IN1  = 1,
    RACK_PATCHBAY_CARLA_PORT_AUDIO_IN2  = 2,
    RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT1 = 3,
    RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT2 = 4,
    RACK_PATCHBAY_CARLA_PORT_MIDI_IN    = 5,
    RACK_PATCHBAY_CARLA_PORT_MIDI_OUT   = 6,
    RACK_PATCHBAY_CARLA_PORT_MAX        = 7
};

static inline
int getCarlaRackPortIdFromName(const char* const shortname) noexcept
{
    if (std::strcmp(shortname, "AudioIn1") == 0)
        return RACK_PATCHBAY_CARLA_PORT_AUDIO_IN1;
    if (std::strcmp(shortname, "AudioIn2") == 0)
        return RACK_PATCHBAY_CARLA_PORT_AUDIO_IN2;
    if (std::strcmp(shortname, "AudioOut1") == 0)
        return RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT1;
    if (std::strcmp(shortname, "AudioOut2") == 0)
        return RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT2;
    if (std::strcmp(shortname, "MidiIn") == 0)
        return RACK_PATCHBAY_CARLA_PORT_MIDI_IN;
    if (std::strcmp(shortname, "MidiOut") == 0)
        return RACK_PATCHBAY_CARLA_PORT_MIDI_OUT;
    return RACK_PATCHBAY_CARLA_PORT_NULL;
}

// -----------------------------------------------------------------------
// RackGraph

bool RackGraph::connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int portB) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(groupA != groupB, false);
    CARLA_SAFE_ASSERT_RETURN(groupA >= RACK_PATCHBAY_GROUP_CARLA && groupA < RACK_PATCHBAY_GROUP_MAX, false);
    CARLA_SAFE_ASSERT_RETURN(groupB >= RACK_PATCHBAY_GROUP_CARLA && groupB < RACK_PATCHBAY_GROUP_MAX, false);
    CARLA_SAFE_ASSERT_RETURN(portA >= 0, false);
    CARLA_SAFE_ASSERT_RETURN(portB >= 0, false);

    int carlaPort, otherPort;

    if (groupA == RACK_PATCHBAY_GROUP_CARLA)
    {
        carlaPort = portA;
        otherPort = portB;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(groupB == RACK_PATCHBAY_GROUP_CARLA, false);

        carlaPort = portB;
        otherPort = portA;
    }

    bool makeConnection = false;

    switch (carlaPort)
    {
    case RACK_PATCHBAY_CARLA_PORT_AUDIO_IN1:
        connectLock.enter();
        connectedIn1.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_CARLA_PORT_AUDIO_IN2:
        connectLock.enter();
        connectedIn2.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT1:
        connectLock.enter();
        connectedOut1.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT2:
        connectLock.enter();
        connectedOut2.append(otherPort);
        connectLock.leave();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_CARLA_PORT_MIDI_IN:
        makeConnection = engine->connectRackMidiInPort(otherPort);
        break;

    case RACK_PATCHBAY_CARLA_PORT_MIDI_OUT:
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

        if (connection.id == connectionId)
        {
#if 0
            const int otherPort((connection.portOut >= 0) ? connection.portOut : connection.portIn);
            const int carlaPort((otherPort == connection.portOut) ? connection.portIn : connection.portOut);

            if (otherPort >= RACK_PATCHBAY_GROUP_MIDI_OUT*1000)
            {
                CARLA_SAFE_ASSERT_RETURN(carlaPort == RACK_PATCHBAY_PORT_MIDI_IN, false);

                const int portId(otherPort-RACK_PATCHBAY_GROUP_MIDI_OUT*1000);
                disconnectRackMidiInPort(portId);
            }
            else if (otherPort >= RACK_PATCHBAY_GROUP_MIDI_IN*1000)
            {
                CARLA_SAFE_ASSERT_RETURN(carlaPort == RACK_PATCHBAY_PORT_MIDI_OUT, false);

                const int portId(otherPort-RACK_PATCHBAY_GROUP_MIDI_IN*1000);
                disconnectRackMidiOutPort(portId);
            }
            else if (otherPort >= RACK_PATCHBAY_GROUP_AUDIO_OUT*1000)
            {
                CARLA_SAFE_ASSERT_RETURN(carlaPort == RACK_PATCHBAY_PORT_AUDIO_OUT1 || carlaPort == RACK_PATCHBAY_PORT_AUDIO_OUT2, false);

                const int portId(otherPort-RACK_PATCHBAY_GROUP_AUDIO_OUT*1000);

                connectLock.enter();

                if (carlaPort == RACK_PATCHBAY_PORT_AUDIO_OUT1)
                    connectedOut1.removeAll(portId);
                else
                    connectedOut2.removeAll(portId);

                connectLock.leave();
            }
            else if (otherPort >= RACK_PATCHBAY_GROUP_AUDIO_IN*1000)
            {
                CARLA_SAFE_ASSERT_RETURN(carlaPort == RACK_PATCHBAY_PORT_AUDIO_IN1 || carlaPort == RACK_PATCHBAY_PORT_AUDIO_IN2, false);

                const int portId(otherPort-RACK_PATCHBAY_GROUP_AUDIO_IN*1000);

                connectLock.enter();

                if (carlaPort == RACK_PATCHBAY_PORT_AUDIO_IN1)
                    connectedIn1.removeAll(portId);
                else
                    connectedIn2.removeAll(portId);

                connectLock.leave();
            }
            else
            {
                CARLA_SAFE_ASSERT_RETURN(false, false);
            }
#endif

            engine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connection.id, 0, 0, 0.0f, nullptr);

            usedConnections.remove(it);
            return true;
        }
    }

    engine->setLastError("Failed to find connection");
    return false;
}

void RackGraph::refresh(CarlaEngine* const engine, const LinkedList<PortNameToId>& /*midiIns*/, const LinkedList<PortNameToId>& /*midiOuts*/) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);

#if 0
    CARLA_SAFE_ASSERT_RETURN(pData->audio.isReady, false);

    fUsedMidiIns.clear();
    fUsedMidiOuts.clear();

    pData->audio.initPatchbay();

    if (! pData->audio.isRack)
    {
        // not implemented yet
        return false;
    }

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    AbstractEngineBuffer* const rack(pData->audio.buffer);

    // Main
    {
        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_CARLA, PATCHBAY_ICON_CARLA, -1, 0.0f, getName());

        callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_CARLA_PORT_AUDIO_IN1,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in1");
        callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_CARLA_PORT_AUDIO_IN2,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in2");
        callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT1, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out1");
        callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT2, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out2");
        callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_CARLA_PORT_MIDI_IN,    PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,  0.0f, "midi-in");
        callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_CARLA_PORT_MIDI_OUT,   PATCHBAY_PORT_TYPE_MIDI,                         0.0f, "midi-out");
    }

    // Audio In
    {
        if (fDeviceName.isNotEmpty())
            std::snprintf(strBuf, STR_MAX, "Capture (%s)", fDeviceName.buffer());
        else
            std::strncpy(strBuf, "Capture", STR_MAX);

        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_AUDIO, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

        for (uint i=0; i < pData->audio.inCount; ++i)
        {
            std::snprintf(strBuf, STR_MAX, "capture_%i", i+1);
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_AUDIO, static_cast<int>(i), PATCHBAY_PORT_TYPE_AUDIO, 0.0f, strBuf);
        }
    }

    // Audio Out
    {
        if (fDeviceName.isNotEmpty())
            std::snprintf(strBuf, STR_MAX, "Playback (%s)", fDeviceName.buffer());
        else
            std::strncpy(strBuf, "Playback", STR_MAX);

        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_AUDIO, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

        for (uint i=0; i < pData->audio.outCount; ++i)
        {
            std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_AUDIO, static_cast<int>(i), PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, strBuf);
        }
    }

    // MIDI In
    {
        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_MIDI, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Readable MIDI ports");

        for (uint i=0, count=fDummyMidiIn.getPortCount(); i < count; ++i)
        {
            PortNameToId portNameToId;
            portNameToId.portId = static_cast<int>(i);
            std::strncpy(portNameToId.name, fDummyMidiIn.getPortName(i).c_str(), STR_MAX);
            portNameToId.name[STR_MAX] = '\0';
            fUsedMidiIns.append(portNameToId);

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_MIDI, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI, 0.0f, portNameToId.name);
        }
    }

    // MIDI Out
    {
        callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_MIDI, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Writable MIDI ports");

        for (uint i=0, count=fDummyMidiOut.getPortCount(); i < count; ++i)
        {
            PortNameToId portNameToId;
            portNameToId.portId = static_cast<int>(i);
            std::strncpy(portNameToId.name, fDummyMidiOut.getPortName(i).c_str(), STR_MAX);
            portNameToId.name[STR_MAX] = '\0';
            fUsedMidiOuts.append(portNameToId);

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_MIDI, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, 0.0f, portNameToId.name);
        }
    }

    // Connections
    rack->connectLock.enter();

    for (LinkedList<int>::Itenerator it = rack->connectedIn1.begin(); it.valid(); it.next())
    {
        const int& port(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->audio.inCount));

        ConnectionToId connectionToId;
        connectionToId.id       = rack->lastConnectionId;
        connectionToId.groupOut = RACK_PATCHBAY_GROUP_AUDIO;
        connectionToId.portOut  = port;
        connectionToId.groupIn  = RACK_PATCHBAY_GROUP_CARLA;
        connectionToId.portIn   = RACK_PATCHBAY_CARLA_PORT_AUDIO_IN1;

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupOut, connectionToId.portOut, connectionToId.groupIn, connectionToId.portIn);
        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, 0, 0, 0.0f, strBuf);

        rack->usedConnections.append(connectionToId);
        ++rack->lastConnectionId;
    }

    for (LinkedList<int>::Itenerator it = rack->connectedIn2.begin(); it.valid(); it.next())
    {
        const int& port(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->audio.inCount));

        ConnectionToId connectionToId;
        connectionToId.id       = rack->lastConnectionId;
        connectionToId.groupOut = RACK_PATCHBAY_GROUP_AUDIO;
        connectionToId.portOut  = port;
        connectionToId.groupIn  = RACK_PATCHBAY_GROUP_CARLA;
        connectionToId.portIn   = RACK_PATCHBAY_CARLA_PORT_AUDIO_IN2;

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupOut, connectionToId.portOut, connectionToId.groupIn, connectionToId.portIn);
        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, 0, 0, 0.0f, strBuf);

        rack->usedConnections.append(connectionToId);
        ++rack->lastConnectionId;
    }

    for (LinkedList<int>::Itenerator it = rack->connectedOut1.begin(); it.valid(); it.next())
    {
        const int& port(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->audio.outCount));

        ConnectionToId connectionToId;
        connectionToId.id       = rack->lastConnectionId;
        connectionToId.groupOut = RACK_PATCHBAY_GROUP_CARLA;
        connectionToId.portOut  = RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT1;
        connectionToId.groupIn  = RACK_PATCHBAY_GROUP_AUDIO;
        connectionToId.portIn   = port;

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupOut, connectionToId.portOut, connectionToId.groupIn, connectionToId.portIn);
        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, 0, 0, 0.0f, strBuf);

        rack->usedConnections.append(connectionToId);
        ++rack->lastConnectionId;
    }

    for (LinkedList<int>::Itenerator it = rack->connectedOut2.begin(); it.valid(); it.next())
    {
        const int& port(it.getValue());
        CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->audio.outCount));

        ConnectionToId connectionToId;
        connectionToId.id       = rack->lastConnectionId;
        connectionToId.groupOut = RACK_PATCHBAY_GROUP_CARLA;
        connectionToId.portOut  = RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT2;
        connectionToId.groupIn  = RACK_PATCHBAY_GROUP_AUDIO;
        connectionToId.portIn   = port;

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupOut, connectionToId.portOut, connectionToId.groupIn, connectionToId.portIn);
        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, 0, 0, 0.0f, strBuf);

        rack->usedConnections.append(connectionToId);
        ++rack->lastConnectionId;
    }

    pData->audio.rack->connectLock.leave();

    for (LinkedList<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
    {
        const MidiPort& midiPort(it.getValue());

        ConnectionToId connectionToId;
        connectionToId.id       = rack->lastConnectionId;
        connectionToId.groupOut = RACK_PATCHBAY_GROUP_MIDI;
        connectionToId.portOut  = midiPort.portId;
        connectionToId.groupIn  = RACK_PATCHBAY_GROUP_CARLA;
        connectionToId.portIn   = RACK_PATCHBAY_CARLA_PORT_MIDI_IN;

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupOut, connectionToId.portOut, connectionToId.groupIn, connectionToId.portIn);
        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, 0, 0, 0.0f, strBuf);

        rack->usedConnections.append(connectionToId);
        ++rack->lastConnectionId;
    }

    for (LinkedList<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
    {
        const MidiPort& midiPort(it.getValue());

        ConnectionToId connectionToId;
        connectionToId.id       = rack->lastConnectionId;
        connectionToId.groupOut = RACK_PATCHBAY_GROUP_CARLA;
        connectionToId.portOut  = RACK_PATCHBAY_CARLA_PORT_MIDI_OUT;
        connectionToId.groupIn  = RACK_PATCHBAY_GROUP_MIDI;
        connectionToId.portIn   = midiPort.portId;

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupOut, connectionToId.portOut, connectionToId.groupIn, connectionToId.portIn);
        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, 0, 0, 0.0f, strBuf);

        rack->usedConnections.append(connectionToId);
        ++rack->lastConnectionId;
    }
#endif
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
        CARLA_SAFE_ASSERT_CONTINUE(connection.groupA >= RACK_PATCHBAY_GROUP_CARLA && connection.groupA < RACK_PATCHBAY_GROUP_MAX);
        CARLA_SAFE_ASSERT_CONTINUE(connection.groupB >= RACK_PATCHBAY_GROUP_CARLA && connection.groupB < RACK_PATCHBAY_GROUP_MAX);
        CARLA_SAFE_ASSERT_CONTINUE(connection.portA >= 0);
        CARLA_SAFE_ASSERT_CONTINUE(connection.portB >= 0);

        int carlaPort, otherPort;

        if (connection.groupA == RACK_PATCHBAY_GROUP_CARLA)
        {
            carlaPort = connection.portA;
            otherPort = connection.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_CONTINUE(connection.groupB == RACK_PATCHBAY_GROUP_CARLA);

            carlaPort = connection.portB;
            otherPort = connection.portA;
        }

        switch (carlaPort)
        {
        case RACK_PATCHBAY_CARLA_PORT_AUDIO_IN1:
        case RACK_PATCHBAY_CARLA_PORT_AUDIO_IN2:
            std::sprintf(strBuf, "AudioIn:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup((carlaPort == RACK_PATCHBAY_CARLA_PORT_AUDIO_IN1) ? "Carla:AudioIn1" : "Carla:AudioIn2"));
            break;

        case RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT1:
        case RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT2:
            connList.append(carla_strdup((carlaPort == RACK_PATCHBAY_CARLA_PORT_AUDIO_OUT1) ? "Carla:AudioOut1" : "Carla:AudioOut2"));
            std::sprintf(strBuf, "AudioOut:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            break;

        case RACK_PATCHBAY_CARLA_PORT_MIDI_IN:
            std::sprintf(strBuf, "MidiIn:%i", otherPort+1);
            connList.append(carla_strdup(strBuf));
            connList.append(carla_strdup("Carla:MidiIn"));
            break;

        case RACK_PATCHBAY_CARLA_PORT_MIDI_OUT:
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

void PatchbayGraph::refresh(CarlaEngine* const engine, const LinkedList<PortNameToId>& /*midiIns*/, const LinkedList<PortNameToId>& /*midiOuts*/) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);
}

const char* const* PatchbayGraph::getConnections() const
{
    return nullptr;
}

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
    for (unsigned int i=nextAction.pluginId; i < curPluginCount; ++i)
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

    const unsigned int id(curPluginCount);

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

    const unsigned int idA(nextAction.pluginId);
    const unsigned int idB(nextAction.value);

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
void CarlaEngineProtectedData::processRack(float* inBufReal[2], float* outBuf[2], const uint32_t frames, const bool isOffline)
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
    for (unsigned int i=0; i < curPluginCount; ++i)
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
            float tmpMin, tmpMax;

            if (oldAudioInCount > 0)
            {
                FloatVectorOperations::findMinAndMax(inBuf0, static_cast<int>(frames), tmpMin, tmpMax);
                pluginData.insPeak[0] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);

                FloatVectorOperations::findMinAndMax(inBuf1, static_cast<int>(frames), tmpMin, tmpMax);
                pluginData.insPeak[1] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);
            }
            else
            {
                pluginData.insPeak[0] = 0.0f;
                pluginData.insPeak[1] = 0.0f;
            }

            if (plugin->getAudioOutCount() > 0)
            {
                FloatVectorOperations::findMinAndMax(outBuf[0], static_cast<int>(frames), tmpMin, tmpMax);
                pluginData.outsPeak[0] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);

                FloatVectorOperations::findMinAndMax(outBuf[1], static_cast<int>(frames), tmpMin, tmpMax);
                pluginData.outsPeak[1] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);
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

void CarlaEngineProtectedData::processRackFull(float** const inBuf, const uint32_t inCount, float** const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline)
{
    const CarlaCriticalSectionScope _cs(graph.rack->connectLock);

    // connect input buffers
    if (graph.rack->connectedIn1.count() == 0)
    {
        FLOAT_CLEAR(audio.inBuf[0], nframes);
    }
    else
    {
        bool first = true;

        for (LinkedList<int>::Itenerator it = graph.rack->connectedIn1.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(inCount));

            if (first)
            {
                FLOAT_COPY(audio.inBuf[0], inBuf[port], nframes);
                first = false;
            }
            else
            {
                FLOAT_ADD(audio.inBuf[0], inBuf[port], nframes);
            }
        }

        if (first)
            FLOAT_CLEAR(audio.inBuf[0], nframes);
    }

    if (graph.rack->connectedIn2.count() == 0)
    {
        FLOAT_CLEAR(audio.inBuf[1], nframes);
    }
    else
    {
        bool first = true;

        for (LinkedList<int>::Itenerator it = graph.rack->connectedIn2.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(inCount));

            if (first)
            {
                FLOAT_COPY(audio.inBuf[1], inBuf[port], nframes);
                first = false;
            }
            else
            {
                FLOAT_ADD(audio.inBuf[1], inBuf[port], nframes);
            }
        }

        if (first)
            FLOAT_CLEAR(audio.inBuf[1], nframes);
    }

    FLOAT_CLEAR(audio.outBuf[0], nframes);
    FLOAT_CLEAR(audio.outBuf[1], nframes);

    // process
    processRack(audio.inBuf, audio.outBuf, nframes, isOffline);

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
