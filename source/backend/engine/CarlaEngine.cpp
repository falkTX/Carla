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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaStateUtils.hpp"
#include "CarlaMIDI.h"

#include "juce_audio_basics.h"

using juce::FloatVectorOperations;

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackEngineEvent;

// -----------------------------------------------------------------------
// Carla Engine port (Abstract)

CarlaEnginePort::CarlaEnginePort(const CarlaEngine& engine, const bool isInput)
    : fEngine(engine),
      fIsInput(isInput)
{
    carla_debug("CarlaEnginePort::CarlaEnginePort(name:\"%s\", %s)", engine.getName(), bool2str(isInput));
}

CarlaEnginePort::~CarlaEnginePort()
{
    carla_debug("CarlaEnginePort::~CarlaEnginePort()");
}

// -----------------------------------------------------------------------
// Carla Engine Audio port

CarlaEngineAudioPort::CarlaEngineAudioPort(const CarlaEngine& engine, const bool isInput)
    : CarlaEnginePort(engine, isInput),
      fBuffer(nullptr)
{
    carla_debug("CarlaEngineAudioPort::CarlaEngineAudioPort(name:\"%s\", %s)", engine.getName(), bool2str(isInput));
}

CarlaEngineAudioPort::~CarlaEngineAudioPort()
{
    carla_debug("CarlaEngineAudioPort::~CarlaEngineAudioPort()");
}

// -----------------------------------------------------------------------
// Carla Engine CV port

CarlaEngineCVPort::CarlaEngineCVPort(const CarlaEngine& engine, const bool isInput)
    : CarlaEnginePort(engine, isInput),
      fBuffer(new float[engine.getBufferSize()])
{
    carla_debug("CarlaEngineCVPort::CarlaEngineCVPort(name:\"%s\", %s)", engine.getName(), bool2str(isInput));
}

CarlaEngineCVPort::~CarlaEngineCVPort()
{
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
    carla_debug("CarlaEngineCVPort::~CarlaEngineCVPort()");

    delete[] fBuffer;
    fBuffer = nullptr;
}

void CarlaEngineCVPort::initBuffer()
{
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

    FloatVectorOperations::clear(fBuffer, fEngine.getBufferSize());
}

void CarlaEngineCVPort::writeBuffer(const uint32_t, const uint32_t)
{
    CARLA_ASSERT(! fIsInput);
}

void CarlaEngineCVPort::setBufferSize(const uint32_t bufferSize)
{
    if (fBuffer != nullptr)
        delete[] fBuffer;

    fBuffer = new float[bufferSize];
}

// -----------------------------------------------------------------------
// Carla Engine Event port

CarlaEngineEventPort::CarlaEngineEventPort(const CarlaEngine& engine, const bool isInput)
    : CarlaEnginePort(engine, isInput),
      fBuffer(nullptr)
{
    carla_debug("CarlaEngineEventPort::CarlaEngineEventPort(name:\"%s\", %s)", engine.getName(), bool2str(isInput));

    if (fEngine.getProccessMode() == PROCESS_MODE_PATCHBAY)
        fBuffer = new EngineEvent[kEngineMaxInternalEventCount];
}

CarlaEngineEventPort::~CarlaEngineEventPort()
{
    carla_debug("CarlaEngineEventPort::~CarlaEngineEventPort()");

    if (fEngine.getProccessMode() == PROCESS_MODE_PATCHBAY)
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        delete[] fBuffer;
        fBuffer = nullptr;
    }
}

void CarlaEngineEventPort::initBuffer()
{
    if (fEngine.getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK || fEngine.getProccessMode() == PROCESS_MODE_BRIDGE)
        fBuffer = fEngine.getInternalEventBuffer(fIsInput);
    else if (fEngine.getProccessMode() == PROCESS_MODE_PATCHBAY && ! fIsInput)
        carla_zeroStruct<EngineEvent>(fBuffer, kEngineMaxInternalEventCount);
}

uint32_t CarlaEngineEventPort::getEventCount() const
{
    CARLA_SAFE_ASSERT_RETURN(fIsInput, 0);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, 0);
    CARLA_SAFE_ASSERT_RETURN(fEngine.getProccessMode() != PROCESS_MODE_SINGLE_CLIENT && fEngine.getProccessMode() != PROCESS_MODE_MULTIPLE_CLIENTS, 0);

    uint32_t i=0;

    for (; i < kEngineMaxInternalEventCount; ++i)
    {
        if (fBuffer[i].type == kEngineEventTypeNull)
            break;
    }

    return i;
}

const EngineEvent& CarlaEngineEventPort::getEvent(const uint32_t index)
{
    CARLA_SAFE_ASSERT_RETURN(fIsInput, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(fEngine.getProccessMode() != PROCESS_MODE_SINGLE_CLIENT && fEngine.getProccessMode() != PROCESS_MODE_MULTIPLE_CLIENTS, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(index < kEngineMaxInternalEventCount, kFallbackEngineEvent);

    return fBuffer[index];
}

const EngineEvent& CarlaEngineEventPort::getEventUnchecked(const uint32_t index)
{
    return fBuffer[index];
}

void CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value)
{
    CARLA_SAFE_ASSERT_RETURN(! fIsInput,);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fEngine.getProccessMode() != PROCESS_MODE_SINGLE_CLIENT && fEngine.getProccessMode() != PROCESS_MODE_MULTIPLE_CLIENTS,);
    CARLA_SAFE_ASSERT_RETURN(type != kEngineControlEventTypeNull,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

    if (type == kEngineControlEventTypeParameter)
    {
        CARLA_ASSERT(! MIDI_IS_CONTROL_BANK_SELECT(param));
    }

    const float fixedValue(carla_fixValue<float>(0.0f, 1.0f, value));

    for (uint32_t i=0; i < kEngineMaxInternalEventCount; ++i)
    {
        if (fBuffer[i].type != kEngineEventTypeNull)
            continue;

        EngineEvent& event(fBuffer[i]);

        event.type    = kEngineEventTypeControl;
        event.time    = time;
        event.channel = channel;

        event.ctrl.type  = type;
        event.ctrl.param = param;
        event.ctrl.value = fixedValue;

        return;
    }

    carla_stderr2("CarlaEngineEventPort::writeControlEvent() - buffer full");
}

void CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t* const data, const uint8_t size)
{
    CARLA_SAFE_ASSERT_RETURN(! fIsInput,);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fEngine.getProccessMode() != PROCESS_MODE_SINGLE_CLIENT && fEngine.getProccessMode() != PROCESS_MODE_MULTIPLE_CLIENTS,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0 && size <= 4,);

    for (uint32_t i=0; i < kEngineMaxInternalEventCount; ++i)
    {
        if (fBuffer[i].type != kEngineEventTypeNull)
            continue;

        EngineEvent& event(fBuffer[i]);

        event.type    = kEngineEventTypeMidi;
        event.time    = time;
        event.channel = channel;

        event.midi.port = port;
        event.midi.size = size;

        event.midi.data[0] = MIDI_GET_CHANNEL_FROM_DATA(data);

        for (uint8_t j=1; j < size; ++j)
            event.midi.data[j] = data[j];

        return;
    }

    carla_stderr2("CarlaEngineEventPort::writeMidiEvent() - buffer full");
}

