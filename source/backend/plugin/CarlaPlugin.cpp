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

#include "CarlaBackendUtils.hpp"
#include "CarlaBase64Utils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPluginUI.hpp"

#include <ctime>

#include "juce_core.h"

using juce::CharPointer_UTF8;
using juce::File;
using juce::MemoryOutputStream;
using juce::ScopedPointer;
using juce::String;
using juce::XmlDocument;
using juce::XmlElement;

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------
// Fallback data

static const ParameterData   kParameterDataNull   = { PARAMETER_UNKNOWN, 0x0, PARAMETER_NULL, -1, -1, 0 };
static const ParameterRanges kParameterRangesNull = { 0.0f, 0.0f, 1.0f, 0.01f, 0.0001f, 0.1f };
static const MidiProgramData kMidiProgramDataNull = { 0, 0, nullptr };
static const CustomData      kCustomDataNull      = { nullptr, nullptr, nullptr };

static const CustomData        kCustomDataFallback        = { nullptr, nullptr, nullptr };
static /* */ CustomData        kCustomDataFallbackNC      = { nullptr, nullptr, nullptr };
static const PluginPostRtEvent kPluginPostRtEventFallback = { kPluginPostRtEventNull, 0, 0, 0.0f };

// -------------------------------------------------------------------
// ParamSymbol struct, needed for CarlaPlugin::loadStateSave()

struct ParamSymbol {
    int32_t index;
    const char* symbol;

    ParamSymbol(const uint32_t i, const char* const s)
        : index(static_cast<int32_t>(i)),
          symbol(carla_strdup(s)) {}

    ~ParamSymbol() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(symbol != nullptr,)

        delete[] symbol;
        symbol = nullptr;
    }

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ParamSymbol() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ParamSymbol)
#endif
};

// -------------------------------------------------------------------
// Constructor and destructor

CarlaPlugin::CarlaPlugin(CarlaEngine* const engine, const uint id)
    : pData(new ProtectedData(engine, id))
{
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr,);
    CARLA_SAFE_ASSERT(id < engine->getMaxPluginNumber());
    carla_debug("CarlaPlugin::CarlaPlugin(%p, %i)", engine, id);

    switch (engine->getProccessMode())
    {
    case ENGINE_PROCESS_MODE_SINGLE_CLIENT:
    case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
        CARLA_SAFE_ASSERT(id < MAX_DEFAULT_PLUGINS);
        break;

    case ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
        CARLA_SAFE_ASSERT(id < MAX_RACK_PLUGINS);
        break;

    case ENGINE_PROCESS_MODE_PATCHBAY:
        CARLA_SAFE_ASSERT(id < MAX_PATCHBAY_PLUGINS);
        break;

    case ENGINE_PROCESS_MODE_BRIDGE:
        CARLA_SAFE_ASSERT(id == 0);
        break;
    }
}

CarlaPlugin::~CarlaPlugin()
{
    carla_debug("CarlaPlugin::~CarlaPlugin()");

    delete pData;
}

// -------------------------------------------------------------------
// Information (base)

uint CarlaPlugin::getId() const noexcept
{
    return pData->id;
}

uint CarlaPlugin::getHints() const noexcept
{
    return pData->hints;
}

uint CarlaPlugin::getOptionsEnabled() const noexcept
{
    return pData->options;
}

bool CarlaPlugin::isEnabled() const noexcept
{
    return pData->enabled;
}

const char* CarlaPlugin::getName() const noexcept
{
    return pData->name;
}

const char* CarlaPlugin::getFilename() const noexcept
{
    return pData->filename;
}

const char* CarlaPlugin::getIconName() const noexcept
{
    return pData->iconName;
}

PluginCategory CarlaPlugin::getCategory() const noexcept
{
    return getPluginCategoryFromName(pData->name);
}

int64_t CarlaPlugin::getUniqueId() const noexcept
{
    return 0;
}

uint32_t CarlaPlugin::getLatencyInFrames() const noexcept
{
    return 0;
}

// -------------------------------------------------------------------
// Information (count)

uint32_t CarlaPlugin::getAudioInCount() const noexcept
{
    return pData->audioIn.count;
}

uint32_t CarlaPlugin::getAudioOutCount() const noexcept
{
    return pData->audioOut.count;
}

uint32_t CarlaPlugin::getCVInCount() const noexcept
{
    return pData->cvIn.count;
}

uint32_t CarlaPlugin::getCVOutCount() const noexcept
{
    return pData->cvOut.count;
}

uint32_t CarlaPlugin::getMidiInCount() const noexcept
{
    return (pData->extraHints & PLUGIN_EXTRA_HINT_HAS_MIDI_IN) ? 1 : 0;
}

uint32_t CarlaPlugin::getMidiOutCount() const noexcept
{
    return (pData->extraHints & PLUGIN_EXTRA_HINT_HAS_MIDI_OUT) ? 1 : 0;
}

uint32_t CarlaPlugin::getParameterCount() const noexcept
{
    return pData->param.count;
}

uint32_t CarlaPlugin::getParameterScalePointCount(const uint32_t parameterId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);
    return 0;
}

uint32_t CarlaPlugin::getProgramCount() const noexcept
{
    return pData->prog.count;
}

uint32_t CarlaPlugin::getMidiProgramCount() const noexcept
{
    return pData->midiprog.count;
}

uint32_t CarlaPlugin::getCustomDataCount() const noexcept
{
    return static_cast<uint32_t>(pData->custom.count());
}

// -------------------------------------------------------------------
// Information (current data)

int32_t CarlaPlugin::getCurrentProgram() const noexcept
{
    return pData->prog.current;
}

int32_t CarlaPlugin::getCurrentMidiProgram() const noexcept
{
    return pData->midiprog.current;
}

const ParameterData& CarlaPlugin::getParameterData(const uint32_t parameterId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, kParameterDataNull);
    return pData->param.data[parameterId];
}

const ParameterRanges& CarlaPlugin::getParameterRanges(const uint32_t parameterId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, kParameterRangesNull);
    return pData->param.ranges[parameterId];
}

bool CarlaPlugin::isParameterOutput(const uint32_t parameterId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);
    return (pData->param.data[parameterId].type == PARAMETER_OUTPUT);
}

const MidiProgramData& CarlaPlugin::getMidiProgramData(const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count, kMidiProgramDataNull);
    return pData->midiprog.data[index];
}

const CustomData& CarlaPlugin::getCustomData(const uint32_t index) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->custom.count(), kCustomDataNull);
    return pData->custom.getAt(index, kCustomDataFallback);
}

std::size_t CarlaPlugin::getChunkData(void** const dataPtr) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);
    CARLA_SAFE_ASSERT(false); // this should never happen
    return 0;
}

// -------------------------------------------------------------------
// Information (per-plugin data)

uint CarlaPlugin::getOptionsAvailable() const noexcept
{
    CARLA_SAFE_ASSERT(false); // this should never happen
    return 0x0;
}

