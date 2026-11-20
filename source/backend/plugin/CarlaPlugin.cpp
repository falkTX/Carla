// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaMIDI.h"
#include "CarlaPluginUI.hpp"
// #include "CarlaScopeUtils.hpp"
#include "CarlaStringList.hpp"

#include <ctime>

#include "extra/Base64.hpp"
#include "extra/ScopedPointer.hpp"

#include "water/files/File.h"
#include "water/streams/MemoryOutputStream.h"
#include "water/xml/XmlDocument.h"
#include "water/xml/XmlElement.h"

using water::CharPointer_UTF8;
using water::File;
using water::MemoryOutputStream;
using water::Result;
using water::XmlDocument;
using water::XmlElement;

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Fallback data

static const ParameterData   kParameterDataNull   = { PARAMETER_UNKNOWN, 0x0, PARAMETER_NULL, -1, 0, CONTROL_INDEX_NONE, 0.0f, 1.0f, 0x0 };
static const ParameterRanges kParameterRangesNull = { 0.0f, 0.0f, 1.0f, 0.01f, 0.0001f, 0.1f };
static const MidiProgramData kMidiProgramDataNull = { 0, 0, nullptr };

static const CustomData        kCustomDataFallback        = { nullptr, nullptr, nullptr };
static /* */ CustomData        kCustomDataFallbackNC      = { nullptr, nullptr, nullptr };
static const PluginPostRtEvent kPluginPostRtEventFallback = { kPluginPostRtEventNull, false, {} };

// -------------------------------------------------------------------------------------------------------------------
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
    CARLA_DECLARE_NON_COPYABLE(ParamSymbol)
#endif
};

// -------------------------------------------------------------------------------------------------------------------
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

uint CarlaPlugin::getAudioPortHints(bool, uint32_t) const noexcept
{
    return 0x0;
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

bool CarlaPlugin::getLabel(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getMaker(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getCopyright(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getRealName(char* const strBuf) const noexcept
{
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);
    CARLA_SAFE_ASSERT(false); // this should never happen
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getParameterText(const uint32_t parameterId, char* const strBuf) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);
    CARLA_SAFE_ASSERT(false); // this should never happen
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getParameterComment(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getParameterGroupName(const uint32_t parameterId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);
    strBuf[0] = '\0';
    return false;
}

bool CarlaPlugin::getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < getParameterCount(), false);
    CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId), false);
    CARLA_SAFE_ASSERT(false); // this should never happen
    strBuf[0] = '\0';
    return false;
}

float CarlaPlugin::getInternalParameterValue(const int32_t parameterId) const noexcept
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
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

bool CarlaPlugin::getProgramName(const uint32_t index, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->prog.count, false);
    CARLA_SAFE_ASSERT_RETURN(pData->prog.names[index] != nullptr, false);
    std::strncpy(strBuf, pData->prog.names[index], STR_MAX);
    return true;
}

bool CarlaPlugin::getMidiProgramName(const uint32_t index, char* const strBuf) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count, false);
    CARLA_SAFE_ASSERT_RETURN(pData->midiprog.data[index].name != nullptr, false);
    std::strncpy(strBuf, pData->midiprog.data[index].name, STR_MAX);
    return true;
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

