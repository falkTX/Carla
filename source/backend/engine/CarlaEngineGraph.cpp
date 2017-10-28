/*
 * Carla Plugin Host
 * Copyright (C) 2011-2017 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaPlugin.hpp"

#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Fallback data

static const PortNameToId kPortNameToIdFallback   = { 0, 0, { '\0' }, { '\0' } };
static /* */ PortNameToId kPortNameToIdFallbackNC = { 0, 0, { '\0' }, { '\0' } };

// -----------------------------------------------------------------------
// External Graph stuff

static inline
uint getExternalGraphPortIdFromName(const char* const shortname) noexcept
{
    if (std::strcmp(shortname, "AudioIn1") == 0 || std::strcmp(shortname, "audio-in1") == 0)
        return kExternalGraphCarlaPortAudioIn1;
    if (std::strcmp(shortname, "AudioIn2") == 0 || std::strcmp(shortname, "audio-in2") == 0)
        return kExternalGraphCarlaPortAudioIn2;
    if (std::strcmp(shortname, "AudioOut1") == 0 || std::strcmp(shortname, "audio-out1") == 0)
        return kExternalGraphCarlaPortAudioOut1;
    if (std::strcmp(shortname, "AudioOut2") == 0 || std::strcmp(shortname, "audio-out2") == 0)
        return kExternalGraphCarlaPortAudioOut2;
    if (std::strcmp(shortname, "MidiIn") == 0 || std::strcmp(shortname, "midi-in") == 0)
        return kExternalGraphCarlaPortMidiIn;
    if (std::strcmp(shortname, "MidiOut") == 0 || std::strcmp(shortname, "midi-out") == 0)
        return kExternalGraphCarlaPortMidiOut;

    carla_stderr("CarlaBackend::getExternalGraphPortIdFromName(%s) - invalid short name", shortname);
    return kExternalGraphCarlaPortNull;
}

static inline
const char* getExternalGraphFullPortNameFromId(const /*RackGraphCarlaPortIds*/ uint portId)
{
    switch (portId)
    {
    case kExternalGraphCarlaPortAudioIn1:
        return "Carla:AudioIn1";
    case kExternalGraphCarlaPortAudioIn2:
        return "Carla:AudioIn2";
    case kExternalGraphCarlaPortAudioOut1:
        return "Carla:AudioOut1";
    case kExternalGraphCarlaPortAudioOut2:
        return "Carla:AudioOut2";
    case kExternalGraphCarlaPortMidiIn:
        return "Carla:MidiIn";
    case kExternalGraphCarlaPortMidiOut:
        return "Carla:MidiOut";
    //case kExternalGraphCarlaPortNull:
    //case kExternalGraphCarlaPortMax:
    //    break;
    }

    carla_stderr("CarlaBackend::getExternalGraphFullPortNameFromId(%i) - invalid port id", portId);
    return nullptr;
}

// -----------------------------------------------------------------------

ExternalGraphPorts::ExternalGraphPorts() noexcept
    : ins(),
      outs() {}

const char* ExternalGraphPorts::getName(const bool isInput, const uint portId) const noexcept
{
    for (LinkedList<PortNameToId>::Itenerator it = isInput ? ins.begin2() : outs.begin2(); it.valid(); it.next())
    {
        const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

        if (portNameToId.port == portId)
            return portNameToId.name;
    }

    return nullptr;
}

uint ExternalGraphPorts::getPortId(const bool isInput, const char portName[], bool* const ok) const noexcept
{
    for (LinkedList<PortNameToId>::Itenerator it = isInput ? ins.begin2() : outs.begin2(); it.valid(); it.next())
    {
        const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

        if (std::strncmp(portNameToId.name, portName, STR_MAX) == 0)
        {
            if (ok != nullptr)
                *ok = true;
            return portNameToId.port;
        }
    }

    if (ok != nullptr)
        *ok = false;
    return 0;
}

// -----------------------------------------------------------------------

ExternalGraph::ExternalGraph(CarlaEngine* const engine) noexcept
    : connections(),
      audioPorts(),
      midiPorts(),
      retCon(),
      kEngine(engine) {}

void ExternalGraph::clear() noexcept
{
    connections.clear();
    audioPorts.ins.clear();
    audioPorts.outs.clear();
    midiPorts.ins.clear();
    midiPorts.outs.clear();
}

