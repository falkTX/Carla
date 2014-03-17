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
// EngineControlEvent

void EngineControlEvent::dumpToMidiData(const uint8_t channel, uint8_t& size, uint8_t data[3]) const noexcept
{
    size = 0;

    switch (type)
    {
    case kEngineControlEventTypeNull:
        break;

    case kEngineControlEventTypeParameter:
        if (param >= MAX_MIDI_VALUE)
        {
            // out of bounds. do nothing
        }
        else if (MIDI_IS_CONTROL_BANK_SELECT(param))
        {
            size    = 3;
            data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
            data[1] = MIDI_CONTROL_BANK_SELECT;
            data[2] = uint8_t(carla_fixValue<float>(0.0f, float(MAX_MIDI_VALUE-1), value));
        }
        else
        {
            size    = 3;
            data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
            data[1] = static_cast<uint8_t>(param);
            data[2] = uint8_t(carla_fixValue<float>(0.0f, 1.0f, value) * float(MAX_MIDI_VALUE-1));
        }
        break;

    case kEngineControlEventTypeMidiBank:
        size    = 3;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
        data[1] = MIDI_CONTROL_BANK_SELECT;
        data[2] = uint8_t(carla_fixValue<uint16_t>(0, MAX_MIDI_VALUE-1, param));
        break;

    case kEngineControlEventTypeMidiProgram:
        size    = 2;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_PROGRAM_CHANGE | (channel & MIDI_CHANNEL_BIT));
        data[1] = uint8_t(carla_fixValue<uint16_t>(0, MAX_MIDI_VALUE-1, param));
        break;

    case kEngineControlEventTypeAllSoundOff:
        size    = 2;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
        break;

    case kEngineControlEventTypeAllNotesOff:
        size    = 2;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
        data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
        break;
    }
}

// -----------------------------------------------------------------------
// EngineEvent

void EngineEvent::fillFromMidiData(const uint8_t size, const uint8_t* const data) noexcept
{
    if (size == 0 || data == nullptr || data[0] < MIDI_STATUS_NOTE_OFF)
    {
        type    = kEngineEventTypeNull;
        channel = 0;
        return;
    }

    // get channel
    channel = uint8_t(MIDI_GET_CHANNEL_FROM_DATA(data));

    // get status
    const uint8_t midiStatus(uint8_t(MIDI_GET_STATUS_FROM_DATA(data)));

    if (midiStatus == MIDI_STATUS_CONTROL_CHANGE)
    {
        type = kEngineEventTypeControl;

        const uint8_t midiControl(data[1]);

        if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
        {
            CARLA_SAFE_ASSERT_INT(size == 3, size);

            const uint8_t midiBank(data[2]);

            ctrl.type  = kEngineControlEventTypeMidiBank;
            ctrl.param = midiBank;
            ctrl.value = 0.0f;
        }
        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
        {
            CARLA_SAFE_ASSERT_INT(size == 2, size);

            ctrl.type  = kEngineControlEventTypeAllSoundOff;
            ctrl.param = 0;
            ctrl.value = 0.0f;
        }
        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
        {
            CARLA_SAFE_ASSERT_INT(size == 2, size);

            ctrl.type  = kEngineControlEventTypeAllNotesOff;
            ctrl.param = 0;
            ctrl.value = 0.0f;
        }
        else
        {
            CARLA_SAFE_ASSERT_INT2(size == 3, size, midiControl);

            const uint8_t midiValue(carla_fixValue<uint8_t>(0, 127, data[2])); // ensures 0.0<->1.0 value range

            ctrl.type  = kEngineControlEventTypeParameter;
            ctrl.param = midiControl;
            ctrl.value = float(midiValue)/127.0f;
        }
    }
    else if (midiStatus == MIDI_STATUS_PROGRAM_CHANGE)
    {
        CARLA_SAFE_ASSERT_INT2(size == 2, size, data[1]);

        type = kEngineEventTypeControl;

        const uint8_t midiProgram(data[1]);

        ctrl.type  = kEngineControlEventTypeMidiProgram;
        ctrl.param = midiProgram;
        ctrl.value = 0.0f;
    }
    else
    {
        type = kEngineEventTypeMidi;

        midi.port = 0;
        midi.size = size;

        if (size > EngineMidiEvent::kDataSize)
        {
            midi.dataExt = data;
            std::memset(midi.data, 0, sizeof(uint8_t)*EngineMidiEvent::kDataSize);
        }
        else
        {
            midi.data[0] = midiStatus;

            uint8_t i=1;
            for (; i < midi.size; ++i)
                midi.data[i] = data[i];
            for (; i < EngineMidiEvent::kDataSize; ++i)
                midi.data[i] = 0;

            midi.dataExt = nullptr;
        }
    }
}