float CarlaPlugin::getParameterValue(const uint32_t parameterId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), 0.0f);
    CARLA_SAFE_ASSERT(false); // this should never happen
    return 0.0f;
}

float CarlaPlugin::getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), 0.0f);
    CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId), 0.0f);
    CARLA_SAFE_ASSERT(false); // this should never happen
    return 0.0f;
}

void CarlaPlugin::getLabel(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
}

void CarlaPlugin::getMaker(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
}

void CarlaPlugin::getCopyright(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
}

void CarlaPlugin::getRealName(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
}

void CarlaPlugin::getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(),);
    CARLA_SAFE_ASSERT(false); // this should never happen
    strBuf[0] = '\0';
}

void CarlaPlugin::getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(),);
    strBuf[0] = '\0';
}

void CarlaPlugin::getParameterText(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(),);
    CARLA_SAFE_ASSERT(false); // this should never happen
    strBuf[0] = '\0';
}

void CarlaPlugin::getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(),);
    strBuf[0] = '\0';
}

void CarlaPlugin::getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(),);
    CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId),);
    CARLA_SAFE_ASSERT(false); // this should never happen
    strBuf[0] = '\0';
}

float CarlaPlugin::getInternalParameterValue(const int32_t parameterId) const noexcept
{
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(parameterId != PARAMETER_NULL && parameterId > PARAMETER_MAX, 0.0f);

    switch (parameterId)
    {
    case PARAMETER_ACTIVE:
        return pData->active;
    case PARAMETER_CTRL_CHANNEL:
        return pData->ctrlChannel;
    case PARAMETER_DRYWET:
        return pData->postProc.dryWet;
    case PARAMETER_VOLUME:
        return pData->postProc.volume;
    case PARAMETER_BALANCE_LEFT:
        return pData->postProc.balanceLeft;
    case PARAMETER_BALANCE_RIGHT:
        return pData->postProc.balanceRight;
    case PARAMETER_PANNING:
        return pData->postProc.panning;
    };
#endif
    CARLA_SAFE_ASSERT_RETURN(parameterId >= 0, 0.0f);

    return getParameterValue(static_cast<uint32_t>(parameterId));
}

void CarlaPlugin::getProgramName(const uint32_t index, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->prog.count,);
    CARLA_SAFE_ASSERT_RETURN(pData->prog.names[index] != nullptr,);
    std::strncpy(strBuf, pData->prog.names[index], STR_MAX);
}

void CarlaPlugin::getMidiProgramName(const uint32_t index, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);
    CARLA_SAFE_ASSERT_RETURN(pData->midiprog.data[index].name != nullptr,);
    std::strncpy(strBuf, pData->midiprog.data[index].name, STR_MAX);
}

void CarlaPlugin::getParameterCountInfo(uint32_t& ins, uint32_t& outs) const noexcept
{
    ins  = 0;
    outs = 0;

    for (uint32_t i=0; i < pData->param.count; ++i)
    {
        if (pData->param.data[i].type == PARAMETER_INPUT)
            ++ins;
        else if (pData->param.data[i].type == PARAMETER_OUTPUT)
            ++outs;
    }
}

// -------------------------------------------------------------------
// Set data (state)

void CarlaPlugin::prepareForSave()
{
}

void CarlaPlugin::resetParameters() noexcept
{
    for (uint i=0; i < pData->param.count; ++i)
    {
        const ParameterData&   paramData(pData->param.data[i]);
        const ParameterRanges& paramRanges(pData->param.ranges[i]);

        if (paramData.type != PARAMETER_INPUT)
            continue;
        if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
            continue;

        setParameterValue(i, paramRanges.def, true, true, true);
    }
}

void CarlaPlugin::randomizeParameters() noexcept
{
    float value, random;

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    std::srand(static_cast<uint>(std::time(nullptr)));

    for (uint i=0; i < pData->param.count; ++i)
    {
        const ParameterData& paramData(pData->param.data[i]);

        if (paramData.type != PARAMETER_INPUT)
            continue;
        if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
            continue;

        getParameterName(i, strBuf);

        if (std::strstr(strBuf, "olume") != nullptr)
            continue;
        if (std::strstr(strBuf, "Master") != nullptr)
            continue;

        const ParameterRanges& paramRanges(pData->param.ranges[i]);

        if (paramData.hints & PARAMETER_IS_BOOLEAN)
        {
            random = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            value  = random > 0.5 ? paramRanges.max : paramRanges.min;
        }
        else
        {
            random = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            value  = random * (paramRanges.max - paramRanges.min) + paramRanges.min;

            if (paramData.hints & PARAMETER_IS_INTEGER)
                value = std::rint(value);
        }

        setParameterValue(i, value, true, true, true);
    }
}

