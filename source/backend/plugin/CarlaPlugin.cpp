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
#include "CarlaMathUtils.hpp"
#include "CarlaPluginUi.hpp"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtXml/QDomNode>

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------
// Fallback data

static const ParameterData   kParameterDataNull   = { PARAMETER_UNKNOWN, 0x0, PARAMETER_NULL, -1, -1, 0 };
static const ParameterRanges kParameterRangesNull = { 0.0f, 0.0f, 1.0f, 0.01f, 0.0001f, 0.1f };
static const MidiProgramData kMidiProgramDataNull = { 0, 0, nullptr };
static const CustomData      kCustomDataNull      = { nullptr, nullptr, nullptr };

static bool gIsLoadingProject = false;

// -------------------------------------------------------------------
// ParamSymbol struct, needed for CarlaPlugin::loadSaveState()

struct ParamSymbol {
    int32_t index;
    const char* symbol;

    ParamSymbol(uint32_t i, const char* s)
        : index(static_cast<int32_t>(i)),
          symbol(carla_strdup(s)) {}

    ~ParamSymbol()
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

// -----------------------------------------------------------------------

void CarlaPluginProtectedData::tryTransient()
{
    if (engine->getOptions().frontendWinId != 0)
        transientTryCounter = 1;
}

// -----------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newFileGIG(const Initializer& init, const bool use16Outs)
{
    carla_debug("CarlaPlugin::newFileGIG({%p, \"%s\", \"%s\", \"%s\"}, %s)", init.engine, init.filename, init.name, init.label, bool2str(use16Outs));
#ifdef WANT_LINUXSAMPLER
    return newLinuxSampler(init, "GIG", use16Outs);
#else
    init.engine->setLastError("GIG support not available");
    return nullptr;

    // unused
    (void)use16Outs;
#endif
}

CarlaPlugin* CarlaPlugin::newFileSF2(const Initializer& init, const bool use16Outs)
{
    carla_debug("CarlaPlugin::newFileSF2({%p, \"%s\", \"%s\", \"%s\"}, %s)", init.engine, init.filename, init.name, init.label, bool2str(use16Outs));
#if defined(WANT_FLUIDSYNTH)
    return newFluidSynth(init, use16Outs);
#elif defined(WANT_LINUXSAMPLER)
    return newLinuxSampler(init, "SF2", use16Outs);
#else
    init.engine->setLastError("SF2 support not available");
    return nullptr;

    // unused
    (void)use16Outs;
#endif
}

CarlaPlugin* CarlaPlugin::newFileSFZ(const Initializer& init)
{
    carla_debug("CarlaPlugin::newFileSFZ({%p, \"%s\", \"%s\", \"%s\"})", init.engine, init.filename, init.name, init.label);
#ifdef WANT_LINUXSAMPLER
    return newLinuxSampler(init, "SFZ", false);
#else
    init.engine->setLastError("SFZ support not available");
    return nullptr;
#endif
}

// -------------------------------------------------------------------
// Constructor and destructor

CarlaPlugin::CarlaPlugin(CarlaEngine* const engine, const unsigned int id)
    : pData(new CarlaPluginProtectedData(engine, id, this))
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

unsigned int CarlaPlugin::getId() const noexcept
{
    return pData->id;
}

unsigned int CarlaPlugin::getHints() const noexcept
{
    return pData->hints;
}

unsigned int CarlaPlugin::getOptionsEnabled() const noexcept
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
    PluginCategory category = PLUGIN_CATEGORY_NONE;

    try {
        category = getPluginCategoryFromName(pData->name);
    } catch(...) {}

    return category;
}

int64_t CarlaPlugin::getUniqueId() const noexcept
{
    return 0;
}

uint32_t CarlaPlugin::getLatencyInFrames() const noexcept
{
    return pData->latency;
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
    return pData->custom.getAt(index);
}

int32_t CarlaPlugin::getChunkData(void** const dataPtr) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);
    CARLA_SAFE_ASSERT(false); // this should never happen
    return 0;
}

// -------------------------------------------------------------------
// Information (per-plugin data)

unsigned int CarlaPlugin::getOptionsAvailable() const noexcept
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

void CarlaPlugin::getParameterText(const uint32_t parameterId, const float, char* const strBuf) const noexcept
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

