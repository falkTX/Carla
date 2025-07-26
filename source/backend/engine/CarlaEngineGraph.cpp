// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaEngineGraph.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaMathUtils.hpp"
#include "CarlaScopeUtils.hpp"

#include "CarlaMIDI.h"

using water::jmax;
using water::jmin;
using water::AudioProcessor;
using water::MidiBuffer;

#define MAX_GRAPH_AUDIO_IO 64U
#define MAX_GRAPH_CV_IO 32U

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Fallback data

static const PortNameToId kPortNameToIdFallback   = { 0, 0, { '\0' }, { '\0' }, { '\0' } };
static /* */ PortNameToId kPortNameToIdFallbackNC = { 0, 0, { '\0' }, { '\0' }, { '\0' } };

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

uint ExternalGraphPorts::getPortIdFromName(const bool isInput, const char name[], bool* const ok) const noexcept
{
    for (LinkedList<PortNameToId>::Itenerator it = isInput ? ins.begin2() : outs.begin2(); it.valid(); it.next())
    {
        const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

        if (std::strncmp(portNameToId.name, name, STR_MAX) == 0)
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

uint ExternalGraphPorts::getPortIdFromIdentifier(const bool isInput, const char identifier[], bool* const ok) const noexcept
{
    for (LinkedList<PortNameToId>::Itenerator it = isInput ? ins.begin2() : outs.begin2(); it.valid(); it.next())
    {
        const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

        if (std::strncmp(portNameToId.identifier, identifier, STR_MAX) == 0)
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
      positions(),
      retCon(),
      kEngine(engine)
{
    carla_zeroStruct(positions);
}

void ExternalGraph::clear() noexcept
{
    connections.clear();
    audioPorts.ins.clear();
    audioPorts.outs.clear();
    midiPorts.ins.clear();
    midiPorts.outs.clear();
}

bool ExternalGraph::connect(const bool sendHost, const bool sendOSC,
                            const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept
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

    kEngine->callback(sendHost, sendOSC,
                      ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0, 0.0f, strBuf);

    connections.list.append(connectionToId);
    return true;
}

bool ExternalGraph::disconnect(const bool sendHost, const bool sendOSC,
                               const uint connectionId) noexcept
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

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connectionToId.id, 0, 0, 0, 0.0f, nullptr);

        connections.list.remove(it);
        return true;
    }

    kEngine->setLastError("Failed to find connection");
    return false;
}

void ExternalGraph::setGroupPos(const bool sendHost, const bool sendOSC,
                                const uint groupId, const int x1, const int y1, const int x2, const int y2)
{
    CARLA_SAFE_ASSERT_UINT_RETURN(groupId >= kExternalGraphGroupCarla && groupId < kExternalGraphGroupMax, groupId,);
    carla_debug("ExternalGraph::setGroupPos(%s, %s, %u, %i, %i, %i, %i)", bool2str(sendHost), bool2str(sendOSC), groupId, x1, y1, x2, y2);

    const PatchbayPosition ppos = { true, x1, y1, x2, y2 };
    positions[groupId] = ppos;

    kEngine->callback(sendHost, sendOSC,
                      ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                      groupId, x1, y1, x2, static_cast<float>(y2),
                      nullptr);
}

void ExternalGraph::refresh(const bool sendHost, const bool sendOSC, const char* const deviceName)
{
    CARLA_SAFE_ASSERT_RETURN(deviceName != nullptr,);

    const bool isRack = kEngine->getOptions().processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK;

    // Main
    {
        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                          kExternalGraphGroupCarla,
                          PATCHBAY_ICON_CARLA,
                          MAIN_CARLA_PLUGIN_ID,
                          0, 0.0f,
                          kEngine->getName());

        if (isRack)
        {
            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupCarla,
                              kExternalGraphCarlaPortAudioIn1,
                              PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT,
                              0, 0.0f,
                              "audio-in1");

            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupCarla,
                              kExternalGraphCarlaPortAudioIn2,
                              PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT,
                              0, 0.0f,
                              "audio-in2");

            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupCarla,
                              kExternalGraphCarlaPortAudioOut1,
                              PATCHBAY_PORT_TYPE_AUDIO,
                              0, 0.0f,
                              "audio-out1");

            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupCarla,
                              kExternalGraphCarlaPortAudioOut2,
                              PATCHBAY_PORT_TYPE_AUDIO,
                              0, 0.0f,
                              "audio-out2");
        }

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                          kExternalGraphGroupCarla,
                          kExternalGraphCarlaPortMidiIn,
                          PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,
                          0, 0.0f,
                          "midi-in");

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                          kExternalGraphGroupCarla,
                          kExternalGraphCarlaPortMidiOut,
                          PATCHBAY_PORT_TYPE_MIDI,
                          0, 0.0f,
                          "midi-out");
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

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                          kExternalGraphGroupAudioIn,
                          PATCHBAY_ICON_HARDWARE,
                          -1,
                          0, 0.0f,
                          strBuf);

        const String groupNameIn(strBuf);

        for (LinkedList<PortNameToId>::Itenerator it = audioPorts.ins.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNameIn + portNameToId.name);

            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupAudioIn,
                              portNameToId.port,
                              PATCHBAY_PORT_TYPE_AUDIO,
                              0, 0.0f,
                              portNameToId.name);
        }

        // Audio Out
        if (deviceName[0] != '\0')
            std::snprintf(strBuf, STR_MAX, "Playback (%s)", deviceName);
        else
            std::strncpy(strBuf, "Playback", STR_MAX);

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                          kExternalGraphGroupAudioOut,
                          PATCHBAY_ICON_HARDWARE,
                          -1,
                          0, 0.0f,
                          strBuf);

        const String groupNameOut(strBuf);

        for (LinkedList<PortNameToId>::Itenerator it = audioPorts.outs.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNameOut + portNameToId.name);

            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupAudioOut,
                              portNameToId.port,
                              PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT,
                              0, 0.0f,
                              portNameToId.name);
        }
    }

    // MIDI In
    {
        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                          kExternalGraphGroupMidiIn,
                          PATCHBAY_ICON_HARDWARE,
                          -1,
                          0, 0.0f,
                          "Readable MIDI ports");

        const String groupNamePlus("Readable MIDI ports:");

        for (LinkedList<PortNameToId>::Itenerator it = midiPorts.ins.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNamePlus + portNameToId.name);

            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupMidiIn,
                              portNameToId.port,
                              PATCHBAY_PORT_TYPE_MIDI,
                              0, 0.0f,
                              portNameToId.name);
        }
    }

    // MIDI Out
    {
        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                          kExternalGraphGroupMidiOut,
                          PATCHBAY_ICON_HARDWARE,
                          -1,
                          0, 0.0f,
                          "Writable MIDI ports");

        const String groupNamePlus("Writable MIDI ports:");

        for (LinkedList<PortNameToId>::Itenerator it = midiPorts.outs.begin2(); it.valid(); it.next())
        {
            PortNameToId& portNameToId(it.getValue(kPortNameToIdFallbackNC));
            CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group > 0);

            portNameToId.setFullName(groupNamePlus + portNameToId.name);

            kEngine->callback(sendHost, sendOSC,
                              ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                              kExternalGraphGroupMidiOut,
                              portNameToId.port,
                              PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,
                              0, 0.0f,
                              portNameToId.name);
        }
    }

    // positions
    for (uint i=kExternalGraphGroupCarla; i<kExternalGraphGroupMax; ++i)
    {
        const PatchbayPosition& eppos(positions[i]);

        if (! eppos.active)
            continue;

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                          i, eppos.x1, eppos.y1, eppos.x2, static_cast<float>(eppos.y2),
                          nullptr);
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

