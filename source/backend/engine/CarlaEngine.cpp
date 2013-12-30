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

/* TODO:
 * - add more checks to oscSend_* stuff
 * - complete processRack(): carefully add to input, sorted events
 * - implement processPatchbay()
 * - implement oscSend_control_switch_plugins()
 * - proper find&load plugins
 * - uncomment CarlaPlugin::newAU and newCSOUND
 * - something about the peaks?
 */

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaStateUtils.hpp"
#include "CarlaMIDI.h"

#include <cmath>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Fallback data

static const EngineEvent kFallbackEngineEvent = { kEngineEventTypeNull, 0, 0, { kEngineControlEventTypeNull, 0, 0.0f } };

// -----------------------------------------------------------------------

void EngineControlEvent::dumpToMidiData(const uint8_t channel, uint8_t& size, uint8_t data[3]) const noexcept
{
    switch (type)
    {
    case kEngineControlEventTypeNull:
        break;

    case kEngineControlEventTypeParameter:
        if (MIDI_IS_CONTROL_BANK_SELECT(param))
        {
            size    = 3;
            data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + channel);
            data[1] = MIDI_CONTROL_BANK_SELECT;
            data[2] = static_cast<uint8_t>(value);
        }
        else
        {
            size    = 3;
            data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + channel);
            data[1] = static_cast<uint8_t>(param);
            data[2] = uint8_t(value * 127.0f);
        }
        break;

    case kEngineControlEventTypeMidiBank:
        size    = 3;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + channel);
        data[1] = MIDI_CONTROL_BANK_SELECT;
        data[2] = static_cast<uint8_t>(param);
        break;

    case kEngineControlEventTypeMidiProgram:
        size    = 2;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_PROGRAM_CHANGE + channel);
        data[1] = static_cast<uint8_t>(param);
        break;

    case kEngineControlEventTypeAllSoundOff:
        size    = 2;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + channel);
        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
        break;

    case kEngineControlEventTypeAllNotesOff:
        size    = 2;
        data[0] = static_cast<uint8_t>(MIDI_STATUS_CONTROL_CHANGE + channel);
        data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
        break;
    }
}

void EngineEvent::fillFromMidiData(const uint8_t size, uint8_t* const data)
{
    // get channel
    channel = MIDI_GET_CHANNEL_FROM_DATA(data);

    // get status
    const uint8_t midiStatus(MIDI_GET_STATUS_FROM_DATA(data));

    // remove channel bit from data
    data[0] = midiStatus;

    if (midiStatus == MIDI_STATUS_CONTROL_CHANGE)
    {
        type = kEngineEventTypeControl;

        const uint8_t midiControl(data[1]);

        if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
        {
            CARLA_SAFE_ASSERT_INT(size == 3, size);

            const uint8_t midiBank(data[2]);

            ctrl.type  = kEngineControlEventTypeMidiBank;
            ctrl.param = midiBank;
            ctrl.value = 0.0f;
        }
        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
        {
            CARLA_SAFE_ASSERT_INT(size == 2, size);

            ctrl.type  = kEngineControlEventTypeAllSoundOff;
            ctrl.param = 0;
            ctrl.value = 0.0f;
        }
        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
        {
            CARLA_SAFE_ASSERT_INT(size == 2, size);

            ctrl.type  = kEngineControlEventTypeAllNotesOff;
            ctrl.param = 0;
            ctrl.value = 0.0f;
        }
        else
        {
            CARLA_SAFE_ASSERT_INT2(size == 3, size, midiControl);

            const uint8_t midiValue(data[2]);

            ctrl.type  = kEngineControlEventTypeParameter;
            ctrl.param = midiControl;
            ctrl.value = float(midiValue)/127.0f;
        }
    }
    else if (midiStatus == MIDI_STATUS_PROGRAM_CHANGE)
    {
        CARLA_SAFE_ASSERT_INT2(size == 2, size, data[1]);

        type = kEngineEventTypeControl;

        const uint8_t midiProgram(data[1]);

        ctrl.type  = kEngineControlEventTypeMidiProgram;
        ctrl.param = midiProgram;
        ctrl.value = 0.0f;
    }
    else
    {
        type = kEngineEventTypeMidi;

        midi.port = 0;
        midi.size = size;

        if (size > EngineMidiEvent::kDataSize)
        {
            midi.dataExt = data;
            std::memset(midi.data, 0, sizeof(uint8_t)*EngineMidiEvent::kDataSize);
        }
        else
        {
            midi.data[0] = midiStatus;

            uint8_t i=1;
            for (; i < midi.size; ++i)
                midi.data[i] = data[i];
            for (; i < EngineMidiEvent::kDataSize; ++i)
                midi.data[i] = 0;

            midi.dataExt = nullptr;
        }
    }
}

// -----------------------------------------------------------------------
// Carla Engine port (Abstract)

CarlaEnginePort::CarlaEnginePort(const CarlaEngine& engine, const bool isInput)
    : fEngine(engine),
      fIsInput(isInput)
{
    carla_debug("CarlaEnginePort::CarlaEnginePort(%s)", bool2str(isInput));
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
    carla_debug("CarlaEngineAudioPort::CarlaEngineAudioPort(%s)", bool2str(isInput));
}

CarlaEngineAudioPort::~CarlaEngineAudioPort()
{
    carla_debug("CarlaEngineAudioPort::~CarlaEngineAudioPort()");
}

void CarlaEngineAudioPort::initBuffer()
{
}

// -----------------------------------------------------------------------
// Carla Engine CV port

CarlaEngineCVPort::CarlaEngineCVPort(const CarlaEngine& engine, const bool isInput)
    : CarlaEnginePort(engine, isInput),
      fBuffer(nullptr),
      fProcessMode(engine.getProccessMode())
{
    carla_debug("CarlaEngineCVPort::CarlaEngineCVPort(%s)", bool2str(isInput));

    if (fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
        fBuffer = new float[engine.getBufferSize()];
}

CarlaEngineCVPort::~CarlaEngineCVPort()
{
    carla_debug("CarlaEngineCVPort::~CarlaEngineCVPort()");

    if (fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        delete[] fBuffer;
        fBuffer = nullptr;
    }
}

void CarlaEngineCVPort::initBuffer()
{
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS,);

    FLOAT_CLEAR(fBuffer, fEngine.getBufferSize());
}

void CarlaEngineCVPort::setBufferSize(const uint32_t bufferSize)
{
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS,);

    delete[] fBuffer;
    fBuffer = new float[bufferSize];
}

// -----------------------------------------------------------------------
// Carla Engine Event port

