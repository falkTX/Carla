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

struct PortNameToId {
    int portId;
    char name[STR_MAX+1];
};

struct ConnectionToId {
    uint id;
    int groupA;
    int portA;
    int groupB;
    int portB;
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
// EngineRackBuffers

struct EngineRackBuffers {
    float* in[2];
    float* out[2];

    // connections stuff
    LinkedList<int> connectedIn1;
    LinkedList<int> connectedIn2;
    LinkedList<int> connectedOut1;
    LinkedList<int> connectedOut2;
    CarlaCriticalSection connectLock;

    uint lastConnectionId;
    LinkedList<ConnectionToId> usedConnections;

    EngineRackBuffers(const uint32_t bufferSize);
    ~EngineRackBuffers() noexcept;
    void clear() noexcept;
    void resize(const uint32_t bufferSize);

    bool connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int port) noexcept;
    bool disconnect(const uint connectionId);

    const char* const* getConnections() const;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineRackBuffers)
};

// -----------------------------------------------------------------------
// EnginePatchbayBuffers

struct EnginePatchbayBuffers {
    // TODO
    EnginePatchbayBuffers(const uint32_t bufferSize);
    ~EnginePatchbayBuffers() noexcept;
    void clear() noexcept;
    void resize(const uint32_t bufferSize);

    bool connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int port) noexcept;
    const char* const* getConnections() const;

    CARLA_DECLARE_NON_COPY_STRUCT(EnginePatchbayBuffers)
};

// -----------------------------------------------------------------------
// EngineRackBuffers

EngineRackBuffers::EngineRackBuffers(const uint32_t bufferSize)
    : lastConnectionId(0)
{
    resize(bufferSize);
}

EngineRackBuffers::~EngineRackBuffers() noexcept
{
    clear();
}

void EngineRackBuffers::clear() noexcept
{
    lastConnectionId = 0;

    if (in[0] != nullptr)
    {
        delete[] in[0];
        in[0] = nullptr;
    }

    if (in[1] != nullptr)
    {
        delete[] in[1];
        in[1] = nullptr;
    }

    if (out[0] != nullptr)
    {
        delete[] out[0];
        out[0] = nullptr;
    }

    if (out[1] != nullptr)
    {
        delete[] out[1];
        out[1] = nullptr;
    }

    connectedIn1.clear();
    connectedIn2.clear();
    connectedOut1.clear();
    connectedOut2.clear();
    usedConnections.clear();
}

void EngineRackBuffers::resize(const uint32_t bufferSize)
{
    if (bufferSize > 0)
    {
        in[0]  = new float[bufferSize];
        in[1]  = new float[bufferSize];
        out[0] = new float[bufferSize];
        out[1] = new float[bufferSize];
    }
    else
    {
        in[0]  = nullptr;
        in[1]  = nullptr;
        out[0] = nullptr;
        out[1] = nullptr;
    }
}

bool EngineRackBuffers::connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int portB) noexcept
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

const char* const* EngineRackBuffers::getConnections() const
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
// EnginePatchbayBuffers

EnginePatchbayBuffers::EnginePatchbayBuffers(const uint32_t bufferSize)
{
    resize(bufferSize);
}

EnginePatchbayBuffers::~EnginePatchbayBuffers() noexcept
{
    clear();
}

void EnginePatchbayBuffers::clear() noexcept
{
}

void EnginePatchbayBuffers::resize(const uint32_t /*bufferSize*/)
{
}

bool EnginePatchbayBuffers::connect(CarlaEngine* const engine, const int, const int, const int, const int) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, false);

    return false;
}

const char* const* EnginePatchbayBuffers::getConnections() const
{
    return nullptr;
}

// -----------------------------------------------------------------------
// EnginePatchbayBuffers

EngineInternalAudio::create(const uint32_t bufferSize)
{
    CARLA_SAFE_ASSERT_RETURN(buffer == nullptr,);

    if (usePatchbay)
        buffer = new EnginePatchbayBuffers(bufferSize);
    else
        buffer = new EngineRackBuffers(bufferSize);

    isReady = true;
}

EngineInternalAudio::initPatchbay() noexcept
{
    if (usePatchbay)
    {
        CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);

        // TODO
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);

        rack->lastConnectionId = 0;
        rack->usedConnections.clear();
    }
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