// -----------------------------------------------------------------------
// Carla Engine client (Abstract)

CarlaEngineClient::CarlaEngineClient(const CarlaEngine& engine)
    : fEngine(engine),
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

bool CarlaEngineClient::isActive() const noexcept
{
    return fActive;
}

bool CarlaEngineClient::isOk() const noexcept
{
    return true;
}

uint32_t CarlaEngineClient::getLatency() const noexcept
{
    return fLatency;
}

void CarlaEngineClient::setLatency(const uint32_t samples) noexcept
{
    fLatency = samples;
}

CarlaEnginePort* CarlaEngineClient::addPort(const EnginePortType portType, const char* const name, const bool isInput)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngineClient::addPort(%i:%s, \"%s\", %s)", portType, EnginePortType2Str(portType), name, bool2str(isInput));

    switch (portType)
    {
    case kEnginePortTypeNull:
        break;
    case kEnginePortTypeAudio:
        return new CarlaEngineAudioPort(fEngine, isInput);
    case kEnginePortTypeCV:
        return new CarlaEngineCVPort(fEngine, isInput);
    case kEnginePortTypeEvent:
        return new CarlaEngineEventPort(fEngine, isInput);
    case kEnginePortTypeOSC:
        return nullptr; //new CarlaEngineOscPort(fEngine, isInput);
    }

    carla_stderr("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
    return nullptr;
}

// -----------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine()
    : fBufferSize(0),
      fSampleRate(0.0),
      pData(new CarlaEngineProtectedData(this))
{
    carla_debug("CarlaEngine::CarlaEngine()");
}

CarlaEngine::~CarlaEngine()
{
    carla_debug("CarlaEngine::~CarlaEngine()");

    delete pData;
}

// -----------------------------------------------------------------------
// Static calls

unsigned int CarlaEngine::getDriverCount()
{
    carla_debug("CarlaEngine::getDriverCount()");

    unsigned int count = 1; // JACK

#ifndef BUILD_BRIDGE
    count += getRtAudioApiCount();
    count += getJuceApiCount();
#endif

    return count;
}

const char* CarlaEngine::getDriverName(const unsigned int index)
{
    carla_debug("CarlaEngine::getDriverName(%i)", index);

    if (index == 0)
        return "JACK";

#ifndef BUILD_BRIDGE
    const unsigned int rtAudioIndex(index-1);

    if (rtAudioIndex < getRtAudioApiCount())
        return getRtAudioApiName(rtAudioIndex);

    const unsigned int juceIndex(index-rtAudioIndex-1);

    if (juceIndex < getJuceApiCount())
        return getJuceApiName(juceIndex);
#endif

    carla_stderr("CarlaEngine::getDriverName(%i) - invalid index", index);
    return nullptr;
}

const char** CarlaEngine::getDriverDeviceNames(const unsigned int index)
{
    carla_debug("CarlaEngine::getDriverDeviceNames(%i)", index);

    if (index == 0)
    {
        static const char* ret[3] = { "Auto-Connect OFF", "Auto-Connect ON", nullptr };
        return ret;
    }

#ifndef BUILD_BRIDGE
    const unsigned int rtAudioIndex(index-1);

    if (rtAudioIndex < getRtAudioApiCount())
        return getRtAudioApiDeviceNames(rtAudioIndex);

    const unsigned int juceIndex(index-rtAudioIndex-1);

    if (juceIndex < getJuceApiCount())
        return getJuceApiDeviceNames(juceIndex);
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i) - invalid index", index);
    return nullptr;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', nullptr);
    carla_debug("CarlaEngine::newDriverByName(\"%s\")", driverName);

    if (std::strcmp(driverName, "JACK") == 0)
        return newJack();

    // common
    if (std::strncmp(driverName, "JACK ", 5) == 0)
        return newRtAudio(AUDIO_API_JACK);

    // linux
    if (std::strcmp(driverName, "ALSA") == 0)
        return newRtAudio(AUDIO_API_ALSA);
    if (std::strcmp(driverName, "OSS") == 0)
        return newRtAudio(AUDIO_API_OSS);
    if (std::strcmp(driverName, "PulseAudio") == 0)
        return newRtAudio(AUDIO_API_PULSE);

    // macos
    if (std::strcmp(driverName, "CoreAudio") == 0)
        return newRtAudio(AUDIO_API_CORE);

    // windows
    if (std::strcmp(driverName, "ASIO") == 0)
        return newRtAudio(AUDIO_API_ASIO);
    if (std::strcmp(driverName, "DirectSound") == 0)
        return newRtAudio(AUDIO_API_DS);

    carla_stderr("CarlaEngine::newDriverByName(\"%s\") - invalid driver name", driverName);
    return nullptr;
}

// -----------------------------------------------------------------------
// Maximum values

unsigned int CarlaEngine::getMaxClientNameSize() const noexcept
{
    return STR_MAX/2;
}

unsigned int CarlaEngine::getMaxPortNameSize() const noexcept
{
    return STR_MAX;
}

unsigned int CarlaEngine::getCurrentPluginCount() const noexcept
{
    return pData->curPluginCount;
}

unsigned int CarlaEngine::getMaxPluginNumber() const noexcept
{
    return pData->maxPluginNumber;
}

// -----------------------------------------------------------------------
// Virtual, per-engine type calls