const CarlaStateSave& CarlaPlugin::getStateSave(const bool callPrepareForSave)
{
    if (callPrepareForSave)
        prepareForSave();

    pData->stateSave.clear();

    const PluginType pluginType(getType());

    char strBuf[STR_MAX+1];

    // ---------------------------------------------------------------
    // Basic info

    getLabel(strBuf);

    pData->stateSave.type     = carla_strdup(getPluginTypeAsString(getType()));
    pData->stateSave.name     = carla_strdup(pData->name);
    pData->stateSave.label    = carla_strdup(strBuf);
    pData->stateSave.uniqueId = getUniqueId();
#ifndef BUILD_BRIDGE
    pData->stateSave.options  = pData->options;
#endif

    if (pData->filename != nullptr)
        pData->stateSave.binary = carla_strdup(pData->filename);

#ifndef BUILD_BRIDGE
    // ---------------------------------------------------------------
    // Internals

    pData->stateSave.active       = pData->active;
    pData->stateSave.dryWet       = pData->postProc.dryWet;
    pData->stateSave.volume       = pData->postProc.volume;
    pData->stateSave.balanceLeft  = pData->postProc.balanceLeft;
    pData->stateSave.balanceRight = pData->postProc.balanceRight;
    pData->stateSave.panning      = pData->postProc.panning;
    pData->stateSave.ctrlChannel  = pData->ctrlChannel;
#endif

    bool usingChunk = false;

    // ---------------------------------------------------------------
    // Chunk

    if (pData->options & PLUGIN_OPTION_USE_CHUNKS)
    {
        void* data = nullptr;
        const std::size_t dataSize(getChunkData(&data));

        if (data != nullptr && dataSize > 0)
        {
            pData->stateSave.chunk = CarlaString::asBase64(data, dataSize).dup();

            if (pluginType != PLUGIN_INTERNAL)
                usingChunk = true;
        }
    }

    // ---------------------------------------------------------------
    // Current Program

    if (pData->prog.current >= 0 && pluginType != PLUGIN_LV2 && pluginType != PLUGIN_GIG)
    {
        pData->stateSave.currentProgramIndex = pData->prog.current;
        pData->stateSave.currentProgramName  = carla_strdup(pData->prog.names[pData->prog.current]);
    }

    // ---------------------------------------------------------------
    // Current MIDI Program

    if (pData->midiprog.current >= 0 && pluginType != PLUGIN_LV2 && pluginType != PLUGIN_SF2)
    {
        const MidiProgramData& mpData(pData->midiprog.getCurrent());

        pData->stateSave.currentMidiBank    = static_cast<int32_t>(mpData.bank);
        pData->stateSave.currentMidiProgram = static_cast<int32_t>(mpData.program);
    }

    // ---------------------------------------------------------------
    // Parameters

    const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));

    for (uint32_t i=0; i < pData->param.count; ++i)
    {
        const ParameterData& paramData(pData->param.data[i]);

        if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
            continue;

        const bool dummy = paramData.type != PARAMETER_INPUT || usingChunk;

        if (dummy && paramData.midiCC <= -1)
            continue;

        CarlaStateSave::Parameter* const stateParameter(new CarlaStateSave::Parameter());

        stateParameter->dummy = dummy;
        stateParameter->index = paramData.index;
#ifndef BUILD_BRIDGE
        stateParameter->midiCC      = paramData.midiCC;
        stateParameter->midiChannel = paramData.midiChannel;
#endif

        getParameterName(i, strBuf);
        stateParameter->name = carla_strdup(strBuf);

        getParameterSymbol(i, strBuf);
        stateParameter->symbol = carla_strdup(strBuf);;

        if (! dummy)
        {
            stateParameter->value = getParameterValue(i);

            if (paramData.hints & PARAMETER_USES_SAMPLERATE)
                stateParameter->value /= sampleRate;
        }

        pData->stateSave.parameters.append(stateParameter);
    }

    // ---------------------------------------------------------------
    // Custom Data

    for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
    {
        const CustomData& cData(it.getValue(kCustomDataFallback));
        CARLA_SAFE_ASSERT_CONTINUE(cData.isValid());

        CarlaStateSave::CustomData* stateCustomData(new CarlaStateSave::CustomData());

        stateCustomData->type  = carla_strdup(cData.type);
        stateCustomData->key   = carla_strdup(cData.key);
        stateCustomData->value = carla_strdup(cData.value);

        pData->stateSave.customData.append(stateCustomData);
    }

    return pData->stateSave;
}

