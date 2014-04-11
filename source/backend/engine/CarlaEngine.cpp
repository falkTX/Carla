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

/* TODO:
 * - complete processRack(): carefully add to input, sorted events
 * - implement processPatchbay()
 * - implement oscSend_control_switch_plugins()
 * - proper find&load plugins
 * - something about the peaks?
 * - patchbayDisconnect should return false sometimes
 */

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"
#include "CarlaStateUtils.hpp"

#include "jackbridge/JackBridge.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtXml/QDomNode>

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackEngineEvent = { kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, 0.0f }} };

// -----------------------------------------------------------------------
// EngineControlEvent

void EngineControlEvent::convertToMidiData(const uint8_t channel, uint8_t& size, uint8_t data[3]) const noexcept
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
// Carla Engine port (Abstract)

CarlaEnginePort::CarlaEnginePort(const CarlaEngineClient& client, const bool isInputPort) noexcept
    : fClient(client),
      fIsInput(isInputPort)
{
    carla_debug("CarlaEnginePort::CarlaEnginePort(%s)", bool2str(isInputPort));
}

CarlaEnginePort::~CarlaEnginePort() noexcept
{
    carla_debug("CarlaEnginePort::~CarlaEnginePort()");
}

// -----------------------------------------------------------------------
// Carla Engine Audio port

CarlaEngineAudioPort::CarlaEngineAudioPort(const CarlaEngineClient& client, const bool isInputPort) noexcept
    : CarlaEnginePort(client, isInputPort),
      fBuffer(nullptr)
{
    carla_debug("CarlaEngineAudioPort::CarlaEngineAudioPort(%s)", bool2str(isInputPort));
}

CarlaEngineAudioPort::~CarlaEngineAudioPort() noexcept
{
    carla_debug("CarlaEngineAudioPort::~CarlaEngineAudioPort()");
}

void CarlaEngineAudioPort::initBuffer() noexcept
{
}

// -----------------------------------------------------------------------
// Carla Engine CV port

CarlaEngineCVPort::CarlaEngineCVPort(const CarlaEngineClient& client, const bool isInputPort) noexcept
    : CarlaEnginePort(client, isInputPort),
      fBuffer(nullptr)
{
    carla_debug("CarlaEngineCVPort::CarlaEngineCVPort(%s)", bool2str(isInputPort));
}

CarlaEngineCVPort::~CarlaEngineCVPort() noexcept
{
    carla_debug("CarlaEngineCVPort::~CarlaEngineCVPort()");
}

void CarlaEngineCVPort::initBuffer() noexcept
{
}

// -----------------------------------------------------------------------
// Carla Engine Event port

CarlaEngineEventPort::CarlaEngineEventPort(const CarlaEngineClient& client, const bool isInputPort) noexcept
    : CarlaEnginePort(client, isInputPort),
      fBuffer(nullptr),
      fProcessMode(client.getEngine().getProccessMode())
{
    carla_debug("CarlaEngineEventPort::CarlaEngineEventPort(%s)", bool2str(isInputPort));

    if (fProcessMode == ENGINE_PROCESS_MODE_PATCHBAY)
        fBuffer = new EngineEvent[kMaxEngineEventInternalCount];
}

CarlaEngineEventPort::~CarlaEngineEventPort() noexcept
{
    carla_debug("CarlaEngineEventPort::~CarlaEngineEventPort()");

    if (fProcessMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        delete[] fBuffer;
        fBuffer = nullptr;
    }
}

void CarlaEngineEventPort::initBuffer() noexcept
{
    if (fProcessMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || fProcessMode == ENGINE_PROCESS_MODE_BRIDGE)
        fBuffer = fClient.getEngine().getInternalEventBuffer(fIsInput);
    else if (fProcessMode == ENGINE_PROCESS_MODE_PATCHBAY && ! fIsInput)
        carla_zeroStruct<EngineEvent>(fBuffer, kMaxEngineEventInternalCount);
}

uint32_t CarlaEngineEventPort::getEventCount() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fIsInput, 0);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, 0);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, 0);

    uint32_t i=0;

    for (; i < kMaxEngineEventInternalCount; ++i)
    {
        if (fBuffer[i].type == kEngineEventTypeNull)
            break;
    }

    return i;
}

const EngineEvent& CarlaEngineEventPort::getEvent(const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fIsInput, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(index < kMaxEngineEventInternalCount, kFallbackEngineEvent);

    return fBuffer[index];
}

const EngineEvent& CarlaEngineEventPort::getEventUnchecked(const uint32_t index) const noexcept
{
    return fBuffer[index];
}

bool CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, false);
    CARLA_SAFE_ASSERT_RETURN(type != kEngineControlEventTypeNull, false);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

    if (type == kEngineControlEventTypeParameter) {
        CARLA_SAFE_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
    }

    for (uint32_t i=0; i < kMaxEngineEventInternalCount; ++i)
    {
        EngineEvent& event(fBuffer[i]);

        if (event.type != kEngineEventTypeNull)
            continue;

        event.type    = kEngineEventTypeControl;
        event.time    = time;
        event.channel = channel;

        event.ctrl.type  = type;
        event.ctrl.param = param;
        event.ctrl.value = carla_fixValue<float>(0.0f, 1.0f, value);

        return true;
    }

    carla_stderr2("CarlaEngineEventPort::writeControlEvent() - buffer full");
    return false;
}

bool CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEvent& ctrl) noexcept
{
    return writeControlEvent(time, channel, ctrl.type, ctrl.param, ctrl.value);
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t size, const uint8_t* const data) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, false);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
    CARLA_SAFE_ASSERT_RETURN(size > 0 && size <= EngineMidiEvent::kDataSize, false);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

    for (uint32_t i=0; i < kMaxEngineEventInternalCount; ++i)
    {
        EngineEvent& event(fBuffer[i]);

        if (event.type != kEngineEventTypeNull)
            continue;

        event.type    = kEngineEventTypeMidi;
        event.time    = time;
        event.channel = channel;

        event.midi.port = port;
        event.midi.size = size;

        event.midi.data[0] = uint8_t(MIDI_GET_STATUS_FROM_DATA(data));

        uint8_t j=1;
        for (; j < size; ++j)
            event.midi.data[j] = data[j];
        for (; j < EngineMidiEvent::kDataSize; ++j)
            event.midi.data[j] = 0;

        return true;
    }

    carla_stderr2("CarlaEngineEventPort::writeMidiEvent() - buffer full");
    return false;
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t size, const uint8_t* const data) noexcept
{
    return writeMidiEvent(time, uint8_t(MIDI_GET_CHANNEL_FROM_DATA(data)), 0, size, data);
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const EngineMidiEvent& midi) noexcept
{
    return writeMidiEvent(time, channel, midi.port, midi.size, midi.data);
}

// -----------------------------------------------------------------------
// Carla Engine client (Abstract)

CarlaEngineClient::CarlaEngineClient(const CarlaEngine& engine)
    : fEngine(engine),
      fActive(false),
      fLatency(0)
{
    carla_debug("CarlaEngineClient::CarlaEngineClient()");
}

CarlaEngineClient::~CarlaEngineClient()
{
    CARLA_SAFE_ASSERT(! fActive);
    carla_debug("CarlaEngineClient::~CarlaEngineClient()");
}

void CarlaEngineClient::activate() noexcept
{
    CARLA_SAFE_ASSERT(! fActive);
    carla_debug("CarlaEngineClient::activate()");

    fActive = true;
}

void CarlaEngineClient::deactivate() noexcept
{
    CARLA_SAFE_ASSERT(fActive);
    carla_debug("CarlaEngineClient::deactivate()");

    fActive = false;
}

bool CarlaEngineClient::isActive() const noexcept
{
    return fActive;
}

bool CarlaEngineClient::isOk() const noexcept
{
    return true;
}

uint32_t CarlaEngineClient::getLatency() const noexcept
{
    return fLatency;
}

void CarlaEngineClient::setLatency(const uint32_t samples) noexcept
{
    fLatency = samples;
}

CarlaEnginePort* CarlaEngineClient::addPort(const EnginePortType portType, const char* const name, const bool isInput)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngineClient::addPort(%i:%s, \"%s\", %s)", portType, EnginePortType2Str(portType), name, bool2str(isInput));

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio:
        return new CarlaEngineAudioPort(*this, isInput);
    case kEnginePortTypeCV:
        return new CarlaEngineCVPort(*this, isInput);
    case kEnginePortTypeEvent:
        return new CarlaEngineEventPort(*this, isInput);
    }

    carla_stderr("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
    return nullptr;
}

// -----------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine()
    : pData(new CarlaEngineProtectedData(this))
{
    carla_debug("CarlaEngine::CarlaEngine()");
}

CarlaEngine::~CarlaEngine()
{
    carla_debug("CarlaEngine::~CarlaEngine()");

    delete pData;
}

// -----------------------------------------------------------------------
// Static calls