CarlaEngineEventPort::CarlaEngineEventPort(const CarlaEngine& engine, const bool isInput)
    : CarlaEnginePort(engine, isInput),
      fBuffer(nullptr),
      fProcessMode(engine.getProccessMode())
{
    carla_debug("CarlaEngineEventPort::CarlaEngineEventPort(%s)", bool2str(isInput));

    if (fProcessMode == ENGINE_PROCESS_MODE_PATCHBAY)
        fBuffer = new EngineEvent[kEngineMaxInternalEventCount];
}

CarlaEngineEventPort::~CarlaEngineEventPort()
{
    carla_debug("CarlaEngineEventPort::~CarlaEngineEventPort()");

    if (fProcessMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        delete[] fBuffer;
        fBuffer = nullptr;
    }
}

void CarlaEngineEventPort::initBuffer()
{
    if (fProcessMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || fProcessMode == ENGINE_PROCESS_MODE_BRIDGE)
        fBuffer = fEngine.getInternalEventBuffer(fIsInput);
    else if (fProcessMode == ENGINE_PROCESS_MODE_PATCHBAY && ! fIsInput)
        carla_zeroStruct<EngineEvent>(fBuffer, kEngineMaxInternalEventCount);
}

uint32_t CarlaEngineEventPort::getEventCount() const
{
    CARLA_SAFE_ASSERT_RETURN(fIsInput, 0);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, 0);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, 0);

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
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, kFallbackEngineEvent);
    CARLA_SAFE_ASSERT_RETURN(index < kEngineMaxInternalEventCount, kFallbackEngineEvent);

    return fBuffer[index];
}

const EngineEvent& CarlaEngineEventPort::getEventUnchecked(const uint32_t index)
{
    return fBuffer[index];
}

bool CarlaEngineEventPort::writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value)
{
    CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, false);
    CARLA_SAFE_ASSERT_RETURN(type != kEngineControlEventTypeNull, false);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
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

        return true;
    }

    carla_stderr2("CarlaEngineEventPort::writeControlEvent() - buffer full");
    return false;
}

bool CarlaEngineEventPort::writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t size, const uint8_t* const data)
{
    CARLA_SAFE_ASSERT_RETURN(! fIsInput, false);
    CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(fProcessMode != ENGINE_PROCESS_MODE_SINGLE_CLIENT && fProcessMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, false);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, false);
    CARLA_SAFE_ASSERT_RETURN(size > 0 && size <= EngineMidiEvent::kDataSize, false);
    CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

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

        event.midi.data[0] = MIDI_GET_STATUS_FROM_DATA(data);

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

// -----------------------------------------------------------------------
// Carla Engine client (Abstract)

CarlaEngineClient::CarlaEngineClient(const CarlaEngine& engine)
    : fEngine(engine),
      fActive(false),
      fLatency(0)
{
    carla_debug("CarlaEngineClient::CarlaEngineClient()");
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
    }

    carla_stderr("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
    return nullptr;
}

// -----------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine()
    : pData(new CarlaEngineProtectedData(this))
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
# ifdef HAVE_JUCE
    count += getJuceApiCount();
# endif
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

# ifdef HAVE_JUCE
    const unsigned int juceIndex(index-rtAudioIndex-1);

    if (juceIndex < getJuceApiCount())
        return getJuceApiName(juceIndex);
# endif
#endif

    carla_stderr("CarlaEngine::getDriverName(%i) - invalid index", index);
    return nullptr;
}

const char* const* CarlaEngine::getDriverDeviceNames(const unsigned int index)
{
    carla_debug("CarlaEngine::getDriverDeviceNames(%i)", index);

    if (index == 0) // JACK
    {
        static const char* ret[3] = { "Auto-Connect OFF", "Auto-Connect ON", nullptr };
        return ret;
    }

#ifndef BUILD_BRIDGE
    const unsigned int rtAudioIndex(index-1);

    if (rtAudioIndex < getRtAudioApiCount())
        return getRtAudioApiDeviceNames(rtAudioIndex);

# ifdef HAVE_JUCE
    const unsigned int juceIndex(index-rtAudioIndex-1);

    if (juceIndex < getJuceApiCount())
        return getJuceApiDeviceNames(juceIndex);
# endif
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i) - invalid index", index);
    return nullptr;
}

const EngineDriverDeviceInfo* CarlaEngine::getDriverDeviceInfo(const unsigned int index, const char* const deviceName)
{
    carla_debug("CarlaEngine::getDriverDeviceInfo(%i, \"%s\")", index, deviceName);

    if (index == 0) // JACK
    {
        static uint32_t bufSizes[11] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 0 };
        static EngineDriverDeviceInfo devInfo;
        devInfo.hints       = ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE;
        devInfo.bufferSizes = bufSizes;
        devInfo.sampleRates = nullptr;
        return &devInfo;
    }

#ifndef BUILD_BRIDGE
    const unsigned int rtAudioIndex(index-1);

    if (rtAudioIndex < getRtAudioApiCount())
        return getRtAudioDeviceInfo(rtAudioIndex, deviceName);

# ifdef HAVE_JUCE
    const unsigned int juceIndex(index-rtAudioIndex-1);

    if (juceIndex < getJuceApiCount())
        return getJuceDeviceInfo(juceIndex, deviceName);
