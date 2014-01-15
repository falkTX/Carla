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

#ifdef BUILD_BRIDGE
# error This file should not be compiled if building bridge
#endif

#include "CarlaEngineInternal.hpp"

#include "LinkedList.hpp"

#ifdef HAVE_JUCE
# include "juce_audio_basics.h"
using juce::FloatVectorOperations;
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------

struct EngineRackBuffers {
    float* in[2];
    float* out[2];

    // connections stuff
    LinkedList<uint> connectedIns[2];
    LinkedList<uint> connectedOuts[2];
    CarlaMutex connectLock;

    int lastConnectionId;
    LinkedList<ConnectionToId> usedConnections;

    EngineRackBuffers(const uint32_t bufferSize);
    ~EngineRackBuffers();
    void clear();
    void resize(const uint32_t bufferSize);
};

// -----------------------------------------------------------------------

struct EnginePatchbayBuffers {
    // TODO
    EnginePatchbayBuffers(const uint32_t bufferSize);
    ~EnginePatchbayBuffers();
    void clear();
    void resize(const uint32_t bufferSize);
};

// -----------------------------------------------------------------------
// EngineRackBuffers

EngineRackBuffers::EngineRackBuffers(const uint32_t bufferSize)
    : lastConnectionId(0)
{
    resize(bufferSize);
}

EngineRackBuffers::~EngineRackBuffers()
{
    clear();
}

void EngineRackBuffers::clear()
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

    connectedIns[0].clear();
    connectedIns[1].clear();
    connectedOuts[0].clear();
    connectedOuts[1].clear();
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

// -----------------------------------------------------------------------
// EnginePatchbayBuffers

EnginePatchbayBuffers::EnginePatchbayBuffers(const uint32_t bufferSize)
{
    resize(bufferSize);
}

EnginePatchbayBuffers::~EnginePatchbayBuffers()
{
    clear();
}

void EnginePatchbayBuffers::clear()
{
}

void EnginePatchbayBuffers::resize(const uint32_t /*bufferSize*/)
{
}

// -----------------------------------------------------------------------
// InternalAudio

InternalAudio::InternalAudio() noexcept
    : isReady(false),
      usePatchbay(false),
      inCount(0),
      outCount(0)
{
    rack = nullptr;
}

InternalAudio::~InternalAudio() noexcept
{
    CARLA_ASSERT(! isReady);
    CARLA_ASSERT(rack == nullptr);
}

void InternalAudio::initPatchbay() noexcept
{
    if (usePatchbay)
    {
        CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);

        rack->lastConnectionId = 0;
        rack->usedConnections.clear();
    }
}

void InternalAudio::clear()
{
    isReady  = false;
    inCount  = 0;
    outCount = 0;

    if (usePatchbay)
    {
        CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);
        delete patchbay;
        patchbay = nullptr;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);
        delete rack;
        rack = nullptr;
    }
}

void InternalAudio::create(const uint32_t bufferSize)
{
    if (usePatchbay)
    {
        CARLA_SAFE_ASSERT_RETURN(patchbay == nullptr,);
        patchbay = new EnginePatchbayBuffers(bufferSize);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(rack == nullptr,);
        rack = new EngineRackBuffers(bufferSize);
    }

    isReady = true;
}

void InternalAudio::resize(const uint32_t bufferSize)
{
    if (usePatchbay)
    {
        CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);
        patchbay->resize(bufferSize);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);
        rack->resize(bufferSize);
    }
}

void CarlaEngineProtectedData::processRackFull(float** const inBuf, const uint32_t inCount, float** const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline)
{
    EngineRackBuffers* const rack(bufAudio.rack);

    const CarlaMutex::ScopedLocker sl(rack->connectLock);

    // connect input buffers
    if (rack->connectedIns[0].count() == 0)
    {
        FLOAT_CLEAR(rack->in[0], nframes);
    }
    else
    {
        bool first = true;

        for (LinkedList<uint>::Itenerator it = rack->connectedIns[0].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < inCount);

            if (first)
            {
                FLOAT_COPY(rack->in[0], inBuf[port], nframes);
                first = false;
            }
            else
            {
                FLOAT_ADD(rack->in[0], inBuf[port], nframes);
            }
        }

        if (first)
            FLOAT_CLEAR(rack->in[0], nframes);
    }

    if (rack->connectedIns[1].count() == 0)
    {
        FLOAT_CLEAR(rack->in[1], nframes);
    }
    else
    {
        bool first = true;

        for (LinkedList<uint>::Itenerator it = rack->connectedIns[1].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < inCount);

            if (first)
            {
                FLOAT_COPY(rack->in[1], inBuf[port], nframes);
                first = false;
            }
            else
            {
                FLOAT_ADD(rack->in[1], inBuf[port], nframes);
            }
        }

        if (first)
            FLOAT_CLEAR(rack->in[1], nframes);
    }

    FLOAT_CLEAR(rack->out[0], nframes);
    FLOAT_CLEAR(rack->out[1], nframes);

    // process
    processRack(rack->in, rack->out, nframes, isOffline);

    // connect output buffers
    if (rack->connectedOuts[0].count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = rack->connectedOuts[0].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < outCount);

            FLOAT_ADD(outBuf[port], rack->out[0], nframes);
        }
    }

    if (rack->connectedOuts[1].count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = rack->connectedOuts[1].begin(); it.valid(); it.next())
        {
            const uint& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port < outCount);

            FLOAT_ADD(outBuf[port], rack->out[1], nframes);
        }
    }
}

