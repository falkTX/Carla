/*
 * Carla Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaStateUtils.hpp"
#include "CarlaMIDI.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Engine Helpers

::QMainWindow* getEngineHostWindow(CarlaEngine* const engine)
{
    return CarlaEngineProtectedData::getHostWindow(engine);
}

#ifndef BUILD_BRIDGE
void registerEnginePlugin(CarlaEngine* const engine, const unsigned int id, CarlaPlugin* const plugin)
{
    CarlaEngineProtectedData::registerEnginePlugin(engine, id, plugin);
}
#endif

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine port (Abstract)

CarlaEnginePort::CarlaEnginePort(const bool isInput, const ProcessMode processMode)
    : kIsInput(isInput),
      kProcessMode(processMode)
{
    carla_debug("CarlaEnginePort::CarlaEnginePort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));
}

CarlaEnginePort::~CarlaEnginePort()
{
    carla_debug("CarlaEnginePort::~CarlaEnginePort()");
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Audio port

CarlaEngineAudioPort::CarlaEngineAudioPort(const bool isInput, const ProcessMode processMode)
    : CarlaEnginePort(isInput, processMode),
      fBuffer(nullptr)
{
    carla_debug("CarlaEngineAudioPort::CarlaEngineAudioPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

    if (kProcessMode == PROCESS_MODE_PATCHBAY)
        fBuffer = new float[PATCHBAY_BUFFER_SIZE];
}

CarlaEngineAudioPort::~CarlaEngineAudioPort()
{
    carla_debug("CarlaEngineAudioPort::~CarlaEngineAudioPort()");

    if (kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        CARLA_ASSERT(fBuffer != nullptr);

        if (fBuffer != nullptr)
            delete[] fBuffer;
    }
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const)
{
    if (kProcessMode == PROCESS_MODE_PATCHBAY && ! kIsInput)
        carla_zeroFloat(fBuffer, PATCHBAY_BUFFER_SIZE);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Event port

static const EngineEvent kFallbackEngineEvent;

CarlaEngineEventPort::CarlaEngineEventPort(const bool isInput, const ProcessMode processMode)
    : CarlaEnginePort(isInput, processMode),
      kMaxEventCount(processMode == PROCESS_MODE_CONTINUOUS_RACK ? RACK_EVENT_COUNT : PATCHBAY_EVENT_COUNT),
      fBuffer(nullptr)
{
    carla_debug("CarlaEngineEventPort::CarlaEngineEventPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

    if (kProcessMode == PROCESS_MODE_PATCHBAY)
        fBuffer = new EngineEvent[PATCHBAY_EVENT_COUNT];
}

CarlaEngineEventPort::~CarlaEngineEventPort()
{
    carla_debug("CarlaEngineEventPort::~CarlaEngineEventPort()");

    if (kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        CARLA_ASSERT(fBuffer != nullptr);

        if (fBuffer != nullptr)
            delete[] fBuffer;
    }
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
    CARLA_ASSERT(kIsInput);
    CARLA_ASSERT(fBuffer != nullptr);
    CARLA_ASSERT(kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY);

    if (! kIsInput)
        return 0;
    if (fBuffer == nullptr)
        return 0;
    if (kProcessMode != PROCESS_MODE_CONTINUOUS_RACK && kProcessMode != PROCESS_MODE_PATCHBAY)
        return 0;

    uint32_t count = 0;
    const EngineEvent* const events = fBuffer;

    for (uint32_t i=0; i < kMaxEventCount; i++, count++)
    {
        if (events[i].type == kEngineEventTypeNull)
            break;
    }

    return count;
}

const EngineEvent& CarlaEngineEventPort::getEvent(const uint32_t index)
{
    CARLA_ASSERT(kIsInput);
    CARLA_ASSERT(fBuffer != nullptr);
    CARLA_ASSERT(kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY);
    CARLA_ASSERT(index < kMaxEventCount);

    if (! kIsInput)
        return kFallbackEngineEvent;
    if (fBuffer == nullptr)
        return kFallbackEngineEvent;
    if (kProcessMode != PROCESS_MODE_CONTINUOUS_RACK && kProcessMode != PROCESS_MODE_PATCHBAY)
        return kFallbackEngineEvent;
    if (index >= kMaxEventCount)
        return kFallbackEngineEvent;

    return fBuffer[index];
}

void CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const double value)
{
    CARLA_ASSERT(! kIsInput);
    CARLA_ASSERT(fBuffer != nullptr);
    CARLA_ASSERT(kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY);
    CARLA_ASSERT(type != kEngineControlEventTypeNull);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_SAFE_ASSERT(value >= 0.0 && value <= 1.0);

    if (kIsInput)
        return;
    if (fBuffer == nullptr)
        return;
    if (kProcessMode != PROCESS_MODE_CONTINUOUS_RACK && kProcessMode != PROCESS_MODE_PATCHBAY)
        return;
    if (type == kEngineControlEventTypeNull)
        return;
    if (channel >= MAX_MIDI_CHANNELS)
        return;
    if (type == kEngineControlEventTypeParameter)
    {
        CARLA_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
    }

    for (uint32_t i=0; i < kMaxEventCount; i++)
    {
        if (fBuffer[i].type != kEngineEventTypeNull)
            continue;

        fBuffer[i].type    = kEngineEventTypeControl;
        fBuffer[i].time    = time;
        fBuffer[i].channel = channel;

        fBuffer[i].ctrl.type  = type;
        fBuffer[i].ctrl.param = param;
        fBuffer[i].ctrl.value = carla_fixValue<double>(0.0, 1.0, value);

        return;
    }

    carla_stderr2("CarlaEngineEventPort::writeControlEvent() - buffer full");
}

void CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t* const data, const uint8_t size)
{
    CARLA_ASSERT(! kIsInput);
    CARLA_ASSERT(fBuffer != nullptr);
    CARLA_ASSERT(kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_PATCHBAY);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(data != nullptr);
    CARLA_ASSERT(size > 0);

    if (kIsInput)
        return;
    if (fBuffer == nullptr)
        return;
    if (kProcessMode != PROCESS_MODE_CONTINUOUS_RACK && kProcessMode != PROCESS_MODE_PATCHBAY)
        return;
    if (channel >= MAX_MIDI_CHANNELS)
        return;
    if (data == nullptr)
        return;
    if (size == 0)
        return;
    if (size > 3)
        return;

    for (uint32_t i=0; i < kMaxEventCount; i++)
    {
        if (fBuffer[i].type != kEngineEventTypeNull)
            continue;

        fBuffer[i].type    = kEngineEventTypeMidi;
        fBuffer[i].time    = time;
        fBuffer[i].channel = channel;

        fBuffer[i].midi.port    = port;
        fBuffer[i].midi.data[0] = data[0];
        fBuffer[i].midi.data[1] = data[1];
        fBuffer[i].midi.data[2] = data[2];
        fBuffer[i].midi.size    = size;

        return;
    }

    carla_stderr2("CarlaEngineEventPort::writeMidiEvent() - buffer full");
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine client (Abstract)

CarlaEngineClient::CarlaEngineClient(const EngineType engineType, const ProcessMode processMode)
    : kEngineType(engineType),
      kProcessMode(processMode),
      fActive(false),
      fLatency(0)
{
    carla_debug("CarlaEngineClient::CarlaEngineClient(%s, %s)", EngineType2Str(engineType), ProcessMode2Str(processMode));
    CARLA_ASSERT(engineType != kEngineTypeNull);
}

CarlaEngineClient::~CarlaEngineClient()
{
    carla_debug("CarlaEngineClient::~CarlaEngineClient()");
    CARLA_ASSERT(! fActive);
}

void CarlaEngineClient::activate()
{
    carla_debug("CarlaEngineClient::activate()");
    CARLA_ASSERT(! fActive);

    fActive = true;
}

void CarlaEngineClient::deactivate()
{
    carla_debug("CarlaEngineClient::deactivate()");
    CARLA_ASSERT(fActive);

    fActive = false;
}

bool CarlaEngineClient::isActive() const
{
    carla_debug("CarlaEngineClient::isActive()");

    return fActive;
}

bool CarlaEngineClient::isOk() const
{
    carla_debug("CarlaEngineClient::isOk()");

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

CarlaEnginePort* CarlaEngineClient::addPort(const EnginePortType portType, const char* const name, const bool isInput)
{
    carla_debug("CarlaEngineClient::addPort(%s, \"%s\", %s)", EnginePortType2Str(portType), name, bool2str(isInput));

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio:
        return new CarlaEngineAudioPort(isInput, kProcessMode);
    case kEnginePortTypeEvent:
        return new CarlaEngineEventPort(isInput, kProcessMode);
    }

    carla_stderr("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine()
    : fBufferSize(0),
      fSampleRate(0.0),
      kData(new CarlaEngineProtectedData(this))
{
    carla_debug("CarlaEngine::CarlaEngine()");
}

CarlaEngine::~CarlaEngine()
{
    carla_debug("CarlaEngine::~CarlaEngine()");

    delete kData;
}

// -----------------------------------------------------------------------
// Helpers

void doPluginRemove(CarlaEngineProtectedData* const kData, const bool unlock)
{
    CARLA_ASSERT(kData->curPluginCount > 0);
    kData->curPluginCount--;

    const unsigned int id = kData->nextAction.pluginId;

    // reset current plugin
    kData->plugins[id].plugin = nullptr;

    CarlaPlugin* plugin;

    // move all plugins 1 spot backwards
    for (unsigned int i=id; i < kData->curPluginCount; i++)
    {
        plugin = kData->plugins[i+1].plugin;

        CARLA_ASSERT(plugin);

        if (plugin == nullptr)
            break;

        plugin->setId(i);

        kData->plugins[i].plugin      = plugin;
        kData->plugins[i].insPeak[0]  = 0.0f;
        kData->plugins[i].insPeak[1]  = 0.0f;
        kData->plugins[i].outsPeak[0] = 0.0f;
        kData->plugins[i].outsPeak[1] = 0.0f;
    }

    kData->nextAction.opcode = EnginePostActionNull;

    if (unlock)
        kData->nextAction.mutex.unlock();
}

const char* findDSSIGUI(const char* const filename, const char* const label)
{
    QString guiFilename;
    guiFilename.clear();

    QString pluginDir(filename);
    pluginDir.resize(pluginDir.lastIndexOf("."));

    QString shortName = QFileInfo(pluginDir).baseName();

    QString checkLabel = QString(label);
    QString checkSName = shortName;

    if (! checkLabel.endsWith("_")) checkLabel += "_";
    if (! checkSName.endsWith("_")) checkSName += "_";

    QStringList guiFiles = QDir(pluginDir).entryList();

    foreach (const QString& gui, guiFiles)
    {
        if (gui.startsWith(checkLabel) || gui.startsWith(checkSName))
        {
            QFileInfo finalname(pluginDir + QDir::separator() + gui);
            guiFilename = finalname.absoluteFilePath();
            break;
        }
    }

    if (guiFilename.isEmpty())
        return nullptr;

    return carla_strdup(guiFilename.toUtf8().constData());
}

// -----------------------------------------------------------------------
// Static values and calls

unsigned int CarlaEngine::getDriverCount()
{
    carla_debug("CarlaEngine::getDriverCount()");

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
    carla_debug("CarlaEngine::getDriverName(%i)", index);

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

    carla_stderr("CarlaEngine::getDriverName(%i) - invalid index", index);
    return nullptr;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
    carla_debug("CarlaEngine::newDriverByName(\"%s\")", driverName);

#ifdef WANT_JACK
    if (std::strcmp(driverName, "JACK") == 0)
        return newJack();
#endif

#ifdef WANT_RTAUDIO
# ifdef __LINUX_ALSA__
    if (std::strcmp(driverName, "ALSA") == 0)
        return newRtAudio(RTAUDIO_LINUX_ALSA);
# endif
# ifdef __LINUX_PULSE__
    if (std::strcmp(driverName, "PulseAudio") == 0)
        return newRtAudio(RTAUDIO_LINUX_PULSE);
# endif
# ifdef __LINUX_OSS__
    if (std::strcmp(driverName, "OSS") == 0)
        return newRtAudio(RTAUDIO_LINUX_OSS);
# endif
# ifdef __UNIX_JACK__
    if (std::strncmp(driverName, "JACK ", 5) == 0)
        return newRtAudio(RTAUDIO_UNIX_JACK);
# endif
# ifdef __MACOSX_CORE__
    if (std::strcmp(driverName, "CoreAudio") == 0)
        return newRtAudio(RTAUDIO_MACOSX_CORE);
# endif
# ifdef __WINDOWS_ASIO__
    if (std::strcmp(driverName, "ASIO") == 0)
        return newRtAudio(RTAUDIO_WINDOWS_ASIO);
# endif
# ifdef __WINDOWS_DS__
    if (std::strcmp(driverName, "DirectSound") == 0)
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
    return kData->curPluginCount;
}

unsigned int CarlaEngine::maxPluginNumber() const
{
    return kData->maxPluginNumber;
}

// -----------------------------------------------------------------------
// Virtual, per-engine type calls

bool CarlaEngine::init(const char* const clientName)
{
    carla_debug("CarlaEngine::init(\"%s\")", clientName);
    CARLA_ASSERT(kData->plugins == nullptr);

    fName = clientName;
    fName.toBasic();

    fTimeInfo.clear();

    kData->aboutToClose = false;
    kData->curPluginCount = 0;

    switch (fOptions.processMode)
    {
    case PROCESS_MODE_CONTINUOUS_RACK:
        kData->maxPluginNumber = MAX_RACK_PLUGINS;
        kData->rack.in  = new EngineEvent[RACK_EVENT_COUNT];
        kData->rack.out = new EngineEvent[RACK_EVENT_COUNT];
        break;
    case PROCESS_MODE_PATCHBAY:
        kData->maxPluginNumber = MAX_PATCHBAY_PLUGINS;
        break;
    case PROCESS_MODE_BRIDGE:
        kData->maxPluginNumber = 1;
        break;
    default:
        kData->maxPluginNumber = MAX_DEFAULT_PLUGINS;
        break;
    }

    //kData->pluginsPool.resize(maxPluginNumber, 999);
    kData->plugins = new EnginePluginData[kData->maxPluginNumber];

    kData->osc.init(clientName);

#ifndef BUILD_BRIDGE
    kData->oscData = kData->osc.getControlData();
#else
    kData->oscData = nullptr; // set later in setOscBridgeData()
#endif

#ifndef BUILD_BRIDGE
    //if (std::strcmp(clientName, "Carla") != 0)
    carla_setprocname(clientName);
#endif

    kData->nextAction.ready();
    kData->thread.startNow();

    return true;
}

bool CarlaEngine::close()
{
    carla_debug("CarlaEngine::close()");
    CARLA_ASSERT(kData->plugins != nullptr);

    kData->nextAction.ready();
    kData->thread.stopNow();

#ifndef BUILD_BRIDGE
    osc_send_control_exit();
#endif
    kData->osc.close();

    kData->oscData = nullptr;

    kData->aboutToClose = true;
    kData->curPluginCount = 0;
    kData->maxPluginNumber = 0;

    //kData->plugins.clear();
    if (kData->plugins != nullptr)
    {
        delete[] kData->plugins;
        kData->plugins = nullptr;
    }

    if (kData->rack.in != nullptr)
    {
        delete[] kData->rack.in;
        kData->rack.in = nullptr;
    }

    if (kData->rack.out != nullptr)
    {
        delete[] kData->rack.out;
        kData->rack.out = nullptr;
    }

    fName.clear();

    return true;
}

void CarlaEngine::idle()
{
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(isRunning());

    for (unsigned int i=0; i < kData->curPluginCount; i++)
    {
        CarlaPlugin* const plugin = kData->plugins[i].plugin;

        if (plugin != nullptr && plugin->enabled())
            plugin->idleGui();
    }
}

// -----------------------------------------------------------------------
// Virtual, per-engine type calls

CarlaEngineClient* CarlaEngine::addClient(CarlaPlugin* const)
{
    return new CarlaEngineClient(type(), fOptions.processMode);
}

// -----------------------------------------------------------------------
// Plugin management

bool CarlaEngine::addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, const void* const extra)
{
    carla_debug("CarlaEngine::addPlugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", BinaryType2Str(btype), PluginType2Str(ptype), filename, name, label, extra);
    CARLA_ASSERT(btype != BINARY_NONE);
    CARLA_ASSERT(ptype != PLUGIN_NONE);

    if (kData->curPluginCount == kData->maxPluginNumber)
    {
        setLastError("Maximum number of plugins reached");
        return false;
    }

    const unsigned int id = kData->curPluginCount;

    CarlaPlugin::Initializer init = {
        this,
        id,
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
        setLastError("Bridged plugins are not implemented yet");
    }
    else
#endif // BUILD_BRIDGE
    {
        switch (ptype)
        {
        case PLUGIN_NONE:
            break;

        case PLUGIN_INTERNAL:
            plugin = CarlaPlugin::newNative(init);
            break;

        case PLUGIN_LADSPA:
            plugin = CarlaPlugin::newLADSPA(init, (const LADSPA_RDF_Descriptor*)extra);
            break;

        case PLUGIN_DSSI:
            plugin = CarlaPlugin::newDSSI(init, (const char*)extra);
            break;

        case PLUGIN_LV2:
            plugin = CarlaPlugin::newLV2(init);
            break;

        case PLUGIN_VST:
            plugin = CarlaPlugin::newVST(init);
            break;

        case PLUGIN_VST3:
            plugin = CarlaPlugin::newVST3(init);
            break;

        case PLUGIN_GIG:
            plugin = CarlaPlugin::newGIG(init, (extra != nullptr));
            break;

        case PLUGIN_SF2:
            plugin = CarlaPlugin::newSF2(init, (extra != nullptr));
            break;

        case PLUGIN_SFZ:
            plugin = CarlaPlugin::newSFZ(init, (extra != nullptr));
            break;
        }
    }

    if (plugin == nullptr)
        return false;

    plugin->registerToOscClient();

    kData->plugins[id].plugin      = plugin;
    kData->plugins[id].insPeak[0]  = 0.0f;
    kData->plugins[id].insPeak[1]  = 0.0f;
    kData->plugins[id].outsPeak[0] = 0.0f;
    kData->plugins[id].outsPeak[1] = 0.0f;

    kData->curPluginCount += 1;

    // FIXME
    callback(CALLBACK_PLUGIN_ADDED, id, 0, 0, 0.0f, nullptr);

    return true;
}

bool CarlaEngine::removePlugin(const unsigned int id)
{
    carla_debug("CarlaEngine::removePlugin(%i)", id);
    CARLA_ASSERT(kData->curPluginCount > 0);
    CARLA_ASSERT(id < kData->curPluginCount);
    CARLA_ASSERT(kData->plugins != nullptr);

    if (kData->plugins == nullptr)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return false;
    }

    CarlaPlugin* const plugin = kData->plugins[id].plugin;

    CARLA_ASSERT(plugin != nullptr);

    if (plugin != nullptr)
    {
        CARLA_ASSERT(plugin->id() == id);

        kData->thread.stopNow();

        kData->nextAction.pluginId = id;
        kData->nextAction.opcode   = EnginePostActionRemovePlugin;

        kData->nextAction.mutex.lock();

        if (isRunning())
        {
            carla_stderr("CarlaEngine::removePlugin(%i) - remove blocking START", id);
            // block wait for unlock on proccessing side
            kData->nextAction.mutex.lock();
            carla_stderr("CarlaEngine::removePlugin(%i) - remove blocking DONE", id);
        }
        else
        {
            doPluginRemove(kData, false);
        }

#ifndef BUILD_BRIDGE
        if (isOscControlRegistered())
            osc_send_control_remove_plugin(id);
#endif

        delete plugin;

        kData->nextAction.mutex.unlock();

        if (isRunning() && ! kData->aboutToClose)
            kData->thread.startNow();

        // FIXME
        callback(CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0.0f, nullptr);

        return true;
    }

    carla_stderr("CarlaEngine::removePlugin(%i) - could not find plugin", id);
    setLastError("Could not find plugin to remove");
    return false;
}

void CarlaEngine::removeAllPlugins()
{
    carla_debug("CarlaEngine::removeAllPlugins() - START");

    kData->thread.stopNow();

    if (kData->curPluginCount > 0)
    {
        const unsigned int oldCount = kData->curPluginCount;

        kData->curPluginCount = 0;

        for (unsigned int i=0; i < oldCount; i++)
        {
            CarlaPlugin* const plugin = kData->plugins[i].plugin;

            CARLA_ASSERT(plugin != nullptr);

            kData->plugins[i].plugin = nullptr;

            if (plugin != nullptr)
                delete plugin;

            // clear this plugin
            kData->plugins[i].insPeak[0]  = 0.0f;
            kData->plugins[i].insPeak[1]  = 0.0f;
            kData->plugins[i].outsPeak[0] = 0.0f;
            kData->plugins[i].outsPeak[1] = 0.0f;
        }
    }

    if (isRunning() && ! kData->aboutToClose)
        kData->thread.startNow();

    carla_debug("CarlaEngine::removeAllPlugins() - END");
}

CarlaPlugin* CarlaEngine::getPlugin(const unsigned int id) const
{
    carla_debug("CarlaEngine::getPlugin(%i) [count:%i]", id, kData->curPluginCount);
    CARLA_ASSERT(kData->curPluginCount > 0);
    CARLA_ASSERT(id < kData->curPluginCount);
    CARLA_ASSERT(kData->plugins != nullptr);

    if (id < kData->curPluginCount && kData->plugins != nullptr)
        return kData->plugins[id].plugin;

    return nullptr;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const unsigned int id) const
{
    return kData->plugins[id].plugin;
}

const char* CarlaEngine::getNewUniquePluginName(const char* const name)
{
    carla_debug("CarlaEngine::getNewUniquePluginName(\"%s\")", name);
    CARLA_ASSERT(kData->maxPluginNumber > 0);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(name != nullptr);

    static CarlaString sname;
    sname = name;

    if (sname.isEmpty() || kData->plugins == nullptr)
    {
        sname = "(No name)";
        return (const char*)sname;
    }

    sname.truncate(maxClientNameSize()-5-1); // 5 = strlen(" (10)")
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (unsigned short i=0; i < kData->curPluginCount; i++)
    {
        CARLA_ASSERT(kData->plugins[i].plugin);

        if (kData->plugins[i].plugin == nullptr)
            continue;

        // Check if unique name doesn't exist
        if (const char* const pluginName = kData->plugins[i].plugin->name())
        {
            if (sname != pluginName)
                continue;
        }

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
                    n2  = '0';
                    n3 += 1;
                }
                else
                    n2 += 1;

                sname[len-2] = n2;
                sname[len-3] = n3;

                continue;
            }
        }

        // Modify string if not
        sname += " (2)";
    }

    return (const char*)sname;
}

#if 0
void CarlaEngine::__bridgePluginRegister(const unsigned short id, CarlaPlugin* const plugin)
{
    data->carlaPlugins[id] = plugin;
}
#endif

// -----------------------------------------------------------------------
// Information (base)

bool CarlaEngine::loadProject(const char* const filename)
{
    carla_debug("CarlaEngine::loadProject(\"%s\")", filename);
    CARLA_ASSERT(filename != nullptr);

    QFile file(filename);

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QDomDocument xml;
    xml.setContent(file.readAll());
    file.close();

    QDomNode xmlNode(xml.documentElement());

    if (xmlNode.toElement().tagName() != "CARLA-PROJECT")
    {
        carla_stderr2("Not a valid Carla project file");
        return false;
    }

    QDomNode node(xmlNode.firstChild());

    while (! node.isNull())
    {
        if (node.toElement().tagName() == "Plugin")
        {
            const SaveState& saveState = getSaveStateDictFromXML(node);
            CARLA_ASSERT(saveState.type != nullptr);

            if (saveState.type == nullptr)
                continue;

            const void* extraStuff = nullptr;

            if (std::strcmp(saveState.type, "DSSI") == 0)
                extraStuff = findDSSIGUI(saveState.binary, saveState.label);

            // TODO - proper find&load plugins
            if (addPlugin(getPluginTypeFromString(saveState.type), saveState.binary, saveState.name, saveState.label, extraStuff))
            {
                if (CarlaPlugin* plugin = getPlugin(kData->curPluginCount-1))
                    plugin->loadSaveState(saveState);
            }
        }
        node = node.nextSibling();
    }

    return true;
}

bool CarlaEngine::saveProject(const char* const filename)
{
    carla_debug("CarlaEngine::saveProject(\"%s\")", filename);
    CARLA_ASSERT(filename != nullptr);

    QFile file(filename);

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PROJECT>\n";
    out << "<CARLA-PROJECT VERSION='1.0'>\n";

    bool firstPlugin = true;
    char strBuf[STR_MAX];

    for (unsigned int i=0; i < kData->curPluginCount; i++)
    {
        CarlaPlugin* const plugin = kData->plugins[i].plugin;

        if (plugin != nullptr && plugin->enabled())
        {
            if (! firstPlugin)
                out << "\n";

            plugin->getRealName(strBuf);

            if (*strBuf != 0)
                out << QString(" <!-- %1 -->\n").arg(xmlSafeString(strBuf, true));

            out << " <Plugin>\n";
            out << getXMLFromSaveState(plugin->getSaveState());
            out << " </Plugin>\n";

            firstPlugin = false;
        }
    }

    out << "</CARLA-PROJECT>\n";

    file.close();
    return true;
}

// -----------------------------------------------------------------------
// Information (peaks)

float CarlaEngine::getInputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < kData->curPluginCount);
    CARLA_ASSERT(id-1 < MAX_PEAKS);

    if (id == 0 || id > MAX_PEAKS)
        return 0.0f;

    return kData->plugins[pluginId].insPeak[id-1];
}

float CarlaEngine::getOutputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < kData->curPluginCount);
    CARLA_ASSERT(id-1 < MAX_PEAKS);

    if (id == 0 || id > MAX_PEAKS)
        return 0.0f;

    return kData->plugins[pluginId].outsPeak[id-1];
}

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const CallbackType action, const unsigned int pluginId, const int value1, const int value2, const float value3, const char* const valueStr)
{
    carla_debug("CarlaEngine::callback(%s, %i, %i, %i, %f, \"%s\")", CallbackType2Str(action), pluginId, value1, value2, value3, valueStr);

    if (kData->callback)
        kData->callback(kData->callbackPtr, action, pluginId, value1, value2, value3, valueStr);
}

void CarlaEngine::setCallback(const CallbackFunc func, void* const ptr)
{
    carla_debug("CarlaEngine::setCallback(%p, %p)", func, ptr);
    CARLA_ASSERT(func);

    kData->callback    = func;
    kData->callbackPtr = ptr;
}

// -----------------------------------------------------------------------
// Patchbay

void CarlaEngine::patchbayConnect(int, int)
{
    // nothing
}

void CarlaEngine::patchbayDisconnect(int)
{
    // nothing
}

// -----------------------------------------------------------------------
// Transport

void CarlaEngine::transportPlay()
{
    kData->time.playing = true;
}

void CarlaEngine::transportPause()
{
    kData->time.playing = false;
}

void CarlaEngine::transportRelocate(const uint32_t frame)
{
    kData->time.frame = frame;
}

// -----------------------------------------------------------------------
// Error handling

const char* CarlaEngine::getLastError() const
{
    return (const char*)kData->lastError;
}

void CarlaEngine::setLastError(const char* const error)
{
    kData->lastError = error;
}

void CarlaEngine::setAboutToClose()
{
    carla_debug("CarlaEngine::setAboutToClose()");
    kData->aboutToClose = true;
}

// -----------------------------------------------------------------------
// Global options

#ifndef BUILD_BRIDGE

const QProcessEnvironment& CarlaEngine::getOptionsAsProcessEnvironment() const
{
    return kData->procEnv;
}

#define CARLA_ENGINE_SET_OPTION_RUNNING_CHECK \
    if (isRunning()) \
        return carla_stderr("CarlaEngine::setOption(%s, %i, \"%s\") - Cannot set this option while engine is running!", OptionsType2Str(option), value, valueStr);

void CarlaEngine::setOption(const OptionsType option, const int value, const char* const valueStr)
{
    carla_debug("CarlaEngine::setOption(%s, %i, \"%s\")", OptionsType2Str(option), value, valueStr);

    switch (option)
    {
    case OPTION_PROCESS_NAME:
        carla_setprocname(valueStr);
        break;

    case OPTION_PROCESS_MODE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK

        if (value < PROCESS_MODE_SINGLE_CLIENT || value > PROCESS_MODE_PATCHBAY)
            return carla_stderr("CarlaEngine::setOption(%s, %i, \"%s\") - invalid value", OptionsType2Str(option), value, valueStr);

        fOptions.processMode = static_cast<ProcessMode>(value);
        break;

    case OPTION_TRANSPORT_MODE:
        // FIXME: Always enable JACK transport for now
#if 0
        if (value < CarlaBackend::TRANSPORT_MODE_INTERNAL || value > CarlaBackend::TRANSPORT_MODE_JACK)
            return carla_stderr2("carla_set_engine_option(OPTION_TRANSPORT_MODE, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.transportMode = static_cast<CarlaBackend::TransportMode>(value);
#endif
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
    return (kData->oscData != nullptr);
}
#else
bool CarlaEngine::isOscControlRegistered() const
{
    return kData->osc.isControlRegistered();
}
#endif

void CarlaEngine::idleOsc()
{
    kData->osc.idle();
}

const char* CarlaEngine::getOscServerPathTCP() const
{
    return kData->osc.getServerPathTCP();
}

const char* CarlaEngine::getOscServerPathUDP() const
{
    return kData->osc.getServerPathUDP();
}

#ifdef BUILD_BRIDGE
void CarlaEngine::setOscBridgeData(const CarlaOscData* const oscData)
{
    kData->oscData = oscData;
}
#endif

// -----------------------------------------------------------------------
// protected calls

void CarlaEngine::bufferSizeChanged(const uint32_t newBufferSize)
{
    carla_debug("CarlaEngine::bufferSizeChanged(%i)", newBufferSize);

    for (unsigned int i=0; i < kData->curPluginCount; i++)
    {
        CarlaPlugin* const plugin = kData->plugins[i].plugin;

        if (plugin != nullptr && plugin->enabled())
            plugin->bufferSizeChanged(newBufferSize);
    }
}

void CarlaEngine::sampleRateChanged(const double newSampleRate)
{
    carla_debug("CarlaEngine::sampleRateChanged(%g)", newSampleRate);

    for (unsigned int i=0; i < kData->curPluginCount; i++)
    {
        CarlaPlugin* const plugin = kData->plugins[i].plugin;

        if (plugin != nullptr && plugin->enabled())
            plugin->sampleRateChanged(newSampleRate);
    }
}

void CarlaEngine::proccessPendingEvents()
{
    //carla_stderr("proccessPendingEvents(%i)", kData->nextAction.opcode);

    switch (kData->nextAction.opcode)
    {
    case EnginePostActionNull:
        break;
    case EnginePostActionRemovePlugin:
        doPluginRemove(kData, true);
        break;
    }

    if (kData->time.playing)
        kData->time.frame += fBufferSize;

    if (fOptions.transportMode == CarlaBackend::TRANSPORT_MODE_INTERNAL)
    {
        fTimeInfo.playing = kData->time.playing;
        fTimeInfo.frame   = kData->time.frame;
    }

    for (unsigned int i=0; i < kData->curPluginCount; i++)
    {
        // TODO - peak values?
    }
}

void CarlaEngine::setPeaks(const unsigned int pluginId, float const inPeaks[MAX_PEAKS], float const outPeaks[MAX_PEAKS])
{
    kData->plugins[pluginId].insPeak[0]  = inPeaks[0];
    kData->plugins[pluginId].insPeak[1]  = inPeaks[1];
    kData->plugins[pluginId].outsPeak[0] = outPeaks[0];
    kData->plugins[pluginId].outsPeak[1] = outPeaks[1];
}

#ifndef BUILD_BRIDGE
EngineEvent* CarlaEngine::getRackEventBuffer(const bool isInput)
{
    return isInput ? kData->rack.in : kData->rack.out;
}

void setValueIfHigher(float& value, const float& compare)
{
    if (value < compare)
        value = compare;
}

void CarlaEngine::processRack(float* inBuf[2], float* outBuf[2], const uint32_t frames)
{
    // initialize outputs (zero)
    carla_zeroFloat(outBuf[0], frames);
    carla_zeroFloat(outBuf[1], frames);
    carla_zeroMem(kData->rack.out, sizeof(EngineEvent)*RACK_EVENT_COUNT);

    bool processed = false;

    // process plugins
    for (unsigned int i=0; i < kData->curPluginCount; i++)
    {
        CarlaPlugin* const plugin = kData->plugins[i].plugin;

        if (plugin == nullptr || ! plugin->enabled() || ! plugin->tryLock())
            continue;

        if (processed)
        {
            // initialize inputs (from previous outputs)
            carla_copyFloat(inBuf[0], outBuf[0], frames);
            carla_copyFloat(inBuf[1], outBuf[1], frames);
            std::memcpy(kData->rack.in, kData->rack.out, sizeof(EngineEvent)*RACK_EVENT_COUNT);

            // initialize outputs (zero)
            carla_zeroFloat(outBuf[0], frames);
            carla_zeroFloat(outBuf[1], frames);
            carla_zeroMem(kData->rack.out, sizeof(EngineEvent)*RACK_EVENT_COUNT);
        }

        // process
        plugin->initBuffers();
        plugin->process(inBuf, outBuf, frames);
        plugin->unlock();

#if 0
        // if plugin has no audio inputs, add previous buffers
        if (plugin->audioInCount() == 0)
        {
            for (uint32_t j=0; j < frames; j++)
            {
                outBuf[0][j] += inBuf[0][j];
                outBuf[1][j] += inBuf[1][j];
            }
        }
        // if plugin has no midi output, add previous events
        if (plugin->midiOutCount() == 0)
        {
            for (uint32_t j=0, k=0; j < frames; j++)
            {

            }
            std::memcpy(kData->rack.out, kData->rack.in, sizeof(EngineEvent)*RACK_EVENT_COUNT);
        }
#endif

        // set peaks
        {
            float inPeak1  = 0.0f;
            float inPeak2  = 0.0f;
            float outPeak1 = 0.0f;
            float outPeak2 = 0.0f;

            for (uint32_t k=0; k < frames; k++)
            {
                setValueIfHigher(inPeak1,  std::fabs(inBuf[0][k]));
                setValueIfHigher(inPeak2,  std::fabs(inBuf[1][k]));
                setValueIfHigher(outPeak1, std::fabs(outBuf[0][k]));
                setValueIfHigher(outPeak2, std::fabs(outBuf[1][k]));
            }

            kData->plugins[i].insPeak[0]  = inPeak1;
            kData->plugins[i].insPeak[1]  = inPeak2;
            kData->plugins[i].outsPeak[0] = outPeak1;
            kData->plugins[i].outsPeak[1] = outPeak2;
        }

        processed = true;
    }
}

void CarlaEngine::processPatchbay(float** inBuf, float** outBuf, const uint32_t bufCount[2], const uint32_t frames)
{
    // TODO
    return;

    // unused, for now
    (void)inBuf;
    (void)outBuf;
    (void)bufCount;
    (void)frames;
}
#endif

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine OSC stuff

#ifndef BUILD_BRIDGE
void CarlaEngine::osc_send_control_add_plugin_start(const int32_t pluginId, const char* const pluginName)
{
    carla_debug("CarlaEngine::osc_send_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);
    CARLA_ASSERT(pluginName);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+18];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/add_plugin_start");
        lo_send(kData->oscData->target, targetPath, "is", pluginId, pluginName);
    }
}

void CarlaEngine::osc_send_control_add_plugin_end(const int32_t pluginId)
{
    carla_debug("CarlaEngine::osc_send_control_add_plugin_end(%i)", pluginId);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+16];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/add_plugin_end");
        lo_send(kData->oscData->target, targetPath, "i", pluginId);
    }
}

void CarlaEngine::osc_send_control_remove_plugin(const int32_t pluginId)
{
    carla_debug("CarlaEngine::osc_send_control_remove_plugin(%i)", pluginId);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+15];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/remove_plugin");
        lo_send(kData->oscData->target, targetPath, "i", pluginId);
    }
}

void CarlaEngine::osc_send_control_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints, const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    carla_debug("CarlaEngine::osc_send_control_set_plugin_data(%i, %i, %i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);
    CARLA_ASSERT(type != PLUGIN_NONE);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+17];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_plugin_data");
        lo_send(kData->oscData->target, targetPath, "iiiissssh", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::osc_send_control_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts, const int32_t cIns, const int32_t cOuts, const int32_t cTotals)
{
    carla_debug("CarlaEngine::osc_send_control_set_plugin_ports(%i, %i, %i, %i, %i, %i, %i, %i)", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+18];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_plugin_ports");
        lo_send(kData->oscData->target, targetPath, "iiiiiiii", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    }
}

void CarlaEngine::osc_send_control_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints, const char* const name, const char* const label, const float current)
{
    carla_debug("CarlaEngine::osc_send_control_set_parameter_data(%i, %i, %i, %i, \"%s\", \"%s\", %g)", pluginId, index, type, hints, name, label, current);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(type != PARAMETER_UNKNOWN);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+20];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_data");
        lo_send(kData->oscData->target, targetPath, "iiiissd", pluginId, index, type, hints, name, label, current);
    }
}

void CarlaEngine::osc_send_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const float min, const float max, const float def, const float step, const float stepSmall, const float stepLarge)
{
    carla_debug("CarlaEngine::osc_send_control_set_parameter_ranges(%i, %i, %g, %g, %g, %g, %g, %g)", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(min < max);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+22];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_ranges");
        lo_send(kData->oscData->target, targetPath, "iidddddd", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc)
{
    carla_debug("CarlaEngine::osc_send_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);
    CARLA_ASSERT(index >= 0);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+23];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_midi_cc");
        lo_send(kData->oscData->target, targetPath, "iii", pluginId, index, cc);
    }
}

void CarlaEngine::osc_send_control_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel)
{
    carla_debug("CarlaEngine::osc_send_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(channel >= 0 && channel < 16);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+28];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_midi_channel");
        lo_send(kData->oscData->target, targetPath, "iii", pluginId, index, channel);
    }
}

void CarlaEngine::osc_send_control_set_parameter_value(const int32_t pluginId, const int32_t index, const float value)
{
#if DEBUG
    if (index < 0)
        carla_debug("CarlaEngine::osc_send_control_set_parameter_value(%i, %s, %g)", pluginId, InternalParametersIndex2Str((InternalParametersIndex)index), value);
    else
        carla_debug("CarlaEngine::osc_send_control_set_parameter_value(%i, %i, %g)", pluginId, index, value);
#endif
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->curPluginCount);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+21];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_value");
        lo_send(kData->oscData->target, targetPath, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_default_value(const int32_t pluginId, const int32_t index, const float value)
{
    carla_debug("CarlaEngine::osc_send_control_set_default_value(%i, %i, %g)", pluginId, index, value);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);
    CARLA_ASSERT(index >= 0);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+19];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_default_value");
        lo_send(kData->oscData->target, targetPath, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_program(const int32_t pluginId, const int32_t index)
{
    carla_debug("CarlaEngine::osc_send_control_set_program(%i, %i)", pluginId, index);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+13];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_program");
        lo_send(kData->oscData->target, targetPath, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_control_set_program_count(const int32_t pluginId, const int32_t count)
{
    carla_debug("CarlaEngine::osc_send_control_set_program_count(%i, %i)", pluginId, count);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);
    CARLA_ASSERT(count >= 0);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+19];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_program_count");
        lo_send(kData->oscData->target, targetPath, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_control_set_program_name(const int32_t pluginId, const int32_t index, const char* const name)
{
    carla_debug("CarlaEngine::osc_send_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(name);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+18];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_program_name");
        lo_send(kData->oscData->target, targetPath, "iis", pluginId, index, name);
    }
}

void CarlaEngine::osc_send_control_set_midi_program(const int32_t pluginId, const int32_t index)
{
    carla_debug("CarlaEngine::osc_send_control_set_midi_program(%i, %i)", pluginId, index);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+18];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_midi_program");
        lo_send(kData->oscData->target, targetPath, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_control_set_midi_program_count(const int32_t pluginId, const int32_t count)
{
    carla_debug("CarlaEngine::osc_send_control_set_midi_program_count(%i, %i)", pluginId, count);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);
    CARLA_ASSERT(count >= 0);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+24];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_midi_program_count");
        lo_send(kData->oscData->target, targetPath, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_control_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name)
{
    carla_debug("CarlaEngine::osc_send_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(bank >= 0);
    CARLA_ASSERT(program >= 0);
    CARLA_ASSERT(name);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+23];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_midi_program_data");
        lo_send(kData->oscData->target, targetPath, "iiiis", pluginId, index, bank, program, name);
    }
}

void CarlaEngine::osc_send_control_note_on(const int32_t pluginId, const int32_t channel, const int32_t note, const int32_t velo)
{
    carla_debug("CarlaEngine::osc_send_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    CARLA_ASSERT(note >= 0 && note < 128);
    CARLA_ASSERT(velo > 0 && velo < 128);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+9];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/note_on");
        lo_send(kData->oscData->target, targetPath, "iiii", pluginId, channel, note, velo);
    }
}

void CarlaEngine::osc_send_control_note_off(const int32_t pluginId, const int32_t channel, const int32_t note)
{
    carla_debug("CarlaEngine::osc_send_control_note_off(%i, %i, %i)", pluginId, channel, note);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    CARLA_ASSERT(note >= 0 && note < 128);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+10];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/note_off");
        lo_send(kData->oscData->target, targetPath, "iii", pluginId, channel, note);
    }
}

void CarlaEngine::osc_send_control_set_peaks(const int32_t pluginId)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < (int32_t)kData->maxPluginNumber);

    const EnginePluginData& pData = kData->plugins[pluginId];

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+22];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_peaks");
        lo_send(kData->oscData->target, targetPath, "iffff", pluginId, pData.insPeak[0], pData.insPeak[1], pData.outsPeak[0], pData.outsPeak[1]);
    }
}

void CarlaEngine::osc_send_control_exit()
{
    carla_debug("CarlaEngine::osc_send_control_exit()");
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData && kData->oscData->target)
    {
        char targetPath[std::strlen(kData->oscData->path)+6];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/exit");
        lo_send(kData->oscData->target, targetPath, "");
    }
}
#else
void CarlaEngine::osc_send_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    carla_debug("CarlaEngine::osc_send_bridge_audio_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+20];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_audio_count");
        lo_send(kData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    carla_debug("CarlaEngine::osc_send_bridge_midi_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+19];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_count");
        lo_send(kData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    carla_debug("CarlaEngine::osc_send_bridge_parameter_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+24];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_count");
        lo_send(kData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_program_count(const int32_t count)
{
    carla_debug("CarlaEngine::osc_send_bridge_program_count(%i)", count);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+22];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_program_count");
        lo_send(kData->oscData->target, targetPath, "i", count);
    }
}

void CarlaEngine::osc_send_bridge_midi_program_count(const int32_t count)
{
    carla_debug("CarlaEngine::osc_send_bridge_midi_program_count(%i)", count);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+27];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_program_count");
        lo_send(kData->oscData->target, targetPath, "i", count);
    }
}

void CarlaEngine::osc_send_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    carla_debug("CarlaEngine::osc_send_bridge_plugin_info(%i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", category, hints, name, label, maker, copyright, uniqueId);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(label != nullptr);
    CARLA_ASSERT(maker != nullptr);
    CARLA_ASSERT(copyright != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+20];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_plugin_info");
        lo_send(kData->oscData->target, targetPath, "iissssh", category, hints, name, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::osc_send_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit)
{
    carla_debug("CarlaEngine::osc_send_bridge_parameter_info(%i, \"%s\", \"%s\")", index, name, unit);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(unit != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+23];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_info");
        lo_send(kData->oscData->target, targetPath, "iss", index, name, unit);
    }
}

void CarlaEngine::osc_send_bridge_parameter_data(const int32_t index, const int32_t type, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC)
{
    carla_debug("CarlaEngine::osc_send_bridge_parameter_data(%i, %i, %i, %i, %i, %i)", index, type, rindex, hints, midiChannel, midiCC);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+23];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_data");
        lo_send(kData->oscData->target, targetPath, "iiiiii", index, type, rindex, hints, midiChannel, midiCC);
    }
}

void CarlaEngine::osc_send_bridge_parameter_ranges(const int32_t index, const float def, const float min, const float max, const float step, const float stepSmall, const float stepLarge)
{
    carla_debug("CarlaEngine::osc_send_bridge_parameter_ranges(%i, %g, %g, %g, %g, %g, %g)", index, def, min, max, step, stepSmall, stepLarge);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+25];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_ranges");
        lo_send(kData->oscData->target, targetPath, "idddddd", index, def, min, max, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_bridge_program_info(const int32_t index, const char* const name)
{
    carla_debug("CarlaEngine::osc_send_bridge_program_info(%i, \"%s\")", index, name);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+21];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_program_info");
        lo_send(kData->oscData->target, targetPath, "is", index, name);
    }
}

void CarlaEngine::osc_send_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label)
{
    carla_debug("CarlaEngine::osc_send_bridge_midi_program_info(%i, %i, %i, \"%s\")", index, bank, program, label);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+26];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_program_info");
        lo_send(kData->oscData->target, targetPath, "iiis", index, bank, program, label);
    }
}

void CarlaEngine::osc_send_bridge_configure(const char* const key, const char* const value)
{
    carla_debug("CarlaEngine::osc_send_bridge_configure(\"%s\", \"%s\")", key, value);
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(key != nullptr);
    CARLA_ASSERT(value != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+18];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_configure");
        lo_send(kData->oscData->target, targetPath, "ss", key, value);
    }
}

void CarlaEngine::osc_send_bridge_set_parameter_value(const int32_t index, const float value)
{
    carla_debug("CarlaEngine::osc_send_bridge_set_parameter_value(%i, %g)", index, value);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+28];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_parameter_value");
        lo_send(kData->oscData->target, targetPath, "id", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_default_value(const int32_t index, const float value)
{
    carla_debug("CarlaEngine::osc_send_bridge_set_default_value(%i, %g)", index, value);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+26];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_default_value");
        lo_send(kData->oscData->target, targetPath, "id", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_program(const int32_t index)
{
    carla_debug("CarlaEngine::osc_send_bridge_set_program(%i)", index);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+20];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_program");
        lo_send(kData->oscData->target, targetPath, "i", index);
    }
}

void CarlaEngine::osc_send_bridge_set_midi_program(const int32_t index)
{
    carla_debug("CarlaEngine::osc_send_bridge_set_midi_program(%i)", index);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+25];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_midi_program");
        lo_send(kData->oscData->target, targetPath, "i", index);
    }
}

void CarlaEngine::osc_send_bridge_set_custom_data(const char* const type, const char* const key, const char* const value)
{
    carla_debug("CarlaEngine::osc_send_bridge_set_custom_data(\"%s\", \"%s\", \"%s\")", type, key, value);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+24];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_custom_data");
        lo_send(kData->oscData->target, targetPath, "sss", type, key, value);
    }
}

void CarlaEngine::osc_send_bridge_set_chunk_data(const char* const chunkFile)
{
    carla_debug("CarlaEngine::osc_send_bridge_set_chunk_data(\"%s\")", chunkFile);
    CARLA_ASSERT(kData->oscData != nullptr);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+23];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_chunk_data");
        lo_send(kData->oscData->target, targetPath, "s", chunkFile);
    }
}

void CarlaEngine::osc_send_bridge_set_peaks()
{
    CARLA_ASSERT(kData->oscData != nullptr);

    const EnginePluginData& pData = kData->plugins[0];

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+22];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_peaks");
        lo_send(kData->oscData->target, targetPath, "ffff", pData.insPeak[0], pData.insPeak[1], pData.outsPeak[0], pData.outsPeak[1]);
    }
}
#endif

CARLA_BACKEND_END_NAMESPACE