bool ExternalGraph::connect(const uint groupA, const uint portA, const uint groupB, const uint portB, const bool sendCallback) noexcept
{
    uint otherGroup, otherPort, carlaPort;

    if (groupA == kExternalGraphGroupCarla)
    {
        CARLA_SAFE_ASSERT_RETURN(groupB != kExternalGraphGroupCarla, false);

        carlaPort  = portA;
        otherGroup = groupB;
        otherPort  = portB;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(groupB == kExternalGraphGroupCarla, false);

        carlaPort  = portB;
        otherGroup = groupA;
        otherPort  = portA;
    }

    CARLA_SAFE_ASSERT_RETURN(carlaPort > kExternalGraphCarlaPortNull && carlaPort < kExternalGraphCarlaPortMax, false);
    CARLA_SAFE_ASSERT_RETURN(otherGroup > kExternalGraphGroupCarla && otherGroup < kExternalGraphGroupMax, false);

    bool makeConnection = false;

    switch (carlaPort)
    {
    case kExternalGraphCarlaPortAudioIn1:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == kExternalGraphGroupAudioIn, false);
        makeConnection = kEngine->connectExternalGraphPort(kExternalGraphConnectionAudioIn1, otherPort, nullptr);
        break;

    case kExternalGraphCarlaPortAudioIn2:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == kExternalGraphGroupAudioIn, false);
        makeConnection = kEngine->connectExternalGraphPort(kExternalGraphConnectionAudioIn2, otherPort, nullptr);
        break;

    case kExternalGraphCarlaPortAudioOut1:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == kExternalGraphGroupAudioOut, false);
        makeConnection = kEngine->connectExternalGraphPort(kExternalGraphConnectionAudioOut1, otherPort, nullptr);
        break;

    case kExternalGraphCarlaPortAudioOut2:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == kExternalGraphGroupAudioOut, false);
        makeConnection = kEngine->connectExternalGraphPort(kExternalGraphConnectionAudioOut2, otherPort, nullptr);
        break;

    case kExternalGraphCarlaPortMidiIn:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == kExternalGraphGroupMidiIn, false);
        if (const char* const portName = midiPorts.getName(true, otherPort))
            makeConnection = kEngine->connectExternalGraphPort(kExternalGraphConnectionMidiInput, 0, portName);
        break;

    case kExternalGraphCarlaPortMidiOut:
        CARLA_SAFE_ASSERT_RETURN(otherGroup == kExternalGraphGroupMidiOut, false);
        if (const char* const portName = midiPorts.getName(false, otherPort))
            makeConnection = kEngine->connectExternalGraphPort(kExternalGraphConnectionMidiOutput, 0, portName);
        break;
    }

    if (! makeConnection)
    {
        kEngine->setLastError("Invalid rack connection");
        return false;
    }

    ConnectionToId connectionToId;
    connectionToId.setData(++connections.lastId, groupA, portA, groupB, portB);

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';
    std::snprintf(strBuf, STR_MAX, "%u:%u:%u:%u", groupA, portA, groupB, portB);

    if (sendCallback)
        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

    connections.list.append(connectionToId);
    return true;
}

bool ExternalGraph::disconnect(const uint connectionId) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(connections.list.count() > 0, false);

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin2(); it.valid(); it.next())
    {
        static const ConnectionToId fallback = { 0, 0, 0, 0, 0 };

        const ConnectionToId& connectionToId(it.getValue(fallback));
        CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id > 0);

        if (connectionToId.id != connectionId)
            continue;

        uint otherGroup, otherPort, carlaPort;

        if (connectionToId.groupA == kExternalGraphGroupCarla)
        {
            CARLA_SAFE_ASSERT_RETURN(connectionToId.groupB != kExternalGraphGroupCarla, false);

            carlaPort  = connectionToId.portA;
            otherGroup = connectionToId.groupB;
            otherPort  = connectionToId.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(connectionToId.groupB == kExternalGraphGroupCarla, false);

            carlaPort  = connectionToId.portB;
            otherGroup = connectionToId.groupA;
            otherPort  = connectionToId.portA;
        }

        CARLA_SAFE_ASSERT_RETURN(carlaPort > kExternalGraphCarlaPortNull && carlaPort < kExternalGraphCarlaPortMax, false);
        CARLA_SAFE_ASSERT_RETURN(otherGroup > kExternalGraphGroupCarla && otherGroup < kExternalGraphGroupMax, false);

        bool makeDisconnection = false;

        switch (carlaPort)
        {
        case kExternalGraphCarlaPortAudioIn1:
            makeDisconnection = kEngine->disconnectExternalGraphPort(kExternalGraphConnectionAudioIn1, otherPort, nullptr);
            break;

        case kExternalGraphCarlaPortAudioIn2:
            makeDisconnection = kEngine->disconnectExternalGraphPort(kExternalGraphConnectionAudioIn2, otherPort, nullptr);
            break;

        case kExternalGraphCarlaPortAudioOut1:
            makeDisconnection = kEngine->disconnectExternalGraphPort(kExternalGraphConnectionAudioOut1, otherPort, nullptr);
            break;

        case kExternalGraphCarlaPortAudioOut2:
            makeDisconnection = kEngine->disconnectExternalGraphPort(kExternalGraphConnectionAudioOut2, otherPort, nullptr);
            break;

        case kExternalGraphCarlaPortMidiIn:
            if (const char* const portName = midiPorts.getName(true, otherPort))
                makeDisconnection = kEngine->disconnectExternalGraphPort(kExternalGraphConnectionMidiInput, 0, portName);
            break;

        case kExternalGraphCarlaPortMidiOut:
            if (const char* const portName = midiPorts.getName(false, otherPort))
                makeDisconnection = kEngine->disconnectExternalGraphPort(kExternalGraphConnectionMidiOutput, 0, portName);
            break;
        }

        if (! makeDisconnection)
        {
            kEngine->setLastError("Invalid rack connection");
            return false;
        }

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connectionToId.id, 0, 0, 0.0f, nullptr);

        connections.list.remove(it);
        return true;
    }

    kEngine->setLastError("Failed to find connection");
    return false;
}