void CarlaPlugin::loadStateSave(const CarlaStateSave& stateSave)
{
    char strBuf[STR_MAX+1];
    const bool usesMultiProgs(pData->hints & PLUGIN_USES_MULTI_PROGS);

    const PluginType pluginType(getType());

    // ---------------------------------------------------------------
    // Part 1 - PRE-set custom data (only those which reload programs)

    for (CarlaStateSave::CustomDataItenerator it = stateSave.customData.begin2(); it.valid(); it.next())
    {
        const CarlaStateSave::CustomData* const stateCustomData(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData != nullptr);
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->isValid());

        const char* const key(stateCustomData->key);

        /**/ if (pluginType == PLUGIN_DSSI && (std::strcmp (key, "reloadprograms") == 0 ||
                                               std::strcmp (key, "load"          ) == 0 ||
                                               std::strncmp(key, "patches",     7) == 0 ))
            pass();
        else if (usesMultiProgs && std::strcmp(key, "midiPrograms") == 0)
            pass();
        else
            continue;

        setCustomData(stateCustomData->type, key, stateCustomData->value, true);
    }

    // ---------------------------------------------------------------
    // Part 2 - set program

    if (stateSave.currentProgramIndex >= 0 && stateSave.currentProgramName != nullptr)
    {
        int32_t programId = -1;

        // index < count
        if (stateSave.currentProgramIndex < static_cast<int32_t>(pData->prog.count))
        {
            programId = stateSave.currentProgramIndex;
        }
        // index not valid, try to find by name
        else
        {
            for (uint32_t i=0; i < pData->prog.count; ++i)
            {
                strBuf[0] = '\0';
                getProgramName(i, strBuf);

                if (strBuf[0] != '\0' && std::strcmp(stateSave.currentProgramName, strBuf) == 0)
                {
                    programId = static_cast<int32_t>(i);
                    break;
                }
            }
        }

        // set program now, if valid
        if (programId >= 0)
            setProgram(programId, true, true, true);
    }

    // ---------------------------------------------------------------
    // Part 3 - set midi program

    if (stateSave.currentMidiBank >= 0 && stateSave.currentMidiProgram >= 0 && ! usesMultiProgs)
        setMidiProgramById(static_cast<uint32_t>(stateSave.currentMidiBank), static_cast<uint32_t>(stateSave.currentMidiProgram), true, true, true);

    // ---------------------------------------------------------------
    // Part 4a - get plugin parameter symbols

    LinkedList<ParamSymbol*> paramSymbols;

    if (pluginType == PLUGIN_LADSPA || pluginType == PLUGIN_LV2)
    {
        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            strBuf[0] = '\0';
            getParameterSymbol(i, strBuf);

            if (strBuf[0] != '\0')
            {
                ParamSymbol* const paramSymbol(new ParamSymbol(i, strBuf));
                paramSymbols.append(paramSymbol);
            }
        }
    }

    // ---------------------------------------------------------------
    // Part 4b - set parameter values (carefully)

    const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));

    for (CarlaStateSave::ParameterItenerator it = stateSave.parameters.begin2(); it.valid(); it.next())
    {
        CarlaStateSave::Parameter* const stateParameter(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(stateParameter != nullptr);

        int32_t index = -1;

        if (pluginType == PLUGIN_LADSPA)
        {
            // Try to set by symbol, otherwise use index
            if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            {
                for (LinkedList<ParamSymbol*>::Itenerator it2 = paramSymbols.begin2(); it2.valid(); it2.next())
                {
                    ParamSymbol* const paramSymbol(it2.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(paramSymbol != nullptr);
                    CARLA_SAFE_ASSERT_CONTINUE(paramSymbol->symbol != nullptr);

                    if (std::strcmp(stateParameter->symbol, paramSymbol->symbol) == 0)
                    {
                        index = paramSymbol->index;
                        break;
                    }
                }
                if (index == -1)
                    index = stateParameter->index;
            }
            else
                index = stateParameter->index;
        }
        else if (pluginType == PLUGIN_LV2)
        {
            // Symbol only
            if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            {
                for (LinkedList<ParamSymbol*>::Itenerator it2 = paramSymbols.begin2(); it2.valid(); it2.next())
                {
                    ParamSymbol* const paramSymbol(it2.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(paramSymbol != nullptr);
                    CARLA_SAFE_ASSERT_CONTINUE(paramSymbol->symbol != nullptr);

                    if (std::strcmp(stateParameter->symbol, paramSymbol->symbol) == 0)
                    {
                        index = paramSymbol->index;
                        break;
                    }
                }
                if (index == -1)
                    carla_stderr("Failed to find LV2 parameter symbol '%s')", stateParameter->symbol);
            }
            else
                carla_stderr("LV2 Plugin parameter '%s' has no symbol", stateParameter->name);
        }
        else
        {
            // Index only
            index = stateParameter->index;
        }

        // Now set parameter
        if (index >= 0 && index < static_cast<int32_t>(pData->param.count))
        {
            //CARLA_SAFE_ASSERT(stateParameter->isInput == (pData

            if (! stateParameter->dummy)
            {
                if (pData->param.data[index].hints & PARAMETER_USES_SAMPLERATE)
                    stateParameter->value *= sampleRate;

                setParameterValue(static_cast<uint32_t>(index), stateParameter->value, true, true, true);
            }

#ifndef BUILD_BRIDGE
            setParameterMidiCC(static_cast<uint32_t>(index), stateParameter->midiCC, true, true);
            setParameterMidiChannel(static_cast<uint32_t>(index), stateParameter->midiChannel, true, true);
#endif
        }
        else
            carla_stderr("Could not set parameter data for '%s'", stateParameter->name);
    }

    // ---------------------------------------------------------------
    // Part 4c - clear

    for (LinkedList<ParamSymbol*>::Itenerator it = paramSymbols.begin2(); it.valid(); it.next())
    {
        ParamSymbol* const paramSymbol(it.getValue(nullptr));
        delete paramSymbol;
    }

    paramSymbols.clear();

    // ---------------------------------------------------------------
    // Part 5 - set custom data

    for (CarlaStateSave::CustomDataItenerator it = stateSave.customData.begin2(); it.valid(); it.next())
    {
        const CarlaStateSave::CustomData* const stateCustomData(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData != nullptr);
        CARLA_SAFE_ASSERT_CONTINUE(stateCustomData->isValid());

        const char* const key(stateCustomData->key);

        if (pluginType == PLUGIN_DSSI && (std::strcmp (key, "reloadprograms") == 0 ||
                                          std::strcmp (key, "load"          ) == 0 ||
                                          std::strncmp(key, "patches",     7) == 0 ))
            continue;
        if (usesMultiProgs && std::strcmp(key, "midiPrograms") == 0)
            continue;

        setCustomData(stateCustomData->type, key, stateCustomData->value, true);
    }

    // ---------------------------------------------------------------
    // Part 5x - set lv2 state

    if (pluginType == PLUGIN_LV2 && pData->custom.count() > 0)
        setCustomData(CUSTOM_DATA_TYPE_STRING, "CarlaLoadLv2StateNow", "true", true);

    // ---------------------------------------------------------------
    // Part 6 - set chunk

    if (stateSave.chunk != nullptr && (pData->options & PLUGIN_OPTION_USE_CHUNKS) != 0)
    {
        std::vector<uint8_t> chunk(carla_getChunkFromBase64String(stateSave.chunk));
        setChunkData(chunk.data(), chunk.size());
    }

#ifndef BUILD_BRIDGE
    // ---------------------------------------------------------------
    // Part 6 - set internal stuff

    const uint availOptions(getOptionsAvailable());

    for (uint i=0; i<10; ++i) // FIXME - get this value somehow...
    {
        const uint option(1u << i);

        if (availOptions & option)
            setOption(option, (stateSave.options & option) != 0, true);
    }

    setDryWet(stateSave.dryWet, true, true);
    setVolume(stateSave.volume, true, true);
    setBalanceLeft(stateSave.balanceLeft, true, true);
    setBalanceRight(stateSave.balanceRight, true, true);
    setPanning(stateSave.panning, true, true);
    setCtrlChannel(stateSave.ctrlChannel, true, true);
    setActive(stateSave.active, true, true);
#endif

    pData->engine->callback(ENGINE_CALLBACK_UPDATE, pData->id, 0, 0, 0.0f, nullptr);
}

bool CarlaPlugin::saveStateToFile(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("CarlaPlugin::saveStateToFile(\"%s\")", filename);

    MemoryOutputStream out, streamState;
    getStateSave().dumpToMemoryStream(streamState);

    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PRESET>\n";
    out << "<CARLA-PRESET VERSION='2.0'>\n";
    out << streamState;
    out << "</CARLA-PRESET>\n";

    const String jfilename = String(CharPointer_UTF8(filename));
    File file(jfilename);

    if (file.replaceWithData(out.getData(), out.getDataSize()))
        return true;

    pData->engine->setLastError("Failed to write file");
    return false;
}

bool CarlaPlugin::loadStateFromFile(const char* const filename)
{
    // TODO set errors

    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("CarlaPlugin::loadStateFromFile(\"%s\")", filename);

    const String jfilename = String(CharPointer_UTF8(filename));
    File file(jfilename);
    CARLA_SAFE_ASSERT_RETURN(file.existsAsFile(), false);

    XmlDocument xml(file);
    ScopedPointer<XmlElement> xmlElement(xml.getDocumentElement(true));
    CARLA_SAFE_ASSERT_RETURN(xmlElement != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(xmlElement->getTagName().equalsIgnoreCase("carla-preset"), false);

    // completely load file
    xmlElement = xml.getDocumentElement(false);
    CARLA_SAFE_ASSERT_RETURN(xmlElement != nullptr, false);

    if (pData->stateSave.fillFromXmlElement(xmlElement))
    {
        loadStateSave(pData->stateSave);
        return true;
    }

    return false;
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setId(const uint newId) noexcept
{
    pData->id = newId;
}

void CarlaPlugin::setName(const char* const newName)
{
    CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0',);

    if (pData->name != nullptr)
        delete[] pData->name;

    pData->name = carla_strdup(newName);
}

void CarlaPlugin::setOption(const uint option, const bool yesNo, const bool sendCallback)
{
    CARLA_SAFE_ASSERT_RETURN(getOptionsAvailable() & option,);

    if (yesNo)
        pData->options |= option;
    else
        pData->options &= ~option;

#ifndef BUILD_BRIDGE
    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_OPTION_CHANGED, pData->id, static_cast<int>(option), yesNo ? 1 : 0, 0.0f, nullptr);
#else
    // unused
    return; (void)sendCallback;
#endif
}

void CarlaPlugin::setEnabled(const bool yesNo) noexcept
{
    if (pData->enabled == yesNo)
        return;

    pData->masterMutex.lock();
    pData->enabled = yesNo;
    pData->masterMutex.unlock();
}

void CarlaPlugin::setActive(const bool active, const bool sendOsc, const bool sendCallback) noexcept
{
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
#endif

    if (pData->active == active)
        return;

    {
        const ScopedSingleProcessLocker spl(this, true);

        if (active)
            activate();
        else
            deactivate();
    }

    pData->active = active;

#ifndef BUILD_BRIDGE
    const float value(active ? 1.0f : 0.0f);

# ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_ACTIVE, value);
# endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_ACTIVE, 0, value, nullptr);
#endif

    // may be unused
    return; (void)sendOsc; (void)sendCallback;
}

#ifndef BUILD_BRIDGE
void CarlaPlugin::setDryWet(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(0.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.dryWet, fixedValue))
        return;

    pData->postProc.dryWet = fixedValue;

#ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_DRYWET, fixedValue);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_DRYWET, 0, fixedValue, nullptr);

    // may be unused
    return; (void)sendOsc;
}

void CarlaPlugin::setVolume(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.27f);

    const float fixedValue(carla_fixedValue<float>(0.0f, 1.27f, value));

    if (carla_isEqual(pData->postProc.volume, fixedValue))
        return;

    pData->postProc.volume = fixedValue;

#ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_VOLUME, fixedValue);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_VOLUME, 0, fixedValue, nullptr);

    // may be unused
    return; (void)sendOsc;
}

void CarlaPlugin::setBalanceLeft(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.balanceLeft, fixedValue))
        return;

    pData->postProc.balanceLeft = fixedValue;

#ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_BALANCE_LEFT, fixedValue);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_BALANCE_LEFT, 0, fixedValue, nullptr);

    // may be unused
    return; (void)sendOsc;
}

void CarlaPlugin::setBalanceRight(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.balanceRight, fixedValue))
        return;

    pData->postProc.balanceRight = fixedValue;

#ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_BALANCE_RIGHT, fixedValue);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_BALANCE_RIGHT, 0, fixedValue, nullptr);

    // may be unused
    return; (void)sendOsc;
}

void CarlaPlugin::setPanning(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.panning, fixedValue))
        return;

    pData->postProc.panning = fixedValue;

#ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_PANNING, fixedValue);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_PANNING, 0, fixedValue, nullptr);

    // may be unused
    return; (void)sendOsc;
}
#endif // ! BUILD_BRIDGE

void CarlaPlugin::setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept
{
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
#endif
    CARLA_SAFE_ASSERT_RETURN(channel >= -1 && channel < MAX_MIDI_CHANNELS,);

    if (pData->ctrlChannel == channel)
        return;

    pData->ctrlChannel = channel;

#ifndef BUILD_BRIDGE
    const float channelf(channel);

# ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_CTRL_CHANNEL, channelf);
# endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_CTRL_CHANNEL, 0, channelf, nullptr);
#endif

    // may be unused
    return; (void)sendOsc; (void)sendCallback;
}

// -------------------------------------------------------------------
// Set data (plugin-specific stuff)

void CarlaPlugin::setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

    if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
        uiParameterChange(parameterId, value);

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(parameterId), value);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(parameterId), 0, value, nullptr);

    // may be unused
    return; (void)sendOsc;
}

void CarlaPlugin::setParameterValueByRealIndex(const int32_t rindex, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(rindex > PARAMETER_MAX && rindex != PARAMETER_NULL,);

    switch (rindex)
    {
    case PARAMETER_ACTIVE:
        return setActive((value > 0.0f), sendOsc, sendCallback);
    case PARAMETER_CTRL_CHANNEL:
        return setCtrlChannel(int8_t(value), sendOsc, sendCallback);
    case PARAMETER_DRYWET:
        return setDryWet(value, sendOsc, sendCallback);
    case PARAMETER_VOLUME:
        return setVolume(value, sendOsc, sendCallback);
    case PARAMETER_BALANCE_LEFT:
        return setBalanceLeft(value, sendOsc, sendCallback);
    case PARAMETER_BALANCE_RIGHT:
        return setBalanceRight(value, sendOsc, sendCallback);
    case PARAMETER_PANNING:
        return setPanning(value, sendOsc, sendCallback);
    }
#endif
    CARLA_SAFE_ASSERT_RETURN(rindex >= 0,);

    for (uint32_t i=0; i < pData->param.count; ++i)
    {
        if (pData->param.data[i].rindex == rindex)
        {
            //if (carla_isNotEqual(getParameterValue(i), value))
            setParameterValue(i, value, sendGui, sendOsc, sendCallback);
            break;
        }
    }
}

void CarlaPlugin::setParameterMidiChannel(const uint32_t parameterId, const uint8_t channel, const bool sendOsc, const bool sendCallback) noexcept
{
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
#endif
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

    pData->param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
# ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_midi_channel(pData->id, parameterId, channel);
# endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, pData->id, static_cast<int>(parameterId), channel, 0.0f, nullptr);
#endif

    // may be unused
    return; (void)sendOsc; (void)sendCallback;
}

void CarlaPlugin::setParameterMidiCC(const uint32_t parameterId, const int16_t cc, const bool sendOsc, const bool sendCallback) noexcept
{
#ifndef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
#endif
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < MAX_MIDI_CONTROL,);

    pData->param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
# ifdef HAVE_LIBLO
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_midi_cc(pData->id, parameterId, cc);
# endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED, pData->id, static_cast<int>(parameterId), cc, 0.0f, nullptr);
#endif

    // may be unused
    return; (void)sendOsc; (void)sendCallback;
}

void CarlaPlugin::setCustomData(const char* const type, const char* const key, const char* const value, const bool)
{
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

    // Ignore some keys
    if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) == 0)
    {
        if (std::strncmp(key, "OSC:", 4) == 0 || std::strncmp(key, "CarlaAlternateFile", 18) == 0 || std::strcmp(key, "guiVisible") == 0)
            return;
    }

    // Check if we already have this key
    for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
    {
        CustomData& customData(it.getValue(kCustomDataFallbackNC));
        CARLA_SAFE_ASSERT_CONTINUE(customData.isValid());

        if (std::strcmp(customData.key, key) == 0)
        {
            if (customData.value != nullptr)
                delete[] customData.value;

            customData.value = carla_strdup(value);
            return;
        }
    }

    // Otherwise store it
    CustomData customData;
    customData.type  = carla_strdup(type);
    customData.key   = carla_strdup(key);
    customData.value = carla_strdup(value);
    pData->custom.append(customData);
}

void CarlaPlugin::setChunkData(const void* const data, const std::size_t dataSize)
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);
    CARLA_SAFE_ASSERT(false); // this should never happen
}

void CarlaPlugin::setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);

    pData->prog.current = index;

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    const bool reallySendOsc(sendOsc && pData->engine->isOscControlRegistered());

    if (reallySendOsc)
        pData->engine->oscSend_control_set_current_program(pData->id, index);
#else
    const bool reallySendOsc(false);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PROGRAM_CHANGED, pData->id, index, 0, 0.0f, nullptr);

    // Change default parameter values
    if (index >= 0)
    {
        if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
            uiProgramChange(static_cast<uint32_t>(index));

        if (getType() == PLUGIN_GIG || getType() == PLUGIN_SF2 || getType() == PLUGIN_SFZ)
            return;

        pData->updateParameterValues(this, reallySendOsc, sendCallback, true);
    }

    // may be unused
    return; (void)sendGui; (void)sendOsc;
}