// -----------------------------------------------------------------------
// EngineOptions

EngineOptions::EngineOptions() noexcept
#ifdef CARLA_OS_LINUX
    : processMode(ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS),
      transportMode(ENGINE_TRANSPORT_MODE_JACK),
#else
    : processMode(ENGINE_PROCESS_MODE_CONTINUOUS_RACK),
      transportMode(ENGINE_TRANSPORT_MODE_INTERNAL),
#endif
      forceStereo(false),
      preferPluginBridges(false),
      preferUiBridges(true),
      uisAlwaysOnTop(true),
      maxParameters(MAX_DEFAULT_PARAMETERS),
      uiBridgesTimeout(4000),
      audioNumPeriods(2),
      audioBufferSize(512),
      audioSampleRate(44100),
      audioDevice(nullptr),
      binaryDir(nullptr),
      resourceDir(nullptr),
      frontendWinId(0) {}

EngineOptions::~EngineOptions() noexcept
{
    if (audioDevice != nullptr)
    {
        delete[] audioDevice;
        audioDevice = nullptr;
    }

    if (binaryDir != nullptr)
    {
        delete[] binaryDir;
        binaryDir = nullptr;
    }

    if (resourceDir != nullptr)
    {
        delete[] resourceDir;
        resourceDir = nullptr;
    }
}

// -----------------------------------------------------------------------
// EngineTimeInfoBBT

EngineTimeInfoBBT::EngineTimeInfoBBT() noexcept
    : bar(0),
      beat(0),
      tick(0),
      barStartTick(0.0),
      beatsPerBar(0.0f),
      beatType(0.0f),
      ticksPerBeat(0.0),
      beatsPerMinute(0.0) {}

// -----------------------------------------------------------------------
// EngineTimeInfo

EngineTimeInfo::EngineTimeInfo() noexcept
    : playing(false),
      frame(0),
      usecs(0),
      valid(0x0) {}

void EngineTimeInfo::clear() noexcept
{
    playing = false;
    frame   = 0;
    usecs   = 0;
    valid   = 0x0;
}

bool EngineTimeInfo::operator==(const EngineTimeInfo& timeInfo) const noexcept
{
    if (timeInfo.playing != playing || timeInfo.frame != frame || timeInfo.valid != valid)
        return false;
    if ((valid & kValidBBT) == 0)
        return true;
    if (timeInfo.bbt.beatsPerMinute != bbt.beatsPerMinute)
        return false;
    return true;
}

bool EngineTimeInfo::operator!=(const EngineTimeInfo& timeInfo) const noexcept
{
    return !operator==(timeInfo);
}

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
// InternalAudio

EngineInternalAudio::EngineInternalAudio() noexcept
    : isReady(false),
      usePatchbay(false),
      inCount(0),
      outCount(0)
{
    rack = nullptr;
}

EngineInternalAudio::~EngineInternalAudio() noexcept
{
    CARLA_SAFE_ASSERT(! isReady);
    CARLA_SAFE_ASSERT(rack == nullptr);
}

void EngineInternalAudio::initPatchbay() noexcept
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

void EngineInternalAudio::clear() noexcept
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

void EngineInternalAudio::create(const uint32_t bufferSize)
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

void EngineInternalAudio::resize(const uint32_t bufferSize)
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

bool EngineInternalAudio::connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int portB) noexcept
{
    if (usePatchbay)
    {
        CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr, nullptr);
        return patchbay->connect(engine, groupA, portA, groupB, portB);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(rack != nullptr, nullptr);
        return rack->connect(engine, groupA, portA, groupB, portB);
    }
}

