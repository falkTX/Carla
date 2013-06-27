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

#include <cmath>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

// -------------------------------------------------------------------------------------------------------------------
// Register native engine plugin

#ifndef BUILD_BRIDGE
# include "CarlaNative.h"

void carla_register_native_plugin_carla()
{
    CARLA_BACKEND_USE_NAMESPACE
    CarlaEngine::registerNativePlugin();
}
#endif

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackEngineEvent;

#ifndef BUILD_BRIDGE
// -------------------------------------------------------------------------------------------------------------------
// Bridge Helper, defined in CarlaPlugin.cpp

extern BinaryType CarlaPluginGetBridgeBinaryType(CarlaPlugin* const plugin);

// -------------------------------------------------------------------------------------------------------------------
// Engine Helpers

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
        {
            delete[] fBuffer;
            fBuffer = nullptr;
        }
    }
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const)
{
    if (kProcessMode == PROCESS_MODE_PATCHBAY && ! kIsInput)
        carla_zeroFloat(fBuffer, PATCHBAY_BUFFER_SIZE);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine CV port

CarlaEngineCVPort::CarlaEngineCVPort(const bool isInput, const ProcessMode processMode, const uint32_t bufferSize)
    : CarlaEnginePort(isInput, processMode),
      fBuffer(new float[bufferSize]),
      fBufferSize(bufferSize)
{
    carla_debug("CarlaEngineCVPort::CarlaEngineCVPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));
}

CarlaEngineCVPort::~CarlaEngineCVPort()
{
    carla_debug("CarlaEngineCVPort::~CarlaEngineCVPort()");

    CARLA_ASSERT(fBuffer != nullptr);

    if (fBuffer != nullptr)
    {
        delete[] fBuffer;
        fBuffer = nullptr;
    }
}

void CarlaEngineCVPort::initBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(engine != nullptr && engine->getBufferSize() == fBufferSize);

    if (! kIsInput)
        carla_zeroFloat(fBuffer, fBufferSize);
}

void CarlaEngineCVPort::writeBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(! kIsInput);
    CARLA_ASSERT(engine != nullptr);

    if (kIsInput)
        return;
    if (engine == nullptr)
        return;

    CARLA_ASSERT(engine->getBufferSize() == fBufferSize);
}

