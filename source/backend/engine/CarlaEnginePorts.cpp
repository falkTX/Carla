/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEnginePorts.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
#include "CarlaEngineGraph.hpp"
#endif

#include "lv2/lv2.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Fallback data

static EngineEvent kFallbackEngineEvent = {
    kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, -1, 0.0f, true }}
};

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

void CarlaEnginePort::setMetaData(const char*, const char*, const char*)
{
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
      fBuffer(nullptr),
      fMinimum(-1.0f),
      fMaximum(1.0f)
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

void CarlaEngineCVPort::setRange(const float min, const float max) noexcept
{
    fMinimum = min;
    fMaximum = max;

    char strBufMin[STR_MAX];
    char strBufMax[STR_MAX];
    carla_zeroChars(strBufMin, STR_MAX);
    carla_zeroChars(strBufMax, STR_MAX);

    {
        const ScopedSafeLocale ssl;
        std::snprintf(strBufMin, STR_MAX-1, "%.12g", static_cast<double>(min));
        std::snprintf(strBufMax, STR_MAX-1, "%.12g", static_cast<double>(max));
    }

    setMetaData(LV2_CORE__minimum, strBufMin, "");
    setMetaData(LV2_CORE__maximum, strBufMax, "");
}

// -----------------------------------------------------------------------
// Carla Engine Event port

CarlaEngineEventPort::CarlaEngineEventPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept
    : CarlaEnginePort(client, isInputPort, indexOffset),
      kProcessMode(client.getEngine().getProccessMode()),
      fBuffer(nullptr)
{
    carla_debug("CarlaEngineEventPort::CarlaEngineEventPort(%s)", bool2str(isInputPort));

    if (kProcessMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        fBuffer = new EngineEvent[kMaxEngineEventInternalCount];
        carla_zeroStructs(fBuffer, kMaxEngineEventInternalCount);
    }
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

EngineEvent& CarlaEngineEventPort::getEvent(const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(kIsInput, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(kProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && kProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(index < kMaxEngineEventInternalCount, kFallbackEngineEvent);

    return fBuffer[index];
}

EngineEvent& CarlaEngineEventPort::getEventUnchecked(const uint32_t index) const noexcept
{
    return fBuffer[index];
}

bool CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEvent& ctrl) noexcept
{
    return writeControlEvent(time, channel, ctrl.type, ctrl.param, ctrl.midiValue, ctrl.normalizedValue);
}

bool CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel,
                                             const EngineControlEventType type,
                                             const uint16_t param,
                                             const int8_t midiValue,
                                             const float normalizedValue) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(! kIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(kProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && kProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, false);
    CARLA_SAFE_ASSERT_RETURN(type != kEngineControlEventTypeNull, false);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
    CARLA_SAFE_ASSERT(normalizedValue >= 0.0f && normalizedValue <= 1.0f);

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

        event.ctrl.type            = type;
        event.ctrl.param           = param;
        event.ctrl.midiValue       = midiValue;
        event.ctrl.normalizedValue = carla_fixedValue<float>(0.0f, 1.0f, normalizedValue);

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
            CARLA_SAFE_ASSERT_RETURN(size >= 2, true);

            switch (data[1])
            {
            case MIDI_CONTROL_BANK_SELECT:
            case MIDI_CONTROL_BANK_SELECT__LSB:
                CARLA_SAFE_ASSERT_RETURN(size >= 3, true);
                event.type                 = kEngineEventTypeControl;
                event.ctrl.type            = kEngineControlEventTypeMidiBank;
                event.ctrl.param           = data[2];
                event.ctrl.midiValue       = -1;
                event.ctrl.normalizedValue = 0.0f;
                event.ctrl.handled         = true;
                return true;

            case MIDI_CONTROL_ALL_SOUND_OFF:
                event.type                 = kEngineEventTypeControl;
                event.ctrl.type            = kEngineControlEventTypeAllSoundOff;
                event.ctrl.param           = 0;
                event.ctrl.midiValue       = -1;
                event.ctrl.normalizedValue = 0.0f;
                event.ctrl.handled         = true;
                return true;

            case MIDI_CONTROL_ALL_NOTES_OFF:
                event.type                 = kEngineEventTypeControl;
                event.ctrl.type            = kEngineControlEventTypeAllNotesOff;
                event.ctrl.param           = 0;
                event.ctrl.midiValue       = -1;
                event.ctrl.normalizedValue = 0.0f;
                event.ctrl.handled         = true;
                return true;
            }
        }

        if (status == MIDI_STATUS_PROGRAM_CHANGE)
        {
            CARLA_SAFE_ASSERT_RETURN(size >= 2, true);

            event.type                 = kEngineEventTypeControl;
            event.ctrl.type            = kEngineControlEventTypeMidiProgram;
            event.ctrl.param           = data[1];
            event.ctrl.midiValue       = -1;
            event.ctrl.normalizedValue = 0.0f;
            event.ctrl.handled         = true;
            return true;
        }

        event.type      = kEngineEventTypeMidi;
        event.midi.size = size;

        if (kIndexOffset < 0xFF /* uint8_t max */)
        {
            event.midi.port = static_cast<uint8_t>(kIndexOffset);
        }
        else
        {
            event.midi.port = 0;
            carla_safe_assert_uint("kIndexOffset < 0xFF", __FILE__, __LINE__, kIndexOffset);
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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
// -----------------------------------------------------------------------
// Carla Engine Meta CV port

CarlaEngineCVSourcePorts::CarlaEngineCVSourcePorts()
    : pData(new ProtectedData())
{
    carla_debug("CarlaEngineCVSourcePorts::CarlaEngineCVSourcePorts()");
}

CarlaEngineCVSourcePorts::~CarlaEngineCVSourcePorts()
{
    carla_debug("CarlaEngineCVSourcePorts::~CarlaEngineCVSourcePorts()");
    delete pData;
}

bool CarlaEngineCVSourcePorts::addCVSource(CarlaEngineCVPort* const port,
                                           const uint32_t portIndexOffset,
                                           const bool reconfigureNow)
{
    CARLA_SAFE_ASSERT_RETURN(port != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(port->isInput(), false);
    carla_debug("CarlaEngineCVSourcePorts::addCVSource(%p, %u)", port, portIndexOffset);

    {
        const CarlaRecursiveMutexLocker crml(pData->rmutex);

        const CarlaEngineEventCV ecv = { port, portIndexOffset, 0.0f };
        if (! pData->cvs.add(ecv))
            return false;

        if (reconfigureNow && pData->graph != nullptr && pData->plugin.get() != nullptr)
            pData->graph->reconfigureForCV(pData->plugin, static_cast<uint>(pData->cvs.size()-1), true);
    }

    return true;
}

bool CarlaEngineCVSourcePorts::removeCVSource(const uint32_t portIndexOffset)
{
    carla_debug("CarlaEngineCVSourcePorts::removeCVSource(%u)", portIndexOffset);

    {
        const CarlaRecursiveMutexLocker crml(pData->rmutex);

        for (int i = pData->cvs.size(); --i >= 0;)
        {
            const CarlaEngineEventCV& ecv(pData->cvs[i]);

            if (ecv.indexOffset == portIndexOffset)
            {
                delete ecv.cvPort;
                pData->cvs.remove(i);

                if (pData->graph != nullptr && pData->plugin.get() != nullptr)
                    pData->graph->reconfigureForCV(pData->plugin, static_cast<uint>(i), false);

                carla_stdout("found cv source to remove %u", portIndexOffset);

                return true;
            }
        }
    }

    carla_stdout("did NOT found cv source to remove %u", portIndexOffset);
    return false;
}

void CarlaEngineCVSourcePorts::initPortBuffers(const float* const* const buffers,
                                               const uint32_t frames,
                                               const bool sampleAccurate,
                                               CarlaEngineEventPort* const eventPort)
{
    CARLA_SAFE_ASSERT_RETURN(buffers != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(eventPort != nullptr,);

    const CarlaRecursiveMutexTryLocker crmtl(pData->rmutex);

    if (! crmtl.wasLocked())
        return;

    const int numCVs = pData->cvs.size();

    if (numCVs == 0)
        return;

    EngineEvent* const buffer = eventPort->fBuffer;
    CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);

    uint32_t eventCount = 0;
    float v, min, max;

    for (; eventCount < kMaxEngineEventInternalCount; ++eventCount)
    {
        if (buffer[eventCount].type == kEngineEventTypeNull)
            break;
    }

    if (eventCount == kMaxEngineEventInternalCount)
        return;

    // TODO be sample accurate

    if (true || ! sampleAccurate)
    {
        const uint32_t eventFrame = eventCount == 0 ? 0 : std::min(buffer[eventCount-1].time, frames-1U);

        for (int i = 0; i < numCVs && eventCount < kMaxEngineEventInternalCount; ++i)
        {
            CarlaEngineEventCV& ecv(pData->cvs.getReference(i));
            CARLA_SAFE_ASSERT_CONTINUE(ecv.cvPort != nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(buffers[i] != nullptr);

            float previousValue = ecv.previousValue;
            ecv.cvPort->getRange(min, max);

            v = buffers[i][eventFrame];

            if (carla_isNotEqual(v, previousValue))
            {
                previousValue = v;

                EngineEvent& event(buffer[eventCount++]);

                event.type    = kEngineEventTypeControl;
                event.time    = eventFrame;
                event.channel = kEngineEventNonMidiChannel;

                event.ctrl.type            = kEngineControlEventTypeParameter;
                event.ctrl.param           = static_cast<uint16_t>(ecv.indexOffset);
                event.ctrl.midiValue       = -1;
                event.ctrl.normalizedValue = carla_fixedValue(0.0f, 1.0f, (v - min) / (max - min));
            }

            ecv.previousValue = previousValue;
        }
    }
}

bool CarlaEngineCVSourcePorts::setCVSourceRange(const uint32_t portIndexOffset, const float minimum, const float maximum)
{
    const CarlaRecursiveMutexLocker crml(pData->rmutex);

    for (int i = pData->cvs.size(); --i >= 0;)
    {
        CarlaEngineEventCV& ecv(pData->cvs.getReference(i));

        if (ecv.indexOffset == portIndexOffset)
        {
            CARLA_SAFE_ASSERT_RETURN(ecv.cvPort != nullptr, false);
            ecv.cvPort->setRange(minimum, maximum);
            return true;
        }
    }

    return false;
}

void CarlaEngineCVSourcePorts::cleanup()
{
    pData->cleanup();
}

/*
void CarlaEngineCVSourcePorts::mixWithCvBuffer(const float* const buffer,
                                            const uint32_t frames,
                                            const uint32_t indexOffset) noexcept
{
    for (LinkedList<CarlaEngineEventCV>::Itenerator it = pData->cvs.begin2(); it.valid(); it.next())
    {
        CarlaEngineEventCV& ecv(it.getValue(kFallbackEngineEventCV));

        if (ecv.indexOffset != indexOffset)
            continue;
        CARLA_SAFE_ASSERT_RETURN(ecv.cvPort != nullptr,);

        float previousValue = ecv.previousValue;
        ecv.cvPort->getRange(min, max);

        for (uint32_t i=0; i<frames; i+=32)
        {
            v = buffer[i];

            if (carla_isNotEqual(v, previousValue))
            {
                previousValue = v;

                EngineEvent& event(pData->buffer[eventIndex++]);

                event.type    = kEngineEventTypeControl;
                event.time    = i;
                event.channel = kEngineEventNonMidiChannel;

                event.ctrl.type  = kEngineControlEventTypeParameter;
                event.ctrl.param = static_cast<uint16_t>(indexOffset);
                event.ctrl.value = carla_fixedValue(0.0f, 1.0f, (v - min) / (max - min));
            }
        }

        ecv.previousValue = previousValue;
        break;
    }
}
*/
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
