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

        const CarlaMutex::ScopedLocker sl(&mutex);

        for (auto it = libs.begin(); it.valid(); it.next())
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

        Lib lib{libPtr, carla_strdup(filename), 1};
        libs.append(lib);

        return libPtr;
    }

    bool close(void* const libPtr)
    {
        CARLA_ASSERT(libPtr != nullptr);

        if (libPtr == nullptr)
            return false;

        const CarlaMutex::ScopedLocker sl(&mutex);

        for (auto it = libs.begin(); it.valid(); it.next())
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
        void* const lib;
        const char* const filename;
        int count;
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

bool CarlaPluginProtectedData::libClose()
{
    const bool ret = sLibMap.close(lib);
    lib = nullptr;
    return ret;
}

void* CarlaPluginProtectedData::libSymbol(const char* const symbol)
{
    return lib_symbol(lib, symbol);
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

#define CHECK_AND_SET_OPTION(STR, BIT)                                  \
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
// Plugin Helpers

CarlaEngine* CarlaPluginGetEngine(CarlaPlugin* const plugin)
{
    return CarlaPluginProtectedData::getEngine(plugin);
}

CarlaEngineAudioPort* CarlaPluginGetAudioInPort(CarlaPlugin* const plugin, const uint32_t index)
{
    return CarlaPluginProtectedData::getAudioInPort(plugin, index);
}

CarlaEngineAudioPort* CarlaPluginGetAudioOutPort(CarlaPlugin* const plugin, const uint32_t index)
{
    return CarlaPluginProtectedData::getAudioOutPort(plugin, index);
}

// -------------------------------------------------------------------
// Constructor and destructor

CarlaPlugin::CarlaPlugin(CarlaEngine* const engine, const unsigned int id)
    : fId(id),
      fHints(0x0),
      fOptions(0x0),
      fEnabled(false),
      kData(new CarlaPluginProtectedData(engine, this))
{
    CARLA_ASSERT(kData != nullptr);
    CARLA_ASSERT(engine != nullptr);
    CARLA_ASSERT(id < engine->maxPluginNumber());
    CARLA_ASSERT(id == engine->currentPluginCount());
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

    kData->cleanup();
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
    (void)parameterId;
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

const CustomData& CarlaPlugin::customData(const uint32_t index) const
{
    CARLA_ASSERT(index < kData->custom.count());

    return (index < kData->custom.count()) ? kData->custom.getAt(index) : kCustomDataNull;
}

int32_t CarlaPlugin::chunkData(void** const dataPtr)
{
    CARLA_ASSERT(dataPtr != nullptr);
    CARLA_ASSERT(false); // this should never happen
    return 0;

    // unused
    (void)dataPtr;
}

// -------------------------------------------------------------------
// Information (per-plugin data)

unsigned int CarlaPlugin::availableOptions()
{
    CARLA_ASSERT(false); // this should never happen
    return 0x0;
}

float CarlaPlugin::getParameterValue(const uint32_t parameterId)
{
    CARLA_ASSERT(parameterId < parameterCount());
    CARLA_ASSERT(false); // this should never happen
    return 0.0f;

    // unused
    (void)parameterId;
}

float CarlaPlugin::getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
{
    CARLA_ASSERT(parameterId < parameterCount());
    CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));
    CARLA_ASSERT(false); // this should never happen
    return 0.0f;

    // unused
    (void)parameterId;
    (void)scalePointId;
}

void CarlaPlugin::getLabel(char* const strBuf)
{
    *strBuf = '\0';
}

void CarlaPlugin::getMaker(char* const strBuf)
{
    *strBuf = '\0';
}

void CarlaPlugin::getCopyright(char* const strBuf)
{
    *strBuf = '\0';
}

void CarlaPlugin::getRealName(char* const strBuf)
{
    *strBuf = '\0';
}

void CarlaPlugin::getParameterName(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    CARLA_ASSERT(false); // this should never happen
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterSymbol(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterText(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    CARLA_ASSERT(false); // this should never happen
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterUnit(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
}

void CarlaPlugin::getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < parameterCount());
    CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));
    CARLA_ASSERT(false); // this should never happen
    *strBuf = '\0';
    return;

    // unused
    (void)parameterId;
    (void)scalePointId;
}

