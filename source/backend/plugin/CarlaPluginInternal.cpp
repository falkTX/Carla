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

#include <QtCore/QSettings>

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------
// Fallback data

static const MidiProgramData kMidiProgramDataNull = { 0, 0, nullptr };

// -----------------------------------------------------------------------
// PluginAudioPort

PluginAudioPort::PluginAudioPort() noexcept
    : rindex(0),
      port(nullptr) {}

PluginAudioPort::~PluginAudioPort() noexcept
{
    CARLA_ASSERT(port == nullptr);
}

// -----------------------------------------------------------------------
// PluginAudioData

PluginAudioData::PluginAudioData() noexcept
    : count(0),
      ports(nullptr) {}

PluginAudioData::~PluginAudioData() noexcept
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_ASSERT(ports == nullptr);
}

void PluginAudioData::createNew(const uint32_t newCount)
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_RETURN(ports == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    ports = new PluginAudioPort[newCount];
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

void PluginAudioData::initBuffers() noexcept
{
    for (uint32_t i=0; i < count; ++i)
    {
        if (ports[i].port != nullptr)
            ports[i].port->initBuffer();
    }
}

// -----------------------------------------------------------------------
// PluginCVPort

PluginCVPort::PluginCVPort() noexcept
    : rindex(0),
      param(0),
      port(nullptr) {}

PluginCVPort::~PluginCVPort() noexcept
{
    CARLA_ASSERT(port == nullptr);
}

// -----------------------------------------------------------------------
// PluginCVData

PluginCVData::PluginCVData() noexcept
    : count(0),
      ports(nullptr) {}

PluginCVData::~PluginCVData() noexcept
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_ASSERT(ports == nullptr);
}

void PluginCVData::createNew(const uint32_t newCount)
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_RETURN(ports == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    ports = new PluginCVPort[newCount];
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

void PluginCVData::initBuffers() noexcept
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
    CARLA_ASSERT(portIn == nullptr);
    CARLA_ASSERT(portOut == nullptr);
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

void PluginEventData::initBuffers() noexcept
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
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_ASSERT(data == nullptr);
    CARLA_ASSERT(ranges == nullptr);
    CARLA_ASSERT(special == nullptr);
}

void PluginParameterData::createNew(const uint32_t newCount, const bool withSpecial, const bool doReset)
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_SAFE_ASSERT_RETURN(data == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(ranges == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(special == nullptr,);
    CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

    data   = new ParameterData[newCount];
    ranges = new ParameterRanges[newCount];
    count  = newCount;

    if (withSpecial)
        special = new SpecialParameterType[newCount];

    if (! doReset)
        return;

    for (uint32_t i=0; i < newCount; ++i)
    {
        data[i].type   = PARAMETER_UNKNOWN;
        data[i].hints  = 0x0;
        data[i].index  = PARAMETER_NULL;
        data[i].rindex = PARAMETER_NULL;
        data[i].midiCC = -1;
        data[i].midiChannel = 0;
        ranges[i].def = 0.0f;
        ranges[i].min = 0.0f;
        ranges[i].max = 0.0f;
        ranges[i].step = 0.0f;
        ranges[i].stepSmall = 0.0f;
        ranges[i].stepLarge = 0.0f;

        if (withSpecial)
            special[i] = PARAMETER_SPECIAL_NULL;
    }
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
    return ranges[parameterId].getFixedValue(value);
}

// -----------------------------------------------------------------------
// PluginProgramData

PluginProgramData::PluginProgramData() noexcept
    : count(0),
      current(-1),
      names(nullptr) {}

PluginProgramData::~PluginProgramData() noexcept
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_ASSERT_INT(current == -1, current);
    CARLA_ASSERT(names == nullptr);
}

void PluginProgramData::createNew(const uint32_t newCount)
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_ASSERT_INT(current == -1, current);
    CARLA_ASSERT(names == nullptr);
    CARLA_ASSERT_INT(newCount > 0, newCount);

    if (names != nullptr || newCount == 0)
        return;

    names = new ProgramName[newCount];
    count = newCount;

    for (uint32_t i=0; i < newCount; ++i)
        names[i] = nullptr;
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

    count = 0;
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
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_ASSERT_INT(current == -1, current);
    CARLA_ASSERT(data == nullptr);
}

void PluginMidiProgramData::createNew(const uint32_t newCount)
{
    CARLA_ASSERT_INT(count == 0, count);
    CARLA_ASSERT_INT(current == -1, current);
    CARLA_ASSERT(data == nullptr);
    CARLA_ASSERT_INT(newCount > 0, newCount);

    if (data != nullptr || newCount == 0)
        return;

    data  = new MidiProgramData[newCount];
    count = newCount;

    for (uint32_t i=0; i < count; ++i)
    {
        data[i].bank    = 0;
        data[i].program = 0;
        data[i].name    = nullptr;
    }
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

    count = 0;
    current = -1;
}

const MidiProgramData& PluginMidiProgramData::getCurrent() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(current >= 0 && current < static_cast<int32_t>(count), kMidiProgramDataNull);
    return data[current];
}