void CarlaPlugin::prepareForSave(bool)
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

        if (! getParameterName(i, strBuf))
            strBuf[0] = '\0';

        if (std::strstr(strBuf, "olume") != nullptr)
            continue;
        if (std::strstr(strBuf, "Master") != nullptr)
            continue;

        const ParameterRanges& paramRanges(pData->param.ranges[i]);

        if (paramData.hints & PARAMETER_IS_BOOLEAN)
        {
            random = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            value  = random > 0.5f ? paramRanges.max : paramRanges.min;
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
    pData->stateSave.clear();

    if (callPrepareForSave)
    {
        pData->stateSave.temporary = true;
        prepareForSave(true);
    }

    const PluginType pluginType = getType();

    char strBuf[STR_MAX+1];
    carla_zeroChars(strBuf, STR_MAX+1);

    // ---------------------------------------------------------------
    // Basic info

    if (! getLabel(strBuf))
        strBuf[0] = '\0';

    pData->stateSave.type     = carla_strdup(getPluginTypeAsString(pluginType));
    pData->stateSave.name     = carla_strdup(pData->name);
    pData->stateSave.label    = carla_strdup(strBuf);
    pData->stateSave.uniqueId = getUniqueId();
// #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    pData->stateSave.options  = pData->options;
// #endif

    if (pData->filename != nullptr)
        pData->stateSave.binary = carla_strdup(pData->filename);

   #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
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

    if (pData->hints & PLUGIN_IS_BRIDGE)
        waitForBridgeSaveSignal();

    // ---------------------------------------------------------------
    // Chunk

    bool usingChunk = false;

    if (pData->options & PLUGIN_OPTION_USE_CHUNKS)
    {
        void* data = nullptr;
        const std::size_t dataSize = getChunkData(&data);

        if (data != nullptr && dataSize > 0)
        {
            pData->stateSave.chunk = carla_strdup(String::asBase64(data, dataSize));

            if (pluginType != PLUGIN_INTERNAL && pluginType != PLUGIN_JSFX)
                usingChunk = true;
        }
    }

    // ---------------------------------------------------------------
    // Current Program

    if (pData->prog.current >= 0 && pluginType != PLUGIN_LV2)
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

    const float sampleRate = static_cast<float>(pData->engine->getSampleRate());

    for (uint32_t i=0; i < pData->param.count; ++i)
    {
        const ParameterData& paramData(pData->param.data[i]);

        if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
            continue;
        if (paramData.hints & PARAMETER_IS_NOT_SAVED)
            continue;

        const bool dummy = paramData.type != PARAMETER_INPUT || usingChunk;

        if (dummy && paramData.mappedControlIndex <= CONTROL_INDEX_NONE)
            continue;

        CarlaStateSave::Parameter* const stateParameter(new CarlaStateSave::Parameter());

        stateParameter->dummy = dummy;
        stateParameter->index = paramData.index;
       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (paramData.mappedControlIndex != CONTROL_INDEX_MIDI_LEARN)
        {
            stateParameter->mappedControlIndex = paramData.mappedControlIndex;
            stateParameter->midiChannel        = paramData.midiChannel;

            if (paramData.hints & PARAMETER_MAPPED_RANGES_SET)
            {
                stateParameter->mappedMinimum = paramData.mappedMinimum;
                stateParameter->mappedMaximum = paramData.mappedMaximum;
                stateParameter->mappedRangeValid = true;

                if (paramData.hints & PARAMETER_USES_SAMPLERATE)
                {
                    stateParameter->mappedMinimum /= sampleRate;
                    stateParameter->mappedMaximum /= sampleRate;
                }
            }
        }
       #endif

        if (! getParameterName(i, strBuf))
            strBuf[0] = '\0';
        stateParameter->name = carla_strdup(strBuf);

        if (! getParameterSymbol(i, strBuf))
            strBuf[0] = '\0';
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
    const bool usesMultiProgs = pData->hints & PLUGIN_USES_MULTI_PROGS;
    const PluginType pluginType = getType();

    char strBuf[STR_MAX+1];
    carla_zeroChars(strBuf, STR_MAX+1);

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
                if (getProgramName(i, strBuf) && std::strcmp(stateSave.currentProgramName, strBuf) == 0)
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

    if (pluginType == PLUGIN_LADSPA || pluginType == PLUGIN_LV2 || pluginType == PLUGIN_CLAP)
    {
        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (pData->param.data[i].hints & PARAMETER_IS_NOT_SAVED)
                continue;

            if (getParameterSymbol(i, strBuf))
            {
                ParamSymbol* const paramSymbol(new ParamSymbol(i, strBuf));
                paramSymbols.append(paramSymbol);
            }
        }
    }

    // ---------------------------------------------------------------
    // Part 4b - set parameter values (carefully)

    const float sampleRate = static_cast<float>(pData->engine->getSampleRate());

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
            {
                index = stateParameter->index;
            }
        }
        else if (pluginType == PLUGIN_LV2 || pluginType == PLUGIN_CLAP)
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
                {
                    if (pluginType == PLUGIN_LV2)
                        carla_stderr("Failed to find LV2 parameter symbol '%s' for '%s'",
                                    stateParameter->symbol, pData->name);
                    else
                        carla_stderr("Failed to find CLAP parameter id '%s' for '%s'",
                                    stateParameter->symbol, pData->name);
                }
            }
            else
            {
                if (pluginType == PLUGIN_LV2)
                    carla_stderr("LV2 Plugin parameter '%s' has no symbol", stateParameter->name);
                else
                    carla_stderr("CLAP Plugin parameter '%s' has no id", stateParameter->name);
            }
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

           #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (stateParameter->mappedRangeValid)
            {
                if (pData->param.data[index].hints & PARAMETER_USES_SAMPLERATE)
                {
                    stateParameter->mappedMinimum *= sampleRate;
                    stateParameter->mappedMaximum *= sampleRate;
                }

                setParameterMappedRange(static_cast<uint32_t>(index),
                                        stateParameter->mappedMinimum,
                                        stateParameter->mappedMaximum, true, true);
            }

            setParameterMappedControlIndex(static_cast<uint32_t>(index),
                                           stateParameter->mappedControlIndex, true, true, false);
            setParameterMidiChannel(static_cast<uint32_t>(index), stateParameter->midiChannel, true, true);
           #endif
        }
        else
            carla_stderr("Could not set parameter '%s' value for '%s'",
                         stateParameter->name, pData->name);
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

    if (pluginType == PLUGIN_LV2)
    {
        for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
        {
            const CustomData& customData(it.getValue(kCustomDataFallback));
            CARLA_SAFE_ASSERT_CONTINUE(customData.isValid());

            if (std::strcmp(customData.type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
                continue;

            restoreLV2State(stateSave.temporary);
            break;
        }
    }

    // ---------------------------------------------------------------
    // Part 6 - set chunk

    if (stateSave.chunk != nullptr && (pData->options & PLUGIN_OPTION_USE_CHUNKS) != 0)
    {
        std::vector<uint8_t> chunk;
        d_getChunkFromBase64String_impl(chunk, stateSave.chunk);
       #ifdef CARLA_PROPER_CPP11_SUPPORT
        setChunkData(chunk.data(), chunk.size());
       #else
        setChunkData(&chunk.front(), chunk.size());
       #endif
    }

   #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // ---------------------------------------------------------------
    // Part 6 - set internal stuff

    const uint availOptions = getOptionsAvailable();

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

    if (! pData->engine->isLoadingProject())
        pData->engine->callback(true, true, ENGINE_CALLBACK_UPDATE, pData->id, 0, 0, 0, 0.0f, nullptr);
   #endif
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

    File file(filename);

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

    File file(filename);
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

#ifndef CARLA_PLUGIN_ONLY_BRIDGE
bool CarlaPlugin::exportAsLV2(const char* const lv2path)
{
    CARLA_SAFE_ASSERT_RETURN(lv2path != nullptr && lv2path[0] != '\0', false);
    carla_debug("CarlaPlugin::exportAsLV2(\"%s\")", lv2path);

    String bundlepath(lv2path);

    if (! bundlepath.endsWith(".lv2"))
        bundlepath += ".lv2";

    const File bundlefolder(bundlepath.buffer());

    if (bundlefolder.existsAsFile())
    {
        pData->engine->setLastError("Requested filename already exists as file, use a folder instead");
        return false;
    }

    if (! bundlefolder.exists())
    {
        const Result res(bundlefolder.createDirectory());

        if (res.failed())
        {
            pData->engine->setLastError(res.getErrorMessage().c_str());
            return false;
        }
    }

    String symbol(pData->name);
    symbol.toBasic();

    {
        const String pluginFilename(bundlepath + CARLA_OS_SEP_STR + symbol + ".xml");

        if (! saveStateToFile(pluginFilename))
            return false;
    }

    {
        MemoryOutputStream manifestStream;

        manifestStream << "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n";
        manifestStream << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
        manifestStream << "@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n";
        manifestStream << "\n";
        manifestStream << "<" << symbol.buffer() << ".ttl>\n";
        manifestStream << "    a lv2:Plugin ;\n";
        manifestStream << "    lv2:binary <" << symbol.buffer() << CARLA_LIB_EXT "> ;\n";
        manifestStream << "    rdfs:seeAlso <" << symbol.buffer() << ".ttl> .\n";
        manifestStream << "\n";
        manifestStream << "<ext-ui>\n";
        manifestStream << "    a <http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget> ;\n";
        manifestStream << "    ui:binary <" << symbol.buffer() << CARLA_LIB_EXT "> ;\n";
        manifestStream << "    lv2:extensionData <http://lv2plug.in/ns/extensions/ui#idleInterface> ,\n";
        manifestStream << "                      <http://lv2plug.in/ns/extensions/ui#showInterface> ;\n";
        manifestStream << "    lv2:requiredFeature <http://lv2plug.in/ns/ext/instance-access> .\n";
        manifestStream << "\n";

        const String manifestFilename(bundlepath + CARLA_OS_SEP_STR "manifest.ttl");
        const File manifestFile(manifestFilename.buffer());

        if (! manifestFile.replaceWithData(manifestStream.getData(), manifestStream.getDataSize()))
        {
            pData->engine->setLastError("Failed to write manifest.ttl file");
            return false;
        }
    }

    {
        MemoryOutputStream mainStream;

        mainStream << "@prefix atom: <http://lv2plug.in/ns/ext/atom#> .\n";
        mainStream << "@prefix doap: <http://usefulinc.com/ns/doap#> .\n";
        mainStream << "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n";
        mainStream << "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n";
        mainStream << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
        mainStream << "@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n";
        mainStream << "\n";
        mainStream << "<>\n";
        mainStream << "    a lv2:Plugin ;\n";
        mainStream << "\n";
        mainStream << "    lv2:requiredFeature <http://lv2plug.in/ns/ext/buf-size#boundedBlockLength> ,\n";
        mainStream << "                        <http://lv2plug.in/ns/ext/options#options> ,\n";
        mainStream << "                        <http://lv2plug.in/ns/ext/urid#map> ;\n";
        mainStream << "\n";

        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            mainStream << "    ui:ui <ext-ui> ;\n";
            mainStream << "\n";
        }

        const uint32_t midiIns  = getMidiInCount();
        const uint32_t midiOuts = getMidiOutCount();

        int portIndex = 0;

        if (midiIns > 0)
        {
            mainStream << "    lv2:port [\n";
            mainStream << "        a lv2:InputPort, atom:AtomPort ;\n";
            mainStream << "        lv2:index 0 ;\n";
            mainStream << "        lv2:symbol \"clv2_events_in\" ;\n";
            mainStream << "        lv2:name \"Events Input\" ;\n";
            mainStream << "        atom:bufferType atom:Sequence ;\n";
            mainStream << "        atom:supports <http://lv2plug.in/ns/ext/midi#MidiEvent> ,\n";
            mainStream << "                      <http://lv2plug.in/ns/ext/time#Position> ;\n";
            mainStream << "    ] ;\n";
            ++portIndex;

            for (uint32_t i=1; i<midiIns; ++i)
            {
                const water::String portIndexNum(portIndex++);
                const water::String portIndexLabel(portIndex);

                mainStream << "    lv2:port [\n";
                mainStream << "        a lv2:InputPort, atom:AtomPort ;\n";
                mainStream << "        lv2:index " << portIndexNum << " ;\n";
                mainStream << "        lv2:symbol \"clv2_midi_in_" << portIndexLabel << "\" ;\n";
                mainStream << "        lv2:name \"MIDI Input " << portIndexLabel << "\" ;\n";
                mainStream << "    ] ;\n";
            }
        }
        else
        {
            mainStream << "    lv2:port [\n";
            mainStream << "        a lv2:InputPort, atom:AtomPort ;\n";
            mainStream << "        lv2:index 0 ;\n";
            mainStream << "        lv2:symbol \"clv2_time_info\" ;\n";
            mainStream << "        lv2:name \"Time Info\" ;\n";
            mainStream << "        atom:bufferType atom:Sequence ;\n";
            mainStream << "        atom:supports <http://lv2plug.in/ns/ext/time#Position> ;\n";
            mainStream << "    ] ;\n";
            ++portIndex;
        }

        for (uint32_t i=0; i<midiOuts; ++i)
        {
            const water::String portIndexNum(portIndex++);
            const water::String portIndexLabel(portIndex);

            mainStream << "    lv2:port [\n";
            mainStream << "        a lv2:InputPort, atom:AtomPort ;\n";
            mainStream << "        lv2:index " << portIndexNum << " ;\n";
            mainStream << "        lv2:symbol \"clv2_midi_out_" << portIndexLabel << "\" ;\n";
            mainStream << "        lv2:name \"MIDI Output " << portIndexLabel << "\" ;\n";
            mainStream << "        atom:bufferType atom:Sequence ;\n";
            mainStream << "        atom:supports <http://lv2plug.in/ns/ext/midi#MidiEvent> ;\n";
            mainStream << "    ] ;\n";
        }

        mainStream << "    lv2:port [\n";
        mainStream << "        a lv2:InputPort, lv2:ControlPort ;\n";
        mainStream << "        lv2:index " << String(portIndex++) << " ;\n";
        mainStream << "        lv2:name \"freewheel\" ;\n";
        mainStream << "        lv2:symbol \"clv2_freewheel\" ;\n";
        mainStream << "        lv2:default 0 ;\n";
        mainStream << "        lv2:minimum 0 ;\n";
        mainStream << "        lv2:maximum 1 ;\n";
        mainStream << "        lv2:designation lv2:freeWheeling ;\n";
        mainStream << "        lv2:portProperty lv2:toggled , lv2:integer ;\n";
        mainStream << "        lv2:portProperty <http://lv2plug.in/ns/ext/port-props#notOnGUI> ;\n";
        mainStream << "    ] ;\n";

        for (uint32_t i=0; i<pData->audioIn.count; ++i)
        {
            const water::String portIndexNum(portIndex++);
            const water::String portIndexLabel(i+1);

            mainStream << "    lv2:port [\n";
            mainStream << "        a lv2:InputPort, lv2:AudioPort ;\n";
            mainStream << "        lv2:index " << portIndexNum << " ;\n";
            mainStream << "        lv2:symbol \"clv2_audio_in_" << portIndexLabel << "\" ;\n";
            mainStream << "        lv2:name \"Audio Input " << portIndexLabel << "\" ;\n";
            mainStream << "    ] ;\n";
        }

        for (uint32_t i=0; i<pData->audioOut.count; ++i)
        {
            const water::String portIndexNum(portIndex++);
            const water::String portIndexLabel(i+1);

            mainStream << "    lv2:port [\n";
            mainStream << "        a lv2:OutputPort, lv2:AudioPort ;\n";
            mainStream << "        lv2:index " << portIndexNum << " ;\n";
            mainStream << "        lv2:symbol \"clv2_audio_out_" << portIndexLabel << "\" ;\n";
            mainStream << "        lv2:name \"Audio Output " << portIndexLabel << "\" ;\n";
            mainStream << "    ] ;\n";
        }

        CarlaStringList uniqueSymbolNames;

        char strBufName[STR_MAX+1];
        char strBufSymbol[STR_MAX+1];
        carla_zeroChars(strBufName, STR_MAX+1);
        carla_zeroChars(strBufSymbol, STR_MAX+1);

        for (uint32_t i=0; i<pData->param.count; ++i)
        {
            const ParameterData&   paramData(pData->param.data[i]);
            const ParameterRanges& paramRanges(pData->param.ranges[i]);

            const water::String portIndexNum(portIndex++);

            mainStream << "    lv2:port [\n";

            if (paramData.type == PARAMETER_INPUT)
                mainStream << "        a lv2:InputPort, lv2:ControlPort ;\n";
            else
                mainStream << "        a lv2:OutputPort, lv2:ControlPort ;\n";

            if (paramData.hints & PARAMETER_IS_BOOLEAN)
                mainStream << "        lv2:portProperty lv2:toggled ;\n";

            if (paramData.hints & PARAMETER_IS_INTEGER)
                mainStream << "        lv2:portProperty lv2:integer ;\n";

            // TODO logarithmic, enabled (not on gui), automatable, samplerate, scalepoints

            if (! getParameterName(i, strBufName))
                strBufName[0] = '\0';
            if (! getParameterSymbol(i, strBufSymbol))
                strBufSymbol[0] = '\0';

            if (strBufSymbol[0] == '\0')
            {
                String s(strBufName);
                s.toBasic();
                std::memcpy(strBufSymbol, s.buffer(), s.length()+1);

                if (strBufSymbol[0] >= '0' && strBufSymbol[0] <= '9')
                {
                    const size_t len(std::strlen(strBufSymbol));
                    std::memmove(strBufSymbol+1, strBufSymbol, len);
                    strBufSymbol[0]   = '_';
                    strBufSymbol[len+1] = '\0';
                }
            }

            if (uniqueSymbolNames.contains(strBufSymbol))
                std::snprintf(strBufSymbol, STR_MAX, "clv2_param_%d", i+1);

            mainStream << "        lv2:index " << portIndexNum << " ;\n";
            mainStream << "        lv2:symbol \"" << strBufSymbol << "\" ;\n";
            mainStream << "        lv2:name \"\"\"" << strBufName << "\"\"\" ;\n";
            mainStream << "        lv2:default " << String(paramRanges.def) << " ;\n";
            mainStream << "        lv2:minimum " << String(paramRanges.min) << " ;\n";
            mainStream << "        lv2:maximum " << String(paramRanges.max) << " ;\n";

            // TODO midiCC, midiChannel

            mainStream << "    ] ;\n";
        }

        char strBuf[STR_MAX+1];
        carla_zeroChars(strBuf, STR_MAX+1);

        if (! getMaker(strBuf))
            strBuf[0] = '\0';

        mainStream << "    rdfs:comment \"Plugin generated using Carla LV2 export.\" ;\n";
        mainStream << "    doap:name \"\"\"" << getName() << "\"\"\" ;\n";
        mainStream << "    doap:maintainer [ foaf:name \"\"\"" << strBuf << "\"\"\" ] .\n";
        mainStream << "\n";

        const String mainFilename(bundlepath + CARLA_OS_SEP_STR + symbol + ".ttl");
        const File mainFile(mainFilename.buffer());

        if (! mainFile.replaceWithData(mainStream.getData(), mainStream.getDataSize()))
        {
            pData->engine->setLastError("Failed to write main plugin ttl file");
            return false;
        }
    }

    const String binaryFilename(bundlepath + CARLA_OS_SEP_STR + symbol + CARLA_LIB_EXT);

    const File binaryFileSource(File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("carla-bridge-lv2" CARLA_LIB_EXT));
    const File binaryFileTarget(binaryFilename.buffer());

    const EngineOptions& opts(pData->engine->getOptions());

    const String binFolderTarget(bundlepath + CARLA_OS_SEP_STR + "bin");
    const String resFolderTarget(bundlepath + CARLA_OS_SEP_STR + "res");

    if (! binaryFileSource.copyFileTo(binaryFileTarget))
    {
        pData->engine->setLastError("Failed to copy plugin binary");
        return false;
    }

#ifdef CARLA_OS_WIN
    File(opts.resourceDir).copyDirectoryTo(File(resFolderTarget.buffer()));

    // Copying all the binaries is pointless, just go through the expected needed bits
    const File binFolder1(opts.binaryDir);
    const File binFolder2(binFolderTarget.buffer());
    binFolder2.createDirectory();

    static const char* files[] = {
      "carla-bridge-native.exe",
      "carla-bridge-win32.exe",
      "carla-discovery-win32.exe",
      "carla-discovery-win64.exe",
      "libcarla_utils.dll"
    };

    for (int i=0; i<5; ++i)
        binFolder1.getChildFile(files[i]).copyFileTo(binFolder2.getChildFile(files[i]));;
#else
    File(opts.binaryDir).createSymbolicLink(File(binFolderTarget.buffer()), true);
    File(opts.resourceDir).createSymbolicLink(File(resFolderTarget.buffer()), true);
#endif

    return true;
}
#endif

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
    CARLA_SAFE_ASSERT_UINT2_RETURN(getOptionsAvailable() & option, getOptionsAvailable(), option,);

    if (yesNo)
        pData->options |= option;
    else
        pData->options &= ~option;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (sendCallback)
        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_OPTION_CHANGED,
                                pData->id,
                                static_cast<int>(option),
                                yesNo ? 1 : 0,
                                0, 0.0f, nullptr);
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

    if (yesNo && ! pData->client->isActive())
        pData->client->activate();

    pData->masterMutex.unlock();
}