void ExternalGraph::refresh(const char* const deviceName)
{
    CARLA_SAFE_ASSERT_RETURN(deviceName != nullptr,);

    const bool isRack(kEngine->getOptions().processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK);

    // Main
    {
        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, kExternalGraphGroupCarla, PATCHBAY_ICON_CARLA, -1, 0.0f, kEngine->getName());

        if (isRack)
        {
            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioIn1,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in1");
            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioIn2,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in2");
            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioOut1, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out1");
            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioOut2, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out2");
        }

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupCarla, kExternalGraphCarlaPortMidiIn,    PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,  0.0f, "midi-in");
        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupCarla, kExternalGraphCarlaPortMidiOut,   PATCHBAY_PORT_TYPE_MIDI,                         0.0f, "midi-out");
    }

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    if (isRack)
    {
        // Audio In
        if (deviceName[0] != '\0')
            std::snprintf(strBuf, STR_MAX, "Capture (%s)", deviceName);
        else
            std::strncpy(strBuf, "Capture", STR_MAX);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, kExternalGraphGroupAudioIn, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

        const CarlaString groupNameIn(strBuf);

        int h = 0;
        for (LinkedList<PortNameToId>::Itenerator it = audioPorts.ins.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNameIn + portNameToId.name);

            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupAudioIn, ++h,
                              PATCHBAY_PORT_TYPE_AUDIO, 0.0f, portNameToId.name);
        }

        // Audio Out
        if (deviceName[0] != '\0')
            std::snprintf(strBuf, STR_MAX, "Playback (%s)", deviceName);
        else
            std::strncpy(strBuf, "Playback", STR_MAX);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, kExternalGraphGroupAudioOut, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

        const CarlaString groupNameOut(strBuf);

        h = 0;
        for (LinkedList<PortNameToId>::Itenerator it = audioPorts.outs.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNameOut + portNameToId.name);

            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupAudioOut, ++h,
                              PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, portNameToId.name);
        }
    }

    // MIDI In
    {
        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, kExternalGraphGroupMidiIn, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Readable MIDI ports");

        const CarlaString groupNamePlus("Readable MIDI ports:");

        int h = 0;
        for (LinkedList<PortNameToId>::Itenerator it = midiPorts.ins.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNamePlus + portNameToId.name);

            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupMidiIn, ++h,
                              PATCHBAY_PORT_TYPE_MIDI, 0.0f, portNameToId.name);
        }
    }

    // MIDI Out
    {
        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, kExternalGraphGroupMidiOut, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Writable MIDI ports");

        const CarlaString groupNamePlus("Writable MIDI ports:");

        int h = 0;
        for (LinkedList<PortNameToId>::Itenerator it = midiPorts.outs.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNamePlus + portNameToId.name);

            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupMidiOut, ++h,
                              PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, 0.0f, portNameToId.name);
        }
    }
}