# endif
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i, \"%s\") - invalid index", index, deviceName);
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
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->name.isEmpty(), "Invalid engine internal data (err #1)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->oscData == nullptr, "Invalid engine internal data (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins == nullptr, "Invalid engine internal data (err #3)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->bufEvents.in  == nullptr, "Invalid engine internal data (err #4)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->bufEvents.out == nullptr, "Invalid engine internal data (err #5)");
    CARLA_SAFE_ASSERT_RETURN_ERR(clientName != nullptr && clientName[0] != '\0', "Invalid client name");
    carla_debug("CarlaEngine::init(\"%s\")", clientName);

    pData->aboutToClose    = false;
    pData->curPluginCount  = 0;
    pData->maxPluginNumber = 0;
    pData->nextPluginId    = 0;

    switch (pData->options.processMode)
    {
    case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
    case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
        pData->maxPluginNumber = MAX_DEFAULT_PLUGINS;
        break;

    case ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
        pData->maxPluginNumber = MAX_RACK_PLUGINS;
        pData->bufEvents.in    = new EngineEvent[kEngineMaxInternalEventCount];
        pData->bufEvents.out   = new EngineEvent[kEngineMaxInternalEventCount];
        break;

    case ENGINE_PROCESS_MODE_PATCHBAY:
        pData->maxPluginNumber = MAX_PATCHBAY_PLUGINS;
        break;

    case ENGINE_PROCESS_MODE_BRIDGE:
        pData->maxPluginNumber = 1;
        pData->bufEvents.in    = new EngineEvent[kEngineMaxInternalEventCount];
        pData->bufEvents.out   = new EngineEvent[kEngineMaxInternalEventCount];
        break;
    }

    CARLA_SAFE_ASSERT_RETURN_ERR(pData->maxPluginNumber != 0, "Invalid engine process mode");

    pData->nextPluginId = pData->maxPluginNumber;

    pData->name = clientName;
    pData->name.toBasic();

    pData->timeInfo.clear();

    pData->plugins = new EnginePluginData[pData->maxPluginNumber];

    pData->osc.init(clientName);
#ifndef BUILD_BRIDGE
    pData->oscData = pData->osc.getControlData();
#endif

    pData->nextAction.ready();
    pData->thread.start();

    callback(ENGINE_CALLBACK_ENGINE_STARTED, 0, 0, 0, 0.0f, getCurrentDriverName());

    return true;
}

bool CarlaEngine::close()
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->name.isNotEmpty(), "Invalid engine internal data (err #6)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #7)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId == pData->maxPluginNumber, "Invalid engine internal data (err #8)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #9)");
    carla_debug("CarlaEngine::close()");

    pData->thread.stop(500);
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

    pData->name.clear();

    callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);

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
            plugin->idle();
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
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #10)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId <= pData->maxPluginNumber, "Invalid engine internal data (err #11)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #12)");
    CARLA_SAFE_ASSERT_RETURN_ERR(btype != BINARY_NONE, "Invalid plugin params (err #1)");
    CARLA_SAFE_ASSERT_RETURN_ERR(ptype != PLUGIN_NONE, "Invalid plugin params (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERR((filename != nullptr && filename[0] != '\0') || (label != nullptr && label[0] != '\0'), "Invalid plugin params (err #3)");
    carla_debug("CarlaEngine::addPlugin(%i:%s, %i:%s, \"%s\", \"%s\", \"%s\", %p)", btype, BinaryType2Str(btype), ptype, PluginType2Str(ptype), filename, name, label, extra);

    unsigned int id;
    CarlaPlugin* oldPlugin = nullptr;

    if (pData->nextPluginId < pData->curPluginCount)
    {
        id = pData->nextPluginId;
        pData->nextPluginId = pData->maxPluginNumber;

        oldPlugin = pData->plugins[id].plugin;

        CARLA_SAFE_ASSERT_RETURN_ERR(oldPlugin != nullptr, "Invalid replace plugin Id");
    }
    else
    {
        id = pData->curPluginCount;

        if (id == pData->maxPluginNumber)
        {
            setLastError("Maximum number of plugins reached");
            return false;
        }

        CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins[id].plugin == nullptr, "Invalid engine internal data (err #13)");
    }

    CarlaPlugin::Initializer init = {
        this,
        id,
        filename,
        name,
        label
    };

    CarlaPlugin* plugin = nullptr;

#if 0 //ndef BUILD_BRIDGE
    const char* bridgeBinary;

    switch (btype)
    {
    case BINARY_POSIX32:
        bridgeBinary = pData->options.bridge_posix32.isNotEmpty() ? (const char*)pData->options.bridge_posix32 : nullptr;
        break;
    case BINARY_POSIX64:
        bridgeBinary = pData->options.bridge_posix64.isNotEmpty() ? (const char*)pData->options.bridge_posix64 : nullptr;
        break;
    case BINARY_WIN32:
        bridgeBinary = pData->options.bridge_win32.isNotEmpty() ? (const char*)pData->options.bridge_win32 : nullptr;
        break;
    case BINARY_WIN64:
        bridgeBinary = pData->options.bridge_win64.isNotEmpty() ? (const char*)pData->options.bridge_win64 : nullptr;
        break;
    default:
        bridgeBinary = nullptr;
        break;
    }

# ifndef CARLA_OS_WIN
    if (btype == BINARY_NATIVE && pData->options.bridge_native.isNotEmpty())
       bridgeBinary = (const char*)pData->options.bridge_native;
# endif

    if (btype != BINARY_NATIVE || (pData->options.preferPluginBridges && bridgeBinary != nullptr))
    {
        if (bridgeBinary != nullptr)
        {
            plugin = CarlaPlugin::newBridge(init, btype, ptype, bridgeBinary);
        }
# ifdef CARLA_OS_LINUX
        else if (btype == BINARY_WIN32)
        {
            // fallback to dssi-vst
            QFileInfo fileInfo(filename);

            CarlaString label2(fileInfo.fileName().toUtf8().constData());
            label2.replace(' ', '*');

            CarlaPlugin::Initializer init2 = {
                this,
                id,
                "/usr/lib/dssi/dssi-vst.so",
                name,
                (const char*)label2
            };

            char* const oldVstPath(getenv("VST_PATH"));
            carla_setenv("VST_PATH", fileInfo.absoluteDir().absolutePath().toUtf8().constData());

            plugin = CarlaPlugin::newDSSI(init2);

            if (oldVstPath != nullptr)
                carla_setenv("VST_PATH", oldVstPath);
        }
# endif
        else
        {
            setLastError("This Carla build cannot handle this binary");
            return false;
        }
    }
    else
#endif // BUILD_BRIDGE
    {
        setLastError("Invalid or unsupported plugin type");

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

    EnginePluginData& pluginData(pData->plugins[id]);
    pluginData.plugin      = plugin;
    pluginData.insPeak[0]  = 0.0f;
    pluginData.insPeak[1]  = 0.0f;
    pluginData.outsPeak[0] = 0.0f;
    pluginData.outsPeak[1] = 0.0f;

    if (oldPlugin != nullptr)
    {
        delete oldPlugin;
        callback(ENGINE_CALLBACK_RELOAD_ALL, id, 0, 0, 0.0f, plugin->getName());
    }
    else
    {
        ++pData->curPluginCount;
        callback(ENGINE_CALLBACK_PLUGIN_ADDED, id, 0, 0, 0.0f, plugin->getName());
    }

    return true;
}

bool CarlaEngine::removePlugin(const unsigned int id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #14)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data (err #15)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #16)");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id (err #1)");
    carla_debug("CarlaEngine::removePlugin(%i)", id);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to remove");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data (err #17)");

    pData->thread.stop(500);

    const bool lockWait(isRunning() && pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);
    const CarlaEngineProtectedData::ScopedActionLock sal(pData, kEnginePostActionRemovePlugin, id, 0, lockWait);

#ifndef BUILD_BRIDGE
    if (isOscControlRegistered())
        oscSend_control_remove_plugin(id);
#endif

    delete plugin;

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.start();

    callback(ENGINE_CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0.0f, nullptr);
    return true;
}

bool CarlaEngine::removeAllPlugins()
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #18)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId == pData->maxPluginNumber, "Invalid engine internal data (err #19)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #20)");
    carla_debug("CarlaEngine::removeAllPlugins()");

    if (pData->curPluginCount == 0)
        return true;

    pData->thread.stop(500);

    const bool lockWait(isRunning());
    const CarlaEngineProtectedData::ScopedActionLock sal(pData, kEnginePostActionZeroCount, 0, 0, lockWait);

    for (unsigned int i=0; i < pData->maxPluginNumber; ++i)
    {
        EnginePluginData& pluginData(pData->plugins[i]);

        if (pluginData.plugin != nullptr)
        {
            delete pluginData.plugin;
            pluginData.plugin = nullptr;
        }

        pluginData.insPeak[0]  = 0.0f;
        pluginData.insPeak[1]  = 0.0f;
        pluginData.outsPeak[0] = 0.0f;
        pluginData.outsPeak[1] = 0.0f;
    }

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.start();

    return true;
}

