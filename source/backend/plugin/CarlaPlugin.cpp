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

#include "CarlaPluginInternal.hpp"

#include "CarlaLibUtils.hpp"
#include "CarlaStateUtils.hpp"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------
// Fallback data

static const ParameterData   kParameterDataNull;
static const ParameterRanges kParameterRangesNull;
static const MidiProgramData kMidiProgramDataNull;
static const CustomData      kCustomDataNull;

// -------------------------------------------------------------------
// Library functions

class LibMap
{
public:
    LibMap() {}

    ~LibMap()
    {
        CARLA_ASSERT(libs.isEmpty());
    }

    void* open(const char* const filename)
    {
        CARLA_ASSERT(filename != nullptr);

        if (filename == nullptr)
            return nullptr;

        const CarlaMutex::ScopedLocker sl(mutex);

        for (NonRtList<Lib>::Itenerator it = libs.begin(); it.valid(); it.next())
        {
            Lib& lib(*it);

            if (std::strcmp(lib.filename, filename) == 0)
            {
                lib.count++;
                return lib.lib;
            }
        }

        void* const libPtr(lib_open(filename));

        if (libPtr == nullptr)
            return nullptr;

#ifdef CARLA_PROPER_CPP11_SUPPORT
        Lib lib{libPtr, carla_strdup(filename), 1};
#else
        Lib lib(libPtr, carla_strdup(filename));
#endif
        libs.append(lib);

        return libPtr;
    }

    bool close(void* const libPtr)
    {
        CARLA_ASSERT(libPtr != nullptr);

        if (libPtr == nullptr)
            return false;

        const CarlaMutex::ScopedLocker sl(mutex);

        for (NonRtList<Lib>::Itenerator it = libs.begin(); it.valid(); it.next())
        {
            Lib& lib(*it);

            if (lib.lib != libPtr)
                continue;

            lib.count--;

            if (lib.count == 0)
            {
                delete[] lib.filename;
                lib_close(lib.lib);

                libs.remove(it);
            }

            return true;
        }

        CARLA_ASSERT(false); // invalid pointer
        return false;
    }

private:
    struct Lib {
        void* lib;
        const char* filename;
        int count;

#ifndef CARLA_PROPER_CPP11_SUPPORT
        Lib(void* const lib_, const char* const filename_)
            : lib(lib_),
              filename(filename_),
              count(1) {}
#endif
    };

    CarlaMutex mutex;
    NonRtList<Lib> libs;
};

static LibMap sLibMap;

bool CarlaPluginProtectedData::libOpen(const char* const filename)
{
    lib = sLibMap.open(filename);
    return (lib != nullptr);
}

bool CarlaPluginProtectedData::uiLibOpen(const char* const filename)
{
    uiLib = sLibMap.open(filename);
    return (uiLib != nullptr);
}

bool CarlaPluginProtectedData::libClose()
{
    const bool ret = sLibMap.close(lib);
    lib = nullptr;
    return ret;
}

bool CarlaPluginProtectedData::uiLibClose()
{
    const bool ret = sLibMap.close(uiLib);
    uiLib = nullptr;
    return ret;
}

void* CarlaPluginProtectedData::libSymbol(const char* const symbol)
{
    return lib_symbol(lib, symbol);
}

void* CarlaPluginProtectedData::uiLibSymbol(const char* const symbol)
{
    return lib_symbol(uiLib, symbol);
}

const char* CarlaPluginProtectedData::libError(const char* const filename)
{
    return lib_error(filename);
}

// -------------------------------------------------------------------
// Settings functions

void CarlaPluginProtectedData::saveSetting(const unsigned int option, const bool yesNo)
{
    QSettings settings("falkTX", "CarlaPluginSettings");
    settings.beginGroup((const char*)idStr);

    switch (option)
    {
    case PLUGIN_OPTION_FIXED_BUFFER:
        settings.setValue("FixedBuffer", yesNo);
        break;
    case PLUGIN_OPTION_FORCE_STEREO:
        settings.setValue("ForceStereo", yesNo);
        break;
    case PLUGIN_OPTION_MAP_PROGRAM_CHANGES:
        settings.setValue("MapProgramChanges", yesNo);
        break;
    case PLUGIN_OPTION_USE_CHUNKS:
        settings.setValue("UseChunks", yesNo);
        break;
    case PLUGIN_OPTION_SEND_CONTROL_CHANGES:
        settings.setValue("SendControlChanges", yesNo);
        break;
    case PLUGIN_OPTION_SEND_CHANNEL_PRESSURE:
        settings.setValue("SendChannelPressure", yesNo);
        break;
    case PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH:
        settings.setValue("SendNoteAftertouch", yesNo);
        break;
    case PLUGIN_OPTION_SEND_PITCHBEND:
        settings.setValue("SendPitchbend", yesNo);
        break;
    case PLUGIN_OPTION_SEND_ALL_SOUND_OFF:
        settings.setValue("SendAllSoundOff", yesNo);
        break;
    default:
        break;
    }

    settings.endGroup();
}

unsigned int CarlaPluginProtectedData::loadSettings(const unsigned int options, const unsigned int availOptions)
{
    QSettings settings("falkTX", "CarlaPluginSettings");
    settings.beginGroup((const char*)idStr);

    unsigned int newOptions = 0x0;

    #define CHECK_AND_SET_OPTION(STR, BIT)                              \
    if ((availOptions & BIT) != 0 || BIT == PLUGIN_OPTION_FORCE_STEREO) \
    {                                                                   \
        if (settings.contains(STR))                                     \
        {                                                               \
            if (settings.value(STR, bool(options & BIT)).toBool())      \
                newOptions |= BIT;                                      \
        }                                                               \
        else if (options & BIT)                                         \
            newOptions |= BIT;                                          \
    }

    CHECK_AND_SET_OPTION("FixedBuffer", PLUGIN_OPTION_FIXED_BUFFER);
    CHECK_AND_SET_OPTION("ForceStereo", PLUGIN_OPTION_FORCE_STEREO);
    CHECK_AND_SET_OPTION("MapProgramChanges", PLUGIN_OPTION_MAP_PROGRAM_CHANGES);
    CHECK_AND_SET_OPTION("UseChunks", PLUGIN_OPTION_USE_CHUNKS);
    CHECK_AND_SET_OPTION("SendControlChanges", PLUGIN_OPTION_SEND_CONTROL_CHANGES);
    CHECK_AND_SET_OPTION("SendChannelPressure", PLUGIN_OPTION_SEND_CHANNEL_PRESSURE);
    CHECK_AND_SET_OPTION("SendNoteAftertouch", PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH);
    CHECK_AND_SET_OPTION("SendPitchbend", PLUGIN_OPTION_SEND_PITCHBEND);
    CHECK_AND_SET_OPTION("SendAllSoundOff", PLUGIN_OPTION_SEND_ALL_SOUND_OFF);

    #undef CHECK_AND_SET_OPTION

    settings.endGroup();

    return newOptions;
}

