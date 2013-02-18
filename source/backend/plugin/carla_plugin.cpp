/*
 * Carla Plugin
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#include "carla_plugin_internal.hpp"
#include "carla_lib_utils.hpp"
#include "carla_state_utils.hpp"
#include "carla_midi.h"

//#include <QtGui/QtEvents>

// TODO - save&load options

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------
// fallback data

static const ParameterData    kParameterDataNull;
static const ParameterRanges  kParameterRangesNull;
static const MidiProgramData  kMidiProgramDataNull;
static const CustomData       kCustomDataNull;

// -------------------------------------------------------------------
// Helpers

CarlaEngine* CarlaPluginGetEngine(CarlaPlugin* const plugin)
{
    return CarlaPluginProtectedData::getEngine(plugin);
}

CarlaEngineAudioPort* CarlaPluginGetAudioInPort(CarlaPlugin* const plugin, uint32_t index)
{
    return CarlaPluginProtectedData::getAudioInPort(plugin, index);
}

CarlaEngineAudioPort* CarlaPluginGetAudioOutPort(CarlaPlugin* const plugin, uint32_t index)
{
    return CarlaPluginProtectedData::getAudioOutPort(plugin, index);
}

#if 0
int CarlaPluginSetOscBridgeInfo(CarlaPlugin* const plugin, const PluginBridgeInfoType type,
                                const int argc, const lo_arg* const* const argv, const char* const types)
{
    return ((BridgePlugin*)plugin)->setOscPluginBridgeInfo(type, argc, argv, types);
}
#endif

// -------------------------------------------------------------------
// Constructor and destructor

CarlaPlugin::CarlaPlugin(CarlaEngine* const engine, const unsigned int id)
    : fId(id),
      fHints(0x0),
      fOptions(0x0),
      fEnabled(false),
      kData(new CarlaPluginProtectedData(engine))
{
    CARLA_ASSERT(kData != nullptr);
    CARLA_ASSERT(engine != nullptr);
    CARLA_ASSERT(id < engine->maxPluginNumber());
    qDebug("CarlaPlugin::CarlaPlugin(%p, %i)", engine, id);

    switch (engine->getProccessMode())
    {
    case PROCESS_MODE_SINGLE_CLIENT:
    case PROCESS_MODE_MULTIPLE_CLIENTS:
        kData->ctrlInChannel = 0;
        break;

    case PROCESS_MODE_CONTINUOUS_RACK:
        CARLA_ASSERT(id < MAX_RACK_PLUGINS && id < MAX_MIDI_CHANNELS);

        if (id < MAX_RACK_PLUGINS && id < MAX_MIDI_CHANNELS)
            kData->ctrlInChannel = id;

        break;

    case PROCESS_MODE_PATCHBAY:
    case PROCESS_MODE_BRIDGE:
        break;
    }
}

CarlaPlugin::~CarlaPlugin()
{
    qDebug("CarlaPlugin::~CarlaPlugin()");

    // Remove client and ports
    if (kData->client != nullptr)
    {
        if (kData->client->isActive())
            kData->client->deactivate();

        // can't call virtual functions in destructor
        CarlaPlugin::deleteBuffers();

        delete kData->client;
    }

    if (kData->latencyBuffers != nullptr)
    {
        for (uint32_t i=0; i < kData->audioIn.count; i++)
            delete[] kData->latencyBuffers[i];

        delete[] kData->latencyBuffers;
    }

    kData->prog.clear();
    kData->midiprog.clear();
    kData->custom.clear();

    libClose();

    delete kData;
}

// -------------------------------------------------------------------
// Information (base)

uint32_t CarlaPlugin::latency() const
{
    return kData->latency;
}

// -------------------------------------------------------------------
// Information (count)

uint32_t CarlaPlugin::audioInCount() const
{
    return kData->audioIn.count;
}

uint32_t CarlaPlugin::audioOutCount() const
{
    return kData->audioOut.count;
}

uint32_t CarlaPlugin::midiInCount() const
{
    return (kData->extraHints & PLUGIN_HINT_HAS_MIDI_IN) ? 1 : 0;
}

uint32_t CarlaPlugin::midiOutCount() const
{
    return (kData->extraHints & PLUGIN_HINT_HAS_MIDI_OUT) ? 1 : 0;
}

uint32_t CarlaPlugin::parameterCount() const
{
    return kData->param.count;
}

uint32_t CarlaPlugin::parameterScalePointCount(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < kData->param.count);
    return 0;

    // unused
    Q_UNUSED(parameterId);
}

uint32_t CarlaPlugin::programCount() const
{
    return kData->prog.count;
}

uint32_t CarlaPlugin::midiProgramCount() const
{
    return kData->midiprog.count;
}

uint32_t CarlaPlugin::customDataCount() const
{
    return kData->custom.count();
}

// -------------------------------------------------------------------
// Information (current data)

int32_t CarlaPlugin::currentProgram() const
{
    return kData->prog.current;
}

int32_t CarlaPlugin::currentMidiProgram() const
{
    return kData->midiprog.current;
}

const ParameterData& CarlaPlugin::parameterData(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < kData->param.count);

    return (parameterId < kData->param.count) ? kData->param.data[parameterId] : kParameterDataNull;
}

const ParameterRanges& CarlaPlugin::parameterRanges(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < kData->param.count);

    return (parameterId < kData->param.count) ? kData->param.ranges[parameterId] : kParameterRangesNull;
}

bool CarlaPlugin::parameterIsOutput(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < kData->param.count);

    return (parameterId < kData->param.count) ? (kData->param.data[parameterId].type == PARAMETER_OUTPUT) : false;
}

const MidiProgramData& CarlaPlugin::midiProgramData(const uint32_t index) const
{
    CARLA_ASSERT(index < kData->midiprog.count);

    return (index < kData->midiprog.count) ? kData->midiprog.data[index] : kMidiProgramDataNull;
}

const CustomData& CarlaPlugin::customData(const size_t index) const
{
    CARLA_ASSERT(index < kData->custom.count());

    return (index < kData->custom.count()) ? kData->custom.getAt(index) : kCustomDataNull;
}

int32_t CarlaPlugin::chunkData(void** const dataPtr)
{
    CARLA_ASSERT(dataPtr != nullptr);
    return 0;

    // unused
    Q_UNUSED(dataPtr);
}

// -------------------------------------------------------------------
// Information (per-plugin data)

float CarlaPlugin::getParameterValue(const uint32_t parameterId)
{
    CARLA_ASSERT(parameterId < parameterCount());
    return 0.0f;

    // unused
    Q_UNUSED(parameterId);
}

float CarlaPlugin::getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
{
    CARLA_ASSERT(parameterId < parameterCount());
    CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));
    return 0.0f;

    // unused
    Q_UNUSED(parameterId);
    Q_UNUSED(scalePointId);
}

void CarlaPlugin::getLabel(char* const strBuf)
{
    *strBuf = 0;
}

void CarlaPlugin::getMaker(char* const strBuf)
{
    *strBuf = 0;
}

void CarlaPlugin::getCopyright(char* const strBuf)
{
    *strBuf = 0;
}

void CarlaPlugin::getRealName(char* const strBuf)
{
    *strBuf = 0;
}

void CarlaPlugin::getParameterName(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    *strBuf = 0;
    return;

    // unused
    Q_UNUSED(parameterId);
}

void CarlaPlugin::getParameterSymbol(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    *strBuf = 0;
    return;

    // unused
    Q_UNUSED(parameterId);
}

void CarlaPlugin::getParameterText(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    *strBuf = 0;
    return;

    // unused
    Q_UNUSED(parameterId);
}

void CarlaPlugin::getParameterUnit(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    *strBuf = 0;
    return;

    // unused
    Q_UNUSED(parameterId);
}

void CarlaPlugin::getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));
    *strBuf = 0;
    return;

    // unused
    Q_UNUSED(parameterId);
    Q_UNUSED(scalePointId);
}

void CarlaPlugin::getProgramName(const uint32_t index, char* const strBuf)
{
    CARLA_ASSERT(index < kData->prog.count);
    CARLA_ASSERT(kData->prog.names[index] != nullptr);

    if (index < kData->prog.count && kData->prog.names[index])
        std::strncpy(strBuf, kData->prog.names[index], STR_MAX);
    else
        *strBuf = 0;
}

void CarlaPlugin::getMidiProgramName(const uint32_t index, char* const strBuf)
{
    CARLA_ASSERT(index < kData->midiprog.count);
    CARLA_ASSERT(kData->midiprog.data[index].name != nullptr);

    if (index < kData->midiprog.count && kData->midiprog.data[index].name)
        std::strncpy(strBuf, kData->midiprog.data[index].name, STR_MAX);
    else
        *strBuf = 0;
}

void CarlaPlugin::getParameterCountInfo(uint32_t* const ins, uint32_t* const outs, uint32_t* const total)
{
    CARLA_ASSERT(ins != nullptr);
    CARLA_ASSERT(outs != nullptr);
    CARLA_ASSERT(total != nullptr);

    if (ins == nullptr || outs == nullptr || total == nullptr)
        return;

    *ins   = 0;
    *outs  = 0;
    *total = kData->param.count;

    for (uint32_t i=0; i < kData->param.count; i++)
    {
        if (kData->param.data[i].type == PARAMETER_INPUT)
            *ins += 1;
        else if (kData->param.data[i].type == PARAMETER_OUTPUT)
            *outs += 1;
    }
}

// -------------------------------------------------------------------
// Set data (state)

const SaveState& CarlaPlugin::getSaveState()
{
    static SaveState saveState;

    // TODO
    return saveState;
}

void CarlaPlugin::loadSaveState(const SaveState& saveState)
{
    // TODO
    Q_UNUSED(saveState);
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setId(const unsigned int id)
{
    fId = id;

    if (kData->engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK)
        kData->ctrlInChannel = id;
}

void CarlaPlugin::setEnabled(const bool yesNo)
{
    fEnabled = yesNo;
}

void CarlaPlugin::setActive(const bool active, const bool sendOsc, const bool sendCallback)
{
    kData->active = active;

    const float value = active ? 1.0f : 0.0f;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_ACTIVE, value);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_ACTIVE, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_ACTIVE, value);
#endif
}

void CarlaPlugin::setDryWet(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(0.0f, 1.0f, value);

    kData->postProc.dryWet = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_DRYWET, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_DRYWET, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_DRYWET, fixedValue);
#endif
}

void CarlaPlugin::setVolume(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0f && value <= 1.27f);

    const float fixedValue = carla_fixValue<float>(0.0f, 1.27f, value);

    kData->postProc.volume = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_VOLUME, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_VOLUME, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_VOLUME, fixedValue);
#endif
}

void CarlaPlugin::setBalanceLeft(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(-1.0f, 1.0f, value);

    kData->postProc.balanceLeft = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_LEFT, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_BALANCE_LEFT, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_BALANCE_LEFT, fixedValue);
#endif
}

void CarlaPlugin::setBalanceRight(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(-1.0f, 1.0f, value);

    kData->postProc.balanceRight = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_RIGHT, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_BALANCE_RIGHT, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_BALANCE_RIGHT, fixedValue);
#endif
}

void CarlaPlugin::setPanning(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(-1.0f, 1.0f, value);

    kData->postProc.panning = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_PANNING, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_PANNING, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_PANNING, fixedValue);
#endif
}

#if 0 //ndef BUILD_BRIDGE
int CarlaPlugin::setOscBridgeInfo(const PluginBridgeInfoType, const int, const lo_arg* const* const, const char* const)
{
    return 1;
}
#endif

// -------------------------------------------------------------------
// Set data (plugin-specific stuff)

void CarlaPlugin::setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < kData->param.count);

    if (sendGui)
        uiParameterChange(parameterId, value);

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, parameterId, value);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, parameterId, 0, value, nullptr);
}

void CarlaPlugin::setParameterValueByRIndex(const int32_t rindex, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(rindex > PARAMETER_MAX && rindex != PARAMETER_NULL);

    if (rindex <= PARAMETER_MAX)
        return;
    if (rindex == PARAMETER_NULL)
        return;
    if (rindex == PARAMETER_ACTIVE)
        return setActive(value > 0.0, sendOsc, sendCallback);
    if (rindex == PARAMETER_DRYWET)
        return setDryWet(value, sendOsc, sendCallback);
    if (rindex == PARAMETER_VOLUME)
        return setVolume(value, sendOsc, sendCallback);
    if (rindex == PARAMETER_BALANCE_LEFT)
        return setBalanceLeft(value, sendOsc, sendCallback);
    if (rindex == PARAMETER_BALANCE_RIGHT)
        return setBalanceRight(value, sendOsc, sendCallback);
    if (rindex == PARAMETER_PANNING)
        return setPanning(value, sendOsc, sendCallback);

    for (uint32_t i=0; i < kData->param.count; i++)
    {
        if (kData->param.data[i].rindex == rindex)
            return setParameterValue(i, value, sendGui, sendOsc, sendCallback);
    }
}

void CarlaPlugin::setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < kData->param.count);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

    if (channel >= MAX_MIDI_CHANNELS)
        channel = MAX_MIDI_CHANNELS;

    kData->param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_midi_channel(fId, parameterId, channel);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, fId, parameterId, channel, 0.0f, nullptr);
}

void CarlaPlugin::setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < kData->param.count);
    CARLA_ASSERT(cc >= -1);

    if (cc < -1 || cc > 0x5F)
        cc = -1;

    kData->param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_midi_cc(fId, parameterId, cc);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_MIDI_CC_CHANGED, fId, parameterId, cc, 0.0f, nullptr);
}

void CarlaPlugin::setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
{
    CARLA_ASSERT(type  != nullptr);
    CARLA_ASSERT(key   != nullptr);
    CARLA_ASSERT(value != nullptr);

    if (type == nullptr)
        return qCritical("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

    if (key == nullptr)
        return qCritical("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

    if (value == nullptr)
        return qCritical("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

    bool saveData = true;

    if (std::strcmp(type, CUSTOM_DATA_STRING) == 0)
    {
        // Ignore some keys
        if (std::strncmp(key, "OSC:", 4) == 0 || std::strcmp(key, "guiVisible") == 0)
            saveData = false;
        //else if (strcmp(key, CARLA_BRIDGE_MSG_SAVE_NOW) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CHUNK) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CUSTOM) == 0)
        //    saveData = false;
    }

    if (saveData)
    {
#if 0
        // Check if we already have this key
        for (size_t i=0; i < kData->custom.count(); i++)
        {
            if (strcmp(custom[i].key, key) == 0)
            {
                delete[] custom[i].value;
                custom[i].value = carla_strdup(value);
                return;
            }
        }
#endif

        // Otherwise store it
        CustomData newData;
        newData.type  = carla_strdup(type);
        newData.key   = carla_strdup(key);
        newData.value = carla_strdup(value);
        kData->custom.append(newData);
    }
}

void CarlaPlugin::setChunkData(const char* const stringData)
{
    CARLA_ASSERT(stringData != nullptr);
    return;

    // unused
    Q_UNUSED(stringData);
}

void CarlaPlugin::setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool)
{
    CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->prog.count));

    if (index > static_cast<int32_t>(kData->prog.count))
        return;

    const int32_t fixedIndex = carla_fixValue<int32_t>(-1, kData->prog.count, index);

    kData->prog.current = fixedIndex;

    // Change default parameter values
    if (fixedIndex >= 0)
    {
        if (sendGui)
            uiProgramChange(fixedIndex);

        for (uint32_t i=0; i < kData->param.count; i++)
        {
            // FIXME?
            kData->param.ranges[i].def = getParameterValue(i);
            kData->param.ranges[i].fixDefault();

            if (sendOsc)
            {
#ifndef BUILD_BRIDGE
                kData->engine->osc_send_control_set_default_value(fId, i, kData->param.ranges[i].def);
                kData->engine->osc_send_control_set_parameter_value(fId, i, kData->param.ranges[i].def);
#endif
            }
        }
    }

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_program(fId, fixedIndex);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PROGRAM_CHANGED, fId, fixedIndex, 0, 0.0f, nullptr);
}

void CarlaPlugin::setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool)
{
    CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

    if (index > static_cast<int32_t>(kData->midiprog.count))
        return;

    const int32_t fixedIndex = carla_fixValue<int32_t>(-1, kData->midiprog.count, index);

    kData->midiprog.current = fixedIndex;

    if (fixedIndex >= 0)
    {
        if (sendGui)
            uiMidiProgramChange(fixedIndex);

        // Change default parameter values (sound banks never change defaults)
#ifndef BUILD_BRIDGE // FIXME
        if (type() != PLUGIN_GIG && type() != PLUGIN_SF2 && type() != PLUGIN_SFZ)
#endif
        {
            for (uint32_t i=0; i < kData->param.count; i++)
            {
                // FIXME?
                kData->param.ranges[i].def = getParameterValue(i);
                kData->param.ranges[i].fixDefault();

                if (sendOsc)
                {
#ifndef BUILD_BRIDGE
                    kData->engine->osc_send_control_set_default_value(fId, i, kData->param.ranges[i].def);
                    kData->engine->osc_send_control_set_parameter_value(fId, i, kData->param.ranges[i].def);
#endif
                }
            }
        }
    }

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_midi_program(fId, fixedIndex);
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fId, fixedIndex, 0, 0.0f, nullptr);
}

void CarlaPlugin::setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
{
    for (uint32_t i=0; i < kData->midiprog.count; i++)
    {
        if (kData->midiprog.data[i].bank == bank && kData->midiprog.data[i].program == program)
            return setMidiProgram(i, sendGui, sendOsc, sendCallback, block);
    }
}

// -------------------------------------------------------------------
// Set gui stuff

void CarlaPlugin::showGui(const bool yesNo)
{
    Q_UNUSED(yesNo);
}

void CarlaPlugin::idleGui()
{
    if (! fEnabled)
        return;

    if (fHints & PLUGIN_USES_SINGLE_THREAD)
    {
        // Process postponed events
        postRtEventsRun();

        // Update parameter outputs
        for (uint32_t i=0; i < kData->param.count; i++)
        {
            if (kData->param.data[i].type == PARAMETER_OUTPUT)
                uiParameterChange(i, getParameterValue(i));
        }
    }
}

// -------------------------------------------------------------------
// Plugin state

void CarlaPlugin::reload()
{
}

void CarlaPlugin::reloadPrograms(const bool)
{
}

void CarlaPlugin::prepareForSave()
{
}

// -------------------------------------------------------------------
// Plugin processing

void CarlaPlugin::process(float** const, float** const, const uint32_t, const uint32_t)
{
}

void CarlaPlugin::bufferSizeChanged(const uint32_t)
{
}

void CarlaPlugin::sampleRateChanged(const double)
{
}

void CarlaPlugin::recreateLatencyBuffers()
{
#if 0
    if (kData->latencyBuffers)
    {
        for (uint32_t i=0; i < kData->audioIn.count; i++)
            delete[] kData->latencyBuffers[i];

        delete[] kData->latencyBuffers;
    }

    if (kData->audioIn.count > 0 && kData->latency > 0)
    {
        kData->latencyBuffers = new float*[kData->audioIn.count];

        for (uint32_t i=0; i < kData->audioIn.count; i++)
            kData->latencyBuffers[i] = new float[kData->latency];
    }
    else
        kData->latencyBuffers = nullptr;
#endif
}

// -------------------------------------------------------------------
// OSC stuff

void CarlaPlugin::registerToOscClient()
{
#ifdef BUILD_BRIDGE
    if (! kData->engine->isOscBridgeRegistered())
        return;
#else
    if (! kData->engine->isOscControlRegistered())
        return;
#endif

#ifndef BUILD_BRIDGE
    kData->engine->osc_send_control_add_plugin_start(fId, fName);
#endif

    // Base data
    {
        char bufName[STR_MAX]  = { 0 };
        char bufLabel[STR_MAX] = { 0 };
        char bufMaker[STR_MAX] = { 0 };
        char bufCopyright[STR_MAX] = { 0 };
        getRealName(bufName);
        getLabel(bufLabel);
        getMaker(bufMaker);
        getCopyright(bufCopyright);

#ifdef BUILD_BRIDGE
        kData->engine->osc_send_bridge_plugin_info(category(), fHints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
#else
        kData->engine->osc_send_control_set_plugin_data(fId, type(), category(), fHints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
#endif
    }

    // Base count
    {
        uint32_t cIns, cOuts, cTotals;
        getParameterCountInfo(&cIns, &cOuts, &cTotals);

#ifdef BUILD_BRIDGE
        kData->engine->osc_send_bridge_audio_count(audioInCount(), audioOutCount(), audioInCount() + audioOutCount());
        kData->engine->osc_send_bridge_midi_count(midiInCount(), midiOutCount(), midiInCount() + midiOutCount());
        kData->engine->osc_send_bridge_parameter_count(cIns, cOuts, cTotals);
#else
        kData->engine->osc_send_control_set_plugin_ports(fId, audioInCount(), audioOutCount(), midiInCount(), midiOutCount(), cIns, cOuts, cTotals);
#endif
    }

    // Plugin Parameters
    if (kData->param.count > 0 && kData->param.count < kData->engine->getOptions().maxParameters)
    {
        char bufName[STR_MAX], bufUnit[STR_MAX];

        for (uint32_t i=0; i < kData->param.count; i++)
        {
            getParameterName(i, bufName);
            getParameterUnit(i, bufUnit);

            const ParameterData& paramData(kData->param.data[i]);
            const ParameterRanges& paramRanges(kData->param.ranges[i]);

#ifdef BUILD_BRIDGE
            kData->engine->osc_send_bridge_parameter_info(i, bufName, bufUnit);
            kData->engine->osc_send_bridge_parameter_data(i, paramData.type, paramData.rindex, paramData.hints, paramData.midiChannel, paramData.midiCC);
            kData->engine->osc_send_bridge_parameter_ranges(i, paramRanges.def, paramRanges.min, paramRanges.max, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            kData->engine->osc_send_bridge_set_parameter_value(i, getParameterValue(i));
#else
            kData->engine->osc_send_control_set_parameter_data(fId, i, paramData.type, paramData.hints, bufName, bufUnit, getParameterValue(i));
            kData->engine->osc_send_control_set_parameter_ranges(fId, i, paramRanges.min, paramRanges.max, paramRanges.def, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            kData->engine->osc_send_control_set_parameter_midi_cc(fId, i, paramData.midiCC);
            kData->engine->osc_send_control_set_parameter_midi_channel(fId, i, paramData.midiChannel);
            kData->engine->osc_send_control_set_parameter_value(fId, i, getParameterValue(i));
#endif
        }
    }

    // Programs
    if (kData->prog.count > 0)
    {
#ifdef BUILD_BRIDGE
        kData->engine->osc_send_bridge_program_count(kData->prog.count);

        for (uint32_t i=0; i < kData->prog.count; i++)
            kData->engine->osc_send_bridge_program_info(i, kData->prog.names[i]);

        kData->engine->osc_send_bridge_set_program(kData->prog.current);
#else
        kData->engine->osc_send_control_set_program_count(fId, kData->prog.count);

        for (uint32_t i=0; i < kData->prog.count; i++)
            kData->engine->osc_send_control_set_program_name(fId, i, kData->prog.names[i]);

        kData->engine->osc_send_control_set_program(fId, kData->prog.current);
#endif
    }

    // MIDI Programs
    if (kData->midiprog.count > 0)
    {
#ifdef BUILD_BRIDGE
        kData->engine->osc_send_bridge_midi_program_count(kData->midiprog.count);

        for (uint32_t i=0; i < kData->midiprog.count; i++)
        {
            const MidiProgramData& mpData(kData->midiprog.data[i]);

            kData->engine->osc_send_bridge_midi_program_info(i, mpData.bank, mpData.program, mpData.name);
        }

        kData->engine->osc_send_bridge_set_midi_program(kData->midiprog.current);
#else
        kData->engine->osc_send_control_set_midi_program_count(fId, kData->midiprog.count);

        for (uint32_t i=0; i < kData->midiprog.count; i++)
        {
            const MidiProgramData& mpData(kData->midiprog.data[i]);

            kData->engine->osc_send_control_set_midi_program_data(fId, i, mpData.bank, mpData.program, mpData.name);
        }

        kData->engine->osc_send_control_set_midi_program(fId, kData->midiprog.current);
#endif
    }

#ifndef BUILD_BRIDGE
    kData->engine->osc_send_control_add_plugin_end(fId);

    // Internal Parameters
    {
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_ACTIVE, kData->active ? 1.0 : 0.0);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_DRYWET, kData->postProc.dryWet);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_VOLUME, kData->postProc.volume);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_LEFT, kData->postProc.balanceLeft);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_RIGHT, kData->postProc.balanceRight);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_PANNING, kData->postProc.panning);
    }
#endif
}

void CarlaPlugin::updateOscData(const lo_address& source, const char* const url)
{
    // FIXME - remove debug prints later
    qWarning("CarlaPlugin::updateOscData(%p, \"%s\")", source, url);

    kData->osc.data.free();

    const int proto = lo_address_get_protocol(source);

    {
        const char* host = lo_address_get_hostname(source);
        const char* port = lo_address_get_port(source);
        kData->osc.data.source = lo_address_new_with_proto(proto, host, port);

        qWarning("CarlaPlugin::updateOscData() - source: host \"%s\", port \"%s\"", host, port);
    }

    {
        char* host = lo_url_get_hostname(url);
        char* port = lo_url_get_port(url);
        kData->osc.data.path   = carla_strdup_free(lo_url_get_path(url));
        kData->osc.data.target = lo_address_new_with_proto(proto, host, port);
        qWarning("CarlaPlugin::updateOscData() - target: host \"%s\", port \"%s\", path \"%s\"", host, port, kData->osc.data.path);

        std::free(host);
        std::free(port);
    }

#ifndef BUILD_BRIDGE
    if (fHints & PLUGIN_IS_BRIDGE)
        return;
#endif

    osc_send_sample_rate(&kData->osc.data, kData->engine->getSampleRate());

#if 0
    for (size_t i=0; i < kData->custom.count(); i++)
    {
        // TODO
        //if (m_type == PLUGIN_LV2)
        //osc_send_lv2_transfer_event(&osc.data, getCustomDataTypeString(custom[i].type), /*custom[i].key,*/ custom[i].value);
        //else
        if (custom[i].type == CUSTOM_DATA_STRING)
            osc_send_configure(&osc.data, custom[i].key, custom[i].value);
    }