bool CarlaEngine::init(const char* const clientName)
{
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    CARLA_ASSERT(fName.isEmpty());
    CARLA_ASSERT(pData->oscData == nullptr);
    CARLA_ASSERT(pData->plugins == nullptr);
    CARLA_ASSERT(pData->bufEvents.in  == nullptr);
    CARLA_ASSERT(pData->bufEvents.out == nullptr);
    carla_debug("CarlaEngine::init(\"%s\")", clientName);

    pData->aboutToClose = false;
    pData->curPluginCount = 0;
    pData->maxPluginNumber = 0;
    pData->nextPluginId = 0;

    switch (fOptions.processMode)
    {
    case PROCESS_MODE_SINGLE_CLIENT:
    case PROCESS_MODE_MULTIPLE_CLIENTS:
        pData->maxPluginNumber = MAX_DEFAULT_PLUGINS;
        break;

    case PROCESS_MODE_CONTINUOUS_RACK:
        pData->maxPluginNumber = MAX_RACK_PLUGINS;
        pData->bufEvents.in  = new EngineEvent[kEngineMaxInternalEventCount];
        pData->bufEvents.out = new EngineEvent[kEngineMaxInternalEventCount];
        break;

    case PROCESS_MODE_PATCHBAY:
        pData->maxPluginNumber = MAX_PATCHBAY_PLUGINS;
        break;

    case PROCESS_MODE_BRIDGE:
        pData->maxPluginNumber = 1;
        pData->bufEvents.in  = new EngineEvent[kEngineMaxInternalEventCount];
        pData->bufEvents.out = new EngineEvent[kEngineMaxInternalEventCount];
        break;
    }

    if (pData->maxPluginNumber == 0)
        return false;

    pData->nextPluginId = pData->maxPluginNumber;

    fName = clientName;
    fName.toBasic();

    fTimeInfo.clear();

    pData->plugins = new EnginePluginData[pData->maxPluginNumber];

    pData->osc.init(clientName);
#ifndef BUILD_BRIDGE
    pData->oscData = pData->osc.getControlData();
#endif

    pData->nextAction.ready();
    pData->thread.startThread();

    return true;
}

bool CarlaEngine::close()
{
    CARLA_ASSERT(fName.isNotEmpty());
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(pData->nextPluginId == pData->maxPluginNumber);
    carla_debug("CarlaEngine::close()");

    pData->thread.stopThread(500);
    pData->nextAction.ready();

#ifndef BUILD_BRIDGE
    oscSend_control_exit();
#endif
    pData->osc.close();
    pData->oscData = nullptr;

    pData->aboutToClose = true;
    pData->curPluginCount = 0;
    pData->maxPluginNumber = 0;
    pData->nextPluginId = 0;

    if (pData->plugins != nullptr)
    {
        delete[] pData->plugins;
        pData->plugins = nullptr;
    }

    if (pData->bufEvents.in != nullptr)
    {
        delete[] pData->bufEvents.in;
        pData->bufEvents.in = nullptr;
    }

    if (pData->bufEvents.out != nullptr)
    {
        delete[] pData->bufEvents.out;
        pData->bufEvents.out = nullptr;
    }

    fName.clear();

    return true;
}

void CarlaEngine::idle()
{
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull); // TESTING, remove later
    CARLA_ASSERT(pData->nextPluginId == pData->maxPluginNumber);     // TESTING, remove later
    CARLA_ASSERT(pData->plugins != nullptr); // this one too maybe

    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
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
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::addPlugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", BinaryType2Str(btype), PluginType2Str(ptype), filename, name, label, extra);

    unsigned int id;
    CarlaPlugin* oldPlugin = nullptr;

    if (pData->nextPluginId < pData->curPluginCount)
    {
        id = pData->nextPluginId;
        pData->nextPluginId = pData->maxPluginNumber;

        oldPlugin = pData->plugins[id].plugin;
        CARLA_ASSERT(oldPlugin != nullptr);
    }
    else
    {
        id = pData->curPluginCount;

        if (id == pData->maxPluginNumber)
        {
            setLastError("Maximum number of plugins reached");
            return false;
        }

        CARLA_ASSERT(pData->plugins[id].plugin == nullptr);
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
            plugin = CarlaPlugin::newDSSI(init);
            break;

        case PLUGIN_LV2:
            plugin = CarlaPlugin::newLV2(init);
            break;

        case PLUGIN_VST:
            plugin = CarlaPlugin::newVST(init);
            break;

        case PLUGIN_AU:
            //plugin = CarlaPlugin::newAU(init);
            break;

        case PLUGIN_CSOUND:
            //plugin = CarlaPlugin::newCSOUND(init);
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

    pData->plugins[id].plugin      = plugin;
    pData->plugins[id].insPeak[0]  = 0.0f;
    pData->plugins[id].insPeak[1]  = 0.0f;
    pData->plugins[id].outsPeak[0] = 0.0f;
    pData->plugins[id].outsPeak[1] = 0.0f;

    if (oldPlugin != nullptr)
    {
        delete oldPlugin;
        callback(CALLBACK_RELOAD_ALL, id, 0, 0, 0.0f, plugin->getName());
    }
    else
    {
        ++pData->curPluginCount;
        callback(CALLBACK_PLUGIN_ADDED, id, 0, 0, 0.0f, plugin->getName());
    }

    return true;
}

bool CarlaEngine::removePlugin(const unsigned int id)
{
    CARLA_ASSERT(pData->curPluginCount != 0);
    CARLA_ASSERT(id < pData->curPluginCount);
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::removePlugin(%i)", id);

    if (pData->plugins == nullptr || pData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return false;
    }

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    if (plugin == nullptr)
    {
        setLastError("Could not find plugin to remove");
        return false;
    }

    CARLA_ASSERT(plugin->getId() == id);

    pData->thread.stopThread(500);

    const bool lockWait(isRunning() && fOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS);
    const CarlaEngineProtectedData::ScopedActionLock sal(pData, kEnginePostActionRemovePlugin, id, 0, lockWait);

#ifndef BUILD_BRIDGE
    if (isOscControlRegistered())
        oscSend_control_remove_plugin(id);
#endif

    delete plugin;

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.startThread();

    callback(CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0.0f, nullptr);
    return true;
}

void CarlaEngine::removeAllPlugins()
{
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::removeAllPlugins() - START");

    if (pData->plugins == nullptr || pData->curPluginCount == 0)
        return;

    pData->thread.stopThread(500);

    const bool lockWait(isRunning());
    const CarlaEngineProtectedData::ScopedActionLock sal(pData, kEnginePostActionZeroCount, 0, 0, lockWait);

    for (unsigned int i=0; i < pData->maxPluginNumber; ++i)
    {
        if (CarlaPlugin* const plugin = pData->plugins[i].plugin)
        {
            pData->plugins[i].plugin = nullptr;
            delete plugin;
        }

        // clear this plugin
        pData->plugins[i].insPeak[0]  = 0.0f;
        pData->plugins[i].insPeak[1]  = 0.0f;
        pData->plugins[i].outsPeak[0] = 0.0f;
        pData->plugins[i].outsPeak[1] = 0.0f;
    }

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.startThread();

    carla_debug("CarlaEngine::removeAllPlugins() - END");
}

