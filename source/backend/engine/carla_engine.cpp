/*
 * Carla Engine
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_engine_internal.hpp"
#include "carla_backend_utils.hpp"
#include "carla_midi.h"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine port (Abstract)

CarlaEnginePort::CarlaEnginePort(const bool isInput, const ProcessMode processMode)
    : kIsInput(isInput),
      kProcessMode(processMode)
{
    qDebug("CarlaEnginePort::CarlaEnginePort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));
}

CarlaEnginePort::~CarlaEnginePort()
{
    qDebug("CarlaEnginePort::~CarlaEnginePort()");
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Audio port

CarlaEngineAudioPort::CarlaEngineAudioPort(const bool isInput, const ProcessMode processMode)
    : CarlaEnginePort(isInput, processMode)
{
    qDebug("CarlaEngineAudioPort::CarlaEngineAudioPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_PATCHBAY)
        fBuffer = new float[PATCHBAY_BUFFER_SIZE];
    else
#endif
        fBuffer = nullptr;
}

CarlaEngineAudioPort::~CarlaEngineAudioPort()
{
    qDebug("CarlaEngineAudioPort::~CarlaEngineAudioPort()");

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        CARLA_ASSERT(fBuffer);

        if (fBuffer)
            delete[] (float*)fBuffer;
    }
#endif
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const)
{
#ifndef BUILD_BRIDGE
    if (kProcessMode != PROCESS_MODE_PATCHBAY)
        fBuffer = nullptr;
    else if (! kIsInput)
        carla_zeroFloat(fBuffer, PATCHBAY_BUFFER_SIZE);
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Event port

CarlaEngineEventPort::CarlaEngineEventPort(const bool isInput, const ProcessMode processMode)
    : CarlaEnginePort(isInput, processMode),
      kMaxEventCount(processMode == PROCESS_MODE_CONTINUOUS_RACK ? RACK_EVENT_COUNT : PATCHBAY_EVENT_COUNT)
{
    qDebug("CarlaEngineEventPort::CarlaEngineEventPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_PATCHBAY)
        fBuffer = new EngineEvent[PATCHBAY_EVENT_COUNT];
    else
#endif
        fBuffer = nullptr;
}

CarlaEngineEventPort::~CarlaEngineEventPort()
{
    qDebug("CarlaEngineEventPort::~CarlaEngineEventPort()");

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        CARLA_ASSERT(fBuffer);

        if (fBuffer)
            delete[] fBuffer;
    }
#endif
}

void CarlaEngineEventPort::initBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(engine != nullptr);

    if (engine == nullptr)
        return;

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_CONTINUOUS_RACK)
        fBuffer = engine->getRackEventBuffer(kIsInput);
    else if (kProcessMode == PROCESS_MODE_PATCHBAY && ! kIsInput)
        carla_zeroMem(fBuffer, sizeof(EngineEvent)*PATCHBAY_EVENT_COUNT);
#endif
}

uint32_t CarlaEngineEventPort::getEventCount()
{
    if (! kIsInput)
        return 0;

    CARLA_ASSERT(fBuffer != nullptr);

    if (fBuffer == nullptr)
        return 0;

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        uint32_t count = 0;
        const EngineEvent* const events = fBuffer;

        for (unsigned short i=0; i < kMaxEventCount; i++, count++)
        {
            if (events[i].type == kEngineEventTypeNull)
                break;
        }

        return count;
    }
#endif

    return 0;
}

const EngineEvent* CarlaEngineEventPort::getEvent(const uint32_t index)
{
    if (! kIsInput)
        return nullptr;

    CARLA_ASSERT(fBuffer != nullptr);
    CARLA_ASSERT(index < kMaxEventCount);

    if (fBuffer == nullptr)
        return nullptr;
    if (index >= kMaxEventCount)
        return nullptr;

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        const EngineEvent* const events = fBuffer;

        return &events[index];
    }
#endif

    return nullptr;
}

void CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t parameter, const double value)
{
    if (kIsInput)
        return;

    CARLA_ASSERT(fBuffer != nullptr);
    CARLA_ASSERT(type != kEngineControlEventTypeNull);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

    if (fBuffer == nullptr)
        return;
    if (type == kEngineControlEventTypeNull)
        return;
    if (channel >= MAX_MIDI_CHANNELS)
        return;
    if (type == kEngineControlEventTypeParameter)
    {
        CARLA_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(parameter));
    }

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        EngineEvent* const events = fBuffer;

        for (unsigned short i=0; i < kMaxEventCount; i++)
        {
            if (events[i].type != kEngineEventTypeNull)
                continue;

            events[i].type    = kEngineEventTypeControl;
            events[i].time    = time;
            events[i].channel = channel;

            events[i].ctrl.type      = type;
            events[i].ctrl.parameter = parameter;
            events[i].ctrl.value     = value;

            return;
        }

        qWarning("CarlaEngineEventPort::writeControlEvent() - buffer full");
    }
#else
    Q_UNUSED(time);
    Q_UNUSED(parameter);
    Q_UNUSED(value);
#endif
}

void CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t* const data, const uint8_t size)
{
    if (kIsInput)
        return;

    CARLA_ASSERT(fBuffer != nullptr);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(data);
    CARLA_ASSERT(size > 0);

    if (fBuffer == nullptr)
        return;
    if (channel >= MAX_MIDI_CHANNELS)
        return;
    if (! (data && size > 0))
        return;

#ifndef BUILD_BRIDGE
    if (kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        if (size > 3)
            return;

        EngineEvent* const events = fBuffer;

        for (unsigned short i=0; i < kMaxEventCount; i++)
        {
            if (events[i].type != kEngineEventTypeNull)
                continue;

            events[i].type    = kEngineEventTypeMidi;
            events[i].time    = time;
            events[i].channel = channel;

            events[i].midi.data[0] = data[0];
            events[i].midi.data[1] = data[1];
            events[i].midi.data[2] = data[2];
            events[i].midi.size    = size;

            return;
        }

        qWarning("CarlaEngineEventPort::writeMidiEvent() - buffer full");
    }
#else
    Q_UNUSED(time);
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine client (Abstract)

CarlaEngineClient::CarlaEngineClient(const EngineType engineType, const ProcessMode processMode)
    : kEngineType(engineType),
      kProcessMode(processMode)
{
    qDebug("CarlaEngineClient::CarlaEngineClient(%s, %s)", EngineType2Str(engineType), ProcessMode2Str(processMode));
    CARLA_ASSERT(engineType != kEngineTypeNull);

    fActive  = false;
    fLatency = 0;
}

CarlaEngineClient::~CarlaEngineClient()
{
    qDebug("CarlaEngineClient::~CarlaEngineClient()");
    CARLA_ASSERT(! fActive);
}

void CarlaEngineClient::activate()
{
    qDebug("CarlaEngineClient::activate()");
    CARLA_ASSERT(! fActive);

    fActive = true;
}

void CarlaEngineClient::deactivate()
{
    qDebug("CarlaEngineClient::deactivate()");
    CARLA_ASSERT(fActive);

    fActive = false;
}

bool CarlaEngineClient::isActive() const
{
    qDebug("CarlaEngineClient::isActive()");

    return fActive;
}

bool CarlaEngineClient::isOk() const
{
    qDebug("CarlaEngineClient::isOk()");

    return true;
}

uint32_t CarlaEngineClient::getLatency() const
{
    return fLatency;
}

void CarlaEngineClient::setLatency(const uint32_t samples)
{
    fLatency = samples;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine()
    : fData(new CarlaEngineProtectedData(this))
{
    qDebug("CarlaEngine::CarlaEngine()");

    fBufferSize = 0;
    fSampleRate = 0.0;
}

CarlaEngine::~CarlaEngine()
{
    qDebug("CarlaEngine::~CarlaEngine()");

    //data = nullptr;
}

// -----------------------------------------------------------------------
// Static values and calls

unsigned int CarlaEngine::getDriverCount()
{
    qDebug("CarlaEngine::getDriverCount()");

    unsigned int count = 0;
#ifdef WANT_JACK
    count += 1;
#endif
#ifdef WANT_RTAUDIO
    count += getRtAudioApiCount();
#endif
    return count;
}

const char* CarlaEngine::getDriverName(unsigned int index)
{
    qDebug("CarlaEngine::getDriverName(%i)", index);

#ifdef WANT_JACK
    if (index == 0)
        return "JACK";
    else
        index -= 1;
#endif

#ifdef WANT_RTAUDIO
    if (index < getRtAudioApiCount())
        return getRtAudioApiName(index);
#endif

    qWarning("CarlaEngine::getDriverName(%i) - invalid index", index);
    return nullptr;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
    qDebug("CarlaEngine::newDriverByName(\"%s\")", driverName);

#ifdef WANT_JACK
    if (strcmp(driverName, "JACK") == 0)
        return newJack();
#else
    if (false)
        pass();
#endif

#ifdef WANT_RTAUDIO
# ifdef __LINUX_ALSA__
    else if (strcmp(driverName, "ALSA") == 0)
        return newRtAudio(RTAUDIO_LINUX_ALSA);
# endif
# ifdef __LINUX_PULSE__
    else if (strcmp(driverName, "PulseAudio") == 0)
        return newRtAudio(RTAUDIO_LINUX_PULSE);
# endif
# ifdef __LINUX_OSS__
    else if (strcmp(driverName, "OSS") == 0)
        return newRtAudio(RTAUDIO_LINUX_OSS);
# endif
# ifdef __UNIX_JACK__
    else if (strcmp(driverName, "JACK (RtAudio)") == 0)
        return newRtAudio(RTAUDIO_UNIX_JACK);
# endif
# ifdef __MACOSX_CORE__
    else if (strcmp(driverName, "CoreAudio") == 0)
        return newRtAudio(RTAUDIO_MACOSX_CORE);
# endif
# ifdef __WINDOWS_ASIO__
    else if (strcmp(driverName, "ASIO") == 0)
        return newRtAudio(RTAUDIO_WINDOWS_ASIO);
# endif
# ifdef __WINDOWS_DS__
    else if (strcmp(driverName, "DirectSound") == 0)
        return newRtAudio(RTAUDIO_WINDOWS_DS);
# endif
#endif

    return nullptr;
}

// -----------------------------------------------------------------------
// Maximum values

unsigned int CarlaEngine::maxClientNameSize() const
{
    return STR_MAX/2;
}

unsigned int CarlaEngine::maxPortNameSize() const
{
    return STR_MAX;
}

unsigned int CarlaEngine::currentPluginCount() const
{
    return fData->curPluginCount;
}

unsigned int CarlaEngine::maxPluginNumber() const
{
    return fData->maxPluginNumber;
}

// -----------------------------------------------------------------------
// Virtual, per-engine type calls

bool CarlaEngine::init(const char* const clientName)
{
    qDebug("CarlaEngine::init(\"%s\")", clientName);
    CARLA_ASSERT(fData->plugins == nullptr);

    fData->aboutToClose = false;
    fData->curPluginCount = 0;
    fData->maxPluginNumber = 0;

    switch (fOptions.processMode)
    {
    case PROCESS_MODE_CONTINUOUS_RACK:
        fData->maxPluginNumber = MAX_RACK_PLUGINS;
        break;
    case PROCESS_MODE_PATCHBAY:
        fData->maxPluginNumber = MAX_PATCHBAY_PLUGINS;
        break;
    default:
        fData->maxPluginNumber = MAX_DEFAULT_PLUGINS;
        break;
    }

    fData->plugins = new EnginePluginData[fData->maxPluginNumber];

    fData->osc.init(clientName);

#ifndef BUILD_BRIDGE
    fData->oscData = fData->osc.getControlData();
#else
    fData->oscData = nullptr; // set in setOscBridgeData()
#endif

#ifndef BUILD_BRIDGE
    //if (strcmp(clientName, "Carla") != 0)
    carla_setprocname(clientName);
#endif

    fData->thread.startNow();

    return true;
}

bool CarlaEngine::close()
{
    qDebug("CarlaEngine::close()");
    CARLA_ASSERT(fData->plugins != nullptr);

    fData->thread.stopNow();

#ifndef BUILD_BRIDGE
    osc_send_control_exit();
#endif
    fData->osc.close();

    fData->oscData = nullptr;

    fData->aboutToClose = true;
    fData->curPluginCount = 0;
    fData->maxPluginNumber = 0;

    if (fData->plugins != nullptr)
    {
        delete[] fData->plugins;
        fData->plugins = nullptr;
    }

    fName.clear();

    return true;
}

// -----------------------------------------------------------------------
// Plugin management

#if 0
int CarlaEngine::getNewPluginId() const
{
    qDebug("CarlaEngine::getNewPluginId()");
    CARLA_ASSERT(data->maxPluginNumber > 0);

    return data->nextPluginId;
}
#endif

CarlaPlugin* CarlaEngine::getPlugin(const int id) const
{
    qDebug("CarlaEngine::getPlugin(%i) [count:%i]", id, fData->curPluginCount);
    CARLA_ASSERT(fData->curPluginCount > 0);
    CARLA_ASSERT(id < fData->curPluginCount);
    CARLA_ASSERT(fData->plugins != nullptr);

    if (id < fData->curPluginCount && fData->plugins)
        return fData->plugins[id].plugin;

    return nullptr;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const int id) const
{
    return fData->plugins[id].plugin;
}

const char* CarlaEngine::getNewUniquePluginName(const char* const name)
{
    qDebug("CarlaEngine::getNewUniquePluginName(\"%s\")", name);
    CARLA_ASSERT(fData->curPluginCount > 0);
    CARLA_ASSERT(fData->plugins != nullptr);
    CARLA_ASSERT(name);

    CarlaString sname(name);

    if (sname.isEmpty() || fData->plugins == nullptr)
        return strdup("(No name)");

    sname.truncate(maxClientNameSize()-5-1); // 5 = strlen(" (10)")
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (unsigned short i=0; i < fData->curPluginCount; i++)
    {
#if 0
        // Check if unique name doesn't exist
        if (const char* const pluginName = fData->plugins[i].plugin->name())
        {
            if (sname != pluginName)
                continue;
        }
#endif

        // Check if string has already been modified
        {
            const size_t len = sname.length();

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
                    n3 = char(n3 + 1);
                }
                else
                    n2 = char(n2 + 1);

                sname[len-2] = n2;
                sname[len-3] = n3;

                continue;
            }
        }

        // Modify string if not
        sname += " (2)";
    }

    return strdup(sname);
}

int CarlaEngine::addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra)
{
    qDebug("CarlaEngine::addPlugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", BinaryType2Str(btype), PluginType2Str(ptype), filename, name, label, extra);
    CARLA_ASSERT(btype != BINARY_NONE);
    CARLA_ASSERT(ptype != PLUGIN_NONE);
    CARLA_ASSERT(filename);
    CARLA_ASSERT(label);

    CarlaPlugin::Initializer init = {
        this,
        filename,
        name,
        label
    };

    CarlaPlugin* plugin = nullptr;

#ifndef BUILD_BRIDGE
    const char* bridgeBinary;

    switch (btype)
    {
    case BINARY_POSIX32:
        bridgeBinary = fOptions.bridge_posix32.isNotEmpty() ? (const char*)fOptions.bridge_posix32 : nullptr;
        break;
    case BINARY_POSIX64:
        bridgeBinary = fOptions.bridge_posix64.isNotEmpty() ? (const char*)fOptions.bridge_posix64 : nullptr;
        break;
    case BINARY_WIN32:
        bridgeBinary = fOptions.bridge_win32.isNotEmpty() ? (const char*)fOptions.bridge_win32 : nullptr;
        break;
    case BINARY_WIN64:
        bridgeBinary = fOptions.bridge_win64.isNotEmpty() ? (const char*)fOptions.bridge_win64 : nullptr;
        break;
    default:
        bridgeBinary = nullptr;
        break;
    }

#ifndef Q_OS_WIN
    if (btype == BINARY_NATIVE && fOptions.bridge_native.isNotEmpty())
        bridgeBinary = (const char*)fOptions.bridge_native;
#endif

    if (fOptions.preferPluginBridges && bridgeBinary != nullptr)
    {
        // TODO
        if (fOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            setLastError("Can only use bridged plugins in JACK Multi-Client mode");
            return -1;
        }

        // TODO
        if (type() != kEngineTypeJack)
        {
            setLastError("Can only use bridged plugins with JACK backend");
            return -1;
        }

#if 0
        plugin = CarlaPlugin::newBridge(init, btype, ptype, bridgeBinary);
#endif
    }
    else
#endif // BUILD_BRIDGE
    {
#if 0
        switch (ptype)
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
#ifndef BUILD_BRIDGE
            plugin = CarlaPlugin::newNative(init);
#endif
            break;

        case PLUGIN_LADSPA:
#ifdef WANT_LADSPA
            plugin = CarlaPlugin::newLADSPA(init, extra);
#endif
            break;

        case PLUGIN_DSSI:
#ifdef WANT_DSSI
            plugin = CarlaPlugin::newDSSI(init, extra);
#endif
            break;

        case PLUGIN_LV2:
#ifdef WANT_LV2
            plugin = CarlaPlugin::newLV2(init);
#endif
            break;

        case PLUGIN_VST:
#ifdef WANT_VST
            plugin = CarlaPlugin::newVST(init);
#endif
            break;

        case PLUGIN_GIG:
#ifdef WANT_LINUXSAMPLER
            plugin = CarlaPlugin::newGIG(init);
#endif
            break;

        case PLUGIN_SF2:
#ifdef WANT_FLUIDSYNTH
            plugin = CarlaPlugin::newSF2(init);
#endif
            break;

        case PLUGIN_SFZ:
#ifdef WANT_LINUXSAMPLER
            plugin = CarlaPlugin::newSFZ(init);
#endif
            break;
        }
#endif
    }

    if (! plugin)
        return -1;

    const unsigned int id = fData->curPluginCount++;

#if 0
    plugin->setId(id);
#endif

    return static_cast<int>(id);
}

bool CarlaEngine::removePlugin(const unsigned int id)
{
    qDebug("CarlaEngine::removePlugin(%i)", id);
    CARLA_ASSERT(fData->curPluginCount > 0);
    CARLA_ASSERT(id < fData->curPluginCount);
    CARLA_ASSERT(fData->plugins != nullptr);

    if (fData->plugins == nullptr)
    {
        // TODO - warning
        return false;
    }

    CarlaPlugin* plugin = fData->plugins[id].plugin;

    CARLA_ASSERT(plugin);

    if (plugin /*&& plugin->id() == id*/)
    {
#if 0
        CARLA_ASSERT(plugin->id() == id);
#endif

        fData->thread.stopNow();

        // wait for processing to stop for this plugin
        // TODO

        // clear this plugin
        fData->plugins[id].plugin      = nullptr;
        fData->plugins[id].insPeak[0]  = 0.0;
        fData->plugins[id].insPeak[1]  = 0.0;
        fData->plugins[id].outsPeak[0] = 0.0;
        fData->plugins[id].outsPeak[1] = 0.0;

        // wait for processing to stop for this plugin
        // TODO

        //processLock();
        //plugin->setEnabled(false);
        //data->carlaPlugins[id] = nullptr;
        //data->uniqueNames[id]  = nullptr;
        //processUnlock();

        delete plugin;

#ifndef BUILD_BRIDGE
        osc_send_control_remove_plugin(static_cast<int32_t>(id));

        // move all plugins 1 spot backwards
        for (unsigned int i=id; i < fData->curPluginCount-1; i++)
        {
            plugin = fData->plugins[i+1].plugin;

            CARLA_ASSERT(plugin);

#if 0
            if (plugin)
                plugin->setId(i);
#endif

            fData->plugins[i].plugin      = plugin;
            fData->plugins[i].insPeak[0]  = 0.0;
            fData->plugins[i].insPeak[1]  = 0.0;
            fData->plugins[i].outsPeak[0] = 0.0;
            fData->plugins[i].outsPeak[1] = 0.0;
        }

        fData->curPluginCount--;

        if (isRunning() && ! fData->aboutToClose)
            fData->thread.startNow();
#endif

        return true;
    }

    qCritical("CarlaEngine::removePlugin(%i) - could not find plugin", id);
    setLastError("Could not find plugin to remove");
    return false;
}