const SaveState& CarlaPlugin::getSaveState()
{
    pData->saveState.reset();
    prepareForSave();

    char strBuf[STR_MAX+1];

    // ---------------------------------------------------------------
    // Basic info

    getLabel(strBuf);

    pData->saveState.type     = carla_strdup(getPluginTypeAsString(getType()));
    pData->saveState.name     = carla_strdup(pData->name);
    pData->saveState.label    = carla_strdup(strBuf);
    pData->saveState.binary   = carla_strdup(pData->filename);
    pData->saveState.uniqueId = getUniqueId();

    // ---------------------------------------------------------------
    // Internals

    pData->saveState.active = pData->active;

#ifndef BUILD_BRIDGE
    pData->saveState.dryWet       = pData->postProc.dryWet;
    pData->saveState.volume       = pData->postProc.volume;
    pData->saveState.balanceLeft  = pData->postProc.balanceLeft;
    pData->saveState.balanceRight = pData->postProc.balanceRight;
    pData->saveState.panning      = pData->postProc.panning;
    pData->saveState.ctrlChannel  = pData->ctrlChannel;
#endif

    // ---------------------------------------------------------------
    // Chunk

    if (pData->options & PLUGIN_OPTION_USE_CHUNKS)
    {
        void* data = nullptr;
        const int32_t dataSize(getChunkData(&data));

        if (data != nullptr && dataSize > 0)
        {
            pData->saveState.chunk = carla_strdup(QByteArray((char*)data, dataSize).toBase64().constData());

            // Don't save anything else if using chunks
            return pData->saveState;
        }
    }

    // ---------------------------------------------------------------
    // Current Program

    if (pData->prog.current >= 0 && getType() != PLUGIN_LV2)
    {
        pData->saveState.currentProgramIndex = pData->prog.current;
        pData->saveState.currentProgramName  = carla_strdup(pData->prog.names[pData->prog.current]);
    }

    // ---------------------------------------------------------------
    // Current MIDI Program

    if (pData->midiprog.current >= 0 && getType() != PLUGIN_LV2)
    {
        const MidiProgramData& mpData(pData->midiprog.getCurrent());

        pData->saveState.currentMidiBank    = static_cast<int32_t>(mpData.bank);
        pData->saveState.currentMidiProgram = static_cast<int32_t>(mpData.program);
    }

    // ---------------------------------------------------------------
    // Parameters

    const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));

    for (uint32_t i=0; i < pData->param.count; ++i)
    {
        const ParameterData& paramData(pData->param.data[i]);

        if ((paramData.hints & PARAMETER_IS_ENABLED) == 0)
            continue;

        StateParameter* const stateParameter(new StateParameter());

        stateParameter->isInput = (paramData.type == PARAMETER_INPUT);
        stateParameter->index   = paramData.index;
        stateParameter->midiCC  = paramData.midiCC;
        stateParameter->midiChannel = paramData.midiChannel;

        getParameterName(i, strBuf);
        stateParameter->name = carla_strdup(strBuf);

        getParameterSymbol(i, strBuf);
        stateParameter->symbol = carla_strdup(strBuf);;

        stateParameter->value = getParameterValue(i);

        if (paramData.hints & PARAMETER_USES_SAMPLERATE)
            stateParameter->value /= sampleRate;

        pData->saveState.parameters.append(stateParameter);
    }

    // ---------------------------------------------------------------
    // Custom Data

    for (LinkedList<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
    {
        const CustomData& cData(it.getValue());

        StateCustomData* stateCustomData(new StateCustomData());

        stateCustomData->type  = carla_strdup(cData.type);
        stateCustomData->key   = carla_strdup(cData.key);
        stateCustomData->value = carla_strdup(cData.value);

        pData->saveState.customData.append(stateCustomData);
    }

    return pData->saveState;
}