bool ExternalGraph::getGroupFromName(const char* groupName, uint& groupId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(groupName != nullptr && groupName[0] != '\0', false);

    if (std::strcmp(groupName, "Carla") == 0)
    {
        groupId = kExternalGraphGroupCarla;
        return true;
    }
    if (std::strcmp(groupName, "AudioIn") == 0)
    {
        groupId = kExternalGraphGroupAudioIn;
        return true;
    }
    if (std::strcmp(groupName, "AudioOut") == 0)
    {
        groupId = kExternalGraphGroupAudioOut;
        return true;
    }
    if (std::strcmp(groupName, "MidiIn") == 0)
    {
        groupId = kExternalGraphGroupMidiIn;
        return true;
    }
    if (std::strcmp(groupName, "MidiOut") == 0)
    {
        groupId = kExternalGraphGroupMidiOut;
        return true;
    }

    return false;
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
            portId = audioPorts.getPortIdFromName(true, portName, &ok);
            return ok;
        }
    }
    else if (std::strncmp(fullPortName, "AudioOut:", 9) == 0)
    {
        groupId = kExternalGraphGroupAudioOut;

        if (const char* const portName = fullPortName+9)
        {
            bool ok;
            portId = audioPorts.getPortIdFromName(false, portName, &ok);
            return ok;
        }
    }
    else if (std::strncmp(fullPortName, "MidiIn:", 7) == 0)
    {
        groupId = kExternalGraphGroupMidiIn;

        if (const char* const portName = fullPortName+7)
        {
            bool ok;
            portId = midiPorts.getPortIdFromName(true, portName, &ok);
            return ok;
        }
    }
    else if (std::strncmp(fullPortName, "MidiOut:", 8) == 0)
    {
        groupId = kExternalGraphGroupMidiOut;

        if (const char* const portName = fullPortName+8)
        {
            bool ok;
            portId = midiPorts.getPortIdFromName(false, portName, &ok);
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
      connectedOut2(),
#ifdef CARLA_PROPER_CPP11_SUPPORT
      inBuf{nullptr, nullptr},
      inBufTmp{nullptr, nullptr},
      outBuf{nullptr, nullptr},
#endif
      unusedBuf(nullptr)
    {
#ifndef CARLA_PROPER_CPP11_SUPPORT
        inBuf[0]    = inBuf[1]    = nullptr;
        inBufTmp[0] = inBufTmp[1] = nullptr;
        outBuf[0]   = outBuf[1]   = nullptr;
#endif
    }

RackGraph::Buffers::~Buffers() noexcept
{
    const CarlaRecursiveMutexLocker cml(mutex);

    if (inBuf[0]    != nullptr) { delete[] inBuf[0];    inBuf[0]    = nullptr; }
    if (inBuf[1]    != nullptr) { delete[] inBuf[1];    inBuf[1]    = nullptr; }
    if (inBufTmp[0] != nullptr) { delete[] inBufTmp[0]; inBufTmp[0] = nullptr; }
    if (inBufTmp[1] != nullptr) { delete[] inBufTmp[1]; inBufTmp[1] = nullptr; }
    if (outBuf[0]   != nullptr) { delete[] outBuf[0];   outBuf[0]   = nullptr; }
    if (outBuf[1]   != nullptr) { delete[] outBuf[1];   outBuf[1]   = nullptr; }
    if (unusedBuf   != nullptr) { delete[] unusedBuf;   unusedBuf   = nullptr; }

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
    if (unusedBuf   != nullptr) { delete[] unusedBuf;   unusedBuf   = nullptr; }

    CARLA_SAFE_ASSERT_RETURN(bufferSize > 0,);

    try {
        inBufTmp[0] = new float[bufferSize];
        inBufTmp[1] = new float[bufferSize];
        unusedBuf   = new float[bufferSize];

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
        if (unusedBuf   != nullptr) { delete[] unusedBuf;   unusedBuf   = nullptr; }

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
    return extGraph.connect(true, true, groupA, portA, groupB, portB);
}

bool RackGraph::disconnect(const uint connectionId) noexcept
{
    return extGraph.disconnect(true, true, connectionId);
}

void RackGraph::refresh(const bool sendHost, const bool sendOSC, const bool, const char* const deviceName)
{
    extGraph.refresh(sendHost, sendOSC, deviceName);

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

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                          connectionToId.id,
                          0, 0, 0, 0.0f,
                          strBuf);

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

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                          connectionToId.id,
                          0, 0, 0, 0.0f,
                          strBuf);

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

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                          connectionToId.id,
                          0, 0, 0, 0.0f,
                          strBuf);

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

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                          connectionToId.id,
                          0, 0, 0, 0.0f,
                          strBuf);

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

void RackGraph::process(CarlaEngine::ProtectedData* const data, const float* inBufReal[2], float* outBufReal[2], const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.out != nullptr,);

    // safe copy
    float* const dummyBuf = audioBuffers.unusedBuf;
    float* const inBuf0   = audioBuffers.inBufTmp[0];
    float* const inBuf1   = audioBuffers.inBufTmp[1];

    // initialize audio inputs
    carla_copyFloats(inBuf0, inBufReal[0], frames);
    carla_copyFloats(inBuf1, inBufReal[1], frames);

    // initialize audio outputs (zero)
    carla_zeroFloats(outBufReal[0], frames);
    carla_zeroFloats(outBufReal[1], frames);

    // initialize event outputs (zero)
    carla_zeroStructs(data->events.out, kMaxEngineEventInternalCount);

    const float* inBuf[MAX_GRAPH_AUDIO_IO];
    float* outBuf[MAX_GRAPH_AUDIO_IO];
    float* cvBuf[MAX_GRAPH_CV_IO];

    uint32_t oldAudioInCount  = 0;
    uint32_t oldAudioOutCount = 0;
    uint32_t oldMidiOutCount  = 0;
    bool processed = false;

    // process plugins
    for (uint i=0; i < data->curPluginCount; ++i)
    {
        const CarlaPluginPtr plugin = data->plugins[i].plugin;

        if (plugin.get() == nullptr || ! plugin->isEnabled() || ! plugin->tryLock(isOffline))
            continue;

        if (processed)
        {
            // initialize audio inputs (from previous outputs)
            carla_copyFloats(inBuf0, outBufReal[0], frames);
            carla_copyFloats(inBuf1, outBufReal[1], frames);

            // initialize audio outputs (zero)
            carla_zeroFloats(outBufReal[0], frames);
            carla_zeroFloats(outBufReal[1], frames);

            // if plugin has no midi out, add previous events
            if (oldMidiOutCount == 0 && data->events.in[0].type != kEngineEventTypeNull)
            {
                if (data->events.out[0].type != kEngineEventTypeNull)
                {
                    // TODO: carefully add to input, sorted events
                    //carla_stderr("TODO midi event mixing here %s", plugin->getName());
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

        const uint32_t numInBufs  = std::max(oldAudioInCount,  2U);
        const uint32_t numOutBufs = std::max(oldAudioOutCount, 2U);
        const uint32_t numCvBufs  = std::max(plugin->getCVInCount(), plugin->getCVOutCount());

        CARLA_SAFE_ASSERT_RETURN(numInBufs <= MAX_GRAPH_AUDIO_IO, plugin->unlock());
        CARLA_SAFE_ASSERT_RETURN(numOutBufs <= MAX_GRAPH_AUDIO_IO, plugin->unlock());
        CARLA_SAFE_ASSERT_RETURN(numCvBufs <= MAX_GRAPH_CV_IO, plugin->unlock());

        inBuf[0] = inBuf0;
        inBuf[1] = inBuf1;

        outBuf[0] = outBufReal[0];
        outBuf[1] = outBufReal[1];

        for (uint32_t j=0; j<numCvBufs; ++j)
            cvBuf[j] = dummyBuf;

        if (numInBufs > 2 || numOutBufs > 2 || numCvBufs != 0)
        {
            carla_zeroFloats(dummyBuf, frames);

            for (uint32_t j=2; j<numInBufs; ++j)
                inBuf[j] = dummyBuf;

            for (uint32_t j=2; j<numOutBufs; ++j)
                outBuf[j] = dummyBuf;
        }

        // process
        plugin->initBuffers();
        plugin->process(inBuf, outBuf, cvBuf, cvBuf, frames);
        plugin->unlock();

        // if plugin has no audio inputs, add input buffer
        if (oldAudioInCount == 0)
        {
            carla_addFloats(outBufReal[0], inBuf0, frames);
            carla_addFloats(outBufReal[1], inBuf1, frames);
        }

        // if plugin only has 1 output, copy it to the 2nd
        if (oldAudioOutCount == 1)
        {
            carla_copyFloats(outBufReal[1], outBufReal[0], frames);
        }

        // set peaks
        {
            EnginePluginData& pluginData(data->plugins[i]);

            if (oldAudioInCount > 0)
            {
                pluginData.peaks[0] = carla_findMaxNormalizedFloat(inBuf0, frames);
                pluginData.peaks[1] = carla_findMaxNormalizedFloat(inBuf1, frames);
            }
            else
            {
                pluginData.peaks[0] = 0.0f;
                pluginData.peaks[1] = 0.0f;
            }

            if (oldAudioOutCount > 0)
            {
                pluginData.peaks[2] = carla_findMaxNormalizedFloat(outBufReal[0], frames);
                pluginData.peaks[3] = carla_findMaxNormalizedFloat(outBufReal[1], frames);
            }
            else
            {
                pluginData.peaks[2] = 0.0f;
                pluginData.peaks[3] = 0.0f;
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
// Patchbay Graph stuff

static const uint32_t kMaxPortsPerPlugin     = 255;
static const uint32_t kAudioInputPortOffset  = kMaxPortsPerPlugin*1;
static const uint32_t kAudioOutputPortOffset = kMaxPortsPerPlugin*2;
static const uint32_t kCVInputPortOffset     = kMaxPortsPerPlugin*3;
static const uint32_t kCVOutputPortOffset    = kMaxPortsPerPlugin*4;
static const uint32_t kMidiInputPortOffset   = kMaxPortsPerPlugin*5;
static const uint32_t kMidiOutputPortOffset  = kMaxPortsPerPlugin*6;
static const uint32_t kMaxPortOffset         = kMaxPortsPerPlugin*7;

static inline
bool adjustPatchbayPortIdForWater(AudioProcessor::ChannelType& channelType, uint& portId)
{
    CARLA_SAFE_ASSERT_RETURN(portId >= kAudioInputPortOffset, false);
    CARLA_SAFE_ASSERT_RETURN(portId < kMaxPortOffset, false);

    if (portId >= kMidiOutputPortOffset)
    {
        portId -= kMidiOutputPortOffset;
        channelType = AudioProcessor::ChannelTypeMIDI;
        return true;
    }
    if (portId >= kMidiInputPortOffset)
    {
        portId -= kMidiInputPortOffset;
        channelType = AudioProcessor::ChannelTypeMIDI;
        return true;
    }
    if (portId >= kCVOutputPortOffset)
    {
        portId -= kCVOutputPortOffset;
        channelType = AudioProcessor::ChannelTypeCV;
        return true;
    }
    if (portId >= kCVInputPortOffset)
    {
        portId -= kCVInputPortOffset;
        channelType = AudioProcessor::ChannelTypeCV;
        return true;
    }
    if (portId >= kAudioOutputPortOffset)
    {
        portId -= kAudioOutputPortOffset;
        channelType = AudioProcessor::ChannelTypeAudio;
        return true;
    }
    if (portId >= kAudioInputPortOffset)
    {
        portId -= kAudioInputPortOffset;
        channelType = AudioProcessor::ChannelTypeAudio;
        return true;
    }

    return false;
}

static inline
const water::String getProcessorFullPortName(AudioProcessor* const proc, const uint32_t portId)
{
    CARLA_SAFE_ASSERT_RETURN(proc != nullptr, {});
    CARLA_SAFE_ASSERT_RETURN(portId >= kAudioInputPortOffset, {});
    CARLA_SAFE_ASSERT_RETURN(portId < kMaxPortOffset, {});

    water::String fullPortName(proc->getName());

    /**/ if (portId >= kMidiOutputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeMIDI) > 0, {});
        fullPortName += ":" + proc->getOutputChannelName(AudioProcessor::ChannelTypeMIDI,
                                                         portId-kMidiOutputPortOffset);
    }
    else if (portId >= kMidiInputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeMIDI) > 0, {});
        fullPortName += ":" + proc->getInputChannelName(AudioProcessor::ChannelTypeMIDI,
                                                        portId-kMidiInputPortOffset);
    }
    else if (portId >= kCVOutputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV) > 0, {});
        fullPortName += ":" + proc->getOutputChannelName(AudioProcessor::ChannelTypeCV,
                                                         portId-kCVOutputPortOffset);
    }
    else if (portId >= kCVInputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeCV) > 0, {});
        fullPortName += ":" + proc->getInputChannelName(AudioProcessor::ChannelTypeCV,
                                                        portId-kCVInputPortOffset);
    }
    else if (portId >= kAudioOutputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeAudio) > 0, {});
        fullPortName += ":" + proc->getOutputChannelName(AudioProcessor::ChannelTypeAudio,
                                                         portId-kAudioOutputPortOffset);
    }
    else if (portId >= kAudioInputPortOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeAudio) > 0, {});
        fullPortName += ":" + proc->getInputChannelName(AudioProcessor::ChannelTypeAudio,
                                                        portId-kAudioInputPortOffset);
    }
    else
    {
        return {};
    }

    return fullPortName;
}