const char* CarlaEngine::renamePlugin(const unsigned int id, const char* const newName)
{
    CARLA_ASSERT(pData->curPluginCount != 0);
    CARLA_ASSERT(id < pData->curPluginCount);
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(newName != nullptr);
    carla_debug("CarlaEngine::renamePlugin(%i, \"%s\")", id, newName);

    if (pData->plugins == nullptr || pData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return nullptr;
    }

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    if (plugin == nullptr)
    {
        carla_stderr("CarlaEngine::clonePlugin(%i) - could not find plugin", id);
        return nullptr;
    }

    CARLA_ASSERT(plugin->getId() == id);

    if (const char* const name = getUniquePluginName(newName))
    {
        plugin->setName(name);
        return name;
    }

    return nullptr;
}

bool CarlaEngine::clonePlugin(const unsigned int id)
{
    CARLA_ASSERT(pData->curPluginCount > 0);
    CARLA_ASSERT(id < pData->curPluginCount);
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::clonePlugin(%i)", id);

    if (pData->plugins == nullptr || pData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return false;
    }

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    if (plugin == nullptr)
    {
        carla_stderr("CarlaEngine::clonePlugin(%i) - could not find plugin", id);
        return false;
    }

    CARLA_ASSERT(plugin->getId() == id);

    char label[STR_MAX+1] = { '\0' };
    plugin->getLabel(label);

    const unsigned int pluginCountBefore(pData->curPluginCount);

    if (! addPlugin(plugin->getBinaryType(), plugin->getType(), plugin->getFilename(), plugin->getName(), label, plugin->getExtraStuff()))
        return false;

    CARLA_ASSERT(pluginCountBefore+1 == pData->curPluginCount);

    if (CarlaPlugin* const newPlugin = pData->plugins[pluginCountBefore].plugin)
        newPlugin->loadSaveState(plugin->getSaveState());

    return true;
}

bool CarlaEngine::replacePlugin(const unsigned int id)
{
    CARLA_ASSERT(pData->curPluginCount > 0);
    CARLA_ASSERT(id < pData->curPluginCount);
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    carla_debug("CarlaEngine::replacePlugin(%i)", id);

    if (id < pData->curPluginCount)
        pData->nextPluginId = id;

    return false;
}

bool CarlaEngine::switchPlugins(const unsigned int idA, const unsigned int idB)
{
    CARLA_ASSERT(pData->curPluginCount >= 2);
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(idA != idB);
    CARLA_ASSERT(idA < pData->curPluginCount);
    CARLA_ASSERT(idB < pData->curPluginCount);
    carla_debug("CarlaEngine::switchPlugins(%i)", idA, idB);

    if (pData->plugins == nullptr || pData->curPluginCount == 0)
    {
        setLastError("Critical error: no plugins are currently loaded!");
        return false;
    }

    pData->thread.stopThread(500);

    const bool lockWait(isRunning() && fOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS);
    const CarlaEngineProtectedData::ScopedActionLock sal(pData, kEnginePostActionSwitchPlugins, idA, idB, lockWait);

#ifndef BUILD_BRIDGE // TODO
    //if (isOscControlRegistered())
    //    oscSend_control_switch_plugins(idA, idB);
#endif

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.startThread();

    return true;
}

CarlaPlugin* CarlaEngine::getPlugin(const unsigned int id) const
{
    CARLA_ASSERT(pData->curPluginCount != 0);
    CARLA_ASSERT(id < pData->curPluginCount);
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull); // TESTING, remove later
    carla_debug("CarlaEngine::getPlugin(%i) [count:%i]", id, pData->curPluginCount);

    if (id < pData->curPluginCount && pData->plugins != nullptr)
        return pData->plugins[id].plugin;

    return nullptr;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const unsigned int id) const noexcept
{
    return pData->plugins[id].plugin;
}

const char* CarlaEngine::getUniquePluginName(const char* const name)
{
    CARLA_ASSERT(pData->maxPluginNumber != 0);
    CARLA_ASSERT(pData->plugins != nullptr);
    CARLA_ASSERT(pData->nextAction.opcode == kEnginePostActionNull);
    CARLA_ASSERT(name != nullptr);
    carla_debug("CarlaEngine::getUniquePluginName(\"%s\")", name);

    //static CarlaMutex m;
    //const CarlaMutex::ScopedLocker sl(m);

    static CarlaString sname;
    sname = name;

    if (sname.isEmpty())
    {
        sname = "(No name)";
        return (const char*)sname;
    }

    if (pData->plugins == nullptr || pData->maxPluginNumber == 0)
        return (const char*)sname;

    sname.truncate(getMaxClientNameSize()-5-1); // 5 = strlen(" (10)")
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (unsigned short i=0; i < pData->curPluginCount; ++i)
    {
        CARLA_ASSERT(pData->plugins[i].plugin != nullptr);

        if (pData->plugins[i].plugin == nullptr)
            break;

        // Check if unique name doesn't exist
        if (const char* const pluginName = pData->plugins[i].plugin->getName())
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
    CARLA_ASSERT(filename != nullptr && filename[0] != '\0');
    carla_debug("CarlaEngine::loadFilename(\"%s\")", filename);

    using namespace juce;

    File file(filename);

    if (! file.existsAsFile())
    {
        setLastError("File does not exist");
        return false;
    }

    CarlaString baseName(file.getFileNameWithoutExtension().toRawUTF8());
    CarlaString extension(file.getFileExtension().toRawUTF8()+1);
    extension.toLower();

    carla_stdout("loadFilename with extension %s", (const char*)extension);

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
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
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
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
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
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
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
            if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
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
    CARLA_ASSERT(filename != nullptr && filename[0] != '\0');
    carla_debug("CarlaEngine::loadProject(\"%s\")", filename);

    using namespace juce;

    File file(filename);

    XmlDocument xml(file);

    if (XmlElement* const xmlCheck = xml.getDocumentElement(true))
    {
        const String& tagNameTest(xmlCheck->getTagName());
        const bool    isPreset(tagNameTest.equalsIgnoreCase("carla-preset"));

        if (tagNameTest.equalsIgnoreCase("carla-project") || isPreset)
        {
            if (XmlElement* const xmlElem = xml.getDocumentElement(false))
            {
                for (XmlElement* subElem = xmlElem->getFirstChildElement(); subElem != nullptr; subElem = subElem->getNextElement())
                {
                    if (isPreset || subElem->getTagName().equalsIgnoreCase("Plugin"))
                    {
                        SaveState saveState;
                        fillSaveStateFromXmlElement(saveState, isPreset ? xmlElem : subElem);

                        CARLA_SAFE_ASSERT_CONTINUE(saveState.type != nullptr);

                        const void* extraStuff = nullptr;

                        if (std::strcmp(saveState.type, "SF2") == 0)
                        {
                            const char* const use16OutsSuffix = " (16 outs)";

                            if (charEndsWith(saveState.label, use16OutsSuffix))
                                extraStuff = (void*)0x1; // non-null
                        }

                        // TODO - proper find&load plugins

                        if (addPlugin(getPluginTypeFromString(saveState.type), saveState.binary, saveState.name, saveState.label, extraStuff))
                        {
                            if (CarlaPlugin* plugin = getPlugin(pData->curPluginCount-1))
                                plugin->loadSaveState(saveState);
                        }
                    }

                    if (isPreset)
                        break;
                }

                delete xmlElem;
                delete xmlCheck;
                return true;
            }
            else
                setLastError("Failed to parse file");
        }
        else
            setLastError("Not a valid Carla project or preset file");

        delete xmlCheck;
        return false;
    }

    setLastError("Not a valid file");
    return false;
}

bool CarlaEngine::saveProject(const char* const filename)
{
    CARLA_ASSERT(filename != nullptr && filename[0] != '\0');
    carla_debug("CarlaEngine::saveProject(\"%s\")", filename);

    using namespace juce;

    File file(filename);

    MemoryOutputStream out;
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PROJECT>\n";
    out << "<CARLA-PROJECT VERSION='2.0'>\n";

    bool firstPlugin = true;
    char strBuf[STR_MAX+1];

    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin = pData->plugins[i].plugin;

        if (plugin != nullptr && plugin->isEnabled())
        {
            if (! firstPlugin)
                out << "\n";

            strBuf[0] = '\0';

            plugin->getRealName(strBuf);

            if (strBuf[0] != '\0')
            {
                out << " <!-- ";
                out << xmlSafeString(strBuf, true);
                out << " -->\n";
            }

            String content;
            fillXmlStringFromSaveState(content, plugin->getSaveState());

            out << " <Plugin>\n";
            out << content;
            out << " </Plugin>\n";

            firstPlugin = false;
        }
    }

    out << "</CARLA-PROJECT>\n";

    return file.replaceWithData(out.getData(), out.getDataSize());
}

