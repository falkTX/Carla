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

#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackEngineEvent = { kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, 0.0f }} };

// -----------------------------------------------------------------------
// Carla Engine port (Abstract)

CarlaEnginePort::CarlaEnginePort(const CarlaEngineClient& client, const bool isInputPort) noexcept
    : kClient(client),
      kIsInput(isInputPort)
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
      kProcessMode(client.getEngine().getProccessMode())
{
    carla_debug("CarlaEngineEventPort::CarlaEngineEventPort(%s)", bool2str(isInputPort));

    if (kProcessMode == ENGINE_PROCESS_MODE_PATCHBAY)
        fBuffer = new EngineEvent[kMaxEngineEventInternalCount];
}

CarlaEngineEventPort::~CarlaEngineEventPort() noexcept
{
    carla_debug("CarlaEngineEventPort::~CarlaEngineEventPort()");

    if (kProcessMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        delete[] fBuffer;
        fBuffer = nullptr;
    }
}

void CarlaEngineEventPort::initBuffer() noexcept
{
    if (kProcessMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == ENGINE_PROCESS_MODE_BRIDGE)
        fBuffer = kClient.getEngine().getInternalEventBuffer(kIsInput);
    else if (kProcessMode == ENGINE_PROCESS_MODE_PATCHBAY && ! kIsInput)
        carla_zeroStruct<EngineEvent>(fBuffer, kMaxEngineEventInternalCount);
}

uint32_t CarlaEngineEventPort::getEventCount() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(kIsInput, 0);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, 0);
    CARLA_SAFE_ASSERT_RETURN(kProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && kProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, 0);

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
    CARLA_SAFE_ASSERT_RETURN(kIsInput, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(kProcessMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == ENGINE_PROCESS_MODE_PATCHBAY, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(index < kMaxEngineEventInternalCount, kFallbackEngineEvent);

    return fBuffer[index];
}

const EngineEvent& CarlaEngineEventPort::getEventUnchecked(const uint32_t index) const noexcept
{
    return fBuffer[index];
}

bool CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEvent& ctrl) noexcept
{
    return writeControlEvent(time, channel, ctrl.type, ctrl.param, ctrl.value);
}

bool CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! kIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(kProcessMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
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

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t size, const uint8_t* const data) noexcept
{
    return writeMidiEvent(time, uint8_t(MIDI_GET_CHANNEL_FROM_DATA(data)), 0, size, data);
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const EngineMidiEvent& midi) noexcept
{
    return writeMidiEvent(time, channel, midi.port, midi.size, midi.data);
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t size, const uint8_t* const data) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! kIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(kProcessMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == ENGINE_PROCESS_MODE_PATCHBAY, false);
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

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