#endif

    if (kData->prog.current >= 0)
        osc_send_program(&kData->osc.data, kData->prog.current);

    if (kData->midiprog.current >= 0)
    {
        const MidiProgramData& curMidiProg(kData->midiprog.getCurrent());

        if (type() == PLUGIN_DSSI)
            osc_send_program(&kData->osc.data, curMidiProg.bank, curMidiProg.program);
        else
            osc_send_midi_program(&kData->osc.data, curMidiProg.bank, curMidiProg.program);
    }

    for (uint32_t i=0; i < kData->param.count; i++)
        osc_send_control(&kData->osc.data, kData->param.data[i].rindex, getParameterValue(i));

    qWarning("CarlaPlugin::updateOscData() - done");
}

void CarlaPlugin::freeOscData()
{
    kData->osc.data.free();
}

bool CarlaPlugin::waitForOscGuiShow()
{
    qWarning("CarlaPlugin::waitForOscGuiShow()");

    // wait for UI 'update' call
    for (uint i=0, oscUiTimeout = kData->engine->getOptions().oscUiTimeout; i < oscUiTimeout; i++)
    {
        if (kData->osc.data.target)
        {
            qWarning("CarlaPlugin::waitForOscGuiShow() - got response, asking UI to show itself now");
            osc_send_show(&kData->osc.data);
            return true;
        }
        else
            carla_msleep(100);
    }

    qWarning("CarlaPlugin::waitForOscGuiShow() - Timeout while waiting for UI to respond (waited %u msecs)", kData->engine->getOptions().oscUiTimeout);
    return false;
}