const char* CarlaEngine::renamePlugin(const unsigned int id, const char* const newName)
{
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->plugins != nullptr, "Invalid engine internal data (err #21)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->curPluginCount != 0, "Invalid engine internal data (err #22)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #23)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(id < pData->curPluginCount, "Invalid plugin Id (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(newName != nullptr && newName[0] != '\0', "Invalid plugin name");
    carla_debug("CarlaEngine::renamePlugin(%i, \"%s\")", id, newName);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERRN(plugin != nullptr, "Could not find plugin to rename");
    CARLA_SAFE_ASSERT_RETURN_ERRN(plugin->getId() == id, "Invalid engine internal data (err #24)");

    if (const char* const name = getUniquePluginName(newName))
    {
        plugin->setName(name);
        return name;
    }

    setLastError("Unable to get new unique plugin name");
    return nullptr;
}

bool CarlaEngine::clonePlugin(const unsigned int id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #25)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data (err #26)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #27)");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id (err #3)");
    carla_debug("CarlaEngine::clonePlugin(%i)", id);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to clone");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data (err #28)");

    char label[STR_MAX+1];
    carla_zeroChar(label, STR_MAX+1);
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
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #29)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data (err #30)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #31)");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id (err #4)");
    carla_debug("CarlaEngine::replacePlugin(%i)", id);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to replace");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data (err #32)");

    pData->nextPluginId = id;

    return true;
}