const char* const* EngineInternalAudio::getConnections() const
{
    if (usePatchbay)
    {
        CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr, nullptr);
        return patchbay->getConnections();
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(rack != nullptr, nullptr);
        return rack->getConnections();
    }
}

// -----------------------------------------------------------------------
// InternalEvents

EngineInternalEvents::EngineInternalEvents() noexcept
    : in(nullptr),
      out(nullptr) {}

EngineInternalEvents::~EngineInternalEvents() noexcept
{
    CARLA_SAFE_ASSERT(in == nullptr);
    CARLA_SAFE_ASSERT(out == nullptr);
}

// -----------------------------------------------------------------------
// InternalTime

EngineInternalTime::EngineInternalTime() noexcept
    : playing(false),
      frame(0) {}

// -----------------------------------------------------------------------
// NextAction

EngineNextAction::EngineNextAction() noexcept
    : opcode(kEnginePostActionNull),
      pluginId(0),
      value(0) {}

EngineNextAction::~EngineNextAction() noexcept
{
    CARLA_SAFE_ASSERT(opcode == kEnginePostActionNull);
}

void EngineNextAction::ready() noexcept
{
    mutex.lock();
    mutex.unlock();
}

// -----------------------------------------------------------------------
// EnginePluginData

void EnginePluginData::clear() noexcept
{
    plugin = nullptr;
    insPeak[0] = insPeak[1] = 0.0f;
    outsPeak[0] = outsPeak[1] = 0.0f;
}

// -----------------------------------------------------------------------
// CarlaEngineProtectedData

CarlaEngineProtectedData::CarlaEngineProtectedData(CarlaEngine* const engine)
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
    CARLA_SAFE_ASSERT_RETURN(bufEvents.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(bufEvents.out != nullptr,);

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
    carla_zeroStruct<EngineEvent>(bufEvents.out, kMaxEngineEventInternalCount);

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
            if (oldMidiOutCount == 0 && bufEvents.in[0].type != kEngineEventTypeNull)
            {
                if (bufEvents.out[0].type != kEngineEventTypeNull)
                {
                    // TODO: carefully add to input, sorted events
                }
                // else nothing needed
            }
            else
            {
                // initialize event inputs from previous outputs
                carla_copyStruct<EngineEvent>(bufEvents.in, bufEvents.out, kMaxEngineEventInternalCount);

                // initialize event outputs (zero)
                carla_zeroStruct<EngineEvent>(bufEvents.out, kMaxEngineEventInternalCount);
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
    EngineRackBuffers* const rack(bufAudio.rack);

    const CarlaCriticalSectionScope _cs2(rack->connectLock);

    // connect input buffers
    if (rack->connectedIn1.count() == 0)
    {
        FLOAT_CLEAR(rack->in[0], nframes);
    }
    else
    {
        bool first = true;

        for (LinkedList<int>::Itenerator it = rack->connectedIn1.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(inCount));

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

    if (rack->connectedIn2.count() == 0)
    {
        FLOAT_CLEAR(rack->in[1], nframes);
    }
    else
    {
        bool first = true;

        for (LinkedList<int>::Itenerator it = rack->connectedIn2.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(inCount));

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
    if (rack->connectedOut1.count() != 0)
    {
        for (LinkedList<int>::Itenerator it = rack->connectedOut1.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(outCount));

            FLOAT_ADD(outBuf[port], rack->out[0], nframes);
        }
    }

    if (rack->connectedOut2.count() != 0)
    {
        for (LinkedList<int>::Itenerator it = rack->connectedOut2.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(outCount));

            FLOAT_ADD(outBuf[port], rack->out[1], nframes);
        }
    }
}
#endif

// -----------------------------------------------------------------------
// ScopedActionLock

CarlaEngineProtectedData::ScopedActionLock::ScopedActionLock(CarlaEngineProtectedData* const data, const EnginePostAction action, const unsigned int pluginId, const unsigned int value, const bool lockWait) noexcept
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

CarlaEngineProtectedData::ScopedActionLock::~ScopedActionLock() noexcept
{
    fData->nextAction.mutex.unlock();
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