void CarlaPlugin::getProgramName(const uint32_t index, char* const strBuf)
{
    CARLA_ASSERT(index < kData->prog.count);
    CARLA_ASSERT(kData->prog.names[index] != nullptr);

    if (index < kData->prog.count && kData->prog.names[index])
        std::strncpy(strBuf, kData->prog.names[index], STR_MAX);
    else
        *strBuf = '\0';
}

void CarlaPlugin::getMidiProgramName(const uint32_t index, char* const strBuf)
{
    CARLA_ASSERT(index < kData->midiprog.count);
    CARLA_ASSERT(kData->midiprog.data[index].name != nullptr);

    if (index < kData->midiprog.count && kData->midiprog.data[index].name)
        std::strncpy(strBuf, kData->midiprog.data[index].name, STR_MAX);
    else
        *strBuf = '\0';
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

    switch (type())
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
    saveState.uniqueID = uniqueId();

    // ----------------------------
    // Internals

    saveState.active = kData->active;
    saveState.dryWet = kData->postProc.dryWet;
    saveState.volume = kData->postProc.volume;
    saveState.balanceLeft  = kData->postProc.balanceLeft;
    saveState.balanceRight = kData->postProc.balanceRight;
    saveState.panning      = kData->postProc.panning;
    saveState.ctrlChannel  = kData->ctrlChannel;

    // ----------------------------
    // Chunk

    if (fOptions & PLUGIN_OPTION_USE_CHUNKS)
    {
        void* data = nullptr;
        const int32_t dataSize(chunkData(&data));

        if (data != nullptr && dataSize >= 4)
        {
            CarlaString chunkStr;
            chunkStr.importBinaryAsBase64((const uint8_t*)data, static_cast<size_t>(dataSize));

            saveState.chunk = carla_strdup(chunkStr);

            // Don't save anything else if using chunks
            return saveState;
        }
    }

    // ----------------------------
    // Current Program

    if (kData->prog.current >= 0)
    {
        saveState.currentProgramIndex = kData->prog.current;
        saveState.currentProgramName  = carla_strdup(kData->prog.names[kData->prog.current]);
    }

    // ----------------------------
    // Current MIDI Program

    if (kData->midiprog.current >= 0)
    {
        const MidiProgramData& mpData(kData->midiprog.getCurrent());

        saveState.currentMidiBank     = mpData.bank;
        saveState.currentMidiProgram  = mpData.program;
    }

    // ----------------------------
    // Parameters

    const float sampleRate(kData->engine->getSampleRate());

    for (uint32_t i=0, count=kData->param.count; i < count; i++)
    {
        const ParameterData& paramData(kData->param.data[i]);

        if ((paramData.hints & PARAMETER_IS_AUTOMABLE) == 0)
            continue;

        StateParameter* stateParameter(new StateParameter());

        stateParameter->index  = paramData.index;
        stateParameter->midiCC = paramData.midiCC;
        stateParameter->midiChannel = paramData.midiChannel + 1;

        getParameterName(i, strBuf);
        stateParameter->name = carla_strdup(strBuf);

        getParameterSymbol(i, strBuf);
        stateParameter->symbol = carla_strdup(strBuf);;

        stateParameter->value  = getParameterValue(i);

        if (paramData.hints & PARAMETER_USES_SAMPLERATE)
            stateParameter->value /= sampleRate;

        saveState.parameters.push_back(stateParameter);
    }

    // ----------------------------
    // Custom Data

    for (auto it = kData->custom.begin(); it.valid(); it.next())
    {
        const CustomData& cData(*it);

        if (cData.type == nullptr)
            continue;

        StateCustomData* stateCustomData(new StateCustomData());

        stateCustomData->type  = carla_strdup(cData.type);
        stateCustomData->key   = carla_strdup(cData.key);
        stateCustomData->value = carla_strdup(cData.value);

        saveState.customData.push_back(stateCustomData);
    }

    return saveState;
}