void CarlaEngine::removeAllPlugins()
{
    qDebug("CarlaEngine::removeAllPlugins()");

    fData->thread.stopNow();

    const unsigned int oldCount = fData->curPluginCount;

    // wait for processing
    // TODO

    fData->curPluginCount = 0;
    fData->maxPluginNumber = 0;

    for (unsigned short i=0; i < oldCount; i++)
    {
        CarlaPlugin* const plugin = fData->plugins[i].plugin;

        CARLA_ASSERT(plugin);

        if (plugin)
            delete plugin;

        // clear this plugin
        fData->plugins[i].plugin      = nullptr;
        fData->plugins[i].insPeak[0]  = 0.0;
        fData->plugins[i].insPeak[1]  = 0.0;
        fData->plugins[i].outsPeak[0] = 0.0;
        fData->plugins[i].outsPeak[1] = 0.0;
    }

    if (isRunning() && ! fData->aboutToClose)
        fData->thread.startNow();
}

#if 0
void CarlaEngine::idlePluginGuis()
{
    CARLA_ASSERT(data->maxPluginNumber > 0);

    for (unsigned short i=0; i < data->maxPluginNumber; i++)
    {
        CarlaPlugin* const plugin = data->carlaPlugins[i];

        if (plugin && plugin->enabled())
            plugin->idleGui();
    }
}