bool CarlaEngine::switchPlugins(const unsigned int idA, const unsigned int idB)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data (err #33)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount >= 2, "Invalid engine internal data (err #34)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #35)");
    CARLA_SAFE_ASSERT_RETURN_ERR(idA != idB, "Invalid operation, cannot switch plugin with itself");
    CARLA_SAFE_ASSERT_RETURN_ERR(idA < pData->curPluginCount, "Invalid plugin Id (err #5)");
    CARLA_SAFE_ASSERT_RETURN_ERR(idB < pData->curPluginCount, "Invalid plugin Id (err #6)");
    carla_debug("CarlaEngine::switchPlugins(%i)", idA, idB);

    CarlaPlugin* const pluginA(pData->plugins[idA].plugin);
    CarlaPlugin* const pluginB(pData->plugins[idB].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA != nullptr, "Could not find plugin to switch (err #1)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA != nullptr, "Could not find plugin to switch (err #2)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA->getId() == idA, "Invalid engine internal data (err #36)");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginB->getId() == idB, "Invalid engine internal data (err #37)");

    pData->thread.stop(500);

    const bool lockWait(isRunning() && pData->options.processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);
    const CarlaEngineProtectedData::ScopedActionLock sal(pData, kEnginePostActionSwitchPlugins, idA, idB, lockWait);

#ifndef BUILD_BRIDGE // TODO
    //if (isOscControlRegistered())
    //    oscSend_control_switch_plugins(idA, idB);
#endif

    if (isRunning() && ! pData->aboutToClose)
        pData->thread.start();

    return true;
}

CarlaPlugin* CarlaEngine::getPlugin(const unsigned int id)
{
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->plugins != nullptr, "Invalid engine internal data (err #38)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->curPluginCount != 0, "Invalid engine internal data (err #39)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data (err #40)");
    CARLA_SAFE_ASSERT_RETURN_ERRN(id < pData->curPluginCount, "Invalid plugin Id (err #7)");
    carla_debug("CarlaEngine::getPlugin(%i) [count:%i]", id, pData->curPluginCount);

    return pData->plugins[id].plugin;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const unsigned int id) const noexcept
{
    return pData->plugins[id].plugin;
}

const char* CarlaEngine::getUniquePluginName(const char* const name) const
{
    CARLA_SAFE_ASSERT_RETURN(pData->plugins != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(pData->maxPluginNumber != 0, nullptr);
    CARLA_SAFE_ASSERT_RETURN(pData->nextAction.opcode == kEnginePostActionNull, nullptr);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngine::getUniquePluginName(\"%s\")", name);

    static CarlaMutex m;
    const CarlaMutex::ScopedLocker sl(m);

    static CarlaString sname;
    sname = name;

    if (sname.isEmpty())
    {
        sname = "(No name)";
        return (const char*)sname;
    }

    sname.truncate(getMaxClientNameSize()-5-1); // 5 = strlen(" (10)")
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (unsigned short i=0; i < pData->curPluginCount; ++i)
    {
        CARLA_SAFE_ASSERT_BREAK(pData->plugins[i].plugin != nullptr);

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
                    n2 = '0';
                    n3 = static_cast<char>(n3 + 1);
                }
                else
                    n2 = static_cast<char>(n2 + 1);

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

bool CarlaEngine::loadFile(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename (err #1)");
    carla_debug("CarlaEngine::loadFile(\"%s\")", filename);

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
    extension.toLower();

    // -------------------------------------------------------------------

    if (extension == "carxp" || extension ==  "carxs")
        return loadProject(filename);

    // -------------------------------------------------------------------

    if (extension == "csd")
        return addPlugin(PLUGIN_CSOUND, filename, baseName, baseName);

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
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
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
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
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
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
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
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, (extension == "xmz") ? "CarlaAlternateFile1" : "CarlaAlternateFile2", filename, true);
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

bool CarlaEngine::loadProject(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename (err #2)");
    carla_debug("CarlaEngine::loadProject(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QDomDocument xml;
    xml.setContent(file.readAll());
    file.close();

    QDomNode xmlNode(xml.documentElement());

    const bool isPreset(xmlNode.toElement().tagName().compare("carla-preset", Qt::CaseInsensitive) == 0);

    if (xmlNode.toElement().tagName().compare("carla-project", Qt::CaseInsensitive) != 0 && ! isPreset)
    {
        setLastError("Not a valid Carla project or preset file");
        return false;
    }

    for (QDomNode node = xmlNode.firstChild(); ! node.isNull(); node = node.nextSibling())
    {
        if (isPreset || node.toElement().tagName().compare("plugin", Qt::CaseInsensitive) == 0)
        {
            SaveState saveState;
            fillSaveStateFromXmlNode(saveState, isPreset ? xmlNode : node);

            CARLA_SAFE_ASSERT_CONTINUE(saveState.type != nullptr);

            const void* extraStuff = nullptr;

            // check if using GIG, SF2 or SFZ 16outs
            static const char kUse16OutsSuffix[] = " (16 outs)";

            if (CarlaString(saveState.label).endsWith(kUse16OutsSuffix))
            {
                if (std::strcmp(saveState.type, "GIG") == 0 || std::strcmp(saveState.type, "SF2") == 0 || std::strcmp(saveState.type, "SFZ") == 0)
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

    return true;
}

bool CarlaEngine::saveProject(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename (err #3)");
    carla_debug("CarlaEngine::saveProject(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
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
                out << QString(" <!-- %1 -->\n").arg(xmlSafeString(strBuf, true));

            QString content;
            fillXmlStringFromSaveState(content, plugin->getSaveState());

            out << " <Plugin>\n";
            out << content;
            out << " </Plugin>\n";

            firstPlugin = false;
        }
    }

    out << "</CARLA-PROJECT>\n";

    file.close();
    return true;
}

// -----------------------------------------------------------------------
// Information (base)

/*!
  * Get the current engine driver hints.
  */
unsigned int CarlaEngine::getHints() const noexcept
{
    return pData->hints;
}

/*!
  * Get the current buffer size.
  */
uint32_t CarlaEngine::getBufferSize() const noexcept
{
    return pData->bufferSize;
}

/*!
  * Get the current sample rate.
  */
double CarlaEngine::getSampleRate() const noexcept
{
    return pData->sampleRate;
}

/*!
  * Get the current engine name.
  */
const char* CarlaEngine::getName() const noexcept
{
    return (const char*)pData->name;
}

/*!
  * Get the current engine proccess mode.
  */
EngineProcessMode CarlaEngine::getProccessMode() const noexcept
{
    return pData->options.processMode;
}

/*!
  * Get the current engine options (read-only).
  */
const EngineOptions& CarlaEngine::getOptions() const noexcept
{
    return pData->options;
}

/*!
  * Get the current Time information (read-only).
  */
const EngineTimeInfo& CarlaEngine::getTimeInfo() const noexcept
{
    return pData->timeInfo;
}

// -----------------------------------------------------------------------
// Information (peaks)

// FIXME

float CarlaEngine::getInputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);
    CARLA_SAFE_ASSERT_RETURN(id == 1 || id == 2, 0.0f);

    return pData->plugins[pluginId].insPeak[id-1];
}

float CarlaEngine::getOutputPeak(const unsigned int pluginId, const unsigned short id) const
{
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);
    CARLA_SAFE_ASSERT_RETURN(id == 1 || id == 2, 0.0f);

    return pData->plugins[pluginId].outsPeak[id-1];
}

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const EngineCallbackOpcode action, const unsigned int pluginId, const int value1, const int value2, const float value3, const char* const valueStr)
{
    carla_debug("CarlaEngine::callback(%s, %i, %i, %i, %f, \"%s\")", EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);

    if (pData->callback != nullptr)
        pData->callback(pData->callbackPtr, action, pluginId, value1, value2, value3, valueStr);
}

void CarlaEngine::setCallback(const EngineCallbackFunc func, void* const ptr)
{
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

bool CarlaEngine::patchbayRefresh()
{
    setLastError("Unsupported operation");
    return false;
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

void CarlaEngine::transportRelocate(const uint64_t frame)
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

void CarlaEngine::setOption(const EngineOption option, const int value, const char* const valueStr)
{
    carla_debug("CarlaEngine::setOption(%i:%s, %i, \"%s\")", option, EngineOption2Str(option), value, valueStr);

    if (isRunning() && (option == ENGINE_OPTION_PROCESS_MODE || option == ENGINE_OPTION_AUDIO_NUM_PERIODS || option == ENGINE_OPTION_AUDIO_DEVICE))
        return carla_stderr("CarlaEngine::setOption(%s, %i, \"%s\") - Cannot set this option while engine is running!", EngineOption2Str(option), value, valueStr);

    switch (option)
    {
    case ENGINE_OPTION_DEBUG:
        break;

    case ENGINE_OPTION_PROCESS_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= ENGINE_PROCESS_MODE_SINGLE_CLIENT && value < ENGINE_PROCESS_MODE_BRIDGE,);
        pData->options.processMode = static_cast<EngineProcessMode>(value);
        break;

    case ENGINE_OPTION_TRANSPORT_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= ENGINE_TRANSPORT_MODE_INTERNAL && value < ENGINE_TRANSPORT_MODE_BRIDGE,);
        pData->options.transportMode = static_cast<EngineTransportMode>(value);
        break;

    case ENGINE_OPTION_FORCE_STEREO:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.forceStereo = (value != 0);
        break;

    case ENGINE_OPTION_PREFER_PLUGIN_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.preferPluginBridges = (value != 0);
        break;

    case ENGINE_OPTION_PREFER_UI_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.preferUiBridges = (value != 0);
        break;

    case ENGINE_OPTION_UIS_ALWAYS_ON_TOP:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.uisAlwaysOnTop = (value != 0);
        break;

    case ENGINE_OPTION_MAX_PARAMETERS:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        pData->options.maxParameters = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_UI_BRIDGES_TIMEOUT:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        pData->options.uiBridgesTimeout = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_NUM_PERIODS:
        CARLA_SAFE_ASSERT_RETURN(value >= 2 && value <= 3,);
        pData->options.audioNumPeriods = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_BUFFER_SIZE:
        CARLA_SAFE_ASSERT_RETURN(value >= 8,);
        pData->options.audioBufferSize = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_SAMPLE_RATE:
        CARLA_SAFE_ASSERT_RETURN(value >= 22050,);
        pData->options.audioSampleRate = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_DEVICE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.audioDevice != nullptr)
            delete[] pData->options.audioDevice;

        pData->options.audioDevice = carla_strdup(valueStr);
        break;

    case ENGINE_OPTION_PATH_BINARIES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.binaryDir != nullptr)
            delete[] pData->options.binaryDir;

        pData->options.binaryDir = carla_strdup(valueStr);
        break;

    case ENGINE_OPTION_PATH_RESOURCES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.resourceDir != nullptr)
            delete[] pData->options.resourceDir;

        pData->options.resourceDir = carla_strdup(valueStr);
        break;
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
// Helper functions

EngineEvent* CarlaEngine::getInternalEventBuffer(const bool isInput) const noexcept
{
    return isInput ? pData->bufEvents.in : pData->bufEvents.out;
}

void CarlaEngine::registerEnginePlugin(const unsigned int id, CarlaPlugin* const plugin)
{
    CARLA_SAFE_ASSERT_RETURN(id == pData->curPluginCount,);
    carla_debug("CarlaEngine::registerEnginePlugin(%i, %p)", id, plugin);

    pData->plugins[id].plugin = plugin;
}

// -----------------------------------------------------------------------
// Internal stuff

void CarlaEngine::bufferSizeChanged(const uint32_t newBufferSize)
{
    carla_debug("CarlaEngine::bufferSizeChanged(%i)", newBufferSize);

    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->bufferSizeChanged(newBufferSize);
    }

    callback(ENGINE_CALLBACK_BUFFER_SIZE_CHANGED, 0, newBufferSize, 0, 0.0f, nullptr);
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

    callback(ENGINE_CALLBACK_SAMPLE_RATE_CHANGED, 0, 0, 0, static_cast<float>(newSampleRate), nullptr);
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
        pData->time.frame += pData->bufferSize;

    if (pData->options.transportMode == ENGINE_TRANSPORT_MODE_INTERNAL)
    {
        pData->timeInfo.playing = pData->time.playing;
        pData->timeInfo.frame   = pData->time.frame;
    }
}

void CarlaEngine::setPluginPeaks(const unsigned int pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept
{
    EnginePluginData& pluginData(pData->plugins[pluginId]);

    pluginData.insPeak[0]  = inPeaks[0];
    pluginData.insPeak[1]  = inPeaks[1];
    pluginData.outsPeak[0] = outPeaks[0];
    pluginData.outsPeak[1] = outPeaks[1];
}

#ifndef BUILD_BRIDGE
void CarlaEngine::processRack(float* inBufReal[2], float* outBuf[2], const uint32_t frames)
{
    CARLA_SAFE_ASSERT_RETURN(pData->bufEvents.in != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pData->bufEvents.out != nullptr,);

    // safe copy
    float inBuf0[frames];
    float inBuf1[frames];
    float* inBuf[2] = { inBuf0, inBuf1 };

    // initialize audio inputs
    FLOAT_COPY(inBuf0, inBufReal[0], frames);
    FLOAT_COPY(inBuf1, inBufReal[1], frames);

    // initialize audio outputs (zero)
    FLOAT_CLEAR(outBuf[0], frames);
    FLOAT_CLEAR(outBuf[1], frames);

    // initialize event outputs (zero)
    carla_zeroStruct<EngineEvent>(pData->bufEvents.out, kEngineMaxInternalEventCount);

    bool processed = false;

    uint32_t oldAudioInCount = 0;
    uint32_t oldMidiOutCount = 0;

    const bool forcedOffline(isOffline());

    // process plugins
    for (unsigned int i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin = pData->plugins[i].plugin;

        if (plugin == nullptr || ! plugin->isEnabled() || ! plugin->tryLock(forcedOffline))
            continue;

        if (processed)
        {
            // initialize audio inputs (from previous outputs)
            FLOAT_COPY(inBuf0, outBuf[0], frames);
            FLOAT_COPY(inBuf1, outBuf[1], frames);

            // initialize audio outputs (zero)
            FLOAT_CLEAR(outBuf[0], frames);
            FLOAT_CLEAR(outBuf[1], frames);

            // if plugin has no midi out, add previous events
            if (oldMidiOutCount == 0 && pData->bufEvents.in[0].type != kEngineEventTypeNull)
            {
                if (pData->bufEvents.out[0].type != kEngineEventTypeNull)
                {
                    // TODO: carefully add to input, sorted events
                }
                // else nothing needed
            }
            else
            {
                // initialize event inputs from previous outputs
                std::memcpy(pData->bufEvents.in, pData->bufEvents.out, sizeof(EngineEvent)*kEngineMaxInternalEventCount);

                // initialize event outputs (zero)
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
            FLOAT_ADD(outBuf[0], inBuf0, frames);
            FLOAT_ADD(outBuf[1], inBuf1, frames);
        }

        // set peaks
        {
            EnginePluginData& pluginData(pData->plugins[i]);

#ifdef HAVE_JUCE
            float tmpMin, tmpMax;

            if (oldAudioInCount > 0)
            {
                FloatVectorOperations::findMinAndMax(inBuf0, frames, tmpMin, tmpMax);
                pluginData.insPeak[0] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);

                FloatVectorOperations::findMinAndMax(inBuf1, frames, tmpMin, tmpMax);
                pluginData.insPeak[1] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);
            }
            else
            {
                pluginData.insPeak[0] = 0.0f;
                pluginData.insPeak[1] = 0.0f;
            }

            if (oldMidiOutCount > 0)
            {
                FloatVectorOperations::findMinAndMax(outBuf[0], frames, tmpMin, tmpMax);
                pluginData.outsPeak[0] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);

                FloatVectorOperations::findMinAndMax(outBuf[1], frames, tmpMin, tmpMax);
                pluginData.outsPeak[1] = carla_max<float>(std::abs(tmpMin), std::abs(tmpMax), 1.0f);
            }
            else
            {
                pluginData.outsPeak[0] = 0.0f;
                pluginData.outsPeak[1] = 0.0f;
            }
#else
            float peak1, peak2;

            if (oldAudioInCount > 0)
            {
                peak1 = peak2 = 0.0f;

                for (uint32_t k=0; k < frames; ++k)
                {
                    peak1  = carla_max<float>(peak1,  std::fabs(inBuf0[k]), 1.0f);
                    peak2  = carla_max<float>(peak2,  std::fabs(inBuf1[k]), 1.0f);
                }

                pluginData.insPeak[0] = peak1;
                pluginData.insPeak[1] = peak2;
            }
            else
            {
                pluginData.insPeak[0] = 0.0f;
                pluginData.insPeak[1] = 0.0f;
            }

            if (oldMidiOutCount > 0)
            {
                peak1 = peak2 = 0.0f;

                for (uint32_t k=0; k < frames; ++k)
                {
                    peak1 = carla_max<float>(peak1, std::fabs(outBuf[0][k]), 1.0f);
                    peak2 = carla_max<float>(peak2, std::fabs(outBuf[1][k]), 1.0f);
                }

                pluginData.outsPeak[0] = peak1;
                pluginData.outsPeak[1] = peak2;
            }
            else
            {
                pluginData.outsPeak[0] = 0.0f;
                pluginData.outsPeak[1] = 0.0f;
            }
#endif
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
// Bridge/Controller OSC stuff

#ifdef BUILD_BRIDGE
void CarlaEngine::oscSend_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(total >= 0 && total >= ins + outs,);
    carla_debug("CarlaEngine::oscSend_bridge_audio_count(%i, %i, %i)", ins, outs, total);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_audio_count");
        lo_send(pData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::oscSend_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(total >= 0 && total >= ins + outs,);
    carla_debug("CarlaEngine::oscSend_bridge_midi_count(%i, %i, %i)", ins, outs, total);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+19];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_count");
        lo_send(pData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::oscSend_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(total >= 0 && total >= ins + outs,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_count(%i, %i, %i)", ins, outs, total);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+24];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_count");
        lo_send(pData->oscData->target, targetPath, "iii", ins, outs, total);
    }
}

void CarlaEngine::oscSend_bridge_program_count(const int32_t count)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count >= 0,);
    carla_debug("CarlaEngine::oscSend_bridge_program_count(%i)", count);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+22];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_program_count");
        lo_send(pData->oscData->target, targetPath, "i", count);
    }
}

void CarlaEngine::oscSend_bridge_midi_program_count(const int32_t count)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count >= 0,);
    carla_debug("CarlaEngine::oscSend_bridge_midi_program_count(%i)", count);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+27];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_program_count");
        lo_send(pData->oscData->target, targetPath, "i", count);
    }
}