static inline
void addNodeToPatchbay(const bool sendHost, const bool sendOSC, CarlaEngine* const engine,
                       AudioProcessorGraph::Node* const node, const int pluginId, const AudioProcessor* const proc)
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(proc != nullptr,);

    const uint groupId = node->nodeId;

    engine->callback(sendHost, sendOSC,
                     ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED,
                     groupId,
                     pluginId >= 0 ? PATCHBAY_ICON_PLUGIN : PATCHBAY_ICON_HARDWARE,
                     pluginId,
                     0, 0.0f,
                     proc->getName().toRawUTF8());

    for (uint i=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeAudio); i<numInputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                         groupId,
                         static_cast<int>(kAudioInputPortOffset+i),
                         PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT,
                         0, 0.0f,
                         proc->getInputChannelName(AudioProcessor::ChannelTypeAudio, i).toRawUTF8());
    }

    for (uint i=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeAudio); i<numOutputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                         groupId,
                         static_cast<int>(kAudioOutputPortOffset+i),
                         PATCHBAY_PORT_TYPE_AUDIO,
                         0, 0.0f,
                         proc->getOutputChannelName(AudioProcessor::ChannelTypeAudio, i).toRawUTF8());
    }

    for (uint i=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeCV); i<numInputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                         groupId,
                         static_cast<int>(kCVInputPortOffset+i),
                         PATCHBAY_PORT_TYPE_CV|PATCHBAY_PORT_IS_INPUT,
                         0, 0.0f,
                         proc->getInputChannelName(AudioProcessor::ChannelTypeCV, i).toRawUTF8());
    }

    for (uint i=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV); i<numOutputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                         groupId,
                         static_cast<int>(kCVOutputPortOffset+i),
                         PATCHBAY_PORT_TYPE_CV,
                         0, 0.0f,
                         proc->getOutputChannelName(AudioProcessor::ChannelTypeCV, i).toRawUTF8());
    }

    for (uint i=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeMIDI); i<numInputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                         groupId,
                         static_cast<int>(kMidiInputPortOffset+i),
                         PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,
                         0, 0.0f,
                         proc->getInputChannelName(AudioProcessor::ChannelTypeMIDI, i).toRawUTF8());
    }

    for (uint i=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeMIDI); i<numOutputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                         groupId,
                         static_cast<int>(kMidiOutputPortOffset+i),
                         PATCHBAY_PORT_TYPE_MIDI,
                         0, 0.0f,
                         proc->getOutputChannelName(AudioProcessor::ChannelTypeMIDI, i).toRawUTF8());
    }

    if (node->properties.position.valid)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                         groupId,
                         node->properties.position.x1,
                         node->properties.position.y1,
                         node->properties.position.x2,
                         static_cast<float>(node->properties.position.y2),
                         nullptr);
    }
}

static inline
void removeNodeFromPatchbay(const bool sendHost, const bool sendOSC, CarlaEngine* const engine,
                            const uint32_t groupId, const AudioProcessor* const proc)
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(proc != nullptr,);

    for (uint i=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeAudio); i<numInputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                         groupId,
                         static_cast<int>(kAudioInputPortOffset+i),
                         0, 0, 0.0f, nullptr);
    }

    for (uint i=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeAudio); i<numOutputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                         groupId,
                         static_cast<int>(kAudioOutputPortOffset+i),
                         0, 0, 0.0f,
                         nullptr);
    }

    for (uint i=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeCV); i<numInputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                         groupId,
                         static_cast<int>(kCVInputPortOffset+i),
                         0, 0, 0.0f, nullptr);
    }

    for (uint i=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV); i<numOutputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                         groupId,
                         static_cast<int>(kCVOutputPortOffset+i),
                         0, 0, 0.0f,
                         nullptr);
    }

    for (uint i=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeMIDI); i<numInputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                         groupId,
                         static_cast<int>(kMidiInputPortOffset+i),
                         0, 0, 0.0f, nullptr);
    }

    for (uint i=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeMIDI); i<numOutputs; ++i)
    {
        engine->callback(sendHost, sendOSC,
                         ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                         groupId,
                         static_cast<int>(kMidiOutputPortOffset+i),
                         0, 0, 0.0f, nullptr);
    }

    engine->callback(sendHost, sendOSC,
                     ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED,
                     groupId,
                     0, 0, 0, 0.0f, nullptr);
}

// -----------------------------------------------------------------------

class CarlaPluginInstance : public AudioProcessor
{
public:
    CarlaPluginInstance(CarlaEngine* const engine, const CarlaPluginPtr plugin)
        : kEngine(engine),
          fPlugin(plugin)
    {
        CarlaEngineClient* const client = plugin->getEngineClient();

        setPlayConfigDetails(client->getPortCount(kEnginePortTypeAudio, true),
                             client->getPortCount(kEnginePortTypeAudio, false),
                             client->getPortCount(kEnginePortTypeCV, true),
                             client->getPortCount(kEnginePortTypeCV, false),
                             client->getPortCount(kEnginePortTypeEvent, true),
                             client->getPortCount(kEnginePortTypeEvent, false),
                             getSampleRate(), getBlockSize());
    }