void CarlaPlugin::setActive(const bool active, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else if (pData->enginePlugin) {
        // nothing here
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }

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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    const float value = active ? 1.0f : 0.0f;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            PARAMETER_ACTIVE,
                            0, 0,
                            value,
                            nullptr);
#endif
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
void CarlaPlugin::setDryWet(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(0.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.dryWet, fixedValue))
        return;

    pData->postProc.dryWet = fixedValue;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            PARAMETER_DRYWET,
                            0, 0,
                            fixedValue,
                            nullptr);
}

void CarlaPlugin::setVolume(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.27f);

    const float fixedValue(carla_fixedValue<float>(0.0f, 1.27f, value));

    if (carla_isEqual(pData->postProc.volume, fixedValue))
        return;

    pData->postProc.volume = fixedValue;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            PARAMETER_VOLUME,
                            0, 0,
                            fixedValue,
                            nullptr);
}

void CarlaPlugin::setBalanceLeft(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.balanceLeft, fixedValue))
        return;

    pData->postProc.balanceLeft = fixedValue;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            PARAMETER_BALANCE_LEFT,
                            0, 0,
                            fixedValue,
                            nullptr);
}

void CarlaPlugin::setBalanceRight(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.balanceRight, fixedValue))
        return;

    pData->postProc.balanceRight = fixedValue;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            PARAMETER_BALANCE_RIGHT,
                            0, 0,
                            fixedValue,
                            nullptr);
}