// -------------------------------------------------------------------
// MIDI events

void CarlaPlugin::sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note < MAX_MIDI_NOTE);
    CARLA_ASSERT(velo < MAX_MIDI_VALUE);

    if (! kData->active)
        return;

    ExternalMidiNote extNote;
    extNote.channel = channel;
    extNote.note    = note;
    extNote.velo    = velo;

    kData->extNotes.mutex.lock();
    kData->extNotes.append(extNote);
    kData->extNotes.mutex.unlock();

    if (sendGui)
    {
        if (velo > 0)
            uiNoteOn(channel, note, velo);
        else
            uiNoteOff(channel, note);
    }

#ifndef BUILD_BRIDGE
    if (sendOsc)
    {
        if (velo > 0)
            kData->engine->osc_send_control_note_on(fId, channel, note, velo);
        else
            kData->engine->osc_send_control_note_off(fId, channel, note);
    }
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        kData->engine->callback(velo ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, fId, channel, note, velo, nullptr);
}

void CarlaPlugin::sendMidiAllNotesOff()
{
    kData->postRtEvents.mutex.lock();

    PluginPostRtEvent postEvent;
    postEvent.type   = kPluginPostRtEventNoteOff;
    postEvent.value1 = kData->ctrlInChannel;
    postEvent.value2 = 0;
    postEvent.value3 = 0.0;

    for (unsigned short i=0; i < MAX_MIDI_NOTE; i++)
    {
        postEvent.value2 = i;
        kData->postRtEvents.data.append(postEvent);
    }

    kData->postRtEvents.mutex.unlock();
}