void CarlaEngineCVPort::setBufferSize(const uint32_t bufferSize)
{
    CARLA_ASSERT(fBuffer != nullptr);

    if (fBufferSize == bufferSize)
        return;

    if (fBuffer != nullptr)
        delete[] fBuffer;

    fBuffer     = new float[bufferSize];
    fBufferSize = bufferSize;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Event port

CarlaEngineEventPort::CarlaEngineEventPort(const bool isInput, const ProcessMode processMode)
    : CarlaEnginePort(isInput, processMode),
      pBuffer(nullptr)
{
    carla_debug("CarlaEngineEventPort::CarlaEngineEventPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

    if (kProcessMode == PROCESS_MODE_PATCHBAY)
        pBuffer = new EngineEvent[INTERNAL_EVENT_COUNT];
}

CarlaEngineEventPort::~CarlaEngineEventPort()
{
    carla_debug("CarlaEngineEventPort::~CarlaEngineEventPort()");

    if (kProcessMode == PROCESS_MODE_PATCHBAY)
    {
        CARLA_ASSERT(pBuffer != nullptr);

        if (pBuffer != nullptr)
        {
            delete[] pBuffer;
            pBuffer = nullptr;
        }
    }
}

void CarlaEngineEventPort::initBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(engine != nullptr);

    if (engine == nullptr)
        return;

    if (kProcessMode == PROCESS_MODE_CONTINUOUS_RACK || kProcessMode == PROCESS_MODE_BRIDGE)
        pBuffer = engine->getInternalEventBuffer(kIsInput);

    else if (kProcessMode == PROCESS_MODE_PATCHBAY && ! kIsInput)
        carla_zeroStruct<EngineEvent>(pBuffer, INTERNAL_EVENT_COUNT);
}

uint32_t CarlaEngineEventPort::getEventCount() const
{
    CARLA_ASSERT(kIsInput);
    CARLA_ASSERT(pBuffer != nullptr);
    CARLA_ASSERT(kProcessMode != PROCESS_MODE_SINGLE_CLIENT && kProcessMode != PROCESS_MODE_MULTIPLE_CLIENTS);

    if (! kIsInput)
        return 0;
    if (pBuffer == nullptr)
        return 0;
    if (kProcessMode == PROCESS_MODE_SINGLE_CLIENT || kProcessMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        return 0;

    for (uint32_t i=0; i < INTERNAL_EVENT_COUNT; ++i)
    {
        if (pBuffer[i].type == kEngineEventTypeNull)
            return i;
    }

    // buffer full
    return INTERNAL_EVENT_COUNT;
}

const EngineEvent& CarlaEngineEventPort::getEvent(const uint32_t index)
{
    CARLA_ASSERT(kIsInput);
    CARLA_ASSERT(pBuffer != nullptr);
    CARLA_ASSERT(kProcessMode != PROCESS_MODE_SINGLE_CLIENT && kProcessMode != PROCESS_MODE_MULTIPLE_CLIENTS);
    CARLA_ASSERT(index < INTERNAL_EVENT_COUNT);

    if (! kIsInput)
        return kFallbackEngineEvent;
    if (pBuffer == nullptr)
        return kFallbackEngineEvent;
    if (kProcessMode == PROCESS_MODE_SINGLE_CLIENT || kProcessMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        return kFallbackEngineEvent;
    if (index >= INTERNAL_EVENT_COUNT)
        return kFallbackEngineEvent;

    return pBuffer[index];
}

void CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value)
{
    CARLA_ASSERT(! kIsInput);
    CARLA_ASSERT(pBuffer != nullptr);
    CARLA_ASSERT(kProcessMode != PROCESS_MODE_SINGLE_CLIENT && kProcessMode != PROCESS_MODE_MULTIPLE_CLIENTS);
    CARLA_ASSERT(type != kEngineControlEventTypeNull);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

    if (kIsInput)
        return;
    if (pBuffer == nullptr)
        return;
    if (kProcessMode == PROCESS_MODE_SINGLE_CLIENT || kProcessMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        return;
    if (type == kEngineControlEventTypeNull)
        return;
    if (channel >= MAX_MIDI_CHANNELS)
        return;
    if (type == kEngineControlEventTypeParameter)
    {
        CARLA_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
    }

    for (uint32_t i=0; i < INTERNAL_EVENT_COUNT; ++i)
    {
        if (pBuffer[i].type != kEngineEventTypeNull)
            continue;

        pBuffer[i].type    = kEngineEventTypeControl;
        pBuffer[i].time    = time;
        pBuffer[i].channel = channel;

        pBuffer[i].ctrl.type  = type;
        pBuffer[i].ctrl.param = param;
        pBuffer[i].ctrl.value = carla_fixValue<float>(0.0f, 1.0f, value);
        return;
    }

    carla_stderr2("CarlaEngineEventPort::writeControlEvent() - buffer full");
}

void CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t* const data, const uint8_t size)
{
    CARLA_ASSERT(! kIsInput);
    CARLA_ASSERT(pBuffer != nullptr);
    CARLA_ASSERT(kProcessMode != PROCESS_MODE_SINGLE_CLIENT && kProcessMode != PROCESS_MODE_MULTIPLE_CLIENTS);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(data != nullptr);
    CARLA_ASSERT(size != 0 && size <= 4);

    if (kIsInput)
        return;
    if (pBuffer == nullptr)
        return;
    if (kProcessMode == PROCESS_MODE_SINGLE_CLIENT || kProcessMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        return;
    if (channel >= MAX_MIDI_CHANNELS)
        return;
    if (data == nullptr)
        return;
    if (size == 0 || size > 4)
        return;

    for (uint32_t i=0; i < INTERNAL_EVENT_COUNT; ++i)
    {
        if (pBuffer[i].type != kEngineEventTypeNull)
            continue;

        pBuffer[i].type    = kEngineEventTypeMidi;
        pBuffer[i].time    = time;
        pBuffer[i].channel = channel;

        pBuffer[i].midi.port = port;
        pBuffer[i].midi.size = size;

        carla_copy<uint8_t>(pBuffer[i].midi.data, data, size);

        // strip MIDI channel from 1st byte
        pBuffer[i].midi.data[0] = MIDI_GET_STATUS_FROM_DATA(pBuffer[i].midi.data);
        return;
    }

    carla_stderr2("CarlaEngineEventPort::writeMidiEvent() - buffer full");
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine client (Abstract)

CarlaEngineClient::CarlaEngineClient(const CarlaEngine& engine)
    : kEngine(engine),
      fActive(false),
      fLatency(0)
{
    carla_debug("CarlaEngineClient::CarlaEngineClient(name:\"%s\")", engine.getName());
}

CarlaEngineClient::~CarlaEngineClient()
{
    CARLA_ASSERT(! fActive);
    carla_debug("CarlaEngineClient::~CarlaEngineClient()");
}

void CarlaEngineClient::activate()
{
    CARLA_ASSERT(! fActive);
    carla_debug("CarlaEngineClient::activate()");

    fActive = true;
}

void CarlaEngineClient::deactivate()
{
    CARLA_ASSERT(fActive);
    carla_debug("CarlaEngineClient::deactivate()");

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
        return new CarlaEngineAudioPort(isInput, kEngine.getProccessMode());
    case kEnginePortTypeCV:
        return new CarlaEngineCVPort(isInput, kEngine.getProccessMode(), kEngine.getBufferSize());
    case kEnginePortTypeEvent:
        return new CarlaEngineEventPort(isInput, kEngine.getProccessMode());
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

// returned value must be deleted
const char* findDSSIGUI(const char* const filename, const char* const label)
{
    QString guiFilename;
    QString pluginDir(filename);
    pluginDir.resize(pluginDir.lastIndexOf("."));

    QString shortName(QFileInfo(pluginDir).baseName());

    QString checkLabel(label);
    QString checkSName(shortName);

    if (! checkLabel.endsWith("_")) checkLabel += "_";
    if (! checkSName.endsWith("_")) checkSName += "_";

    QStringList guiFiles(QDir(pluginDir).entryList());

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

    unsigned int count = 1; // JACK

#ifdef WANT_RTAUDIO
    count += getRtAudioApiCount();
#endif

    return count;
}

const char* CarlaEngine::getDriverName(const unsigned int index)
{
    carla_debug("CarlaEngine::getDriverName(%i)", index);

    if (index == 0)
        return "JACK";

#ifdef WANT_RTAUDIO
    const unsigned int rtIndex(index-1);

    if (rtIndex < getRtAudioApiCount())
        return getRtAudioApiName(rtIndex);
#endif

    carla_stderr("CarlaEngine::getDriverName(%i) - invalid index", index);
    return nullptr;
}

const char** CarlaEngine::getDriverDeviceNames(const unsigned int index)
{
    carla_debug("CarlaEngine::getDriverDeviceNames(%i)", index);

    if (index == 0)
        return nullptr;

#ifdef WANT_RTAUDIO
    const unsigned int rtIndex(index-1);

    if (rtIndex < getRtAudioApiCount())
        return getRtAudioApiDeviceNames(rtIndex);
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i) - invalid index", index);
    return nullptr;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
    carla_debug("CarlaEngine::newDriverByName(\"%s\")", driverName);

    if (std::strcmp(driverName, "JACK") == 0)
        return newJack();

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

    carla_stderr("CarlaEngine::newDriverByName(\"%s\") - invalid driver name", driverName);
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
    CARLA_ASSERT(fName.isEmpty());
    CARLA_ASSERT(kData->oscData == nullptr);
    CARLA_ASSERT(kData->plugins == nullptr);
    CARLA_ASSERT(kData->bufEvents.in  == nullptr);
    CARLA_ASSERT(kData->bufEvents.out == nullptr);
    carla_debug("CarlaEngine::init(\"%s\")", clientName);

    kData->aboutToClose = false;
    kData->curPluginCount = 0;
    kData->maxPluginNumber = 0;
    kData->nextPluginId = 0;

    switch (fOptions.processMode)
    {
    case PROCESS_MODE_SINGLE_CLIENT:
    case PROCESS_MODE_MULTIPLE_CLIENTS:
        kData->maxPluginNumber = MAX_DEFAULT_PLUGINS;
        break;

    case PROCESS_MODE_CONTINUOUS_RACK:
        kData->maxPluginNumber = MAX_RACK_PLUGINS;
        kData->bufEvents.in  = new EngineEvent[INTERNAL_EVENT_COUNT];
        kData->bufEvents.out = new EngineEvent[INTERNAL_EVENT_COUNT];
        break;

    case PROCESS_MODE_PATCHBAY:
        kData->maxPluginNumber = MAX_PATCHBAY_PLUGINS;
        break;

    case PROCESS_MODE_BRIDGE:
        kData->maxPluginNumber = 1;
        kData->bufEvents.in  = new EngineEvent[INTERNAL_EVENT_COUNT];
        kData->bufEvents.out = new EngineEvent[INTERNAL_EVENT_COUNT];
        break;
    }

    if (kData->maxPluginNumber == 0)
        return false;

    kData->nextPluginId = kData->maxPluginNumber;

    fName = clientName;
    fName.toBasic();

    fTimeInfo.clear();

    kData->plugins = new EnginePluginData[kData->maxPluginNumber];

    kData->osc.init(clientName);
#ifndef BUILD_BRIDGE
    kData->oscData = kData->osc.getControlData();
#endif

    if (type() != kEngineTypePlugin)
        carla_setprocname(clientName);

    kData->nextAction.ready();
    kData->thread.startNow();

    return true;
}

bool CarlaEngine::close()
{
    CARLA_ASSERT(fName.isNotEmpty());
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(kData->nextPluginId == kData->maxPluginNumber);
    carla_debug("CarlaEngine::close()");

    kData->thread.stopNow();
    kData->nextAction.ready();

#ifndef BUILD_BRIDGE
    osc_send_control_exit();
#endif
    kData->osc.close();
    kData->oscData = nullptr;

    kData->aboutToClose = true;
    kData->curPluginCount = 0;
    kData->maxPluginNumber = 0;
    kData->nextPluginId = 0;

    if (kData->plugins != nullptr)
    {
        delete[] kData->plugins;
        kData->plugins = nullptr;
    }

    if (kData->bufEvents.in != nullptr)
    {
        delete[] kData->bufEvents.in;
        kData->bufEvents.in = nullptr;
    }

    if (kData->bufEvents.out != nullptr)
    {
        delete[] kData->bufEvents.out;
        kData->bufEvents.out = nullptr;
    }

    fName.clear();

    return true;
}

void CarlaEngine::idle()
{
    CARLA_ASSERT(kData->plugins != nullptr); // this one too maybe
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull); // TESTING, remove later
    CARLA_ASSERT(kData->nextPluginId == kData->maxPluginNumber);     // TESTING, remove later

    for (unsigned int i=0; i < kData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(kData->plugins[i].plugin);

        if (plugin != nullptr && plugin->enabled())
            plugin->idleGui();
    }
}

CarlaEngineClient* CarlaEngine::addClient(CarlaPlugin* const)
{
    return new CarlaEngineClient(*this);
}

// -----------------------------------------------------------------------
// Plugin management

bool CarlaEngine::addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, const void* const extra)
{
    CARLA_ASSERT(btype != BINARY_NONE);
    CARLA_ASSERT(ptype != PLUGIN_NONE);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::addPlugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", BinaryType2Str(btype), PluginType2Str(ptype), filename, name, label, extra);

    unsigned int id;

    if (kData->nextPluginId < kData->curPluginCount)
    {
        id = kData->nextPluginId;
        kData->nextPluginId = kData->maxPluginNumber;
        CARLA_ASSERT(kData->plugins[id].plugin != nullptr);
    }
    else
    {
        id = kData->curPluginCount;

        if (id == kData->maxPluginNumber)
        {
            setLastError("Maximum number of plugins reached");
            return false;
        }

        CARLA_ASSERT(kData->plugins[id].plugin == nullptr);
    }

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

# ifndef Q_OS_WIN
    if (btype == BINARY_NATIVE && fOptions.bridge_native.isNotEmpty())
        bridgeBinary = (const char*)fOptions.bridge_native;
# endif

    if (bridgeBinary != nullptr && (btype != BINARY_NATIVE || fOptions.preferPluginBridges))
    {
        plugin = CarlaPlugin::newBridge(init, btype, ptype, bridgeBinary);
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

    ++kData->curPluginCount;

    callback(CALLBACK_PLUGIN_ADDED, id, 0, 0, 0.0f, plugin->name());
    return true;
}

bool CarlaEngine::removePlugin(const unsigned int id)
{
    CARLA_ASSERT(kData->curPluginCount != 0);
    CARLA_ASSERT(id < kData->curPluginCount);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::removePlugin(%i)", id);

    if (kData->plugins == nullptr || kData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return false;
    }

    CarlaPlugin* const plugin(kData->plugins[id].plugin);

    if (plugin == nullptr)
    {
        setLastError("Could not find plugin to remove");
        return false;
    }

    CARLA_ASSERT(plugin->id() == id);

    kData->thread.stopNow();

    const bool lockWait(isRunning() && fOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS);
    const CarlaEngineProtectedData::ScopedPluginAction spa(kData, kEnginePostActionRemovePlugin, id, 0, lockWait);

#ifndef BUILD_BRIDGE
    if (isOscControlRegistered())
        osc_send_control_remove_plugin(id);
#endif

    delete plugin;

    if (isRunning() && ! kData->aboutToClose)
        kData->thread.startNow();

    callback(CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0.0f, nullptr);
    return true;
}

void CarlaEngine::removeAllPlugins()
{
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::removeAllPlugins() - START");

    if (kData->plugins == nullptr || kData->curPluginCount == 0)
        return;

    kData->thread.stopNow();

    const bool lockWait(isRunning());
    const CarlaEngineProtectedData::ScopedPluginAction spa(kData, kEnginePostActionZeroCount, 0, 0, lockWait);

    for (unsigned int i=0; i < kData->maxPluginNumber; ++i)
    {
        CarlaPlugin* const plugin(kData->plugins[i].plugin);

        kData->plugins[i].plugin = nullptr;

        if (plugin != nullptr)
            delete plugin;

        // clear this plugin
        kData->plugins[i].insPeak[0]  = 0.0f;
        kData->plugins[i].insPeak[1]  = 0.0f;
        kData->plugins[i].outsPeak[0] = 0.0f;
        kData->plugins[i].outsPeak[1] = 0.0f;
    }

    if (isRunning() && ! kData->aboutToClose)
        kData->thread.startNow();

    carla_debug("CarlaEngine::removeAllPlugins() - END");
}

const char* CarlaEngine::renamePlugin(const unsigned int id, const char* const newName)
{
    CARLA_ASSERT(kData->curPluginCount != 0);
    CARLA_ASSERT(id < kData->curPluginCount);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(newName != nullptr);
    carla_debug("CarlaEngine::renamePlugin(%i, \"%s\")", id, newName);

    if (kData->plugins == nullptr || kData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return nullptr;
    }

    CarlaPlugin* const plugin(kData->plugins[id].plugin);

    if (plugin == nullptr)
    {
        carla_stderr("CarlaEngine::clonePlugin(%i) - could not find plugin", id);
        return nullptr;
    }

    CARLA_ASSERT(plugin->id() == id);

    if (const char* const name = getUniquePluginName(newName))
    {
        plugin->setName(name);
        return name;
    }

    return nullptr;
}

bool CarlaEngine::clonePlugin(const unsigned int id)
{
    CARLA_ASSERT(kData->curPluginCount > 0);
    CARLA_ASSERT(id < kData->curPluginCount);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::clonePlugin(%i)", id);

    if (kData->plugins == nullptr || kData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return false;
    }

    CarlaPlugin* const plugin(kData->plugins[id].plugin);

    if (plugin == nullptr)
    {
        carla_stderr("CarlaEngine::clonePlugin(%i) - could not find plugin", id);
        return false;
    }

    CARLA_ASSERT(plugin->id() == id);

    char label[STR_MAX+1] = { '\0' };
    plugin->getLabel(label);

    BinaryType binaryType = BINARY_NATIVE;

#ifndef BUILD_BRIDGE
    if (plugin->hints() & PLUGIN_IS_BRIDGE)
        binaryType = CarlaPluginGetBridgeBinaryType(plugin);
#endif

    const unsigned int pluginCountBefore(kData->curPluginCount);

    if (! addPlugin(binaryType, plugin->type(), plugin->filename(), plugin->name(), label, plugin->getExtraStuff()))
        return false;

    CARLA_ASSERT(pluginCountBefore+1 == kData->curPluginCount);

    if (CarlaPlugin* const newPlugin = kData->plugins[pluginCountBefore].plugin)
        newPlugin->loadSaveState(plugin->getSaveState());

    return true;
}

bool CarlaEngine::replacePlugin(const unsigned int id)
{
    CARLA_ASSERT(kData->curPluginCount > 0);
    CARLA_ASSERT(id < kData->curPluginCount);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::replacePlugin(%i)", id);

    if (id < kData->curPluginCount)
        kData->nextPluginId = id;

    return false;
}

bool CarlaEngine::switchPlugins(const unsigned int idA, const unsigned int idB)
{
    CARLA_ASSERT(kData->curPluginCount >= 2);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(idA != idB);
    CARLA_ASSERT(idA < kData->curPluginCount);
    CARLA_ASSERT(idB < kData->curPluginCount);
    carla_debug("CarlaEngine::switchPlugins(%i)", idA, idB);

    if (kData->plugins == nullptr || kData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return false;
    }

    kData->thread.stopNow();

    const bool lockWait(isRunning() && fOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS);
    const CarlaEngineProtectedData::ScopedPluginAction spa(kData, kEnginePostActionSwitchPlugins, idA, idB, lockWait);

#ifndef BUILD_BRIDGE // TODO
    //if (isOscControlRegistered())
    //    osc_send_control_switch_plugins(idA, idB);
#endif

    if (isRunning() && ! kData->aboutToClose)
        kData->thread.startNow();

    return true;
}

CarlaPlugin* CarlaEngine::getPlugin(const unsigned int id) const
{
    CARLA_ASSERT(kData->curPluginCount != 0);
    CARLA_ASSERT(id < kData->curPluginCount);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull); // TESTING, remove later
    carla_debug("CarlaEngine::getPlugin(%i) [count:%i]", id, kData->curPluginCount);

    if (id < kData->curPluginCount && kData->plugins != nullptr)
        return kData->plugins[id].plugin;

    return nullptr;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const unsigned int id) const
{
    return kData->plugins[id].plugin;
}

const char* CarlaEngine::getUniquePluginName(const char* const name)
{
    CARLA_ASSERT(kData->maxPluginNumber != 0);
    CARLA_ASSERT(kData->plugins != nullptr);
    CARLA_ASSERT(kData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(name != nullptr);
    carla_debug("CarlaEngine::getUniquePluginName(\"%s\")", name);

    static CarlaString sname;
    sname = name;

    if (sname.isEmpty())
    {
        sname = "(No name)";
        return (const char*)sname;
    }

    if (kData->plugins == nullptr || kData->maxPluginNumber == 0)
        return (const char*)sname;

    sname.truncate(maxClientNameSize()-5-1); // 5 = strlen(" (10)")
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (unsigned short i=0; i < kData->curPluginCount; ++i)
    {
        CARLA_ASSERT(kData->plugins[i].plugin != nullptr);

        if (kData->plugins[i].plugin == nullptr)
            break;

        // Check if unique name doesn't exist
        if (const char* const pluginName = kData->plugins[i].plugin->name())
        {
            if (sname != pluginName)
                continue;
        }

        // Check if string has already been modified
        {
            const size_t len(sname.length());

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

// -----------------------------------------------------------------------
// Project management

bool CarlaEngine::loadFilename(const char* const filename)
{
    CARLA_ASSERT(filename != nullptr);
    carla_debug("CarlaEngine::loadFilename(\"%s\")", filename);

    QFileInfo fileInfo(filename);

    if (! fileInfo.exists())
    {
        setLastError("File does not exist");
        return false;
    }

    if (! fileInfo.isFile())
    {
        setLastError("Not a file");
        return false;
    }

    if (! fileInfo.isReadable())
    {
        setLastError("File is not readable");
        return false;
    }

    CarlaString baseName(fileInfo.baseName().toUtf8().constData());
    CarlaString extension(fileInfo.suffix().toLower().toUtf8().constData());

    // -------------------------------------------------------------------

    if (extension == "carxp" || extension ==  "carxs")
        return loadProject(filename);

    // -------------------------------------------------------------------

    if (extension == "gig")
        return addPlugin(PLUGIN_GIG, filename, baseName, baseName);

    if (extension == "sf2")
        return addPlugin(PLUGIN_SF2, filename, baseName, baseName);

    if (extension == "sfz")
        return addPlugin(PLUGIN_SFZ, filename, baseName, baseName);

    // -------------------------------------------------------------------

    if (extension == "aiff" || extension == "flac" || extension == "oga" || extension == "ogg" || extension == "w64" || extension == "wav")
    {
#ifdef WANT_AUDIOFILE
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "audiofile"))
        {
            if (CarlaPlugin* const plugin = getPlugin(kData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_STRING, "file", filename, true);
            return true;
        }
        return false;
#else
        setLastError("This Carla build does not have Audio file support");
        return false;
#endif
    }

    if (extension == "3g2" || extension == "3gp" || extension == "aac" || extension == "ac3" || extension == "amr" || extension == "ape" ||
        extension == "mp2" || extension == "mp3" || extension == "mpc" || extension == "wma")
    {
#ifdef WANT_AUDIOFILE
# ifdef HAVE_FFMPEG
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "audiofile"))
        {
            if (CarlaPlugin* const plugin = getPlugin(kData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_STRING, "file", filename, true);
            return true;
        }
        return false;
# else
        setLastError("This Carla build has Audio file support, but not libav/ffmpeg");
        return false;
# endif
#else
        setLastError("This Carla build does not have Audio file support");
        return false;
#endif
    }

    // -------------------------------------------------------------------

    if (extension == "mid" || extension == "midi")
    {
#ifdef WANT_MIDIFILE
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "midifile"))
        {
            if (CarlaPlugin* const plugin = getPlugin(kData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_STRING, "file", filename, true);
            return true;
        }
        return false;
#else
        setLastError("This Carla build does not have MIDI file support");
        return false;
#endif
    }

    // -------------------------------------------------------------------
    // ZynAddSubFX

    if (extension == "xmz" || extension == "xiz")
    {
#ifdef WANT_ZYNADDSUBFX
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "zynaddsubfx"))
        {
            if (CarlaPlugin* const plugin = getPlugin(kData->curPluginCount-1))
                plugin->setCustomData(CUSTOM_DATA_STRING, (extension == "xmz") ? "CarlaAlternateFile1" : "CarlaAlternateFile2", filename, true);
            return true;
        }
        return false;
#else
        setLastError("This Carla build does not have ZynAddSubFX support");
        return false;
#endif
    }

    // -------------------------------------------------------------------

    setLastError("Unknown file extension");
    return false;
}

bool charEndsWith(const char* const str, const char* const suffix)
{
    if (str == nullptr || suffix == nullptr)
        return false;

    const size_t strLen(std::strlen(str));
    const size_t suffixLen(std::strlen(suffix));

    if (strLen < suffixLen)
        return false;

    return (std::strncmp(str + (strLen-suffixLen), suffix, suffixLen) == 0);
}

bool CarlaEngine::loadProject(const char* const filename)
{
    CARLA_ASSERT(filename != nullptr);
    carla_debug("CarlaEngine::loadProject(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QDomDocument xml;
    xml.setContent(file.readAll());
    file.close();

    QDomNode xmlNode(xml.documentElement());

    if (xmlNode.toElement().tagName() != "CARLA-PROJECT" && xmlNode.toElement().tagName() != "CARLA-PRESET")
    {
        setLastError("Not a valid Carla project or preset file");
        return false;
    }

    const bool isPreset(xmlNode.toElement().tagName() == "CARLA-PRESET");

    QDomNode node(xmlNode.firstChild());

    while (! node.isNull())
    {
        if (isPreset || node.toElement().tagName() == "Plugin")
        {
            const SaveState& saveState(getSaveStateDictFromXML(isPreset ? xmlNode : node));
            CARLA_ASSERT(saveState.type != nullptr);

            if (saveState.type == nullptr)
                continue;

            const void* extraStuff = nullptr;

            if (std::strcmp(saveState.type, "DSSI") == 0)
            {
                extraStuff = findDSSIGUI(saveState.binary, saveState.label);
            }
            else if (std::strcmp(saveState.type, "SF2") == 0)
            {
                const char use16OutsSuffix[] = " (16 outs)";

                if (charEndsWith(saveState.label, use16OutsSuffix))
                    extraStuff = (void*)0x1; // non-null
            }

            // TODO - proper find&load plugins
            if (addPlugin(getPluginTypeFromString(saveState.type), saveState.binary, saveState.name, saveState.label, extraStuff))
            {
                if (CarlaPlugin* plugin = getPlugin(kData->curPluginCount-1))
                    plugin->loadSaveState(saveState);
            }
        }

        if (isPreset)
            break;

        node = node.nextSibling();
    }

    return true;
}

bool CarlaEngine::saveProject(const char* const filename)
{
    CARLA_ASSERT(filename != nullptr);
    carla_debug("CarlaEngine::saveProject(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PROJECT>\n";
    out << "<CARLA-PROJECT VERSION='1.0'>\n";

    bool firstPlugin = true;
    char strBuf[STR_MAX+1];

    for (unsigned int i=0; i < kData->curPluginCount; ++i)
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

// FIXME

float CarlaEngine::getInputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < kData->curPluginCount);
    CARLA_ASSERT(id-1 < 2);

    if (id == 0 || id > 2)
        return 0.0f;

    return kData->plugins[pluginId].insPeak[id-1];
}

float CarlaEngine::getOutputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < kData->curPluginCount);
    CARLA_ASSERT(id-1 < 2);

    if (id == 0 || id > 2)
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
    CARLA_ASSERT(func != nullptr);
    carla_debug("CarlaEngine::setCallback(%p, %p)", func, ptr);

    kData->callback    = func;
    kData->callbackPtr = ptr;
}