void CarlaPlugin::loadSaveState(const SaveState& saveState)
{
    char strBuf[STR_MAX+1];

    // ---------------------------------------------------------------------
    // Part 1 - set custom data (except binary/chunks)

    for (auto it = saveState.customData.begin(); it != saveState.customData.end(); ++it)
    {
        const StateCustomData* const stateCustomData(*it);

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_CHUNK) != 0)
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
        else if (saveState.currentProgramIndex < static_cast<int32_t>(kData->prog.count))
        {
            programId = saveState.currentProgramIndex;
        }
        // index not valid, try to find by name
        else
        {
            for (uint32_t i=0; i < kData->prog.count; i++)
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

    if (saveState.currentMidiBank >= 0 && saveState.currentMidiProgram >= 0)
        setMidiProgramById(saveState.currentMidiBank, saveState.currentMidiProgram, true, true, true);

    // ---------------------------------------------------------------------
    // Part 4a - get plugin parameter symbols

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

        ParamSymbol() = delete;
        ParamSymbol(ParamSymbol&) = delete;
        ParamSymbol(const ParamSymbol&) = delete;
    };

    QVector<ParamSymbol*> paramSymbols;

    if (type() == PLUGIN_LADSPA || type() == PLUGIN_LV2)
    {
        for (uint32_t i=0; i < kData->param.count; i++)
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

    const float sampleRate(kData->engine->getSampleRate());

    for (auto it = saveState.parameters.begin(); it != saveState.parameters.end(); ++it)
    {
        StateParameter* const stateParameter(*it);

        int32_t index = -1;

        if (type() == PLUGIN_LADSPA)
        {
            // Try to set by symbol, otherwise use index
            if (stateParameter->symbol != nullptr && *stateParameter->symbol != 0)
            {
                foreach (const ParamSymbol* paramSymbol, paramSymbols)
                {
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
        else if (type() == PLUGIN_LV2)
        {
            // Symbol only
            if (stateParameter->symbol != nullptr && *stateParameter->symbol != 0)
            {
                foreach (const ParamSymbol* paramSymbol, paramSymbols)
                {
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
        if (index >= 0 && index < static_cast<int32_t>(kData->param.count))
        {
            if (kData->param.data[index].hints & PARAMETER_USES_SAMPLERATE)
                stateParameter->value *= sampleRate;

            setParameterValue(index, stateParameter->value, true, true, true);
            setParameterMidiCC(index, stateParameter->midiCC, true, true);
            setParameterMidiChannel(index, stateParameter->midiChannel, true, true);
        }
        else
            carla_stderr("Could not set parameter data for '%s'", stateParameter->name);
    }

    // clear
    foreach (ParamSymbol* paramSymbol, paramSymbols)
    {
        paramSymbol->free();
        delete paramSymbol;
    }

    paramSymbols.clear();

    // ---------------------------------------------------------------------
    // Part 5 - set chunk data

    for (auto it = saveState.customData.begin(); it != saveState.customData.end(); ++it)
    {
        const StateCustomData* const stateCustomData(*it);

        if (std::strcmp(stateCustomData->type, CUSTOM_DATA_CHUNK) == 0)
            setCustomData(stateCustomData->type, stateCustomData->key, stateCustomData->value, true);
    }

    if (saveState.chunk != nullptr && (fOptions & PLUGIN_OPTION_USE_CHUNKS) != 0)
        setChunkData(saveState.chunk);

    // ---------------------------------------------------------------------
    // Part 6 - set internal stuff

    setDryWet(saveState.dryWet, true, true);
    setVolume(saveState.volume, true, true);
    setBalanceLeft(saveState.balanceLeft, true, true);
    setBalanceRight(saveState.balanceRight, true, true);
    setPanning(saveState.panning, true, true);
    setCtrlChannel(saveState.ctrlChannel, true, true);

    setActive(saveState.active, true, true);
}

bool CarlaPlugin::saveStateToFile(const char* const filename)
{
    carla_debug("CarlaPlugin::saveStateToFile(\"%s\")", filename);
    CARLA_ASSERT(filename != nullptr);

    QFile file(filename);

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<!DOCTYPE CARLA-PRESET>\n";
    out << "<CARLA-PRESET VERSION='1.0'>\n";
    out << getXMLFromSaveState(getSaveState());
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
        carla_stderr2("Not a valid Carla preset file");
        return false;
    }

    loadSaveState(getSaveStateDictFromXML(xmlNode));
    return true;
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setId(const unsigned int id)
{
    fId = id;
}

void CarlaPlugin::setOption(const unsigned int option, const bool yesNo)
{
    CARLA_ASSERT(availableOptions() & option);

    if (yesNo)
        fOptions |= option;
    else
        fOptions &= ~option;

    kData->saveSetting(option, yesNo);
}

void CarlaPlugin::setEnabled(const bool yesNo)
{
    fEnabled = yesNo;
}

// -------------------------------------------------------------------
// Set data (internal stuff)

void CarlaPlugin::setActive(const bool active, const bool sendOsc, const bool sendCallback)
{
    if (kData->active == active)
        return;

    if (active)
        activate();
    else
        deactivate();

    kData->active = active;

    const float value = active ? 1.0f : 0.0f;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_ACTIVE, value);
#else
    // unused
    (void)sendOsc;
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

    if (kData->postProc.dryWet == fixedValue)
        return;

    kData->postProc.dryWet = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_DRYWET, fixedValue);
#else
    // unused
    (void)sendOsc;
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

    if (kData->postProc.volume == fixedValue)
        return;

    kData->postProc.volume = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_VOLUME, fixedValue);
#else
    // unused
    (void)sendOsc;
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

    if (kData->postProc.balanceLeft == fixedValue)
        return;

    kData->postProc.balanceLeft = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_LEFT, fixedValue);
#else
    // unused
    (void)sendOsc;
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

    if (kData->postProc.balanceRight == fixedValue)
        return;

    kData->postProc.balanceRight = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_RIGHT, fixedValue);
#else
    // unused
    (void)sendOsc;
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

    if (kData->postProc.panning == fixedValue)
        return;

    kData->postProc.panning = fixedValue;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_PANNING, fixedValue);
#else
    // unused
    (void)sendOsc;
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_PANNING, 0, fixedValue, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_PANNING, fixedValue);
#endif
}