// -------------------------------------------------------------------
// Post-poned events

void CarlaPlugin::postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const double value3)
{
    PluginPostRtEvent event;
    event.type   = type;
    event.value1 = value1;
    event.value2 = value2;
    event.value3 = value3;

    kData->postRtEvents.appendRT(event);
}

void CarlaPlugin::postRtEventsRun()
{
    unsigned short k = 0;
    PluginPostRtEvent listData[MAX_RT_EVENTS];

    // Make a safe copy of events while clearing them
    kData->postRtEvents.mutex.lock();

    while (! kData->postRtEvents.data.isEmpty())
    {
        PluginPostRtEvent& event = kData->postRtEvents.data.getFirst(true);
        listData[k++] = event;
        //std::memcpy(&listData[k++], &event, sizeof(PluginPostRtEvent));
    }

    kData->postRtEvents.mutex.unlock();

    // Handle events now
    for (unsigned short i=0; i < k; i++)
    {
        const PluginPostRtEvent& event = listData[i];

        switch (event.type)
        {
        case kPluginPostRtEventNull:
            break;

        case kPluginPostRtEventDebug:
            kData->engine->callback(CALLBACK_DEBUG, fId, event.value1, event.value2, event.value3, nullptr);
            break;

        case kPluginPostRtEventParameterChange:
            // Update UI
            if (event.value1 >= 0)
                uiParameterChange(event.value1, event.value3);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (kData->engine->isOscControlRegistered())
                kData->engine->osc_send_control_set_parameter_value(fId, event.value1, event.value3);
#endif

            // Update Host
            kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, event.value1, 0, event.value3, nullptr);
            break;

        case kPluginPostRtEventProgramChange:
            // Update UI
            if (event.value1 >= 0)
                uiProgramChange(event.value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (kData->engine->isOscControlRegistered())
            {
                kData->engine->osc_send_control_set_program(fId, event.value1);

                for (uint32_t j=0; j < kData->param.count; j++)
                    kData->engine->osc_send_control_set_default_value(fId, j, kData->param.ranges[j].def);
            }
#endif

            // Update Host
            kData->engine->callback(CALLBACK_PROGRAM_CHANGED, fId, event.value1, 0, 0.0, nullptr);
            break;

        case kPluginPostRtEventMidiProgramChange:
            // Update UI
            if (event.value1 >= 0)
                uiMidiProgramChange(event.value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (kData->engine->isOscControlRegistered())
            {
                kData->engine->osc_send_control_set_midi_program(fId, event.value1);

                for (uint32_t j=0; j < kData->param.count; j++)
                    kData->engine->osc_send_control_set_default_value(fId, j, kData->param.ranges[j].def);
            }
#endif

            // Update Host
            kData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fId, event.value1, 0, 0.0, nullptr);
            break;

        case kPluginPostRtEventNoteOn:
            // Update UI
            uiNoteOn(event.value1, event.value2, int(event.value3));

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (kData->engine->isOscControlRegistered())
                kData->engine->osc_send_control_note_on(fId, event.value1, event.value2, int(event.value3));
#endif

            // Update Host
            kData->engine->callback(CALLBACK_NOTE_ON, fId, event.value1, event.value2, int(event.value3), nullptr);
            break;

        case kPluginPostRtEventNoteOff:
            // Update UI
            uiNoteOff(event.value1, event.value2);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (kData->engine->isOscControlRegistered())
                kData->engine->osc_send_control_note_off(fId, event.value1, event.value2);
#endif

            // Update Host
            kData->engine->callback(CALLBACK_NOTE_OFF, fId, event.value1, event.value2, 0.0, nullptr);
            break;
        }
    }
}