void CarlaEngine::__bridgePluginRegister(const unsigned short id, CarlaPlugin* const plugin)
{
    data->carlaPlugins[id] = plugin;
}
#endif

// -----------------------------------------------------------------------
// Information (base)

void CarlaEngine::setAboutToClose()
{
    qDebug("CarlaEngine::setAboutToClose()");
    fData->aboutToClose = true;
}

#if 0
// -----------------------------------------------------------------------
// Information (audio peaks)

double CarlaEngine::getInputPeak(const unsigned short pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < data->maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    return data->insPeak[pluginId*MAX_PEAKS + id];
}

double CarlaEngine::getOutputPeak(const unsigned short pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < data->maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    return data->outsPeak[pluginId*MAX_PEAKS + id];
}

void CarlaEngine::setInputPeak(const unsigned short pluginId, const unsigned short id, double value)
{
    CARLA_ASSERT(pluginId < data->maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    data->insPeak[pluginId*MAX_PEAKS + id] = value;
}

void CarlaEngine::setOutputPeak(const unsigned short pluginId, const unsigned short id, double value)
{
    CARLA_ASSERT(pluginId < data->maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    data->outsPeak[pluginId*MAX_PEAKS + id] = value;
}
#endif

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const CallbackType action, const unsigned short pluginId, const int value1, const int value2, const double value3, const char* const valueStr)
{
    qDebug("CarlaEngine::callback(%s, %i, %i, %i, %f, \"%s\")", CallbackType2Str(action), pluginId, value1, value2, value3, valueStr);

    if (fData->callback)
        fData->callback(fData->callbackPtr, action, pluginId, value1, value2, value3, valueStr);
}

void CarlaEngine::setCallback(const CallbackFunc func, void* const ptr)
{
    qDebug("CarlaEngine::setCallback(%p, %p)", func, ptr);
    CARLA_ASSERT(func);

    fData->callback    = func;
    fData->callbackPtr = ptr;
}

// -----------------------------------------------------------------------
// Error handling

const char* CarlaEngine::getLastError() const
{
    return (const char*)fData->lastError;
}

void CarlaEngine::setLastError(const char* const error)
{
    fData->lastError = error;
}

// -----------------------------------------------------------------------
// Global options

#ifndef BUILD_BRIDGE

const QProcessEnvironment& CarlaEngine::getOptionsAsProcessEnvironment() const
{
    return fData->procEnv;
}

#define CARLA_ENGINE_SET_OPTION_RUNNING_CHECK \
    if (isRunning()) \
        return qCritical("CarlaEngine::setOption(%s, %i, \"%s\") - Cannot set this option while engine is running!", OptionsType2Str(option), value, valueStr);

void CarlaEngine::setOption(const OptionsType option, const int value, const char* const valueStr)
{
    qDebug("CarlaEngine::setOption(%s, %i, \"%s\")", OptionsType2Str(option), value, valueStr);

    switch (option)
    {
    case OPTION_PROCESS_NAME:
        carla_setprocname(valueStr);
        break;

    case OPTION_PROCESS_MODE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK

        if (value < PROCESS_MODE_SINGLE_CLIENT || value > PROCESS_MODE_PATCHBAY)
            return qCritical("CarlaEngine::setOption(%s, %i, \"%s\") - invalid value", OptionsType2Str(option), value, valueStr);

        fOptions.processMode = static_cast<ProcessMode>(value);
        break;

    case OPTION_MAX_PARAMETERS:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK

        if (value < 0)
            return; // TODO error here

        fOptions.maxParameters = static_cast<uint>(value);
        break;

    case OPTION_PREFERRED_BUFFER_SIZE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.preferredBufferSize = static_cast<uint>(value);
        break;

    case OPTION_PREFERRED_SAMPLE_RATE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.preferredSampleRate = static_cast<uint>(value);
        break;

    case OPTION_FORCE_STEREO:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.forceStereo = (value != 0);
        break;

#ifdef WANT_DSSI
    case OPTION_USE_DSSI_VST_CHUNKS:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.useDssiVstChunks = (value != 0);
        break;
#endif

    case OPTION_PREFER_PLUGIN_BRIDGES:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.preferPluginBridges = (value != 0);
        break;

    case OPTION_PREFER_UI_BRIDGES:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.preferUiBridges = (value != 0);
        break;

    case OPTION_OSC_UI_TIMEOUT:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.oscUiTimeout = static_cast<uint>(value);
        break;

    case OPTION_PATH_BRIDGE_NATIVE:
        fOptions.bridge_native = valueStr;
        break;
    case OPTION_PATH_BRIDGE_POSIX32:
        fOptions.bridge_posix32 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_POSIX64:
        fOptions.bridge_posix64 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_WIN32:
        fOptions.bridge_win32 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_WIN64:
        fOptions.bridge_win64 = valueStr;
        break;

#ifdef WANT_LV2
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        fOptions.bridge_lv2gtk2 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_GTK3:
        fOptions.bridge_lv2gtk3 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_QT4:
        fOptions.bridge_lv2qt4 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_QT5:
        fOptions.bridge_lv2qt5 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_COCOA:
        fOptions.bridge_lv2cocoa = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_WINDOWS:
        fOptions.bridge_lv2win = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_X11:
        fOptions.bridge_lv2x11 = valueStr;
        break;
#endif

#ifdef WANT_VST
    case OPTION_PATH_BRIDGE_VST_COCOA:
        fOptions.bridge_vstcocoa = valueStr;
        break;
    case OPTION_PATH_BRIDGE_VST_HWND:
        fOptions.bridge_vsthwnd = valueStr;
        break;
    case OPTION_PATH_BRIDGE_VST_X11:
        fOptions.bridge_vstx11 = valueStr;
        break;
#endif
    }
}
#endif

// -----------------------------------------------------------------------
// OSC Stuff

#ifdef BUILD_BRIDGE
bool CarlaEngine::isOscBridgeRegistered() const
{
    return (fData->oscData != nullptr);
}
#else
bool CarlaEngine::isOscControlRegistered() const
{
    return fData->osc.isControlRegistered();
}
#endif

void CarlaEngine::idleOsc()
{
    fData->osc.idle();
}

const char* CarlaEngine::getOscServerPathTCP() const
{
    return fData->osc.getServerPathTCP();
}

const char* CarlaEngine::getOscServerPathUDP() const
{
    return fData->osc.getServerPathUDP();
}

#ifdef BUILD_BRIDGE
void CarlaEngine::setOscBridgeData(const CarlaOscData* const oscData)
{
    fData->oscData = oscData;
}
#endif

// -----------------------------------------------------------------------
// protected calls

#ifndef BUILD_BRIDGE
EngineEvent* CarlaEngine::getRackEventBuffer(const bool isInput)
{
    // TODO
    return nullptr;
}

void CarlaEngine::processRack(float* inBuf[2], float* outBuf[2], const uint32_t frames)
{
    // initialize outputs (zero)
    carla_zeroFloat(outBuf[0], frames);
    carla_zeroFloat(outBuf[1], frames);
#if 0
    std::memset(rackEventsOut, 0, sizeof(EngineEvent)*MAX_EVENTS);

    bool processed = false;

    // process plugins
    for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
    {
        CarlaPlugin* const plugin = getPluginUnchecked(i);

        if (! (plugin && plugin->enabled()))
            continue;

        if (processed)
        {
            // initialize inputs (from previous outputs)
            memcpy(inBuf[0], outBuf[0], sizeof(float)*frames);
            memcpy(inBuf[1], outBuf[1], sizeof(float)*frames);
            memcpy(rackMidiEventsIn, rackMidiEventsOut, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);

            // initialize outputs (zero)
            carla_zeroFloat(outBuf[0], frames);
            carla_zeroFloat(outBuf[1], frames);
            memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);
        }

        // process
        processLock();
        plugin->initBuffers();

        if (false /*plugin->data->processHighPrecision*/)
        {
            float* inBuf2[2];
            float* outBuf2[2];

            for (uint32_t j=0; j < frames; j += 8)
            {
                inBuf2[0] = inBuf[0] + j;
                inBuf2[1] = inBuf[1] + j;

                outBuf2[0] = outBuf[0] + j;
                outBuf2[1] = outBuf[1] + j;

                plugin->process(inBuf2, outBuf2, 8, j);
            }
        }
        else
            plugin->process(inBuf, outBuf, frames);

        processUnlock();

        // if plugin has no audio inputs, add previous buffers
        if (plugin->audioInCount() == 0)
        {
            for (uint32_t j=0; j < frames; j++)
            {
                outBuf[0][j] += inBuf[0][j];
                outBuf[1][j] += inBuf[1][j];
            }
        }

        // if plugin has no midi output, add previous midi input
        if (plugin->midiOutCount() == 0)
        {
            memcpy(rackMidiEventsOut, rackMidiEventsIn, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);
        }

        // set peaks
        {
            double inPeak1  = 0.0;
            double inPeak2  = 0.0;
            double outPeak1 = 0.0;
            double outPeak2 = 0.0;

            for (uint32_t k=0; k < frames; k++)
            {
                // TODO - optimize this
                if (std::abs(inBuf[0][k]) > inPeak1)
                    inPeak1 = std::abs(inBuf[0][k]);
                if (std::abs(inBuf[1][k]) > inPeak2)
                    inPeak2 = std::abs(inBuf[1][k]);
                if (std::abs(outBuf[0][k]) > outPeak1)
                    outPeak1 = std::abs(outBuf[0][k]);
                if (std::abs(outBuf[1][k]) > outPeak2)
                    outPeak2 = std::abs(outBuf[1][k]);
            }

            data->insPeak[i*MAX_PEAKS + 0] = inPeak1;
            data->insPeak[i*MAX_PEAKS + 1] = inPeak2;
            data->outsPeak[i*MAX_PEAKS + 0] = outPeak1;
            data->outsPeak[i*MAX_PEAKS + 1] = outPeak2;
        }

        processed = true;
    }

    // if no plugins in the rack, copy inputs over outputs
    if (! processed)
    {
        std::memcpy(outBuf[0], inBuf[0], sizeof(float)*frames);
        std::memcpy(outBuf[1], inBuf[1], sizeof(float)*frames);
        std::memcpy(rackEventsOut, rackEventsIn, sizeof(EngineEvent)*MAX_EVENTS);
    }
#endif
}