void CarlaPlugin::setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback)
{
    if (kData->ctrlChannel == channel)
        return;

    kData->ctrlChannel = channel;

#ifndef BUILD_BRIDGE
    const float ctrlf = channel;

    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_CTRL_CHANNEL, ctrlf);
#else
    // unused
    (void)sendOsc;
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, PARAMETER_CTRL_CHANNEL, 0, channel, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, PARAMETER_CTRL_CHANNEL, ctrlf);
#endif
}

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
    // unused
    (void)sendOsc;
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, fId, parameterId, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
        osc_send_control(&kData->osc.data, parameterId, value);
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
    if (rindex == PARAMETER_CTRL_CHANNEL)
        return setCtrlChannel(int8_t(value), sendOsc, sendCallback);

    for (uint32_t i=0; i < kData->param.count; i++)
    {
        if (kData->param.data[i].rindex == rindex)
            return setParameterValue(i, value, sendGui, sendOsc, sendCallback);
    }
}

void CarlaPlugin::setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < kData->param.count);
    CARLA_ASSERT_INT(channel < MAX_MIDI_CHANNELS, channel);

    if (channel >= MAX_MIDI_CHANNELS)
        channel = MAX_MIDI_CHANNELS;

    kData->param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_midi_channel(fId, parameterId, channel);
