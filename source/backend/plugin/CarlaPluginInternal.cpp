/*
 * Carla Plugin
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaLibCounter.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------
// Fallback data

static const MidiProgramData kMidiProgramDataNull  = { 0, 0, nullptr };
static /* */ CustomData      kCustomDataFallbackNC = { nullptr, nullptr, nullptr };

// -----------------------------------------------------------------------
// PluginAudioData

PluginAudioData::PluginAudioData() noexcept
    : count(0),
      ports(nullptr) {}

PluginAudioData::~PluginAudioData() noexcept
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT(ports == nullptr);
}

void PluginAudioData::createNew(const uint32_t newCount)
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_RETURN(ports == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    ports = new PluginAudioPort[newCount];
    carla_zeroStructs(ports, newCount);

    count = newCount;
}

void PluginAudioData::clear() noexcept
{
    if (ports != nullptr)
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (ports[i].port != nullptr)
            {
                delete ports[i].port;
                ports[i].port = nullptr;
            }
        }

        delete[] ports;
        ports = nullptr;
    }

    count = 0;
}

void PluginAudioData::initBuffers() const noexcept
{
    for (uint32_t i=0; i < count; ++i)
    {
        if (ports[i].port != nullptr)
            ports[i].port->initBuffer();
    }
}

// -----------------------------------------------------------------------
// PluginCVData

PluginCVData::PluginCVData() noexcept
    : count(0),
      ports(nullptr) {}

PluginCVData::~PluginCVData() noexcept
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT(ports == nullptr);
}

void PluginCVData::createNew(const uint32_t newCount)
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_RETURN(ports == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    ports = new PluginCVPort[newCount];
    carla_zeroStructs(ports, newCount);

    count = newCount;
}

void PluginCVData::clear() noexcept
{
    if (ports != nullptr)
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (ports[i].port != nullptr)
            {
                delete ports[i].port;
                ports[i].port = nullptr;
            }
        }

        delete[] ports;
        ports = nullptr;
    }

    count = 0;
}

void PluginCVData::initBuffers() const noexcept
{
    for (uint32_t i=0; i < count; ++i)
    {
        if (ports[i].port != nullptr)
            ports[i].port->initBuffer();
    }
}

// -----------------------------------------------------------------------
// PluginEventData

PluginEventData::PluginEventData() noexcept
    : portIn(nullptr),
      portOut(nullptr)
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    , cvSourcePorts(nullptr)
#endif
    {
    }

PluginEventData::~PluginEventData() noexcept
{
    CARLA_SAFE_ASSERT(portIn == nullptr);
    CARLA_SAFE_ASSERT(portOut == nullptr);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT(cvSourcePorts == nullptr);
#endif
}

void PluginEventData::clear() noexcept
{
    if (portIn != nullptr)
    {
        delete portIn;
        portIn = nullptr;
    }

    if (portOut != nullptr)
    {
        delete portOut;
        portOut = nullptr;
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (cvSourcePorts != nullptr)
    {
        cvSourcePorts->cleanup();
        cvSourcePorts = nullptr;
    }
#endif
}

void PluginEventData::initBuffers() const noexcept
{
    if (portIn != nullptr)
        portIn->initBuffer();

    if (portOut != nullptr)
        portOut->initBuffer();
}

// -----------------------------------------------------------------------
// PluginParameterData

PluginParameterData::PluginParameterData() noexcept
    : count(0),
      data(nullptr),
      ranges(nullptr),
      special(nullptr) {}

PluginParameterData::~PluginParameterData() noexcept
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT(data == nullptr);
    CARLA_SAFE_ASSERT(ranges == nullptr);
    CARLA_SAFE_ASSERT(special == nullptr);
}