const char* const* ExternalGraph::getConnections() const noexcept
{
    if (connections.list.count() == 0)
        return nullptr;

    CarlaStringList connList;

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin2(); it.valid(); it.next())
    {
        static const ConnectionToId fallback = { 0, 0, 0, 0, 0 };

        const ConnectionToId& connectionToId(it.getValue(fallback));
        CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id > 0);

        uint otherGroup, otherPort, carlaPort;

        if (connectionToId.groupA == kExternalGraphGroupCarla)
        {
            CARLA_SAFE_ASSERT_CONTINUE(connectionToId.groupB != kExternalGraphGroupCarla);

            carlaPort  = connectionToId.portA;
            otherGroup = connectionToId.groupB;
            otherPort  = connectionToId.portB;
        }
        else
        {
            CARLA_SAFE_ASSERT_CONTINUE(connectionToId.groupB == kExternalGraphGroupCarla);

            carlaPort  = connectionToId.portB;
            otherGroup = connectionToId.groupA;
            otherPort  = connectionToId.portA;
        }

        CARLA_SAFE_ASSERT_CONTINUE(carlaPort > kExternalGraphCarlaPortNull && carlaPort < kExternalGraphCarlaPortMax);
        CARLA_SAFE_ASSERT_CONTINUE(otherGroup > kExternalGraphGroupCarla && otherGroup < kExternalGraphGroupMax);

        switch (carlaPort)
        {
        case kExternalGraphCarlaPortAudioIn1:
        case kExternalGraphCarlaPortAudioIn2:
            std::snprintf(strBuf, STR_MAX, "AudioIn:%s", audioPorts.getName(true, otherPort));
            connList.append(strBuf);
            connList.append(getExternalGraphFullPortNameFromId(carlaPort));
            break;

        case kExternalGraphCarlaPortAudioOut1:
        case kExternalGraphCarlaPortAudioOut2:
            std::snprintf(strBuf, STR_MAX, "AudioOut:%s", audioPorts.getName(false, otherPort));
            connList.append(getExternalGraphFullPortNameFromId(carlaPort));
            connList.append(strBuf);
            break;

        case kExternalGraphCarlaPortMidiIn:
            std::snprintf(strBuf, STR_MAX, "MidiIn:%s", midiPorts.getName(true, otherPort));
            connList.append(strBuf);
            connList.append(getExternalGraphFullPortNameFromId(carlaPort));
            break;

        case kExternalGraphCarlaPortMidiOut:
            std::snprintf(strBuf, STR_MAX, "MidiOut:%s", midiPorts.getName(false, otherPort));
            connList.append(getExternalGraphFullPortNameFromId(carlaPort));
            connList.append(strBuf);
            break;
        }
    }

    if (connList.count() == 0)
        return nullptr;

    retCon = connList.toCharStringListPtr();

    return retCon;
}

bool ExternalGraph::getGroupAndPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fullPortName != nullptr && fullPortName[0] != '\0', false);

    if (std::strncmp(fullPortName, "Carla:", 6) == 0)
    {
        groupId = kExternalGraphGroupCarla;
        portId  = getExternalGraphPortIdFromName(fullPortName+6);

        if (portId > kExternalGraphCarlaPortNull && portId < kExternalGraphCarlaPortMax)
            return true;
    }
    else if (std::strncmp(fullPortName, "AudioIn:", 8) == 0)
    {
        groupId = kExternalGraphGroupAudioIn;

        if (const char* const portName = fullPortName+8)
        {
            bool ok;
            portId = audioPorts.getPortId(true, portName, &ok);
            return ok;
        }
    }
    else if (std::strncmp(fullPortName, "AudioOut:", 9) == 0)
    {
        groupId = kExternalGraphGroupAudioOut;

        if (const char* const portName = fullPortName+9)
        {
            bool ok;
            portId = audioPorts.getPortId(false, portName, &ok);
            return ok;
        }
    }
    else if (std::strncmp(fullPortName, "MidiIn:", 7) == 0)
    {
        groupId = kExternalGraphGroupMidiIn;

        if (const char* const portName = fullPortName+7)
        {
            bool ok;
            portId = midiPorts.getPortId(true, portName, &ok);
            return ok;
        }
    }
    else if (std::strncmp(fullPortName, "MidiOut:", 8) == 0)
    {
        groupId = kExternalGraphGroupMidiOut;

        if (const char* const portName = fullPortName+8)
        {
            bool ok;
            portId = midiPorts.getPortId(false, portName, &ok);
            return ok;
        }
    }

    return false;
}

// -----------------------------------------------------------------------
// RackGraph Buffers

RackGraph::Buffers::Buffers() noexcept
    : mutex(),
      connectedIn1(),
      connectedIn2(),
      connectedOut1(),
      connectedOut2()
#ifdef CARLA_PROPER_CPP11_SUPPORT
    , inBuf{nullptr, nullptr},
      inBufTmp{nullptr, nullptr},
      outBuf{nullptr, nullptr} {}
#else
    {
        inBuf[0]    = inBuf[1]    = nullptr;
        inBufTmp[0] = inBufTmp[1] = nullptr;
        outBuf[0]   = outBuf[1]   = nullptr;
    }
#endif

RackGraph::Buffers::~Buffers() noexcept
{
    const CarlaRecursiveMutexLocker cml(mutex);

    if (inBuf[0]    != nullptr) { delete[] inBuf[0];    inBuf[0]    = nullptr; }
    if (inBuf[1]    != nullptr) { delete[] inBuf[1];    inBuf[1]    = nullptr; }
    if (inBufTmp[0] != nullptr) { delete[] inBufTmp[0]; inBufTmp[0] = nullptr; }
    if (inBufTmp[1] != nullptr) { delete[] inBufTmp[1]; inBufTmp[1] = nullptr; }
    if (outBuf[0]   != nullptr) { delete[] outBuf[0];   outBuf[0]   = nullptr; }
    if (outBuf[1]   != nullptr) { delete[] outBuf[1];   outBuf[1]   = nullptr; }

    connectedIn1.clear();
    connectedIn2.clear();
    connectedOut1.clear();
    connectedOut2.clear();
}