void CarlaEngine::oscSend_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(maker != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(copyright != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_plugin_info(%i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", category, hints, name, label, maker, copyright, uniqueId);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_plugin_info");
        lo_send(pData->oscData->target, targetPath, "iissssh", category, hints, name, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::oscSend_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(unit != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_info(%i, \"%s\", \"%s\")", index, name, unit);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_info");
        lo_send(pData->oscData->target, targetPath, "iss", index, name, unit);
    }
}

void CarlaEngine::oscSend_bridge_parameter_data(const int32_t index, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_data(%i, %i, %i, %i, %i)", index, rindex, hints, midiChannel, midiCC);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_data");
        lo_send(pData->oscData->target, targetPath, "iiiii", index, rindex, hints, midiChannel, midiCC);
    }
}

void CarlaEngine::oscSend_bridge_parameter_ranges(const int32_t index, const float def, const float min, const float max, const float step, const float stepSmall, const float stepLarge)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_parameter_ranges(%i, %f, %f, %f, %f, %f, %f)", index, def, min, max, step, stepSmall, stepLarge);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+25];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_parameter_ranges");
        lo_send(pData->oscData->target, targetPath, "iffffff", index, def, min, max, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::oscSend_bridge_program_info(const int32_t index, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_program_info(%i, \"%s\")", index, name);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+21];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_program_info");
        lo_send(pData->oscData->target, targetPath, "is", index, name);
    }
}

