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
// Constructor and destructor

CarlaPlugin::CarlaPlugin(CarlaEngine* const engine, const unsigned int id)
    : fData(new CarlaPluginProtectedData(engine, id))
{
    CARLA_ASSERT(fData != nullptr);
    CARLA_ASSERT(engine != nullptr);
    CARLA_ASSERT(id < engine->maxPluginNumber());
    qDebug("CarlaPlugin::CarlaPlugin(%p, %i)", engine, id);

    if (engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK)
    {
        CARLA_ASSERT(id < MAX_RACK_PLUGINS && id < MAX_MIDI_CHANNELS);

        if (id < MAX_RACK_PLUGINS && id < MAX_MIDI_CHANNELS)
            fData->ctrlInChannel = id;
    }
}

CarlaPlugin::~CarlaPlugin()
{
    qDebug("CarlaPlugin::~CarlaPlugin()");

    // Remove client and ports
    if (fData->client != nullptr)
    {
        if (fData->client->isActive())
            fData->client->deactivate();

        removeClientPorts();

        delete fData->client;
    }

    // Delete data
    deleteBuffers();

    // Unload DLL
    libClose();

    fData->prog.clear();
    fData->midiprog.clear();
    fData->custom.clear();

#if 0
    if (fData->latencyBuffers)
    {
        for (uint32_t i=0; i < fData->audioIn.count; i++)
            delete[] fData->latencyBuffers[i];

        delete[] fData->latencyBuffers;
    }
#endif
}

// -------------------------------------------------------------------
// Information (base)

unsigned int CarlaPlugin::id() const
{
    return fData->id;
}

unsigned int CarlaPlugin::hints() const
{
    return fData->hints;
}

unsigned int CarlaPlugin::options() const
{
    return fData->options;
}

bool CarlaPlugin::enabled() const
{
    return fData->enabled;
}

const char* CarlaPlugin::name() const
{
    return (const char*)fData->name;
}

const char* CarlaPlugin::filename() const
{
    return (const char*)fData->filename;
}

uint32_t CarlaPlugin::latency()
{
    return 0;
}

// -------------------------------------------------------------------
// Information (count)

uint32_t CarlaPlugin::audioInCount()
{
    return fData->audioIn.count;
}

uint32_t CarlaPlugin::audioOutCount()
{
    return fData->audioOut.count;
}

uint32_t CarlaPlugin::midiInCount()
{
    return (fData->options2 & PLUGIN_OPTION2_HAS_MIDI_IN) ? 1 : 0;
}

uint32_t CarlaPlugin::midiOutCount()
{
    return (fData->options2 & PLUGIN_OPTION2_HAS_MIDI_OUT) ? 1 : 0;
}

uint32_t CarlaPlugin::parameterCount() const
{
    return fData->param.count;
}

uint32_t CarlaPlugin::parameterScalePointCount(const uint32_t parameterId)
{
    CARLA_ASSERT(parameterId < fData->param.count);
    return 0;

    // unused
    Q_UNUSED(parameterId);
}

uint32_t CarlaPlugin::programCount() const
{
    return fData->prog.count;
}

uint32_t CarlaPlugin::midiProgramCount() const
{
    return fData->midiprog.count;
}

size_t CarlaPlugin::customDataCount() const
{
    return fData->custom.count();
}

// -------------------------------------------------------------------
// Information (current data)

int32_t CarlaPlugin::currentProgram() const
{
    return fData->prog.current;
}

int32_t CarlaPlugin::currentMidiProgram() const
{
    return fData->midiprog.current;
}

const ParameterData& CarlaPlugin::parameterData(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < fData->param.count);

    return (parameterId < fData->param.count) ? fData->param.data[parameterId] : kParameterDataNull;
}

const ParameterRanges& CarlaPlugin::parameterRanges(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < fData->param.count);

    return (parameterId < fData->param.count) ? fData->param.ranges[parameterId] : kParameterRangesNull;
}

bool CarlaPlugin::parameterIsOutput(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < fData->param.count);

    return (parameterId < fData->param.count) ? (fData->param.data[parameterId].type == PARAMETER_OUTPUT) : false;
}

const MidiProgramData& CarlaPlugin::midiProgramData(const uint32_t index) const
{
    CARLA_ASSERT(index < fData->midiprog.count);

    return (index < fData->midiprog.count) ? fData->midiprog.data[index] : kMidiProgramDataNull;
}