void PluginParameterData::createNew(const uint32_t newCount, const bool withSpecial)
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_RETURN(data == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(ranges == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(special == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    data = new ParameterData[newCount];
    carla_zeroStructs(data, newCount);

    for (uint32_t i=0; i < newCount; ++i)
    {
        data[i].index  = PARAMETER_NULL;
        data[i].rindex = PARAMETER_NULL;
        data[i].mappedControlIndex = CONTROL_INDEX_NONE;
        data[i].mappedMinimum = -1.0f;
        data[i].mappedMaximum = 1.0f;
    }

    ranges = new ParameterRanges[newCount];
    carla_zeroStructs(ranges, newCount);

    if (withSpecial)
    {
        special = new SpecialParameterType[newCount];
        carla_zeroStructs(special, newCount);
    }

    count = newCount;
}

void PluginParameterData::clear() noexcept
{
    if (data != nullptr)
    {
        delete[] data;
        data = nullptr;
    }

    if (ranges != nullptr)
    {
        delete[] ranges;
        ranges = nullptr;
    }

    if (special != nullptr)
    {
        delete[] special;
        special = nullptr;
    }

    count = 0;
}

float PluginParameterData::getFixedValue(const uint32_t parameterId, float value) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < count, 0.0f);

    const uint             paramHints (data[parameterId].hints);
    const ParameterRanges& paramRanges(ranges[parameterId]);

    // if boolean, return either min or max
    if (paramHints & PARAMETER_IS_BOOLEAN)
    {
        const float middlePoint = paramRanges.min + (paramRanges.max-paramRanges.min)/2.0f;
        return value >= middlePoint ? paramRanges.max : paramRanges.min;
    }

    // if integer, round first
    if (paramHints & PARAMETER_IS_INTEGER)
        return paramRanges.getFixedValue(std::round(value));

    // normal mode
    return paramRanges.getFixedValue(value);
}

// copied from ParameterRanges::getUnnormalizedValue
static float _getUnnormalizedValue(const float min, const float max, const float value) noexcept
{
    if (value <= 0.0f)
        return min;
    if (value >= 1.0f)
        return max;

    return value * (max - min) + min;
}

// copied from ParameterRanges::getUnnormalizedLogValue
static float _getUnnormalizedLogValue(const float min, const float max, const float value) noexcept
{
    if (value <= 0.0f)
        return min;
    if (value >= 1.0f)
        return max;

    float rmin = min;

    if (std::abs(min) < std::numeric_limits<float>::epsilon())
        rmin = 0.00001f;

    return rmin * std::pow(max/rmin, value);
}

float PluginParameterData::getFinalUnnormalizedValue(const uint32_t parameterId,
                                                     const float normalizedValue) const noexcept
{
    float min, max, value;

    if (data[parameterId].mappedControlIndex != CONTROL_INDEX_CV
        && (data[parameterId].hints & PARAMETER_MAPPED_RANGES_SET) != 0x0)
    {
        min = data[parameterId].mappedMinimum;
        max = data[parameterId].mappedMaximum;
    }
    else
    {
        min = ranges[parameterId].min;
        max = ranges[parameterId].max;
    }

    if (data[parameterId].hints & PARAMETER_IS_BOOLEAN)
    {
        value = (normalizedValue < 0.5f) ? min : max;
    }
    else
    {
        if (data[parameterId].hints & PARAMETER_IS_LOGARITHMIC)
            value = _getUnnormalizedLogValue(min, max, normalizedValue);
        else
            value = _getUnnormalizedValue(min, max, normalizedValue);

        if (data[parameterId].hints & PARAMETER_IS_INTEGER)
            value = std::rint(value);
    }

    return value;
}

float PluginParameterData::getFinalValueWithMidiDelta(const uint32_t parameterId,
                                                      float value, int8_t delta) const noexcept
{
    if (delta < 0)
        return value;
    if (data[parameterId].mappedControlIndex <= 0 || data[parameterId].mappedControlIndex >= MAX_MIDI_CONTROL)
        return value;

    float min, max;

    if ((data[parameterId].hints & PARAMETER_MAPPED_RANGES_SET) != 0x0)
    {
        min = data[parameterId].mappedMinimum;
        max = data[parameterId].mappedMaximum;
    }
    else
    {
        min = ranges[parameterId].min;
        max = ranges[parameterId].max;
    }

    if (data[parameterId].hints & PARAMETER_IS_BOOLEAN)
    {
        value = delta > 63 ? min : max;
    }
    else
    {
        if (data[parameterId].hints & PARAMETER_IS_INTEGER)
        {
            if (delta > 63)
                value += delta - 128.0f;
            else
                value += delta;
        }
        else
        {
            if (delta > 63)
                delta = static_cast<int8_t>(delta - 128);

            value += (max - min) * (static_cast<float>(delta) / 127.0f);
        }

        if (value < min)
            value = min;
        else if (value > max)
            value = max;
    }

    return value;
}

