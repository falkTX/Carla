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
#include "CarlaPlugin.hpp"

#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

using juce::AudioPluginInstance;
using juce::AudioProcessor;
using juce::AudioProcessorEditor;
using juce::FloatVectorOperations;
using juce::MemoryBlock;
using juce::PluginDescription;
using juce::String;
using juce::jmin;
using juce::jmax;

CARLA_BACKEND_START_NAMESPACE

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
        static const PortNameToId portNameFallback = { 0, 0, { '\0' }, { '\0' } };

        const PortNameToId& portNameToId(it.getValue(portNameFallback));
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
        static const PortNameToId portNameFallback = { 0, 0, { '\0' }, { '\0' } };

        const PortNameToId& portNameToId(it.getValue(portNameFallback));
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

    // Audio In
    if (isRack)
    {
        if (deviceName[0] != '\0')
            std::snprintf(strBuf, STR_MAX, "Capture (%s)", deviceName);
        else
            std::strncpy(strBuf, "Capture", STR_MAX);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, kExternalGraphGroupAudioIn, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

        const CarlaString groupName(strBuf);

        int h = 0;
        for (LinkedList<PortNameToId>::Itenerator it = audioPorts.ins.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue());
            portNameToId.setFullName(groupName + portNameToId.name);

            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, kExternalGraphGroupAudioIn, ++h,
                              PATCHBAY_PORT_TYPE_AUDIO, 0.0f, portNameToId.name);
        }
    }

    // Audio Out
    if (isRack)
    {
        if (deviceName[0] != '\0')
            std::snprintf(strBuf, STR_MAX, "Playback (%s)", deviceName);
        else
            std::strncpy(strBuf, "Playback", STR_MAX);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, kExternalGraphGroupAudioOut, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

        const CarlaString groupName(strBuf);

        int h = 0;
        for (LinkedList<PortNameToId>::Itenerator it = audioPorts.outs.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue());
            portNameToId.setFullName(groupName + portNameToId.name);

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
            PortNameToId& portNameToId(it.getValue());
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
            PortNameToId& portNameToId(it.getValue());
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
    const int bufferSizei(static_cast<int>(bufferSize));

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

    FloatVectorOperations::clear(inBufTmp[0], bufferSizei);
    FloatVectorOperations::clear(inBufTmp[1], bufferSizei);

    if (createBuffers)
    {
        FloatVectorOperations::clear(inBuf[0],  bufferSizei);
        FloatVectorOperations::clear(inBuf[1],  bufferSizei);
        FloatVectorOperations::clear(outBuf[0], bufferSizei);
        FloatVectorOperations::clear(outBuf[1], bufferSizei);
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

    const int iframes(static_cast<int>(frames));

    // safe copy
    float inBuf0[frames];
    float inBuf1[frames];
    const float* inBuf[2] = { inBuf0, inBuf1 };

    // initialize audio inputs
    FloatVectorOperations::copy(inBuf0, inBufReal[0], iframes);
    FloatVectorOperations::copy(inBuf1, inBufReal[1], iframes);

    // initialize audio outputs (zero)
    FloatVectorOperations::clear(outBuf[0], iframes);
    FloatVectorOperations::clear(outBuf[1], iframes);

    // initialize event outputs (zero)
    carla_zeroStructs(data->events.out, kMaxEngineEventInternalCount);

    uint32_t oldAudioInCount  = 0;
    uint32_t oldAudioOutCount = 0;
    uint32_t oldMidiOutCount  = 0;
    bool processed = false;
    juce::Range<float> range;

    // process plugins
    for (uint i=0; i < data->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin = data->plugins[i].plugin;

        if (plugin == nullptr || ! plugin->isEnabled() || ! plugin->tryLock(isOffline))
            continue;

        if (processed)
        {
            // initialize audio inputs (from previous outputs)
            FloatVectorOperations::copy(inBuf0, outBuf[0], iframes);
            FloatVectorOperations::copy(inBuf1, outBuf[1], iframes);

            // initialize audio outputs (zero)
            FloatVectorOperations::clear(outBuf[0], iframes);
            FloatVectorOperations::clear(outBuf[1], iframes);

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
            FloatVectorOperations::add(outBuf[0], inBuf0, iframes);
            FloatVectorOperations::add(outBuf[1], inBuf1, iframes);
        }

        // if plugin only has 1 output, copy it to the 2nd
        if (oldAudioOutCount == 1)
        {
            FloatVectorOperations::copy(outBuf[1], outBuf[0], iframes);
        }

        // set peaks
        {
            EnginePluginData& pluginData(data->plugins[i]);

            if (oldAudioInCount > 0)
            {
                range = FloatVectorOperations::findMinAndMax(inBuf0, iframes);
                pluginData.insPeak[0] = carla_maxLimited<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);

                range = FloatVectorOperations::findMinAndMax(inBuf1, iframes);
                pluginData.insPeak[1] = carla_maxLimited<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);
            }
            else
            {
                pluginData.insPeak[0] = 0.0f;
                pluginData.insPeak[1] = 0.0f;
            }

            if (oldAudioOutCount > 0)
            {
                range = FloatVectorOperations::findMinAndMax(outBuf[0], iframes);
                pluginData.outsPeak[0] = carla_maxLimited<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);

                range = FloatVectorOperations::findMinAndMax(outBuf[1], iframes);
                pluginData.outsPeak[1] = carla_maxLimited<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);
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

    const int iframes(static_cast<int>(frames));

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
                FloatVectorOperations::copy(audioBuffers.inBuf[0], inBuf[port], iframes);
                noConnections = false;
            }
            else
            {
                FloatVectorOperations::add(audioBuffers.inBuf[0], inBuf[port], iframes);
            }
        }

        if (noConnections)
            FloatVectorOperations::clear(audioBuffers.inBuf[0], iframes);

        noConnections = true;

        for (LinkedList<uint>::Itenerator it = audioBuffers.connectedIn2.begin2(); it.valid(); it.next())
        {
            const uint& port(it.getValue(0));
            CARLA_SAFE_ASSERT_CONTINUE(port > 0);
            CARLA_SAFE_ASSERT_CONTINUE(port <= inputs);

            if (noConnections)
            {
                FloatVectorOperations::copy(audioBuffers.inBuf[1], inBuf[port-1], iframes);
                noConnections = false;
            }
            else
            {
                FloatVectorOperations::add(audioBuffers.inBuf[1], inBuf[port-1], iframes);
            }
        }

        if (noConnections)
            FloatVectorOperations::clear(audioBuffers.inBuf[1], iframes);
    }
    else
    {
        FloatVectorOperations::clear(audioBuffers.inBuf[0], iframes);
        FloatVectorOperations::clear(audioBuffers.inBuf[1], iframes);
    }

    FloatVectorOperations::clear(audioBuffers.outBuf[0], iframes);
    FloatVectorOperations::clear(audioBuffers.outBuf[1], iframes);

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

            FloatVectorOperations::add(outBuf[port-1], audioBuffers.outBuf[0], iframes);
        }
    }

    if (audioBuffers.connectedOut2.count() != 0)
    {
        for (LinkedList<uint>::Itenerator it = audioBuffers.connectedOut2.begin2(); it.valid(); it.next())
        {
            const uint& port(it.getValue(0));
            CARLA_SAFE_ASSERT_CONTINUE(port > 0);
            CARLA_SAFE_ASSERT_CONTINUE(port <= outputs);

            FloatVectorOperations::add(outBuf[port-1], audioBuffers.outBuf[1], iframes);
        }
    }
}