void CarlaPlugin::loadSaveState(const SaveState& saveState)
{
    char strBuf[STR_MAX+1];
    const bool usesMultiProgs(pData->extraHints & PLUGIN_EXTRA_HINT_USES_MULTI_PROGS);

    gIsLoadingProject = true;
    ScopedValueSetter<bool>(gIsLoadingProject, false);

    // ---------------------------------------------------------------
    // Part 1 - PRE-set custom data (only that which reload programs)

    for (LinkedList<StateCustomData*>::Itenerator it = saveState.customData.begin(); it.valid(); it.next())
    {
        const StateCustomData* const stateCustomData(it.getValue());
        const char* const key(stateCustomData->key);

        bool wantData = false;

        if (getType() == PLUGIN_DSSI && (std::strcmp(key, "reloadprograms") == 0 || std::strcmp(key, "load") == 0 || std::strncmp(key, "patches", 7) == 0))
            wantData = true;
        else if (usesMultiProgs && std::strcmp(key, "midiPrograms") == 0)
            wantData = true;

        if (wantData)
            setCustomData(stateCustomData->type, stateCustomData->key, stateCustomData->value, true);
    }

    // ---------------------------------------------------------------
    // Part 2 - set program

    if (saveState.currentProgramIndex >= 0 && saveState.currentProgramName != nullptr)
    {
        int32_t programId = -1;

        // index < count
        if (saveState.currentProgramIndex < static_cast<int32_t>(pData->prog.count))
        {
            programId = saveState.currentProgramIndex;
        }
        // index not valid, try to find by name
        else
        {
            for (uint32_t i=0; i < pData->prog.count; ++i)
            {
                strBuf[0] = '\0';
                getProgramName(i, strBuf);

                if (strBuf[0] != '\0' && std::strcmp(saveState.currentProgramName, strBuf) == 0)
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

    if (saveState.currentMidiBank >= 0 && saveState.currentMidiProgram >= 0 && ! usesMultiProgs)
        setMidiProgramById(static_cast<uint32_t>(saveState.currentMidiBank), static_cast<uint32_t>(saveState.currentMidiProgram), true, true, true);

    // ---------------------------------------------------------------
    // Part 4a - get plugin parameter symbols

    LinkedList<ParamSymbol*> paramSymbols;

    if (getType() == PLUGIN_LADSPA || getType() == PLUGIN_LV2)
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

    for (LinkedList<StateParameter*>::Itenerator it = saveState.parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(it.getValue());

        int32_t index = -1;

        if (getType() == PLUGIN_LADSPA)
        {
            // Try to set by symbol, otherwise use index
            if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            {
                for (LinkedList<ParamSymbol*>::Itenerator it2 = paramSymbols.begin(); it2.valid(); it2.next())
                {
                    ParamSymbol* const paramSymbol(it2.getValue());

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
        else if (getType() == PLUGIN_LV2)
        {
            // Symbol only
            if (stateParameter->symbol != nullptr && stateParameter->symbol[0] != '\0')
            {
                for (LinkedList<ParamSymbol*>::Itenerator it2 = paramSymbols.begin(); it2.valid(); it2.next())
                {
                    ParamSymbol* const paramSymbol(it2.getValue());

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

            if (stateParameter->isInput)
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

    for (LinkedList<ParamSymbol*>::Itenerator it = paramSymbols.begin(); it.valid(); it.next())
    {
        ParamSymbol* const paramSymbol(it.getValue());
        delete paramSymbol;
    }

    paramSymbols.clear();

    // ---------------------------------------------------------------
    // Part 5 - set custom data

    for (LinkedList<StateCustomData*>::Itenerator it = saveState.customData.begin(); it.valid(); it.next())
    {
        const StateCustomData* const stateCustomData(it.getValue());
        const char* const key(stateCustomData->key);

        if (getType() == PLUGIN_DSSI && (std::strcmp(key, "reloadprograms") == 0 || std::strcmp(key, "load") == 0 || std::strncmp(key, "patches", 7) == 0))
            continue;
        if (usesMultiProgs && std::strcmp(key, "midiPrograms") == 0)
            continue;

        setCustomData(stateCustomData->type, stateCustomData->key, stateCustomData->value, true);
    }

    // ---------------------------------------------------------------
    // Part 5x - set lv2 state

    if (getType() == PLUGIN_LV2 && pData->custom.count() > 0)
        setCustomData(CUSTOM_DATA_TYPE_STRING, "CarlaLoadLv2StateNow", "true", true);

    // ---------------------------------------------------------------
    // Part 6 - set chunk

    if (saveState.chunk != nullptr && (pData->options & PLUGIN_OPTION_USE_CHUNKS) != 0)
        setChunkData(saveState.chunk);

    // ---------------------------------------------------------------
    // Part 6 - set internal stuff

#ifndef BUILD_BRIDGE
    setDryWet(saveState.dryWet, true, true);
    setVolume(saveState.volume, true, true);
    setBalanceLeft(saveState.balanceLeft, true, true);
    setBalanceRight(saveState.balanceRight, true, true);
    setPanning(saveState.panning, true, true);
    setCtrlChannel(saveState.ctrlChannel, true, true);
#endif

    setActive(saveState.active, true, true);
}

bool CarlaPlugin::saveStateToFile(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("CarlaPlugin::saveStateToFile(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QString content;
    fillXmlStringFromSaveState(content, getSaveState());

    QTextStream out(&file);
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PRESET>\n";
    out << "<CARLA-PRESET VERSION='2.0'>\n";
    out << content;
    out << "</CARLA-PRESET>\n";

    file.close();
    return true;
}

bool CarlaPlugin::loadStateFromFile(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("CarlaPlugin::loadStateFromFile(\"%s\")", filename);

    QFile file(filename);

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QDomDocument xml;
    xml.setContent(file.readAll());
    file.close();

    QDomNode xmlNode(xml.documentElement());

    if (xmlNode.toElement().tagName().compare("carla-preset", Qt::CaseInsensitive) == 0)
    {
        pData->engine->setLastError("Not a valid Carla preset file");
        return false;
    }

    pData->saveState.reset();
    fillSaveStateFromXmlNode(pData->saveState, xmlNode);
    loadSaveState(pData->saveState);

    return true;
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setId(const unsigned int newId) noexcept
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

void CarlaPlugin::setOption(const unsigned int option, const bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(getOptionsAvailable() & option,);

    if (yesNo)
        pData->options |= option;
    else
        pData->options &= ~option;

    pData->saveSetting(option, yesNo);
}

void CarlaPlugin::setEnabled(const bool yesNo) noexcept
{
    if (pData->enabled == yesNo)
        return;

    pData->enabled = yesNo;

    pData->masterMutex.lock();
    pData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Set data (internal stuff)

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

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_ACTIVE, value);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_ACTIVE, 0, value, nullptr);
#else
    return;

    // unused
    (void)sendOsc;
    (void)sendCallback;
#endif
}

#ifndef BUILD_BRIDGE
void CarlaPlugin::setDryWet(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(0.0f, 1.0f, value));

    if (pData->postProc.dryWet == fixedValue)
        return;

    pData->postProc.dryWet = fixedValue;

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_DRYWET, fixedValue);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_DRYWET, 0, fixedValue, nullptr);
}

void CarlaPlugin::setVolume(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= 0.0f && value <= 1.27f);

    const float fixedValue(carla_fixValue<float>(0.0f, 1.27f, value));

    if (pData->postProc.volume == fixedValue)
        return;

    pData->postProc.volume = fixedValue;

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_VOLUME, fixedValue);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_VOLUME, 0, fixedValue, nullptr);
}

void CarlaPlugin::setBalanceLeft(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(-1.0f, 1.0f, value));

    if (pData->postProc.balanceLeft == fixedValue)
        return;

    pData->postProc.balanceLeft = fixedValue;

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_BALANCE_LEFT, fixedValue);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_BALANCE_LEFT, 0, fixedValue, nullptr);
}

void CarlaPlugin::setBalanceRight(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(-1.0f, 1.0f, value));

    if (pData->postProc.balanceRight == fixedValue)
        return;

    pData->postProc.balanceRight = fixedValue;

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_BALANCE_RIGHT, fixedValue);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_BALANCE_RIGHT, 0, fixedValue, nullptr);
}

