/*
 * Carla Engine utils
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

#ifndef CARLA_ENGINE_UTILS_HPP_INCLUDED
#define CARLA_ENGINE_UTILS_HPP_INCLUDED

#include "CarlaEngine.hpp"
#include "CarlaUtils.hpp"

#include "CarlaMIDI.h"

#include "juce_audio_basics.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Maximum internal pre-allocated events

const ushort kMaxEngineEventInternalCount = 512;

// -----------------------------------------------------------------------

static inline
const char* EngineType2Str(const EngineType type) noexcept
{
    switch (type)
    {
    case kEngineTypeNull:
        return "kEngineTypeNull";
    case kEngineTypeJack:
        return "kEngineTypeJack";
    case kEngineTypeJuce:
        return "kEngineTypeJuce";
    case kEngineTypeRtAudio:
        return "kEngineTypeRtAudio";
    case kEngineTypePlugin:
        return "kEngineTypePlugin";
    case kEngineTypeBridge:
        return "kEngineTypeBridge";
    }

    carla_stderr("CarlaBackend::EngineType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* EnginePortType2Str(const EnginePortType type) noexcept
{
    switch (type)
    {
    case kEnginePortTypeNull:
        return "kEnginePortTypeNull";
    case kEnginePortTypeAudio:
        return "kEnginePortTypeAudio";
    case kEnginePortTypeCV:
        return "kEnginePortTypeCV";
    case kEnginePortTypeEvent:
        return "kEnginePortTypeEvent";
    }

    carla_stderr("CarlaBackend::EnginePortType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* EngineEventType2Str(const EngineEventType type) noexcept
{
    switch (type)
    {
    case kEngineEventTypeNull:
        return "kEngineEventTypeNull";
    case kEngineEventTypeControl:
        return "kEngineEventTypeControl";
    case kEngineEventTypeMidi:
        return "kEngineEventTypeMidi";
    }

    carla_stderr("CarlaBackend::EngineEventType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* EngineControlEventType2Str(const EngineControlEventType type) noexcept
{
    switch (type)
    {
    case kEngineControlEventTypeNull:
        return "kEngineNullEvent";
    case kEngineControlEventTypeParameter:
        return "kEngineControlEventTypeParameter";
    case kEngineControlEventTypeMidiBank:
        return "kEngineControlEventTypeMidiBank";
    case kEngineControlEventTypeMidiProgram:
        return "kEngineControlEventTypeMidiProgram";
    case kEngineControlEventTypeAllSoundOff:
        return "kEngineControlEventTypeAllSoundOff";
    case kEngineControlEventTypeAllNotesOff:
        return "kEngineControlEventTypeAllNotesOff";
    }

    carla_stderr("CarlaBackend::EngineControlEventType2Str(%i) - invalid type", type);
    return nullptr;
}

// -----------------------------------------------------------------------

static inline
void fillEngineEventsFromJuceMidiBuffer(EngineEvent engineEvents[kMaxEngineEventInternalCount], const juce::MidiBuffer& midiBuffer)
{
    const uint8_t* midiData;
    int numBytes, sampleNumber;
    ushort engineEventIndex = 0;

    for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
    {
        const EngineEvent& engineEvent(engineEvents[i]);

        if (engineEvent.type != kEngineEventTypeNull)
            continue;

        engineEventIndex = i;
        break;
    }

    for (juce::MidiBuffer::Iterator midiBufferIterator(midiBuffer); midiBufferIterator.getNextEvent(midiData, numBytes, sampleNumber) && engineEventIndex < kMaxEngineEventInternalCount;)
    {
        CARLA_SAFE_ASSERT_CONTINUE(numBytes > 0);
        CARLA_SAFE_ASSERT_CONTINUE(sampleNumber >= 0);
        CARLA_SAFE_ASSERT_CONTINUE(numBytes < 0xFF /* uint8_t max */);

        EngineEvent& engineEvent(engineEvents[engineEventIndex++]);

        engineEvent.time = static_cast<uint32_t>(sampleNumber);
        engineEvent.fillFromMidiData(static_cast<uint8_t>(numBytes), midiData, 0);
    }
}

// -----------------------------------------------------------------------

static inline
void fillJuceMidiBufferFromEngineEvents(juce::MidiBuffer& midiBuffer, const EngineEvent engineEvents[kMaxEngineEventInternalCount])
{
    uint8_t        size     = 0;
    uint8_t        mdata[3] = { 0, 0, 0 };
    const uint8_t* mdataPtr = mdata;
    uint8_t        mdataTmp[EngineMidiEvent::kDataSize];

    for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
    {
        const EngineEvent& engineEvent(engineEvents[i]);

        if (engineEvent.type == kEngineEventTypeNull)
        {
            break;
        }
        else if (engineEvent.type == kEngineEventTypeControl)
        {
            const EngineControlEvent& ctrlEvent(engineEvent.ctrl);

            ctrlEvent.convertToMidiData(engineEvent.channel, size, mdata);
            mdataPtr = mdata;
        }
        else if (engineEvent.type == kEngineEventTypeMidi)
        {
            const EngineMidiEvent& midiEvent(engineEvent.midi);

            size = midiEvent.size;

            if (size > EngineMidiEvent::kDataSize && midiEvent.dataExt != nullptr)
            {
                mdataPtr = midiEvent.dataExt;
            }
            else
            {
                // copy
                carla_copy<uint8_t>(mdataTmp, midiEvent.data, size);
                // add channel
                mdataTmp[0] |= (engineEvent.channel & MIDI_CHANNEL_BIT);
                // done
                mdataPtr = mdataTmp;
            }
        }
        else
        {
            continue;
        }

        if (size > 0)
            midiBuffer.addEvent(mdataPtr, static_cast<int>(size), static_cast<int>(engineEvent.time));
    }
}

// -------------------------------------------------------------------
// Helper classes

class ScopedEngineEnvironmentLocker
{
public:
      ScopedEngineEnvironmentLocker(CarlaEngine* const engine) noexcept;
      ~ScopedEngineEnvironmentLocker() noexcept;

private:
    CarlaEngine::ProtectedData* const pData;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ScopedEngineEnvironmentLocker)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_UTILS_HPP_INCLUDED