void CarlaEngine::oscSend_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_midi_program_info(%i, %i, %i, \"%s\")", index, bank, program, label);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+26];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_midi_program_info");
        lo_send(pData->oscData->target, targetPath, "iiis", index, bank, program, label);
    }
}

void CarlaEngine::oscSend_bridge_configure(const char* const key, const char* const value)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_configure(\"%s\", \"%s\")", key, value);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_configure");
        lo_send(pData->oscData->target, targetPath, "ss", key, value);
    }
}

void CarlaEngine::oscSend_bridge_set_parameter_value(const int32_t index, const float value)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_set_parameter_value(%i, %f)", index, value);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+28];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_parameter_value");
        lo_send(pData->oscData->target, targetPath, "if", index, value);
    }
}

void CarlaEngine::oscSend_bridge_set_default_value(const int32_t index, const float value)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_set_default_value(%i, %f)", index, value);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+26];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_default_value");
        lo_send(pData->oscData->target, targetPath, "if", index, value);
    }
}

void CarlaEngine::oscSend_bridge_set_program(const int32_t index)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_set_program(%i)", index);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_program");
        lo_send(pData->oscData->target, targetPath, "i", index);
    }
}

void CarlaEngine::oscSend_bridge_set_midi_program(const int32_t index)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_set_midi_program(%i)", index);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+25];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_midi_program");
        lo_send(pData->oscData->target, targetPath, "i", index);
    }
}

void CarlaEngine::oscSend_bridge_set_custom_data(const char* const type, const char* const key, const char* const value)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_set_custom_data(\"%s\", \"%s\", \"%s\")", type, key, value);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+24];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_custom_data");
        lo_send(pData->oscData->target, targetPath, "sss", type, key, value);
    }
}

void CarlaEngine::oscSend_bridge_set_chunk_data(const char* const chunkFile)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_bridge_set_chunk_data(\"%s\")", chunkFile);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/bridge_set_chunk_data");
        lo_send(pData->oscData->target, targetPath, "s", chunkFile);
    }
}
#else
void CarlaEngine::oscSend_control_add_plugin_start(const uint pluginId, const char* const pluginName)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(pluginName != nullptr && pluginName[0] != '\0',);
    carla_debug("CarlaEngine::oscSend_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/add_plugin_start");
        lo_send(pData->oscData->target, targetPath, "is", static_cast<int32_t>(pluginId), pluginName);
    }
}

void CarlaEngine::oscSend_control_add_plugin_end(const uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_add_plugin_end(%i)", pluginId);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+16];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/add_plugin_end");
        lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(pluginId));
    }
}

void CarlaEngine::oscSend_control_remove_plugin(const uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_remove_plugin(%i)", pluginId);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+15];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/remove_plugin");
        lo_send(pData->oscData->target, targetPath, "i", static_cast<int32_t>(pluginId));
    }
}

void CarlaEngine::oscSend_control_set_plugin_info1(const uint pluginId, const PluginType type, const PluginCategory category, const uint hints, const long uniqueId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(type != PLUGIN_NONE,);
    carla_debug("CarlaEngine::oscSend_control_set_plugin_data(%i, %i:%s, %i:%s, %X, %l)", pluginId, type, PluginType2Str(type), category, PluginCategory2Str(category), hints, uniqueId);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_plugin_info1");
        lo_send(pData->oscData->target, targetPath, "iiiih", static_cast<int32_t>(pluginId), static_cast<int32_t>(type), static_cast<int32_t>(category), static_cast<int32_t>(hints), static_cast<int64_t>(uniqueId));
    }
}

void CarlaEngine::oscSend_control_set_plugin_info2(const uint pluginId, const char* const realName, const char* const label, const char* const maker, const char* const copyright)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(realName != nullptr && realName[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(maker != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(copyright != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_plugin_data(%i, \"%s\", \"%s\", \"%s\", \"%s\")", pluginId, realName, label, maker, copyright);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_plugin_info2");
        lo_send(pData->oscData->target, targetPath, "issss", static_cast<int32_t>(pluginId), realName, label, maker, copyright);
    }
}

void CarlaEngine::oscSend_control_set_audio_count(const uint pluginId, const uint32_t ins, const uint32_t outs)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_audio_count(%i, %i, %i)", pluginId, ins, outs);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_audio_count");
        lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
    }
}