// -------------------------------------------------------------------
// Constructor and destructor

CarlaPlugin::CarlaPlugin(CarlaEngine* const engine, const unsigned int id)
    : fId(id),
      fHints(0x0),
      fOptions(0x0),
      fEnabled(false),
      fIconName("plugin"),
      pData(new CarlaPluginProtectedData(engine, this))
{
    CARLA_ASSERT(pData != nullptr);
    CARLA_ASSERT(engine != nullptr);
    CARLA_ASSERT(id < engine->getMaxPluginNumber());
    CARLA_ASSERT(id == engine->getCurrentPluginCount());
    carla_debug("CarlaPlugin::CarlaPlugin(%p, %i)", engine, id);

    switch (engine->getProccessMode())
    {
    case PROCESS_MODE_SINGLE_CLIENT:
    case PROCESS_MODE_MULTIPLE_CLIENTS:
        CARLA_ASSERT(id < MAX_DEFAULT_PLUGINS);
        break;

    case PROCESS_MODE_CONTINUOUS_RACK:
        CARLA_ASSERT(id < MAX_RACK_PLUGINS);
        break;

    case PROCESS_MODE_PATCHBAY:
        CARLA_ASSERT(id < MAX_PATCHBAY_PLUGINS);
        break;

    case PROCESS_MODE_BRIDGE:
        CARLA_ASSERT(id == 0);
        break;
    }
}

CarlaPlugin::~CarlaPlugin()
{
    carla_debug("CarlaPlugin::~CarlaPlugin()");

    pData->cleanup();
    delete pData;
}

// -------------------------------------------------------------------
// Information (base)

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
    return (pData->extraHints & PLUGIN_HINT_HAS_MIDI_IN) ? 1 : 0;
}

uint32_t CarlaPlugin::getMidiOutCount() const noexcept
{
    return (pData->extraHints & PLUGIN_HINT_HAS_MIDI_OUT) ? 1 : 0;
}

uint32_t CarlaPlugin::getParameterCount() const noexcept
{
    return pData->param.count;
}

uint32_t CarlaPlugin::getParameterScalePointCount(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < pData->param.count);
    return 0;

    // unused
    (void)parameterId;
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
    return pData->custom.count();
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

const ParameterData& CarlaPlugin::getParameterData(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < pData->param.count);

    return (parameterId < pData->param.count) ? pData->param.data[parameterId] : kParameterDataNull;
}

const ParameterRanges& CarlaPlugin::getParameterRanges(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < pData->param.count);

    return (parameterId < pData->param.count) ? pData->param.ranges[parameterId] : kParameterRangesNull;
}

bool CarlaPlugin::isParameterOutput(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < pData->param.count);

    return (parameterId < pData->param.count) ? (pData->param.data[parameterId].type == PARAMETER_OUTPUT) : false;
}

const MidiProgramData& CarlaPlugin::getMidiProgramData(const uint32_t index) const
{
    CARLA_ASSERT(index < pData->midiprog.count);

    return (index < pData->midiprog.count) ? pData->midiprog.data[index] : kMidiProgramDataNull;
}

const CustomData& CarlaPlugin::getCustomData(const uint32_t index) const
{
    CARLA_ASSERT(index < pData->custom.count());

    return (index < pData->custom.count()) ? pData->custom.getAt(index) : kCustomDataNull;
}

int32_t CarlaPlugin::getChunkData(void** const dataPtr) const
{
    CARLA_ASSERT(dataPtr != nullptr);
    CARLA_ASSERT(false); // this should never happen
    return 0;

    // unused
    (void)dataPtr;
}

// -------------------------------------------------------------------
// Information (per-plugin data)

unsigned int CarlaPlugin::getAvailableOptions() const
{
    CARLA_ASSERT(false); // this should never happen
    return 0x0;
}

float CarlaPlugin::getParameterValue(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < getParameterCount());
    CARLA_ASSERT(false); // this should never happen
    return 0.0f;

    // unused
    (void)parameterId;
}

float CarlaPlugin::getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const
{
    CARLA_ASSERT(parameterId < getParameterCount());
    CARLA_ASSERT(scalePointId < getParameterScalePointCount(parameterId));
    CARLA_ASSERT(false); // this should never happen
    return 0.0f;

    // unused
    (void)parameterId;
    (void)scalePointId;
}

void CarlaPlugin::getLabel(char* const strBuf) const noexcept
{
    *strBuf = '\0';
}

void CarlaPlugin::getMaker(char* const strBuf) const noexcept
{
    *strBuf = '\0';
}

void CarlaPlugin::getCopyright(char* const strBuf) const noexcept
{
    *strBuf = '\0';
}

void CarlaPlugin::getRealName(char* const strBuf) const noexcept
{
    *strBuf = '\0';
}