uint CarlaEngine::getDriverCount()
{
    carla_debug("CarlaEngine::getDriverCount()");

    uint count = 0;

    if (jackbridge_is_ok())
        count += 1;

#ifndef BUILD_BRIDGE
    count += getRtAudioApiCount();
# ifdef HAVE_JUCE
    count += getJuceApiCount();
# endif
#endif

    return count;
}

const char* CarlaEngine::getDriverName(const uint index2)
{
    carla_debug("CarlaEngine::getDriverName(%i)", index2);

    uint index(index2);

    if (jackbridge_is_ok() && index-- == 0)
        return "JACK";

#ifndef BUILD_BRIDGE
    if (index < getRtAudioApiCount())
        return getRtAudioApiName(index);

    index -= getRtAudioApiCount();

# ifdef HAVE_JUCE
    if (index < getJuceApiCount())
        return getJuceApiName(index);
# endif
#endif

    carla_stderr("CarlaEngine::getDriverName(%i) - invalid index", index);
    return nullptr;
}

const char* const* CarlaEngine::getDriverDeviceNames(const uint index2)
{
    carla_debug("CarlaEngine::getDriverDeviceNames(%i)", index2);

    uint index(index2);

    if (jackbridge_is_ok() && index-- == 0)
    {
        static const char* ret[3] = { "Auto-Connect OFF", "Auto-Connect ON", nullptr };
        return ret;
    }

#ifndef BUILD_BRIDGE
    if (index < getRtAudioApiCount())
        return getRtAudioApiDeviceNames(index);

    index -= getRtAudioApiCount();

# ifdef HAVE_JUCE
    if (index < getJuceApiCount())
        return getJuceApiDeviceNames(index);
# endif
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i) - invalid index", index);
    return nullptr;
}

const EngineDriverDeviceInfo* CarlaEngine::getDriverDeviceInfo(const uint index2, const char* const deviceName)
{
    carla_debug("CarlaEngine::getDriverDeviceInfo(%i, \"%s\")", index2, deviceName);

    uint index(index2);

    if (jackbridge_is_ok() && index-- == 0)
    {
        static uint32_t bufSizes[11] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 0 };
        static EngineDriverDeviceInfo devInfo;
        devInfo.hints       = ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE;
        devInfo.bufferSizes = bufSizes;
        devInfo.sampleRates = nullptr;
        return &devInfo;
    }

#ifndef BUILD_BRIDGE
    if (index < getRtAudioApiCount())
        return getRtAudioDeviceInfo(index, deviceName);

    index -= getRtAudioApiCount();

# ifdef HAVE_JUCE
    if (index < getJuceApiCount())
        return getJuceDeviceInfo(index, deviceName);
# endif
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i, \"%s\") - invalid index", index, deviceName);
    return nullptr;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', nullptr);
    carla_debug("CarlaEngine::newDriverByName(\"%s\")", driverName);

    if (std::strcmp(driverName, "JACK") == 0)
        return newJack();

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // common

    if (std::strncmp(driverName, "JACK ", 5) == 0)
        return newRtAudio(AUDIO_API_JACK);

    // -------------------------------------------------------------------
    // linux

    if (std::strcmp(driverName, "ALSA") == 0)
    {
#if 0//def HAVE_JUCE
        return newJuce(AUDIO_API_ALSA);
#else
        return newRtAudio(AUDIO_API_ALSA);
#endif
    }

    if (std::strcmp(driverName, "OSS") == 0)
        return newRtAudio(AUDIO_API_OSS);
    if (std::strcmp(driverName, "PulseAudio") == 0)
        return newRtAudio(AUDIO_API_PULSE);

    // -------------------------------------------------------------------
    // macos

    if (std::strcmp(driverName, "CoreAudio") == 0)
    {
#if 0//def HAVE_JUCE
        return newJuce(AUDIO_API_CORE);
#else
        return newRtAudio(AUDIO_API_CORE);
#endif
    }

    // -------------------------------------------------------------------
    // windows

    if (std::strcmp(driverName, "ASIO") == 0)
    {
#if 0//def HAVE_JUCE
        return newJuce(AUDIO_API_ASIO);
#else
        return newRtAudio(AUDIO_API_ASIO);
#endif
    }

    if (std::strcmp(driverName, "DirectSound") == 0)
    {
#if 0//def HAVE_JUCE
        return newJuce(AUDIO_API_DS);
#else
        return newRtAudio(AUDIO_API_DS);
#endif
    }
#endif

    carla_stderr("CarlaEngine::newDriverByName(\"%s\") - invalid driver name", driverName);
    return nullptr;
}

// -----------------------------------------------------------------------
// Maximum values

uint CarlaEngine::getMaxClientNameSize() const noexcept
{
    return STR_MAX/2;
}

uint CarlaEngine::getMaxPortNameSize() const noexcept
{
    return STR_MAX;
}

uint CarlaEngine::getCurrentPluginCount() const noexcept
{
    return pData->curPluginCount;
}

uint CarlaEngine::getMaxPluginNumber() const noexcept
{
    return pData->maxPluginNumber;
}

// -----------------------------------------------------------------------
// Virtual, per-engine type calls

bool CarlaEngine::init(const char* const clientName)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->name.isEmpty(), "Invalid engine internal data (err #1)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->oscData == nullptr, "Invalid engine internal data (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins == nullptr, "Invalid engine internal data (err #3)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->events.in  == nullptr, "Invalid engine internal data (err #4)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->events.out == nullptr, "Invalid engine internal data (err #5)");
    CARLA_SAFE_ASSERT_RETURN_ERR(clientName != nullptr && clientName[0] != '\0', "Invalid client name");
    carla_debug("CarlaEngine::init(\"%s\")", clientName);

    pData->aboutToClose    = false;
    pData->curPluginCount  = 0;
    pData->maxPluginNumber = 0;
    pData->nextPluginId    = 0;

    switch (pData->options.processMode)
    {
    case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
    case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
        pData->maxPluginNumber = MAX_DEFAULT_PLUGINS;
        break;

    case ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
        pData->maxPluginNumber = MAX_RACK_PLUGINS;
        pData->events.in  = new EngineEvent[kMaxEngineEventInternalCount];
        pData->events.out = new EngineEvent[kMaxEngineEventInternalCount];
        break;

    case ENGINE_PROCESS_MODE_PATCHBAY:
        pData->maxPluginNumber = MAX_PATCHBAY_PLUGINS;
        break;

    case ENGINE_PROCESS_MODE_BRIDGE:
        pData->maxPluginNumber = 1;
        pData->events.in  = new EngineEvent[kMaxEngineEventInternalCount];
        pData->events.out = new EngineEvent[kMaxEngineEventInternalCount];
        break;
    }

    CARLA_SAFE_ASSERT_RETURN_ERR(pData->maxPluginNumber != 0, "Invalid engine process mode");

    pData->nextPluginId = pData->maxPluginNumber;

    pData->name = clientName;
    pData->name.toBasic();

    pData->timeInfo.clear();

    pData->plugins = new EnginePluginData[pData->maxPluginNumber];

    for (uint i=0; i < pData->maxPluginNumber; ++i)
        pData->plugins[i].clear();

    pData->osc.init(clientName);

#ifndef BUILD_BRIDGE
    pData->oscData = pData->osc.getControlData();

    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        pData->graph.isRack = (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK);
        pData->graph.create();
    }
#endif

    pData->nextAction.ready();
    pData->thread.startThread();

    callback(ENGINE_CALLBACK_ENGINE_STARTED, 0, 0, 0, 0.0f, getCurrentDriverName());

    return true;
}

bool CarlaEngine::close()
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->name.isNotEmpty(), "Invalid engine internal data (err #6)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #7)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId == pData->maxPluginNumber, "Invalid engine internal data (err #8)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #9)");
    carla_debug("CarlaEngine::close()");

    pData->aboutToClose = true;

    if (pData->curPluginCount != 0)
        removeAllPlugins();

    pData->thread.stopThread(500);
    pData->nextAction.ready();

#ifndef BUILD_BRIDGE
    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        pData->graph.clear();
    }

    if (pData->osc.isControlRegistered())
        oscSend_control_exit();
#endif

    pData->osc.close();
    pData->oscData = nullptr;

    pData->curPluginCount = 0;
    pData->maxPluginNumber = 0;
    pData->nextPluginId = 0;

    if (pData->plugins != nullptr)
    {
        delete[] pData->plugins;
        pData->plugins = nullptr;
    }

    pData->events.clear();
#ifndef BUILD_BRIDGE
    pData->audio.clear();
#endif

    pData->name.clear();

    callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);

    return true;
}

void CarlaEngine::idle()
{
    CARLA_SAFE_ASSERT_RETURN(pData->nextAction.opcode == kEnginePostActionNull,); // TESTING, remove later
    CARLA_SAFE_ASSERT_RETURN(pData->nextPluginId == pData->maxPluginNumber,);     // TESTING, remove later
    CARLA_SAFE_ASSERT_RETURN(pData->plugins != nullptr,); // this one too maybe

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->idle();
    }

    idleOsc();
}