void CarlaPlugin::setPanning(const float value, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(-1.0f, 1.0f, value));

    if (pData->postProc.panning == fixedValue)
        return;

    pData->postProc.panning = fixedValue;

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_PANNING, fixedValue);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_PANNING, 0, fixedValue, nullptr);
}
#endif

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
    const float ctrlf(channel);

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, PARAMETER_CTRL_CHANNEL, ctrlf);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, PARAMETER_CTRL_CHANNEL, 0, ctrlf, nullptr);

    if (pData->hints & PLUGIN_IS_BRIDGE)
        osc_send_control(pData->osc.data, PARAMETER_CTRL_CHANNEL, ctrlf);
#else
    return;

    // unused
    (void)sendOsc;
    (void)sendCallback;
#endif
}

// -------------------------------------------------------------------
// Set data (plugin-specific stuff)

void CarlaPlugin::setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
#ifdef BUILD_BRIDGE
    if (! gIsLoadingProject)
    {
        CARLA_ASSERT(! sendGui); // this should never happen
    }
#endif

#ifndef BUILD_BRIDGE
    if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
        uiParameterChange(parameterId, value);

    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(parameterId), value);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(parameterId), 0, value, nullptr);

#ifdef BUILD_BRIDGE
    return;

    // unused
    (void)sendGui;
    (void)sendOsc;
#endif
}

void CarlaPlugin::setParameterValueByRealIndex(const int32_t rindex, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(rindex > PARAMETER_MAX && rindex != PARAMETER_NULL,);

    if (rindex == PARAMETER_ACTIVE)
        return setActive((value > 0.0f), sendOsc, sendCallback);
    if (rindex == PARAMETER_CTRL_CHANNEL)
        return setCtrlChannel(int8_t(value), sendOsc, sendCallback);

#ifndef BUILD_BRIDGE
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
#endif

    for (uint32_t i=0; i < pData->param.count; ++i)
    {
        if (pData->param.data[i].rindex == rindex)
        {
            if (getParameterValue(i) != value)
                setParameterValue(i, value, sendGui, sendOsc, sendCallback);
            break;
        }
    }
}

void CarlaPlugin::setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

    pData->param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_midi_channel(pData->id, parameterId, channel);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, pData->id, static_cast<int>(parameterId), channel, 0.0f, nullptr);

    if (pData->hints & PLUGIN_IS_BRIDGE)
        {} // TODO
#else
    return;

    // unused
    (void)sendOsc;
    (void)sendCallback;
#endif
}

void CarlaPlugin::setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sendOsc || sendCallback,); // never call this from RT
    CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc <= 0x5F,);

    pData->param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
    if (sendOsc && pData->engine->isOscControlRegistered())
        pData->engine->oscSend_control_set_parameter_midi_cc(pData->id, parameterId, cc);

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED, pData->id, static_cast<int>(parameterId), cc, 0.0f, nullptr);

    if (pData->hints & PLUGIN_IS_BRIDGE)
        {} // TODO
#else
    return;

    // unused
    (void)sendOsc;
    (void)sendCallback;
#endif
}