// -----------------------------------------------------------------------
// Information (peaks)

// FIXME

float CarlaEngine::getInputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < pData->curPluginCount);
    CARLA_ASSERT(id-1 < 2);

    if (id == 0 || id > 2)
        return 0.0f;

    return pData->plugins[pluginId].insPeak[id-1];
}

float CarlaEngine::getOutputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < pData->curPluginCount);
    CARLA_ASSERT(id-1 < 2);

    if (id == 0 || id > 2)
        return 0.0f;

    return pData->plugins[pluginId].outsPeak[id-1];
}

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const CallbackType action, const unsigned int pluginId, const int value1, const int value2, const float value3, const char* const valueStr)
{
    carla_debug("CarlaEngine::callback(%s, %i, %i, %i, %f, \"%s\")", CallbackType2Str(action), pluginId, value1, value2, value3, valueStr);

    if (pData->callback != nullptr)
        pData->callback(pData->callbackPtr, action, pluginId, value1, value2, value3, valueStr);
}

void CarlaEngine::setCallback(const CallbackFunc func, void* const ptr)
{
    CARLA_ASSERT(func != nullptr);
    carla_debug("CarlaEngine::setCallback(%p, %p)", func, ptr);

    pData->callback    = func;
    pData->callbackPtr = ptr;
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
    pData->time.playing = true;
}

void CarlaEngine::transportPause()
{
    pData->time.playing = false;
}

void CarlaEngine::transportRelocate(const uint32_t frame)
{
    pData->time.frame = frame;
}

// -----------------------------------------------------------------------
// Error handling

const char* CarlaEngine::getLastError() const noexcept
{
    return (const char*)pData->lastError;
}

void CarlaEngine::setLastError(const char* const error)
{
    pData->lastError = error;
}

void CarlaEngine::setAboutToClose()
{
    carla_debug("CarlaEngine::setAboutToClose()");
    pData->aboutToClose = true;
}

// -----------------------------------------------------------------------
// Global options