void RackGraph::Buffers::setBufferSize(const uint32_t bufferSize, const bool createBuffers) noexcept
{
    const CarlaRecursiveMutexLocker cml(mutex);

    if (inBuf[0]    != nullptr) { delete[] inBuf[0];    inBuf[0]    = nullptr; }
    if (inBuf[1]    != nullptr) { delete[] inBuf[1];    inBuf[1]    = nullptr; }
    if (inBufTmp[0] != nullptr) { delete[] inBufTmp[0]; inBufTmp[0] = nullptr; }
    if (inBufTmp[1] != nullptr) { delete[] inBufTmp[1]; inBufTmp[1] = nullptr; }
    if (outBuf[0]   != nullptr) { delete[] outBuf[0];   outBuf[0]   = nullptr; }
    if (outBuf[1]   != nullptr) { delete[] outBuf[1];   outBuf[1]   = nullptr; }

    CARLA_SAFE_ASSERT_RETURN(bufferSize > 0,);

    try {
        inBufTmp[0] = new float[bufferSize];
        inBufTmp[1] = new float[bufferSize];

        if (createBuffers)
        {
            inBuf[0]  = new float[bufferSize];
            inBuf[1]  = new float[bufferSize];
            outBuf[0] = new float[bufferSize];
            outBuf[1] = new float[bufferSize];
        }
    }
    catch(...) {
        if (inBufTmp[0] != nullptr) { delete[] inBufTmp[0]; inBufTmp[0] = nullptr; }
        if (inBufTmp[1] != nullptr) { delete[] inBufTmp[1]; inBufTmp[1] = nullptr; }

        if (createBuffers)
        {
            if (inBuf[0]  != nullptr) { delete[] inBuf[0];  inBuf[0]  = nullptr; }
            if (inBuf[1]  != nullptr) { delete[] inBuf[1];  inBuf[1]  = nullptr; }
            if (outBuf[0] != nullptr) { delete[] outBuf[0]; outBuf[0] = nullptr; }
            if (outBuf[1] != nullptr) { delete[] outBuf[1]; outBuf[1] = nullptr; }
        }
        return;
    }

    carla_zeroFloats(inBufTmp[0], bufferSize);
    carla_zeroFloats(inBufTmp[1], bufferSize);

    if (createBuffers)
    {
        carla_zeroFloats(inBuf[0],  bufferSize);
        carla_zeroFloats(inBuf[1],  bufferSize);
        carla_zeroFloats(outBuf[0], bufferSize);
        carla_zeroFloats(outBuf[1], bufferSize);
    }
}

// -----------------------------------------------------------------------
// RackGraph

RackGraph::RackGraph(CarlaEngine* const engine, const uint32_t ins, const uint32_t outs) noexcept
    : extGraph(engine),
      inputs(ins),
      outputs(outs),
      isOffline(false),
      audioBuffers(),
      kEngine(engine)
{
    setBufferSize(engine->getBufferSize());
}

RackGraph::~RackGraph() noexcept
{
    extGraph.clear();
}

void RackGraph::setBufferSize(const uint32_t bufferSize) noexcept
{
    audioBuffers.setBufferSize(bufferSize, (inputs > 0 || outputs > 0));
}

void RackGraph::setOffline(const bool offline) noexcept
{
    isOffline = offline;
}