    ~CarlaPluginInstance() override
    {
    }

    void reconfigure() override
    {
        const CarlaPluginPtr plugin = fPlugin;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr,);

        CarlaEngineClient* const client = plugin->getEngineClient();
        CARLA_SAFE_ASSERT_RETURN(client != nullptr,);

        carla_stdout("reconfigure called");

        setPlayConfigDetails(client->getPortCount(kEnginePortTypeAudio, true),
                             client->getPortCount(kEnginePortTypeAudio, false),
                             client->getPortCount(kEnginePortTypeCV, true),
                             client->getPortCount(kEnginePortTypeCV, false),
                             client->getPortCount(kEnginePortTypeEvent, true),
                             client->getPortCount(kEnginePortTypeEvent, false),
                             getSampleRate(), getBlockSize());
    }

    void invalidatePlugin() noexcept
    {
        fPlugin.reset();
    }

    // -------------------------------------------------------------------

    const water::String getName() const override
    {
        const CarlaPluginPtr plugin = fPlugin;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr, {});

        return plugin->getName();
    }

    void processBlockWithCV(AudioSampleBuffer& audio,
                            const AudioSampleBuffer& cvIn,
                            AudioSampleBuffer& cvOut,
                            MidiBuffer& midi) override
    {
        const CarlaPluginPtr plugin = fPlugin;

        if (plugin.get() == nullptr || !plugin->isEnabled() || !plugin->tryLock(kEngine->isOffline()))
        {
            audio.clear();
            cvOut.clear();
            midi.clear();
            return;
        }

        if (CarlaEngineEventPort* const port = plugin->getDefaultEventInPort())
        {
            EngineEvent* const engineEvents(port->fBuffer);
            CARLA_SAFE_ASSERT_RETURN(engineEvents != nullptr,);

            carla_zeroStructs(engineEvents, kMaxEngineEventInternalCount);
            fillEngineEventsFromWaterMidiBuffer(engineEvents, midi);
        }

        midi.clear();

        plugin->initBuffers();

        const uint32_t numSamples   = audio.getNumSamples();
        const uint32_t numAudioChan = audio.getNumChannels();
        const uint32_t numCVInChan  = cvIn.getNumChannels();
        const uint32_t numCVOutChan = cvOut.getNumChannels();

        if (numAudioChan+numCVInChan+numCVOutChan == 0)
        {
            // nothing to process
            plugin->process(nullptr, nullptr, nullptr, nullptr, numSamples);
        }
        else if (numAudioChan != 0)
        {
            // processing audio, include code for peaks
            const uint32_t numChan2 = jmin(numAudioChan, 2U);

            if (plugin->getAudioInCount() == 0)
                audio.clear();

            float* audioBuffers[MAX_GRAPH_AUDIO_IO];
            float* cvOutBuffers[MAX_GRAPH_CV_IO];
            const float* cvInBuffers[MAX_GRAPH_CV_IO];
            CARLA_SAFE_ASSERT_RETURN(numAudioChan <= MAX_GRAPH_AUDIO_IO, plugin->unlock());
            CARLA_SAFE_ASSERT_RETURN(numCVOutChan <= MAX_GRAPH_CV_IO, plugin->unlock());
            CARLA_SAFE_ASSERT_RETURN(numCVInChan <= MAX_GRAPH_CV_IO, plugin->unlock());

            for (uint32_t i=0; i<numAudioChan; ++i)
                audioBuffers[i] = audio.getWritePointer(i);
            for (uint32_t i=0; i<numCVOutChan; ++i)
                cvOutBuffers[i] = cvOut.getWritePointer(i);
            for (uint32_t i=0; i<numCVInChan; ++i)
                cvInBuffers[i] = cvIn.getReadPointer(i);

            float inPeaks[2] = { 0.0f };
            float outPeaks[2] = { 0.0f };

            for (uint32_t i=0, count=jmin(plugin->getAudioInCount(), numChan2); i<count; ++i)
                inPeaks[i] = carla_findMaxNormalizedFloat(audioBuffers[i], numSamples);

            plugin->process(const_cast<const float**>(audioBuffers), audioBuffers,
                            cvInBuffers, cvOutBuffers,
                            numSamples);

            for (uint32_t i=0, count=jmin(plugin->getAudioOutCount(), numChan2); i<count; ++i)
                outPeaks[i] = carla_findMaxNormalizedFloat(audioBuffers[i], numSamples);

            kEngine->setPluginPeaksRT(plugin->getId(), inPeaks, outPeaks);
        }
        else
        {
            // processing CV only, skip audiopeaks
            float* cvOutBuffers[MAX_GRAPH_CV_IO];
            const float* cvInBuffers[MAX_GRAPH_CV_IO];

            for (uint32_t i=0; i<numCVOutChan; ++i)
                cvOutBuffers[i] = cvOut.getWritePointer(i);
            for (uint32_t i=0; i<numCVInChan; ++i)
                cvInBuffers[i] = cvIn.getReadPointer(i);

            plugin->process(nullptr, nullptr,
                            cvInBuffers, cvOutBuffers,
                            numSamples);
        }

        midi.clear();

        if (CarlaEngineEventPort* const port = plugin->getDefaultEventOutPort())
        {
            /*const*/ EngineEvent* const engineEvents(port->fBuffer);
            CARLA_SAFE_ASSERT_RETURN(engineEvents != nullptr,);

            fillWaterMidiBufferFromEngineEvents(midi, engineEvents);
            carla_zeroStructs(engineEvents, kMaxEngineEventInternalCount);
        }

        plugin->unlock();
    }

    const water::String getInputChannelName(ChannelType t, uint i) const override
    {
        const CarlaPluginPtr plugin = fPlugin;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr, {});

        CarlaEngineClient* const client = plugin->getEngineClient();

        switch (t)
        {
        case ChannelTypeAudio:
            return client->getAudioPortName(true, i);
        case ChannelTypeCV:
            return client->getCVPortName(true, i);
        case ChannelTypeMIDI:
            return client->getEventPortName(true, i);
        }

        return {};
    }

    const water::String getOutputChannelName(ChannelType t, uint i) const override
    {
        const CarlaPluginPtr plugin = fPlugin;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr, {});

        CarlaEngineClient* const client(plugin->getEngineClient());

        switch (t)
        {
        case ChannelTypeAudio:
            return client->getAudioPortName(false, i);
        case ChannelTypeCV:
            return client->getCVPortName(false, i);
        case ChannelTypeMIDI:
            return client->getEventPortName(false, i);
        }

        return {};
    }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    bool acceptsMidi()  const override
    {
        const CarlaPluginPtr plugin = fPlugin;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr, false);

        return plugin->getDefaultEventInPort() != nullptr;
    }

    bool producesMidi() const override
    {
        const CarlaPluginPtr plugin = fPlugin;
        CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr, false);

        return plugin->getDefaultEventOutPort() != nullptr;
    }

private:
    CarlaEngine* const kEngine;
    CarlaPluginPtr fPlugin;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginInstance)
};

// -----------------------------------------------------------------------
// Patchbay Graph

class NamedAudioGraphIOProcessor : public AudioProcessorGraph::AudioGraphIOProcessor
{
public:
    NamedAudioGraphIOProcessor(const IODeviceType iotype)
        : AudioProcessorGraph::AudioGraphIOProcessor(iotype),
          inputNames(),
          outputNames() {}

    const water::String getInputChannelName (ChannelType, uint _index) const override
    {
        const int index = static_cast<int>(_index); // FIXME
        if (index < inputNames.size())
            return inputNames[index];
        return water::String("Playback ") + String(index+1);
    }

    const water::String getOutputChannelName (ChannelType, uint _index) const override
    {
        const int index = static_cast<int>(_index); // FIXME
        if (index < outputNames.size())
            return outputNames[index];
        return water::String("Capture ") + String(index+1);
    }

    void setNames(const bool setInputNames, const water::StringArray& names)
    {
        if (setInputNames)
            inputNames = names;
        else
            outputNames = names;
    }

private:
    water::StringArray inputNames;
    water::StringArray outputNames;
};