CarlaEngineClient* CarlaEngine::addClient(CarlaPlugin* const)
{
    return new CarlaEngineClient(*this);
}

// -----------------------------------------------------------------------
// Plugin management

bool CarlaEngine::addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, const int64_t uniqueId, const void* const extra)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #10)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId <= pData->maxPluginNumber, "Invalid engine internal data (err #11)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #12)");
    CARLA_SAFE_ASSERT_RETURN_ERR(btype != BINARY_NONE, "Invalid plugin params (err #1)");
    CARLA_SAFE_ASSERT_RETURN_ERR(ptype != PLUGIN_NONE, "Invalid plugin params (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERR((filename != nullptr && filename[0] != '\0') || (label != nullptr && label[0] != '\0'), "Invalid plugin params (err #3)");
    carla_debug("CarlaEngine::addPlugin(%i:%s, %i:%s, \"%s\", \"%s\", \"%s\", " P_INT64 ", %p)", btype, BinaryType2Str(btype), ptype, PluginType2Str(ptype), filename, name, label, uniqueId, extra);

    uint id;

#ifndef BUILD_BRIDGE
    CarlaPlugin* oldPlugin = nullptr;

    if (pData->nextPluginId < pData->curPluginCount)
    {
        id = pData->nextPluginId;
        pData->nextPluginId = pData->maxPluginNumber;

        oldPlugin = pData->plugins[id].plugin;

        CARLA_SAFE_ASSERT_RETURN_ERR(oldPlugin != nullptr, "Invalid replace plugin Id");
    }
    else
#endif
    {
        id = pData->curPluginCount;

        if (id == pData->maxPluginNumber)
        {
            setLastError("Maximum number of plugins reached");
            return false;
        }

        CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins[id].plugin == nullptr, "Invalid engine internal data (err #13)");
    }

    CarlaPlugin::Initializer initializer = {
        this,
        id,
        filename,
        name,
        label,
        uniqueId
    };

    CarlaPlugin* plugin = nullptr;

#ifndef BUILD_BRIDGE
    CarlaString bridgeBinary(pData->options.binaryDir);

    if (bridgeBinary.isNotEmpty())
    {
# ifdef CARLA_OS_LINUX
        // test for local build
        if (bridgeBinary.endsWith("/source/backend/"))
            bridgeBinary += "../bridges/";
# endif

# ifndef CARLA_OS_WIN
        if (btype == BINARY_NATIVE)
        {
            bridgeBinary += "carla-bridge-native";
        }
        else
# endif
        {
            switch (btype)
            {
            case BINARY_POSIX32:
                bridgeBinary += "carla-bridge-posix32";
                break;
            case BINARY_POSIX64:
                bridgeBinary += "carla-bridge-posix64";
                break;
            case BINARY_WIN32:
                bridgeBinary += "carla-bridge-win32.exe";
                break;
            case BINARY_WIN64:
                bridgeBinary += "carla-bridge-win64.exe";
                break;
            default:
                bridgeBinary.clear();
                break;
            }
        }

        QFile file(bridgeBinary.buffer());

        if (! file.exists())
            bridgeBinary.clear();
    }

    if (ptype != PLUGIN_INTERNAL && ptype != PLUGIN_JACK && (btype != BINARY_NATIVE || (pData->options.preferPluginBridges && bridgeBinary.isNotEmpty())))
    {
        if (bridgeBinary.isNotEmpty())
        {
            plugin = CarlaPlugin::newBridge(initializer, btype, ptype, bridgeBinary);
        }
# ifdef CARLA_OS_LINUX
        else if (btype == BINARY_WIN32)
        {
            // fallback to dssi-vst
            QFileInfo fileInfo(filename);

            CarlaString label2(fileInfo.fileName().toUtf8().constData());
            label2.replace(' ', '*');

            CarlaPlugin::Initializer init2 = {
                this,
                id,
                "/usr/lib/dssi/dssi-vst.so",
                name,
                label2,
                uniqueId
            };

            char* const oldVstPath(getenv("VST_PATH"));
            carla_setenv("VST_PATH", fileInfo.absoluteDir().absolutePath().toUtf8().constData());

            plugin = CarlaPlugin::newDSSI(init2);

            if (oldVstPath != nullptr)
                carla_setenv("VST_PATH", oldVstPath);
        }
# endif
        else
        {
            setLastError("This Carla build cannot handle this binary");
            return false;
        }
    }
    else
#endif // ! BUILD_BRIDGE
    {
        bool use16Outs;
        setLastError("Invalid or unsupported plugin type");

        switch (ptype)
        {
        case PLUGIN_NONE:
            break;

        case PLUGIN_INTERNAL:
            if (std::strcmp(label, "Csound") == 0)
            {
                plugin = CarlaPlugin::newCsound(initializer);
            }
            else if (std::strcmp(label, "FluidSynth") == 0)
            {
                use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
                plugin = CarlaPlugin::newFluidSynth(initializer, use16Outs);
            }
            else if (std::strcmp(label, "LinuxSampler (GIG)") == 0)
            {
                use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
                plugin = CarlaPlugin::newLinuxSampler(initializer, "GIG", use16Outs);
            }
            else if (std::strcmp(label, "LinuxSampler (SF2)") == 0)
            {
                use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
                plugin = CarlaPlugin::newLinuxSampler(initializer, "SF2", use16Outs);
            }
            else if (std::strcmp(label, "LinuxSampler (SFZ)") == 0)
            {
                use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
                plugin = CarlaPlugin::newLinuxSampler(initializer, "SFZ", use16Outs);
            }
            else
            {
                plugin = CarlaPlugin::newNative(initializer);
            }
            break;

        case PLUGIN_LADSPA:
            plugin = CarlaPlugin::newLADSPA(initializer, (const LADSPA_RDF_Descriptor*)extra);
            break;

        case PLUGIN_DSSI:
            plugin = CarlaPlugin::newDSSI(initializer);
            break;

        case PLUGIN_LV2:
            plugin = CarlaPlugin::newLV2(initializer);
            break;

        case PLUGIN_VST:
            plugin = CarlaPlugin::newVST(initializer);
            break;

        case PLUGIN_VST3:
            plugin = CarlaPlugin::newVST3(initializer);
            break;

        case PLUGIN_AU:
            plugin = CarlaPlugin::newAU(initializer);
            break;

        case PLUGIN_JACK:
            plugin = CarlaPlugin::newJACK(initializer);
            break;

        case PLUGIN_REWIRE:
            plugin = CarlaPlugin::newReWire(initializer);
            break;

        case PLUGIN_FILE_CSD:
            plugin = CarlaPlugin::newFileCSD(initializer);
            break;

        case PLUGIN_FILE_GIG:
            use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
            plugin = CarlaPlugin::newFileGIG(initializer, use16Outs);
            break;

        case PLUGIN_FILE_SF2:
            use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
            plugin = CarlaPlugin::newFileSF2(initializer, use16Outs);
            break;

        case PLUGIN_FILE_SFZ:
            plugin = CarlaPlugin::newFileSFZ(initializer);
            break;
        }
    }

    if (plugin == nullptr)
    {
#ifndef BUILD_BRIDGE
        pData->plugins[id].plugin = oldPlugin;
#endif
        return false;
    }

    plugin->registerToOscClient();

    EnginePluginData& pluginData(pData->plugins[id]);
    pluginData.plugin      = plugin;
    pluginData.insPeak[0]  = 0.0f;
    pluginData.insPeak[1]  = 0.0f;
    pluginData.outsPeak[0] = 0.0f;
    pluginData.outsPeak[1] = 0.0f;

#ifndef BUILD_BRIDGE
    if (oldPlugin != nullptr)
    {
        bool  wasActive = (oldPlugin->getInternalParameterValue(PARAMETER_ACTIVE) >= 0.5f);
        float oldDryWet =  oldPlugin->getInternalParameterValue(PARAMETER_DRYWET);
        float oldVolume =  oldPlugin->getInternalParameterValue(PARAMETER_VOLUME);

        delete oldPlugin;
        callback(ENGINE_CALLBACK_RELOAD_ALL, id, 0, 0, 0.0f, plugin->getName());

        if (wasActive)
            plugin->setActive(true, true, true);

        if (plugin->getHints() & PLUGIN_CAN_DRYWET)
            plugin->setDryWet(oldDryWet, true, true);

        if (plugin->getHints() & PLUGIN_CAN_VOLUME)
            plugin->setVolume(oldVolume, true, true);
    }
    else
#endif
    {
        ++pData->curPluginCount;
        callback(ENGINE_CALLBACK_PLUGIN_ADDED, id, 0, 0, 0.0f, plugin->getName());

        //if (pData->curPluginCount == 1 && pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        //    callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED, 0, PATCHBAY_ICON_CARLA, 0, 0.0f, nullptr);
    }

    return true;
}