void CarlaPlugin::setPanning(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.panning, fixedValue))
        return;

    pData->postProc.panning = fixedValue;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            PARAMETER_PANNING,
                            0, 0,
                            fixedValue,
                            nullptr);
}

void CarlaPlugin::setDryWetRT(const float value, const bool sendCallbackLater) noexcept
{
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(0.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.dryWet, fixedValue))
        return;

    pData->postProc.dryWet = fixedValue;
    pData->postponeParameterChangeRtEvent(sendCallbackLater, PARAMETER_DRYWET, fixedValue);
}

void CarlaPlugin::setVolumeRT(const float value, const bool sendCallbackLater) noexcept
{
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.27f);

    const float fixedValue(carla_fixedValue<float>(0.0f, 1.27f, value));

    if (carla_isEqual(pData->postProc.volume, fixedValue))
        return;

    pData->postProc.volume = fixedValue;
    pData->postponeParameterChangeRtEvent(sendCallbackLater, PARAMETER_VOLUME, fixedValue);
}

void CarlaPlugin::setBalanceLeftRT(const float value, const bool sendCallbackLater) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.balanceLeft, fixedValue))
        return;

    pData->postProc.balanceLeft = fixedValue;
    pData->postponeParameterChangeRtEvent(sendCallbackLater, PARAMETER_BALANCE_LEFT, fixedValue);
}