void CarlaPlugin::getParameterName(const uint32_t parameterId, char* const strBuf) const
{
    CARLA_ASSERT(parameterId < getParameterCount());
    CARLA_ASSERT(false); // this should never happen
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterSymbol(const uint32_t parameterId, char* const strBuf) const
{
    CARLA_ASSERT(parameterId < getParameterCount());
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterText(const uint32_t parameterId, char* const strBuf) const
{
    CARLA_ASSERT(parameterId < getParameterCount());
    CARLA_ASSERT(false); // this should never happen
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterUnit(const uint32_t parameterId, char* const strBuf) const
{
    CARLA_ASSERT(parameterId < getParameterCount());
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const
{
    CARLA_ASSERT(parameterId < getParameterCount());
    CARLA_ASSERT(scalePointId < getParameterScalePointCount(parameterId));
    CARLA_ASSERT(false); // this should never happen
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
    (void)scalePointId;
}

void CarlaPlugin::getProgramName(const uint32_t index, char* const strBuf) const
{
    CARLA_ASSERT(index < pData->prog.count);
    CARLA_ASSERT(pData->prog.names[index] != nullptr);

    if (index < pData->prog.count && pData->prog.names[index])
        std::strncpy(strBuf, pData->prog.names[index], STR_MAX);
    else
        *strBuf = '\0';
}

void CarlaPlugin::getMidiProgramName(const uint32_t index, char* const strBuf) const
{
    CARLA_ASSERT(index < pData->midiprog.count);
    CARLA_ASSERT(pData->midiprog.data[index].name != nullptr);

    if (index < pData->midiprog.count && pData->midiprog.data[index].name)
        std::strncpy(strBuf, pData->midiprog.data[index].name, STR_MAX);
    else
        *strBuf = '\0';
}

void CarlaPlugin::getParameterCountInfo(uint32_t* const ins, uint32_t* const outs, uint32_t* const total) const
{
    CARLA_ASSERT(ins != nullptr);
    CARLA_ASSERT(outs != nullptr);
    CARLA_ASSERT(total != nullptr);

    if (ins == nullptr || outs == nullptr || total == nullptr)
        return;

    *ins   = 0;
    *outs  = 0;
    *total = pData->param.count;

    for (uint32_t i=0; i < pData->param.count; ++i)
    {
        if (pData->param.data[i].type == PARAMETER_INPUT)
            *ins += 1;
        else if (pData->param.data[i].type == PARAMETER_OUTPUT)
            *outs += 1;
    }
}

// -------------------------------------------------------------------
// Set data (state)

void CarlaPlugin::prepareForSave()
{
}

const SaveState& CarlaPlugin::getSaveState()
{
    static SaveState saveState;
    saveState.reset();
    prepareForSave();

    char strBuf[STR_MAX+1];

    // ----------------------------
    // Basic info

    switch (getType())
    {
    case PLUGIN_NONE:
        saveState.type = carla_strdup("None");
        break;
    case PLUGIN_INTERNAL:
        saveState.type = carla_strdup("Internal");
        break;
    case PLUGIN_LADSPA:
        saveState.type = carla_strdup("LADSPA");
        break;
    case PLUGIN_DSSI:
        saveState.type = carla_strdup("DSSI");
        break;
    case PLUGIN_LV2:
        saveState.type = carla_strdup("LV2");
        break;
    case PLUGIN_VST:
        saveState.type = carla_strdup("VST");
        break;
    case PLUGIN_VST3:
        saveState.type = carla_strdup("VST3");
        break;
    case PLUGIN_AU:
        saveState.type = carla_strdup("AU");
        break;
    case PLUGIN_GIG:
        saveState.type = carla_strdup("GIG");
        break;
    case PLUGIN_SF2:
        saveState.type = carla_strdup("SF2");
        break;
    case PLUGIN_SFZ:
        saveState.type = carla_strdup("SFZ");
        break;
    }

    getLabel(strBuf);

    saveState.name   = carla_strdup(fName);
    saveState.label  = carla_strdup(strBuf);
    saveState.binary = carla_strdup(fFilename);
    saveState.uniqueID = getUniqueId();

    // ----------------------------
    // Internals

    saveState.active = pData->active;

#ifndef BUILD_BRIDGE
    saveState.dryWet = pData->postProc.dryWet;
    saveState.volume = pData->postProc.volume;
    saveState.balanceLeft  = pData->postProc.balanceLeft;
    saveState.balanceRight = pData->postProc.balanceRight;
    saveState.panning      = pData->postProc.panning;
    saveState.ctrlChannel  = pData->ctrlChannel;
#endif

    // ----------------------------
    // Chunk

    if (fOptions & PLUGIN_OPTION_USE_CHUNKS)
    {
        void* data = nullptr;
        const int32_t dataSize(getChunkData(&data));

        if (data != nullptr && dataSize > 0)
        {
            saveState.chunk = carla_strdup(QByteArray((char*)data, dataSize).toBase64().constData());

            // Don't save anything else if using chunks
            return saveState;
        }
    }

    // ----------------------------
    // Current Program

    if (pData->prog.current >= 0)
    {
        saveState.currentProgramIndex = pData->prog.current;
        saveState.currentProgramName  = carla_strdup(pData->prog.names[pData->prog.current]);
    }

    // ----------------------------
    // Current MIDI Program

    if (pData->midiprog.current >= 0)
    {
        const MidiProgramData& mpData(pData->midiprog.getCurrent());

        saveState.currentMidiBank    = mpData.bank;
        saveState.currentMidiProgram = mpData.program;
    }

    // ----------------------------
    // Parameters

    const float sampleRate(pData->engine->getSampleRate());

    for (uint32_t i=0, count=pData->param.count; i < count; ++i)
    {
        const ParameterData& paramData(pData->param.data[i]);

        if ((paramData.hints & PARAMETER_IS_AUTOMABLE) == 0)
            continue;

        StateParameter* stateParameter(new StateParameter());

        stateParameter->index  = paramData.index;
        stateParameter->midiCC = paramData.midiCC;
        stateParameter->midiChannel = paramData.midiChannel;

        getParameterName(i, strBuf);
        stateParameter->name = carla_strdup(strBuf);

        getParameterSymbol(i, strBuf);
        stateParameter->symbol = carla_strdup(strBuf);;

        stateParameter->value  = getParameterValue(i);

        if (paramData.hints & PARAMETER_USES_SAMPLERATE)
            stateParameter->value /= sampleRate;

        saveState.parameters.append(stateParameter);
    }

    // ----------------------------
    // Custom Data

    for (NonRtList<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
    {
        const CustomData& cData(*it);

        if (cData.type == nullptr)
            continue;

        StateCustomData* stateCustomData(new StateCustomData());

        stateCustomData->type  = carla_strdup(cData.type);
        stateCustomData->key   = carla_strdup(cData.key);
        stateCustomData->value = carla_strdup(cData.value);

        saveState.customData.append(stateCustomData);
    }

    return saveState;
}


struct ParamSymbol {
    uint32_t index;
    const char* symbol;

    ParamSymbol(uint32_t index_, const char* symbol_)
        : index(index_),
          symbol(carla_strdup(symbol_)) {}

    void free()
    {
        if (symbol != nullptr)
        {
            delete[] symbol;
            symbol = nullptr;
        }
    }

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ParamSymbol() = delete;
    ParamSymbol(ParamSymbol&) = delete;
    ParamSymbol(const ParamSymbol&) = delete;
#endif
};

void CarlaPlugin::loadSaveState(const SaveState& saveState)
{
    char strBuf[STR_MAX+1];
    const bool usesMultiProgs(getType() == PLUGIN_SF2 || (getType() == PLUGIN_INTERNAL && (fHints & PLUGIN_IS_SYNTH) != 0));

    // ---------------------------------------------------------------------
    // Part 1 - PRE-set custom data (only that which reload programs)

    for (NonRtList<StateCustomData*>::Itenerator it = saveState.customData.begin(); it.valid(); it.next())
    {
        const StateCustomData* const stateCustomData(*it);
        const char* const key(stateCustomData->key);

        bool wantData = false;

        if (getType() == PLUGIN_DSSI && (std::strcmp(key, "reloadprograms") == 0 || std::strcmp(key, "load") == 0 || std::strncmp(key, "patches", 7) == 0))
            wantData = true;
        else if (usesMultiProgs && std::strcmp(key, "midiPrograms") == 0)
            wantData = true;

        if (wantData)
            setCustomData(stateCustomData->type, stateCustomData->key, stateCustomData->value, true);
    }

    // ---------------------------------------------------------------------
    // Part 2 - set program

    int32_t programId = -1;

    if (saveState.currentProgramName != nullptr)
    {
        getProgramName(saveState.currentProgramIndex, strBuf);

        // Program name matches
        if (std::strcmp(saveState.currentProgramName, strBuf) == 0)
        {
            programId = saveState.currentProgramIndex;
        }
        // index < count
        else if (saveState.currentProgramIndex < static_cast<int32_t>(pData->prog.count))
        {
            programId = saveState.currentProgramIndex;
        }
        // index not valid, try to find by name
        else
        {
            for (uint32_t i=0; i < pData->prog.count; ++i)
            {
                getProgramName(i, strBuf);

                if (std::strcmp(saveState.currentProgramName, strBuf) == 0)
                {
                    programId = i;
                    break;
                }
            }
        }
    }

    // set program now, if valid
    if (programId >= 0)
        setProgram(programId, true, true, true);

    // ---------------------------------------------------------------------
    // Part 3 - set midi program

    if (saveState.currentMidiBank >= 0 && saveState.currentMidiProgram >= 0 && ! usesMultiProgs)
        setMidiProgramById(saveState.currentMidiBank, saveState.currentMidiProgram, true, true, true);

    // ---------------------------------------------------------------------
    // Part 4a - get plugin parameter symbols

    NonRtList<ParamSymbol*> paramSymbols;

    if (getType() == PLUGIN_LADSPA || getType() == PLUGIN_LV2)
    {
        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            getParameterSymbol(i, strBuf);

            if (*strBuf != '\0')
            {
                ParamSymbol* const paramSymbol(new ParamSymbol(i, strBuf));
                paramSymbols.append(paramSymbol);
            }
        }
    }

    // ---------------------------------------------------------------------
    // Part 4b - set parameter values (carefully)

    const float sampleRate(pData->engine->getSampleRate());

    for (NonRtList<StateParameter*>::Itenerator it = saveState.parameters.begin(); it.valid(); it.next())
    {
        StateParameter* const stateParameter(*it);

        int32_t index = -1;

        if (getType() == PLUGIN_LADSPA)
        {
            // Try to set by symbol, otherwise use index
            if (stateParameter->symbol != nullptr && *stateParameter->symbol != 0)
            {
                for (NonRtList<ParamSymbol*>::Itenerator it = paramSymbols.begin(); it.valid(); it.next())
                {
                    ParamSymbol* const paramSymbol(*it);

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
            if (stateParameter->symbol != nullptr && *stateParameter->symbol != 0)
            {
                for (NonRtList<ParamSymbol*>::Itenerator it = paramSymbols.begin(); it.valid(); it.next())
                {
                    ParamSymbol* const paramSymbol(*it);

                    if (std::strcmp(stateParameter->symbol, paramSymbol->symbol) == 0)
                    {
                        index = paramSymbol->index;
                        break;
                    }
                }
                if (index == -1)
                    carla_stderr("Failed to find LV2 parameter symbol for '%s')", stateParameter->symbol);
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
            if (pData->param.data[index].hints & PARAMETER_USES_SAMPLERATE)
                stateParameter->value *= sampleRate;

            setParameterValue(index, stateParameter->value, true, true, true);
#ifndef BUILD_BRIDGE
            setParameterMidiCC(index, stateParameter->midiCC, true, true);
            setParameterMidiChannel(index, stateParameter->midiChannel, true, true);
#endif
        }
        else
            carla_stderr("Could not set parameter data for '%s'", stateParameter->name);
    }

    // clear
    for (NonRtList<ParamSymbol*>::Itenerator it = paramSymbols.begin(); it.valid(); it.next())
    {
        ParamSymbol* const paramSymbol(*it);
        paramSymbol->free();
        delete paramSymbol;
    }

    paramSymbols.clear();

    // ---------------------------------------------------------------------
    // Part 5 - set custom data

    for (NonRtList<StateCustomData*>::Itenerator it = saveState.customData.begin(); it.valid(); it.next())
    {
        const StateCustomData* const stateCustomData(*it);
        const char* const key(stateCustomData->key);

        if (getType() == PLUGIN_DSSI && (std::strcmp(key, "reloadprograms") == 0 || std::strcmp(key, "load") == 0 || std::strncmp(key, "patches", 7) == 0))
            continue;
        if (usesMultiProgs && std::strcmp(key, "midiPrograms") == 0)
            continue;

        setCustomData(stateCustomData->type, stateCustomData->key, stateCustomData->value, true);
    }

    // ---------------------------------------------------------------------
    // Part 6 - set chunk

    if (saveState.chunk != nullptr && (fOptions & PLUGIN_OPTION_USE_CHUNKS) != 0)
        setChunkData(saveState.chunk);

    // ---------------------------------------------------------------------
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
    carla_debug("CarlaPlugin::saveStateToFile(\"%s\")", filename);
    CARLA_ASSERT(filename != nullptr);

    QFile file(filename);

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QString content;
    fillXmlStringFromSaveState(content, getSaveState());

    QTextStream out(&file);
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PRESET>\n";
    out << "<CARLA-PRESET VERSION='1.0'>\n";
    out << content;
    out << "</CARLA-PRESET>\n";

    file.close();
    return true;
}

bool CarlaPlugin::loadStateFromFile(const char* const filename)
{
    carla_debug("CarlaPlugin::loadStateFromFile(\"%s\")", filename);
    CARLA_ASSERT(filename != nullptr);

    QFile file(filename);

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QDomDocument xml;
    xml.setContent(file.readAll());
    file.close();

    QDomNode xmlNode(xml.documentElement());

    if (xmlNode.toElement().tagName() != "CARLA-PRESET")
    {
        pData->engine->setLastError("Not a valid Carla preset file");
        return false;
    }

    SaveState saveState;
    fillSaveStateFromXmlNode(saveState, xmlNode);
    loadSaveState(saveState);

    return true;
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setId(const unsigned int newId) noexcept
{
    fId = newId;
}

void CarlaPlugin::setName(const char* const newName)
{
    CARLA_ASSERT(newName != nullptr);

    fName = newName;
}

void CarlaPlugin::setOption(const unsigned int option, const bool yesNo)
{
    CARLA_ASSERT(getAvailableOptions() & option);

    if (yesNo)
        fOptions |= option;
    else
        fOptions &= ~option;

    pData->saveSetting(option, yesNo);
}

void CarlaPlugin::setEnabled(const bool yesNo)
{
    if (fEnabled == yesNo)
        return;

    fEnabled = yesNo;

    pData->masterMutex.lock();
    pData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setActive(const bool active, const bool sendOsc, const bool sendCallback)
{
#ifndef BUILD_BRIDGE
    CARLA_ASSERT(sendOsc || sendCallback); // never call this from RT
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

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_ACTIVE, value);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_ACTIVE, 0, value, nullptr);
#else
    return;

    // unused
    (void)sendOsc;
    (void)sendCallback;
#endif
}

#ifndef BUILD_BRIDGE
void CarlaPlugin::setDryWet(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(0.0f, 1.0f, value));

    if (pData->postProc.dryWet == fixedValue)
        return;

    pData->postProc.dryWet = fixedValue;

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_DRYWET, fixedValue);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_DRYWET, 0, fixedValue, nullptr);
}

void CarlaPlugin::setVolume(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0f && value <= 1.27f);

    const float fixedValue(carla_fixValue<float>(0.0f, 1.27f, value));

    if (pData->postProc.volume == fixedValue)
        return;

    pData->postProc.volume = fixedValue;

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_VOLUME, fixedValue);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_VOLUME, 0, fixedValue, nullptr);
}

void CarlaPlugin::setBalanceLeft(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(-1.0f, 1.0f, value));

    if (pData->postProc.balanceLeft == fixedValue)
        return;

    pData->postProc.balanceLeft = fixedValue;

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_BALANCE_LEFT, fixedValue);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_BALANCE_LEFT, 0, fixedValue, nullptr);
}