#else
    // unused
    (void)sendOsc;
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, fId, parameterId, channel, 0.0f, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
         {} // TODO
#endif
}

void CarlaPlugin::setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < kData->param.count);
    CARLA_ASSERT_INT(cc >= -1, cc);

    if (cc < -1 || cc > 0x5F)
        cc = -1;

    kData->param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
    if (sendOsc)
        kData->engine->osc_send_control_set_parameter_midi_cc(fId, parameterId, cc);
#else
    // unused
    (void)sendOsc;
#endif

    if (sendCallback)
        kData->engine->callback(CALLBACK_PARAMETER_MIDI_CC_CHANGED, fId, parameterId, cc, 0.0f, nullptr);
#ifndef BUILD_BRIDGE
    else if (fHints & PLUGIN_IS_BRIDGE)
         {} // TODO
#endif
}

void CarlaPlugin::setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
{
    CARLA_ASSERT(type != nullptr);
    CARLA_ASSERT(key != nullptr);
    CARLA_ASSERT(value != nullptr);

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
        if (std::strncmp(key, "OSC:", 4) == 0 || std::strcmp(key, "guiVisible") == 0)
            saveData = false;
        //else if (std::strcmp(key, CARLA_BRIDGE_MSG_SAVE_NOW) == 0 || std::strcmp(key, CARLA_BRIDGE_MSG_SET_CHUNK) == 0 || std::strcmp(key, CARLA_BRIDGE_MSG_SET_CUSTOM) == 0)
        //    saveData = false;
    }

    if (saveData)
    {
        // Check if we already have this key
        for (auto it = kData->custom.begin(); it.valid(); it.next())
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
        kData->custom.append(newData);
    }
}

void CarlaPlugin::setChunkData(const char* const stringData)
{
    CARLA_ASSERT(stringData != nullptr);
    return;

    // unused
    (void)stringData;
}

void CarlaPlugin::setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback)
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

void CarlaPlugin::setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback)
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

void CarlaPlugin::setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    for (uint32_t i=0; i < kData->midiprog.count; i++)
    {
        if (kData->midiprog.data[i].bank == bank && kData->midiprog.data[i].program == program)
            return setMidiProgram(i, sendGui, sendOsc, sendCallback);
    }
}

// -------------------------------------------------------------------
// Set gui stuff

void CarlaPlugin::showGui(const bool yesNo)
{
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

// -------------------------------------------------------------------
// Plugin processing

void CarlaPlugin::activate()
{
    CARLA_ASSERT(! kData->active);
}

void CarlaPlugin::deactivate()
{
    CARLA_ASSERT(kData->active);
}

void CarlaPlugin::process(float** const, float** const, const uint32_t)
{
}

void CarlaPlugin::bufferSizeChanged(const uint32_t)
{
}

void CarlaPlugin::sampleRateChanged(const double)
{
}

bool CarlaPlugin::tryLock()
{
    return kData->masterMutex.tryLock();
}

void CarlaPlugin::unlock()
{
    kData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Plugin buffers

void CarlaPlugin::initBuffers()
{
    kData->audioIn.initBuffers(kData->engine);
    kData->audioOut.initBuffers(kData->engine);
    kData->event.initBuffers(kData->engine);
}

void CarlaPlugin::clearBuffers()
{
    kData->clearBuffers();
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
        char bufName[STR_MAX+1]  = { 0 };
        char bufLabel[STR_MAX+1] = { 0 };
        char bufMaker[STR_MAX+1] = { 0 };
        char bufCopyright[STR_MAX+1] = { 0 };
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
        char bufName[STR_MAX+1], bufUnit[STR_MAX+1];

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
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_DRYWET, kData->postProc.dryWet);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_VOLUME, kData->postProc.volume);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_LEFT, kData->postProc.balanceLeft);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_BALANCE_RIGHT, kData->postProc.balanceRight);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_PANNING, kData->postProc.panning);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_CTRL_CHANNEL, kData->ctrlChannel);
        kData->engine->osc_send_control_set_parameter_value(fId, PARAMETER_ACTIVE, kData->active ? 1.0f : 0.0f);
    }