const CustomData& CarlaPlugin::customData(const size_t index) const
{
    CARLA_ASSERT(index < fData->custom.count());

    return (index < fData->custom.count()) ? fData->custom.getAt(index) : kCustomDataNull;
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
    CARLA_ASSERT(index < fData->prog.count);
    CARLA_ASSERT(fData->prog.names[index] != nullptr);

    if (index < fData->prog.count && fData->prog.names[index])
        std::strncpy(strBuf, fData->prog.names[index], STR_MAX);
    else
        *strBuf = 0;
}

void CarlaPlugin::getMidiProgramName(const uint32_t index, char* const strBuf)
{
    CARLA_ASSERT(index < fData->midiprog.count);
    CARLA_ASSERT(fData->midiprog.data[index].name != nullptr);

    if (index < fData->midiprog.count && fData->midiprog.data[index].name)
        std::strncpy(strBuf, fData->midiprog.data[index].name, STR_MAX);
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
    *total = fData->param.count;

    for (uint32_t i=0; i < fData->param.count; i++)
    {
        if (fData->param.data[i].type == PARAMETER_INPUT)
            *ins += 1;
        else if (fData->param.data[i].type == PARAMETER_OUTPUT)
            *outs += 1;
    }
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setId(const unsigned int id)
{
    fData->id = id;

    if (fData->engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK)
        fData->ctrlInChannel = id;
}

void CarlaPlugin::setEnabled(const bool yesNo)
{
    fData->enabled = yesNo;
}

void CarlaPlugin::setActive(const bool active, const bool sendOsc, const bool sendCallback)
{
    fData->active = active;

    const float value = active ? 1.0f : 0.0f;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_ACTIVE, value);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, PARAMETER_ACTIVE, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (fData->hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&fData->osc.data, PARAMETER_ACTIVE, value);
#endif
}

void CarlaPlugin::setDryWet(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(0.0f, 1.0f, value);

    fData->postProc.dryWet = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_DRYWET, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, PARAMETER_DRYWET, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fData->hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&fData->osc.data, PARAMETER_DRYWET, fixedValue);
#endif
}

void CarlaPlugin::setVolume(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0f && value <= 1.27f);

    const float fixedValue = carla_fixValue<float>(0.0f, 1.27f, value);

    fData->postProc.volume = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_VOLUME, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, PARAMETER_VOLUME, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fData->hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&fData->osc.data, PARAMETER_VOLUME, fixedValue);
#endif
}

void CarlaPlugin::setBalanceLeft(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(-1.0f, 1.0f, value);

    fData->postProc.balanceLeft = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_BALANCE_LEFT, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, PARAMETER_BALANCE_LEFT, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fData->hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&fData->osc.data, PARAMETER_BALANCE_LEFT, fixedValue);
#endif
}

void CarlaPlugin::setBalanceRight(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(-1.0f, 1.0f, value);

    fData->postProc.balanceRight = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_BALANCE_RIGHT, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, PARAMETER_BALANCE_RIGHT, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fData->hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&fData->osc.data, PARAMETER_BALANCE_RIGHT, fixedValue);
#endif
}

void CarlaPlugin::setPanning(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue = carla_fixValue<float>(-1.0f, 1.0f, value);

    fData->postProc.panning = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_PANNING, fixedValue);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, PARAMETER_PANNING, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fData->hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&fData->osc.data, PARAMETER_PANNING, fixedValue);
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
    CARLA_ASSERT(parameterId < fData->param.count);

    if (sendGui)
        uiParameterChange(parameterId, value);

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_value(fData->id, parameterId, value);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, parameterId, 0, value, nullptr);
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

    for (uint32_t i=0; i < fData->param.count; i++)
    {
        if (fData->param.data[i].rindex == rindex)
            return setParameterValue(i, value, sendGui, sendOsc, sendCallback);
    }
}

void CarlaPlugin::setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < fData->param.count);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

    if (channel >= MAX_MIDI_CHANNELS)
        channel = MAX_MIDI_CHANNELS;

    fData->param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_midi_channel(fData->id, parameterId, channel);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, fData->id, parameterId, channel, 0.0f, nullptr);
}