void CarlaPlugin::setBalanceRight(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(-1.0f, 1.0f, value));

    if (pData->postProc.balanceRight == fixedValue)
        return;

    pData->postProc.balanceRight = fixedValue;

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_BALANCE_RIGHT, fixedValue);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_BALANCE_RIGHT, 0, fixedValue, nullptr);
}

void CarlaPlugin::setPanning(const float value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0f && value <= 1.0f);

    const float fixedValue(carla_fixValue<float>(-1.0f, 1.0f, value));

    if (pData->postProc.panning == fixedValue)
        return;

    pData->postProc.panning = fixedValue;

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_PANNING, fixedValue);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_PANNING, 0, fixedValue, nullptr);
}
#endif

void CarlaPlugin::setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback)
{
#ifndef BUILD_BRIDGE
    CARLA_ASSERT(sendOsc || sendCallback); // never call this from RT
#endif
    CARLA_ASSERT_INT(channel >= -1 && channel < MAX_MIDI_CHANNELS, channel);

    if (pData->ctrlChannel == channel)
        return;

    pData->ctrlChannel = channel;

#ifndef BUILD_BRIDGE
    const float ctrlf(channel);

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_CTRL_CHANNEL, ctrlf);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_CTRL_CHANNEL, 0, ctrlf, nullptr);

    if (fHints & PLUGIN_IS_BRIDGE)
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