void CarlaPlugin::setBalanceRightRT(const float value, const bool sendCallbackLater) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.balanceRight, fixedValue))
        return;

    pData->postProc.balanceRight = fixedValue;
    pData->postponeParameterChangeRtEvent(sendCallbackLater, PARAMETER_BALANCE_RIGHT, fixedValue);
}

void CarlaPlugin::setPanningRT(const float value, const bool sendCallbackLater) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixedValue<float>(-1.0f, 1.0f, value));

    if (carla_isEqual(pData->postProc.panning, fixedValue))
        return;

    pData->postProc.panning = fixedValue;
    pData->postponeParameterChangeRtEvent(sendCallbackLater, PARAMETER_PANNING, fixedValue);
}
#endif // ! BUILD_BRIDGE_ALTERNATIVE_ARCH

void CarlaPlugin::setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT_RETURN(channel >= -1 && channel < MAX_MIDI_CHANNELS,);

    if (pData->ctrlChannel == channel)
        return;

    pData->ctrlChannel = channel;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    const float channelf = static_cast<float>(channel);

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            PARAMETER_CTRL_CHANNEL,
                            0, 0,
                            channelf, nullptr);
#endif
}

// -------------------------------------------------------------------
// Set data (plugin-specific stuff)

void CarlaPlugin::setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        // NOTE: some LV2 plugins feedback messages to UI on purpose
        CARLA_SAFE_ASSERT_RETURN(getType() == PLUGIN_LV2 || !sendGui,);
    } else if (pData->enginePlugin) {
        // nothing here
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

    if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
        uiParameterChange(parameterId, value);

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                            pData->id,
                            static_cast<int>(parameterId),
                            0, 0,
                            value,
                            nullptr);
}

void CarlaPlugin::setParameterValueRT(const uint32_t parameterId, const float value, uint32_t, const bool sendCallbackLater) noexcept
{
    pData->postponeParameterChangeRtEvent(sendCallbackLater, static_cast<int32_t>(parameterId), value);
}

void CarlaPlugin::setParameterValueByRealIndex(const int32_t rindex, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
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
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

    if (pData->param.data[parameterId].midiChannel == channel)
        return;

    pData->param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED,
                            pData->id,
                            static_cast<int>(parameterId),
                            channel,
                            0, 0.0f, nullptr);
#endif
}

void CarlaPlugin::setParameterMappedControlIndex(const uint32_t parameterId, const int16_t index,
                                                 const bool sendOsc, const bool sendCallback,
                                                 const bool reconfigureNow) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
    CARLA_SAFE_ASSERT_RETURN(index >= CONTROL_INDEX_NONE && index <= CONTROL_INDEX_MAX_ALLOWED,);

    ParameterData& paramData(pData->param.data[parameterId]);

    if (paramData.mappedControlIndex == index)
        return;

    const ParameterRanges& paramRanges(pData->param.ranges[parameterId]);

    if ((paramData.hints & PARAMETER_MAPPED_RANGES_SET) == 0x0)
        setParameterMappedRange(parameterId, paramRanges.min, paramRanges.max, true, true);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    char strBuf[STR_MAX+1];
    carla_zeroChars(strBuf, STR_MAX+1);
    if (! getParameterName(parameterId, strBuf))
        std::snprintf(strBuf, STR_MAX, "Param %u", parameterId);

    const uint portNameSize = pData->engine->getMaxPortNameSize();
    if (portNameSize < STR_MAX)
        strBuf[portNameSize] = '\0';

    // was learning something else before, stop that first
    if (pData->midiLearnParameterIndex >= 0 && pData->midiLearnParameterIndex != static_cast<int32_t>(parameterId))
    {
        const int32_t oldParameterId = pData->midiLearnParameterIndex;
        pData->midiLearnParameterIndex = -1;

        CARLA_SAFE_ASSERT_RETURN(oldParameterId < static_cast<int32_t>(pData->param.count),);

        pData->param.data[oldParameterId].mappedControlIndex = CONTROL_INDEX_NONE;
        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED,
                                pData->id,
                                oldParameterId,
                                CONTROL_INDEX_NONE,
                                0, 0.0f, nullptr);
    }

    // mapping new parameter to CV
    if (index == CONTROL_INDEX_CV)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->event.cvSourcePorts != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(paramData.type == PARAMETER_INPUT,);
        CARLA_SAFE_ASSERT_RETURN(paramData.hints & PARAMETER_CAN_BE_CV_CONTROLLED,);

        CarlaEngineCVPort* const cvPort =
            (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, strBuf, true, parameterId);
        cvPort->setRange(paramData.mappedMinimum, paramData.mappedMaximum);
        pData->event.cvSourcePorts->addCVSource(cvPort, parameterId, reconfigureNow);
    }
    // unmapping from CV
    else if (paramData.mappedControlIndex == CONTROL_INDEX_CV)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->event.cvSourcePorts != nullptr,);

        CARLA_SAFE_ASSERT(pData->client->removePort(kEnginePortTypeCV, strBuf, true));
        CARLA_SAFE_ASSERT(pData->event.cvSourcePorts->removeCVSource(parameterId));
    }
    // mapping to something new
    else if (paramData.mappedControlIndex == CONTROL_INDEX_NONE)
    {
        // when doing MIDI CC mapping, ensure ranges are within bounds
        if (paramData.mappedMinimum < paramRanges.min || paramData.mappedMaximum > paramRanges.max)
            setParameterMappedRange(parameterId,
                                    std::max(paramData.mappedMinimum, paramRanges.min),
                                    std::min(paramData.mappedMaximum, paramRanges.max),
                                    true, true);
    }