bool CarlaEngine::addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, const int64_t uniqueId, const void* const extra)
{
    return addPlugin(BINARY_NATIVE, ptype, filename, name, label, uniqueId, extra);
}

bool CarlaEngine::removePlugin(const uint id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #14)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data (err #15)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #16)");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id (err #1)");
    carla_debug("CarlaEngine::removePlugin(%i)", id);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to remove");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data (err #17)");

    pData->thread.stopThread(500);

    const bool lockWait(isRunning() && pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);
    const ScopedActionLock sal(pData, kEnginePostActionRemovePlugin, id, 0, lockWait);

#ifndef BUILD_BRIDGE
    if (isOscControlRegistered())
        oscSend_control_remove_plugin(id);
#endif

    delete plugin;

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.startThread();

    callback(ENGINE_CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0.0f, nullptr);
    return true;
}

bool CarlaEngine::removeAllPlugins()
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #18)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId == pData->maxPluginNumber, "Invalid engine internal data (err #19)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #20)");
    carla_debug("CarlaEngine::removeAllPlugins()");

    if (pData->curPluginCount == 0)
        return true;

    pData->thread.stopThread(500);

    const bool lockWait(isRunning());
    const ScopedActionLock sal(pData, kEnginePostActionZeroCount, 0, 0, lockWait);

    callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

    for (uint i=0; i < pData->maxPluginNumber; ++i)
    {
        EnginePluginData& pluginData(pData->plugins[i]);

        if (pluginData.plugin != nullptr)
        {
            delete pluginData.plugin;
            pluginData.plugin = nullptr;
        }

        pluginData.insPeak[0]  = 0.0f;
        pluginData.insPeak[1]  = 0.0f;
        pluginData.outsPeak[0] = 0.0f;
        pluginData.outsPeak[1] = 0.0f;

        callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);
    }

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.startThread();

    return true;
}

const char* CarlaEngine::renamePlugin(const uint id, const char* const newName)
{
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->plugins != nullptr, "Invalid engine internal data (err #21)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->curPluginCount != 0, "Invalid engine internal data (err #22)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #23)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(id < pData->curPluginCount, "Invalid plugin Id (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(newName != nullptr && newName[0] != '\0', "Invalid plugin name");
    carla_debug("CarlaEngine::renamePlugin(%i, \"%s\")", id, newName);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERRN(plugin != nullptr, "Could not find plugin to rename");
    CARLA_SAFE_ASSERT_RETURN_ERRN(plugin->getId() == id, "Invalid engine internal data (err #24)");

    if (const char* const name = getUniquePluginName(newName))
    {
        plugin->setName(name);
        return name;
    }

    setLastError("Unable to get new unique plugin name");
    return nullptr;
}

bool CarlaEngine::clonePlugin(const uint id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #25)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data (err #26)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #27)");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id (err #3)");
    carla_debug("CarlaEngine::clonePlugin(%i)", id);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to clone");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data (err #28)");

    char label[STR_MAX+1];
    carla_zeroChar(label, STR_MAX+1);
    plugin->getLabel(label);

    const uint pluginCountBefore(pData->curPluginCount);

    if (! addPlugin(plugin->getBinaryType(), plugin->getType(), plugin->getFilename(), plugin->getName(), label, plugin->getUniqueId(), plugin->getExtraStuff()))
        return false;

    CARLA_SAFE_ASSERT_RETURN_ERR(pluginCountBefore+1 == pData->curPluginCount, "No new plugin found");

    if (CarlaPlugin* const newPlugin = pData->plugins[pluginCountBefore].plugin)
        newPlugin->loadSaveState(plugin->getSaveState());

    return true;
}

bool CarlaEngine::replacePlugin(const uint id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #29)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data (err #30)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #31)");
    carla_debug("CarlaEngine::replacePlugin(%i)", id);

    // might use this to reset
    if (id == pData->curPluginCount || id == pData->maxPluginNumber)
    {
        pData->nextPluginId = pData->maxPluginNumber;
        return true;
    }

    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id (err #4)");

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to replace");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data (err #32)");

    pData->nextPluginId = id;

    return true;
}

bool CarlaEngine::switchPlugins(const uint idA, const uint idB)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #33)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount >= 2, "Invalid engine internal data (err #34)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #35)");
    CARLA_SAFE_ASSERT_RETURN_ERR(idA != idB, "Invalid operation, cannot switch plugin with itself");
    CARLA_SAFE_ASSERT_RETURN_ERR(idA < pData->curPluginCount, "Invalid plugin Id (err #5)");
    CARLA_SAFE_ASSERT_RETURN_ERR(idB < pData->curPluginCount, "Invalid plugin Id (err #6)");
    carla_debug("CarlaEngine::switchPlugins(%i)", idA, idB);

    CarlaPlugin* const pluginA(pData->plugins[idA].plugin);
    CarlaPlugin* const pluginB(pData->plugins[idB].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA != nullptr, "Could not find plugin to switch (err #1)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA != nullptr, "Could not find plugin to switch (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA->getId() == idA, "Invalid engine internal data (err #36)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginB->getId() == idB, "Invalid engine internal data (err #37)");

    pData->thread.stopThread(500);

    const bool lockWait(isRunning() && pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);
    const ScopedActionLock sal(pData, kEnginePostActionSwitchPlugins, idA, idB, lockWait);

#ifndef BUILD_BRIDGE // TODO
    //if (isOscControlRegistered())
    //    oscSend_control_switch_plugins(idA, idB);
#endif

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.startThread();

    return true;
}

CarlaPlugin* CarlaEngine::getPlugin(const uint id) const
{
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->plugins != nullptr, "Invalid engine internal data (err #38)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->curPluginCount != 0, "Invalid engine internal data (err #39)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #40)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(id < pData->curPluginCount, "Invalid plugin Id (err #7)");

    return pData->plugins[id].plugin;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const uint id) const noexcept
{
    return pData->plugins[id].plugin;
}

const char* CarlaEngine::getUniquePluginName(const char* const name) const
{
    CARLA_SAFE_ASSERT_RETURN(pData->nextAction.opcode == kEnginePostActionNull, nullptr);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngine::getUniquePluginName(\"%s\")", name);

    CarlaString sname;
    sname = name;

    if (sname.isEmpty())
    {
        sname = "(No name)";
        return sname.dup();
    }

    const size_t maxNameSize(carla_min<uint>(getMaxClientNameSize(), 0xff, 6) - 6); // 6 = strlen(" (10)") + 1

    if (maxNameSize == 0 || ! isRunning())
        return sname.dup();

    sname.truncate(maxNameSize);
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CARLA_SAFE_ASSERT_BREAK(pData->plugins[i].plugin != nullptr);

        // Check if unique name doesn't exist
        if (const char* const pluginName = pData->plugins[i].plugin->getName())
        {
            if (sname != pluginName)
                continue;
        }

        // Check if string has already been modified
        {
            const size_t len(sname.length());

            // 1 digit, ex: " (2)"
            if (sname[len-4] == ' ' && sname[len-3] == '(' && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                int number = sname[len-2] - '0';

                if (number == 9)
                {
                    // next number is 10, 2 digits
                    sname.truncate(len-4);
                    sname += " (10)";
                    //sname.replace(" (9)", " (10)");
                }
                else
                    sname[len-2] = char('0' + number + 1);

                continue;
            }

            // 2 digits, ex: " (11)"
            if (sname[len-5] == ' ' && sname[len-4] == '(' && sname.isDigit(len-3) && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                char n2 = sname[len-2];
                char n3 = sname[len-3];

                if (n2 == '9')
                {
                    n2 = '0';
                    n3 = static_cast<char>(n3 + 1);
                }
                else
                    n2 = static_cast<char>(n2 + 1);

                sname[len-2] = n2;
                sname[len-3] = n3;

                continue;
            }
        }

        // Modify string if not
        sname += " (2)";
    }

    return sname.dup();
}

// -----------------------------------------------------------------------
// Project management

bool CarlaEngine::loadFile(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename (err #1)");
    carla_debug("CarlaEngine::loadFile(\"%s\")", filename);

    QFileInfo fileInfo(filename);

    if (! fileInfo.exists())
    {
        setLastError("File does not exist");
        return false;
    }

    if (! fileInfo.isFile())
    {
        setLastError("Not a file");
        return false;
    }

    if (! fileInfo.isReadable())
    {
        setLastError("File is not readable");
        return false;
    }

    CarlaString baseName(fileInfo.baseName().toUtf8().constData());
    CarlaString extension(fileInfo.suffix().toLower().toUtf8().constData());
    extension.toLower();

    // -------------------------------------------------------------------

    if (extension == "carxp" || extension == "carxs")
        return loadProject(filename);

    // -------------------------------------------------------------------

    if (extension == "csd")
        return addPlugin(PLUGIN_FILE_CSD, filename, baseName, baseName, 0, nullptr);

    if (extension == "gig")
        return addPlugin(PLUGIN_FILE_GIG, filename, baseName, baseName, 0, nullptr);

    if (extension == "sf2")
        return addPlugin(PLUGIN_FILE_SF2, filename, baseName, baseName, 0, nullptr);

    if (extension == "sfz")
        return addPlugin(PLUGIN_FILE_SFZ, filename, baseName, baseName, 0, nullptr);

    // -------------------------------------------------------------------

    if (extension == "aiff" || extension == "flac" || extension == "oga" || extension == "ogg" || extension == "w64" || extension == "wav")
    {
#ifdef WANT_AUDIOFILE
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "audiofile", 0, nullptr))
        {
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
            return true;
        }
        return false;
#else
        setLastError("This Carla build does not have Audio file support");
        return false;
#endif
    }

    if (extension == "3g2" || extension == "3gp" || extension == "aac" || extension == "ac3" || extension == "amr" || extension == "ape" ||
            extension == "mp2" || extension == "mp3" || extension == "mpc" || extension == "wma")
    {
#ifdef WANT_AUDIOFILE
# ifdef HAVE_FFMPEG
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "audiofile", 0, nullptr))
        {
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
            return true;
        }
        return false;
# else
        setLastError("This Carla build has Audio file support, but not libav/ffmpeg");
        return false;
# endif
#else
        setLastError("This Carla build does not have Audio file support");
        return false;
#endif
    }

    // -------------------------------------------------------------------

    if (extension == "mid" || extension == "midi")
    {
#ifdef WANT_MIDIFILE
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "midifile", 0, nullptr))
        {
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
            return true;
        }
        return false;