void CarlaPlugin::setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < pData->param.count);
#ifdef BUILD_BRIDGE
    CARLA_ASSERT(! sendGui); // this should never happen
#endif

#ifndef BUILD_BRIDGE
    if (sendGui)
        uiParameterChange(parameterId, value);

    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_value(fId, parameterId, value);
#endif

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, parameterId, 0, value, nullptr);

#ifdef BUILD_BRIDGE
    return;

    // unused
    (void)sendGui;
    (void)sendOsc;
#endif
}

void CarlaPlugin::setParameterValueByRealIndex(const int32_t rindex, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(rindex > PARAMETER_MAX && rindex != PARAMETER_NULL);

    if (rindex <= PARAMETER_MAX)
        return;
    if (rindex == PARAMETER_NULL)
        return;
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

#ifndef BUILD_BRIDGE
void CarlaPlugin::setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(sendOsc || sendCallback); // never call this from RT
    CARLA_ASSERT(parameterId < pData->param.count);
    CARLA_ASSERT_INT(channel < MAX_MIDI_CHANNELS, channel);

    if (channel >= MAX_MIDI_CHANNELS)
        channel = MAX_MIDI_CHANNELS;

    pData->param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_midi_channel(fId, parameterId, channel);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, fId, parameterId, channel, 0.0f, nullptr);

    if (fHints & PLUGIN_IS_BRIDGE)
        {} // TODO
#else
    return;

    // unused
    (void)sendOsc;
    (void)sendCallback;
#endif
}

void CarlaPlugin::setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(sendOsc || sendCallback); // never call this from RT
    CARLA_ASSERT(parameterId < pData->param.count);
    CARLA_ASSERT_INT(cc >= -1 && cc <= 0x5F, cc);

    if (cc < -1 || cc > 0x5F)
        cc = -1;

    pData->param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        pData->engine->oscSend_control_set_parameter_midi_cc(fId, parameterId, cc);

    if (sendCallback)
        pData->engine->callback(CALLBACK_PARAMETER_MIDI_CC_CHANGED, fId, parameterId, cc, 0.0f, nullptr);

    if (fHints & PLUGIN_IS_BRIDGE)
        {} // TODO
#else
    return;

    // unused
    (void)sendOsc;
    (void)sendCallback;
#endif
}
#endif

void CarlaPlugin::setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
{
    CARLA_ASSERT(type != nullptr);
    CARLA_ASSERT(key != nullptr);
    CARLA_ASSERT(value != nullptr);
#ifdef BUILD_BRIDGE
    CARLA_ASSERT(! sendGui); // this should never happen
#endif

    if (type == nullptr)
        return carla_stderr2("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is null", type, key, value, bool2str(sendGui));

    if (key == nullptr)
        return carla_stderr2("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

    if (value == nullptr)
        return carla_stderr2("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

    bool saveData = true;

    if (std::strcmp(type, CUSTOM_DATA_STRING) == 0)
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
    for (NonRtList<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
    {
        CustomData& cData(*it);

        CARLA_ASSERT(cData.type != nullptr);
        CARLA_ASSERT(cData.key != nullptr);
        CARLA_ASSERT(cData.value != nullptr);

        if (cData.type == nullptr)
            return;
        if (cData.key == nullptr)
            return;

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
    CARLA_ASSERT(stringData != nullptr);
    CARLA_ASSERT(false); // this should never happen
    return;

    // unused
    (void)stringData;
}

void CarlaPlugin::setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(pData->prog.count));
#ifdef BUILD_BRIDGE
    CARLA_ASSERT(! sendGui); // this should never happen
#endif

    if (index > static_cast<int32_t>(pData->prog.count))
        return;

    const int32_t fixedIndex(carla_fixValue<int32_t>(-1, pData->prog.count, index));

    pData->prog.current = fixedIndex;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        pData->engine->oscSend_control_set_program(fId, fixedIndex);
#endif

    if (sendCallback)
        pData->engine->callback(CALLBACK_PROGRAM_CHANGED, fId, fixedIndex, 0, 0.0f, nullptr);

    // Change default parameter values
    if (fixedIndex >= 0)
    {
#ifndef BUILD_BRIDGE
        if (sendGui)
            uiProgramChange(fixedIndex);
#endif

        if (getType() == PLUGIN_GIG || getType() == PLUGIN_SF2 || getType() == PLUGIN_SFZ)
            return;

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            const float value(pData->param.ranges[i].getFixedValue(getParameterValue(i)));

            pData->param.ranges[i].def = value;

            if (sendOsc || sendCallback)
            {
#ifndef BUILD_BRIDGE
                pData->engine->oscSend_control_set_default_value(fId, i, value);
                pData->engine->oscSend_control_set_parameter_value(fId, i, value);
#endif

                pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, i, 0, value, nullptr);
                pData->engine->callback(CALLBACK_PARAMETER_DEFAULT_CHANGED, fId, i, 0, value, nullptr);
            }
        }
    }

#ifdef BUILD_BRIDGE
    return;

    // unused
    (void)sendGui;
#endif
}