PatchbayGraph::PatchbayGraph(CarlaEngine* const engine,
                             const uint32_t audioIns, const uint32_t audioOuts,
                             const uint32_t cvIns, const uint32_t cvOuts,
                             const bool withMidiIn, const bool withMidiOut)
    : CarlaRunner("PatchbayReorderRunner"),
      connections(),
      graph(),
      audioBuffer(),
      cvInBuffer(),
      cvOutBuffer(),
      midiBuffer(),
      numAudioIns(carla_fixedValue(0U, MAX_GRAPH_AUDIO_IO, audioIns)),
      numAudioOuts(carla_fixedValue(0U, MAX_GRAPH_AUDIO_IO, audioOuts)),
      numCVIns(carla_fixedValue(0U, MAX_GRAPH_CV_IO, cvIns)),
      numCVOuts(carla_fixedValue(0U, MAX_GRAPH_CV_IO, cvOuts)),
      retCon(),
      usingExternalHost(false),
      usingExternalOSC(false),
      extGraph(engine),
      kEngine(engine)
{
    const uint32_t bufferSize(engine->getBufferSize());
    const double   sampleRate(engine->getSampleRate());

    graph.setPlayConfigDetails(numAudioIns, numAudioOuts,
                               numCVIns, numCVOuts,
                               1, 1,
                               sampleRate, static_cast<int>(bufferSize));
    graph.prepareToPlay(sampleRate, static_cast<int>(bufferSize));

    audioBuffer.setSize(jmax(numAudioIns, numAudioOuts), bufferSize);
    cvInBuffer.setSize(numCVIns, bufferSize);
    cvOutBuffer.setSize(numCVOuts, bufferSize);

    midiBuffer.ensureSize(kMaxEngineEventInternalCount*2);
    midiBuffer.clear();

    water::StringArray channelNames;

    switch (numAudioIns)
    {
    case 2:
        channelNames.add("Left");
        channelNames.add("Right");
        break;
    case 3:
        channelNames.add("Left");
        channelNames.add("Right");
        channelNames.add("Sidechain");
        break;
    }

    if (numAudioIns != 0)
    {
        NamedAudioGraphIOProcessor* const proc(
            new NamedAudioGraphIOProcessor(NamedAudioGraphIOProcessor::audioInputNode));
        proc->setNames(false, channelNames);

        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.isPlugin = false;
        node->properties.isOutput = false;
        node->properties.isAudio = true;
        node->properties.isCV = false;
        node->properties.isMIDI = false;
        node->properties.isOSC = false;
    }

    if (numAudioOuts != 0)
    {
        NamedAudioGraphIOProcessor* const proc(
            new NamedAudioGraphIOProcessor(NamedAudioGraphIOProcessor::audioOutputNode));
        proc->setNames(true, channelNames);

        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.isPlugin = false;
        node->properties.isOutput = false;
        node->properties.isAudio = true;
        node->properties.isCV = false;
        node->properties.isMIDI = false;
        node->properties.isOSC = false;
    }

    if (numCVIns != 0)
    {
        NamedAudioGraphIOProcessor* const proc(
            new NamedAudioGraphIOProcessor(NamedAudioGraphIOProcessor::cvInputNode));
        // proc->setNames(false, channelNames);

        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.isPlugin = false;
        node->properties.isOutput = false;
        node->properties.isAudio = false;
        node->properties.isCV = true;
        node->properties.isMIDI = false;
        node->properties.isOSC = false;
    }

    if (numCVOuts != 0)
    {
        NamedAudioGraphIOProcessor* const proc(
            new NamedAudioGraphIOProcessor(NamedAudioGraphIOProcessor::cvOutputNode));
        // proc->setNames(true, channelNames);

        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.isPlugin = false;
        node->properties.isOutput = false;
        node->properties.isAudio = false;
        node->properties.isCV = true;
        node->properties.isMIDI = false;
        node->properties.isOSC = false;
    }

    if (withMidiIn)
    {
        NamedAudioGraphIOProcessor* const proc(
            new NamedAudioGraphIOProcessor(NamedAudioGraphIOProcessor::midiInputNode));
        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.isPlugin = false;
        node->properties.isOutput = false;
        node->properties.isAudio = false;
        node->properties.isCV = false;
        node->properties.isMIDI = true;
        node->properties.isOSC = false;
    }

    if (withMidiOut)
    {
        NamedAudioGraphIOProcessor* const proc(
            new NamedAudioGraphIOProcessor(NamedAudioGraphIOProcessor::midiOutputNode));
        AudioProcessorGraph::Node* const node(graph.addNode(proc));
        node->properties.isPlugin = false;
        node->properties.isOutput = true;
        node->properties.isAudio = false;
        node->properties.isCV = false;
        node->properties.isMIDI = true;
        node->properties.isOSC = false;
    }

    startRunner(100);
}

PatchbayGraph::~PatchbayGraph()
{
    stopRunner();

    connections.clear();
    extGraph.clear();

    graph.releaseResources();
    graph.clear();
    audioBuffer.clear();
    cvInBuffer.clear();
    cvOutBuffer.clear();
}

void PatchbayGraph::setBufferSize(const uint32_t bufferSize)
{
    const CarlaRecursiveMutexLocker cml1(graph.getReorderMutex());

    graph.releaseResources();
    graph.prepareToPlay(kEngine->getSampleRate(), static_cast<int>(bufferSize));
    audioBuffer.setSize(audioBuffer.getNumChannels(), bufferSize);
    cvInBuffer.setSize(numCVIns, bufferSize);
    cvOutBuffer.setSize(numCVOuts, bufferSize);
}

void PatchbayGraph::setSampleRate(const double sampleRate)
{
    const CarlaRecursiveMutexLocker cml1(graph.getReorderMutex());

    graph.releaseResources();
    graph.prepareToPlay(sampleRate, static_cast<int>(kEngine->getBufferSize()));
}

void PatchbayGraph::setOffline(const bool offline)
{
    graph.setNonRealtime(offline);
}

void PatchbayGraph::addPlugin(const CarlaPluginPtr plugin)
{
    CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr,);
    carla_debug("PatchbayGraph::addPlugin(%p)", plugin.get());

    CarlaPluginInstance* const instance(new CarlaPluginInstance(kEngine, plugin));
    AudioProcessorGraph::Node* const node(graph.addNode(instance));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    const bool sendHost = !usingExternalHost;
    const bool sendOSC  = !usingExternalOSC;

    plugin->setPatchbayNodeId(node->nodeId);

    node->properties.isPlugin = true;
    node->properties.pluginId = plugin->getId();

    addNodeToPatchbay(sendHost, sendOSC, kEngine, node, static_cast<int>(plugin->getId()), instance);
}

void PatchbayGraph::replacePlugin(const CarlaPluginPtr oldPlugin, const CarlaPluginPtr newPlugin)
{
    CARLA_SAFE_ASSERT_RETURN(oldPlugin.get() != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newPlugin.get() != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(oldPlugin != newPlugin,);
    CARLA_SAFE_ASSERT_RETURN(oldPlugin->getId() == newPlugin->getId(),);

    AudioProcessorGraph::Node* const oldNode(graph.getNodeForId(oldPlugin->getPatchbayNodeId()));
    CARLA_SAFE_ASSERT_RETURN(oldNode != nullptr,);

    const bool sendHost = !usingExternalHost;
    const bool sendOSC  = !usingExternalOSC;

    disconnectInternalGroup(oldNode->nodeId);
    removeNodeFromPatchbay(sendHost, sendOSC, kEngine, oldNode->nodeId, oldNode->getProcessor());

    ((CarlaPluginInstance*)oldNode->getProcessor())->invalidatePlugin();

    graph.removeNode(oldNode->nodeId);

    CarlaPluginInstance* const instance(new CarlaPluginInstance(kEngine, newPlugin));
    AudioProcessorGraph::Node* const node(graph.addNode(instance));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    newPlugin->setPatchbayNodeId(node->nodeId);

    node->properties.isPlugin = true;
    node->properties.pluginId = newPlugin->getId();

    addNodeToPatchbay(sendHost, sendOSC, kEngine, node, static_cast<int>(newPlugin->getId()), instance);
}

void PatchbayGraph::renamePlugin(const CarlaPluginPtr plugin, const char* const newName)
{
    CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr,);
    carla_debug("PatchbayGraph::renamePlugin(%p)", plugin.get(), newName);

    AudioProcessorGraph::Node* const node(graph.getNodeForId(plugin->getPatchbayNodeId()));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    const bool sendHost = !usingExternalHost;
    const bool sendOSC  = !usingExternalOSC;

    kEngine->callback(sendHost, sendOSC,
                      ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED,
                      node->nodeId,
                      0, 0, 0, 0.0f,
                      newName);
}

void PatchbayGraph::switchPlugins(CarlaPluginPtr pluginA, CarlaPluginPtr pluginB)
{
    CARLA_SAFE_ASSERT_RETURN(pluginA.get() != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginB.get() != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginA != pluginB,);
    CARLA_SAFE_ASSERT_RETURN(pluginA->getId() != pluginB->getId(),);

    AudioProcessorGraph::Node* const nodeA(graph.getNodeForId(pluginA->getPatchbayNodeId()));
    CARLA_SAFE_ASSERT_RETURN(nodeA != nullptr,);

    AudioProcessorGraph::Node* const nodeB(graph.getNodeForId(pluginB->getPatchbayNodeId()));
    CARLA_SAFE_ASSERT_RETURN(nodeB != nullptr,);

    nodeA->properties.pluginId = pluginB->getId();
    nodeB->properties.pluginId = pluginA->getId();
}