// -----------------------------------------------------------------------
// Patchbay Graph stuff

static const uint32_t kAudioInputPortOffset  = MAX_PATCHBAY_PLUGINS*1;
static const uint32_t kAudioOutputPortOffset = MAX_PATCHBAY_PLUGINS*2;
static const uint32_t kMidiInputPortOffset   = MAX_PATCHBAY_PLUGINS*3;
static const uint32_t kMidiOutputPortOffset  = MAX_PATCHBAY_PLUGINS*3+1;

static const uint kMidiChannelIndex = static_cast<uint>(AudioProcessorGraph::midiChannelIndex);

static inline
bool adjustPatchbayPortIdForJuce(uint& portId)
{
    CARLA_SAFE_ASSERT_RETURN(portId >= kAudioInputPortOffset, false);
    CARLA_SAFE_ASSERT_RETURN(portId <= kMidiOutputPortOffset, false);

    if (portId == kMidiInputPortOffset)
    {
        portId = kMidiChannelIndex;
        return true;
    }
    if (portId == kMidiOutputPortOffset)
    {
        portId = kMidiChannelIndex;
        return true;
    }
    if (portId >= kAudioOutputPortOffset)
    {
        portId -= kAudioOutputPortOffset;
        return true;
    }
    if (portId >= kAudioInputPortOffset)
    {
        portId -= kAudioInputPortOffset;
        return true;
    }

    return false;
}

static inline
const String getProcessorFullPortName(AudioProcessor* const proc, const uint32_t portId)
{
    CARLA_SAFE_ASSERT_RETURN(proc != nullptr, String());
    CARLA_SAFE_ASSERT_RETURN(portId >= kAudioInputPortOffset, String());
    CARLA_SAFE_ASSERT_RETURN(portId <= kMidiOutputPortOffset, String());

    String fullPortName(proc->getName());

    if (portId == kMidiOutputPortOffset)
    {
        fullPortName += ":events-out";
    }
    else if (portId == kMidiInputPortOffset)
    {
        fullPortName += ":events-in";
    }
    else if (portId >= kAudioOutputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getNumOutputChannels() > 0, String());
        fullPortName += ":" + proc->getOutputChannelName(static_cast<int>(portId-kAudioOutputPortOffset));
    }
    else if (portId >= kAudioInputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getNumInputChannels() > 0, String());
        fullPortName += ":" + proc->getInputChannelName(static_cast<int>(portId-kAudioInputPortOffset));
    }
    else
    {
        return String();
    }

    return fullPortName;
}