void CarlaEngine::setOption(const OptionsType option, const int value, const char* const valueStr)
{
    carla_debug("CarlaEngine::setOption(%s, %i, \"%s\")", OptionsType2Str(option), value, valueStr);

#ifndef BUILD_BRIDGE
    if (option >= OPTION_PROCESS_MODE && option < OPTION_PATH_RESOURCES && isRunning())
        return carla_stderr("CarlaEngine::setOption(%s, %i, \"%s\") - Cannot set this option while engine is running!", OptionsType2Str(option), value, valueStr);
#endif

    switch (option)
    {
    case OPTION_PROCESS_NAME:
        break;

    case OPTION_PROCESS_MODE:
        if (value < PROCESS_MODE_SINGLE_CLIENT || value > PROCESS_MODE_PATCHBAY)
            return carla_stderr("CarlaEngine::setOption(OPTION_PROCESS_MODE, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.processMode = static_cast<ProcessMode>(value);
        break;

    case OPTION_TRANSPORT_MODE:
        if (value < CarlaBackend::TRANSPORT_MODE_INTERNAL || value > CarlaBackend::TRANSPORT_MODE_JACK)
            return carla_stderr2("carla_set_engine_option(OPTION_TRANSPORT_MODE, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.transportMode = static_cast<CarlaBackend::TransportMode>(value);
        break;

    case OPTION_FORCE_STEREO:
        fOptions.forceStereo = (value != 0);
        break;

    case OPTION_PREFER_PLUGIN_BRIDGES:
        fOptions.preferPluginBridges = (value != 0);
        break;

    case OPTION_PREFER_UI_BRIDGES:
        fOptions.preferUiBridges = (value != 0);
        break;

    case OPTION_UIS_ALWAYS_ON_TOP:
        fOptions.uisAlwaysOnTop = (value != 0);
        break;

    case OPTION_MAX_PARAMETERS:
        if (value < 1)
            return carla_stderr2("carla_set_engine_option(OPTION_MAX_PARAMETERS, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.maxParameters = static_cast<uint>(value);
        break;

    case OPTION_UI_BRIDGES_TIMEOUT:
        if (value < 1)
            return carla_stderr2("carla_set_engine_option(OPTION_UI_BRIDGES_TIMEOUT, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.uiBridgesTimeout = static_cast<uint>(value);
        break;

    case OPTION_AUDIO_NUM_PERIODS:
        if (value < 2 || value > 3)
            return carla_stderr2("carla_set_engine_option(OPTION_AUDIO_NUM_PERIODS, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.audioNumPeriods = static_cast<uint>(value);
        break;

    case OPTION_AUDIO_BUFFER_SIZE:
        if (value < 8)
            return carla_stderr2("carla_set_engine_option(OPTION_AUDIO_BUFFER_SIZE, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.audioBufferSize = static_cast<uint>(value);
        break;

    case OPTION_AUDIO_SAMPLE_RATE:
        if (value < 22050)
            return carla_stderr2("carla_set_engine_option(OPTION_AUDIO_SAMPLE_RATE, %i, \"%s\") - invalid value", value, valueStr);

        fOptions.audioSampleRate = static_cast<uint>(value);
        break;

    case OPTION_AUDIO_DEVICE:
        fOptions.audioDevice = valueStr;
        break;

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
    case OPTION_PATH_BRIDGE_LV2_EXTERNAL:
        fOptions.bridge_lv2Extrn = valueStr;
        break;
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
    case OPTION_PATH_BRIDGE_VST_MAC:
        fOptions.bridge_vstMac = valueStr;
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
bool CarlaEngine::isOscBridgeRegistered() const noexcept
{
    return (pData->oscData != nullptr);
}
#else
bool CarlaEngine::isOscControlRegistered() const noexcept
{
    return pData->osc.isControlRegistered();
}
#endif

void CarlaEngine::idleOsc()
{
    pData->osc.idle();
}

const char* CarlaEngine::getOscServerPathTCP() const noexcept
{
    return pData->osc.getServerPathTCP();
}

const char* CarlaEngine::getOscServerPathUDP() const noexcept
{
    return pData->osc.getServerPathUDP();
}

#ifdef BUILD_BRIDGE
void CarlaEngine::setOscBridgeData(const CarlaOscData* const oscData) noexcept
{
    pData->oscData = oscData;
}
#endif

// -----------------------------------------------------------------------
// protected calls

void CarlaEngine::bufferSizeChanged(const uint32_t newBufferSize)
{
    carla_debug("CarlaEngine::bufferSizeChanged(%i)", newBufferSize);

    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->bufferSizeChanged(newBufferSize);
    }

    callback(CALLBACK_BUFFER_SIZE_CHANGED, 0, newBufferSize, 0, 0.0f, nullptr);
}

void CarlaEngine::sampleRateChanged(const double newSampleRate)
{
    carla_debug("CarlaEngine::sampleRateChanged(%g)", newSampleRate);

    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->sampleRateChanged(newSampleRate);
    }

    callback(CALLBACK_SAMPLE_RATE_CHANGED, 0, 0, 0, newSampleRate, nullptr);
}

void CarlaEngine::offlineModeChanged(const bool isOffline)
{
    carla_debug("CarlaEngine::offlineModeChanged(%s)", bool2str(isOffline));

    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->offlineModeChanged(isOffline);
    }
}

void CarlaEngine::runPendingRtEvents()
{
    pData->doNextPluginAction(true);

    if (pData->time.playing)
        pData->time.frame += fBufferSize;

    if (fOptions.transportMode == CarlaBackend::TRANSPORT_MODE_INTERNAL)
    {
        fTimeInfo.playing = pData->time.playing;
        fTimeInfo.frame   = pData->time.frame;
    }

    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        // TODO - peak values?
    }
}

void CarlaEngine::setPluginPeaks(const unsigned int pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept
{
    pData->plugins[pluginId].insPeak[0]  = inPeaks[0];
    pData->plugins[pluginId].insPeak[1]  = inPeaks[1];
    pData->plugins[pluginId].outsPeak[0] = outPeaks[0];
    pData->plugins[pluginId].outsPeak[1] = outPeaks[1];
}

EngineEvent* CarlaEngine::getInternalEventBuffer(const bool isInput) const noexcept
{
    return isInput ? pData->bufEvents.in : pData->bufEvents.out;
}

void CarlaEngine::registerEnginePlugin(const unsigned int id, CarlaPlugin* const plugin)
{
    CARLA_ASSERT(id == pData->curPluginCount);

    if (id == pData->curPluginCount)
        pData->plugins[id].plugin = plugin;
}

#ifndef BUILD_BRIDGE
void setValueIfHigher(float& value, const float& compare)
{
    if (value < compare)
        value = compare;
}

void CarlaEngine::processRack(float* inBufReal[2], float* outBuf[2], const uint32_t frames)
{
    CARLA_ASSERT(pData->bufEvents.in != nullptr);
    CARLA_ASSERT(pData->bufEvents.out != nullptr);

    // safe copy
    float inBuf0[frames];
    float inBuf1[frames];
    float* inBuf[2] = { inBuf0, inBuf1 };

    // initialize inputs
    FloatVectorOperations::copy(inBuf0, inBufReal[0], frames);
    FloatVectorOperations::copy(inBuf1, inBufReal[1], frames);

    // initialize outputs (zero)
    FloatVectorOperations::clear(outBuf[0], frames);
    FloatVectorOperations::clear(outBuf[1], frames);
    carla_zeroMem(pData->bufEvents.out, sizeof(EngineEvent)*kEngineMaxInternalEventCount);

    bool processed = false;

    uint32_t oldAudioInCount = 0;
    uint32_t oldMidiOutCount = 0;

    // process plugins
    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin = pData->plugins[i].plugin;

        if (plugin == nullptr || ! plugin->isEnabled() || ! plugin->tryLock())
            continue;

        if (processed)
        {
            // initialize inputs (from previous outputs)
            FloatVectorOperations::copy(inBuf0, outBuf[0], frames);
            FloatVectorOperations::copy(inBuf1, outBuf[1], frames);

            // initialize outputs (zero)
            FloatVectorOperations::clear(outBuf[0], frames);
            FloatVectorOperations::clear(outBuf[1], frames);

            // if plugin has no midi out, add previous events
            if (oldMidiOutCount == 0 && pData->bufEvents.in[0].type != CarlaBackend::kEngineEventTypeNull)
            {
                if (pData->bufEvents.out[0].type != CarlaBackend::kEngineEventTypeNull)
                {
                    // TODO: carefully add to input, sorted events
                }
                // else nothing needed
            }
            else
            {
                // initialize input from previous output and zero output
                std::memcpy(pData->bufEvents.in, pData->bufEvents.out, sizeof(EngineEvent)*kEngineMaxInternalEventCount);
                std::memset(pData->bufEvents.out, 0, sizeof(EngineEvent)*kEngineMaxInternalEventCount);
            }
        }

        oldAudioInCount = plugin->getAudioInCount();
        oldMidiOutCount = plugin->getMidiOutCount();
        // process
        plugin->initBuffers();
        plugin->process(inBuf, outBuf, frames);
        plugin->unlock();

        // if plugin has no audio inputs, add input buffer
        if (oldAudioInCount == 0)
        {
            FloatVectorOperations::add(outBuf[0], inBuf0, frames);
            FloatVectorOperations::add(outBuf[1], inBuf1, frames);
        }

        // set peaks
        {
            float inPeak1  = 0.0f;
            float inPeak2  = 0.0f;
            float outPeak1 = 0.0f;
            float outPeak2 = 0.0f;

            for (uint32_t k=0; k < frames; ++k)
            {
                setValueIfHigher(inPeak1,  std::fabs(inBuf0[k]));
                setValueIfHigher(inPeak2,  std::fabs(inBuf1[k]));
                setValueIfHigher(outPeak1, std::fabs(outBuf[0][k]));
                setValueIfHigher(outPeak2, std::fabs(outBuf[1][k]));
            }

            pData->plugins[i].insPeak[0]  = inPeak1;
            pData->plugins[i].insPeak[1]  = inPeak2;
            pData->plugins[i].outsPeak[0] = outPeak1;
            pData->plugins[i].outsPeak[1] = outPeak2;
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

// -----------------------------------------------------------------------
// Carla Engine OSC stuff

#ifndef BUILD_BRIDGE
void CarlaEngine::oscSend_control_add_plugin_start(const int32_t pluginId, const char* const pluginName)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(pluginName != nullptr);
    carla_debug("CarlaEngine::oscSend_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/add_plugin_start");
        lo_send(pData->oscData->target, targetPath, "is", pluginId, pluginName);
    }
}

void CarlaEngine::oscSend_control_add_plugin_end(const int32_t pluginId)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    carla_debug("CarlaEngine::oscSend_control_add_plugin_end(%i)", pluginId);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+16];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/add_plugin_end");
        lo_send(pData->oscData->target, targetPath, "i", pluginId);
    }
}

void CarlaEngine::oscSend_control_remove_plugin(const int32_t pluginId)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->curPluginCount));
    carla_debug("CarlaEngine::oscSend_control_remove_plugin(%i)", pluginId);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+15];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/remove_plugin");
        lo_send(pData->oscData->target, targetPath, "i", pluginId);
    }
}