// -----------------------------------------------------------------------
// Patchbay

bool CarlaEngine::patchbayConnect(const int portA, const int portB)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->bufAudio.isReady, false);
    carla_debug("CarlaEngineRtAudio::patchbayConnect(%i, %i)", portA, portB);

    if (pData->bufAudio.usePatchbay)
    {
        // not implemented yet
        return false;
    }

    EngineRackBuffers* const rack(pData->bufAudio.rack);

    CARLA_SAFE_ASSERT_RETURN_ERR(portA > RACK_PATCHBAY_PORT_MAX, "Invalid output port");
    CARLA_SAFE_ASSERT_RETURN_ERR(portB > RACK_PATCHBAY_PORT_MAX, "Invalid input port");

    // only allow connections between Carla and other ports
    if (portA < 0 && portB < 0)
    {
        setLastError("Invalid connection (1)");
        return false;
    }
    if (portA >= 0 && portB >= 0)
    {
        setLastError("Invalid connection (2)");
        return false;
    }

    const int carlaPort  = (portA < 0) ? portA : portB;
    const int targetPort = (carlaPort == portA) ? portB : portA;
    bool makeConnection  = false;

    switch (carlaPort)
    {
    case RACK_PATCHBAY_PORT_AUDIO_IN1:
        CARLA_SAFE_ASSERT_BREAK(targetPort >= RACK_PATCHBAY_GROUP_AUDIO_IN*1000);
        CARLA_SAFE_ASSERT_BREAK(targetPort <= RACK_PATCHBAY_GROUP_AUDIO_IN*1000+999);
        rack->connectLock.lock();
        rack->connectedIns[0].append(targetPort - RACK_PATCHBAY_GROUP_AUDIO_IN*1000);
        rack->connectLock.unlock();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_PORT_AUDIO_IN2:
        CARLA_SAFE_ASSERT_BREAK(targetPort >= RACK_PATCHBAY_GROUP_AUDIO_IN*1000);
        CARLA_SAFE_ASSERT_BREAK(targetPort <= RACK_PATCHBAY_GROUP_AUDIO_IN*1000+999);
        rack->connectLock.lock();
        rack->connectedIns[1].append(targetPort - RACK_PATCHBAY_GROUP_AUDIO_IN*1000);
        rack->connectLock.unlock();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_PORT_AUDIO_OUT1:
        CARLA_SAFE_ASSERT_BREAK(targetPort >= RACK_PATCHBAY_GROUP_AUDIO_OUT*1000);
        CARLA_SAFE_ASSERT_BREAK(targetPort <= RACK_PATCHBAY_GROUP_AUDIO_OUT*1000+999);
        rack->connectLock.lock();
        rack->connectedOuts[0].append(targetPort - RACK_PATCHBAY_GROUP_AUDIO_OUT*1000);
        rack->connectLock.unlock();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_PORT_AUDIO_OUT2:
        CARLA_SAFE_ASSERT_BREAK(targetPort >= RACK_PATCHBAY_GROUP_AUDIO_OUT*1000);
        CARLA_SAFE_ASSERT_BREAK(targetPort <= RACK_PATCHBAY_GROUP_AUDIO_OUT*1000+999);
        rack->connectLock.lock();
        rack->connectedOuts[1].append(targetPort - RACK_PATCHBAY_GROUP_AUDIO_OUT*1000);
        rack->connectLock.unlock();
        makeConnection = true;
        break;

    case RACK_PATCHBAY_PORT_MIDI_IN:
        CARLA_SAFE_ASSERT_BREAK(targetPort >= RACK_PATCHBAY_GROUP_MIDI_IN*1000);
        CARLA_SAFE_ASSERT_BREAK(targetPort <= RACK_PATCHBAY_GROUP_MIDI_IN*1000+999);
        makeConnection = connectRackMidiInPort(targetPort - RACK_PATCHBAY_GROUP_MIDI_IN*1000);
        break;

    case RACK_PATCHBAY_PORT_MIDI_OUT:
        CARLA_SAFE_ASSERT_BREAK(targetPort >= RACK_PATCHBAY_GROUP_MIDI_OUT*1000);
        CARLA_SAFE_ASSERT_BREAK(targetPort <= RACK_PATCHBAY_GROUP_MIDI_OUT*1000+999);
        makeConnection = connectRackMidiOutPort(targetPort - RACK_PATCHBAY_GROUP_MIDI_OUT*1000);
        break;
    }

    if (! makeConnection)
    {
        setLastError("Invalid connection (3)");
        return false;
    }

    ConnectionToId connectionToId;
    connectionToId.id      = rack->lastConnectionId;
    connectionToId.portOut = portA;
    connectionToId.portIn  = portB;

    callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, portA, portB, 0.0f, nullptr);

    rack->usedConnections.append(connectionToId);
    rack->lastConnectionId++;

    return true;
}