void CarlaPlugin::setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < fData->param.count);
    CARLA_ASSERT(cc >= -1);

    if (cc < -1 || cc > 0x5F)
        cc = -1;

    fData->param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_parameter_midi_cc(fData->id, parameterId, cc);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PARAMETER_MIDI_CC_CHANGED, fData->id, parameterId, cc, 0.0f, nullptr);
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
        if (strncmp(key, "OSC:", 4) == 0 || std::strcmp(key, "guiVisible") == 0)
            saveData = false;
        //else if (strcmp(key, CARLA_BRIDGE_MSG_SAVE_NOW) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CHUNK) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CUSTOM) == 0)
        //    saveData = false;
    }

    if (saveData)
    {
#if 0
        // Check if we already have this key
        for (size_t i=0; i < fData->custom.count(); i++)
        {
            if (strcmp(custom[i].key, key) == 0)
            {
                free((void*)custom[i].value);
                custom[i].value = strdup(value);
                return;
            }
        }
#endif

        // Otherwise store it
        CustomData newData;
        newData.type  = strdup(type);
        newData.key   = strdup(key);
        newData.value = strdup(value);
        fData->custom.append(newData);
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
    CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(fData->prog.count));

    if (index > static_cast<int32_t>(fData->prog.count))
        return;

    const int32_t fixedIndex = carla_fixValue<int32_t>(-1, fData->prog.count, index);

    fData->prog.current = fixedIndex;

    // Change default parameter values
    if (fixedIndex >= 0)
    {
        if (sendGui)
            uiProgramChange(fixedIndex);

        for (uint32_t i=0; i < fData->param.count; i++)
        {
            // FIXME?
            fData->param.ranges[i].def = getParameterValue(i);
            fData->param.ranges[i].fixDefault();

            if (sendOsc)
            {
#ifndef BUILD_BRIDGE
                fData->engine->osc_send_control_set_default_value(fData->id, i, fData->param.ranges[i].def);
                fData->engine->osc_send_control_set_parameter_value(fData->id, i, fData->param.ranges[i].def);
#endif
            }
        }
    }

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_program(fData->id, fixedIndex);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_PROGRAM_CHANGED, fData->id, fixedIndex, 0, 0.0f, nullptr);
}

void CarlaPlugin::setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool)
{
    CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(fData->midiprog.count));

    if (index > static_cast<int32_t>(fData->midiprog.count))
        return;

    const int32_t fixedIndex = carla_fixValue<int32_t>(-1, fData->midiprog.count, index);

    fData->midiprog.current = fixedIndex;

    if (fixedIndex >= 0)
    {
        if (sendGui)
            uiMidiProgramChange(fixedIndex);

        // Change default parameter values (sound banks never change defaults)
#ifndef BUILD_BRIDGE // FIXME
        if (type() != PLUGIN_GIG && type() != PLUGIN_SF2 && type() != PLUGIN_SFZ)
#endif
        {
            for (uint32_t i=0; i < fData->param.count; i++)
            {
                // FIXME?
                fData->param.ranges[i].def = getParameterValue(i);
                fData->param.ranges[i].fixDefault();

                if (sendOsc)
                {
#ifndef BUILD_BRIDGE
                    fData->engine->osc_send_control_set_default_value(fData->id, i, fData->param.ranges[i].def);
                    fData->engine->osc_send_control_set_parameter_value(fData->id, i, fData->param.ranges[i].def);
#endif
                }
            }
        }
    }

#ifndef BUILD_BRIDGE
    if (sendOsc)
        fData->engine->osc_send_control_set_midi_program(fData->id, fixedIndex);
#endif

    if (sendCallback)
        fData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fData->id, fixedIndex, 0, 0.0f, nullptr);
}