// -----------------------------------------------------------------------

CarlaPluginProtectedData::ExternalNotes::ExternalNotes()
    : dataPool(32, 152),
      data(dataPool) {}

CarlaPluginProtectedData::ExternalNotes::~ExternalNotes()
{
    mutex.lock();
    data.clear();
    mutex.unlock();
}

void CarlaPluginProtectedData::ExternalNotes::append(const ExternalMidiNote& note)
{
    mutex.lock();
    data.append_sleepy(note);
    mutex.unlock();
}

// -----------------------------------------------------------------------

CarlaPluginProtectedData::PostRtEvents::PostRtEvents()
    : dataPool(128, 128),
      data(dataPool),
      dataPendingRT(dataPool) {}

CarlaPluginProtectedData::PostRtEvents::~PostRtEvents()
{
    clear();
}

void CarlaPluginProtectedData::PostRtEvents::appendRT(const PluginPostRtEvent& e)
{
    dataPendingRT.append(e);
}

void CarlaPluginProtectedData::PostRtEvents::trySplice()
{
    if (mutex.tryLock())
    {
        dataPendingRT.spliceAppend(data);
        mutex.unlock();
    }
}

void CarlaPluginProtectedData::PostRtEvents::clear()
{
    mutex.lock();
    data.clear();
    dataPendingRT.clear();
    mutex.unlock();
}

// -----------------------------------------------------------------------

#ifndef BUILD_BRIDGE
CarlaPluginProtectedData::PostProc::PostProc() noexcept
    : dryWet(1.0f),
      volume(1.0f),
      balanceLeft(-1.0f),
      balanceRight(1.0f),
      panning(0.0f) {}
#endif

// -----------------------------------------------------------------------

CarlaPluginProtectedData::OSC::OSC(CarlaEngine* const eng, CarlaPlugin* const plug)
    : thread(eng, plug) {}

// -----------------------------------------------------------------------

CarlaPluginProtectedData::CarlaPluginProtectedData(CarlaEngine* const eng, const unsigned int idx, CarlaPlugin* const self)
    : engine(eng),
      client(nullptr),
      id(idx),
      hints(0x0),
      options(0x0),
      active(false),
      enabled(false),
      needsReset(false),
      lib(nullptr),
      uiLib(nullptr),
      ctrlChannel(0),
      extraHints(0x0),
      transientTryCounter(0),
      latency(0),
      latencyBuffers(nullptr),
      name(nullptr),
      filename(nullptr),
      iconName(nullptr),
      identifier(nullptr),
      osc(eng, self) {}