bool CarlaEngine::patchbayDisconnect(const int connectionId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->bufAudio.isReady, false);
    carla_debug("CarlaEngineRtAudio::patchbayDisconnect(%i)", connectionId);

    if (pData->bufAudio.usePatchbay)
    {
        // not implemented yet
        return false;
    }

    EngineRackBuffers* const rack(pData->bufAudio.rack);

    CARLA_SAFE_ASSERT_RETURN_ERR(rack->usedConnections.count() > 0, "No connections available");

    for (LinkedList<ConnectionToId>::Itenerator it=rack->usedConnections.begin(); it.valid(); it.next())
    {
        const ConnectionToId& connection(it.getValue());

        if (connection.id == connectionId)
        {
            const int targetPort((connection.portOut >= 0) ? connection.portOut : connection.portIn);
            const int carlaPort((targetPort == connection.portOut) ? connection.portIn : connection.portOut);

            if (targetPort >= RACK_PATCHBAY_GROUP_MIDI_OUT*1000)
            {
                const int portId(targetPort-RACK_PATCHBAY_GROUP_MIDI_OUT*1000);
                disconnectRackMidiInPort(portId);
            }
            else if (targetPort >= RACK_PATCHBAY_GROUP_MIDI_IN*1000)
            {
                const int portId(targetPort-RACK_PATCHBAY_GROUP_MIDI_IN*1000);
                disconnectRackMidiOutPort(portId);
            }
            else if (targetPort >= RACK_PATCHBAY_GROUP_AUDIO_OUT*1000)
            {
                CARLA_SAFE_ASSERT_RETURN(carlaPort == RACK_PATCHBAY_PORT_AUDIO_OUT1 || carlaPort == RACK_PATCHBAY_PORT_AUDIO_OUT2, false);

                const int portId(targetPort-RACK_PATCHBAY_GROUP_AUDIO_OUT*1000);

                rack->connectLock.lock();

                if (carlaPort == RACK_PATCHBAY_PORT_AUDIO_OUT1)
                    rack->connectedOuts[0].removeAll(portId);
                else
                    rack->connectedOuts[1].removeAll(portId);

                rack->connectLock.unlock();
            }
            else if (targetPort >= RACK_PATCHBAY_GROUP_AUDIO_IN*1000)
            {
                CARLA_SAFE_ASSERT_RETURN(carlaPort == RACK_PATCHBAY_PORT_AUDIO_IN1 || carlaPort == RACK_PATCHBAY_PORT_AUDIO_IN2, false);

                const int portId(targetPort-RACK_PATCHBAY_GROUP_AUDIO_IN*1000);

                rack->connectLock.lock();

                if (carlaPort == RACK_PATCHBAY_PORT_AUDIO_IN1)
                    rack->connectedIns[0].removeAll(portId);
                else
                    rack->connectedIns[1].removeAll(portId);

                rack->connectLock.unlock();
            }
            else
            {
                CARLA_SAFE_ASSERT_RETURN(false, false);
            }

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connection.id, connection.portOut, connection.portIn, 0.0f, nullptr);

            rack->usedConnections.remove(it);
            break;
        }
    }

    return true;
}

bool CarlaEngine::patchbayRefresh()
{
    setLastError("Unsupported operation");
    return false;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