#else
        setLastError("This Carla build does not have MIDI file support");
        return false;
#endif
    }

    // -------------------------------------------------------------------
    // ZynAddSubFX

    if (extension == "xmz" || extension == "xiz")
    {
#ifdef WANT_ZYNADDSUBFX
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "zynaddsubfx", 0, nullptr))
        {
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, (extension == "xmz") ? "CarlaAlternateFile1" : "CarlaAlternateFile2", filename, true);
            return true;
        }
        return false;
#else
        setLastError("This Carla build does not have ZynAddSubFX support");
        return false;
#endif
    }

    // -------------------------------------------------------------------

    setLastError("Unknown file extension");
    return false;
}

bool CarlaEngine::loadProject(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename (err #2)");
    carla_debug("CarlaEngine::loadProject(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QDomDocument xml;
    xml.setContent(file.readAll());
    file.close();

    QDomNode xmlNode(xml.documentElement());

    const bool isPreset(xmlNode.toElement().tagName().compare("carla-preset", Qt::CaseInsensitive) == 0);

    if (xmlNode.toElement().tagName().compare("carla-project", Qt::CaseInsensitive) != 0 && ! isPreset)
    {
        setLastError("Not a valid Carla project or preset file");
        return false;
    }

    // handle plugins first
    for (QDomNode node = xmlNode.firstChild(); ! node.isNull(); node = node.nextSibling())
    {
        if (isPreset || node.toElement().tagName().compare("plugin", Qt::CaseInsensitive) == 0)
        {
            SaveState saveState;
            fillSaveStateFromXmlNode(saveState, isPreset ? xmlNode : node);

            callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

            CARLA_SAFE_ASSERT_CONTINUE(saveState.type != nullptr);

            const void* extraStuff = nullptr;

            // check if using GIG, SF2 or SFZ 16outs
            static const char kUse16OutsSuffix[] = " (16 outs)";

            const PluginType ptype(getPluginTypeFromString(saveState.type));

            if (CarlaString(saveState.label).endsWith(kUse16OutsSuffix))
            {
                if (ptype == PLUGIN_FILE_GIG || ptype == PLUGIN_FILE_SF2)
                    extraStuff = "true";
            }

            // TODO - proper find&load plugins

            if (addPlugin(ptype, saveState.binary, saveState.name, saveState.label, saveState.uniqueId, extraStuff))
            {
                if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
                    plugin->loadSaveState(saveState);
            }
            else
                carla_stderr2("Failed to load a plugin, error was:%s\n", getLastError());
        }

        if (isPreset)
            return true;
    }

#ifndef BUILD_BRIDGE
    callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

    // if we're running inside some session-manager, let them handle the connections
    if (pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY)
    {
        if (std::getenv("LADISH_APP_NAME") != nullptr || std::getenv("NSM_URL") != nullptr)
            return true;
    }

    // now connections
    for (QDomNode node = xmlNode.firstChild(); ! node.isNull(); node = node.nextSibling())
    {
        if (node.toElement().tagName().compare("patchbay", Qt::CaseInsensitive) == 0)
        {
            CarlaString sourcePort, targetPort;

            for (QDomNode patchNode = node.firstChild(); ! patchNode.isNull(); patchNode = patchNode.nextSibling())
            {
                sourcePort.clear();
                targetPort.clear();

                if (patchNode.toElement().tagName().compare("connection", Qt::CaseInsensitive) != 0)
                    continue;

                for (QDomNode connNode = patchNode.firstChild(); ! connNode.isNull(); connNode = connNode.nextSibling())
                {
                    const QString tag(connNode.toElement().tagName());
                    const QString text(connNode.toElement().text().trimmed());

                    if (tag.compare("source", Qt::CaseInsensitive) == 0)
                        sourcePort = text.toUtf8().constData();
                    else if (tag.compare("target", Qt::CaseInsensitive) == 0)
                        targetPort = text.toUtf8().constData();
                }

                if (sourcePort.isNotEmpty() && targetPort.isNotEmpty())
                    restorePatchbayConnection(sourcePort, targetPort);
            }
            break;
        }
    }
#endif

    return true;
}

bool CarlaEngine::saveProject(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename (err #3)");
    carla_debug("CarlaEngine::saveProject(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PROJECT>\n";
    out << "<CARLA-PROJECT VERSION='2.0'>\n";

    bool firstPlugin = true;
    char strBuf[STR_MAX+1];

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
        {
            if (! firstPlugin)
                out << "\n";

            strBuf[0] = '\0';

            plugin->getRealName(strBuf);

            //if (strBuf[0] != '\0')
            //    out << QString(" <!-- %1 -->\n").arg(xmlSafeString(strBuf, true));

            QString content;
            fillXmlStringFromSaveState(content, plugin->getSaveState());

            out << " <Plugin>\n";
            out << content;
            out << " </Plugin>\n";

            firstPlugin = false;
        }
    }

#ifndef BUILD_BRIDGE
    // if we're running inside some session-manager, let them handle the connections
    if (pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY)
    {
        if (std::getenv("LADISH_APP_NAME") != nullptr || std::getenv("NSM_URL") != nullptr)
            return true;
    }

    if (const char* const* patchbayConns = getPatchbayConnections())
    {
        if (! firstPlugin)
            out << "\n";

        out << " <Patchbay>\n";

        for (int i=0; patchbayConns[i] != nullptr && patchbayConns[i+1] != nullptr; ++i, ++i )
        {
            const char* const connSource(patchbayConns[i]);
            const char* const connTarget(patchbayConns[i+1]);

            CARLA_SAFE_ASSERT_CONTINUE(connSource != nullptr && connSource[0] != '\0');
            CARLA_SAFE_ASSERT_CONTINUE(connTarget != nullptr && connTarget[0] != '\0');

            out << "  <Connection>\n";
            out << "   <Source>" << connSource << "</Source>\n";
            out << "   <Target>" << connTarget << "</Target>\n";
            out << "  </Connection>\n";

            delete[] connSource;
            delete[] connTarget;
        }

        out << " </Patchbay>\n";
    }
#endif

    out << "</CARLA-PROJECT>\n";

    file.close();
    return true;
}

// -----------------------------------------------------------------------
// Information (base)

uint CarlaEngine::getHints() const noexcept
{
    return pData->hints;
}

uint32_t CarlaEngine::getBufferSize() const noexcept
{
    return pData->bufferSize;
}

double CarlaEngine::getSampleRate() const noexcept
{
    return pData->sampleRate;
}

const char* CarlaEngine::getName() const noexcept
{
    return pData->name;
}

EngineProcessMode CarlaEngine::getProccessMode() const noexcept
{
    return pData->options.processMode;
}

const EngineOptions& CarlaEngine::getOptions() const noexcept
{
    return pData->options;
}

const EngineTimeInfo& CarlaEngine::getTimeInfo() const noexcept
{
    return pData->timeInfo;
}

// -----------------------------------------------------------------------
// Information (peaks)

float CarlaEngine::getInputPeak(const uint pluginId, const bool isLeft) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);

    return pData->plugins[pluginId].insPeak[isLeft ? 0 : 1];
}

float CarlaEngine::getOutputPeak(const uint pluginId, const bool isLeft) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);

    return pData->plugins[pluginId].outsPeak[isLeft ? 0 : 1];
}

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr) noexcept
{
    carla_debug("CarlaEngine::callback(%s, %i, %i, %i, %f, \"%s\")", EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);

    if (pData->callback != nullptr)
    {
        try {
            pData->callback(pData->callbackPtr, action, pluginId, value1, value2, value3, valueStr);
        } catch(...) {}
    }
}