void CarlaPlugin::setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count));
#ifdef BUILD_BRIDGE
    CARLA_ASSERT(! sendGui); // this should never happen
#endif

    if (index > static_cast<int32_t>(pData->midiprog.count))
        return;

    const int32_t fixedIndex(carla_fixValue<int32_t>(-1, pData->midiprog.count, index));

    pData->midiprog.current = fixedIndex;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        pData->engine->oscSend_control_set_midi_program(fId, fixedIndex);
#endif

    if (sendCallback)
        pData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fId, fixedIndex, 0, 0.0f, nullptr);

    if (fixedIndex >= 0)
    {
#ifndef BUILD_BRIDGE
        if (sendGui)
            uiMidiProgramChange(fixedIndex);
#endif

        if (getType() == PLUGIN_GIG || getType() == PLUGIN_SF2 || getType() == PLUGIN_SFZ)
            return;

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            const float value(pData->param.ranges[i].getFixedValue(getParameterValue(i)));

            pData->param.ranges[i].def = value;

            if (sendOsc || sendCallback)
            {
#ifndef BUILD_BRIDGE
                pData->engine->oscSend_control_set_default_value(fId, i, value);
                pData->engine->oscSend_control_set_parameter_value(fId, i, value);
#endif

                pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, i, 0, value, nullptr);
                pData->engine->callback(CALLBACK_PARAMETER_DEFAULT_CHANGED, fId, i, 0, value, nullptr);
            }
        }
    }

#ifdef BUILD_BRIDGE
    return;

    // unused
    (void)sendGui;
#endif
}

void CarlaPlugin::setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    for (uint32_t i=0; i < pData->midiprog.count; ++i)
    {
        if (pData->midiprog.data[i].bank == bank && pData->midiprog.data[i].program == program)
            return setMidiProgram(i, sendGui, sendOsc, sendCallback);
    }
}

// -------------------------------------------------------------------
// Set gui stuff

void CarlaPlugin::showGui(const bool yesNo)
{
    CARLA_ASSERT(false);
    return;

    // unused
    (void)yesNo;
}

void CarlaPlugin::idleGui()
{
    if (! fEnabled)
        return;

    if (fHints & PLUGIN_HAS_SINGLE_THREAD)
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
}

// -------------------------------------------------------------------
// Plugin state

void CarlaPlugin::reloadPrograms(const bool)
{
}

// -------------------------------------------------------------------
// Plugin processing

void CarlaPlugin::activate()
{
    CARLA_ASSERT(! pData->active);
}

void CarlaPlugin::deactivate()
{
    CARLA_ASSERT(pData->active);
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

bool CarlaPlugin::tryLock()
{
    return pData->masterMutex.tryLock();
}

void CarlaPlugin::unlock()
{
    pData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Plugin buffers

void CarlaPlugin::initBuffers()
{
    pData->audioIn.initBuffers(pData->engine);
    pData->audioOut.initBuffers(pData->engine);
    pData->event.initBuffers(pData->engine);
}

void CarlaPlugin::clearBuffers()
{
    pData->clearBuffers();
}

// -------------------------------------------------------------------
// OSC stuff

void CarlaPlugin::registerToOscClient()
{
#ifdef BUILD_BRIDGE
    if (! pData->engine->isOscBridgeRegistered())
        return;
#else
    if (! pData->engine->isOscControlRegistered())
        return;
#endif

#ifndef BUILD_BRIDGE
    pData->engine->oscSend_control_add_plugin_start(fId, fName);
#endif

    // Base data
    {
        char bufName[STR_MAX+1]  = { '\0' };
        char bufLabel[STR_MAX+1] = { '\0' };
        char bufMaker[STR_MAX+1] = { '\0' };
        char bufCopyright[STR_MAX+1] = { '\0' };
        getRealName(bufName);
        getLabel(bufLabel);
        getMaker(bufMaker);
        getCopyright(bufCopyright);

#ifdef BUILD_BRIDGE
        pData->engine->osc_send_bridge_plugin_info(category(), fHints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
#else
        pData->engine->oscSend_control_set_plugin_data(fId, getType(), getCategory(), fHints, bufName, bufLabel, bufMaker, bufCopyright, getUniqueId());
#endif
    }

    // Base count
    {
        uint32_t cIns, cOuts, cTotals;
        getParameterCountInfo(&cIns, &cOuts, &cTotals);

#ifdef BUILD_BRIDGE
        pData->engine->osc_send_bridge_audio_count(audioInCount(), audioOutCount(), audioInCount() + audioOutCount());
        pData->engine->osc_send_bridge_midi_count(midiInCount(), midiOutCount(), midiInCount() + midiOutCount());
        pData->engine->osc_send_bridge_parameter_count(cIns, cOuts, cTotals);
#else
        pData->engine->oscSend_control_set_plugin_ports(fId, getAudioInCount(), getAudioOutCount(), getMidiInCount(), getMidiOutCount(), cIns, cOuts, cTotals);
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
            pData->engine->osc_send_bridge_parameter_info(i, bufName, bufUnit);
            pData->engine->osc_send_bridge_parameter_data(i, paramData.type, paramData.rindex, paramData.hints, paramData.midiChannel, paramData.midiCC);
            pData->engine->osc_send_bridge_parameter_ranges(i, paramRanges.def, paramRanges.min, paramRanges.max, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            pData->engine->osc_send_bridge_set_parameter_value(i, getParameterValue(i));
#else
            pData->engine->oscSend_control_set_parameter_data(fId, i, paramData.type, paramData.hints, bufName, bufUnit, getParameterValue(i));
            pData->engine->oscSend_control_set_parameter_ranges(fId, i, paramRanges.min, paramRanges.max, paramRanges.def, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            pData->engine->oscSend_control_set_parameter_midi_cc(fId, i, paramData.midiCC);
            pData->engine->oscSend_control_set_parameter_midi_channel(fId, i, paramData.midiChannel);
#endif
        }
    }

    // Programs
    if (pData->prog.count > 0)
    {
#ifdef BUILD_BRIDGE
        pData->engine->osc_send_bridge_program_count(pData->prog.count);

        for (uint32_t i=0; i < pData->prog.count; ++i)
            pData->engine->osc_send_bridge_program_info(i, pData->prog.names[i]);

        pData->engine->osc_send_bridge_set_program(pData->prog.current);
#else
        pData->engine->oscSend_control_set_program_count(fId, pData->prog.count);

        for (uint32_t i=0; i < pData->prog.count; ++i)
            pData->engine->oscSend_control_set_program_name(fId, i, pData->prog.names[i]);

        pData->engine->oscSend_control_set_program(fId, pData->prog.current);
#endif
    }

    // MIDI Programs
    if (pData->midiprog.count > 0)
    {
#ifdef BUILD_BRIDGE
        pData->engine->osc_send_bridge_midi_program_count(pData->midiprog.count);

        for (uint32_t i=0; i < pData->midiprog.count; ++i)
        {
            const MidiProgramData& mpData(pData->midiprog.data[i]);

            pData->engine->osc_send_bridge_midi_program_info(i, mpData.bank, mpData.program, mpData.name);
        }

        pData->engine->osc_send_bridge_set_midi_program(pData->midiprog.current);
#else
        pData->engine->oscSend_control_set_midi_program_count(fId, pData->midiprog.count);

        for (uint32_t i=0; i < pData->midiprog.count; ++i)
        {
            const MidiProgramData& mpData(pData->midiprog.data[i]);

            pData->engine->oscSend_control_set_midi_program_data(fId, i, mpData.bank, mpData.program, mpData.name);
        }

        pData->engine->oscSend_control_set_midi_program(fId, pData->midiprog.current);
#endif
    }

#ifndef BUILD_BRIDGE
    pData->engine->oscSend_control_add_plugin_end(fId);

    // Internal Parameters
    {
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_DRYWET, pData->postProc.dryWet);
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_VOLUME, pData->postProc.volume);
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_BALANCE_LEFT, pData->postProc.balanceLeft);
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_BALANCE_RIGHT, pData->postProc.balanceRight);
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_PANNING, pData->postProc.panning);
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_CTRL_CHANNEL, pData->ctrlChannel);
        pData->engine->oscSend_control_set_parameter_value(fId, PARAMETER_ACTIVE, pData->active ? 1.0f : 0.0f);
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
    if (fHints & PLUGIN_IS_BRIDGE)
        return;