#endif

    paramData.mappedControlIndex = index;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (index == CONTROL_INDEX_MIDI_LEARN)
        pData->midiLearnParameterIndex = static_cast<int32_t>(parameterId);
    else
        pData->midiLearnParameterIndex = -1;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED,
                            pData->id,
                            static_cast<int>(parameterId),
                            index,
                            0, 0.0f, nullptr);
#else
    return;
    // unused
    (void)reconfigureNow;
#endif
}

void CarlaPlugin::setParameterMappedRange(const uint32_t parameterId, const float minimum, const float maximum,
                                          const bool sendOsc, const bool sendCallback) noexcept
{
    if (pData->engineBridged) {
        CARLA_SAFE_ASSERT_RETURN(!sendOsc && !sendCallback,);
    } else {
        CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    }
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

    ParameterData& paramData(pData->param.data[parameterId]);

    if (carla_isEqual(paramData.mappedMinimum, minimum) &&
        carla_isEqual(paramData.mappedMaximum, maximum) &&
        (paramData.hints & PARAMETER_MAPPED_RANGES_SET) != 0x0)
        return;

    if (paramData.mappedControlIndex != CONTROL_INDEX_NONE && paramData.mappedControlIndex != CONTROL_INDEX_CV)
    {
        const ParameterRanges& paramRanges(pData->param.ranges[parameterId]);
        CARLA_SAFE_ASSERT_RETURN(minimum >= paramRanges.min,);
        CARLA_SAFE_ASSERT_RETURN(maximum <= paramRanges.max,);
    }

    paramData.hints |= PARAMETER_MAPPED_RANGES_SET;
    paramData.mappedMinimum = minimum;
    paramData.mappedMaximum = maximum;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->event.cvSourcePorts != nullptr && paramData.mappedControlIndex == CONTROL_INDEX_CV)
        pData->event.cvSourcePorts->setCVSourceRange(parameterId, minimum, maximum);

    char strBuf[STR_MAX+1];
    carla_zeroChars(strBuf, STR_MAX+1);
    std::snprintf(strBuf, STR_MAX, "%.12g:%.12g", static_cast<double>(minimum), static_cast<double>(maximum));

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED,
                            pData->id,
                            static_cast<int>(parameterId),
                            0, 0, 0.0f,
                            strBuf);
#endif
}

void CarlaPlugin::setCustomData(const char* const type, const char* const key, const char* const value, const bool)
{
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

    // Ignore some keys
    if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) == 0)
    {
        const PluginType ptype = getType();

        if ((ptype == PLUGIN_INTERNAL && std::strncmp(key, "CarlaAlternateFile", 18) == 0) ||
            (ptype == PLUGIN_DSSI     && std::strcmp (key, "guiVisible") == 0) ||
            (ptype == PLUGIN_LV2      && std::strncmp(key, "OSC:", 4) == 0))
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

void CarlaPlugin::setProgram(const int32_t index,
                             const bool sendGui, const bool sendOsc, const bool sendCallback, const bool) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);

    pData->prog.current = index;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_PROGRAM_CHANGED,
                            pData->id,
                            index,
                            0, 0, 0.0f, nullptr);

    // Change default parameter values
    if (index >= 0)
    {
        if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
            uiProgramChange(static_cast<uint32_t>(index));

        switch (getType())
        {
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
            break;

        default:
            pData->updateParameterValues(this, sendCallback, sendOsc, true);
            break;
        }
    }
}

void CarlaPlugin::setMidiProgram(const int32_t index,
                                 const bool sendGui, const bool sendOsc, const bool sendCallback, const bool) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

    pData->midiprog.current = index;

    pData->engine->callback(sendCallback, sendOsc,
                            ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED,
                            pData->id,
                            index,
                            0, 0, 0.0f, nullptr);

    // Change default parameter values
    if (index >= 0)
    {
        if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
            uiMidiProgramChange(static_cast<uint32_t>(index));

        switch (getType())
        {
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
            break;

        default:
            pData->updateParameterValues(this, sendCallback, sendOsc, true);
            break;
        }
    }
}

void CarlaPlugin::setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    for (uint32_t i=0; i < pData->midiprog.count; ++i)
    {
        if (pData->midiprog.data[i].bank == bank && pData->midiprog.data[i].program == program)
            return setMidiProgram(static_cast<int32_t>(i), sendGui, sendOsc, sendCallback);
    }
}

void CarlaPlugin::setProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(uindex < pData->prog.count,);

    const int32_t index = static_cast<int32_t>(uindex);
    pData->prog.current = index;

    // Change default parameter values
    switch (getType())
    {
    case PLUGIN_SF2:
    case PLUGIN_SFZ:
        break;

    default:
        pData->updateDefaultParameterValues(this);
        break;
    }

    pData->postponeProgramChangeRtEvent(sendCallbackLater, uindex);
}