void CarlaEngine::setCallback(const EngineCallbackFunc func, void* const ptr) noexcept
{
    carla_debug("CarlaEngine::setCallback(%p, %p)", func, ptr);

    pData->callback    = func;
    pData->callbackPtr = ptr;
}

// -----------------------------------------------------------------------
// File Callback

const char* CarlaEngine::runFileCallback(const FileCallbackOpcode action, const bool isDir, const char* const title, const char* const filter) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(title != nullptr && title[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(filter != nullptr, nullptr);
    carla_debug("CarlaEngine::runFileCallback(%i:%s, %s, \"%s\", \"%s\")", action, FileCallbackOpcode2Str(action), bool2str(isDir), title, filter);

    const char* ret = nullptr;

    if (pData->fileCallback != nullptr)
    {
        try {
            ret = pData->fileCallback(pData->fileCallbackPtr, action, isDir, title, filter);
        } catch(...) {}
    }

    return ret;
}

void CarlaEngine::setFileCallback(const FileCallbackFunc func, void* const ptr) noexcept
{
    carla_debug("CarlaEngine::setFileCallback(%p, %p)", func, ptr);

    pData->fileCallback    = func;
    pData->fileCallbackPtr = ptr;
}

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// Patchbay

bool CarlaEngine::patchbayConnect(const int groupA, const int portA, const int groupB, const int portB)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->audio.isReady, false);
    carla_debug("CarlaEngine::patchbayConnect(%i, %i)", portA, portB);

    if (portA < 0 || portB < 0)
    {
        setLastError("Invalid connection");
        return false;
    }

    if (pData->graph.isRack)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.rack != nullptr, nullptr);
        return pData->graph.rack->connect(this, groupA, portA, groupB, portB);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.patchbay != nullptr, nullptr);
        return pData->graph.patchbay->connect(this, groupA, portA, groupB, portB);
    }
}

bool CarlaEngine::patchbayDisconnect(const uint connectionId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
    CARLA_SAFE_ASSERT_RETURN(pData->audio.isReady, false);
    carla_debug("CarlaEngineRtAudio::patchbayDisconnect(%i)", connectionId);

    if (pData->graph.isRack)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.rack != nullptr, nullptr);
        return pData->graph.rack->disconnect(this, connectionId);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.patchbay != nullptr, nullptr);
        return pData->graph.patchbay->disconnect(this, connectionId);
    }
}

bool CarlaEngine::patchbayRefresh()
{
    setLastError("Unsupported operation");
    return false;
}
#endif

// -----------------------------------------------------------------------
// Transport

void CarlaEngine::transportPlay() noexcept
{
    pData->time.playing = true;
}

void CarlaEngine::transportPause() noexcept
{
    pData->time.playing = false;
}

void CarlaEngine::transportRelocate(const uint64_t frame) noexcept
{
    pData->time.frame = frame;
}

// -----------------------------------------------------------------------
// Error handling

const char* CarlaEngine::getLastError() const noexcept
{
    return pData->lastError;
}

void CarlaEngine::setLastError(const char* const error) const noexcept
{
    pData->lastError = error;
}

void CarlaEngine::setAboutToClose() noexcept
{
    carla_debug("CarlaEngine::setAboutToClose()");

    pData->aboutToClose = true;
}

// -----------------------------------------------------------------------
// Global options

void CarlaEngine::setOption(const EngineOption option, const int value, const char* const valueStr)
{
    carla_debug("CarlaEngine::setOption(%i:%s, %i, \"%s\")", option, EngineOption2Str(option), value, valueStr);

    if (isRunning() && (option == ENGINE_OPTION_PROCESS_MODE || option == ENGINE_OPTION_AUDIO_NUM_PERIODS || option == ENGINE_OPTION_AUDIO_DEVICE))
        return carla_stderr("CarlaEngine::setOption(%i:%s, %i, \"%s\") - Cannot set this option while engine is running!", option, EngineOption2Str(option), value, valueStr);

    switch (option)
    {
    case ENGINE_OPTION_DEBUG:
    case ENGINE_OPTION_NSM_INIT:
        break;

    case ENGINE_OPTION_PROCESS_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= ENGINE_PROCESS_MODE_SINGLE_CLIENT && value <= ENGINE_PROCESS_MODE_BRIDGE,);
        pData->options.processMode = static_cast<EngineProcessMode>(value);
        break;

    case ENGINE_OPTION_TRANSPORT_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= ENGINE_TRANSPORT_MODE_INTERNAL && value <= ENGINE_TRANSPORT_MODE_BRIDGE,);
        pData->options.transportMode = static_cast<EngineTransportMode>(value);
        break;

    case ENGINE_OPTION_FORCE_STEREO:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.forceStereo = (value != 0);
        break;

    case ENGINE_OPTION_PREFER_PLUGIN_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.preferPluginBridges = (value != 0);
        break;

    case ENGINE_OPTION_PREFER_UI_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.preferUiBridges = (value != 0);
        break;

    case ENGINE_OPTION_UIS_ALWAYS_ON_TOP:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.uisAlwaysOnTop = (value != 0);
        break;

    case ENGINE_OPTION_MAX_PARAMETERS:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        pData->options.maxParameters = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_UI_BRIDGES_TIMEOUT:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        pData->options.uiBridgesTimeout = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_NUM_PERIODS:
        CARLA_SAFE_ASSERT_RETURN(value >= 2 && value <= 3,);
        pData->options.audioNumPeriods = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_BUFFER_SIZE:
        CARLA_SAFE_ASSERT_RETURN(value >= 8,);
        pData->options.audioBufferSize = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_SAMPLE_RATE:
        CARLA_SAFE_ASSERT_RETURN(value >= 22050,);
        pData->options.audioSampleRate = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_DEVICE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

        if (pData->options.audioDevice != nullptr)
            delete[] pData->options.audioDevice;

        pData->options.audioDevice = carla_strdup(valueStr);
        break;

    case ENGINE_OPTION_PATH_BINARIES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.binaryDir != nullptr)
            delete[] pData->options.binaryDir;

        pData->options.binaryDir = carla_strdup(valueStr);
        break;

    case ENGINE_OPTION_PATH_RESOURCES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.resourceDir != nullptr)
            delete[] pData->options.resourceDir;

        pData->options.resourceDir = carla_strdup(valueStr);
        break;

    case ENGINE_OPTION_FRONTEND_WIN_ID:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);
        const long long winId(std::strtoll(valueStr, nullptr, 16));
        CARLA_SAFE_ASSERT_RETURN(winId >= 0,);
        pData->options.frontendWinId = static_cast<uintptr_t>(winId);
        break;
    }
}

// -----------------------------------------------------------------------
// OSC Stuff

#ifdef BUILD_BRIDGE
bool CarlaEngine::isOscBridgeRegistered() const noexcept
{
    return (pData->oscData != nullptr);
}
#else
bool CarlaEngine::isOscControlRegistered() const noexcept
{
    return pData->osc.isControlRegistered();
}
#endif


void CarlaEngine::idleOsc() const noexcept
{
    try {
        pData->osc.idle();
    } catch(...) {}
}

const char* CarlaEngine::getOscServerPathTCP() const noexcept
{
    return pData->osc.getServerPathTCP();
}

const char* CarlaEngine::getOscServerPathUDP() const noexcept
{
    return pData->osc.getServerPathUDP();
}

#ifdef BUILD_BRIDGE
void CarlaEngine::setOscBridgeData(const CarlaOscData* const oscData) const noexcept
{

    pData->oscData = oscData;
}
#endif

// -----------------------------------------------------------------------
// Helper functions

EngineEvent* CarlaEngine::getInternalEventBuffer(const bool isInput) const noexcept
{
    return isInput ? pData->events.in : pData->events.out;
}

void CarlaEngine::registerEnginePlugin(const uint id, CarlaPlugin* const plugin) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(id == pData->curPluginCount,);
    carla_debug("CarlaEngine::registerEnginePlugin(%i, %p)", id, plugin);

    pData->plugins[id].plugin = plugin;
}

// -----------------------------------------------------------------------
// Internal stuff

void CarlaEngine::bufferSizeChanged(const uint32_t newBufferSize)
{
    carla_debug("CarlaEngine::bufferSizeChanged(%i)", newBufferSize);

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->bufferSizeChanged(newBufferSize);
    }

    callback(ENGINE_CALLBACK_BUFFER_SIZE_CHANGED, 0, static_cast<int>(newBufferSize), 0, 0.0f, nullptr);
}

void CarlaEngine::sampleRateChanged(const double newSampleRate)
{
    carla_debug("CarlaEngine::sampleRateChanged(%g)", newSampleRate);

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->sampleRateChanged(newSampleRate);
    }

    callback(ENGINE_CALLBACK_SAMPLE_RATE_CHANGED, 0, 0, 0, static_cast<float>(newSampleRate), nullptr);
}