void CarlaEngine::processPatchbay(float** inBuf, float** outBuf, const uint32_t bufCount[2], const uint32_t frames)
{
    // TODO
    Q_UNUSED(inBuf);
    Q_UNUSED(outBuf);
    Q_UNUSED(bufCount);
    Q_UNUSED(frames);
}
#endif

void CarlaEngine::bufferSizeChanged(const uint32_t newBufferSize)
{
    qDebug("CarlaEngine::bufferSizeChanged(%i)", newBufferSize);

#if 0
    for (unsigned short i=0; i < data->maxPluginNumber; i++)
    {
        if (data->carlaPlugins[i] && data->carlaPlugins[i]->enabled() /*&& ! data->carlaPlugins[i]->data->processHighPrecision*/)
            data->carlaPlugins[i]->bufferSizeChanged(newBufferSize);
    }
#endif
}

void CarlaEngine::sampleRateChanged(const double newSampleRate)
{
    qDebug("CarlaEngine::sampleRateChanged(%g)", newSampleRate);

    // TODO
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine OSC stuff

#ifdef BUILD_BRIDGE
void CarlaEngine::osc_send_peaks(CarlaPlugin* const plugin)
#else
void CarlaEngine::osc_send_peaks(CarlaPlugin* const plugin, const unsigned short& id)
#endif
{
#if 0
    // Peak values
    if (plugin->audioInCount() > 0)
    {
#ifdef BUILD_BRIDGE
        osc_send_bridge_set_inpeak(1);
        osc_send_bridge_set_inpeak(2);
#else
        osc_send_control_set_input_peak_value(id, 1);
        osc_send_control_set_input_peak_value(id, 2);
#endif
    }
    if (plugin->audioOutCount() > 0)
    {
#ifdef BUILD_BRIDGE
        osc_send_bridge_set_outpeak(1);
        osc_send_bridge_set_outpeak(2);
#else
        osc_send_control_set_output_peak_value(id, 1);
        osc_send_control_set_output_peak_value(id, 2);
#endif
    }
#endif
}

#ifndef BUILD_BRIDGE
void CarlaEngine::osc_send_control_add_plugin_start(const int32_t pluginId, const char* const pluginName)
{
    qDebug("CarlaEngine::osc_send_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);
    CARLA_ASSERT(pluginName);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+18];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/add_plugin_start");
        lo_send(fData->oscData->target, target_path, "is", pluginId, pluginName);
    }
}