// -----------------------------------------------------------------------
// PluginProgramData

PluginProgramData::PluginProgramData() noexcept
    : count(0),
      current(-1),
      names(nullptr) {}

PluginProgramData::~PluginProgramData() noexcept
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_INT(current == -1, current);
    CARLA_SAFE_ASSERT(names == nullptr);
}

void PluginProgramData::createNew(const uint32_t newCount)
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_INT(current == -1, current);
    CARLA_SAFE_ASSERT_RETURN(names == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    names = new ProgramName[newCount];
    carla_zeroStructs(names, newCount);

    count   = newCount;
    current = -1;
}

void PluginProgramData::clear() noexcept
{
    if (names != nullptr)
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (names[i] != nullptr)
            {
                delete[] names[i];
                names[i] = nullptr;
            }
        }

        delete[] names;
        names = nullptr;
    }

    count   = 0;
    current = -1;
}

// -----------------------------------------------------------------------
// PluginMidiProgramData

PluginMidiProgramData::PluginMidiProgramData() noexcept
    : count(0),
      current(-1),
      data(nullptr) {}

PluginMidiProgramData::~PluginMidiProgramData() noexcept
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_INT(current == -1, current);
    CARLA_SAFE_ASSERT(data == nullptr);
}

void PluginMidiProgramData::createNew(const uint32_t newCount)
{
    CARLA_SAFE_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_INT(current == -1, current);
    CARLA_SAFE_ASSERT_RETURN(data == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    data = new MidiProgramData[newCount];
    carla_zeroStructs(data, newCount);

    count   = newCount;
    current = -1;
}

void PluginMidiProgramData::clear() noexcept
{
    if (data != nullptr)
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (data[i].name != nullptr)
            {
                delete[] data[i].name;
                data[i].name = nullptr;
            }
        }

        delete[] data;
        data = nullptr;
    }

    count   = 0;
    current = -1;
}

const MidiProgramData& PluginMidiProgramData::getCurrent() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(current >= 0 && current < static_cast<int32_t>(count), kMidiProgramDataNull);
    return data[current];
}

// -----------------------------------------------------------------------
// ProtectedData::ExternalNotes

CarlaPlugin::ProtectedData::ExternalNotes::ExternalNotes() noexcept
    : mutex(),
      dataPool("CarlaPlugin::ProtectedData::ExternalNotes", 32, 152),
      data(dataPool) {}

CarlaPlugin::ProtectedData::ExternalNotes::~ExternalNotes() noexcept
{
    clear();
}

void CarlaPlugin::ProtectedData::ExternalNotes::appendNonRT(const ExternalMidiNote& note) noexcept
{
    mutex.lock();
    data.append_sleepy(note);
    mutex.unlock();
}

void CarlaPlugin::ProtectedData::ExternalNotes::clear() noexcept
{
    mutex.lock();
    data.clear();
    mutex.unlock();
}

// -----------------------------------------------------------------------
// ProtectedData::Latency

CarlaPlugin::ProtectedData::Latency::Latency() noexcept
#ifdef BUILD_BRIDGE
    : frames(0) {}
#else
    : frames(0),
      channels(0),
      buffers(nullptr) {}
#endif

#ifndef BUILD_BRIDGE
CarlaPlugin::ProtectedData::Latency::~Latency() noexcept
{
    clearBuffers();
}

void CarlaPlugin::ProtectedData::Latency::clearBuffers() noexcept
{
    if (buffers != nullptr)
    {
        for (uint32_t i=0; i < channels; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(buffers[i] != nullptr);

            delete[] buffers[i];
            buffers[i] = nullptr;
        }

        delete[] buffers;
        buffers = nullptr;
    }

    channels = 0;
    frames   = 0;
}