bool RackGraph::connect(const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept
{
    return extGraph.connect(groupA, portA, groupB, portB, true);
}

bool RackGraph::disconnect(const uint connectionId) noexcept
{
    return extGraph.disconnect(connectionId);
}

void RackGraph::refresh(const char* const deviceName)
{
    extGraph.refresh(deviceName);

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    // Connections
    const CarlaRecursiveMutexLocker cml(audioBuffers.mutex);

    for (LinkedList<uint>::Itenerator it = audioBuffers.connectedIn1.begin2(); it.valid(); it.next())
    {
        const uint& portId(it.getValue(0));
        CARLA_SAFE_ASSERT_CONTINUE(portId > 0);
        CARLA_SAFE_ASSERT_CONTINUE(portId <= extGraph.audioPorts.ins.count());

        ConnectionToId connectionToId;
        connectionToId.setData(++(extGraph.connections.lastId), kExternalGraphGroupAudioIn, portId, kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioIn1);

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

        extGraph.connections.list.append(connectionToId);
    }

    for (LinkedList<uint>::Itenerator it = audioBuffers.connectedIn2.begin2(); it.valid(); it.next())
    {
        const uint& portId(it.getValue(0));
        CARLA_SAFE_ASSERT_CONTINUE(portId > 0);
        CARLA_SAFE_ASSERT_CONTINUE(portId <= extGraph.audioPorts.ins.count());

        ConnectionToId connectionToId;
        connectionToId.setData(++(extGraph.connections.lastId), kExternalGraphGroupAudioIn, portId, kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioIn2);

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

        extGraph.connections.list.append(connectionToId);
    }

    for (LinkedList<uint>::Itenerator it = audioBuffers.connectedOut1.begin2(); it.valid(); it.next())
    {
        const uint& portId(it.getValue(0));
        CARLA_SAFE_ASSERT_CONTINUE(portId > 0);
        CARLA_SAFE_ASSERT_CONTINUE(portId <= extGraph.audioPorts.outs.count());

        ConnectionToId connectionToId;
        connectionToId.setData(++(extGraph.connections.lastId), kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioOut1, kExternalGraphGroupAudioOut, portId);

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

        extGraph.connections.list.append(connectionToId);
    }

    for (LinkedList<uint>::Itenerator it = audioBuffers.connectedOut2.begin2(); it.valid(); it.next())
    {
        const uint& portId(it.getValue(0));
        CARLA_SAFE_ASSERT_CONTINUE(portId > 0);
        CARLA_SAFE_ASSERT_CONTINUE(portId <= extGraph.audioPorts.outs.count());

        ConnectionToId connectionToId;
        connectionToId.setData(++(extGraph.connections.lastId), kExternalGraphGroupCarla, kExternalGraphCarlaPortAudioOut2, kExternalGraphGroupAudioOut, portId);

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

        extGraph.connections.list.append(connectionToId);
    }
}

const char* const* RackGraph::getConnections() const noexcept
{
    return extGraph.getConnections();
}

bool RackGraph::getGroupAndPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const noexcept
{
    return extGraph.getGroupAndPortIdFromFullName(fullPortName, groupId, portId);
}

void RackGraph::process(CarlaEngine::ProtectedData* const data, const float* inBufReal[2], float* outBuf[2], const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.out != nullptr,);

    // safe copy
    float inBuf0[frames];
    float inBuf1[frames];
    const float* inBuf[2] = { inBuf0, inBuf1 };

    // initialize audio inputs
    carla_copyFloats(inBuf0, inBufReal[0], frames);
    carla_copyFloats(inBuf1, inBufReal[1], frames);

    // initialize audio outputs (zero)
    carla_zeroFloats(outBuf[0], frames);
    carla_zeroFloats(outBuf[1], frames);

    // initialize event outputs (zero)
    carla_zeroStructs(data->events.out, kMaxEngineEventInternalCount);

    uint32_t oldAudioInCount  = 0;
    uint32_t oldAudioOutCount = 0;
    uint32_t oldMidiOutCount  = 0;
    bool processed = false;

    // process plugins
    for (uint i=0; i < data->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin = data->plugins[i].plugin;

        if (plugin == nullptr || ! plugin->isEnabled() || ! plugin->tryLock(isOffline))
            continue;

        if (processed)
        {
            // initialize audio inputs (from previous outputs)
            carla_copyFloats(inBuf0, outBuf[0], frames);
            carla_copyFloats(inBuf1, outBuf[1], frames);

            // initialize audio outputs (zero)
            carla_zeroFloats(outBuf[0], frames);
            carla_zeroFloats(outBuf[1], frames);

            // if plugin has no midi out, add previous events
            if (oldMidiOutCount == 0 && data->events.in[0].type != kEngineEventTypeNull)
            {
                if (data->events.out[0].type != kEngineEventTypeNull)
                {
                    // TODO: carefully add to input, sorted events
                }
                // else nothing needed
            }
            else
            {
                // initialize event inputs from previous outputs
                carla_copyStructs(data->events.in, data->events.out, kMaxEngineEventInternalCount);

                // initialize event outputs (zero)
                carla_zeroStructs(data->events.out, kMaxEngineEventInternalCount);
            }
        }

        oldAudioInCount  = plugin->getAudioInCount();
        oldAudioOutCount = plugin->getAudioOutCount();
        oldMidiOutCount  = plugin->getMidiOutCount();

        // process
        plugin->initBuffers();
        plugin->process(inBuf, outBuf, nullptr, nullptr, frames);
        plugin->unlock();

        // if plugin has no audio inputs, add input buffer
        if (oldAudioInCount == 0)
        {
            carla_addFloats(outBuf[0], inBuf0, frames);
            carla_addFloats(outBuf[1], inBuf1, frames);
        }

        // if plugin only has 1 output, copy it to the 2nd
        if (oldAudioOutCount == 1)
        {
            carla_copyFloats(outBuf[1], outBuf[0], frames);
        }

        // set peaks
        {
            EnginePluginData& pluginData(data->plugins[i]);

            if (oldAudioInCount > 0)
            {
                pluginData.insPeak[0] = carla_findMaxNormalizedFloat(inBuf0, frames);
                pluginData.insPeak[1] = carla_findMaxNormalizedFloat(inBuf1, frames);
            }
            else
            {
                pluginData.insPeak[0] = 0.0f;
                pluginData.insPeak[1] = 0.0f;
            }

            if (oldAudioOutCount > 0)
            {
                pluginData.outsPeak[0] = carla_findMaxNormalizedFloat(outBuf[0], frames);
                pluginData.outsPeak[1] = carla_findMaxNormalizedFloat(outBuf[1], frames);
            }
            else
            {
                pluginData.outsPeak[0] = 0.0f;
                pluginData.outsPeak[1] = 0.0f;
            }
        }

        processed = true;
    }
}

