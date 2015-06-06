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

#include "CarlaEngine.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

CARLA_BACKEND_START_NAMESPACE

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
            data[2] = uint8_t(carla_fixedValue<float>(0.0f, float(MAX_MIDI_VALUE-1), value));
        }
        else
        {
            size    = 3;
            data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
            data[1] = static_cast<uint8_t>(param);
            data[2] = uint8_t(carla_fixedValue<float>(0.0f, 1.0f, value) * float(MAX_MIDI_VALUE-1));
        }
        break;

    case kEngineControlEventTypeMidiBank:
        size    = 3;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
        data[1] = MIDI_CONTROL_BANK_SELECT;
        data[2] = uint8_t(carla_fixedValue<uint16_t>(0, MAX_MIDI_VALUE-1, param));
        break;

    case kEngineControlEventTypeMidiProgram:
        size    = 2;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_PROGRAM_CHANGE | (channel & MIDI_CHANNEL_BIT));
        data[1] = uint8_t(carla_fixedValue<uint16_t>(0, MAX_MIDI_VALUE-1, param));
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

void EngineEvent::fillFromMidiData(const uint8_t size, const uint8_t* const data, const uint8_t midiPortOffset) noexcept
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
        CARLA_SAFE_ASSERT_RETURN(size >= 2,);

        type = kEngineEventTypeControl;

        const uint8_t midiControl(data[1]);

        if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
        {
            CARLA_SAFE_ASSERT_RETURN(size >= 3,);

            const uint8_t midiBank(data[2]);

            ctrl.type  = kEngineControlEventTypeMidiBank;
            ctrl.param = midiBank;
            ctrl.value = 0.0f;
        }
        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
        {
            ctrl.type  = kEngineControlEventTypeAllSoundOff;
            ctrl.param = 0;
            ctrl.value = 0.0f;
        }
        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
        {
            ctrl.type  = kEngineControlEventTypeAllNotesOff;
            ctrl.param = 0;
            ctrl.value = 0.0f;
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(size >= 3,);

            const uint8_t midiValue(carla_fixedValue<uint8_t>(0, 127, data[2])); // ensures 0.0<->1.0 value range

            ctrl.type  = kEngineControlEventTypeParameter;
            ctrl.param = midiControl;
            ctrl.value = float(midiValue)/127.0f;
        }
    }
    else if (midiStatus == MIDI_STATUS_PROGRAM_CHANGE)
    {
        CARLA_SAFE_ASSERT_RETURN(size >= 2,);

        type = kEngineEventTypeControl;

        const uint8_t midiProgram(data[1]);

        ctrl.type  = kEngineControlEventTypeMidiProgram;
        ctrl.param = midiProgram;
        ctrl.value = 0.0f;
    }
    else
    {
        type = kEngineEventTypeMidi;

        midi.port = midiPortOffset;
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
            for (; i < size; ++i)
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
    : processMode(ENGINE_PROCESS_MODE_PATCHBAY),
      transportMode(ENGINE_TRANSPORT_MODE_INTERNAL),
#endif
      forceStereo(false),
      preferPluginBridges(false),
#ifdef CARLA_OS_WIN
      preferUiBridges(false),
#else
      preferUiBridges(true),
#endif
      uisAlwaysOnTop(true),
      maxParameters(MAX_DEFAULT_PARAMETERS),
      uiBridgesTimeout(4000),
      audioNumPeriods(2),
      audioBufferSize(512),
      audioSampleRate(44100),
      audioDevice(nullptr),
      pathLADSPA(nullptr),
      pathDSSI(nullptr),
      pathLV2(nullptr),
      pathVST2(nullptr),
      pathVST3(nullptr),
      pathGIG(nullptr),
      pathSF2(nullptr),
      pathSFZ(nullptr),
      binaryDir(nullptr),
      resourceDir(nullptr),
      preventBadBehaviour(false),
      frontendWinId(0) {}

EngineOptions::~EngineOptions() noexcept
{
    if (audioDevice != nullptr)
    {
        delete[] audioDevice;
        audioDevice = nullptr;
    }

    if (pathLADSPA != nullptr)
    {
        delete[] pathLADSPA;
        pathLADSPA = nullptr;
    }

    if (pathDSSI != nullptr)
    {
        delete[] pathDSSI;
        pathDSSI = nullptr;
    }

    if (pathLV2 != nullptr)
    {
        delete[] pathLV2;
        pathLV2 = nullptr;
    }

    if (pathVST2 != nullptr)
    {
        delete[] pathVST2;
        pathVST2 = nullptr;
    }

    if (pathVST3 != nullptr)
    {
        delete[] pathVST3;
        pathVST3 = nullptr;
    }

    if (pathGIG != nullptr)
    {
        delete[] pathGIG;
        pathGIG = nullptr;
    }

    if (pathSF2 != nullptr)
    {
        delete[] pathSF2;
        pathSF2 = nullptr;
    }

    if (pathSFZ != nullptr)
    {
        delete[] pathSFZ;
        pathSFZ = nullptr;
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
      valid(0x0),
      bbt() {}

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
    if (carla_isNotEqual(timeInfo.bbt.beatsPerMinute, bbt.beatsPerMinute))
        return false;
    return true;
}

bool EngineTimeInfo::operator!=(const EngineTimeInfo& timeInfo) const noexcept
{
    return !operator==(timeInfo);
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