void CarlaPlugin::ProtectedData::Latency::recreateBuffers(const uint32_t newChannels, const uint32_t newFrames)
{
    CARLA_SAFE_ASSERT_RETURN(channels != newChannels || frames != newFrames,);

    const bool retrieveOldBuffer = (channels == newChannels && channels > 0 && frames > 0 && newFrames > 0);
    float** const oldBuffers = buffers;
    const uint32_t oldFrames = frames;

    channels = newChannels;
    frames   = newFrames;

    if (channels > 0 && frames > 0)
    {
        buffers = new float*[channels];

        for (uint32_t i=0; i < channels; ++i)
        {
            buffers[i] = new float[frames];

            if (retrieveOldBuffer)
            {
                if (oldFrames > frames)
                {
                    const uint32_t diff = oldFrames - frames;
                    carla_copyFloats(buffers[i], oldBuffers[i] + diff, frames);
                }
                else
                {
                    const uint32_t diff = frames - oldFrames;
                    carla_zeroFloats(buffers[i], diff);
                    carla_copyFloats(buffers[i] + diff, oldBuffers[i], oldFrames);
                }
            }
            else
            {
                carla_zeroFloats(buffers[i], frames);
            }
        }
    }
    else
    {
        buffers = nullptr;
    }

    // delete old buffer
    if (oldBuffers != nullptr)
    {
        for (uint32_t i=0; i < channels; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(oldBuffers[i] != nullptr);

            delete[] oldBuffers[i];
            oldBuffers[i] = nullptr;
        }

        delete[] oldBuffers;
    }
}
#endif

// -----------------------------------------------------------------------
// ProtectedData::PostRtEvents

CarlaPlugin::ProtectedData::PostRtEvents::PostRtEvents() noexcept
    : dataPool("CarlaPlugin::ProtectedData::PostRtEvents", 512, 512),
      data(dataPool),
      dataPendingRT(dataPool),
      dataMutex(),
      dataPendingMutex(),
      poolMutex() {}

CarlaPlugin::ProtectedData::PostRtEvents::~PostRtEvents() noexcept
{
    const CarlaMutexLocker cml1(dataMutex);
    const CarlaMutexLocker cml2(dataPendingMutex);
    const CarlaMutexLocker cml3(poolMutex);

    data.clear();
    dataPendingRT.clear();
}

void CarlaPlugin::ProtectedData::PostRtEvents::appendRT(const PluginPostRtEvent& e) noexcept
{
    CARLA_SAFE_ASSERT_INT_RETURN(dataPendingMutex.tryLock(), e.type,);
    {
        const CarlaMutexLocker cml(poolMutex);
        dataPendingRT.append(e);
    }
    dataPendingMutex.unlock();
}

void CarlaPlugin::ProtectedData::PostRtEvents::trySplice() noexcept
{
    const CarlaMutexTryLocker cmtl(dataPendingMutex);

    if (cmtl.wasLocked() && dataPendingRT.isNotEmpty() && dataMutex.tryLock())
    {
        {
            const CarlaMutexLocker cml(poolMutex);
            dataPendingRT.moveTo(data, true);
        }
        dataMutex.unlock();
    }
}

// -----------------------------------------------------------------------
// ProtectedData::PostUiEvents

CarlaPlugin::ProtectedData::PostUiEvents::PostUiEvents() noexcept
    : mutex(),
      data() {}

CarlaPlugin::ProtectedData::PostUiEvents::~PostUiEvents() noexcept
{
    clear();
}

void CarlaPlugin::ProtectedData::PostUiEvents::append(const PluginPostRtEvent& e) noexcept
{
    mutex.lock();
    data.append(e);
    mutex.unlock();
}