void CarlaPlugin::setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

    pData->midiprog.current = index;

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    const bool reallySendOsc(sendOsc && pData->engine->isOscControlRegistered());

    if (reallySendOsc)
        pData->engine->oscSend_control_set_current_midi_program(pData->id, index);
#else
    const bool reallySendOsc(false);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED, pData->id, index, 0, 0.0f, nullptr);

    if (index >= 0)
    {
        if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
            uiMidiProgramChange(static_cast<uint32_t>(index));

        if (getType() == PLUGIN_GIG || getType() == PLUGIN_SF2 || getType() == PLUGIN_SFZ)
            return;

        pData->updateParameterValues(this, reallySendOsc, sendCallback, true);
    }

    // may be unused
    return; (void)sendGui; (void)sendOsc;
}

void CarlaPlugin::setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    for (uint32_t i=0; i < pData->midiprog.count; ++i)
    {
        if (pData->midiprog.data[i].bank == bank && pData->midiprog.data[i].program == program)
            return setMidiProgram(static_cast<int32_t>(i), sendGui, sendOsc, sendCallback);
    }
}

// -------------------------------------------------------------------
// Plugin state

void CarlaPlugin::reloadPrograms(const bool)
{
}

// -------------------------------------------------------------------
// Plugin processing

void CarlaPlugin::activate() noexcept
{
    CARLA_SAFE_ASSERT(! pData->active);
}

void CarlaPlugin::deactivate() noexcept
{
    CARLA_SAFE_ASSERT(pData->active);
}

void CarlaPlugin::bufferSizeChanged(const uint32_t)
{
}

void CarlaPlugin::sampleRateChanged(const double)
{
}

void CarlaPlugin::offlineModeChanged(const bool)
{
}

// -------------------------------------------------------------------
// Misc

void CarlaPlugin::idle()
{
    if (! pData->enabled)
        return;

    const bool hasUI(pData->hints & PLUGIN_HAS_CUSTOM_UI);
    const bool needsUiMainThread(pData->hints & PLUGIN_NEEDS_UI_MAIN_THREAD);
#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    const bool sendOsc(pData->engine->isOscControlRegistered());
#endif
    const uint32_t latency(getLatencyInFrames());

    if (pData->latency.frames != latency)
    {
        carla_stdout("latency changed to %i", latency);

        const ScopedSingleProcessLocker sspl(this, true);

        pData->client->setLatency(latency);
        pData->latency.recreateBuffers(pData->latency.channels, latency);
    }

    const CarlaMutexLocker sl(pData->postRtEvents.mutex);

    for (RtLinkedList<PluginPostRtEvent>::Itenerator it = pData->postRtEvents.data.begin2(); it.valid(); it.next())
    {
        const PluginPostRtEvent& event(it.getValue(kPluginPostRtEventFallback));
        CARLA_SAFE_ASSERT_CONTINUE(event.type != kPluginPostRtEventNull);

        switch (event.type)
        {
        case kPluginPostRtEventNull: {
        } break;

        case kPluginPostRtEventDebug: {
            pData->engine->callback(ENGINE_CALLBACK_DEBUG, pData->id, event.value1, event.value2, event.value3, nullptr);
        } break;

        case kPluginPostRtEventParameterChange: {
            // Update UI
            if (event.value1 >= 0 && hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiParameterChange(static_cast<uint32_t>(event.value1), event.value3);
            }

            if (event.value2 != 1)
            {
#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
                // Update OSC control client
                if (sendOsc)
                    pData->engine->oscSend_control_set_parameter_value(pData->id, event.value1, event.value3);
#endif
                // Update Host
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, event.value1, 0, event.value3, nullptr);
            }
        } break;

        case kPluginPostRtEventProgramChange: {
            // Update UI
            if (event.value1 >= 0 && hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiProgramChange(static_cast<uint32_t>(event.value1));
            }

            // Update param values
            for (uint32_t j=0; j < pData->param.count; ++j)
            {
                const float paramDefault(pData->param.ranges[j].def);
                const float paramValue(getParameterValue(j));

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
                if (sendOsc)
                {
                    pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(j), paramValue);
                    pData->engine->oscSend_control_set_default_value(pData->id, j, paramDefault);
                }
#endif
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(j), 0, paramValue, nullptr);
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED, pData->id, static_cast<int>(j), 0, paramDefault, nullptr);
            }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_set_current_program(pData->id, event.value1);
#endif
            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_PROGRAM_CHANGED, pData->id, event.value1, 0, 0.0f, nullptr);

        } break;

        case kPluginPostRtEventMidiProgramChange: {
            // Update UI
            if (event.value1 >= 0 && hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiMidiProgramChange(static_cast<uint32_t>(event.value1));
            }

            // Update param values
            for (uint32_t j=0; j < pData->param.count; ++j)
            {
                const float paramDefault(pData->param.ranges[j].def);
                const float paramValue(getParameterValue(j));

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
                if (sendOsc)
                {
                    pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(j), paramValue);
                    pData->engine->oscSend_control_set_default_value(pData->id, j, paramDefault);
                }
#endif
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(j), 0, paramValue, nullptr);
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED, pData->id, static_cast<int>(j), 0, paramDefault, nullptr);
            }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_set_current_midi_program(pData->id, event.value1);
#endif
            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED, pData->id, event.value1, 0, 0.0f, nullptr);

        } break;

        case kPluginPostRtEventNoteOn: {
            CARLA_SAFE_ASSERT_BREAK(event.value1 >= 0 && event.value1 < MAX_MIDI_CHANNELS);
            CARLA_SAFE_ASSERT_BREAK(event.value2 >= 0 && event.value2 < MAX_MIDI_NOTE);
            CARLA_SAFE_ASSERT_BREAK(event.value3 >= 0 && event.value3 < MAX_MIDI_VALUE);

            const uint8_t channel  = static_cast<uint8_t>(event.value1);
            const uint8_t note     = static_cast<uint8_t>(event.value2);
            const uint8_t velocity = uint8_t(event.value3);

            // Update UI
            if (hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiNoteOn(channel, note, velocity);
            }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_note_on(pData->id, channel, note, velocity);
#endif
            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_NOTE_ON, pData->id, event.value1, event.value2, event.value3, nullptr);

        } break;

        case kPluginPostRtEventNoteOff: {
            CARLA_SAFE_ASSERT_BREAK(event.value1 >= 0 && event.value1 < MAX_MIDI_CHANNELS);
            CARLA_SAFE_ASSERT_BREAK(event.value2 >= 0 && event.value2 < MAX_MIDI_NOTE);

            const uint8_t channel = static_cast<uint8_t>(event.value1);
            const uint8_t note    = static_cast<uint8_t>(event.value2);

            // Update UI
            if (hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiNoteOff(channel, note);
            }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_note_off(pData->id, channel, note);
#endif
            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_NOTE_OFF, pData->id, event.value1, event.value2, 0.0f, nullptr);

        } break;
        }
    }

    pData->postRtEvents.data.clear();
}