// -----------------------------------------------------------------------
// Patchbay

bool CarlaEngine::patchbayConnect(int, int)
{
    setLastError("Unsupported operation");
    return false;
}

bool CarlaEngine::patchbayDisconnect(int)
{
    setLastError("Unsupported operation");
    return false;
}

void CarlaEngine::patchbayRefresh()
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

        if (value < PROCESS_MODE_SINGLE_CLIENT || value > PROCESS_MODE_BRIDGE)
            return carla_stderr("CarlaEngine::setOption(%s, %i, \"%s\") - invalid value", OptionsType2Str(option), value, valueStr);

        fOptions.processMode = static_cast<ProcessMode>(value);
        break;

    case OPTION_TRANSPORT_MODE:
        // FIXME: Always enable JACK transport for now
#if 0
        if (value < CarlaBackend::TRANSPORT_MODE_INTERNAL || value > CarlaBackend::TRANSPORT_MODE_BRIDGE)
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

    case OPTION_JACK_AUTOCONNECT:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.jackAutoConnect = (value != 0);
        break;

    case OPTION_JACK_TIMEMASTER:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.jackTimeMaster = (value != 0);
        break;

#ifdef WANT_RTAUDIO
    case OPTION_RTAUDIO_BUFFER_SIZE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.rtaudioBufferSize = static_cast<uint>(value);
        break;

    case OPTION_RTAUDIO_SAMPLE_RATE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.rtaudioSampleRate = static_cast<uint>(value);
        break;

    case OPTION_RTAUDIO_DEVICE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        fOptions.rtaudioDevice = valueStr;
        break;