void CarlaEngine::offlineModeChanged(const bool isOfflineNow)
{
    carla_debug("CarlaEngine::offlineModeChanged(%s)", bool2str(isOfflineNow));

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->offlineModeChanged(isOfflineNow);
    }
}

void CarlaEngine::runPendingRtEvents() noexcept
{
    pData->doNextPluginAction(true);

    if (pData->time.playing)
        pData->time.frame += pData->bufferSize;

    if (pData->options.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
    {
        pData->timeInfo.playing = pData->time.playing;
        pData->timeInfo.frame   = pData->time.frame;
    }
}

void CarlaEngine::setPluginPeaks(const uint pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept
{
    EnginePluginData& pluginData(pData->plugins[pluginId]);

    pluginData.insPeak[0]  = inPeaks[0];
    pluginData.insPeak[1]  = inPeaks[1];
    pluginData.outsPeak[0] = outPeaks[0];
    pluginData.outsPeak[1] = outPeaks[1];
}

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// Patchbay stuff

const char* const* CarlaEngine::getPatchbayConnections() const
{
    carla_debug("CarlaEngine::getPatchbayConnections()");

    if (pData->graph.isRack)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.rack != nullptr, nullptr);
        return pData->graph.rack->getConnections();
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.patchbay != nullptr, nullptr);
        return pData->graph.patchbay->getConnections();
    }
}

void CarlaEngine::restorePatchbayConnection(const char* const connSource, const char* const connTarget)
{
    CARLA_SAFE_ASSERT_RETURN(connSource != nullptr && connSource[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(connTarget != nullptr && connTarget[0] != '\0',);
    carla_debug("CarlaEngine::restorePatchbayConnection(\"%s\", \"%s\")", connSource, connTarget);

    int sourceGroup, targetGroup;
    int sourcePort,  targetPort;

    if (pData->graph.isRack)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.rack != nullptr,);
        if (! pData->graph.rack->getPortIdFromName(connSource, sourceGroup, sourcePort))
            return;
        if (! pData->graph.rack->getPortIdFromName(connTarget, targetGroup, targetPort))
            return;
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.patchbay != nullptr,);
        if (! pData->graph.patchbay->getPortIdFromName(connSource, sourceGroup, sourcePort))
            return;
        if (! pData->graph.patchbay->getPortIdFromName(connTarget, targetGroup, targetPort))
            return;
    }

    patchbayConnect(targetGroup, targetPort, sourceGroup, sourcePort);
}
#endif

// -----------------------------------------------------------------------
// Bridge/Controller OSC stuff

#ifdef BUILD_BRIDGE
void CarlaEngine::oscSend_bridge_plugin_info1(const PluginCategory category, const uint hints, const int64_t uniqueId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_plugin_info1(%i:%s, %X, " P_INT64 ")", category, PluginCategory2Str(category), hints, uniqueId);

    char targetPath[std::strlen(pData->oscData->path)+21];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_plugin_info1");
    try_lo_send(pData->oscData->target, targetPath, "iih", static_cast<int32_t>(category), static_cast<int32_t>(hints), uniqueId);
}

void CarlaEngine::oscSend_bridge_plugin_info2(const char* const realName, const char* const label, const char* const maker, const char* const copyright) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(realName != nullptr && realName[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(maker != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(copyright != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_plugin_info2(\"%s\", \"%s\", \"%s\", \"%s\")", realName, label, maker, copyright);

    char targetPath[std::strlen(pData->oscData->path)+21];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_plugin_info2");
    try_lo_send(pData->oscData->target, targetPath, "ssss", realName, label, maker, copyright);
}

void CarlaEngine::oscSend_bridge_audio_count(const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_audio_count(%i, %i)", ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+20];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_audio_count");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_bridge_midi_count(const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_midi_count(%i, %i)", ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+19];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_midi_count");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_bridge_parameter_count(const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_count(%i, %i)", ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_parameter_count");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_bridge_program_count(const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_program_count(%i)", count);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_program_count");
    try_lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(count));
}

void CarlaEngine::oscSend_bridge_midi_program_count(const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_midi_program_count(%i)", count);

    char targetPath[std::strlen(pData->oscData->path)+27];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_midi_program_count");
    try_lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(count));
}

void CarlaEngine::oscSend_bridge_parameter_data(const uint32_t index, const int32_t rindex, const ParameterType type, const uint hints, const char* const name, const char* const unit) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(unit != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_data(%i, %i, %i:%s, %X, \"%s\", \"%s\")", index, rindex, type, ParameterType2Str(type), hints, name, unit);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_parameter_data");
    try_lo_send(pData->oscData->target, targetPath, "iiiiss", static_cast<int32_t>(index), static_cast<int32_t>(rindex), static_cast<int32_t>(type), static_cast<int32_t>(hints), name, unit);
}

void CarlaEngine::oscSend_bridge_parameter_ranges1(const uint32_t index, const float def, const float min, const float max) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_ranges(%i, %f, %f, %f)", index, def, min, max);

    char targetPath[std::strlen(pData->oscData->path)+26];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_parameter_ranges1");
    try_lo_send(pData->oscData->target, targetPath, "ifff", static_cast<int32_t>(index), def, min, max);
}

void CarlaEngine::oscSend_bridge_parameter_ranges2(const uint32_t index, const float step, const float stepSmall, const float stepLarge) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_ranges(%i, %f, %f, %f)", index, step, stepSmall, stepLarge);

    char targetPath[std::strlen(pData->oscData->path)+26];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_parameter_ranges2");
    try_lo_send(pData->oscData->target, targetPath, "ifff", static_cast<int32_t>(index), step, stepSmall, stepLarge);
}

void CarlaEngine::oscSend_bridge_parameter_midi_cc(const uint32_t index, const int16_t cc) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_midi_cc(%i, %i)", index, cc);

    char targetPath[std::strlen(pData->oscData->path)+26];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_parameter_midi_cc");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(index), static_cast<int32_t>(cc));
}

void CarlaEngine::oscSend_bridge_parameter_midi_channel(const uint32_t index, const uint8_t channel) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_midi_channel(%i, %i)", index, channel);

    char targetPath[std::strlen(pData->oscData->path)+31];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_parameter_midi_channel");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(index), static_cast<int32_t>(channel));
}

void CarlaEngine::oscSend_bridge_parameter_value(const uint32_t index, const float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_value(%i, %f)", index, value);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_parameter_value");
    try_lo_send(pData->oscData->target, targetPath, "if", static_cast<int32_t>(index), value);
}

void CarlaEngine::oscSend_bridge_default_value(const uint32_t index, const float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_default_value(%i, %f)", index, value);

    char targetPath[std::strlen(pData->oscData->path)+22];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_default_value");
    try_lo_send(pData->oscData->target, targetPath, "if", static_cast<int32_t>(index), value);
}

void CarlaEngine::oscSend_bridge_current_program(const int32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_current_program(%i)", index);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_current_program");
    try_lo_send(pData->oscData->target, targetPath, "i", index);
}

void CarlaEngine::oscSend_bridge_current_midi_program(const int32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_current_midi_program(%i)", index);

    char targetPath[std::strlen(pData->oscData->path)+30];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_current_midi_program");
    try_lo_send(pData->oscData->target, targetPath, "i", index);
}

void CarlaEngine::oscSend_bridge_program_name(const uint32_t index, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_program_name(%i, \"%s\")", index, name);

    char targetPath[std::strlen(pData->oscData->path)+21];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_program_name");
    try_lo_send(pData->oscData->target, targetPath, "is", static_cast<int32_t>(index), name);
}

void CarlaEngine::oscSend_bridge_midi_program_data(const uint32_t index, const uint32_t bank, const uint32_t program, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_midi_program_data(%i, %i, %i, \"%s\")", index, bank, program, name);

    char targetPath[std::strlen(pData->oscData->path)+26];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_midi_program_data");
    try_lo_send(pData->oscData->target, targetPath, "iiis", static_cast<int32_t>(index), static_cast<int32_t>(bank), static_cast<int32_t>(program), name);
}

void CarlaEngine::oscSend_bridge_configure(const char* const key, const char* const value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_configure(\"%s\", \"%s\")", key, value);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_configure");
    try_lo_send(pData->oscData->target, targetPath, "ss", key, value);
}

void CarlaEngine::oscSend_bridge_set_custom_data(const char* const type, const char* const key, const char* const value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_set_custom_data(\"%s\", \"%s\", \"%s\")", type, key, value);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_set_custom_data");
    try_lo_send(pData->oscData->target, targetPath, "sss", type, key, value);
}

void CarlaEngine::oscSend_bridge_set_chunk_data(const char* const chunkFile) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(chunkFile != nullptr && chunkFile[0] != '\0',);
    carla_debug("CarlaEngine::oscSend_bridge_set_chunk_data(\"%s\")", chunkFile);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_set_chunk_data");
    try_lo_send(pData->oscData->target, targetPath, "s", chunkFile);
}