void CarlaPlugin::uiParameterChange(const uint32_t index, const float value)
{
    CARLA_ASSERT(index < parameterCount());
    return;

    // unused
    Q_UNUSED(index);
    Q_UNUSED(value);
}

void CarlaPlugin::uiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < programCount());
    return;

    // unused
    Q_UNUSED(index);
}

void CarlaPlugin::uiMidiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < midiProgramCount());
    return;

    // unused
    Q_UNUSED(index);
}

void CarlaPlugin::uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
{
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note < MAX_MIDI_NOTE);
    CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);
    return;

    // unused
    Q_UNUSED(channel);
    Q_UNUSED(note);
    Q_UNUSED(velo);
}

void CarlaPlugin::uiNoteOff(const uint8_t channel, const uint8_t note)
{
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note < MAX_MIDI_NOTE);
    return;

    // unused
    Q_UNUSED(channel);
    Q_UNUSED(note);
}

// -------------------------------------------------------------------
// Cleanup

void CarlaPlugin::initBuffers()
{
    kData->audioIn.initBuffers(kData->engine);
    kData->audioOut.initBuffers(kData->engine);
    kData->event.initBuffers(kData->engine);
}

void CarlaPlugin::deleteBuffers()
{
    qDebug("CarlaPlugin::deleteBuffers() - start");

    kData->audioIn.clear();
    kData->audioOut.clear();
    kData->param.clear();
    kData->event.clear();

    qDebug("CarlaPlugin::deleteBuffers() - end");
}