void CarlaPlugin::ProtectedData::PostUiEvents::clear() noexcept
{
    mutex.lock();
    data.clear();
    mutex.unlock();
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
// -----------------------------------------------------------------------
// ProtectedData::PostProc

CarlaPlugin::ProtectedData::PostProc::PostProc() noexcept
    : dryWet(1.0f),
      volume(1.0f),
      balanceLeft(-1.0f),
      balanceRight(1.0f),
      panning(0.0f),
      extraBuffer(nullptr) {}
#endif

// -----------------------------------------------------------------------

CarlaPlugin::ProtectedData::ProtectedData(CarlaEngine* const eng, const uint idx) noexcept
    : engine(eng),
      client(nullptr),
      id(idx),
      hints(0x0),
      options(0x0),
      nodeId(0),
      active(false),
      enabled(false),
      needsReset(false),
      engineBridged(eng->getType() == kEngineTypeBridge),
      enginePlugin(eng->getType() == kEngineTypePlugin),
      lib(nullptr),
      uiLib(nullptr),
      ctrlChannel(0),
      extraHints(0x0),
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
      midiLearnParameterIndex(-1),
      transientTryCounter(0),
      transientFirstTry(true),
#endif
      name(nullptr),
      filename(nullptr),
      iconName(nullptr),
      audioIn(),
      audioOut(),
      cvIn(),
      cvOut(),
      event(),
      param(),
      prog(),
      midiprog(),
      custom(),
      masterMutex(),
      singleMutex(),
      stateSave(),
      uiTitle(),
      extNotes(),
      latency(),
      postRtEvents(),
      postUiEvents()
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    , postProc()
#endif
      {}

CarlaPlugin::ProtectedData::~ProtectedData() noexcept
{
    CARLA_SAFE_ASSERT(! (active && needsReset));
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT(transientTryCounter == 0);
#endif

    {
        // mutex MUST have been locked before
        const bool lockMaster(masterMutex.tryLock());
        const bool lockSingle(singleMutex.tryLock());
        CARLA_SAFE_ASSERT(! lockMaster);
        CARLA_SAFE_ASSERT(! lockSingle);
    }

    if (client != nullptr)
    {
        if (client->isActive())
        {
            // must not happen
            carla_safe_assert("client->isActive()", __FILE__, __LINE__);
            client->deactivate(true);
        }

        clearBuffers();

        delete client;
        client = nullptr;
    }

    if (name != nullptr)
    {
        delete[] name;
        name = nullptr;
    }

    if (filename != nullptr)
    {
        delete[] filename;
        filename = nullptr;
    }

    if (iconName != nullptr)
    {
        delete[] iconName;
        iconName = nullptr;
    }

    for (LinkedList<CustomData>::Itenerator it = custom.begin2(); it.valid(); it.next())
    {
        CustomData& customData(it.getValue(kCustomDataFallbackNC));
        //CARLA_SAFE_ASSERT_CONTINUE(customData.isValid());

        if (customData.type != nullptr)
        {
            delete[] customData.type;
            customData.type = nullptr;
        }
        else
            carla_safe_assert("customData.type != nullptr", __FILE__, __LINE__);

        if (customData.key != nullptr)
        {
            delete[] customData.key;
            customData.key = nullptr;
        }
        else
            carla_safe_assert("customData.key != nullptr", __FILE__, __LINE__);

        if (customData.value != nullptr)
        {
            delete[] customData.value;
            customData.value = nullptr;
        }
        else
            carla_safe_assert("customData.value != nullptr", __FILE__, __LINE__);
    }

    prog.clear();
    midiprog.clear();
    custom.clear();

    // MUST have been locked before
    masterMutex.unlock();
    singleMutex.unlock();

    CARLA_SAFE_ASSERT(uiLib == nullptr);

    if (lib != nullptr)
        libClose();
}

// -----------------------------------------------------------------------
// Buffer functions

void CarlaPlugin::ProtectedData::clearBuffers() noexcept
{
    audioIn.clear();
    audioOut.clear();
    cvIn.clear();
    cvOut.clear();
    param.clear();
    event.clear();
#ifndef BUILD_BRIDGE
    latency.clearBuffers();
#endif
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (postProc.extraBuffer != nullptr)
    {
        delete[] postProc.extraBuffer;
        postProc.extraBuffer = nullptr;
    }
#endif
}

// -----------------------------------------------------------------------
// Post-poned events

void CarlaPlugin::ProtectedData::postponeRtEvent(const PluginPostRtEvent& rtEvent) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(rtEvent.type != kPluginPostRtEventNull,);

    postRtEvents.appendRT(rtEvent);
}

void CarlaPlugin::ProtectedData::postponeParameterChangeRtEvent(const bool sendCallbackLater,
                                                                const int32_t index,
                                                                const float value) noexcept
{
    PluginPostRtEvent rtEvent = { kPluginPostRtEventParameterChange, sendCallbackLater, {} };
    rtEvent.parameter.index = index;
    rtEvent.parameter.value = value;

    postRtEvents.appendRT(rtEvent);
}

void CarlaPlugin::ProtectedData::postponeProgramChangeRtEvent(const bool sendCallbackLater,
                                                              const uint32_t index) noexcept
{
    PluginPostRtEvent rtEvent = { kPluginPostRtEventProgramChange, sendCallbackLater, {} };
    rtEvent.program.index = index;

    postRtEvents.appendRT(rtEvent);
}