void CarlaEngine::osc_send_control_add_plugin_end(const int32_t pluginId)
{
    qDebug("CarlaEngine::osc_send_control_add_plugin_end(%i)", pluginId);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+16];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/add_plugin_end");
        lo_send(fData->oscData->target, target_path, "i", pluginId);
    }
}

void CarlaEngine::osc_send_control_remove_plugin(const int32_t pluginId)
{
    qDebug("CarlaEngine::osc_send_control_remove_plugin(%i)", pluginId);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+15];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/remove_plugin");
        lo_send(fData->oscData->target, target_path, "i", pluginId);
    }
}

void CarlaEngine::osc_send_control_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints, const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    qDebug("CarlaEngine::osc_send_control_set_plugin_data(%i, %i, %i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);
    CARLA_ASSERT(type != PLUGIN_NONE);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+17];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_plugin_data");
        lo_send(fData->oscData->target, target_path, "iiiissssh", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::osc_send_control_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts, const int32_t cIns, const int32_t cOuts, const int32_t cTotals)
{
    qDebug("CarlaEngine::osc_send_control_set_plugin_ports(%i, %i, %i, %i, %i, %i, %i, %i)", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+18];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_plugin_ports");
        lo_send(fData->oscData->target, target_path, "iiiiiiii", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    }
}