#endif

    case OPTION_PATH_RESOURCES:
        fOptions.resourceDir = valueStr;
        break;

#ifndef BUILD_BRIDGE
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
#endif

#ifdef WANT_LV2
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        fOptions.bridge_lv2Gtk2 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_GTK3:
        fOptions.bridge_lv2Gtk3 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_QT4:
        fOptions.bridge_lv2Qt4 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_QT5:
        fOptions.bridge_lv2Qt5 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_COCOA:
        fOptions.bridge_lv2Cocoa = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_WINDOWS:
        fOptions.bridge_lv2Win = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_X11:
        fOptions.bridge_lv2X11 = valueStr;
        break;
#endif

#ifdef WANT_VST
    case OPTION_PATH_BRIDGE_VST_COCOA:
        fOptions.bridge_vstCocoa = valueStr;
        break;
    case OPTION_PATH_BRIDGE_VST_HWND:
        fOptions.bridge_vstHWND = valueStr;
        break;
    case OPTION_PATH_BRIDGE_VST_X11:
        fOptions.bridge_vstX11 = valueStr;
        break;
#endif
    }
}

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

    for (unsigned int i=0; i < kData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(kData->plugins[i].plugin);

        if (plugin != nullptr && plugin->enabled())
            plugin->bufferSizeChanged(newBufferSize);
    }

    callback(CALLBACK_BUFFER_SIZE_CHANGED, 0, newBufferSize, 0, 0.0f, nullptr);
}