CarlaPluginProtectedData::~CarlaPluginProtectedData()
{
    CARLA_SAFE_ASSERT(! needsReset);
    CARLA_SAFE_ASSERT(transientTryCounter == 0);

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

    if (identifier != nullptr)
    {
        delete[] identifier;
        identifier = nullptr;
    }

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

    for (LinkedList<CustomData>::Itenerator it = custom.begin(); it.valid(); it.next())
    {
        CustomData& cData(it.getValue());

        if (cData.type != nullptr)
        {
            delete[] cData.type;
            cData.type = nullptr;
        }
        else
            carla_safe_assert("cData.type != nullptr", __FILE__, __LINE__);

        if (cData.key != nullptr)
        {
            delete[] cData.key;
            cData.key = nullptr;
        }
        else
            carla_safe_assert("cData.key != nullptr", __FILE__, __LINE__);

        if (cData.value != nullptr)
        {
            delete[] cData.value;
            cData.value = nullptr;
        }
        else
            carla_safe_assert("cData.value != nullptr", __FILE__, __LINE__);
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

void CarlaPluginProtectedData::clearBuffers()
{
    if (latencyBuffers != nullptr)
    {
        CARLA_SAFE_ASSERT(audioIn.count > 0);

        for (uint32_t i=0; i < audioIn.count; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(latencyBuffers[i] != nullptr);

            delete[] latencyBuffers[i];
            latencyBuffers[i] = nullptr;
        }

        delete[] latencyBuffers;
        latencyBuffers = nullptr;
        latency = 0;
    }
    else
    {
        CARLA_SAFE_ASSERT(latency == 0);
    }

    audioIn.clear();
    audioOut.clear();
    param.clear();
    event.clear();
}

void CarlaPluginProtectedData::recreateLatencyBuffers()
{
    if (latencyBuffers != nullptr)
    {
        CARLA_ASSERT(audioIn.count > 0);

        for (uint32_t i=0; i < audioIn.count; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(latencyBuffers[i] != nullptr);

            delete[] latencyBuffers[i];
            latencyBuffers[i] = nullptr;
        }

        delete[] latencyBuffers;
        latencyBuffers = nullptr;
    }

    if (audioIn.count > 0 && latency > 0)
    {
        latencyBuffers = new float*[audioIn.count];

        for (uint32_t i=0; i < audioIn.count; ++i)
        {
            latencyBuffers[i] = new float[latency];
            FLOAT_CLEAR(latencyBuffers[i], latency);
        }
    }
}

// -----------------------------------------------------------------------
// Post-poned events

void CarlaPluginProtectedData::postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3)
{
    CARLA_SAFE_ASSERT_RETURN(type != kPluginPostRtEventNull,);

    PluginPostRtEvent rtEvent = { type, value1, value2, value3 };

    postRtEvents.appendRT(rtEvent);
}

// -----------------------------------------------------------------------
// Library functions

static LibCounter sLibCounter;

const char* CarlaPluginProtectedData::libError(const char* const fname)
{
    return lib_error(fname);
}

bool CarlaPluginProtectedData::libOpen(const char* const fname)
{
    lib = sLibCounter.open(fname);
    return (lib != nullptr);
}

bool CarlaPluginProtectedData::libClose()
{
    const bool ret = sLibCounter.close(lib);
    lib = nullptr;
    return ret;
}

void* CarlaPluginProtectedData::libSymbol(const char* const symbol)
{
    return lib_symbol(lib, symbol);
}

bool CarlaPluginProtectedData::uiLibOpen(const char* const fname)
{
    uiLib = sLibCounter.open(fname);
    return (uiLib != nullptr);
}

bool CarlaPluginProtectedData::uiLibClose()
{
    const bool ret = sLibCounter.close(uiLib);
    uiLib = nullptr;
    return ret;
}

void* CarlaPluginProtectedData::uiLibSymbol(const char* const symbol)
{
    return lib_symbol(uiLib, symbol);
}

// -----------------------------------------------------------------------
// Settings functions

void CarlaPluginProtectedData::saveSetting(const uint option, const bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(identifier != nullptr && identifier[0] != '\0',);

    QSettings settings("falkTX", "CarlaPluginSettings");
    settings.beginGroup(identifier);

    switch (option)
    {
    case PLUGIN_OPTION_FIXED_BUFFERS:
        settings.setValue("FixedBuffers", yesNo);
        break;
    case PLUGIN_OPTION_FORCE_STEREO:
        settings.setValue("ForceStereo", yesNo);
        break;
    case PLUGIN_OPTION_MAP_PROGRAM_CHANGES:
        settings.setValue("MapProgramChanges", yesNo);
        break;
    case PLUGIN_OPTION_USE_CHUNKS:
        settings.setValue("UseChunks", yesNo);
        break;
    case PLUGIN_OPTION_SEND_CONTROL_CHANGES:
        settings.setValue("SendControlChanges", yesNo);
        break;
    case PLUGIN_OPTION_SEND_CHANNEL_PRESSURE:
        settings.setValue("SendChannelPressure", yesNo);
        break;
    case PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH:
        settings.setValue("SendNoteAftertouch", yesNo);
        break;
    case PLUGIN_OPTION_SEND_PITCHBEND:
        settings.setValue("SendPitchbend", yesNo);
        break;
    case PLUGIN_OPTION_SEND_ALL_SOUND_OFF:
        settings.setValue("SendAllSoundOff", yesNo);
        break;
    default:
        break;
    }

    settings.endGroup();
}

uint CarlaPluginProtectedData::loadSettings(const uint curOptions, const uint availOptions)
{
    CARLA_SAFE_ASSERT_RETURN(identifier != nullptr && identifier[0] != '\0', 0x0);

    QSettings settings("falkTX", "CarlaPluginSettings");
    settings.beginGroup(identifier);

    unsigned int newOptions = 0x0;

    #define CHECK_AND_SET_OPTION(STR, BIT)                              \
    if ((availOptions & BIT) != 0 || BIT == PLUGIN_OPTION_FORCE_STEREO) \
    {                                                                   \
        if (settings.contains(STR))                                     \
        {                                                               \
            if (settings.value(STR, (curOptions & BIT) != 0).toBool())  \
                newOptions |= BIT;                                      \
        }                                                               \
        else if (curOptions & BIT)                                      \
            newOptions |= BIT;                                          \
    }

    CHECK_AND_SET_OPTION("FixedBuffers", PLUGIN_OPTION_FIXED_BUFFERS);
    CHECK_AND_SET_OPTION("ForceStereo", PLUGIN_OPTION_FORCE_STEREO);
    CHECK_AND_SET_OPTION("MapProgramChanges", PLUGIN_OPTION_MAP_PROGRAM_CHANGES);
    CHECK_AND_SET_OPTION("UseChunks", PLUGIN_OPTION_USE_CHUNKS);
    CHECK_AND_SET_OPTION("SendControlChanges", PLUGIN_OPTION_SEND_CONTROL_CHANGES);
    CHECK_AND_SET_OPTION("SendChannelPressure", PLUGIN_OPTION_SEND_CHANNEL_PRESSURE);
    CHECK_AND_SET_OPTION("SendNoteAftertouch", PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH);
    CHECK_AND_SET_OPTION("SendPitchbend", PLUGIN_OPTION_SEND_PITCHBEND);
    CHECK_AND_SET_OPTION("SendAllSoundOff", PLUGIN_OPTION_SEND_ALL_SOUND_OFF);

    #undef CHECK_AND_SET_OPTION

    settings.endGroup();

    return newOptions;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