void CarlaPlugin::setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
{
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
#ifdef BUILD_BRIDGE
    if (! gIsLoadingProject) {
        CARLA_SAFE_ASSERT_RETURN(! sendGui,); // this should never happen
    }
#else
    // unused
    (void)sendGui;
#endif

    bool saveData = true;

    if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) == 0)
    {
        // Ignore some keys
        if (std::strncmp(key, "OSC:", 4) == 0 || std::strncmp(key, "CarlaAlternateFile", 18) == 0 || std::strcmp(key, "guiVisible") == 0)
            saveData = false;
        //else if (std::strcmp(key, CARLA_BRIDGE_MSG_SAVE_NOW) == 0 || std::strcmp(key, CARLA_BRIDGE_MSG_SET_CHUNK) == 0 || std::strcmp(key, CARLA_BRIDGE_MSG_SET_CUSTOM) == 0)
        //    saveData = false;
    }

    if (! saveData)
        return;

    // Check if we already have this key
    for (LinkedList<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
    {
        CustomData& cData(it.getValue());

        CARLA_SAFE_ASSERT_CONTINUE(cData.type != nullptr && cData.type[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(cData.key != nullptr && cData.key[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(cData.value != nullptr);

        if (std::strcmp(cData.key, key) == 0)
        {
            if (cData.value != nullptr)
                delete[] cData.value;

            cData.value = carla_strdup(value);
            return;
        }
    }

    // Otherwise store it
    CustomData newData;
    newData.type  = carla_strdup(type);
    newData.key   = carla_strdup(key);
    newData.value = carla_strdup(value);
    pData->custom.append(newData);
}

void CarlaPlugin::setChunkData(const char* const stringData)
{
    CARLA_SAFE_ASSERT_RETURN(stringData != nullptr && stringData[0] != '\0',);
    CARLA_SAFE_ASSERT(false); // this should never happen
}

void CarlaPlugin::setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->prog.count),);
#ifdef BUILD_BRIDGE
    if (! gIsLoadingProject) {
        CARLA_ASSERT(! sendGui); // this should never happen
    }
#endif

    pData->prog.current = index;

#ifndef BUILD_BRIDGE
    const bool reallySendOsc(sendOsc && pData->engine->isOscControlRegistered());

    if (reallySendOsc)
        pData->engine->oscSend_control_set_current_program(pData->id, index);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_PROGRAM_CHANGED, pData->id, index, 0, 0.0f, nullptr);

    // Change default parameter values
    if (index >= 0)
    {
#ifndef BUILD_BRIDGE
        if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
            uiProgramChange(static_cast<uint32_t>(index));
#endif

        if (getType() == PLUGIN_FILE_CSD || getType() == PLUGIN_FILE_GIG || getType() == PLUGIN_FILE_SF2 || getType() == PLUGIN_FILE_SFZ)
            return;

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            const float value(pData->param.ranges[i].getFixedValue(getParameterValue(i)));

            pData->param.ranges[i].def = value;

#ifndef BUILD_BRIDGE
            if (reallySendOsc)
            {
                pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(i), value);
                pData->engine->oscSend_control_set_default_value(pData->id, i, value);
            }
#endif

            if (sendCallback)
            {
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(i), 0, value, nullptr);
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED, pData->id, static_cast<int>(i), 0, value, nullptr);
            }
        }
    }

#ifdef BUILD_BRIDGE
    return;

    // unused
    (void)sendGui;
    (void)sendOsc;
#endif
}

void CarlaPlugin::setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);
#ifdef BUILD_BRIDGE
    if (! gIsLoadingProject) {
        CARLA_ASSERT(! sendGui); // this should never happen
    }
#endif

    pData->midiprog.current = index;

#ifndef BUILD_BRIDGE
    const bool reallySendOsc(sendOsc && pData->engine->isOscControlRegistered());

    if (reallySendOsc)
        pData->engine->oscSend_control_set_current_midi_program(pData->id, index);
#endif

    if (sendCallback)
        pData->engine->callback(ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED, pData->id, index, 0, 0.0f, nullptr);

    if (index >= 0)
    {
#ifndef BUILD_BRIDGE
        if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
            uiMidiProgramChange(static_cast<uint32_t>(index));
#endif

        if (getType() == PLUGIN_FILE_CSD || getType() == PLUGIN_FILE_GIG || getType() == PLUGIN_FILE_SF2 || getType() == PLUGIN_FILE_SFZ)
            return;

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            const float value(pData->param.ranges[i].getFixedValue(getParameterValue(i)));

            pData->param.ranges[i].def = value;

#ifndef BUILD_BRIDGE
            if (reallySendOsc)
            {
                pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(i), value);
                pData->engine->oscSend_control_set_default_value(pData->id, i, value);
            }
#endif

            if (sendCallback)
            {
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(i), 0, value, nullptr);
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED, pData->id, static_cast<int>(i), 0, value, nullptr);
            }
        }
    }

#ifdef BUILD_BRIDGE
    return;

    // unused
    (void)sendGui;
    (void)sendOsc;
#endif
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
// Set ui stuff

void CarlaPlugin::idle()
{
    if (! pData->enabled)
        return;

    if (pData->hints & PLUGIN_NEEDS_SINGLE_THREAD)
    {
        // Process postponed events
        postRtEventsRun();

        // Update parameter outputs
        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (pData->param.data[i].type == PARAMETER_OUTPUT)
                uiParameterChange(i, getParameterValue(i));
        }
    }

    if (pData->transientTryCounter == 0)
        return;
    if (++pData->transientTryCounter % 10 != 0)
        return;
    if (pData->transientTryCounter >= 200)
        return;

    carla_stdout("Trying to get window...");

    QString uiTitle(QString("%1 (GUI)").arg(pData->name));
    if (CarlaPluginUi::tryTransientWinIdMatch(pData->osc.data.target != nullptr ? pData->osc.thread.getPid() : 0, uiTitle.toUtf8().constData(), pData->engine->getOptions().frontendWinId))
        pData->transientTryCounter = 0;
}