#endif

    osc_send_sample_rate(pData->osc.data, pData->engine->getSampleRate());

    for (NonRtList<CustomData>::Itenerator it = pData->custom.begin(); it.valid(); it.next())
    {
        const CustomData& cData(*it);

        CARLA_ASSERT(cData.type != nullptr);
        CARLA_ASSERT(cData.key != nullptr);
        CARLA_ASSERT(cData.value != nullptr);

        if (std::strcmp(cData.type, CUSTOM_DATA_STRING) == 0)
            osc_send_configure(pData->osc.data, cData.key, cData.value);
    }

    if (pData->prog.current >= 0)
        osc_send_program(pData->osc.data, pData->prog.current);

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
        else
            carla_msleep(100);
    }

    carla_stdout("CarlaPlugin::waitForOscGuiShow() - Timeout while waiting for UI to respond (waited %u msecs)", oscUiTimeout);
    return false;
}

// -------------------------------------------------------------------
// MIDI events

#ifndef BUILD_BRIDGE
void CarlaPlugin::sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note < MAX_MIDI_NOTE);
    CARLA_ASSERT(velo < MAX_MIDI_VALUE);

    if (! pData->active)
        return;

    ExternalMidiNote extNote;
    extNote.channel = channel;
    extNote.note    = note;
    extNote.velo    = velo;

    pData->extNotes.append(extNote);

    if (sendGui)
    {
        if (velo > 0)
            uiNoteOn(channel, note, velo);
        else
            uiNoteOff(channel, note);
    }

    if (sendOsc)
    {
        if (velo > 0)
            pData->engine->oscSend_control_note_on(fId, channel, note, velo);
        else
            pData->engine->oscSend_control_note_off(fId, channel, note);
    }

    if (sendCallback)
        pData->engine->callback((velo > 0) ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, fId, channel, note, velo, nullptr);
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

    for (unsigned short i=0; i < MAX_MIDI_NOTE; ++i)
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

    while (! pData->postRtEvents.data.isEmpty())
    {
        const PluginPostRtEvent& event(pData->postRtEvents.data.getFirst(true));

        switch (event.type)
        {
        case kPluginPostRtEventNull:
            break;

        case kPluginPostRtEventDebug:
#ifndef BUILD_BRIDGE
            pData->engine->callback(CALLBACK_DEBUG, fId, event.value1, event.value2, event.value3, nullptr);
#endif
            break;

        case kPluginPostRtEventParameterChange:
            // Update UI
            if (event.value1 >= 0)
                uiParameterChange(event.value1, event.value3);

#ifndef BUILD_BRIDGE
            if (event.value2 != 1)
            {
                // Update OSC control client
                if (pData->engine->isOscControlRegistered())
                    pData->engine->oscSend_control_set_parameter_value(fId, event.value1, event.value3);

                // Update Host
                pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, event.value1, 0, event.value3, nullptr);
            }
#endif
            break;

        case kPluginPostRtEventProgramChange:
            // Update UI
            if (event.value1 >= 0)
                uiProgramChange(event.value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (pData->engine->isOscControlRegistered())
                pData->engine->oscSend_control_set_program(fId, event.value1);

            // Update Host
            pData->engine->callback(CALLBACK_PROGRAM_CHANGED, fId, event.value1, 0, 0.0f, nullptr);

            // Update param values
            {
                const bool sendOsc(pData->engine->isOscControlRegistered());

                for (uint32_t j=0; j < pData->param.count; ++j)
                {
                    const float value(getParameterValue(j));

                    if (sendOsc)
                    {
                        pData->engine->oscSend_control_set_parameter_value(fId, j, value);
                        pData->engine->oscSend_control_set_default_value(fId, j, pData->param.ranges[j].def);
                    }

                    pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, j, 0, value, nullptr);
                    pData->engine->callback(CALLBACK_PARAMETER_DEFAULT_CHANGED, fId, j, 0, pData->param.ranges[j].def, nullptr);
                }
            }
#endif
            break;

        case kPluginPostRtEventMidiProgramChange:
            // Update UI
            if (event.value1 >= 0)
                uiMidiProgramChange(event.value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (pData->engine->isOscControlRegistered())
                pData->engine->oscSend_control_set_midi_program(fId, event.value1);

            // Update Host
            pData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fId, event.value1, 0, 0.0f, nullptr);

            // Update param values
            {
                const bool sendOsc(pData->engine->isOscControlRegistered());

                for (uint32_t j=0; j < pData->param.count; ++j)
                {
                    const float value(getParameterValue(j));

                    if (sendOsc)
                    {
                        pData->engine->oscSend_control_set_parameter_value(fId, j, value);
                        pData->engine->oscSend_control_set_default_value(fId, j, pData->param.ranges[j].def);
                    }

                    pData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, j, 0, value, nullptr);
                    pData->engine->callback(CALLBACK_PARAMETER_DEFAULT_CHANGED, fId, j, 0, pData->param.ranges[j].def, nullptr);
                }
            }