// -------------------------------------------------------------------
// Library functions

bool CarlaPlugin::libOpen(const char* const filename)
{
    kData->lib = lib_open(filename);
    return bool(kData->lib);
}

bool CarlaPlugin::libClose()
{
    if (kData->lib == nullptr)
        return false;

    const bool ret = lib_close(kData->lib);
    kData->lib = nullptr;
    return ret;
}

void* CarlaPlugin::libSymbol(const char* const symbol)
{
    return lib_symbol(kData->lib, symbol);
}

const char* CarlaPlugin::libError(const char* const filename)
{
    return lib_error(filename);
}

// -------------------------------------------------------------------
// Scoped Disabler

CarlaPlugin::ScopedDisabler::ScopedDisabler(CarlaPlugin* const plugin)
    : kPlugin(plugin)
{
    if (plugin->fEnabled)
    {
        plugin->fEnabled = false;
        plugin->kData->engine->waitForProccessEnd();
    }
}

CarlaPlugin::ScopedDisabler::~ScopedDisabler()
{
    kPlugin->fEnabled = true;
}

// -------------------------------------------------------------------
// CarlaPluginGUI

CarlaPluginGUI::CarlaPluginGUI(QWidget* const parent, Callback* const callback)
    : QMainWindow(parent),
      kCallback(callback)
{
    qDebug("CarlaPluginGUI::CarlaPluginGUI(%p)", parent);
    //CARLA_ASSERT(callback);

    //m_container = new GuiContainer(this);
    //setCentralWidget(m_container);
    //adjustSize();

    //m_container->setParent(this);
    //m_container->show();

    //m_resizable = true;

    //setNewSize(50, 50);

    QMainWindow::setVisible(false);
}