void CarlaPlugin::setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
{
    for (uint32_t i=0; i < fData->midiprog.count; i++)
    {
        if (fData->midiprog.data[i].bank == bank && fData->midiprog.data[i].program == program)
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
    if (! fData->enabled)
        return;

    if (fData->hints & PLUGIN_USES_SINGLE_THREAD)
    {
        // Process postponed events
        postRtEventsRun();

        // Update parameter outputs
        for (uint32_t i=0; i < fData->param.count; i++)
        {
            if (fData->param.data[i].type == PARAMETER_OUTPUT)
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
    if (fData->latencyBuffers)
    {
        for (uint32_t i=0; i < fData->audioIn.count; i++)
            delete[] fData->latencyBuffers[i];

        delete[] fData->latencyBuffers;
    }

    if (fData->audioIn.count > 0 && fData->latency > 0)
    {
        fData->latencyBuffers = new float*[fData->audioIn.count];

        for (uint32_t i=0; i < fData->audioIn.count; i++)
            fData->latencyBuffers[i] = new float[fData->latency];
    }
    else
        fData->latencyBuffers = nullptr;
#endif
}

// -------------------------------------------------------------------
// OSC stuff

void CarlaPlugin::registerToOscClient()
{
#ifdef BUILD_BRIDGE
    if (! fData->engine->isOscBridgeRegistered())
        return;
#else
    if (! fData->engine->isOscControlRegistered())
        return;
#endif

#ifndef BUILD_BRIDGE
    fData->engine->osc_send_control_add_plugin_start(fData->id, fData->name);
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
        fData->engine->osc_send_bridge_plugin_info(category(), fData->hints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
#else
        fData->engine->osc_send_control_set_plugin_data(fData->id, type(), category(), fData->hints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
#endif
    }

    // Base count
    {
        uint32_t cIns, cOuts, cTotals;
        getParameterCountInfo(&cIns, &cOuts, &cTotals);

#ifdef BUILD_BRIDGE
        fData->engine->osc_send_bridge_audio_count(audioInCount(), audioOutCount(), audioInCount() + audioOutCount());
        fData->engine->osc_send_bridge_midi_count(midiInCount(), midiOutCount(), midiInCount() + midiOutCount());
        fData->engine->osc_send_bridge_parameter_count(cIns, cOuts, cTotals);
#else
        fData->engine->osc_send_control_set_plugin_ports(fData->id, audioInCount(), audioOutCount(), midiInCount(), midiOutCount(), cIns, cOuts, cTotals);
#endif
    }

    // Plugin Parameters
    if (fData->param.count > 0 && fData->param.count < fData->engine->getOptions().maxParameters)
    {
        char bufName[STR_MAX], bufUnit[STR_MAX];

        for (uint32_t i=0; i < fData->param.count; i++)
        {
            getParameterName(i, bufName);
            getParameterUnit(i, bufUnit);

            const ParameterData& paramData(fData->param.data[i]);
            const ParameterRanges& paramRanges(fData->param.ranges[i]);

#ifdef BUILD_BRIDGE
            fData->engine->osc_send_bridge_parameter_info(i, bufName, bufUnit);
            fData->engine->osc_send_bridge_parameter_data(i, paramData.type, paramData.rindex, paramData.hints, paramData.midiChannel, paramData.midiCC);
            fData->engine->osc_send_bridge_parameter_ranges(i, paramRanges.def, paramRanges.min, paramRanges.max, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            fData->engine->osc_send_bridge_set_parameter_value(i, getParameterValue(i));
#else
            fData->engine->osc_send_control_set_parameter_data(fData->id, i, paramData.type, paramData.hints, bufName, bufUnit, getParameterValue(i));
            fData->engine->osc_send_control_set_parameter_ranges(fData->id, i, paramRanges.min, paramRanges.max, paramRanges.def, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            fData->engine->osc_send_control_set_parameter_midi_cc(fData->id, i, paramData.midiCC);
            fData->engine->osc_send_control_set_parameter_midi_channel(fData->id, i, paramData.midiChannel);
            fData->engine->osc_send_control_set_parameter_value(fData->id, i, getParameterValue(i));
#endif
        }
    }

    // Programs
    if (fData->prog.count > 0)
    {
#ifdef BUILD_BRIDGE
        fData->engine->osc_send_bridge_program_count(fData->prog.count);

        for (uint32_t i=0; i < fData->prog.count; i++)
            fData->engine->osc_send_bridge_program_info(i, fData->prog.names[i]);

        fData->engine->osc_send_bridge_set_program(fData->prog.current);
#else
        fData->engine->osc_send_control_set_program_count(fData->id, fData->prog.count);

        for (uint32_t i=0; i < fData->prog.count; i++)
            fData->engine->osc_send_control_set_program_name(fData->id, i, fData->prog.names[i]);

        fData->engine->osc_send_control_set_program(fData->id, fData->prog.current);
#endif
    }

    // MIDI Programs
    if (fData->midiprog.count > 0)
    {
#ifdef BUILD_BRIDGE
        fData->engine->osc_send_bridge_midi_program_count(fData->midiprog.count);

        for (uint32_t i=0; i < fData->midiprog.count; i++)
        {
            const MidiProgramData& mpData(fData->midiprog.data[i]);

            fData->engine->osc_send_bridge_midi_program_info(i, mpData.bank, mpData.program, mpData.name);
        }

        fData->engine->osc_send_bridge_set_midi_program(fData->midiprog.current);
#else
        fData->engine->osc_send_control_set_midi_program_count(fData->id, fData->midiprog.count);

        for (uint32_t i=0; i < fData->midiprog.count; i++)
        {
            const MidiProgramData& mpData(fData->midiprog.data[i]);

            fData->engine->osc_send_control_set_midi_program_data(fData->id, i, mpData.bank, mpData.program, mpData.name);
        }

        fData->engine->osc_send_control_set_midi_program(fData->id, fData->midiprog.current);
#endif
    }

#ifndef BUILD_BRIDGE
    fData->engine->osc_send_control_add_plugin_end(fData->id);

    // Internal Parameters
    {
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_ACTIVE, fData->active ? 1.0 : 0.0);
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_DRYWET, fData->postProc.dryWet);
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_VOLUME, fData->postProc.volume);
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_BALANCE_LEFT, fData->postProc.balanceLeft);
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_BALANCE_RIGHT, fData->postProc.balanceRight);
        fData->engine->osc_send_control_set_parameter_value(fData->id, PARAMETER_PANNING, fData->postProc.panning);
    }
#endif
}

void CarlaPlugin::updateOscData(const lo_address& source, const char* const url)
{
    // FIXME - remove debug prints later
    qWarning("CarlaPlugin::updateOscData(%p, \"%s\")", source, url);

    const char* host;
    const char* port;
    const int proto = lo_address_get_protocol(source);

    fData->osc.data.free();

    host = lo_address_get_hostname(source);
    port = lo_address_get_port(source);
    fData->osc.data.source = lo_address_new_with_proto(proto, host, port);
    qWarning("CarlaPlugin::updateOscData() - source: host \"%s\", port \"%s\"", host, port);

    host = lo_url_get_hostname(url);
    port = lo_url_get_port(url);
    fData->osc.data.path   = lo_url_get_path(url);
    fData->osc.data.target = lo_address_new_with_proto(proto, host, port);
    qWarning("CarlaPlugin::updateOscData() - target: host \"%s\", port \"%s\", path \"%s\"", host, port, fData->osc.data.path);

    free((void*)host);
    free((void*)port);

#ifndef BUILD_BRIDGE
    if (fData->hints & PLUGIN_IS_BRIDGE)
        return;
#endif

    osc_send_sample_rate(&fData->osc.data, fData->engine->getSampleRate());

#if 0
    for (size_t i=0; i < fData->custom.count(); i++)
    {
        // TODO
        //if (m_type == PLUGIN_LV2)
        //osc_send_lv2_transfer_event(&osc.data, getCustomDataTypeString(custom[i].type), /*custom[i].key,*/ custom[i].value);
        //else
        if (custom[i].type == CUSTOM_DATA_STRING)
            osc_send_configure(&osc.data, custom[i].key, custom[i].value);
    }
#endif

    if (fData->prog.current >= 0)
        osc_send_program(&fData->osc.data, fData->prog.current);

    if (fData->midiprog.current >= 0)
    {
        const MidiProgramData& curMidiProg(fData->midiprog.getCurrent());

        if (type() == PLUGIN_DSSI)
            osc_send_program(&fData->osc.data, curMidiProg.bank, curMidiProg.program);
        else
            osc_send_midi_program(&fData->osc.data, curMidiProg.bank, curMidiProg.program);
    }

    for (uint32_t i=0; i < fData->param.count; i++)
        osc_send_control(&fData->osc.data, fData->param.data[i].rindex, getParameterValue(i));

    qWarning("CarlaPlugin::updateOscData() - done");
}

void CarlaPlugin::freeOscData()
{
    fData->osc.data.free();
}

bool CarlaPlugin::waitForOscGuiShow()
{
    qWarning("CarlaPlugin::waitForOscGuiShow()");

    // wait for UI 'update' call
    for (uint i=0, oscUiTimeout = fData->engine->getOptions().oscUiTimeout; i < oscUiTimeout; i++)
    {
        if (fData->osc.data.target)
        {
            qWarning("CarlaPlugin::waitForOscGuiShow() - got response, asking UI to show itself now");
            osc_send_show(&fData->osc.data);
            return true;
        }
        else
            carla_msleep(100);
    }

    qWarning("CarlaPlugin::waitForOscGuiShow() - Timeout while waiting for UI to respond (waited %u msecs)", fData->engine->getOptions().oscUiTimeout);
    return false;
}

// -------------------------------------------------------------------
// MIDI events

void CarlaPlugin::sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note < MAX_MIDI_NOTE);
    CARLA_ASSERT(velo < MAX_MIDI_VALUE);

    if (! fData->active)
        return;

    ExternalMidiNote extNote;
    extNote.channel = channel;
    extNote.note    = note;
    extNote.velo    = velo;

    fData->extNotes.mutex.lock();
    fData->extNotes.append(extNote);
    fData->extNotes.mutex.unlock();

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
            fData->engine->osc_send_control_note_on(fData->id, channel, note, velo);
        else
            fData->engine->osc_send_control_note_off(fData->id, channel, note);
    }
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        fData->engine->callback(velo ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, fData->id, channel, note, velo, nullptr);
}