void CarlaEngine::oscSend_control_set_midi_count(const uint pluginId, const uint32_t ins, const uint32_t outs)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_count(%i, %i, %i)", pluginId, ins, outs);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_midi_count");
        lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
    }
}

void CarlaEngine::oscSend_control_set_parameter_count(const uint pluginId, const uint32_t ins, const uint32_t outs)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_count(%i, %i, %i)", pluginId, ins, outs);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_count");
        lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(ins), static_cast<int32_t>(outs));
    }
}

void CarlaEngine::oscSend_control_set_program_count(const uint pluginId, const uint32_t count)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_program_count(%i, %i)", pluginId, count);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+19];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_program_count");
        lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
    }
}

void CarlaEngine::oscSend_control_set_midi_program_count(const uint pluginId, const uint32_t count)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_count(%i, %i)", pluginId, count);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+24];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_midi_program_count");
        lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), static_cast<int32_t>(count));
    }
}

void CarlaEngine::oscSend_control_set_parameter_data(const uint pluginId, const uint32_t index, const ParameterType type, const uint hints, const char* const name, const char* const unit)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(unit != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_data(%i, %i, %i:%s, %X, \"%s\", \"%s\")", pluginId, index, type, ParameterType2Str(type), hints, name, unit);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+20];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_data");
        lo_send(pData->oscData->target, targetPath, "iiiiss", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(type), static_cast<int32_t>(hints), name, unit);
    }
}

void CarlaEngine::oscSend_control_set_parameter_ranges1(const uint pluginId, const uint32_t index, const float def, const float min, const float max)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(def <= min && def >= max,);
    CARLA_SAFE_ASSERT_RETURN(min < max,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_ranges1(%i, %i, %f, %f, %f)", pluginId, index, def, min, max, def);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_ranges1");
        lo_send(pData->oscData->target, targetPath, "iifff", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), def, min, max);
    }
}

void CarlaEngine::oscSend_control_set_parameter_ranges2(const uint pluginId, const uint32_t index, const float step, const float stepSmall, const float stepLarge)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(step <= stepSmall && step >= stepLarge,);
    CARLA_SAFE_ASSERT_RETURN(stepSmall <= stepLarge,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_ranges2(%i, %i, %f, %f, %f)", pluginId, index, step, stepSmall, stepLarge);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_ranges");
        lo_send(pData->oscData->target, targetPath, "iifff", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), step, stepSmall, stepLarge);
    }
}

void CarlaEngine::oscSend_control_set_parameter_midi_cc(const uint pluginId, const uint32_t index, const int16_t cc)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(cc <= 0x5F,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_midi_cc");
        lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(cc));
    }
}

void CarlaEngine::oscSend_control_set_parameter_midi_channel(const uint pluginId, const uint32_t index, const uint8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+28];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_midi_channel");
        lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(channel));
    }
}

void CarlaEngine::oscSend_control_set_parameter_value(const uint pluginId, const int32_t index, const float value)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_parameter_value(%i, %i:%s, %f)", pluginId, index, (index < 0) ? InternalParameterIndex2Str(static_cast<InternalParameterIndex>(index)) : "(none)", value);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+21];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_parameter_value");
        lo_send(pData->oscData->target, targetPath, "iif", static_cast<int32_t>(pluginId), index, value);
    }
}

void CarlaEngine::oscSend_control_set_default_value(const uint pluginId, const uint32_t index, const float value)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_default_value(%i, %i, %f)", pluginId, index, value);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+19];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_default_value");
        lo_send(pData->oscData->target, targetPath, "iif", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), value);
    }
}

void CarlaEngine::oscSend_control_set_current_program(const uint pluginId, const int32_t index)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_current_program(%i, %i)", pluginId, index);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+21];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_current_program");
        lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), index);
    }
}

void CarlaEngine::oscSend_control_set_current_midi_program(const uint pluginId, const int32_t index)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    carla_debug("CarlaEngine::oscSend_control_set_current_midi_program(%i, %i)", pluginId, index);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+26];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_current_midi_program");
        lo_send(pData->oscData->target, targetPath, "ii", static_cast<int32_t>(pluginId), index);
    }
}

void CarlaEngine::oscSend_control_set_program_name(const uint pluginId, const uint32_t index, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+18];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_program_name");
        lo_send(pData->oscData->target, targetPath, "iis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), name);
    }
}

void CarlaEngine::oscSend_control_set_midi_program_data(const uint pluginId, const uint32_t index, const uint32_t bank, const uint32_t program, const char* const name)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+23];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_midi_program_data");
        lo_send(pData->oscData->target, targetPath, "iiiis", static_cast<int32_t>(pluginId), static_cast<int32_t>(index), static_cast<int32_t>(bank), static_cast<int32_t>(program), name);
    }
}

void CarlaEngine::oscSend_control_note_on(const uint pluginId, const uint8_t channel, const uint8_t note, const uint8_t velo)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    CARLA_SAFE_ASSERT_RETURN(velo < MAX_MIDI_VALUE,);
    carla_debug("CarlaEngine::oscSend_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+9];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/note_on");
        lo_send(pData->oscData->target, targetPath, "iiii", static_cast<int32_t>(pluginId), static_cast<int32_t>(channel), static_cast<int32_t>(note), static_cast<int32_t>(velo));
    }
}

void CarlaEngine::oscSend_control_note_off(const uint pluginId, const uint8_t channel, const uint8_t note)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    carla_debug("CarlaEngine::oscSend_control_note_off(%i, %i, %i)", pluginId, channel, note);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+10];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/note_off");
        lo_send(pData->oscData->target, targetPath, "iii", static_cast<int32_t>(pluginId), static_cast<int32_t>(channel), static_cast<int32_t>(note));
    }
}

void CarlaEngine::oscSend_control_set_peaks(const uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount,);

    // TODO - try and see if we can get peaks[4] ref
    const EnginePluginData& epData(pData->plugins[pluginId]);

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+11];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/set_peaks");
        lo_send(pData->oscData->target, targetPath, "iffff", static_cast<int32_t>(pluginId), epData.insPeak[0], epData.insPeak[1], epData.outsPeak[0], epData.outsPeak[1]);
    }
}

void CarlaEngine::oscSend_control_exit()
{
    CARLA_SAFE_ASSERT_RETURN(pData->oscData != nullptr,);
    carla_debug("CarlaEngine::oscSend_control_exit()");

    if (pData->oscData->target != nullptr)
    {
        char targetPath[std::strlen(pData->oscData->path)+6];
        std::strcpy(targetPath, pData->oscData->path);
        std::strcat(targetPath, "/exit");
        lo_send(pData->oscData->target, targetPath, "");
    }
}
#endif

// -----------------------------------------------------------------------

#undef CARLA_SAFE_ASSERT_RETURN_ERR

CARLA_BACKEND_END_NAMESPACE