void RackGraph::processHelper(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(audioBuffers.outBuf[1] != nullptr,);

    const CarlaRecursiveMutexLocker _cml(audioBuffers.mutex);

    if (inBuf != nullptr && inputs > 0)
    {
        bool noConnections = true;

        // connect input buffers
        for (LinkedList<uint>::Itenerator it = audioBuffers.connectedIn1.begin2(); it.valid(); it.next())
        {
            const uint& port(it.getValue(0));
            CARLA_SAFE_ASSERT_CONTINUE(port > 0);
            CARLA_SAFE_ASSERT_CONTINUE(port <= inputs);

            if (noConnections)
            {
                carla_copyFloats(audioBuffers.inBuf[0], inBuf[port], frames);
                noConnections = false;
            }
            else
            {
                carla_addFloats(audioBuffers.inBuf[0], inBuf[port], frames);
            }
        }

        if (noConnections)
            carla_zeroFloats(audioBuffers.inBuf[0], frames);

        noConnections = true;

        for (LinkedList<uint>::Itenerator it = audioBuffers.connectedIn2.begin2(); it.valid(); it.next())
        {
            const uint& port(it.getValue(0));
            CARLA_SAFE_ASSERT_CONTINUE(port > 0);
            CARLA_SAFE_ASSERT_CONTINUE(port <= inputs);

            if (noConnections)
            {
                carla_copyFloats(audioBuffers.inBuf[1], inBuf[port-1], frames);
                noConnections = false;
            }
            else
            {
                carla_addFloats(audioBuffers.inBuf[1], inBuf[port-1], frames);
            }
        }

        if (noConnections)
            carla_zeroFloats(audioBuffers.inBuf[1], frames);
    }
    else
    {
        carla_zeroFloats(audioBuffers.inBuf[0], frames);
        carla_zeroFloats(audioBuffers.inBuf[1], frames);
    }

    carla_zeroFloats(audioBuffers.outBuf[0], frames);
    carla_zeroFloats(audioBuffers.outBuf[1], frames);

    // process
    process(data, const_cast<const float**>(audioBuffers.inBuf), audioBuffers.outBuf, frames);

    // connect output buffers
    if (audioBuffers.connectedOut1.count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = audioBuffers.connectedOut1.begin2(); it.valid(); it.next())
        {
            const uint& port(it.getValue(0));
            CARLA_SAFE_ASSERT_CONTINUE(port > 0);
            CARLA_SAFE_ASSERT_CONTINUE(port <= outputs);

            carla_addFloats(outBuf[port-1], audioBuffers.outBuf[0], frames);
        }
    }

    if (audioBuffers.connectedOut2.count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = audioBuffers.connectedOut2.begin2(); it.valid(); it.next())
        {
            const uint& port(it.getValue(0));
            CARLA_SAFE_ASSERT_CONTINUE(port > 0);
            CARLA_SAFE_ASSERT_CONTINUE(port <= outputs);

            carla_addFloats(outBuf[port-1], audioBuffers.outBuf[1], frames);
        }
    }
}

// -----------------------------------------------------------------------
// InternalGraph

EngineInternalGraph::EngineInternalGraph(CarlaEngine* const engine) noexcept
    : fIsReady(false),
      fRack(nullptr),
      kEngine(engine) {}

EngineInternalGraph::~EngineInternalGraph() noexcept
{
    CARLA_SAFE_ASSERT(! fIsReady);
    CARLA_SAFE_ASSERT(fRack == nullptr);
}

void EngineInternalGraph::create(const uint32_t inputs, const uint32_t outputs)
{
    CARLA_SAFE_ASSERT_RETURN(fRack == nullptr,);
    fRack = new RackGraph(kEngine, inputs, outputs);
    fIsReady = true;
}

void EngineInternalGraph::destroy() noexcept
{
    if (! fIsReady)
    {
        CARLA_SAFE_ASSERT(fRack == nullptr);
        return;
    }

    CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
    delete fRack;
    fRack = nullptr;

    fIsReady = false;
}