bool CarlaPlugin::tryLock(const bool forcedOffline) noexcept
{
    if (forcedOffline)
    {
        pData->masterMutex.lock();
        return true;
    }

    return pData->masterMutex.tryLock();
}

void CarlaPlugin::unlock() noexcept
{
    pData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Plugin buffers

void CarlaPlugin::initBuffers() const noexcept
{
    pData->audioIn.initBuffers();
    pData->audioOut.initBuffers();
    pData->cvIn.initBuffers();
    pData->cvOut.initBuffers();
    pData->event.initBuffers();
}

void CarlaPlugin::clearBuffers() noexcept
{
    pData->clearBuffers();
}

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
// -------------------------------------------------------------------
// OSC stuff

void CarlaPlugin::registerToOscClient() noexcept
{
    if (! pData->engine->isOscControlRegistered())
        return;

    pData->engine->oscSend_control_add_plugin_start(pData->id, pData->name);

    // Base data
    {
        char bufName[STR_MAX+1], bufLabel[STR_MAX+1], bufMaker[STR_MAX+1], bufCopyright[STR_MAX+1];
        carla_zeroChars(bufName, STR_MAX);
        carla_zeroChars(bufLabel, STR_MAX);
        carla_zeroChars(bufMaker, STR_MAX);
        carla_zeroChars(bufCopyright, STR_MAX);

        getRealName(bufName);
        getLabel(bufLabel);
        getMaker(bufMaker);
        getCopyright(bufCopyright);

        pData->engine->oscSend_control_set_plugin_info1(pData->id, getType(), getCategory(), pData->hints, getUniqueId());
        pData->engine->oscSend_control_set_plugin_info2(pData->id, bufName, bufLabel, bufMaker, bufCopyright);
    }

    // Base count
    {
        uint32_t paramIns, paramOuts;
        getParameterCountInfo(paramIns, paramOuts);

        pData->engine->oscSend_control_set_audio_count(pData->id, getAudioInCount(), getAudioOutCount());
        pData->engine->oscSend_control_set_midi_count(pData->id, getMidiInCount(), getMidiOutCount());
        pData->engine->oscSend_control_set_parameter_count(pData->id, paramIns, paramOuts);
    }

    // Plugin Parameters
    if (const uint32_t count = pData->param.count)
    {
        char bufName[STR_MAX+1], bufUnit[STR_MAX+1];

        for (uint32_t i=0, maxParams=pData->engine->getOptions().maxParameters; i<count && i<maxParams; ++i)
        {
            carla_zeroChars(bufName, STR_MAX);
            carla_zeroChars(bufUnit, STR_MAX);

            getParameterName(i, bufName);
            getParameterUnit(i, bufUnit);

            const ParameterData& paramData(pData->param.data[i]);
            const ParameterRanges& paramRanges(pData->param.ranges[i]);

            pData->engine->oscSend_control_set_parameter_data(pData->id, i, paramData.type, paramData.hints, bufName, bufUnit);
            pData->engine->oscSend_control_set_parameter_ranges1(pData->id, i, paramRanges.def, paramRanges.min, paramRanges.max);
            pData->engine->oscSend_control_set_parameter_ranges2(pData->id, i, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(i), getParameterValue(i));
            pData->engine->oscSend_control_set_parameter_midi_cc(pData->id, i, paramData.midiCC);
            pData->engine->oscSend_control_set_parameter_midi_channel(pData->id, i, paramData.midiChannel);
        }
    }

    // Programs
    if (const uint32_t count = pData->prog.count)
    {
        pData->engine->oscSend_control_set_program_count(pData->id, count);

        for (uint32_t i=0; i < count; ++i)
            pData->engine->oscSend_control_set_program_name(pData->id, i, pData->prog.names[i]);

        pData->engine->oscSend_control_set_current_program(pData->id, pData->prog.current);
    }

    // MIDI Programs
    if (const uint32_t count = pData->midiprog.count)
    {
        pData->engine->oscSend_control_set_midi_program_count(pData->id, count);

        for (uint32_t i=0; i < count; ++i)
        {
            const MidiProgramData& mpData(pData->midiprog.data[i]);

            pData->engine->oscSend_control_set_midi_program_data(pData->id, i, mpData.bank, mpData.program, mpData.name);
        }

        pData->engine->oscSend_control_set_current_midi_program(pData->id, pData->midiprog.current);
    }

    pData->engine->oscSend_control_add_plugin_end(pData->id);

    // Internal Parameters
    {
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_DRYWET, pData->postProc.dryWet);
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_VOLUME, pData->postProc.volume);
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_BALANCE_LEFT, pData->postProc.balanceLeft);
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_BALANCE_RIGHT, pData->postProc.balanceRight);
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_PANNING, pData->postProc.panning);
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_CTRL_CHANNEL, pData->ctrlChannel);
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_ACTIVE, pData->active ? 1.0f : 0.0f);
    }
}
#endif

// FIXME
void CarlaPlugin::handleOscMessage(const char* const, const int, const void* const, const char* const, const lo_message)
{
    // do nothing
}
//#endif // HAVE_LIBLO && ! BUILD_BRIDGE

// -------------------------------------------------------------------
// MIDI events

void CarlaPlugin::sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    CARLA_SAFE_ASSERT_RETURN(velo < MAX_MIDI_VALUE,);

    if (! pData->active)
        return;

    ExternalMidiNote extNote;
    extNote.channel = static_cast<int8_t>(channel);
    extNote.note    = note;
    extNote.velo    = velo;

    pData->extNotes.appendNonRT(extNote);

    if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
    {
        if (velo > 0)
            uiNoteOn(channel, note, velo);
        else
            uiNoteOff(channel, note);
    }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    if (sendOsc && pData->engine->isOscControlRegistered())
    {
        if (velo > 0)
            pData->engine->oscSend_control_note_on(pData->id, channel, note, velo);
        else
            pData->engine->oscSend_control_note_off(pData->id, channel, note);
    }
#endif

    if (sendCallback)
        pData->engine->callback((velo > 0) ? ENGINE_CALLBACK_NOTE_ON : ENGINE_CALLBACK_NOTE_OFF, pData->id, channel, note, velo, nullptr);

    // may be unused
    return; (void)sendOsc;
}

#ifndef BUILD_BRIDGE
void CarlaPlugin::sendMidiAllNotesOffToCallback()
{
    if (pData->ctrlChannel < 0 || pData->ctrlChannel >= MAX_MIDI_CHANNELS)
        return;

    PluginPostRtEvent postEvent;
    postEvent.type   = kPluginPostRtEventNoteOff;
    postEvent.value1 = pData->ctrlChannel;
    postEvent.value2 = 0;
    postEvent.value3 = 0.0f;

    for (int32_t i=0; i < MAX_MIDI_NOTE; ++i)
    {
        postEvent.value2 = i;
        pData->postRtEvents.appendRT(postEvent);
    }
}
#endif