static inline
void addNodeToPatchbay(CarlaEngine* const engine, const uint32_t groupId, const int clientId, const AudioProcessor* const proc)
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(proc != nullptr,);

    const int icon((clientId >= 0) ? PATCHBAY_ICON_PLUGIN : PATCHBAY_ICON_HARDWARE);
    engine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, groupId, icon, clientId, 0.0f, proc->getName().toRawUTF8());

    for (int i=0, numInputs=proc->getNumInputChannels(); i<numInputs; ++i)
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, groupId, static_cast<int>(kAudioInputPortOffset)+i,
                         PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, proc->getInputChannelName(i).toRawUTF8());
    }

    for (int i=0, numOutputs=proc->getNumOutputChannels(); i<numOutputs; ++i)
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, groupId, static_cast<int>(kAudioOutputPortOffset)+i,
                         PATCHBAY_PORT_TYPE_AUDIO, 0.0f, proc->getOutputChannelName(i).toRawUTF8());
    }

    if (proc->acceptsMidi())
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, groupId, static_cast<int>(kMidiInputPortOffset),
                         PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, 0.0f, "events-in");
    }

    if (proc->producesMidi())
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, groupId, static_cast<int>(kMidiOutputPortOffset),
                         PATCHBAY_PORT_TYPE_MIDI, 0.0f, "events-out");
    }
}

static inline
void removeNodeFromPatchbay(CarlaEngine* const engine, const uint32_t groupId, const AudioProcessor* const proc)
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(proc != nullptr,);

    for (int i=0, numInputs=proc->getNumInputChannels(); i<numInputs; ++i)
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED, groupId, static_cast<int>(kAudioInputPortOffset)+i,
                         0, 0.0f, nullptr);
    }

    for (int i=0, numOutputs=proc->getNumOutputChannels(); i<numOutputs; ++i)
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED, groupId, static_cast<int>(kAudioOutputPortOffset)+i,
                         0, 0.0f, nullptr);
    }

    if (proc->acceptsMidi())
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED, groupId, static_cast<int>(kMidiInputPortOffset),
                         0, 0.0f, nullptr);
    }

    if (proc->producesMidi())
    {
        engine->callback(ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED, groupId, static_cast<int>(kMidiOutputPortOffset),
                         0, 0.0f, nullptr);
    }

    engine->callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED, groupId, 0, 0, 0.0f, nullptr);
}

// -----------------------------------------------------------------------

class CarlaPluginInstance : public AudioPluginInstance
{
public:
    CarlaPluginInstance(CarlaEngine* const engine, CarlaPlugin* const plugin)
        : kEngine(engine),
          fPlugin(plugin)
    {
        setPlayConfigDetails(static_cast<int>(fPlugin->getAudioInCount()),
                             static_cast<int>(fPlugin->getAudioOutCount()),
                             getSampleRate(), getBlockSize());
    }

    ~CarlaPluginInstance() override
    {
    }

    void invalidatePlugin() noexcept
    {
        fPlugin = nullptr;
    }

    // -------------------------------------------------------------------

    void* getPlatformSpecificData() noexcept override
    {
        return fPlugin;
    }

    void fillInPluginDescription(PluginDescription& d) const override
    {
        d.pluginFormatName = "Carla";
        d.category = "Carla Plugin";
        d.version = "1.0";

        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';

        fPlugin->getRealName(strBuf);
        d.name = strBuf;

        fPlugin->getLabel(strBuf);
        d.descriptiveName = strBuf;

        fPlugin->getMaker(strBuf);
        d.manufacturerName = strBuf;

        d.uid = d.name.hashCode();
        d.isInstrument = (fPlugin->getHints() & PLUGIN_IS_SYNTH);

        d.numInputChannels  = static_cast<int>(fPlugin->getAudioInCount());
        d.numOutputChannels = static_cast<int>(fPlugin->getAudioOutCount());
        //d.hasSharedContainer = true;
    }

    // -------------------------------------------------------------------

    const String getName() const override
    {
        return fPlugin->getName();
    }

    void processBlock(AudioSampleBuffer& audio, MidiBuffer& midi)
    {
        if (fPlugin == nullptr || ! fPlugin->isEnabled())
        {
            audio.clear();
            midi.clear();
            return;
        }

        if (! fPlugin->tryLock(kEngine->isOffline()))
        {
            audio.clear();
            midi.clear();
            return;
        }

        fPlugin->initBuffers();

        if (CarlaEngineEventPort* const port = fPlugin->getDefaultEventInPort())
        {
            EngineEvent* const engineEvents(port->fBuffer);
            CARLA_SAFE_ASSERT_RETURN(engineEvents != nullptr,);

            carla_zeroStructs(engineEvents, kMaxEngineEventInternalCount);
            fillEngineEventsFromJuceMidiBuffer(engineEvents, midi);
        }

        midi.clear();

        // TODO - CV support

        const int numSamples(audio.getNumSamples());

        if (const int numChan = audio.getNumChannels())
        {
            if (fPlugin->getAudioInCount() == 0)
                audio.clear();

            float* audioBuffers[numChan];

            for (int i=0; i<numChan; ++i)
                audioBuffers[i] = audio.getWritePointer(i);

            float inPeaks[2] = { 0.0f };
            float outPeaks[2] = { 0.0f };
            juce::Range<float> range;

            for (int i=jmin(fPlugin->getAudioInCount(), 2U); --i>=0;)
            {
                range = FloatVectorOperations::findMinAndMax(audioBuffers[i], numSamples);
                inPeaks[i] = carla_maxLimited<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);
            }

            fPlugin->process(const_cast<const float**>(audioBuffers), audioBuffers, nullptr, nullptr, static_cast<uint32_t>(numSamples));

            for (int i=jmin(fPlugin->getAudioOutCount(), 2U); --i>=0;)
            {
                range = FloatVectorOperations::findMinAndMax(audioBuffers[i], numSamples);
                outPeaks[i] = carla_maxLimited<float>(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);
            }

            kEngine->setPluginPeaks(fPlugin->getId(), inPeaks, outPeaks);
        }
        else
        {
            fPlugin->process(nullptr, nullptr, nullptr, nullptr, static_cast<uint32_t>(numSamples));
        }