#endif
}

void CarlaPlugin::updateOscData(const lo_address& source, const char* const url)
{
    // FIXME - remove debug prints later
    carla_stdout("CarlaPlugin::updateOscData(%p, \"%s\")", source, url);

    kData->osc.data.free();

    const int proto = lo_address_get_protocol(source);

    {
        const char* host = lo_address_get_hostname(source);
        const char* port = lo_address_get_port(source);
        kData->osc.data.source = lo_address_new_with_proto(proto, host, port);

        carla_stdout("CarlaPlugin::updateOscData() - source: host \"%s\", port \"%s\"", host, port);
    }

    {
        char* host = lo_url_get_hostname(url);
        char* port = lo_url_get_port(url);
        kData->osc.data.path   = carla_strdup_free(lo_url_get_path(url));
        kData->osc.data.target = lo_address_new_with_proto(proto, host, port);
        carla_stdout("CarlaPlugin::updateOscData() - target: host \"%s\", port \"%s\", path \"%s\"", host, port, kData->osc.data.path);

        std::free(host);
        std::free(port);
    }

#ifndef BUILD_BRIDGE
    if (fHints & PLUGIN_IS_BRIDGE)
        return;
#endif

    osc_send_sample_rate(&kData->osc.data, kData->engine->getSampleRate());

    for (auto it = kData->custom.begin(); it.valid(); it.next())
    {
        const CustomData& cData(*it);

        CARLA_ASSERT(cData.type != nullptr);
        CARLA_ASSERT(cData.key != nullptr);
        CARLA_ASSERT(cData.value != nullptr);

#ifdef WANT_LV2
        if (type() == PLUGIN_LV2)
            osc_send_lv2_transfer_event(&kData->osc.data, 0, cData.type, cData.value);
        else
#endif
            if (std::strcmp(cData.type, CUSTOM_DATA_STRING) == 0)
                osc_send_configure(&kData->osc.data, cData.key, cData.value);
    }

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

    carla_stdout("CarlaPlugin::updateOscData() - done");
}

void CarlaPlugin::freeOscData()
{
    kData->osc.data.free();
}

bool CarlaPlugin::waitForOscGuiShow()
{
    carla_stdout("CarlaPlugin::waitForOscGuiShow()");
    uint i=0, oscUiTimeout = kData->engine->getOptions().oscUiTimeout;

    // wait for UI 'update' call
    for (; i < oscUiTimeout; i++)
    {
        if (kData->osc.data.target != nullptr)
        {
            carla_stdout("CarlaPlugin::waitForOscGuiShow() - got response, asking UI to show itself now");
            osc_send_show(&kData->osc.data);
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

    kData->extNotes.append(extNote);

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
    // unused
    (void)sendOsc;
#endif

    if (sendCallback)
        kData->engine->callback((velo > 0) ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, fId, channel, note, velo, nullptr);
}

void CarlaPlugin::sendMidiAllNotesOff()
{
    if (kData->ctrlChannel < 0 || kData->ctrlChannel >= MAX_MIDI_CHANNELS)
        return;

    PluginPostRtEvent postEvent;
    postEvent.type   = kPluginPostRtEventNoteOff;
    postEvent.value1 = kData->ctrlChannel;
    postEvent.value2 = 0;
    postEvent.value3 = 0.0f;

    for (unsigned short i=0; i < MAX_MIDI_NOTE; i++)
    {
        postEvent.value2 = i;
        kData->postRtEvents.appendRT(postEvent);
    }
}

// -------------------------------------------------------------------
// Post-poned events

void CarlaPlugin::postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3)
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
    const CarlaMutex::ScopedLocker sl(&kData->postRtEvents.mutex);

    while (! kData->postRtEvents.data.isEmpty())
    {
        const PluginPostRtEvent& event = kData->postRtEvents.data.getFirst(true);

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
            kData->engine->callback(CALLBACK_PROGRAM_CHANGED, fId, event.value1, 0, 0.0f, nullptr);
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
            kData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fId, event.value1, 0, 0.0f, nullptr);
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
            kData->engine->callback(CALLBACK_NOTE_OFF, fId, event.value1, event.value2, 0.0f, nullptr);
            break;
        }
    }
}