void CarlaEngine::oscSend_control_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints, const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(type != PLUGIN_NONE);
    CARLA_ASSERT(realName != nullptr);
    CARLA_ASSERT(label != nullptr);
    CARLA_ASSERT(maker != nullptr);
    CARLA_ASSERT(copyright != nullptr);
    carla_debug("CarlaEngine::oscSend_control_set_plugin_data(%i, %i, %i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+17];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_plugin_data");
        lo_send(pData->oscData->target, targetPath, "iiiissssh", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::oscSend_control_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts, const int32_t cIns, const int32_t cOuts, const int32_t cTotals)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    carla_debug("CarlaEngine::oscSend_control_set_plugin_ports(%i, %i, %i, %i, %i, %i, %i, %i)", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_plugin_ports");
        lo_send(pData->oscData->target, targetPath, "iiiiiiii", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    }
}

void CarlaEngine::oscSend_control_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints, const char* const name, const char* const label, const float current)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(type != PARAMETER_UNKNOWN);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(label != nullptr);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_data(%i, %i, %i, %i, \"%s\", \"%s\", %f)", pluginId, index, type, hints, name, label, current);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_data");
        lo_send(pData->oscData->target, targetPath, "iiiissf", pluginId, index, type, hints, name, label, current);
    }
}

void CarlaEngine::oscSend_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const float min, const float max, const float def, const float step, const float stepSmall, const float stepLarge)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(min < max);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_ranges(%i, %i, %f, %f, %f, %f, %f, %f)", pluginId, index, min, max, def, step, stepSmall, stepLarge);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+22];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_ranges");
        lo_send(pData->oscData->target, targetPath, "iiffffff", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::oscSend_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_midi_cc");
        lo_send(pData->oscData->target, targetPath, "iii", pluginId, index, cc);
    }
}

void CarlaEngine::oscSend_control_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+28];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_midi_channel");
        lo_send(pData->oscData->target, targetPath, "iii", pluginId, index, channel);
    }
}

void CarlaEngine::oscSend_control_set_parameter_value(const int32_t pluginId, const int32_t index, const float value)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
#if DEBUG
    if (index < 0)
        carla_debug("CarlaEngine::oscSend_control_set_parameter_value(%i, %s, %f)", pluginId, InternalParametersIndex2Str((InternalParametersIndex)index), value);
    else
        carla_debug("CarlaEngine::oscSend_control_set_parameter_value(%i, %i, %f)", pluginId, index, value);
#endif

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+21];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_value");
        lo_send(pData->oscData->target, targetPath, "iif", pluginId, index, value);
    }
}

void CarlaEngine::oscSend_control_set_default_value(const int32_t pluginId, const int32_t index, const float value)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    carla_debug("CarlaEngine::oscSend_control_set_default_value(%i, %i, %f)", pluginId, index, value);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+19];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_default_value");
        lo_send(pData->oscData->target, targetPath, "iif", pluginId, index, value);
    }
}

void CarlaEngine::oscSend_control_set_program(const int32_t pluginId, const int32_t index)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    carla_debug("CarlaEngine::oscSend_control_set_program(%i, %i)", pluginId, index);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+13];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_program");
        lo_send(pData->oscData->target, targetPath, "ii", pluginId, index);
    }
}

void CarlaEngine::oscSend_control_set_program_count(const int32_t pluginId, const int32_t count)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::oscSend_control_set_program_count(%i, %i)", pluginId, count);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+19];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_program_count");
        lo_send(pData->oscData->target, targetPath, "ii", pluginId, count);
    }
}

void CarlaEngine::oscSend_control_set_program_name(const int32_t pluginId, const int32_t index, const char* const name)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(name != nullptr);
    carla_debug("CarlaEngine::oscSend_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_program_name");
        lo_send(pData->oscData->target, targetPath, "iis", pluginId, index, name);
    }
}

void CarlaEngine::oscSend_control_set_midi_program(const int32_t pluginId, const int32_t index)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    carla_debug("CarlaEngine::oscSend_control_set_midi_program(%i, %i)", pluginId, index);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_midi_program");
        lo_send(pData->oscData->target, targetPath, "ii", pluginId, index);
    }
}

void CarlaEngine::oscSend_control_set_midi_program_count(const int32_t pluginId, const int32_t count)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_count(%i, %i)", pluginId, count);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+24];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_midi_program_count");
        lo_send(pData->oscData->target, targetPath, "ii", pluginId, count);
    }
}

void CarlaEngine::oscSend_control_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->maxPluginNumber));
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(bank >= 0);
    CARLA_ASSERT(program >= 0);
    CARLA_ASSERT(name != nullptr);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_midi_program_data");
        lo_send(pData->oscData->target, targetPath, "iiiis", pluginId, index, bank, program, name);
    }
}