void PatchbayGraph::reconfigureForCV(const CarlaPluginPtr plugin, const uint portIndex, bool added)
{
    CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr,);
    carla_debug("PatchbayGraph::reconfigureForCV(%p, %u, %s)", plugin.get(), portIndex, bool2str(added));

    AudioProcessorGraph::Node* const node = graph.getNodeForId(plugin->getPatchbayNodeId());
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    CarlaPluginInstance* const proc = dynamic_cast<CarlaPluginInstance*>(node->getProcessor());
    CARLA_SAFE_ASSERT_RETURN(proc != nullptr,);

    const bool sendHost = !usingExternalHost;
    const bool sendOSC  = !usingExternalOSC;

    const uint oldCvIn = proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeCV);
    // const uint oldCvOut = proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV);

    {
        const CarlaRecursiveMutexLocker crml(graph.getCallbackLock());

        proc->reconfigure();

        graph.buildRenderingSequence();
    }

    const uint newCvIn = proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeCV);
    // const uint newCvOut = proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV);

    if (added)
    {
        CARLA_SAFE_ASSERT_UINT2_RETURN(newCvIn > oldCvIn, newCvIn, oldCvIn,);
        // CARLA_SAFE_ASSERT_UINT2_RETURN(newCvOut >= oldCvOut, newCvOut, oldCvOut,);

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_PORT_ADDED,
                          node->nodeId,
                          static_cast<int>(kCVInputPortOffset + plugin->getCVInCount() + portIndex),
                          PATCHBAY_PORT_TYPE_CV|PATCHBAY_PORT_IS_INPUT,
                          0, 0.0f,
                          proc->getInputChannelName(AudioProcessor::ChannelTypeCV, portIndex).toRawUTF8());
    }
    else
    {
        CARLA_SAFE_ASSERT_UINT2_RETURN(newCvIn < oldCvIn, newCvIn, oldCvIn,);
        // CARLA_SAFE_ASSERT_UINT2_RETURN(newCvOut <= oldCvOut, newCvOut, oldCvOut,);

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED,
                          node->nodeId,
                          static_cast<int>(kCVInputPortOffset + plugin->getCVInCount() + portIndex),
                          0, 0, 0.0f, nullptr);
    }
}

void PatchbayGraph::removePlugin(const CarlaPluginPtr plugin)
{
    CARLA_SAFE_ASSERT_RETURN(plugin.get() != nullptr,);
    carla_debug("PatchbayGraph::removePlugin(%p)", plugin.get());

    AudioProcessorGraph::Node* const node(graph.getNodeForId(plugin->getPatchbayNodeId()));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    const bool sendHost = !usingExternalHost;
    const bool sendOSC  = !usingExternalOSC;

    disconnectInternalGroup(node->nodeId);
    removeNodeFromPatchbay(sendHost, sendOSC, kEngine, node->nodeId, node->getProcessor());

    ((CarlaPluginInstance*)node->getProcessor())->invalidatePlugin();

    // Fix plugin Ids properties
    for (uint i=plugin->getId()+1, count=kEngine->getCurrentPluginCount(); i<count; ++i)
    {
        const CarlaPluginPtr plugin2 = kEngine->getPlugin(i);
        CARLA_SAFE_ASSERT_BREAK(plugin2.get() != nullptr);

        if (AudioProcessorGraph::Node* const node2 = graph.getNodeForId(plugin2->getPatchbayNodeId()))
        {
            CARLA_SAFE_ASSERT_CONTINUE(node2->properties.isPlugin);
            node2->properties.pluginId = i - 1;
        }
    }

    CARLA_SAFE_ASSERT_RETURN(graph.removeNode(node->nodeId),);
}

void PatchbayGraph::removeAllPlugins(const bool aboutToClose)
{
    carla_debug("PatchbayGraph::removeAllPlugins()");

    stopRunner();

    const bool sendHost = !usingExternalHost;
    const bool sendOSC  = !usingExternalOSC;

    for (uint i=0, count=kEngine->getCurrentPluginCount(); i<count; ++i)
    {
        const CarlaPluginPtr plugin = kEngine->getPlugin(i);
        CARLA_SAFE_ASSERT_CONTINUE(plugin.get() != nullptr);

        AudioProcessorGraph::Node* const node(graph.getNodeForId(plugin->getPatchbayNodeId()));
        CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

        disconnectInternalGroup(node->nodeId);
        removeNodeFromPatchbay(sendHost, sendOSC, kEngine, node->nodeId, node->getProcessor());

        ((CarlaPluginInstance*)node->getProcessor())->invalidatePlugin();

        graph.removeNode(node->nodeId);
    }

    if (!aboutToClose)
        startRunner(100);
}

bool PatchbayGraph::connect(const bool external,
                            const uint groupA, const uint portA, const uint groupB, const uint portB)
{
    if (external)
        return extGraph.connect(usingExternalHost, usingExternalOSC, groupA, portA, groupB, portB);

    uint adjustedPortA = portA;
    uint adjustedPortB = portB;

    AudioProcessor::ChannelType channelType;
    if (! adjustPatchbayPortIdForWater(channelType, adjustedPortA))
        return false;
    if (! adjustPatchbayPortIdForWater(channelType, adjustedPortB))
        return false;

    if (! graph.addConnection(channelType, groupA, adjustedPortA, groupB, adjustedPortB))
    {
        kEngine->setLastError("Failed from water");
        return false;
    }

    ConnectionToId connectionToId;
    connectionToId.setData(++connections.lastId, groupA, portA, groupB, portB);

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';
    std::snprintf(strBuf, STR_MAX, "%u:%u:%u:%u", groupA, portA, groupB, portB);

    kEngine->callback(!usingExternalHost, !usingExternalOSC,
                      ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                      connectionToId.id,
                      0, 0, 0, 0.0f,
                      strBuf);

    connections.list.append(connectionToId);
    return true;
}

bool PatchbayGraph::disconnect(const bool external, const uint connectionId)
{
    if (external)
        return extGraph.disconnect(usingExternalHost, usingExternalOSC, connectionId);

    for (LinkedList<ConnectionToId>::Itenerator it=connections.list.begin2(); it.valid(); it.next())
    {
        static const ConnectionToId fallback = { 0, 0, 0, 0, 0 };

        const ConnectionToId& connectionToId(it.getValue(fallback));
        CARLA_SAFE_ASSERT_CONTINUE(connectionToId.id > 0);

        if (connectionToId.id != connectionId)
            continue;

        uint adjustedPortA = connectionToId.portA;
        uint adjustedPortB = connectionToId.portB;

        AudioProcessor::ChannelType channelType;
        if (! adjustPatchbayPortIdForWater(channelType, adjustedPortA))
            return false;
        if (! adjustPatchbayPortIdForWater(channelType, adjustedPortB))
            return false;

        if (! graph.removeConnection(channelType,
                                     connectionToId.groupA, adjustedPortA,
                                     connectionToId.groupB, adjustedPortB))
            return false;

        kEngine->callback(!usingExternalHost, !usingExternalOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED,
                          connectionToId.id,
                          0, 0, 0, 0.0f,
                          nullptr);

        connections.list.remove(it);
        return true;
    }

    kEngine->setLastError("Failed to find connection");
    return false;
}

void PatchbayGraph::disconnectInternalGroup(const uint groupId) noexcept
{
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

        AudioProcessor::ChannelType channelType;
        if (! adjustPatchbayPortIdForWater(adjustedPortA))
            return false;
        if (! adjustPatchbayPortIdForWater(adjustedPortB))
            return false;

        graph.removeConnection(connectionToId.groupA, static_cast<int>(adjustedPortA),
                               connectionToId.groupB, static_cast<int>(adjustedPortB));
        */

        kEngine->callback(!usingExternalHost, !usingExternalOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED,
                          connectionToId.id,
                          0, 0, 0, 0.0f,
                          nullptr);

        connections.list.remove(it);
    }
}

void PatchbayGraph::setGroupPos(const bool sendHost, const bool sendOSC, const bool external,
                                uint groupId, int x1, int y1, int x2, int y2)
{
    if (external)
        return extGraph.setGroupPos(sendHost, sendOSC, groupId, x1, y1, x2, y2);

    AudioProcessorGraph::Node* const node(graph.getNodeForId(groupId));
    CARLA_SAFE_ASSERT_RETURN(node != nullptr,);

    node->properties.position.x1 = x1;
    node->properties.position.y1 = y1;
    node->properties.position.x2 = x2;
    node->properties.position.y2 = y2;
    node->properties.position.valid = true;

    kEngine->callback(sendHost, sendOSC,
                      ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED,
                      groupId, x1, y1, x2, static_cast<float>(y2),
                      nullptr);
}