void CarlaPlugin::sendMidiAllNotesOff()
{
    // TODO
#if 0
    fData->extNotes.mutex.lock();
    fData->postRtEvents.mutex.lock();

    ExternalMidiNote extNote;
    extNote.channel = fData->ctrlInChannel;
    extNote.note    = 0;
    extNote.velo    = 0;

    PluginPostRtEvent postEvent;
    postEvent.type   = kPluginPostRtEventNoteOff;
    postEvent.value1 = fData->ctrlInChannel;
    postEvent.value2 = 0;
    postEvent.value3 = 0.0;

    for (unsigned short i=0; i < MAX_MIDI_NOTE; i++)
    {
        extNote.note     = i;
        postEvent.value2 = i;

        fData->extNotes.append(extNote);
        fData->postRtEvents.appendNonRT(event);
    }

    fData->postRtEvents.mutex.unlock();
    fData->extNotes.mutex.unlock();
#endif
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

    fData->postRtEvents.appendRT(event);
}

void CarlaPlugin::postRtEventsRun()
{
    unsigned short i = 0;
    PluginPostRtEvent listData[MAX_RT_EVENTS];

    // Make a safe copy of events while clearing them
    fData->postRtEvents.mutex.lock();

    while (! fData->postRtEvents.data.isEmpty())
    {
        PluginPostRtEvent& event = fData->postRtEvents.data.getFirst(true);
        listData[i++] = event;
    }

    fData->postRtEvents.mutex.unlock();

    // Handle events now
    for (i=0; i < MAX_RT_EVENTS; i++)
    {
        const PluginPostRtEvent* const event = &listData[i];

        switch (event->type)
        {
        case kPluginPostRtEventNull:
            break;

        case kPluginPostRtEventDebug:
            fData->engine->callback(CALLBACK_DEBUG, fData->id, event->value1, event->value2, event->value3, nullptr);
            break;

        case kPluginPostRtEventParameterChange:
            // Update UI
            if (event->value1 >= 0)
                uiParameterChange(event->value1, event->value3);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (fData->engine->isOscControlRegistered())
                fData->engine->osc_send_control_set_parameter_value(fData->id, event->value1, event->value3);
#endif

            // Update Host
            fData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fData->id, event->value1, 0, event->value3, nullptr);
            break;

        case kPluginPostRtEventProgramChange:
            // Update UI
            if (event->value1 >= 0)
                uiProgramChange(event->value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (fData->engine->isOscControlRegistered())
            {
                fData->engine->osc_send_control_set_program(fData->id, event->value1);

                for (uint32_t j=0; j < fData->param.count; j++)
                    fData->engine->osc_send_control_set_default_value(fData->id, j, fData->param.ranges[j].def);
            }
#endif

            // Update Host
            fData->engine->callback(CALLBACK_PROGRAM_CHANGED, fData->id, event->value1, 0, 0.0, nullptr);
            break;

        case kPluginPostRtEventMidiProgramChange:
            // Update UI
            if (event->value1 >= 0)
                uiMidiProgramChange(event->value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (fData->engine->isOscControlRegistered())
            {
                fData->engine->osc_send_control_set_midi_program(fData->id, event->value1);

                for (uint32_t j=0; j < fData->param.count; j++)
                    fData->engine->osc_send_control_set_default_value(fData->id, j, fData->param.ranges[j].def);
            }
#endif

            // Update Host
            fData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fData->id, event->value1, 0, 0.0, nullptr);
            break;

        case kPluginPostRtEventNoteOn:
            // Update UI
            uiNoteOn(event->value1, event->value2, int(event->value3));

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (fData->engine->isOscControlRegistered())
                fData->engine->osc_send_control_note_on(fData->id, event->value1, event->value2, int(event->value3));
#endif

            // Update Host
            fData->engine->callback(CALLBACK_NOTE_ON, fData->id, event->value1, event->value2, int(event->value3), nullptr);
            break;

        case kPluginPostRtEventNoteOff:
            // Update UI
            uiNoteOff(event->value1, event->value2);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (fData->engine->isOscControlRegistered())
                fData->engine->osc_send_control_note_off(fData->id, event->value1, event->value2);
#endif

            // Update Host
            fData->engine->callback(CALLBACK_NOTE_OFF, fData->id, event->value1, event->value2, 0.0, nullptr);
            break;
        }
    }
}