void CarlaPlugin::showCustomUI(const bool yesNo)
{
    CARLA_SAFE_ASSERT(false);
    return;

    // unused
    (void)yesNo;
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

void CarlaPlugin::initBuffers()
{
    pData->audioIn.initBuffers();
    pData->audioOut.initBuffers();
    pData->event.initBuffers();
}

void CarlaPlugin::clearBuffers()
{
    pData->clearBuffers();
}

// -------------------------------------------------------------------
// OSC stuff

void CarlaPlugin::registerToOscClient() noexcept
{
#ifdef BUILD_BRIDGE
    if (! pData->engine->isOscBridgeRegistered())
#else
    if (! pData->engine->isOscControlRegistered())
#endif
        return;


#ifndef BUILD_BRIDGE
    pData->engine->oscSend_control_add_plugin_start(pData->id, pData->name);
#endif

    // Base data
    {
        // TODO - clear buf
        char bufName[STR_MAX+1]  = { '\0' };
        char bufLabel[STR_MAX+1] = { '\0' };
        char bufMaker[STR_MAX+1] = { '\0' };
        char bufCopyright[STR_MAX+1] = { '\0' };
        getRealName(bufName);
        getLabel(bufLabel);
        getMaker(bufMaker);
        getCopyright(bufCopyright);

#ifdef BUILD_BRIDGE
        pData->engine->oscSend_bridge_plugin_info1(getCategory(), pData->hints, getUniqueId());
        pData->engine->oscSend_bridge_plugin_info2(bufName, bufLabel, bufMaker, bufCopyright);
#else
        pData->engine->oscSend_control_set_plugin_info1(pData->id, getType(), getCategory(), pData->hints, getUniqueId());
        pData->engine->oscSend_control_set_plugin_info2(pData->id, bufName, bufLabel, bufMaker, bufCopyright);
#endif
    }

    // Base count
    {
        uint32_t paramIns, paramOuts;
        getParameterCountInfo(paramIns, paramOuts);

#ifdef BUILD_BRIDGE
        pData->engine->oscSend_bridge_audio_count(getAudioInCount(), getAudioOutCount());
        pData->engine->oscSend_bridge_midi_count(getMidiInCount(), getMidiOutCount());
        pData->engine->oscSend_bridge_parameter_count(paramIns, paramOuts);
#else
        pData->engine->oscSend_control_set_audio_count(pData->id, getAudioInCount(), getAudioOutCount());
        pData->engine->oscSend_control_set_midi_count(pData->id, getMidiInCount(), getMidiOutCount());
        pData->engine->oscSend_control_set_parameter_count(pData->id, paramIns, paramOuts);
#endif
    }

    // Plugin Parameters
    if (pData->param.count > 0 && pData->param.count < pData->engine->getOptions().maxParameters)
    {
        char bufName[STR_MAX+1], bufUnit[STR_MAX+1];

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            carla_fill<char>(bufName, STR_MAX, '\0');
            carla_fill<char>(bufUnit, STR_MAX, '\0');

            getParameterName(i, bufName);
            getParameterUnit(i, bufUnit);

            const ParameterData& paramData(pData->param.data[i]);
            const ParameterRanges& paramRanges(pData->param.ranges[i]);

#ifdef BUILD_BRIDGE
            pData->engine->oscSend_bridge_parameter_data(i, paramData.rindex, paramData.type, paramData.hints, bufName, bufUnit);
            pData->engine->oscSend_bridge_parameter_ranges1(i, paramRanges.def, paramRanges.min, paramRanges.max);
            pData->engine->oscSend_bridge_parameter_ranges2(i, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            pData->engine->oscSend_bridge_parameter_value(i, getParameterValue(i));
            pData->engine->oscSend_bridge_parameter_midi_cc(i, paramData.midiCC);
            pData->engine->oscSend_bridge_parameter_midi_channel(i, paramData.midiChannel);
#else
            pData->engine->oscSend_control_set_parameter_data(pData->id, i, paramData.type, paramData.hints, bufName, bufUnit);
            pData->engine->oscSend_control_set_parameter_ranges1(pData->id, i, paramRanges.def, paramRanges.min, paramRanges.max);
            pData->engine->oscSend_control_set_parameter_ranges2(pData->id, i, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(i), getParameterValue(i));
            pData->engine->oscSend_control_set_parameter_midi_cc(pData->id, i, paramData.midiCC);
            pData->engine->oscSend_control_set_parameter_midi_channel(pData->id, i, paramData.midiChannel);
#endif
        }
    }

    // Programs
    if (pData->prog.count > 0)
    {
#ifdef BUILD_BRIDGE
        pData->engine->oscSend_bridge_program_count(pData->prog.count);

        for (uint32_t i=0; i < pData->prog.count; ++i)
            pData->engine->oscSend_bridge_program_name(i, pData->prog.names[i]);

        pData->engine->oscSend_bridge_current_program(pData->prog.current);
#else
        pData->engine->oscSend_control_set_program_count(pData->id, pData->prog.count);

        for (uint32_t i=0; i < pData->prog.count; ++i)
            pData->engine->oscSend_control_set_program_name(pData->id, i, pData->prog.names[i]);

        pData->engine->oscSend_control_set_current_program(pData->id, pData->prog.current);
#endif
    }

    // MIDI Programs
    if (pData->midiprog.count > 0)
    {
#ifdef BUILD_BRIDGE
        pData->engine->oscSend_bridge_midi_program_count(pData->midiprog.count);

        for (uint32_t i=0; i < pData->midiprog.count; ++i)
        {
            const MidiProgramData& mpData(pData->midiprog.data[i]);

            pData->engine->oscSend_bridge_midi_program_data(i, mpData.bank, mpData.program, mpData.name);
        }

        pData->engine->oscSend_bridge_current_midi_program(pData->midiprog.current);
#else
        pData->engine->oscSend_control_set_midi_program_count(pData->id, pData->midiprog.count);

        for (uint32_t i=0; i < pData->midiprog.count; ++i)
        {
            const MidiProgramData& mpData(pData->midiprog.data[i]);

            pData->engine->oscSend_control_set_midi_program_data(pData->id, i, mpData.bank, mpData.program, mpData.name);
        }

        pData->engine->oscSend_control_set_current_midi_program(pData->id, pData->midiprog.current);
#endif
    }

#ifndef BUILD_BRIDGE
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
#endif
}

void CarlaPlugin::updateOscData(const lo_address& source, const char* const url)
{
    // FIXME - remove debug prints later
    carla_stdout("CarlaPlugin::updateOscData(%p, \"%s\")", source, url);

    pData->osc.data.free();

    const int proto = lo_address_get_protocol(source);

    {
        const char* host = lo_address_get_hostname(source);
        const char* port = lo_address_get_port(source);
        pData->osc.data.source = lo_address_new_with_proto(proto, host, port);

        carla_stdout("CarlaPlugin::updateOscData() - source: host \"%s\", port \"%s\"", host, port);
    }

    {
        char* host = lo_url_get_hostname(url);
        char* port = lo_url_get_port(url);
        pData->osc.data.path   = carla_strdup_free(lo_url_get_path(url));
        pData->osc.data.target = lo_address_new_with_proto(proto, host, port);
        carla_stdout("CarlaPlugin::updateOscData() - target: host \"%s\", port \"%s\", path \"%s\"", host, port, pData->osc.data.path);

        std::free(host);
        std::free(port);
    }

#ifndef BUILD_BRIDGE
    if (pData->hints & PLUGIN_IS_BRIDGE)
    {
        carla_stdout("CarlaPlugin::updateOscData() - done");
        return;
    }
#endif

    osc_send_sample_rate(pData->osc.data, static_cast<float>(pData->engine->getSampleRate()));

    for (LinkedList<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
    {
        const CustomData& cData(it.getValue());

        CARLA_SAFE_ASSERT_CONTINUE(cData.type != nullptr && cData.type[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(cData.key != nullptr && cData.key[0] != '\0');
        CARLA_SAFE_ASSERT_CONTINUE(cData.value != nullptr);

        if (std::strcmp(cData.type, CUSTOM_DATA_TYPE_STRING) == 0)
            osc_send_configure(pData->osc.data, cData.key, cData.value);
    }

    if (pData->prog.current >= 0)
        osc_send_program(pData->osc.data, static_cast<uint32_t>(pData->prog.current));

    if (pData->midiprog.current >= 0)
    {
        const MidiProgramData& curMidiProg(pData->midiprog.getCurrent());

        if (getType() == PLUGIN_DSSI)
            osc_send_program(pData->osc.data, curMidiProg.bank, curMidiProg.program);
        else
            osc_send_midi_program(pData->osc.data, curMidiProg.bank, curMidiProg.program);
    }

    for (uint32_t i=0; i < pData->param.count; ++i)
        osc_send_control(pData->osc.data, pData->param.data[i].rindex, getParameterValue(i));

    if ((pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0 && pData->engine->getOptions().frontendWinId != 0)
        pData->transientTryCounter = 1;

    carla_stdout("CarlaPlugin::updateOscData() - done");
}

// void CarlaPlugin::freeOscData()
// {
//     pData->osc.data.free();
// }

bool CarlaPlugin::waitForOscGuiShow()
{
    carla_stdout("CarlaPlugin::waitForOscGuiShow()");
    uint i=0, oscUiTimeout = pData->engine->getOptions().uiBridgesTimeout;

    // wait for UI 'update' call
    for (; i < oscUiTimeout/100; ++i)
    {
        if (pData->osc.data.target != nullptr)
        {
            carla_stdout("CarlaPlugin::waitForOscGuiShow() - got response, asking UI to show itself now");
            osc_send_show(pData->osc.data);
            return true;
        }

        if (pData->osc.thread.isThreadRunning())
            carla_msleep(100);
        else
            return false;
    }

    carla_stdout("CarlaPlugin::waitForOscGuiShow() - Timeout while waiting for UI to respond (waited %u msecs)", oscUiTimeout);
    return false;
}

// -------------------------------------------------------------------
// MIDI events

#ifndef BUILD_BRIDGE
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

    pData->extNotes.append(extNote);

    if (sendGui && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
    {
        if (velo > 0)
            uiNoteOn(channel, note, velo);
        else
            uiNoteOff(channel, note);
    }

    if (sendOsc && pData->engine->isOscControlRegistered())
    {
        if (velo > 0)
            pData->engine->oscSend_control_note_on(pData->id, channel, note, velo);
        else
            pData->engine->oscSend_control_note_off(pData->id, channel, note);
    }

    if (sendCallback)
        pData->engine->callback((velo > 0) ? ENGINE_CALLBACK_NOTE_ON : ENGINE_CALLBACK_NOTE_OFF, pData->id, channel, note, velo, nullptr);
}
#endif

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

// -------------------------------------------------------------------
// Post-poned events

void CarlaPlugin::postRtEventsRun()
{
    const CarlaMutex::ScopedLocker sl(pData->postRtEvents.mutex);
#ifndef BUILD_BRIDGE
    const bool sendOsc(pData->engine->isOscControlRegistered());
#endif

    while (! pData->postRtEvents.data.isEmpty())
    {
        const PluginPostRtEvent& event(pData->postRtEvents.data.getFirst(true));

        switch (event.type)
        {
        case kPluginPostRtEventNull:
            break;

        case kPluginPostRtEventDebug:
#ifndef BUILD_BRIDGE
            pData->engine->callback(ENGINE_CALLBACK_DEBUG, pData->id, event.value1, event.value2, event.value3, nullptr);
#endif
            break;

        case kPluginPostRtEventParameterChange:
            // Update UI
            if (event.value1 >= 0 && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
                uiParameterChange(static_cast<uint32_t>(event.value1), event.value3);

#ifndef BUILD_BRIDGE
            if (event.value2 != 1)
            {
                // Update OSC control client
                if (sendOsc)
                    pData->engine->oscSend_control_set_parameter_value(pData->id, event.value1, event.value3);

                // Update Host
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, event.value1, 0, event.value3, nullptr);
            }
#endif
            break;

        case kPluginPostRtEventProgramChange:
            // Update UI
            if (event.value1 >= 0 && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
                uiProgramChange(static_cast<uint32_t>(event.value1));

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_set_current_program(pData->id, event.value1);

            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_PROGRAM_CHANGED, pData->id, event.value1, 0, 0.0f, nullptr);

            // Update param values
            for (uint32_t j=0; j < pData->param.count; ++j)
            {
                const float paramValue(getParameterValue(j));

                if (sendOsc)
                {
                    pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(j), paramValue);
                    pData->engine->oscSend_control_set_default_value(pData->id, j, pData->param.ranges[j].def);
                }

                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(j), 0, paramValue, nullptr);
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED, pData->id, static_cast<int>(j), 0, pData->param.ranges[j].def, nullptr);
            }
#endif
            break;

        case kPluginPostRtEventMidiProgramChange:
            // Update UI
            if (event.value1 >= 0 && (pData->hints & PLUGIN_HAS_CUSTOM_UI) != 0)
                uiMidiProgramChange(static_cast<uint32_t>(event.value1));

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_set_current_midi_program(pData->id, event.value1);

            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED, pData->id, event.value1, 0, 0.0f, nullptr);

            // Update param values
            for (uint32_t j=0; j < pData->param.count; ++j)
            {
                const float paramValue(getParameterValue(j));

                if (sendOsc)
                {
                    pData->engine->oscSend_control_set_parameter_value(pData->id, static_cast<int32_t>(j), paramValue);
                    pData->engine->oscSend_control_set_default_value(pData->id, j, pData->param.ranges[j].def);
                }

                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pData->id, static_cast<int>(j), 0, paramValue, nullptr);
                pData->engine->callback(ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED, pData->id, static_cast<int>(j), 0, pData->param.ranges[j].def, nullptr);
            }
#endif
            break;

        case kPluginPostRtEventNoteOn:
        {
            CARLA_SAFE_ASSERT_BREAK(event.value1 >= 0 && event.value1 < MAX_MIDI_CHANNELS);
            CARLA_SAFE_ASSERT_BREAK(event.value2 >= 0 && event.value2 < MAX_MIDI_NOTE);
            CARLA_SAFE_ASSERT_BREAK(event.value3 >= 0 && event.value3 < MAX_MIDI_VALUE);

            const uint8_t channel  = static_cast<uint8_t>(event.value1);
            const uint8_t note     = static_cast<uint8_t>(event.value2);
            const uint8_t velocity = uint8_t(event.value3);

            // Update UI
            if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
                uiNoteOn(channel, note, velocity);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_note_on(pData->id, channel, note, velocity);

            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_NOTE_ON, pData->id, event.value1, event.value2, event.value3, nullptr);
#endif
            break;
        }

        case kPluginPostRtEventNoteOff:
        {
            CARLA_SAFE_ASSERT_BREAK(event.value1 >= 0 && event.value1 < MAX_MIDI_CHANNELS);
            CARLA_SAFE_ASSERT_BREAK(event.value2 >= 0 && event.value2 < MAX_MIDI_NOTE);

            const uint8_t channel = static_cast<uint8_t>(event.value1);
            const uint8_t note    = static_cast<uint8_t>(event.value2);

            // Update UI
            if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
                uiNoteOff(channel, note);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (sendOsc)
                pData->engine->oscSend_control_note_off(pData->id, channel, note);

            // Update Host
            pData->engine->callback(ENGINE_CALLBACK_NOTE_OFF, pData->id, event.value1, event.value2, 0.0f, nullptr);
#endif
            break;
        }
        }
    }
}

// -------------------------------------------------------------------
// Post-poned UI Stuff

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