void PatchbayGraph::refresh(const bool sendHost, const bool sendOSC, const bool external, const char* const deviceName)
{
    if (external)
        return extGraph.refresh(sendHost, sendOSC, deviceName);

    CARLA_SAFE_ASSERT_RETURN(deviceName != nullptr,);

    connections.clear();
    graph.removeIllegalConnections();

    for (int i=0, count=graph.getNumNodes(); i<count; ++i)
    {
        AudioProcessorGraph::Node* const node(graph.getNode(i));
        CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

        AudioProcessor* const proc(node->getProcessor());
        CARLA_SAFE_ASSERT_CONTINUE(proc != nullptr);

        int pluginId = -1;

        // plugin node
        if (node->properties.isPlugin)
            pluginId = static_cast<int>(node->properties.pluginId);

        addNodeToPatchbay(sendHost, sendOSC, kEngine, node, pluginId, proc);
    }

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    for (size_t i=0, count=graph.getNumConnections(); i<count; ++i)
    {
        const AudioProcessorGraph::Connection* const conn(graph.getConnection(i));
        CARLA_SAFE_ASSERT_CONTINUE(conn != nullptr);

        const uint groupA = conn->sourceNodeId;
        const uint groupB = conn->destNodeId;

        uint portA = conn->sourceChannelIndex;
        uint portB = conn->destChannelIndex;

        switch (conn->channelType)
        {
        case AudioProcessor::ChannelTypeAudio:
            portA += kAudioOutputPortOffset;
            portB += kAudioInputPortOffset;
            break;
        case AudioProcessor::ChannelTypeCV:
            portA += kCVOutputPortOffset;
            portB += kCVInputPortOffset;
            break;
        case AudioProcessor::ChannelTypeMIDI:
            portA += kMidiOutputPortOffset;
            portB += kMidiInputPortOffset;
            break;
        }

        ConnectionToId connectionToId;
        connectionToId.setData(++connections.lastId, groupA, portA, groupB, portB);

        std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", groupA, portA, groupB, portB);

        kEngine->callback(sendHost, sendOSC,
                          ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                          connectionToId.id,
                          0, 0, 0, 0.0f,
                          strBuf);

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

        water::String fullPortNameA(getProcessorFullPortName(procA, connectionToId.portA));
        CARLA_SAFE_ASSERT_CONTINUE(fullPortNameA.isNotEmpty());

        water::String fullPortNameB(getProcessorFullPortName(procB, connectionToId.portB));
        CARLA_SAFE_ASSERT_CONTINUE(fullPortNameB.isNotEmpty());

        connList.append(fullPortNameA.toRawUTF8());
        connList.append(fullPortNameB.toRawUTF8());
    }

    if (connList.count() == 0)
        return nullptr;

    retCon = connList.toCharStringListPtr();

    return retCon;
}

const CarlaEngine::PatchbayPosition* PatchbayGraph::getPositions(bool external, uint& count) const
{
    CarlaEngine::PatchbayPosition* ret;

    if (external)
    {
        try {
            ret = new CarlaEngine::PatchbayPosition[kExternalGraphGroupMax];
        } CARLA_SAFE_EXCEPTION_RETURN("new CarlaEngine::PatchbayPosition", nullptr);

        count = 0;

        for (uint i=kExternalGraphGroupCarla; i<kExternalGraphGroupMax; ++i)
        {
            const PatchbayPosition& eppos(extGraph.positions[i]);

            if (! eppos.active)
                continue;

            CarlaEngine::PatchbayPosition& ppos(ret[count++]);

            switch (i)
            {
            case kExternalGraphGroupCarla:
                ppos.name = "Carla";
                break;
            case kExternalGraphGroupAudioIn:
                ppos.name = "AudioIn";
                break;
            case kExternalGraphGroupAudioOut:
                ppos.name = "AudioOut";
                break;
            case kExternalGraphGroupMidiIn:
                ppos.name = "MidiIn";
                break;
            case kExternalGraphGroupMidiOut:
                ppos.name = "MidiOut";
                break;
            }
            ppos.dealloc = false;
            ppos.pluginId = -1;

            ppos.x1 = eppos.x1;
            ppos.y1 = eppos.y1;
            ppos.x2 = eppos.x2;
            ppos.y2 = eppos.y2;
        }

        return ret;
    }
    else
    {
        const int numNodes = graph.getNumNodes();
        CARLA_SAFE_ASSERT_RETURN(numNodes > 0, nullptr);

        try {
            ret = new CarlaEngine::PatchbayPosition[numNodes];
        } CARLA_SAFE_EXCEPTION_RETURN("new CarlaEngine::PatchbayPosition", nullptr);

        count = 0;

        for (int i=numNodes; --i >= 0;)
        {
            AudioProcessorGraph::Node* const node(graph.getNode(i));
            CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

            if (! node->properties.position.valid)
                continue;

            AudioProcessor* const proc(node->getProcessor());
            CARLA_SAFE_ASSERT_CONTINUE(proc != nullptr);

            CarlaEngine::PatchbayPosition& ppos(ret[count++]);

            ppos.name = carla_strdup(proc->getName().toRawUTF8());
            ppos.dealloc = true;
            ppos.pluginId = node->properties.isPlugin ? static_cast<int>(node->properties.pluginId) : -1;

            ppos.x1 = node->properties.position.x1;
            ppos.y1 = node->properties.position.y1;
            ppos.x2 = node->properties.position.x2;
            ppos.y2 = node->properties.position.y2;
        }

        return ret;
    }
}

bool PatchbayGraph::getGroupFromName(bool external, const char* groupName, uint& groupId) const
{
    if (external)
        return extGraph.getGroupFromName(groupName, groupId);

    for (int i=0, count=graph.getNumNodes(); i<count; ++i)
    {
        AudioProcessorGraph::Node* const node(graph.getNode(i));
        CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

        AudioProcessor* const proc(node->getProcessor());
        CARLA_SAFE_ASSERT_CONTINUE(proc != nullptr);

        if (proc->getName() != groupName)
            continue;

        groupId = node->nodeId;
        return true;
    }

    return false;
}

bool PatchbayGraph::getGroupAndPortIdFromFullName(const bool external, const char* const fullPortName, uint& groupId, uint& portId) const
{
    if (external)
        return extGraph.getGroupAndPortIdFromFullName(fullPortName, groupId, portId);

    water::String groupName(water::String(fullPortName).upToFirstOccurrenceOf(":", false, false));
    water::String portName(water::String(fullPortName).fromFirstOccurrenceOf(":", false, false));

    for (int i=0, count=graph.getNumNodes(); i<count; ++i)
    {
        AudioProcessorGraph::Node* const node(graph.getNode(i));
        CARLA_SAFE_ASSERT_CONTINUE(node != nullptr);

        AudioProcessor* const proc(node->getProcessor());
        CARLA_SAFE_ASSERT_CONTINUE(proc != nullptr);

        if (proc->getName() != groupName)
            continue;

        groupId = node->nodeId;

        for (uint j=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeAudio); j<numInputs; ++j)
        {
            if (proc->getInputChannelName(AudioProcessor::ChannelTypeAudio, j) != portName)
                continue;

            portId = kAudioInputPortOffset+j;
            return true;
        }

        for (uint j=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeAudio); j<numOutputs; ++j)
        {
            if (proc->getOutputChannelName(AudioProcessor::ChannelTypeAudio, j) != portName)
                continue;

            portId = kAudioOutputPortOffset+j;
            return true;
        }

        for (uint j=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeCV); j<numInputs; ++j)
        {
            if (proc->getInputChannelName(AudioProcessor::ChannelTypeCV, j) != portName)
                continue;

            portId = kCVInputPortOffset+j;
            return true;
        }

        for (uint j=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV); j<numOutputs; ++j)
        {
            if (proc->getOutputChannelName(AudioProcessor::ChannelTypeCV, j) != portName)
                continue;

            portId = kCVOutputPortOffset+j;
            return true;
        }

        for (uint j=0, numInputs=proc->getTotalNumInputChannels(AudioProcessor::ChannelTypeMIDI); j<numInputs; ++j)
        {
            if (proc->getInputChannelName(AudioProcessor::ChannelTypeMIDI, j) != portName)
                continue;

            portId = kMidiInputPortOffset+j;
            return true;
        }

        for (uint j=0, numOutputs=proc->getTotalNumOutputChannels(AudioProcessor::ChannelTypeMIDI); j<numOutputs; ++j)
        {
            if (proc->getOutputChannelName(AudioProcessor::ChannelTypeMIDI, j) != portName)
                continue;

            portId = kMidiOutputPortOffset+j;
            return true;
        }
    }

    return false;
}