void CarlaEngine::osc_send_control_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints, const char* const name, const char* const label, const double current)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_data(%i, %i, %i, %i, \"%s\", \"%s\", %g)", pluginId, index, type, hints, name, label, current);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(type != PARAMETER_UNKNOWN);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+20];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_parameter_data");
        lo_send(fData->oscData->target, target_path, "iiiissd", pluginId, index, type, hints, name, label, current);
    }
}

void CarlaEngine::osc_send_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const double min, const double max, const double def, const double step, const double stepSmall, const double stepLarge)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_ranges(%i, %i, %g, %g, %g, %g, %g, %g)", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(min < max);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+22];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_parameter_ranges");
        lo_send(fData->oscData->target, target_path, "iidddddd", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);
    CARLA_ASSERT(index >= 0);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+23];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_parameter_midi_cc");
        lo_send(fData->oscData->target, target_path, "iii", pluginId, index, cc);
    }
}

void CarlaEngine::osc_send_control_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(channel >= 0 && channel < 16);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+28];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_parameter_midi_channel");
        lo_send(fData->oscData->target, target_path, "iii", pluginId, index, channel);
    }
}

void CarlaEngine::osc_send_control_set_parameter_value(const int32_t pluginId, const int32_t index, const double value)
{
#if DEBUG
    if (index < 0)
        qDebug("CarlaEngine::osc_send_control_set_parameter_value(%i, %s, %g)", pluginId, InternalParametersIndex2Str((InternalParametersIndex)index), value);
    else
        qDebug("CarlaEngine::osc_send_control_set_parameter_value(%i, %i, %g)", pluginId, index, value);
#endif
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->curPluginCount);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+21];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_parameter_value");
        lo_send(fData->oscData->target, target_path, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_default_value(const int32_t pluginId, const int32_t index, const double value)
{
    qDebug("CarlaEngine::osc_send_control_set_default_value(%i, %i, %g)", pluginId, index, value);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(index >= 0);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+19];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_default_value");
        lo_send(fData->oscData->target, target_path, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_program(const int32_t pluginId, const int32_t index)
{
    qDebug("CarlaEngine::osc_send_control_set_program(%i, %i)", pluginId, index);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+13];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_program");
        lo_send(fData->oscData->target, target_path, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_control_set_program_count(const int32_t pluginId, const int32_t count)
{
    qDebug("CarlaEngine::osc_send_control_set_program_count(%i, %i)", pluginId, count);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(count >= 0);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+19];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_program_count");
        lo_send(fData->oscData->target, target_path, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_control_set_program_name(const int32_t pluginId, const int32_t index, const char* const name)
{
    qDebug("CarlaEngine::osc_send_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(name);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+18];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_program_name");
        lo_send(fData->oscData->target, target_path, "iis", pluginId, index, name);
    }
}

void CarlaEngine::osc_send_control_set_midi_program(const int32_t pluginId, const int32_t index)
{
    qDebug("CarlaEngine::osc_send_control_set_midi_program(%i, %i)", pluginId, index);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+18];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_midi_program");
        lo_send(fData->oscData->target, target_path, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_control_set_midi_program_count(const int32_t pluginId, const int32_t count)
{
    qDebug("CarlaEngine::osc_send_control_set_midi_program_count(%i, %i)", pluginId, count);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(count >= 0);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+24];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_midi_program_count");
        lo_send(fData->oscData->target, target_path, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_control_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name)
{
    qDebug("CarlaEngine::osc_send_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(bank >= 0);
    CARLA_ASSERT(program >= 0);
    CARLA_ASSERT(name);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+23];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_midi_program_data");
        lo_send(fData->oscData->target, target_path, "iiiis", pluginId, index, bank, program, name);
    }
}

void CarlaEngine::osc_send_control_note_on(const int32_t pluginId, const int32_t channel, const int32_t note, const int32_t velo)
{
    qDebug("CarlaEngine::osc_send_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    CARLA_ASSERT(note >= 0 && note < 128);
    CARLA_ASSERT(velo > 0 && velo < 128);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+9];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/note_on");
        lo_send(fData->oscData->target, target_path, "iiii", pluginId, channel, note, velo);
    }
}

void CarlaEngine::osc_send_control_note_off(const int32_t pluginId, const int32_t channel, const int32_t note)
{
    qDebug("CarlaEngine::osc_send_control_note_off(%i, %i, %i)", pluginId, channel, note);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    CARLA_ASSERT(note >= 0 && note < 128);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+10];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/note_off");
        lo_send(fData->oscData->target, target_path, "iii", pluginId, channel, note);
    }
}