void CarlaPlugin::uiParameterChange(const uint32_t index, const double value)
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

void CarlaPlugin::removeClientPorts()
{
    qDebug("CarlaPlugin::removeClientPorts() - start");

    fData->audioIn.freePorts();
    fData->audioOut.freePorts();
    fData->event.freePorts();

    qDebug("CarlaPlugin::removeClientPorts() - end");
}

void CarlaPlugin::initBuffers()
{
    fData->audioIn.initBuffers(fData->engine);
    fData->audioOut.initBuffers(fData->engine);
    fData->event.initBuffers(fData->engine);
}

void CarlaPlugin::deleteBuffers()
{
    qDebug("CarlaPlugin::deleteBuffers() - start");

    fData->audioIn.clear();
    fData->audioOut.clear();
    fData->param.clear();
    fData->event.clear();

    qDebug("CarlaPlugin::deleteBuffers() - end");
}

// -------------------------------------------------------------------
// Library functions

bool CarlaPlugin::libOpen(const char* const filename)
{
    fData->lib = lib_open(filename);
    return bool(fData->lib);
}

bool CarlaPlugin::libClose()
{
    const bool ret = lib_close(fData->lib);
    fData->lib = nullptr;
    return ret;
}

void* CarlaPlugin::libSymbol(const char* const symbol)
{
    return lib_symbol(fData->lib, symbol);
}