CarlaPluginGUI::~CarlaPluginGUI()
{
    qDebug("CarlaPluginGUI::~CarlaPluginGUI()");
    //CARLA_ASSERT(m_container);

    // FIXME, automatically deleted by parent ?
    //delete m_container;
}

#if 0

// -------------------------------------------------------------------

GuiContainer* CarlaPluginGUI::getContainer() const
{
    return m_container;
}

WId CarlaPluginGUI::getWinId() const
{
    return m_container->winId();
}

// -------------------------------------------------------------------

void CarlaPluginGUI::setNewSize(int width, int height)
{
    qDebug("CarlaPluginGUI::setNewSize(%i, %i)", width, height);

    if (width < 30)
        width = 30;
    if (height < 30)
        height = 30;

    if (m_resizable)
    {
        resize(width, height);
    }
    else
    {
        setFixedSize(width, height);
        m_container->setFixedSize(width, height);
    }
}

void CarlaPluginGUI::setResizable(bool resizable)
{
    m_resizable = resizable;
    setNewSize(width(), height());

#ifdef Q_OS_WIN
    if (! resizable)
        setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
}

void CarlaPluginGUI::setTitle(const char* const title)
{
    CARLA_ASSERT(title);
    setWindowTitle(QString("%1 (GUI)").arg(title));
}

void CarlaPluginGUI::setVisible(const bool yesNo)
{
    qDebug("CarlaPluginGUI::setVisible(%s)", bool2str(yesNo));

    if (yesNo)
    {
        if (! m_geometry.isNull())
            restoreGeometry(m_geometry);
    }
    else
        m_geometry = saveGeometry();

    QMainWindow::setVisible(yesNo);
}

// -------------------------------------------------------------------

void CarlaPluginGUI::hideEvent(QHideEvent* const event)
{
    qDebug("CarlaPluginGUI::hideEvent(%p)", event);
    CARLA_ASSERT(event);

    event->accept();
    close();
}

void CarlaPluginGUI::closeEvent(QCloseEvent* const event)
{
    qDebug("CarlaPluginGUI::closeEvent(%p)", event);
    CARLA_ASSERT(event);

    if (event->spontaneous())
    {
        if (m_callback)
            m_callback->guiClosedCallback();

        QMainWindow::closeEvent(event);
        return;
    }

    event->ignore();
}
#endif

// -------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