// -------------------------------------------------------------------
// Post-poned UI Stuff

void CarlaPlugin::uiParameterChange(const uint32_t index, const float value)
{
    CARLA_ASSERT(index < parameterCount());
    return;

    // unused
    (void)index;
    (void)value;
}

void CarlaPlugin::uiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < programCount());
    return;

    // unused
    (void)index;
}

void CarlaPlugin::uiMidiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < midiProgramCount());
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
    : kPlugin(plugin)
{
    carla_debug("CarlaPlugin::ScopedDisabler(%p)", plugin);
    CARLA_ASSERT(plugin != nullptr);
    CARLA_ASSERT(plugin->kData != nullptr);
    CARLA_ASSERT(plugin->kData->client != nullptr);

    if (plugin == nullptr)
        return;
    if (plugin->kData == nullptr)
        return;
    if (plugin->kData->client == nullptr)
        return;

    plugin->kData->masterMutex.lock();

    if (plugin->fEnabled)
        plugin->fEnabled = false;

    if (plugin->kData->client->isActive())
        plugin->kData->client->deactivate();
}

CarlaPlugin::ScopedDisabler::~ScopedDisabler()
{
    carla_debug("CarlaPlugin::~ScopedDisabler()");
    CARLA_ASSERT(kPlugin != nullptr);
    CARLA_ASSERT(kPlugin->kData != nullptr);
    CARLA_ASSERT(kPlugin->kData->client != nullptr);

    if (kPlugin == nullptr)
        return;
    if (kPlugin->kData == nullptr)
        return;
    if (kPlugin->kData->client == nullptr)
        return;

    kPlugin->fEnabled = true;
    kPlugin->kData->client->activate();
    kPlugin->kData->masterMutex.unlock();
}

// -------------------------------------------------------------------
// Scoped Process Locker

CarlaPlugin::ScopedSingleProcessLocker::ScopedSingleProcessLocker(CarlaPlugin* const plugin, const bool block)
    : kPlugin(plugin),
      kBlock(block)
{
    carla_debug("CarlaPlugin::ScopedSingleProcessLocker(%p, %s)", plugin, bool2str(block));
    CARLA_ASSERT(kPlugin != nullptr && kPlugin->kData != nullptr);

    if (kPlugin == nullptr)
        return;
    if (kPlugin->kData == nullptr)
        return;

    if (kBlock)
        plugin->kData->singleMutex.lock();
}

CarlaPlugin::ScopedSingleProcessLocker::~ScopedSingleProcessLocker()
{
    carla_debug("CarlaPlugin::~ScopedSingleProcessLocker()");
    CARLA_ASSERT(kPlugin != nullptr && kPlugin->kData != nullptr);

    if (kPlugin == nullptr)
        return;
    if (kPlugin->kData == nullptr)
        return;

    if (kBlock)
    {
        if (kPlugin->kData->singleMutex.wasTryLockCalled())
            kPlugin->kData->needsReset = true;

        kPlugin->kData->singleMutex.unlock();
    }
}

// -------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