        midi.clear();

        if (CarlaEngineEventPort* const port = fPlugin->getDefaultEventOutPort())
        {
            /*const*/ EngineEvent* const engineEvents(port->fBuffer);
            CARLA_SAFE_ASSERT_RETURN(engineEvents != nullptr,);

            fillJuceMidiBufferFromEngineEvents(midi, engineEvents);
            carla_zeroStructs(engineEvents, kMaxEngineEventInternalCount);
        }

        fPlugin->unlock();
    }

    const String getInputChannelName(int i)  const override
    {
        CARLA_SAFE_ASSERT_RETURN(i >= 0, String());
        CarlaEngineClient* const client(fPlugin->getEngineClient());
        return client->getAudioPortName(true, static_cast<uint>(i));
    }

    const String getOutputChannelName(int i) const override
    {
        CARLA_SAFE_ASSERT_RETURN(i >= 0, String());
        CarlaEngineClient* const client(fPlugin->getEngineClient());
        return client->getAudioPortName(false, static_cast<uint>(i));
    }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    const String getParameterName(int)         override { return String(); }
          String getParameterName(int, int)    override { return String(); }
    const String getParameterText(int)         override { return String(); }
          String getParameterText(int, int)    override { return String(); }
    const String getProgramName(int)           override { return String(); }

    double getTailLengthSeconds()        const override { return 0.0;  }
    float  getParameter(int)                   override { return 0.0f; }

    bool isInputChannelStereoPair(int)   const override { return false; }
    bool isOutputChannelStereoPair(int)  const override { return false; }
    bool silenceInProducesSilenceOut()   const override { return true;  }
    bool acceptsMidi()                   const override { return fPlugin->getDefaultEventInPort()  != nullptr; }
    bool producesMidi()                  const override { return fPlugin->getDefaultEventOutPort() != nullptr; }

    void setParameter(int, float)              override {}
    void setCurrentProgram(int)                override {}
    void changeProgramName(int, const String&) override {}
    void getStateInformation(MemoryBlock&)     override {}
    void setStateInformation(const void*, int) override {}

    int getNumParameters()                     override { return 0; }
    int getNumPrograms()                       override { return 0; }
    int getCurrentProgram()                    override { return 0; }

#ifndef JUCE_AUDIO_PROCESSOR_NO_GUI
    bool hasEditor()                     const override { return false; }
    AudioProcessorEditor* createEditor()       override { return nullptr; }
#endif

    // -------------------------------------------------------------------

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* fPlugin;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginInstance)
};

// -----------------------------------------------------------------------
// Patchbay Graph