void CarlaEngine::sampleRateChanged(const double newSampleRate)
{
    carla_debug("CarlaEngine::sampleRateChanged(%g)", newSampleRate);

    for (unsigned int i=0; i < kData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(kData->plugins[i].plugin);

        if (plugin != nullptr && plugin->enabled())
            plugin->sampleRateChanged(newSampleRate);
    }

    callback(CALLBACK_SAMPLE_RATE_CHANGED, 0, 0, 0, newSampleRate, nullptr);
}

void CarlaEngine::offlineModeChanged(const bool isOffline)
{
    carla_debug("CarlaEngine::offlineModeChanged(%s)", bool2str(isOffline));

    for (unsigned int i=0; i < kData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(kData->plugins[i].plugin);

        if (plugin != nullptr && plugin->enabled())
           plugin->offlineModeChanged(isOffline);
    }
}

void CarlaEngine::proccessPendingEvents()
{
    //carla_stderr("proccessPendingEvents(%i)", kData->nextAction.opcode);

    kData->doNextPluginAction(true);

    if (kData->time.playing)
        kData->time.frame += fBufferSize;

    if (fOptions.transportMode == CarlaBackend::TRANSPORT_MODE_INTERNAL)
    {
        fTimeInfo.playing = kData->time.playing;
        fTimeInfo.frame   = kData->time.frame;
    }

    for (unsigned int i=0; i < kData->curPluginCount; ++i)
    {
        // TODO - peak values?
    }
}

