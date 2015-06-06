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

CarlaEnginePort::CarlaEnginePort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept
    : kClient(client),
      kIsInput(isInputPort),
      kIndexOffset(indexOffset)
{
    carla_debug("CarlaEnginePort::CarlaEnginePort(%s)", bool2str(isInputPort));
}

CarlaEnginePort::~CarlaEnginePort() noexcept
{
    carla_debug("CarlaEnginePort::~CarlaEnginePort()");
}

// -----------------------------------------------------------------------
// Carla Engine Audio port

CarlaEngineAudioPort::CarlaEngineAudioPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept
    : CarlaEnginePort(client, isInputPort, indexOffset),
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

CarlaEngineCVPort::CarlaEngineCVPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept
    : CarlaEnginePort(client, isInputPort, indexOffset),
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

CarlaEngineEventPort::CarlaEngineEventPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept
    : CarlaEnginePort(client, isInputPort, indexOffset),
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
        carla_zeroStructs(fBuffer, kMaxEngineEventInternalCount);
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
    CARLA_SAFE_ASSERT_RETURN(kProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && kProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, kFallbackEngineEvent);
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
    CARLA_SAFE_ASSERT_RETURN(kProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && kProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, false);
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
        event.ctrl.value = carla_fixedValue<float>(0.0f, 1.0f, value);

        return true;
    }

    carla_stderr2("CarlaEngineEventPort::writeControlEvent() - buffer full");
    return false;
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t size, const uint8_t* const data) noexcept
{
    return writeMidiEvent(time, uint8_t(MIDI_GET_CHANNEL_FROM_DATA(data)), size, data);
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const EngineMidiEvent& midi) noexcept
{
    CARLA_SAFE_ASSERT(midi.port == kIndexOffset);
    return writeMidiEvent(time, channel, midi.size, midi.data);
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t size, const uint8_t* const data) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! kIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(kProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && kProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, false);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
    CARLA_SAFE_ASSERT_RETURN(size > 0 && size <= EngineMidiEvent::kDataSize, false);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

    for (uint32_t i=0; i < kMaxEngineEventInternalCount; ++i)
    {
        EngineEvent& event(fBuffer[i]);

        if (event.type != kEngineEventTypeNull)
            continue;

        event.time    = time;
        event.channel = channel;

        const uint8_t status(uint8_t(MIDI_GET_STATUS_FROM_DATA(data)));

        if (status == MIDI_STATUS_CONTROL_CHANGE)
        {
            CARLA_SAFE_ASSERT_RETURN(size >= 3, true);

            switch (data[1])
            {
            case MIDI_CONTROL_BANK_SELECT:
            case MIDI_CONTROL_BANK_SELECT__LSB:
                event.type       = kEngineEventTypeControl;
                event.ctrl.type  = kEngineControlEventTypeMidiBank;
                event.ctrl.param = data[2];
                event.ctrl.value = 0.0f;
                return true;

            case MIDI_CONTROL_ALL_SOUND_OFF:
                event.type       = kEngineEventTypeControl;
                event.ctrl.type  = kEngineControlEventTypeAllSoundOff;
                event.ctrl.param = 0;
                event.ctrl.value = 0.0f;
                return true;

            case MIDI_CONTROL_ALL_NOTES_OFF:
                event.type       = kEngineEventTypeControl;
                event.ctrl.type  = kEngineControlEventTypeAllNotesOff;
                event.ctrl.param = 0;
                event.ctrl.value = 0.0f;
                return true;
            }
        }

        if (status == MIDI_STATUS_PROGRAM_CHANGE)
        {
            CARLA_SAFE_ASSERT_RETURN(size == 2, true);

            event.type       = kEngineEventTypeControl;
            event.ctrl.type  = kEngineControlEventTypeMidiBank;
            event.ctrl.param = data[1];
            event.ctrl.value = 0.0f;
            return true;
        }

        event.type      = kEngineEventTypeMidi;
        event.midi.size = size;

        if (kIndexOffset < 0xFF /* uint8_t max */)
        {
            event.midi.port = kIndexOffset;
        }
        else
        {
            event.midi.port = 0;
            carla_safe_assert_int("kIndexOffset < 0xFF", __FILE__, __LINE__, kIndexOffset);
        }

        event.midi.data[0] = status;

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