void PatchbayGraph::process(CarlaEngine::ProtectedData* const data,
                            const float* const* const inBuf,
                            float* const* const outBuf,
                            const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(data->events.out != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(frames > 0,);

    // put events in water buffer
    {
        midiBuffer.clear();
        fillWaterMidiBufferFromEngineEvents(midiBuffer, data->events.in);
    }

    // set audio and cv buffer size, needed for water internals
    if (! audioBuffer.setSizeRT(frames))
        return;
    if (! cvInBuffer.setSizeRT(frames))
        return;
    if (! cvOutBuffer.setSizeRT(frames))
        return;

    // put carla audio and cv in water buffer
    {
        uint32_t i=0;

        for (; i < numAudioIns; ++i) {
            CARLA_SAFE_ASSERT_BREAK(inBuf[i]);
            audioBuffer.copyFrom(i, 0, inBuf[i], frames);
        }

        for (uint32_t j=0; j < numCVIns; ++j, ++i) {
            CARLA_SAFE_ASSERT_BREAK(inBuf[i]);
            cvInBuffer.copyFrom(j, 0, inBuf[i], frames);
        }

        // clear remaining channels
        for (uint32_t j=numAudioIns, count=audioBuffer.getNumChannels(); j < count; ++j)
            audioBuffer.clear(j, 0, frames);

        for (uint32_t j=0; j < numCVOuts; ++j)
            cvOutBuffer.clear(j, 0, frames);
    }

    // ready to go!
    graph.processBlockWithCV(audioBuffer, cvInBuffer, cvOutBuffer, midiBuffer);

    // put water audio and cv in carla buffer
    {
        uint32_t i=0;

        for (; i < numAudioOuts; ++i)
            carla_copyFloats(outBuf[i], audioBuffer.getReadPointer(i), frames);

        for (uint32_t j=0; j < numCVOuts; ++j, ++i)
            carla_copyFloats(outBuf[i], cvOutBuffer.getReadPointer(j), frames);
    }

    // put water events in carla buffer
    {
        carla_zeroStructs(data->events.out, kMaxEngineEventInternalCount);
        fillEngineEventsFromWaterMidiBuffer(data->events.out, midiBuffer);
        midiBuffer.clear();
    }
}

bool PatchbayGraph::run()
{
    graph.reorderNowIfNeeded();
    return true;
}

// -----------------------------------------------------------------------
// InternalGraph

EngineInternalGraph::EngineInternalGraph(CarlaEngine* const engine) noexcept
    : fIsRack(false),
      fNumAudioOuts(0),
      fIsReady(false),
      fRack(nullptr),
      kEngine(engine) {}

EngineInternalGraph::~EngineInternalGraph() noexcept
{
    CARLA_SAFE_ASSERT(! fIsReady);
    CARLA_SAFE_ASSERT(fRack == nullptr);
}

void EngineInternalGraph::create(const uint32_t audioIns, const uint32_t audioOuts,
                                 const uint32_t cvIns, const uint32_t cvOuts,
                                 const bool withMidiIn, const bool withMidiOut)
{
    fIsRack = (kEngine->getOptions().processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK);

    if (fIsRack)
    {
        CARLA_SAFE_ASSERT_RETURN(fRack == nullptr,);
        fRack = new RackGraph(kEngine, audioIns, audioOuts);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(fPatchbay == nullptr,);
        fPatchbay = new PatchbayGraph(kEngine, audioIns, audioOuts, cvIns, cvOuts, withMidiIn, withMidiOut);
    }

    fNumAudioOuts = audioOuts;
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
    fNumAudioOuts = 0;
}

void EngineInternalGraph::setBufferSize(const uint32_t bufferSize)
{
    CarlaScopedValueSetter<volatile bool> svs(fIsReady, false, true);

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
    CarlaScopedValueSetter<volatile bool> svs(fIsReady, false, true);

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
    CarlaScopedValueSetter<volatile bool> svs(fIsReady, false, true);

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

PatchbayGraph* EngineInternalGraph::getPatchbayGraphOrNull() const noexcept
{
    return fIsRack ? nullptr : fPatchbay;
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
        fPatchbay->process(data, inBuf, outBuf, frames);
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

void EngineInternalGraph::addPlugin(const CarlaPluginPtr plugin)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->addPlugin(plugin);
}

void EngineInternalGraph::replacePlugin(const CarlaPluginPtr oldPlugin, const CarlaPluginPtr newPlugin)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->replacePlugin(oldPlugin, newPlugin);
}

void EngineInternalGraph::renamePlugin(const CarlaPluginPtr plugin, const char* const newName)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->renamePlugin(plugin, newName);
}

void EngineInternalGraph::switchPlugins(CarlaPluginPtr pluginA, CarlaPluginPtr pluginB)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->switchPlugins(pluginA, pluginB);
}

void EngineInternalGraph::removePlugin(const CarlaPluginPtr plugin)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->removePlugin(plugin);
}

void EngineInternalGraph::removeAllPlugins(const bool aboutToClose)
{
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->removeAllPlugins(aboutToClose);
}

bool EngineInternalGraph::isUsingExternalHost() const noexcept
{
    if (fIsRack)
        return true;

    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr, false);
    return fPatchbay->usingExternalHost;
}

bool EngineInternalGraph::isUsingExternalOSC() const noexcept
{
    if (fIsRack)
        return true;

    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr, false);
    return fPatchbay->usingExternalOSC;
}

void EngineInternalGraph::setUsingExternalHost(const bool usingExternal) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! fIsRack,);
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->usingExternalHost = usingExternal;
}

void EngineInternalGraph::setUsingExternalOSC(const bool usingExternal) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! fIsRack,);
    CARLA_SAFE_ASSERT_RETURN(fPatchbay != nullptr,);
    fPatchbay->usingExternalOSC = usingExternal;
}

// -----------------------------------------------------------------------
// CarlaEngine Patchbay stuff

bool CarlaEngine::patchbayConnect(const bool external,
                                  const uint groupA, const uint portA,
                                  const uint groupB, const uint portB)
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

        return graph->connect(external, groupA, portA, groupB, portB);
    }
}

bool CarlaEngine::patchbayDisconnect(const bool external, const uint connectionId)
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

        return graph->disconnect(external, connectionId);
    }
}

bool CarlaEngine::patchbaySetGroupPos(const bool sendHost, const bool sendOSC, const bool external,
                                      const uint groupId, const int x1, const int y1, const int x2, const int y2)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
                             pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);
    carla_debug("CarlaEngine::patchbaySetGroupPos(%u, %i, %i, %i, %i)", groupId, x1, y1, x2, y2);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        // we don't bother to save position in this case, there is only midi in/out
        return true;
    }
    else
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        graph->setGroupPos(sendHost, sendOSC, external, groupId, x1, y1, x2, y2);
        return true;
    }
}

bool CarlaEngine::patchbayRefresh(const bool sendHost, const bool sendOSC, const bool external)
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

        graph->refresh(sendHost, sendOSC, external, "");
        return true;
    }

    setLastError("Unsupported operation");
    return false;
}

// -----------------------------------------------------------------------
// Patchbay stuff

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

const CarlaEngine::PatchbayPosition* CarlaEngine::getPatchbayPositions(bool external, uint& count) const
{
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), nullptr);
    carla_debug("CarlaEngine::getPatchbayPositions(%s)", bool2str(external));

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, nullptr);

        return graph->getPositions(external, count);
    }

    return nullptr;
}

void CarlaEngine::restorePatchbayConnection(const bool external,
                                            const char* const sourcePort,
                                            const char* const targetPort)
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

        graph->connect(external, groupA, portA, groupB, portB);
    }
}

bool CarlaEngine::restorePatchbayGroupPosition(const bool external, PatchbayPosition& ppos)
{
    CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);
    CARLA_SAFE_ASSERT_RETURN(ppos.name != nullptr && ppos.name[0] != '\0', false);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        PatchbayGraph* const graph = pData->graph.getPatchbayGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        const char* const orig_name = ppos.name;

        // strip client name prefix if present
        if (ppos.pluginId >= 0)
        {
            if (const char* const rname2 = std::strstr(ppos.name, "."))
                if (const char* const rname3 = std::strstr(rname2 + 1, "/"))
                    ppos.name = rname3 + 1;
        }

        uint groupId;
        CARLA_SAFE_ASSERT_INT_RETURN(graph->getGroupFromName(external, ppos.name, groupId), external, false);

        graph->setGroupPos(true, true, external, groupId, ppos.x1, ppos.y1, ppos.x2, ppos.y2);

        return ppos.name != orig_name;
    }

    return false;
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