void CarlaEngine::oscSend_bridge_pong() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    //carla_debug("CarlaEngine::oscSend_pong()");

    char targetPath[std::strlen(pData->oscData->path)+13];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/bridge_pong");
    try_lo_send(pData->oscData->target, targetPath, "");
}
#else
void CarlaEngine::oscSend_control_add_plugin_start(const uint pluginId, const char* const pluginName) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(pluginName != nullptr && pluginName[0] != '\0',);
    carla_debug("CarlaEngine::oscSend_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/add_plugin_start");
    try_lo_send(pData->oscData->target, targetPath, "is", static_cast<int32_t>(pluginId), pluginName);
}

void CarlaEngine::oscSend_control_add_plugin_end(const uint pluginId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_add_plugin_end(%i)", pluginId);

    char targetPath[std::strlen(pData->oscData->path)+16];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/add_plugin_end");
    try_lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(pluginId));
}

void CarlaEngine::oscSend_control_remove_plugin(const uint pluginId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_remove_plugin(%i)", pluginId);

    char targetPath[std::strlen(pData->oscData->path)+15];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/remove_plugin");
    try_lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(pluginId));
}

void CarlaEngine::oscSend_control_set_plugin_info1(const uint pluginId, const PluginType type, const PluginCategory category, const uint hints, const int64_t uniqueId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(type != PLUGIN_NONE,);
    carla_debug("CarlaEngine::oscSend_control_set_plugin_data(%i, %i:%s, %i:%s, %X, " P_INT64 ")", pluginId, type, PluginType2Str(type), category, PluginCategory2Str(category), hints, uniqueId);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_plugin_info1");
    try_lo_send(pData->oscData->target, targetPath, "iiiih", static_cast<int32_t>(pluginId), static_cast<int32_t>(type), static_cast<int32_t>(category), static_cast<int32_t>(hints), static_cast<int64_t>(uniqueId));
}

void CarlaEngine::oscSend_control_set_plugin_info2(const uint pluginId, const char* const realName, const char* const label, const char* const maker, const char* const copyright) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(realName != nullptr && realName[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(maker != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(copyright != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_plugin_data(%i, \"%s\", \"%s\", \"%s\", \"%s\")", pluginId, realName, label, maker, copyright);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_plugin_info2");
    try_lo_send(pData->oscData->target, targetPath, "issss", static_cast<int32_t>(pluginId), realName, label, maker, copyright);
}

void CarlaEngine::oscSend_control_set_audio_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_audio_count(%i, %i, %i)", pluginId, ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_audio_count");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_control_set_midi_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_count(%i, %i, %i)", pluginId, ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_midi_count");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_control_set_parameter_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_count(%i, %i, %i)", pluginId, ins, outs);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_count");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
}

void CarlaEngine::oscSend_control_set_program_count(const uint pluginId, const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_program_count(%i, %i)", pluginId, count);

    char targetPath[std::strlen(pData->oscData->path)+19];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_program_count");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
}

void CarlaEngine::oscSend_control_set_midi_program_count(const uint pluginId, const uint32_t count) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_count(%i, %i)", pluginId, count);

    char targetPath[std::strlen(pData->oscData->path)+24];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_midi_program_count");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
}

void CarlaEngine::oscSend_control_set_parameter_data(const uint pluginId, const uint32_t index, const ParameterType type, const uint hints, const char* const name, const char* const unit) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(unit != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_data(%i, %i, %i:%s, %X, \"%s\", \"%s\")", pluginId, index, type, ParameterType2Str(type), hints, name, unit);

    char targetPath[std::strlen(pData->oscData->path)+20];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_data");
    try_lo_send(pData->oscData->target, targetPath, "iiiiss", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(type), static_cast<int32_t>(hints), name, unit);
}

void CarlaEngine::oscSend_control_set_parameter_ranges1(const uint pluginId, const uint32_t index, const float def, const float min, const float max) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(def <= min && def >= max,);
    CARLA_SAFE_ASSERT_RETURN(min < max,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_ranges1(%i, %i, %f, %f, %f)", pluginId, index, def, min, max, def);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_ranges1");
    try_lo_send(pData->oscData->target, targetPath, "iifff", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), def, min, max);
}

void CarlaEngine::oscSend_control_set_parameter_ranges2(const uint pluginId, const uint32_t index, const float step, const float stepSmall, const float stepLarge) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(step <= stepSmall && step >= stepLarge,);
    CARLA_SAFE_ASSERT_RETURN(stepSmall <= stepLarge,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_ranges2(%i, %i, %f, %f, %f)", pluginId, index, step, stepSmall, stepLarge);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_ranges");
    try_lo_send(pData->oscData->target, targetPath, "iifff", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), step, stepSmall, stepLarge);
}

void CarlaEngine::oscSend_control_set_parameter_midi_cc(const uint pluginId, const uint32_t index, const int16_t cc) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(cc <= 0x5F,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_midi_cc");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(cc));
}

void CarlaEngine::oscSend_control_set_parameter_midi_channel(const uint pluginId, const uint32_t index, const uint8_t channel) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);

    char targetPath[std::strlen(pData->oscData->path)+28];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_midi_channel");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(channel));
}

void CarlaEngine::oscSend_control_set_parameter_value(const uint pluginId, const int32_t index, const float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(index != PARAMETER_NULL,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_value(%i, %i:%s, %f)", pluginId, index, (index < 0) ? InternalParameterIndex2Str(static_cast<InternalParameterIndex>(index)) : "(none)", value);

    char targetPath[std::strlen(pData->oscData->path)+21];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_parameter_value");
    try_lo_send(pData->oscData->target, targetPath, "iif", static_cast<int32_t>(pluginId), index, value);
}

void CarlaEngine::oscSend_control_set_default_value(const uint pluginId, const uint32_t index, const float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_default_value(%i, %i, %f)", pluginId, index, value);

    char targetPath[std::strlen(pData->oscData->path)+19];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_default_value");
    try_lo_send(pData->oscData->target, targetPath, "iif", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), value);
}

void CarlaEngine::oscSend_control_set_current_program(const uint pluginId, const int32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_current_program(%i, %i)", pluginId, index);

    char targetPath[std::strlen(pData->oscData->path)+21];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_current_program");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), index);
}

void CarlaEngine::oscSend_control_set_current_midi_program(const uint pluginId, const int32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_current_midi_program(%i, %i)", pluginId, index);

    char targetPath[std::strlen(pData->oscData->path)+26];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_current_midi_program");
    try_lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), index);
}

void CarlaEngine::oscSend_control_set_program_name(const uint pluginId, const uint32_t index, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);

    char targetPath[std::strlen(pData->oscData->path)+18];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_program_name");
    try_lo_send(pData->oscData->target, targetPath, "iis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), name);
}

void CarlaEngine::oscSend_control_set_midi_program_data(const uint pluginId, const uint32_t index, const uint32_t bank, const uint32_t program, const char* const name) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);

    char targetPath[std::strlen(pData->oscData->path)+23];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_midi_program_data");
    try_lo_send(pData->oscData->target, targetPath, "iiiis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(bank), static_cast<int32_t>(program), name);
}

void CarlaEngine::oscSend_control_note_on(const uint pluginId, const uint8_t channel, const uint8_t note, const uint8_t velo) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    CARLA_SAFE_ASSERT_RETURN(velo < MAX_MIDI_VALUE,);
    carla_debug("CarlaEngine::oscSend_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);

    char targetPath[std::strlen(pData->oscData->path)+9];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/note_on");
    try_lo_send(pData->oscData->target, targetPath, "iiii", static_cast<int32_t>(pluginId), static_cast<int32_t>(channel), static_cast<int32_t>(note), static_cast<int32_t>(velo));
}

void CarlaEngine::oscSend_control_note_off(const uint pluginId, const uint8_t channel, const uint8_t note) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    carla_debug("CarlaEngine::oscSend_control_note_off(%i, %i, %i)", pluginId, channel, note);

    char targetPath[std::strlen(pData->oscData->path)+10];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/note_off");
    try_lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(channel), static_cast<int32_t>(note));
}

void CarlaEngine::oscSend_control_set_peaks(const uint pluginId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);

    // TODO - try and see if we can get peaks[4] ref
    const EnginePluginData& epData(pData->plugins[pluginId]);

    char targetPath[std::strlen(pData->oscData->path)+11];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/set_peaks");
    try_lo_send(pData->oscData->target, targetPath, "iffff", static_cast<int32_t>(pluginId), epData.insPeak[0], epData.insPeak[1], epData.outsPeak[0], epData.outsPeak[1]);
}

void CarlaEngine::oscSend_control_exit() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->path != nullptr && pData->oscData->path[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(pData->oscData->target != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_exit()");

    char targetPath[std::strlen(pData->oscData->path)+6];
    std::strcpy(targetPath, pData->oscData->path);
    std::strcat(targetPath, "/exit");
    try_lo_send(pData->oscData->target, targetPath, "");
}
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