void CarlaPlugin::setMidiProgramRT(const uint32_t uindex, const bool sendCallbackLater) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(uindex < pData->midiprog.count,);

    const int32_t index = static_cast<int32_t>(uindex);
    pData->midiprog.current = index;

    // Change default parameter values
    switch (getType())
    {
    case PLUGIN_SF2:
    case PLUGIN_SFZ:
        break;

    default:
        pData->updateDefaultParameterValues(this);
        break;
    }

    pData->postponeMidiProgramChangeRtEvent(sendCallbackLater, uindex);
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

void CarlaPlugin::bufferSizeChanged(const uint32_t newBufferSize)
{
   #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    delete[] pData->postProc.extraBuffer;
    pData->postProc.extraBuffer = new float[newBufferSize];
   #else
    // unused
    (void)newBufferSize;
   #endif
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

    const bool hasUI = pData->hints & PLUGIN_HAS_CUSTOM_UI;
    const bool needsUiMainThread = pData->hints & PLUGIN_NEEDS_UI_MAIN_THREAD;
    const uint32_t latency = getLatencyInFrames();

    if (pData->latency.frames != latency)
    {
        carla_stdout("latency changed to %i samples", latency);

        const ScopedSingleProcessLocker sspl(this, true);

        pData->client->setLatency(latency);
#ifndef BUILD_BRIDGE
        pData->latency.recreateBuffers(pData->latency.channels, latency);
#else
        pData->latency.frames = latency;
#endif
    }

    ProtectedData::PostRtEvents::Access rtEvents(pData->postRtEvents);

    if (rtEvents.isEmpty())
        return;

    for (RtLinkedList<PluginPostRtEvent>::Itenerator it = rtEvents.getDataIterator(); it.valid(); it.next())
    {
        const PluginPostRtEvent& event(it.getValue(kPluginPostRtEventFallback));
        CARLA_SAFE_ASSERT_CONTINUE(event.type != kPluginPostRtEventNull);

        switch (event.type)
        {
        case kPluginPostRtEventNull: {
        } break;

        case kPluginPostRtEventParameterChange: {
            // Update UI
            if (event.parameter.index >= 0 && hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiParameterChange(static_cast<uint32_t>(event.parameter.index), event.parameter.value);
            }

            if (event.sendCallback)
            {
                // Update Host
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                                        pData->id,
                                        event.parameter.index,
                                        0, 0,
                                        event.parameter.value,
                                        nullptr);
            }
        } break;

        case kPluginPostRtEventProgramChange: {
            // Update UI
            if (hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiProgramChange(event.program.index);
            }

            // Update param values
            for (uint32_t j=0; j < pData->param.count; ++j)
            {
                const float paramDefault(pData->param.ranges[j].def);
                const float paramValue(getParameterValue(j));

                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                                        pData->id,
                                        static_cast<int>(j),
                                        0, 0,
                                        paramValue,
                                        nullptr);
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED,
                                        pData->id,
                                        static_cast<int>(j),
                                        0, 0,
                                        paramDefault,
                                        nullptr);
            }

            if (event.sendCallback)
            {
                // Update Host
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PROGRAM_CHANGED,
                                        pData->id,
                                        static_cast<int>(event.program.index),
                                        0, 0, 0.0f, nullptr);
            }
        } break;

        case kPluginPostRtEventMidiProgramChange: {
            // Update UI
            if (hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiMidiProgramChange(event.program.index);
            }

            // Update param values
            for (uint32_t j=0; j < pData->param.count; ++j)
            {
                const float paramDefault(pData->param.ranges[j].def);
                const float paramValue(getParameterValue(j));

                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED,
                                        pData->id,
                                        static_cast<int>(j),
                                        0, 0,
                                        paramValue,
                                        nullptr);
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED,
                                        pData->id,
                                        static_cast<int>(j),
                                        0, 0,
                                        paramDefault,
                                        nullptr);
            }

            if (event.sendCallback)
            {
                // Update Host
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED,
                                        pData->id,
                                        static_cast<int>(event.program.index),
                                        0, 0, 0.0f, nullptr);
            }
        } break;

        case kPluginPostRtEventNoteOn: {
            CARLA_SAFE_ASSERT_BREAK(event.note.channel  < MAX_MIDI_CHANNELS);
            CARLA_SAFE_ASSERT_BREAK(event.note.note     < MAX_MIDI_NOTE);
            CARLA_SAFE_ASSERT_BREAK(event.note.velocity < MAX_MIDI_VALUE);

            // Update UI
            if (hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiNoteOn(event.note.channel, event.note.note, event.note.velocity);
            }

            if (event.sendCallback)
            {
                // Update Host
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_NOTE_ON,
                                        pData->id,
                                        static_cast<int>(event.note.channel),
                                        static_cast<int>(event.note.note),
                                        static_cast<int>(event.note.velocity),
                                        0.0f, nullptr);
            }
        } break;

        case kPluginPostRtEventNoteOff: {
            CARLA_SAFE_ASSERT_BREAK(event.note.channel  < MAX_MIDI_CHANNELS);
            CARLA_SAFE_ASSERT_BREAK(event.note.note     < MAX_MIDI_NOTE);

            // Update UI
            if (hasUI)
            {
                if (needsUiMainThread)
                    pData->postUiEvents.append(event);
                else
                    uiNoteOff(event.note.channel, event.note.note);
            }

            if (event.sendCallback)
            {
                // Update Host
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_NOTE_OFF,
                                        pData->id,
                                        static_cast<int>(event.note.channel),
                                        static_cast<int>(event.note.note),
                                        0, 0.0f, nullptr);
            }
        } break;

        case kPluginPostRtEventMidiLearn: {
            CARLA_SAFE_ASSERT_BREAK(event.midiLearn.cc      < MAX_MIDI_VALUE);
            CARLA_SAFE_ASSERT_BREAK(event.midiLearn.channel < MAX_MIDI_CHANNELS);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (event.sendCallback)
            {
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED,
                                        pData->id,
                                        static_cast<int>(event.midiLearn.parameter),
                                        static_cast<int>(event.midiLearn.cc),
                                        0, 0.0f, nullptr);

                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED,
                                        pData->id,
                                        static_cast<int>(event.midiLearn.parameter),
                                        static_cast<int>(event.midiLearn.channel),
                                        0, 0.0f, nullptr);
            }