PatchbayGraph::PatchbayGraph(CarlaEngine* const engine, const uint32_t ins, const uint32_t outs)
    : connections(),
      graph(),
      audioBuffer(),
      midiBuffer(),
      inputs(carla_fixedValue(0U, MAX_PATCHBAY_PLUGINS-2, ins)),
      outputs(carla_fixedValue(0U, MAX_PATCHBAY_PLUGINS-2, outs)),
      retCon(),
      usingExternal(false),
      extGraph(engine),
      kEngine(engine)
{
    const int    bufferSize(static_cast<int>(engine->getBufferSize()));
    const double sampleRate(engine->getSampleRate());

    graph.setPlayConfigDetails(static_cast<int>(inputs), static_cast<int>(outputs), sampleRate, bufferSize);
    graph.prepareToPlay(sampleRate, bufferSize);

    audioBuffer.setSize(static_cast<int>(jmax(inputs, outputs)), bufferSize);

    midiBuffer.ensureSize(kMaxEngineEventInternalCount*2);
    midiBuffer.clear();

    {
        AudioProcessorGraph::AudioGraphIOProcessor* const proc(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.set("isPlugin", false);
        node->properties.set("isOutput", false);
        node->properties.set("isAudio", true);
        node->properties.set("isCV", false);
        node->properties.set("isMIDI", false);
        node->properties.set("isOSC", false);
    }

    {
        AudioProcessorGraph::AudioGraphIOProcessor* const proc(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.set("isPlugin", false);
        node->properties.set("isOutput", false);
        node->properties.set("isAudio", true);
        node->properties.set("isCV", false);
        node->properties.set("isMIDI", false);
        node->properties.set("isOSC", false);
    }

    {
        AudioProcessorGraph::AudioGraphIOProcessor* const proc(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.set("isPlugin", false);
        node->properties.set("isOutput", false);
        node->properties.set("isAudio", false);
        node->properties.set("isCV", false);
        node->properties.set("isMIDI", true);
        node->properties.set("isOSC", false);
    }

    {
        AudioProcessorGraph::AudioGraphIOProcessor* const proc(new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode));
        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.set("isPlugin", false);
        node->properties.set("isOutput", true);
        node->properties.set("isAudio", false);
        node->properties.set("isCV", false);
        node->properties.set("isMIDI", true);
        node->properties.set("isOSC", false);
    }
}

PatchbayGraph::~PatchbayGraph()
{
    connections.clear();
    extGraph.clear();

    graph.releaseResources();
    graph.clear();
    audioBuffer.clear();
}

void PatchbayGraph::setBufferSize(const uint32_t bufferSize)
{
    const int bufferSizei(static_cast<int>(bufferSize));

    graph.releaseResources();
    graph.prepareToPlay(kEngine->getSampleRate(), bufferSizei);
    audioBuffer.setSize(audioBuffer.getNumChannels(), bufferSizei);
}

void PatchbayGraph::setSampleRate(const double sampleRate)
{
    graph.releaseResources();
    graph.prepareToPlay(sampleRate, static_cast<int>(kEngine->getBufferSize()));
}

void PatchbayGraph::setOffline(const bool offline)
{
    graph.setNonRealtime(offline);
}

void PatchbayGraph::addPlugin(CarlaPlugin* const plugin)
{
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("PatchbayGraph::addPlugin(%p)", plugin);

    CarlaPluginInstance* const instance(new CarlaPluginInstance(kEngine, plugin));
    AudioProcessorGraph::Node* const node(graph.addNode(instance));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    plugin->setPatchbayNodeId(node->nodeId);

    node->properties.set("isPlugin", true);
    node->properties.set("pluginId", static_cast<int>(plugin->getId()));

    if (! usingExternal)
        addNodeToPatchbay(plugin->getEngine(), node->nodeId, static_cast<int>(plugin->getId()), instance);
}

void PatchbayGraph::replacePlugin(CarlaPlugin* const oldPlugin, CarlaPlugin* const newPlugin)
{
    CARLA_SAFE_ASSERT_RETURN(oldPlugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newPlugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(oldPlugin != newPlugin,);
    CARLA_SAFE_ASSERT_RETURN(oldPlugin->getId() == newPlugin->getId(),);

    AudioProcessorGraph::Node* const oldNode(graph.getNodeForId(oldPlugin->getPatchbayNodeId()));
    CARLA_SAFE_ASSERT_RETURN(oldNode != nullptr,);

    if (! usingExternal)
    {
        disconnectInternalGroup(oldNode->nodeId);
        removeNodeFromPatchbay(kEngine, oldNode->nodeId, oldNode->getProcessor());
    }

    ((CarlaPluginInstance*)oldNode->getProcessor())->invalidatePlugin();

    graph.removeNode(oldNode->nodeId);

    CarlaPluginInstance* const instance(new CarlaPluginInstance(kEngine, newPlugin));
    AudioProcessorGraph::Node* const node(graph.addNode(instance));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    newPlugin->setPatchbayNodeId(node->nodeId);

    node->properties.set("isPlugin", true);
    node->properties.set("pluginId", static_cast<int>(newPlugin->getId()));

    if (! usingExternal)
        addNodeToPatchbay(newPlugin->getEngine(), node->nodeId, static_cast<int>(newPlugin->getId()), instance);
}

void PatchbayGraph::removePlugin(CarlaPlugin* const plugin)
{
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    carla_debug("PatchbayGraph::removePlugin(%p)", plugin);

    AudioProcessorGraph::Node* const node(graph.getNodeForId(plugin->getPatchbayNodeId()));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    if (! usingExternal)
    {
        disconnectInternalGroup(node->nodeId);
        removeNodeFromPatchbay(kEngine, node->nodeId, node->getProcessor());
    }

    ((CarlaPluginInstance*)node->getProcessor())->invalidatePlugin();

    // Fix plugin Ids properties
    for (uint i=plugin->getId()+1, count=kEngine->getCurrentPluginCount(); i<count; ++i)
    {
        CarlaPlugin* const plugin2(kEngine->getPlugin(i));
        CARLA_SAFE_ASSERT_BREAK(plugin2 != nullptr);

        if (AudioProcessorGraph::Node* const node2 = graph.getNodeForId(plugin2->getPatchbayNodeId()))
        {
            CARLA_SAFE_ASSERT_CONTINUE(node2->properties.getWithDefault("pluginId", -1) != juce::var(-1));
            node2->properties.set("pluginId", static_cast<int>(i-1));
        }
    }

    CARLA_SAFE_ASSERT_RETURN(graph.removeNode(node->nodeId),);
}

void PatchbayGraph::removeAllPlugins()
{
    carla_debug("PatchbayGraph::removeAllPlugins()");

    for (uint i=0, count=kEngine->getCurrentPluginCount(); i<count; ++i)
    {
        CarlaPlugin* const plugin(kEngine->getPlugin(i));
        CARLA_SAFE_ASSERT_CONTINUE(plugin != nullptr);

        AudioProcessorGraph::Node* const node(graph.getNodeForId(plugin->getPatchbayNodeId()));
        CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

        if (! usingExternal)
        {
            disconnectInternalGroup(node->nodeId);
            removeNodeFromPatchbay(kEngine, node->nodeId, node->getProcessor());
        }

        ((CarlaPluginInstance*)node->getProcessor())->invalidatePlugin();

        graph.removeNode(node->nodeId);
    }
}

bool PatchbayGraph::connect(const bool external, const uint groupA, const uint portA, const uint groupB, const uint portB, const bool sendCallback)
{
    if (external)
        return extGraph.connect(groupA, portA, groupB, portB, sendCallback);

    uint adjustedPortA = portA;
    uint adjustedPortB = portB;

    if (! adjustPatchbayPortIdForJuce(adjustedPortA))
        return false;
    if (! adjustPatchbayPortIdForJuce(adjustedPortB))
        return false;

    if (! graph.addConnection(groupA, static_cast<int>(adjustedPortA), groupB, static_cast<int>(adjustedPortB)))
    {
        kEngine->setLastError("Failed from juce");
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

bool PatchbayGraph::disconnect(const uint connectionId)
{
    if (usingExternal)
        return extGraph.disconnect(connectionId);

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin2(); it.valid(); it.next())
    {
        static const ConnectionToId fallback = { 0, 0, 0, 0, 0 };

        const ConnectionToId& connectionToId(it.getValue(fallback));
        CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id > 0);

        if (connectionToId.id != connectionId)
            continue;

        uint adjustedPortA = connectionToId.portA;
        uint adjustedPortB = connectionToId.portB;

        if (! adjustPatchbayPortIdForJuce(adjustedPortA))
            return false;
        if (! adjustPatchbayPortIdForJuce(adjustedPortB))
            return false;

        if (! graph.removeConnection(connectionToId.groupA, static_cast<int>(adjustedPortA),
                                     connectionToId.groupB, static_cast<int>(adjustedPortB)))
            return false;

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connectionToId.id, 0, 0, 0.0f, nullptr);

        connections.list.remove(it);
        return true;
    }

    kEngine->setLastError("Failed to find connection");
    return false;
}

void PatchbayGraph::disconnectInternalGroup(const uint groupId) noexcept
{
    CARLA_SAFE_ASSERT(! usingExternal);

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin2(); it.valid(); it.next())
    {
        static const ConnectionToId fallback = { 0, 0, 0, 0, 0 };

        const ConnectionToId& connectionToId(it.getValue(fallback));
        CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id > 0);

        if (connectionToId.groupA != groupId && connectionToId.groupB != groupId)
            continue;

        /*
        uint adjustedPortA = connectionToId.portA;
        uint adjustedPortB = connectionToId.portB;

        if (! adjustPatchbayPortIdForJuce(adjustedPortA))
            return false;
        if (! adjustPatchbayPortIdForJuce(adjustedPortB))
            return false;

        graph.removeConnection(connectionToId.groupA, static_cast<int>(adjustedPortA),
                               connectionToId.groupB, static_cast<int>(adjustedPortB));
        */

        if (! usingExternal)
            kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connectionToId.id, 0, 0, 0.0f, nullptr);

        connections.list.remove(it);
    }
}