// -------------------------------------------------------------------
// UI Stuff

void CarlaPlugin::showCustomUI(const bool)
{
    CARLA_SAFE_ASSERT(false);
}

void CarlaPlugin::uiIdle()
{
    if (pData->hints & PLUGIN_NEEDS_UI_MAIN_THREAD)
    {
        // Update parameter outputs
        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (pData->param.data[i].type == PARAMETER_OUTPUT)
                uiParameterChange(i, getParameterValue(i));
        }

        const CarlaMutexLocker sl(pData->postUiEvents.mutex);

        for (LinkedList<PluginPostRtEvent>::Itenerator it = pData->postUiEvents.data.begin2(); it.valid(); it.next())
        {
            const PluginPostRtEvent& event(it.getValue(kPluginPostRtEventFallback));
            CARLA_SAFE_ASSERT_CONTINUE(event.type != kPluginPostRtEventNull);

            switch (event.type)
            {
            case kPluginPostRtEventNull:
            case kPluginPostRtEventDebug:
                break;

            case kPluginPostRtEventParameterChange:
                uiParameterChange(static_cast<uint32_t>(event.value1), event.value3);
                break;

            case kPluginPostRtEventProgramChange:
                uiProgramChange(static_cast<uint32_t>(event.value1));
                break;

            case kPluginPostRtEventMidiProgramChange:
                uiMidiProgramChange(static_cast<uint32_t>(event.value1));
                break;

            case kPluginPostRtEventNoteOn:
                uiNoteOn(static_cast<uint8_t>(event.value1), static_cast<uint8_t>(event.value2), uint8_t(event.value3));
                break;

            case kPluginPostRtEventNoteOff:
                uiNoteOff(static_cast<uint8_t>(event.value1), static_cast<uint8_t>(event.value2));
                break;
            }
        }

        pData->postUiEvents.data.clear();
    }

    if (pData->transientTryCounter == 0)
        return;
    if (++pData->transientTryCounter % 10 != 0)
        return;
    if (pData->transientTryCounter >= 200)
        return;

    carla_stdout("Trying to get window...");

    CarlaString uiTitle(pData->name);
    uiTitle += " (GUI)";

    if (CarlaPluginUI::tryTransientWinIdMatch(getUiBridgeProcessId(), uiTitle, pData->engine->getOptions().frontendWinId, true))
        pData->transientTryCounter = 0;
}

void CarlaPlugin::uiParameterChange(const uint32_t index, const float value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(),);
    return;

    // unused
    (void)value;
}

void CarlaPlugin::uiProgramChange(const uint32_t index) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < getProgramCount(),);
}

void CarlaPlugin::uiMidiProgramChange(const uint32_t index) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < getMidiProgramCount(),);
}

void CarlaPlugin::uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
    CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);
}

void CarlaPlugin::uiNoteOff(const uint8_t channel, const uint8_t note) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
}

bool CarlaPlugin::canRunInRack() const noexcept
{
    return (pData->extraHints & PLUGIN_EXTRA_HINT_CAN_RUN_RACK) != 0;
}

CarlaEngine* CarlaPlugin::getEngine() const noexcept
{
    return pData->engine;
}

CarlaEngineClient* CarlaPlugin::getEngineClient() const noexcept
{
    return pData->client;
}

CarlaEngineAudioPort* CarlaPlugin::getAudioInPort(const uint32_t index) const noexcept
{
    return pData->audioIn.ports[index].port;
}

CarlaEngineAudioPort* CarlaPlugin::getAudioOutPort(const uint32_t index) const noexcept
{
    return pData->audioOut.ports[index].port;
}

CarlaEngineCVPort* CarlaPlugin::getCVInPort(const uint32_t index) const noexcept
{
    return pData->cvIn.ports[index].port;
}

CarlaEngineCVPort* CarlaPlugin::getCVOutPort(const uint32_t index) const noexcept
{
    return pData->cvOut.ports[index].port;
}

CarlaEngineEventPort* CarlaPlugin::getDefaultEventInPort() const noexcept
{
    return pData->event.portIn;
}

CarlaEngineEventPort* CarlaPlugin::getDefaultEventOutPort() const noexcept
{
    return pData->event.portOut;
}

void* CarlaPlugin::getNativeHandle() const noexcept
{
    return nullptr;
}

const void* CarlaPlugin::getNativeDescriptor() const noexcept
{
    return nullptr;
}

uintptr_t CarlaPlugin::getUiBridgeProcessId() const noexcept
{
    return 0;
}

// -------------------------------------------------------------------

uint32_t CarlaPlugin::getPatchbayNodeId() const noexcept
{
    return pData->nodeId;
}

void CarlaPlugin::setPatchbayNodeId(const uint32_t nodeId) noexcept
{
    pData->nodeId = nodeId;
}

// -------------------------------------------------------------------
// Scoped Disabler

CarlaPlugin::ScopedDisabler::ScopedDisabler(CarlaPlugin* const plugin) noexcept
    : fPlugin(plugin)
{
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin->pData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin->pData->client != nullptr,);
    carla_debug("CarlaPlugin::ScopedDisabler(%p)", plugin);

    plugin->pData->masterMutex.lock();

    if (plugin->pData->enabled)
        plugin->pData->enabled = false;

    if (plugin->pData->client->isActive())
        plugin->pData->client->deactivate();
}

CarlaPlugin::ScopedDisabler::~ScopedDisabler() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fPlugin->pData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fPlugin->pData->client != nullptr,);
    carla_debug("CarlaPlugin::~ScopedDisabler()");

    fPlugin->pData->enabled = true;
    fPlugin->pData->client->activate();
    fPlugin->pData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Scoped Process Locker

CarlaPlugin::ScopedSingleProcessLocker::ScopedSingleProcessLocker(CarlaPlugin* const plugin, const bool block) noexcept
    : fPlugin(plugin),
      fBlock(block)
{
    CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fPlugin->pData != nullptr,);
    carla_debug("CarlaPlugin::ScopedSingleProcessLocker(%p, %s)", plugin, bool2str(block));

    if (! fBlock)
        return;

    plugin->pData->singleMutex.lock();
}

CarlaPlugin::ScopedSingleProcessLocker::~ScopedSingleProcessLocker() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fPlugin->pData != nullptr,);
    carla_debug("CarlaPlugin::~ScopedSingleProcessLocker()");

    if (! fBlock)
        return;

#ifndef BUILD_BRIDGE
    if (fPlugin->pData->singleMutex.wasTryLockCalled())
        fPlugin->pData->needsReset = true;
#endif

    fPlugin->pData->singleMutex.unlock();
}

// -------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
