/*
 * Carla Plugin
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaLibCounter.hpp"
#include "CarlaMathUtils.hpp"

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
      portOut(nullptr) {}

PluginEventData::~PluginEventData() noexcept
{
    CARLA_SAFE_ASSERT(portIn == nullptr);
    CARLA_SAFE_ASSERT(portOut == nullptr);
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
        data[i].midiCC = -1;
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

float PluginParameterData::getFixedValue(const uint32_t parameterId, const float& value) const noexcept
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
      dataPool(32, 152),
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
    : channels(0),
      frames(0),
      buffers(nullptr) {}

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

    // delete old buffer
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

    channels = newChannels;
    frames   = newFrames;

    if (channels > 0 && frames > 0)
    {
        buffers = new float*[channels];

        for (uint32_t i=0; i < channels; ++i)
        {
            buffers[i] = new float[frames];
            FloatVectorOperations::clear(buffers[i], static_cast<int>(frames));
        }
    }
}

// -----------------------------------------------------------------------
// ProtectedData::PostRtEvents

CarlaPlugin::ProtectedData::PostRtEvents::PostRtEvents() noexcept
    : mutex(),
      dataPool(128, 128),
      data(dataPool),
      dataPendingRT(dataPool) {}

CarlaPlugin::ProtectedData::PostRtEvents::~PostRtEvents() noexcept
{
    clear();
}

void CarlaPlugin::ProtectedData::PostRtEvents::appendRT(const PluginPostRtEvent& e) noexcept
{
    dataPendingRT.append(e);
}

void CarlaPlugin::ProtectedData::PostRtEvents::trySplice() noexcept
{
    if (mutex.tryLock())
    {
        if (dataPendingRT.count() > 0)
            dataPendingRT.moveTo(data, true);
        mutex.unlock();
    }
}

void CarlaPlugin::ProtectedData::PostRtEvents::clear() noexcept
{
    mutex.lock();
    data.clear();
    dataPendingRT.clear();
    mutex.unlock();
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

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// ProtectedData::PostProc

CarlaPlugin::ProtectedData::PostProc::PostProc() noexcept
    : dryWet(1.0f),
      volume(1.0f),
      balanceLeft(-1.0f),
      balanceRight(1.0f),
      panning(0.0f) {}
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
      lib(nullptr),
      uiLib(nullptr),
      ctrlChannel(0),
      extraHints(0x0),
      transientTryCounter(0),
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
      extNotes(),
      latency(),
      postRtEvents(),
      postUiEvents()
#ifndef BUILD_BRIDGE
    , postProc()
#endif
      {}

CarlaPlugin::ProtectedData::~ProtectedData() noexcept
{
    CARLA_SAFE_ASSERT(! needsReset);
    CARLA_SAFE_ASSERT(transientTryCounter == 0);

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
            client->deactivate();
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

    if (lib != nullptr)
        libClose();

    CARLA_SAFE_ASSERT(uiLib == nullptr);
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
    latency.clearBuffers();
}

// -----------------------------------------------------------------------
// Post-poned events

void CarlaPlugin::ProtectedData::postponeRtEvent(const PluginPostRtEvent& rtEvent) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(rtEvent.type != kPluginPostRtEventNull,);

    postRtEvents.appendRT(rtEvent);
}

void CarlaPlugin::ProtectedData::postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(type != kPluginPostRtEventNull,);

    PluginPostRtEvent rtEvent = { type, value1, value2, value3 };

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

#ifndef BUILD_BRIDGE
void CarlaPlugin::ProtectedData::tryTransient() noexcept
{
    if (engine->getOptions().frontendWinId != 0)
        transientTryCounter = 1;
}
#endif

void CarlaPlugin::ProtectedData::updateParameterValues(CarlaPlugin* const plugin, const bool sendOsc, const bool sendCallback, const bool useDefault) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback || useDefault,);

    for (uint32_t i=0; i < param.count; ++i)
    {
        const float value(param.ranges[i].getFixedValue(plugin->getParameterValue(i)));

        if (useDefault)
            param.ranges[i].def = value;

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        if (sendOsc)
        {
            if (useDefault)
                engine->oscSend_control_set_default_value(id, i, value);
            engine->oscSend_control_set_parameter_value(id, static_cast<int32_t>(i), value);
        }
#endif

        if (sendCallback)
        {
            if (useDefault)
                engine->callback(ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED, id, static_cast<int>(i), 0, value, nullptr);
            engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, id, static_cast<int>(i), 0, value, nullptr);
        }
    }

    // may be unused
    return; (void)sendOsc;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