void PatchbayGraph::refresh(const char* const deviceName)
{
    if (usingExternal)
        return extGraph.refresh(deviceName);

    CARLA_SAFE_ASSERT_RETURN(deviceName != nullptr,);

    connections.clear();
    graph.removeIllegalConnections();

    for (int i=0, count=graph.getNumNodes(); i<count; ++i)
    {
        AudioProcessorGraph::Node* const node(graph.getNode(i));
        CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

        AudioProcessor* const proc(node->getProcessor());
        CARLA_SAFE_ASSERT_CONTINUE(proc != nullptr);

        int clientId = -1;

        // plugin node
        if (node->properties.getWithDefault("isPlugin", false) == juce::var(true))
            clientId = node->properties.getWithDefault("pluginId", -1);

        addNodeToPatchbay(kEngine, node->nodeId, clientId, proc);
    }

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    for (int i=0, count=graph.getNumConnections(); i<count; ++i)
    {
        const AudioProcessorGraph::Connection* const conn(graph.getConnection(i));
        CARLA_SAFE_ASSERT_CONTINUE(conn != nullptr);
        CARLA_SAFE_ASSERT_CONTINUE(conn->sourceChannelIndex >= 0);
        CARLA_SAFE_ASSERT_CONTINUE(conn->destChannelIndex >= 0);

        const uint groupA = conn->sourceNodeId;
        const uint groupB = conn->destNodeId;

        uint portA = static_cast<uint>(conn->sourceChannelIndex);
        uint portB = static_cast<uint>(conn->destChannelIndex);

        if (portA == kMidiChannelIndex)
            portA  = kMidiOutputPortOffset;
        else
            portA += kAudioOutputPortOffset;

        if (portB == kMidiChannelIndex)
            portB  = kMidiInputPortOffset;
        else
            portB += kAudioInputPortOffset;

        ConnectionToId connectionToId;
        connectionToId.setData(++connections.lastId, groupA, portA, groupB, portB);

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", groupA, portA, groupB, portB);

        kEngine->callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

        connections.list.append(connectionToId);
    }
}

const char* const* PatchbayGraph::getConnections(const bool external) const
{
    if (external)
        return extGraph.getConnections();

    if (connections.list.count() == 0)
        return nullptr;

    CarlaStringList connList;

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin2(); it.valid(); it.next())
    {
        static const ConnectionToId fallback = { 0, 0, 0, 0, 0 };

        const ConnectionToId& connectionToId(it.getValue(fallback));
        CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id > 0);

        AudioProcessorGraph::Node* const nodeA(graph.getNodeForId(connectionToId.groupA));
        CARLA_SAFE_ASSERT_CONTINUE(nodeA != nullptr);

        AudioProcessorGraph::Node* const nodeB(graph.getNodeForId(connectionToId.groupB));
        CARLA_SAFE_ASSERT_CONTINUE(nodeB != nullptr);

        AudioProcessor* const procA(nodeA->getProcessor());
        CARLA_SAFE_ASSERT_CONTINUE(procA != nullptr);

        AudioProcessor* const procB(nodeB->getProcessor());
        CARLA_SAFE_ASSERT_CONTINUE(procB != nullptr);

        String fullPortNameA(getProcessorFullPortName(procA, connectionToId.portA));
        CARLA_SAFE_ASSERT_CONTINUE(fullPortNameA.isNotEmpty());

        String fullPortNameB(getProcessorFullPortName(procB, connectionToId.portB));
        CARLA_SAFE_ASSERT_CONTINUE(fullPortNameB.isNotEmpty());

        connList.append(fullPortNameA.toRawUTF8());
        connList.append(fullPortNameB.toRawUTF8());
    }

    if (connList.count() == 0)
        return nullptr;

    retCon = connList.toCharStringListPtr();

    return retCon;
}