void CarlaPlugin::ProtectedData::postponeMidiProgramChangeRtEvent(const bool sendCallbackLater,
                                                                  const uint32_t index) noexcept
{
    PluginPostRtEvent rtEvent = { kPluginPostRtEventMidiProgramChange, sendCallbackLater, {} };
    rtEvent.program.index = index;

    postRtEvents.appendRT(rtEvent);
}

void CarlaPlugin::ProtectedData::postponeNoteOnRtEvent(const bool sendCallbackLater,
                                                       const uint8_t channel,
                                                       const uint8_t note,
                                                       const uint8_t velocity) noexcept
{
    PluginPostRtEvent rtEvent = { kPluginPostRtEventNoteOn, sendCallbackLater, {} };
    rtEvent.note.channel = channel;
    rtEvent.note.note = note;
    rtEvent.note.velocity = velocity;

    postRtEvents.appendRT(rtEvent);
}
void CarlaPlugin::ProtectedData::postponeNoteOffRtEvent(const bool sendCallbackLater,
                                                        const uint8_t channel,
                                                        const uint8_t note) noexcept
{
    PluginPostRtEvent rtEvent = { kPluginPostRtEventNoteOff, sendCallbackLater, {} };
    rtEvent.note.channel = channel;
    rtEvent.note.note = note;

    postRtEvents.appendRT(rtEvent);
}
void CarlaPlugin::ProtectedData::postponeMidiLearnRtEvent(const bool sendCallbackLater,
                                                          const uint32_t parameter,
                                                          const uint8_t cc,
                                                          const uint8_t channel) noexcept
{
    PluginPostRtEvent rtEvent = { kPluginPostRtEventMidiLearn, sendCallbackLater, {} };
    rtEvent.midiLearn.parameter = parameter;
    rtEvent.midiLearn.cc = cc;
    rtEvent.midiLearn.channel = channel;

    postRtEvents.appendRT(rtEvent);
}

// -----------------------------------------------------------------------
// Library functions

static LibCounter sLibCounter;

const char* CarlaPlugin::ProtectedData::libError(const char* const fname) noexcept
{
    return lib_error(fname);
}

bool CarlaPlugin::ProtectedData::libOpen(const char* const fname) noexcept
{
    lib = sLibCounter.open(fname);
    return (lib != nullptr);
}

bool CarlaPlugin::ProtectedData::libClose() noexcept
{
    const bool ret = sLibCounter.close(lib);
    lib = nullptr;
    return ret;
}

void CarlaPlugin::ProtectedData::setCanDeleteLib(const bool canDelete) noexcept
{
    sLibCounter.setCanDelete(lib, canDelete);
}

bool CarlaPlugin::ProtectedData::uiLibOpen(const char* const fname, const bool canDelete) noexcept
{
    uiLib = sLibCounter.open(fname, canDelete);
    return (uiLib != nullptr);
}

bool CarlaPlugin::ProtectedData::uiLibClose() noexcept
{
    const bool ret = sLibCounter.close(uiLib);
    uiLib = nullptr;
    return ret;
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
void CarlaPlugin::ProtectedData::tryTransient() noexcept
{
    if (engine->getOptions().frontendWinId != 0)
        transientTryCounter = 1;
}
#endif

void CarlaPlugin::ProtectedData::updateParameterValues(CarlaPlugin* const plugin,
                                                       const bool sendCallback,
                                                       const bool sendOsc,
                                                       const bool useDefault) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback || useDefault,);

    for (uint32_t i=0; i < param.count; ++i)
    {
        const float value(param.ranges[i].getFixedValue(plugin->getParameterValue(i)));

        if (useDefault)
            param.ranges[i].def = value;

        if (useDefault) {
            engine->callback(sendCallback, sendOsc,
                             ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED,
                             id,
                             static_cast<int>(i),
                             0, 0,
                             value,
                             nullptr);
        }

        engine->callback(sendCallback, sendOsc,
                         ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                         id,
                         static_cast<int>(i),
                         0, 0,
                         value,
                         nullptr);
    }
}

void CarlaPlugin::ProtectedData::updateDefaultParameterValues(CarlaPlugin* const plugin) noexcept
{
    for (uint32_t i=0; i < param.count; ++i)
        param.ranges[i].def = param.ranges[i].getFixedValue(plugin->getParameterValue(i));
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