#if 0
void CarlaEngine::osc_send_control_set_input_peak_value(const int32_t pluginId, const int32_t portId)
{
    //qDebug("CarlaEngine::osc_send_control_set_input_peak_value(%i, %i)", pluginId, portId);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+22];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_input_peak_value");
        lo_send(fData->oscData->target, target_path, "iid", pluginId, portId, data->insPeak[pluginId*MAX_PEAKS + portId-1]);
    }
}

void CarlaEngine::osc_send_control_set_output_peak_value(const int32_t pluginId, const int32_t portId)
{
    //qDebug("CarlaEngine::osc_send_control_set_output_peak_value(%i, %i)", pluginId, portId);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)fData->maxPluginNumber);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+23];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/set_output_peak_value");
        lo_send(fData->oscData->target, target_path, "iid", pluginId, portId, data->outsPeak[pluginId*MAX_PEAKS + portId-1]);
    }
}
#endif

void CarlaEngine::osc_send_control_exit()
{
    qDebug("CarlaEngine::osc_send_control_exit()");
    CARLA_ASSERT(fData->oscData);

    if (fData->oscData && fData->oscData->target)
    {
        char target_path[strlen(fData->oscData->path)+6];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/exit");
        lo_send(fData->oscData->target, target_path, "");
    }
}
#else
void CarlaEngine::osc_send_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    qDebug("CarlaEngine::osc_send_bridge_audio_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+20];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_audio_count");
        lo_send(fData->oscData->target, target_path, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    qDebug("CarlaEngine::osc_send_bridge_midi_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+19];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_midi_count");
        lo_send(fData->oscData->target, target_path, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+24];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_parameter_count");
        lo_send(fData->oscData->target, target_path, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_program_count(const int32_t count)
{
    qDebug("CarlaEngine::osc_send_bridge_program_count(%i)", count);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+22];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_program_count");
        lo_send(fData->oscData->target, target_path, "i", count);
    }
}