bool PatchbayGraph::getGroupAndPortIdFromFullName(const bool external, const char* const fullPortName, uint& groupId, uint& portId) const
{
    if (external)
        return extGraph.getGroupAndPortIdFromFullName(fullPortName, groupId, portId);

    String groupName(String(fullPortName).upToFirstOccurrenceOf(":", false, false));
    String portName(String(fullPortName).fromFirstOccurrenceOf(":", false, false));

    for (int i=0, count=graph.getNumNodes(); i<count; ++i)
    {
        AudioProcessorGraph::Node* const node(graph.getNode(i));
        CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

        AudioProcessor* const proc(node->getProcessor());
        CARLA_SAFE_ASSERT_CONTINUE(proc != nullptr);

        if (proc->getName() != groupName)
            continue;

        groupId = node->nodeId;

        if (portName == "events-in")
        {
            portId = kMidiInputPortOffset;
            return true;
        }

        if (portName == "events-out")
        {
            portId = kMidiOutputPortOffset;
            return true;
        }

        for (int j=0, numInputs=proc->getNumInputChannels(); j<numInputs; ++j)
        {
            if (proc->getInputChannelName(j) != portName)
                continue;

            portId = kAudioInputPortOffset+static_cast<uint>(j);
            return true;
        }

        for (int j=0, numOutputs=proc->getNumOutputChannels(); j<numOutputs; ++j)
        {
            if (proc->getOutputChannelName(j) != portName)
                continue;

            portId = kAudioOutputPortOffset+static_cast<uint>(j);
            return true;
        }
    }

    return false;
}

void PatchbayGraph::process(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const int frames)
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.out != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(frames > 0,);

    // put events in juce buffer
    {
        midiBuffer.clear();
        fillJuceMidiBufferFromEngineEvents(midiBuffer, data->events.in);
    }

    // put carla audio in juce buffer
    {
        int i=0;

        for (; i < static_cast<int>(inputs); ++i)
            FloatVectorOperations::copy(audioBuffer.getWritePointer(i), inBuf[i], frames);

        // clear remaining channels
        for (const int count=audioBuffer.getNumChannels(); i<count; ++i)
            audioBuffer.clear(i, 0, frames);
    }

    graph.processBlock(audioBuffer, midiBuffer);

    // put juce audio in carla buffer
    {
        for (int i=0; i < static_cast<int>(outputs); ++i)
            FloatVectorOperations::copy(outBuf[i], audioBuffer.getReadPointer(i), frames);
    }

    // put juce events in carla buffer
    {
        carla_zeroStructs(data->events.out, kMaxEngineEventInternalCount);
        fillEngineEventsFromJuceMidiBuffer(data->events.out, midiBuffer);
        midiBuffer.clear();
    }
}

// -----------------------------------------------------------------------
// InternalGraph

EngineInternalGraph::EngineInternalGraph(CarlaEngine* const engine) noexcept
    : fIsRack(true),
      fIsReady(false),
      kEngine(engine)
{
    fRack = nullptr;
}

EngineInternalGraph::~EngineInternalGraph() noexcept
{
    CARLA_SAFE_ASSERT(! fIsReady);
    CARLA_SAFE_ASSERT(fRack == nullptr);
}

void EngineInternalGraph::create(const uint32_t inputs, const uint32_t outputs)
{
    fIsRack = (kEngine->getOptions().processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK);

    if (fIsRack)
    {
        CARLA_SAFE_ASSERT_RETURN(fRack == nullptr,);
        fRack = new RackGraph(kEngine, inputs, outputs);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fPatchbay == nullptr,);
        fPatchbay = new PatchbayGraph(kEngine, inputs, outputs);
    }

    fIsReady = true;
}

void EngineInternalGraph::destroy() noexcept
{
    if (! fIsReady)
    {
        CARLA_SAFE_ASSERT(fRack == nullptr);
        return;
    }

    if (fIsRack)
    {
        CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
        delete fRack;
        fRack = nullptr;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
        delete fPatchbay;
        fPatchbay = nullptr;
    }

    fIsReady = false;
}

void EngineInternalGraph::setBufferSize(const uint32_t bufferSize)
{
    ScopedValueSetter<bool> svs(fIsReady, false, true);

    if (fIsRack)
    {
        CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
        fRack->setBufferSize(bufferSize);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
        fPatchbay->setBufferSize(bufferSize);
    }
}

void EngineInternalGraph::setSampleRate(const double sampleRate)
{
    ScopedValueSetter<bool> svs(fIsReady, false, true);

    if (fIsRack)
    {
        CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
        fPatchbay->setSampleRate(sampleRate);
    }
}