const char* CarlaPlugin::libError(const char* const filename)
{
    return lib_error(filename);
}

// -------------------------------------------------------------------
// Engine helpers

float* CarlaPlugin::getAudioInPortBuffer(uint32_t index)
{
    CARLA_ASSERT(fData->audioIn.ports[index].port);

    return fData->audioIn.ports[index].port->getBuffer();
}

float* CarlaPlugin::getAudioOutPortBuffer(uint32_t index)
{
    CARLA_ASSERT(fData->audioOut.ports[index].port);

    return fData->audioOut.ports[index].port->getBuffer();
}

CarlaEngine* CarlaPlugin::getEngine() const
{
    return fData->engine;
}

// -------------------------------------------------------------------
// Scoped Disabler

CarlaPlugin::ScopedDisabler::ScopedDisabler(CarlaPlugin* const plugin)
    : kPlugin(plugin)
{
    if (plugin->fData->enabled)
    {
        plugin->fData->enabled = false;
        plugin->fData->engine->waitForProccessEnd();
    }
}

CarlaPlugin::ScopedDisabler::~ScopedDisabler()
{
    kPlugin->fData->enabled = true;
}

// -------------------------------------------------------------------
// CarlaPluginGUI

CarlaPluginGUI::CarlaPluginGUI(QWidget* const parent)
    : QMainWindow(parent)
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
