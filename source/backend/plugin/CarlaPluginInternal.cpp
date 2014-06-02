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
    count = newCount;

    for (uint32_t i=0; i < count; ++i)
    {
        ports[i].rindex = 0;
        ports[i].port   = nullptr;
    }
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
    count = newCount;

    for (uint32_t i=0; i < count; ++i)
    {
        ports[i].rindex = 0;
        ports[i].param  = 0;
        ports[i].port   = nullptr;
    }
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

    data   = new ParameterData[newCount];
    ranges = new ParameterRanges[newCount];
    count  = newCount;

    if (withSpecial)
        special = new SpecialParameterType[newCount];

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

    names   = new ProgramName[newCount];
    count   = newCount;
    current = -1;

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

    data    = new MidiProgramData[newCount];
    count   = newCount;
    current = -1;

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
// ProtectedData::ExternalNotes

CarlaPlugin::ProtectedData::ExternalNotes::ExternalNotes() noexcept
    : dataPool(32, 152),
      data(dataPool) {}

CarlaPlugin::ProtectedData::ExternalNotes::~ExternalNotes() noexcept
{
    mutex.lock();
    data.clear();
    mutex.unlock();
}

void CarlaPlugin::ProtectedData::ExternalNotes::appendNonRT(const ExternalMidiNote& note) noexcept
{
    mutex.lock();
    data.append_sleepy(note);
    mutex.unlock();
}

// -----------------------------------------------------------------------
// ProtectedData::PostRtEvents

CarlaPlugin::ProtectedData::PostRtEvents::PostRtEvents() noexcept
    : dataPool(128, 128),
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
        dataPendingRT.spliceAppendTo(data);
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

CarlaPlugin::ProtectedData::OSC::OSC(CarlaEngine* const eng, CarlaPlugin* const plug) noexcept
    : thread(eng, plug) {}

// -----------------------------------------------------------------------

CarlaPlugin::ProtectedData::ProtectedData(CarlaEngine* const eng, const uint idx, CarlaPlugin* const plug) noexcept
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
#ifndef BUILD_BRIDGE
      iconName(nullptr),
      identifier(nullptr),
#endif
      osc(eng, plug) {}

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

#ifndef BUILD_BRIDGE
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
#endif

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

void CarlaPlugin::ProtectedData::clearBuffers() noexcept
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
        if (latency != 0)
        {
#ifndef BUILD_BRIDGE
            carla_safe_assert_int("latency != 0", __FILE__, __LINE__, static_cast<int>(latency));
#endif
            latency = 0;
        }
    }

    audioIn.clear();
    audioOut.clear();
    param.clear();
    event.clear();
}

#ifndef BUILD_BRIDGE
void CarlaPlugin::ProtectedData::recreateLatencyBuffers()
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
#endif

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

void* CarlaPlugin::ProtectedData::libSymbol(const char* const symbol) const noexcept
{
    return lib_symbol(lib, symbol);
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

void* CarlaPlugin::ProtectedData::uiLibSymbol(const char* const symbol) const noexcept
{
    return lib_symbol(uiLib, symbol);
}

// -----------------------------------------------------------------------
// Settings functions

void CarlaPlugin::ProtectedData::saveSetting(const uint option, const bool yesNo) const
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

uint CarlaPlugin::ProtectedData::loadSettings(const uint curOptions, const uint availOptions) const
{
    CARLA_SAFE_ASSERT_RETURN(identifier != nullptr && identifier[0] != '\0', 0x0);

    QSettings settings("falkTX", "CarlaPluginSettings");
    settings.beginGroup(identifier);

    uint newOptions = 0x0;

    #define CHECK_AND_SET_OPTION(STR, BIT)                                    \
    if ((availOptions & BIT) != 0 || BIT == PLUGIN_OPTION_FORCE_STEREO)       \
    {                                                                         \
        if (settings.contains(STR))                                           \
        {                                                                     \
            if (settings.value(STR, bool((curOptions & BIT) != 0)).toBool())  \
                newOptions |= BIT;                                            \
        }                                                                     \
        else if (curOptions & BIT)                                            \
            newOptions |= BIT;                                                \
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

void CarlaPlugin::ProtectedData::tryTransient() noexcept
{
    if (engine->getOptions().frontendWinId != 0)
        transientTryCounter = 1;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