void EngineInternalGraph::setOffline(const bool offline)
{
    ScopedValueSetter<bool> svs(fIsReady, false, true);

    if (fIsRack)
    {
        CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
        fRack->setOffline(offline);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
        fPatchbay->setOffline(offline);
    }
}

bool EngineInternalGraph::isReady() const noexcept
{
    return fIsReady;
}

RackGraph* EngineInternalGraph::getRackGraph() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fIsRack, nullptr);

    return fRack;
}

PatchbayGraph* EngineInternalGraph::getPatchbayGraph() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! fIsRack, nullptr);

    return fPatchbay;
}

void EngineInternalGraph::process(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const uint32_t frames)
{
    if (fIsRack)
    {
        CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);
        fRack->processHelper(data, inBuf, outBuf, frames);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
        fPatchbay->process(data, inBuf, outBuf, static_cast<int>(frames));
    }
}

void EngineInternalGraph::processRack(CarlaEngine::ProtectedData* const data, const float* inBuf[2], float* outBuf[2], const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(fIsRack,);
    CARLA_SAFE_ASSERT_RETURN(fRack != nullptr,);

    fRack->process(data, inBuf, outBuf, frames);
}

// -----------------------------------------------------------------------
// used for internal patchbay mode

void EngineInternalGraph::addPlugin(CarlaPlugin* const plugin)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->addPlugin(plugin);
}

void EngineInternalGraph::replacePlugin(CarlaPlugin* const oldPlugin, CarlaPlugin* const newPlugin)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->replacePlugin(oldPlugin, newPlugin);
}

void EngineInternalGraph::removePlugin(CarlaPlugin* const plugin)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->removePlugin(plugin);
}

void EngineInternalGraph::removeAllPlugins()
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->removeAllPlugins();
}

bool EngineInternalGraph::isUsingExternal() const noexcept
{
    if (fIsRack)
        return true;

    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr, false);
    return fPatchbay->usingExternal;
}

void EngineInternalGraph::setUsingExternal(const bool usingExternal) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->usingExternal = usingExternal;
}

// -----------------------------------------------------------------------
// CarlaEngine Patchbay stuff

bool CarlaEngine::patchbayConnect(const uint groupA, const uint portA, const uint groupB, const uint portB)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);
    carla_debug("CarlaEngine::patchbayConnect(%u, %u, %u, %u)", groupA, portA, groupB, portB);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        RackGraph* const graph = pData->graph.getRackGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        return graph->connect(groupA, portA, groupB, portB);
    }
    else
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        return graph->connect(graph->usingExternal, groupA, portA, groupB, portB, true);
    }

    return false;
}

bool CarlaEngine::patchbayDisconnect(const uint connectionId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);
    carla_debug("CarlaEngine::patchbayDisconnect(%u)", connectionId);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        RackGraph* const graph = pData->graph.getRackGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        return graph->disconnect(connectionId);
    }
    else
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        return graph->disconnect(connectionId);
    }

    return false;
}

bool CarlaEngine::patchbayRefresh(const bool external)
{
    // subclasses should handle this
    CARLA_SAFE_ASSERT_RETURN(! external, false);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        // This is implemented in engine subclasses
        setLastError("Unsupported operation");
        return false;
    }

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        graph->refresh("");
        return true;
    }

    setLastError("Unsupported operation");
    return false;
}

// -----------------------------------------------------------------------

const char* const* CarlaEngine::getPatchbayConnections(const bool external) const
{
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), nullptr);
    carla_debug("CarlaEngine::getPatchbayConnections(%s)", bool2str(external));

    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        RackGraph* const graph = pData->graph.getRackGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(external, nullptr);

        return graph->getConnections();
    }
    else
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, nullptr);

        return graph->getConnections(external);
    }

    return nullptr;
}

void CarlaEngine::restorePatchbayConnection(const bool external, const char* const sourcePort, const char* const targetPort, const bool sendCallback)
{
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(),);
    CARLA_SAFE_ASSERT_RETURN(sourcePort != nullptr && sourcePort[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(targetPort != nullptr && targetPort[0] != '\0',);
    carla_debug("CarlaEngine::restorePatchbayConnection(%s, \"%s\", \"%s\")", bool2str(external), sourcePort, targetPort);

    uint groupA, portA;
    uint groupB, portB;

    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        RackGraph* const graph = pData->graph.getRackGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(external,);

        if (! graph->getGroupAndPortIdFromFullName(sourcePort, groupA, portA))
            return;
        if (! graph->getGroupAndPortIdFromFullName(targetPort, groupB, portB))
            return;

        graph->connect(groupA, portA, groupB, portB);
    }
    else
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr,);

        if (! graph->getGroupAndPortIdFromFullName(external, sourcePort, groupA, portA))
            return;
        if (! graph->getGroupAndPortIdFromFullName(external, targetPort, groupB, portB))
            return;

        graph->connect(external, groupA, portA, groupB, portB, sendCallback);
    }
}

// -----------------------------------------------------------------------

bool CarlaEngine::connectExternalGraphPort(const uint connectionType, const uint portId, const char* const portName)
{
    CARLA_SAFE_ASSERT_RETURN(connectionType != 0 || (portName != nullptr && portName[0] != '\0'), false);
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK, false);

    RackGraph* const graph(pData->graph.getRackGraph());
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

    RackGraph* const graph(pData->graph.getRackGraph());
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

// -----------------------------------------------------------------------