void EngineInternalGraph::setBufferSize(const uint32_t bufferSize)
{
    ScopedValueSetter<bool> svs(fIsReady, false, true);

    CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
    fRack->setBufferSize(bufferSize);
}

void EngineInternalGraph::setOffline(const bool offline)
{
    ScopedValueSetter<bool> svs(fIsReady, false, true);

    CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
    fRack->setOffline(offline);
}

bool EngineInternalGraph::isReady() const noexcept
{
    return fIsReady;
}

RackGraph* EngineInternalGraph::getGraph() const noexcept
{
    return fRack;
}

void EngineInternalGraph::process(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
    fRack->processHelper(data, inBuf, outBuf, frames);
}

void EngineInternalGraph::processRack(CarlaEngine::ProtectedData* const data, const float* inBuf[2], float* outBuf[2], const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
    fRack->process(data, inBuf, outBuf, frames);
}

// -----------------------------------------------------------------------
// CarlaEngine Patchbay stuff

bool CarlaEngine::patchbayConnect(const uint groupA, const uint portA, const uint groupB, const uint portB)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);
    carla_debug("CarlaEngine::patchbayConnect(%u, %u, %u, %u)", groupA, portA, groupB, portB);

    RackGraph* const graph = pData->graph.getGraph();
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

    return graph->connect(groupA, portA, groupB, portB);
}

bool CarlaEngine::patchbayDisconnect(const uint connectionId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);
    carla_debug("CarlaEngine::patchbayDisconnect(%u)", connectionId);

    RackGraph* const graph = pData->graph.getGraph();
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

    return graph->disconnect(connectionId);
}

bool CarlaEngine::patchbayRefresh()
{
    // This is implemented in engine subclasses
    setLastError("Unsupported operation");
    return false;
}

// -----------------------------------------------------------------------

const char* const* CarlaEngine::getPatchbayConnections() const
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK, nullptr);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), nullptr);
    carla_debug("CarlaEngine::getPatchbayConnections(%s)", bool2str(external));

    RackGraph* const graph = pData->graph.getGraph();
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr, nullptr);

    return graph->getConnections();
}

void CarlaEngine::restorePatchbayConnection(const char* const sourcePort, const char* const targetPort)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK,);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(),);
    CARLA_SAFE_ASSERT_RETURN(sourcePort != nullptr && sourcePort[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(targetPort != nullptr && targetPort[0] != '\0',);
    carla_debug("CarlaEngine::restorePatchbayConnection(\"%s\", \"%s\")", sourcePort, targetPort);

    uint groupA, portA;
    uint groupB, portB;

    RackGraph* const graph = pData->graph.getGraph();
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr,);

    if (! graph->getGroupAndPortIdFromFullName(sourcePort, groupA, portA))
        return;
    if (! graph->getGroupAndPortIdFromFullName(targetPort, groupB, portB))
        return;

    graph->connect(groupA, portA, groupB, portB);
}

// -----------------------------------------------------------------------

bool CarlaEngine::connectExternalGraphPort(const uint connectionType, const uint portId, const char* const portName)
{
    CARLA_SAFE_ASSERT_RETURN(connectionType != 0 || (portName != nullptr && portName[0] != '\0'), false);
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK, false);

    RackGraph* const graph(pData->graph.getGraph());
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

    const CarlaRecursiveMutexLocker cml(graph->audioBuffers.mutex);

    switch (connectionType)
    {
    case kExternalGraphConnectionAudioIn1:
        return graph->audioBuffers.connectedIn1.append(portId);
    case kExternalGraphConnectionAudioIn2:
        return graph->audioBuffers.connectedIn2.append(portId);
    case kExternalGraphConnectionAudioOut1:
        return graph->audioBuffers.connectedOut1.append(portId);
    case kExternalGraphConnectionAudioOut2:
        return graph->audioBuffers.connectedOut2.append(portId);
    }

    return false;
}

bool CarlaEngine::disconnectExternalGraphPort(const uint connectionType, const uint portId, const char* const portName)
{
    CARLA_SAFE_ASSERT_RETURN(connectionType != 0 || (portName != nullptr && portName[0] != '\0'), false);
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK, false);

    RackGraph* const graph(pData->graph.getGraph());
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

    const CarlaRecursiveMutexLocker cml(graph->audioBuffers.mutex);

    switch (connectionType)
    {
    case kExternalGraphConnectionAudioIn1:
        return graph->audioBuffers.connectedIn1.removeOne(portId);
    case kExternalGraphConnectionAudioIn2:
        return graph->audioBuffers.connectedIn2.removeOne(portId);
    case kExternalGraphConnectionAudioOut1:
        return graph->audioBuffers.connectedOut1.removeOne(portId);
    case kExternalGraphConnectionAudioOut2:
        return graph->audioBuffers.connectedOut2.removeOne(portId);
    }

    return false;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