void CarlaEngine::oscSend_control_note_on(const int32_t pluginId, const int32_t channel, const int32_t note, const int32_t velo)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->curPluginCount));
    CARLA_ASSERT(channel >= 0 && channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note >= 0 && note < MAX_MIDI_NOTE);
    CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);
    carla_debug("CarlaEngine::oscSend_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+9];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/note_on");
        lo_send(pData->oscData->target, targetPath, "iiii", pluginId, channel, note, velo);
    }
}

void CarlaEngine::oscSend_control_note_off(const int32_t pluginId, const int32_t channel, const int32_t note)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->curPluginCount));
    CARLA_ASSERT(channel >= 0 && channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note >= 0 && note < MAX_MIDI_NOTE);
    carla_debug("CarlaEngine::oscSend_control_note_off(%i, %i, %i)", pluginId, channel, note);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+10];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/note_off");
        lo_send(pData->oscData->target, targetPath, "iii", pluginId, channel, note);
    }
}

void CarlaEngine::oscSend_control_set_peaks(const int32_t pluginId)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(pluginId >= 0 && pluginId < static_cast<int32_t>(pData->curPluginCount));

    const EnginePluginData& epData(pData->plugins[pluginId]);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+22];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_peaks");
        lo_send(pData->oscData->target, targetPath, "iffff", pluginId, epData.insPeak[0], epData.insPeak[1], epData.outsPeak[0], epData.outsPeak[1]);
    }
}

void CarlaEngine::oscSend_control_exit()
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_control_exit()");

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+6];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/exit");
        lo_send(pData->oscData->target, targetPath, "");
    }
}
#else
void CarlaEngine::oscSend_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);
    carla_debug("CarlaEngine::oscSend_bridge_audio_count(%i, %i, %i)", ins, outs, total);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_audio_count");
        lo_send(pData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::oscSend_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);
    carla_debug("CarlaEngine::oscSend_bridge_midi_count(%i, %i, %i)", ins, outs, total);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+19];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_count");
        lo_send(pData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::oscSend_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_count(%i, %i, %i)", ins, outs, total);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+24];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_count");
        lo_send(pData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::oscSend_bridge_program_count(const int32_t count)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::oscSend_bridge_program_count(%i)", count);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+22];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_program_count");
        lo_send(pData->oscData->target, targetPath, "i", count);
    }
}

void CarlaEngine::oscSend_bridge_midi_program_count(const int32_t count)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(count >= 0);
    carla_debug("CarlaEngine::oscSend_bridge_midi_program_count(%i)", count);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+27];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_program_count");
        lo_send(pData->oscData->target, targetPath, "i", count);
    }
}

void CarlaEngine::oscSend_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(label != nullptr);
    CARLA_ASSERT(maker != nullptr);
    CARLA_ASSERT(copyright != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_plugin_info(%i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", category, hints, name, label, maker, copyright, uniqueId);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_plugin_info");
        lo_send(pData->oscData->target, targetPath, "iissssh", category, hints, name, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::oscSend_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(name != nullptr);
    CARLA_ASSERT(unit != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_info(%i, \"%s\", \"%s\")", index, name, unit);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_info");
        lo_send(pData->oscData->target, targetPath, "iss", index, name, unit);
    }
}

void CarlaEngine::oscSend_bridge_parameter_data(const int32_t index, const int32_t type, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_data(%i, %i, %i, %i, %i, %i)", index, type, rindex, hints, midiChannel, midiCC);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_data");
        lo_send(pData->oscData->target, targetPath, "iiiiii", index, type, rindex, hints, midiChannel, midiCC);
    }
}

void CarlaEngine::oscSend_bridge_parameter_ranges(const int32_t index, const float def, const float min, const float max, const float step, const float stepSmall, const float stepLarge)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_ranges(%i, %f, %f, %f, %f, %f, %f)", index, def, min, max, step, stepSmall, stepLarge);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+25];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_ranges");
        lo_send(pData->oscData->target, targetPath, "iffffff", index, def, min, max, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::oscSend_bridge_program_info(const int32_t index, const char* const name)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_program_info(%i, \"%s\")", index, name);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+21];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_program_info");
        lo_send(pData->oscData->target, targetPath, "is", index, name);
    }
}

void CarlaEngine::oscSend_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_midi_program_info(%i, %i, %i, \"%s\")", index, bank, program, label);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+26];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_program_info");
        lo_send(pData->oscData->target, targetPath, "iiis", index, bank, program, label);
    }
}

void CarlaEngine::oscSend_bridge_configure(const char* const key, const char* const value)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    CARLA_ASSERT(key != nullptr);
    CARLA_ASSERT(value != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_configure(\"%s\", \"%s\")", key, value);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_configure");
        lo_send(pData->oscData->target, targetPath, "ss", key, value);
    }
}

void CarlaEngine::oscSend_bridge_set_parameter_value(const int32_t index, const float value)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_set_parameter_value(%i, %f)", index, value);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+28];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_parameter_value");
        lo_send(pData->oscData->target, targetPath, "if", index, value);
    }
}

void CarlaEngine::oscSend_bridge_set_default_value(const int32_t index, const float value)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_set_default_value(%i, %f)", index, value);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+26];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_default_value");
        lo_send(pData->oscData->target, targetPath, "if", index, value);
    }
}

void CarlaEngine::oscSend_bridge_set_program(const int32_t index)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_set_program(%i)", index);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_program");
        lo_send(pData->oscData->target, targetPath, "i", index);
    }
}

void CarlaEngine::oscSend_bridge_set_midi_program(const int32_t index)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_set_midi_program(%i)", index);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+25];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_midi_program");
        lo_send(pData->oscData->target, targetPath, "i", index);
    }
}

void CarlaEngine::oscSend_bridge_set_custom_data(const char* const type, const char* const key, const char* const value)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_set_custom_data(\"%s\", \"%s\", \"%s\")", type, key, value);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+24];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_custom_data");
        lo_send(pData->oscData->target, targetPath, "sss", type, key, value);
    }
}

void CarlaEngine::oscSend_bridge_set_chunk_data(const char* const chunkFile)
{
    CARLA_ASSERT(pData->oscData != nullptr);
    carla_debug("CarlaEngine::oscSend_bridge_set_chunk_data(\"%s\")", chunkFile);

    if (pData->oscData != nullptr && pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_chunk_data");
        lo_send(pData->oscData->target, targetPath, "s", chunkFile);
    }
}
#endif

CARLA_BACKEND_END_NAMESPACE