void CarlaEngine::setPeaks(const unsigned int pluginId, float const inPeaks[2], float const outPeaks[2])
{
    kData->plugins[pluginId].insPeak[0]  = inPeaks[0];
    kData->plugins[pluginId].insPeak[1]  = inPeaks[1];
    kData->plugins[pluginId].outsPeak[0] = outPeaks[0];
    kData->plugins[pluginId].outsPeak[1] = outPeaks[1];
}

EngineEvent* CarlaEngine::getInternalEventBuffer(const bool isInput) const
{
    return isInput ? kData->bufEvents.in : kData->bufEvents.out;
}

#ifndef BUILD_BRIDGE
void setValueIfHigher(float& value, const float& compare)
{
    if (value < compare)
        value = compare;
}

void CarlaEngine::processRack(float* inBuf[2], float* outBuf[2], const uint32_t frames)
{
    CARLA_ASSERT(kData->bufEvents.in != nullptr);
    CARLA_ASSERT(kData->bufEvents.out != nullptr);

    // initialize outputs (zero)
    carla_zeroFloat(outBuf[0], frames);
    carla_zeroFloat(outBuf[1], frames);
    carla_zeroMem(kData->bufEvents.out, sizeof(EngineEvent)*INTERNAL_EVENT_COUNT);

    bool processed = false;

    // process plugins
    for (unsigned int i=0; i < kData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin = kData->plugins[i].plugin;

        if (plugin == nullptr || ! plugin->enabled() || ! plugin->tryLock())
            continue;

        if (processed)
        {
            // initialize inputs (from previous outputs)
            carla_copyFloat(inBuf[0], outBuf[0], frames);
            carla_copyFloat(inBuf[1], outBuf[1], frames);
            std::memcpy(kData->bufEvents.in, kData->bufEvents.out, sizeof(EngineEvent)*INTERNAL_EVENT_COUNT);

            // initialize outputs (zero)
            carla_zeroFloat(outBuf[0], frames);
            carla_zeroFloat(outBuf[1], frames);
            carla_zeroMem(kData->bufEvents.out, sizeof(EngineEvent)*INTERNAL_EVENT_COUNT);
        }

        // process
        plugin->initBuffers();
        plugin->process(inBuf, outBuf, frames);
        plugin->unlock();

#if 0
        // if plugin has no audio inputs, add previous buffers
        if (plugin->audioInCount() == 0)
        {
            for (uint32_t j=0; j < frames; ++j)
            {
                outBuf[0][j] += inBuf[0][j];
                outBuf[1][j] += inBuf[1][j];
            }
        }
        // if plugin has no midi output, add previous events
        if (plugin->midiOutCount() == 0)
        {
            for (uint32_t j=0, k=0; j < frames; ++j)
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

            for (uint32_t k=0; k < frames; ++k)
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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(pluginName != nullptr);
    carla_debug("CarlaEngine::osc_send_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    carla_debug("CarlaEngine::osc_send_control_add_plugin_end(%i)", pluginId);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->curPluginCount));
    carla_debug("CarlaEngine::osc_send_control_remove_plugin(%i)", pluginId);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(type != PLUGIN_NONE);
    CARLA_ASSERT(realName != nullptr);
    CARLA_ASSERT(label != nullptr);
    CARLA_ASSERT(maker != nullptr);
    CARLA_ASSERT(copyright != nullptr);
    carla_debug("CarlaEngine::osc_send_control_set_plugin_data(%i, %i, %i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    carla_debug("CarlaEngine::osc_send_control_set_plugin_ports(%i, %i, %i, %i, %i, %i, %i, %i)", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(type != PARAMETER_UNKNOWN);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(label != nullptr);
    carla_debug("CarlaEngine::osc_send_control_set_parameter_data(%i, %i, %i, %i, \"%s\", \"%s\", %f)", pluginId, index, type, hints, name, label, current);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+20];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_data");
        lo_send(kData->oscData->target, targetPath, "iiiissf", pluginId, index, type, hints, name, label, current);
    }
}

void CarlaEngine::osc_send_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const float min, const float max, const float def, const float step, const float stepSmall, const float stepLarge)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(min < max);
    carla_debug("CarlaEngine::osc_send_control_set_parameter_ranges(%i, %i, %f, %f, %f, %f, %f, %f)", pluginId, index, min, max, def, step, stepSmall, stepLarge);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+22];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_ranges");
        lo_send(kData->oscData->target, targetPath, "iiffffff", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    carla_debug("CarlaEngine::osc_send_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    carla_debug("CarlaEngine::osc_send_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
#if DEBUG
    if (index < 0)
        carla_debug("CarlaEngine::osc_send_control_set_parameter_value(%i, %s, %f)", pluginId, InternalParametersIndex2Str((InternalParametersIndex)index), value);
    else
        carla_debug("CarlaEngine::osc_send_control_set_parameter_value(%i, %i, %f)", pluginId, index, value);
#endif

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+21];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_parameter_value");
        lo_send(kData->oscData->target, targetPath, "iif", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_default_value(const int32_t pluginId, const int32_t index, const float value)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    carla_debug("CarlaEngine::osc_send_control_set_default_value(%i, %i, %f)", pluginId, index, value);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+19];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/set_default_value");
        lo_send(kData->oscData->target, targetPath, "iif", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_program(const int32_t pluginId, const int32_t index)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    carla_debug("CarlaEngine::osc_send_control_set_program(%i, %i)", pluginId, index);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::osc_send_control_set_program_count(%i, %i)", pluginId, count);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(name != nullptr);
    carla_debug("CarlaEngine::osc_send_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    carla_debug("CarlaEngine::osc_send_control_set_midi_program(%i, %i)", pluginId, index);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::osc_send_control_set_midi_program_count(%i, %i)", pluginId, count);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(bank >= 0);
    CARLA_ASSERT(program >= 0);
    CARLA_ASSERT(name != nullptr);
    carla_debug("CarlaEngine::osc_send_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->curPluginCount));
    CARLA_ASSERT(channel >= 0 && channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note >= 0 && note < MAX_MIDI_NOTE);
    CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);
    carla_debug("CarlaEngine::osc_send_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->curPluginCount));
    CARLA_ASSERT(channel >= 0 && channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note >= 0 && note < MAX_MIDI_NOTE);
    carla_debug("CarlaEngine::osc_send_control_note_off(%i, %i, %i)", pluginId, channel, note);

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
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(kData->curPluginCount));

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_control_exit()");

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);
    carla_debug("CarlaEngine::osc_send_bridge_audio_count(%i, %i, %i)", ins, outs, total);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);
    carla_debug("CarlaEngine::osc_send_bridge_midi_count(%i, %i, %i)", ins, outs, total);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);
    carla_debug("CarlaEngine::osc_send_bridge_parameter_count(%i, %i, %i)", ins, outs, total);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::osc_send_bridge_program_count(%i)", count);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::osc_send_bridge_midi_program_count(%i)", count);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(label != nullptr);
    CARLA_ASSERT(maker != nullptr);
    CARLA_ASSERT(copyright != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_plugin_info(%i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", category, hints, name, label, maker, copyright, uniqueId);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(unit != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_parameter_info(%i, \"%s\", \"%s\")", index, name, unit);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_parameter_data(%i, %i, %i, %i, %i, %i)", index, type, rindex, hints, midiChannel, midiCC);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_parameter_ranges(%i, %f, %f, %f, %f, %f, %f)", index, def, min, max, step, stepSmall, stepLarge);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+25];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_ranges");
        lo_send(kData->oscData->target, targetPath, "iffffff", index, def, min, max, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_bridge_program_info(const int32_t index, const char* const name)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_program_info(%i, \"%s\")", index, name);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_midi_program_info(%i, %i, %i, \"%s\")", index, bank, program, label);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    CARLA_ASSERT(key != nullptr);
    CARLA_ASSERT(value != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_configure(\"%s\", \"%s\")", key, value);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_set_parameter_value(%i, %f)", index, value);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+28];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_parameter_value");
        lo_send(kData->oscData->target, targetPath, "if", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_default_value(const int32_t index, const float value)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_set_default_value(%i, %f)", index, value);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+26];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_default_value");
        lo_send(kData->oscData->target, targetPath, "if", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_program(const int32_t index)
{
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_set_program(%i)", index);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_set_midi_program(%i)", index);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_set_custom_data(\"%s\", \"%s\", \"%s\")", type, key, value);

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
    CARLA_ASSERT(kData->oscData != nullptr);
    carla_debug("CarlaEngine::osc_send_bridge_set_chunk_data(\"%s\")", chunkFile);

    if (kData->oscData != nullptr && kData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(kData->oscData->path)+23];
        std::strcpy(targetPath, kData->oscData->path);
        std::strcat(targetPath, "/bridge_set_chunk_data");
        lo_send(kData->oscData->target, targetPath, "s", chunkFile);
    }
}
#endif

CARLA_BACKEND_END_NAMESPACE