#endif
        } break;
        }
    }
}

bool CarlaPlugin::tryLock(const bool forcedOffline) noexcept
{
    if (forcedOffline)
    {
#ifndef STOAT_TEST_BUILD
        pData->masterMutex.lock();
        return true;
#endif
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

// -------------------------------------------------------------------
// OSC stuff

// FIXME
void CarlaPlugin::handleOscMessage(const char*, int, const void*, const char*, void*)
{
    // do nothing
}

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

    pData->engine->callback(sendCallback, sendOsc,
                            (velo > 0) ? ENGINE_CALLBACK_NOTE_ON : ENGINE_CALLBACK_NOTE_OFF,
                            pData->id,
                            channel,
                            note,
                            velo,
                            0.0f, nullptr);
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
void CarlaPlugin::postponeRtAllNotesOff()
{
    if (pData->ctrlChannel < 0 || pData->ctrlChannel >= MAX_MIDI_CHANNELS)
        return;

    PluginPostRtEvent postEvent = { kPluginPostRtEventNoteOff, true, {} };
    postEvent.note.channel = static_cast<uint8_t>(pData->ctrlChannel);

    for (uint8_t i=0; i < MAX_MIDI_NOTE; ++i)
    {
        postEvent.note.note = i;
        pData->postRtEvents.appendRT(postEvent);
    }
}
#endif

// -------------------------------------------------------------------
// UI Stuff

void CarlaPlugin::setCustomUITitle(const char* const title) noexcept
{
    pData->uiTitle = title;
}

void CarlaPlugin::showCustomUI(const bool yesNo)
{
    if (yesNo) {
        CARLA_SAFE_ASSERT(false);
    }
}

void* CarlaPlugin::embedCustomUI(void*)
{
    return nullptr;
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
            case kPluginPostRtEventMidiLearn:
                break;

            case kPluginPostRtEventParameterChange:
                uiParameterChange(static_cast<uint32_t>(event.parameter.index), event.parameter.value);
                break;

            case kPluginPostRtEventProgramChange:
                uiProgramChange(event.program.index);
                break;

            case kPluginPostRtEventMidiProgramChange:
                uiMidiProgramChange(event.program.index);
                break;

            case kPluginPostRtEventNoteOn:
                uiNoteOn(event.note.channel, event.note.note, event.note.velocity);
                break;

            case kPluginPostRtEventNoteOff:
                uiNoteOff(event.note.channel, event.note.note);
                break;
            }
        }

        pData->postUiEvents.data.clear();
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->transientTryCounter == 0)
        return;
    if (++pData->transientTryCounter % 10 != 0)
        return;
    if (pData->transientTryCounter >= 200)
        return;

    carla_stdout("Trying to get window...");

    String uiTitle;

    if (pData->uiTitle.isNotEmpty())
    {
        uiTitle = pData->uiTitle;
    }
    else
    {
        uiTitle  = pData->name;
        uiTitle += " (GUI)";
    }

    if (CarlaPluginUI::tryTransientWinIdMatch(getUiBridgeProcessId(), uiTitle,
                                              pData->engine->getOptions().frontendWinId, pData->transientFirstTry))
    {
        pData->transientTryCounter = 0;
        pData->transientFirstTry = false;
    }
#endif
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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
void CarlaPlugin::checkForMidiLearn(EngineEvent& event) noexcept
{
    if (pData->midiLearnParameterIndex < 0)
        return;
    if (event.ctrl.param == MIDI_CONTROL_BANK_SELECT || event.ctrl.param == MIDI_CONTROL_BANK_SELECT__LSB)
        return;
    if (event.ctrl.param >= MAX_MIDI_CONTROL)
        return;

    const uint32_t parameterId = static_cast<uint32_t>(pData->midiLearnParameterIndex);
    CARLA_SAFE_ASSERT_UINT2_RETURN(parameterId < pData->param.count, parameterId, pData->param.count,);

    ParameterData& paramData(pData->param.data[parameterId]);
    CARLA_SAFE_ASSERT_INT_RETURN(paramData.mappedControlIndex == CONTROL_INDEX_MIDI_LEARN,
                                 paramData.mappedControlIndex,);

    event.ctrl.handled = true;
    paramData.mappedControlIndex = static_cast<int16_t>(event.ctrl.param);
    paramData.midiChannel = event.channel;

    pData->postponeMidiLearnRtEvent(true, parameterId, static_cast<uint8_t>(event.ctrl.param), event.channel);
    pData->midiLearnParameterIndex = -1;
}
#endif

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

void CarlaPlugin::cloneLV2Files(const CarlaPlugin&)
{
    carla_stderr2("Warning: cloneLV2Files() called for non-implemented type");
}

void CarlaPlugin::restoreLV2State(const bool temporary) noexcept
{
    carla_stderr2("Warning: restoreLV2State(%s) called for non-implemented type", bool2str(temporary));
}

void CarlaPlugin::prepareForDeletion() noexcept
{
    carla_debug("CarlaPlugin::prepareForDeletion");

    const CarlaMutexLocker cml(pData->masterMutex);

    pData->client->deactivate(true);
}

void CarlaPlugin::waitForBridgeSaveSignal() noexcept
{
}

// -------------------------------------------------------------------
// Scoped Disabler

CarlaPlugin::ScopedDisabler::ScopedDisabler(CarlaPlugin* const plugin) noexcept
    : fPlugin(plugin),
      fWasEnabled(false)
{
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin->pData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(plugin->pData->client != nullptr,);
    carla_debug("CarlaPlugin::ScopedDisabler(%p)", plugin);

    plugin->pData->masterMutex.lock();

    if (plugin->pData->enabled)
    {
        fWasEnabled = true;
        plugin->pData->enabled = false;

        if (plugin->pData->client->isActive())
            plugin->pData->client->deactivate(false);
    }
}

CarlaPlugin::ScopedDisabler::~ScopedDisabler() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fPlugin->pData != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(fPlugin->pData->client != nullptr,);
    carla_debug("CarlaPlugin::~ScopedDisabler()");

    if (fWasEnabled)
    {
        fPlugin->pData->enabled = true;
        fPlugin->pData->client->activate();
    }

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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (fPlugin->pData->singleMutex.wasTryLockCalled())
        fPlugin->pData->needsReset = true;
#endif

    fPlugin->pData->singleMutex.unlock();
}

// -------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