void CarlaEngine::osc_send_bridge_midi_program_count(const int32_t count)
{
    qDebug("CarlaEngine::osc_send_bridge_midi_program_count(%i)", count);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+27];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_midi_program_count");
        lo_send(fData->oscData->target, target_path, "i", count);
    }
}

void CarlaEngine::osc_send_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    qDebug("CarlaEngine::osc_send_bridge_plugin_info(%i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", category, hints, name, label, maker, copyright, uniqueId);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(name);
    CARLA_ASSERT(label);
    CARLA_ASSERT(maker);
    CARLA_ASSERT(copyright);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+20];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_plugin_info");
        lo_send(fData->oscData->target, target_path, "iissssh", category, hints, name, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::osc_send_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_info(%i, \"%s\", \"%s\")", index, name, unit);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(name);
    CARLA_ASSERT(unit);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+23];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_parameter_info");
        lo_send(fData->oscData->target, target_path, "iss", index, name, unit);
    }
}

void CarlaEngine::osc_send_bridge_parameter_data(const int32_t index, const int32_t type, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_data(%i, %i, %i, %i, %i, %i)", index, type, rindex, hints, midiChannel, midiCC);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+23];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_parameter_data");
        lo_send(fData->oscData->target, target_path, "iiiiii", index, type, rindex, hints, midiChannel, midiCC);
    }
}

void CarlaEngine::osc_send_bridge_parameter_ranges(const int32_t index, const double def, const double min, const double max, const double step, const double stepSmall, const double stepLarge)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_ranges(%i, %g, %g, %g, %g, %g, %g)", index, def, min, max, step, stepSmall, stepLarge);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+25];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_parameter_ranges");
        lo_send(fData->oscData->target, target_path, "idddddd", index, def, min, max, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_bridge_program_info(const int32_t index, const char* const name)
{
    qDebug("CarlaEngine::osc_send_bridge_program_info(%i, \"%s\")", index, name);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+21];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_program_info");
        lo_send(fData->oscData->target, target_path, "is", index, name);
    }
}

void CarlaEngine::osc_send_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label)
{
    qDebug("CarlaEngine::osc_send_bridge_midi_program_info(%i, %i, %i, \"%s\")", index, bank, program, label);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+26];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_midi_program_info");
        lo_send(fData->oscData->target, target_path, "iiis", index, bank, program, label);
    }
}

void CarlaEngine::osc_send_bridge_configure(const char* const key, const char* const value)
{
    qDebug("CarlaEngine::osc_send_bridge_configure(\"%s\", \"%s\")", key, value);
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(key);
    CARLA_ASSERT(value);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+18];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_configure");
        lo_send(fData->oscData->target, target_path, "ss", key, value);
    }
}

void CarlaEngine::osc_send_bridge_set_parameter_value(const int32_t index, const double value)
{
    qDebug("CarlaEngine::osc_send_bridge_set_parameter_value(%i, %g)", index, value);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+28];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_parameter_value");
        lo_send(fData->oscData->target, target_path, "id", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_default_value(const int32_t index, const double value)
{
    qDebug("CarlaEngine::osc_send_bridge_set_default_value(%i, %g)", index, value);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+26];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_default_value");
        lo_send(fData->oscData->target, target_path, "id", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_program(const int32_t index)
{
    qDebug("CarlaEngine::osc_send_bridge_set_program(%i)", index);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+20];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_program");
        lo_send(fData->oscData->target, target_path, "i", index);
    }
}

void CarlaEngine::osc_send_bridge_set_midi_program(const int32_t index)
{
    qDebug("CarlaEngine::osc_send_bridge_set_midi_program(%i)", index);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+25];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_midi_program");
        lo_send(fData->oscData->target, target_path, "i", index);
    }
}

void CarlaEngine::osc_send_bridge_set_custom_data(const char* const type, const char* const key, const char* const value)
{
    qDebug("CarlaEngine::osc_send_bridge_set_custom_data(\"%s\", \"%s\", \"%s\")", type, key, value);
    CARLA_ASSERT(fData->oscData);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+24];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_custom_data");
        lo_send(fData->oscData->target, target_path, "sss", type, key, value);
    }
}

void CarlaEngine::osc_send_bridge_set_chunk_data(const char* const chunkFile)
{
    qDebug("CarlaEngine::osc_send_bridge_set_chunk_data(\"%s\")", chunkFile);
    CARLA_ASSERT(fData->oscData != nullptr);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+23];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_chunk_data");
        lo_send(fData->oscData->target, target_path, "s", chunkFile);
    }
}

#if 0
void CarlaEngine::osc_send_bridge_set_inpeak(const int32_t portId)
{
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+28];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_inpeak");
        lo_send(fData->oscData->target, target_path, "id", portId, data->insPeak[portId-1]);
    }
}

void CarlaEngine::osc_send_bridge_set_outpeak(const int32_t portId)
{
    CARLA_ASSERT(fData->oscData != nullptr);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (fData->oscData != nullptr && fData->oscData->target != nullptr)
    {
        char target_path[strlen(fData->oscData->path)+29];
        strcpy(target_path, fData->oscData->path);
        strcat(target_path, "/bridge_set_outpeak");
        lo_send(fData->oscData->target, target_path, "id", portId, data->insPeak[portId-1]);
    }
}
#endif
#endif

CARLA_BACKEND_END_NAMESPACE