#endif
            break;

        case kPluginPostRtEventNoteOn:
            // Update UI
            uiNoteOn(event.value1, event.value2, int(event.value3));

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (pData->engine->isOscControlRegistered())
                pData->engine->oscSend_control_note_on(fId, event.value1, event.value2, int(event.value3));

            // Update Host
            pData->engine->callback(CALLBACK_NOTE_ON, fId, event.value1, event.value2, int(event.value3), nullptr);
#endif
            break;

        case kPluginPostRtEventNoteOff:
            // Update UI
            uiNoteOff(event.value1, event.value2);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (pData->engine->isOscControlRegistered())
                pData->engine->oscSend_control_note_off(fId, event.value1, event.value2);

            // Update Host
            pData->engine->callback(CALLBACK_NOTE_OFF, fId, event.value1, event.value2, 0.0f, nullptr);
#endif
            break;
        }
    }
}

// -------------------------------------------------------------------
// Post-poned UI Stuff

void CarlaPlugin::uiParameterChange(const uint32_t index, const float value)
{
    CARLA_ASSERT(index < getParameterCount());
    return;

    // unused
    (void)index;
    (void)value;
}

void CarlaPlugin::uiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < getProgramCount());
    return;

    // unused
    (void)index;
}

void CarlaPlugin::uiMidiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < getMidiProgramCount());
    return;

    // unused
    (void)index;
}

void CarlaPlugin::uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
{
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note < MAX_MIDI_NOTE);
    CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);
    return;

    // unused
    (void)channel;
    (void)note;
    (void)velo;
}

void CarlaPlugin::uiNoteOff(const uint8_t channel, const uint8_t note)
{
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
    CARLA_ASSERT(note < MAX_MIDI_NOTE);
    return;

    // unused
    (void)channel;
    (void)note;
}

// -------------------------------------------------------------------
// Scoped Disabler

CarlaPlugin::ScopedDisabler::ScopedDisabler(CarlaPlugin* const plugin)
    : fPlugin(plugin)
{
    carla_debug("CarlaPlugin::ScopedDisabler(%p)", plugin);
    CARLA_ASSERT(plugin != nullptr);
    CARLA_ASSERT(plugin->pData != nullptr);
    CARLA_ASSERT(plugin->pData->client != nullptr);

    if (plugin == nullptr)
        return;
    if (plugin->pData == nullptr)
        return;
    if (plugin->pData->client == nullptr)
        return;

    plugin->pData->masterMutex.lock();

    if (plugin->fEnabled)
        plugin->fEnabled = false;

    if (plugin->pData->client->isActive())
        plugin->pData->client->deactivate();
}

CarlaPlugin::ScopedDisabler::~ScopedDisabler()
{
    carla_debug("CarlaPlugin::~ScopedDisabler()");
    CARLA_ASSERT(fPlugin != nullptr);
    CARLA_ASSERT(fPlugin->pData != nullptr);
    CARLA_ASSERT(fPlugin->pData->client != nullptr);

    if (fPlugin == nullptr)
        return;
    if (fPlugin->pData == nullptr)
        return;
    if (fPlugin->pData->client == nullptr)
        return;

    fPlugin->fEnabled = true;
    fPlugin->pData->client->activate();
    fPlugin->pData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Scoped Process Locker

CarlaPlugin::ScopedSingleProcessLocker::ScopedSingleProcessLocker(CarlaPlugin* const plugin, const bool block)
    : fPlugin(plugin),
      fBlock(block)
{
    carla_debug("CarlaPlugin::ScopedSingleProcessLocker(%p, %s)", plugin, bool2str(block));
    CARLA_ASSERT(fPlugin != nullptr && fPlugin->pData != nullptr);

    if (fPlugin == nullptr)
        return;
    if (fPlugin->pData == nullptr)
        return;

    if (fBlock)
        plugin->pData->singleMutex.lock();
}

CarlaPlugin::ScopedSingleProcessLocker::~ScopedSingleProcessLocker()
{
    carla_debug("CarlaPlugin::~ScopedSingleProcessLocker()");
    CARLA_ASSERT(fPlugin != nullptr && fPlugin->pData != nullptr);

    if (fPlugin == nullptr)
        return;
    if (fPlugin->pData == nullptr)
        return;

    if (fBlock)
    {
#ifndef BUILD_BRIDGE
        if (fPlugin->pData->singleMutex.wasTryLockCalled())
            fPlugin->pData->needsReset = true;
#endif

        fPlugin->pData->singleMutex.unlock();
    }
}

// #ifdef BUILD_BRIDGE
// CarlaPlugin* newFailAsBridge(const CarlaPlugin::Initializer& init)
// {
//     init.engine->setLastError("Can't use this in plugin bridges");
//     return nullptr;
// }
//
// CarlaPlugin* CarlaPlugin::newNative(const Initializer& init)          { return newFailAsBridge(init); }
// CarlaPlugin* CarlaPlugin::newGIG(const Initializer& init, const bool) { return newFailAsBridge(init); }
// CarlaPlugin* CarlaPlugin::newSF2(const Initializer& init, const bool) { return newFailAsBridge(init); }
// CarlaPlugin* CarlaPlugin::newSFZ(const Initializer& init, const bool) { return newFailAsBridge(init); }
//
// # ifdef WANT_NATIVE
// size_t                  CarlaPlugin::getNativePluginCount()                  { return 0;       }
// const PluginDescriptor* CarlaPlugin::getNativePluginDescriptor(const size_t) { return nullptr; }
// # endif
// #endif

// -------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
