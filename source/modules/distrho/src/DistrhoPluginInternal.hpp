/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED
#define DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED

#include "../DistrhoPlugin.hpp"

#ifdef DISTRHO_PLUGIN_TARGET_VST3
# include "DistrhoPluginVST.hpp"
#endif

#include <set>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Maxmimum values

static const uint32_t kMaxMidiEvents = 512;

// -----------------------------------------------------------------------
// Static data, see DistrhoPlugin.cpp

extern uint32_t    d_nextBufferSize;
extern double      d_nextSampleRate;
extern const char* d_nextBundlePath;
extern bool        d_nextPluginIsDummy;
extern bool        d_nextPluginIsSelfTest;
extern bool        d_nextCanRequestParameterValueChanges;

// -----------------------------------------------------------------------
// DSP callbacks

typedef bool (*writeMidiFunc) (void* ptr, const MidiEvent& midiEvent);
typedef bool (*requestParameterValueChangeFunc) (void* ptr, uint32_t index, float value);
typedef bool (*updateStateValueFunc) (void* ptr, const char* key, const char* value);

// -----------------------------------------------------------------------
// Helpers

struct AudioPortWithBusId : AudioPort {
    uint32_t busId;

    AudioPortWithBusId()
        : AudioPort(),
          busId(0) {}
};

struct PortGroupWithId : PortGroup {
    uint32_t groupId;

    PortGroupWithId()
        : PortGroup(),
          groupId(kPortGroupNone) {}
};

static inline
void fillInPredefinedPortGroupData(const uint32_t groupId, PortGroup& portGroup)
{
    switch (groupId)
    {
    case kPortGroupNone:
        portGroup.name.clear();
        portGroup.symbol.clear();
        break;
    case kPortGroupMono:
        portGroup.name = "Mono";
        portGroup.symbol = "dpf_mono";
        break;
    case kPortGroupStereo:
        portGroup.name = "Stereo";
        portGroup.symbol = "dpf_stereo";
        break;
    }
}

static inline
void d_strncpy(char* const dst, const char* const src, const size_t length)
{
    DISTRHO_SAFE_ASSERT_RETURN(length > 0,);

    if (const size_t len = std::min(std::strlen(src), length-1U))
    {
        std::memcpy(dst, src, len);
        dst[len] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }
}

template<typename T>
static inline
void snprintf_t(char* const dst, const T value, const char* const format, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);
    std::snprintf(dst, size-1, format, value);
    dst[size-1] = '\0';
}

static inline
void snprintf_f32(char* const dst, const float value, const size_t size)
{
    return snprintf_t<float>(dst, value, "%f", size);
}

static inline
void snprintf_f32(char* const dst, const double value, const size_t size)
{
    return snprintf_t<double>(dst, value, "%f", size);
}

static inline
void snprintf_i32(char* const dst, const int32_t value, const size_t size)
{
    return snprintf_t<int32_t>(dst, value, "%d", size);
}

static inline
void snprintf_u32(char* const dst, const uint32_t value, const size_t size)
{
    return snprintf_t<uint32_t>(dst, value, "%u", size);
}

// -----------------------------------------------------------------------
// Plugin private data

struct Plugin::PrivateData {
    const bool canRequestParameterValueChanges;
    const bool isDummy;
    const bool isSelfTest;
    bool isProcessing;

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    AudioPortWithBusId* audioPorts;
#endif

    uint32_t   parameterCount;
    uint32_t   parameterOffset;
    Parameter* parameters;

    uint32_t         portGroupCount;
    PortGroupWithId* portGroups;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t programCount;
    String*  programNames;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t stateCount;
    State*   states;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t latency;
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition timePosition;
#endif

    // Callbacks
    void*         callbacksPtr;
    writeMidiFunc writeMidiCallbackFunc;
    requestParameterValueChangeFunc requestParameterValueChangeCallbackFunc;
    updateStateValueFunc updateStateValueCallbackFunc;

    uint32_t bufferSize;
    double   sampleRate;
    char*    bundlePath;

    PrivateData() noexcept
        : canRequestParameterValueChanges(d_nextCanRequestParameterValueChanges),
          isDummy(d_nextPluginIsDummy),
          isSelfTest(d_nextPluginIsSelfTest),
          isProcessing(false),
#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
          audioPorts(nullptr),
#endif
          parameterCount(0),
          parameterOffset(0),
          parameters(nullptr),
          portGroupCount(0),
          portGroups(nullptr),
#if DISTRHO_PLUGIN_WANT_PROGRAMS
          programCount(0),
          programNames(nullptr),
#endif
#if DISTRHO_PLUGIN_WANT_STATE
          stateCount(0),
          states(nullptr),
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
          latency(0),
#endif
          callbacksPtr(nullptr),
          writeMidiCallbackFunc(nullptr),
          requestParameterValueChangeCallbackFunc(nullptr),
          updateStateValueCallbackFunc(nullptr),
          bufferSize(d_nextBufferSize),
          sampleRate(d_nextSampleRate),
          bundlePath(d_nextBundlePath != nullptr ? strdup(d_nextBundlePath) : nullptr)
    {
        DISTRHO_SAFE_ASSERT(bufferSize != 0);
        DISTRHO_SAFE_ASSERT(d_isNotZero(sampleRate));

#if defined(DISTRHO_PLUGIN_TARGET_DSSI) || defined(DISTRHO_PLUGIN_TARGET_LV2)
        parameterOffset += DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;
# if DISTRHO_PLUGIN_WANT_LATENCY
        parameterOffset += 1;
# endif
#endif

#ifdef DISTRHO_PLUGIN_TARGET_LV2
# if (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_STATE || DISTRHO_PLUGIN_WANT_TIMEPOS)
        parameterOffset += 1;
# endif
# if (DISTRHO_PLUGIN_WANT_MIDI_OUTPUT || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
# endif
#endif

#ifdef DISTRHO_PLUGIN_TARGET_VST3
        parameterOffset += kVst3InternalParameterCount;
#endif
    }

    ~PrivateData() noexcept
    {
#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        if (audioPorts != nullptr)
        {
            delete[] audioPorts;
            audioPorts = nullptr;
        }
#endif

        if (parameters != nullptr)
        {
            delete[] parameters;
            parameters = nullptr;
        }

        if (portGroups != nullptr)
        {
            delete[] portGroups;
            portGroups = nullptr;
        }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (programNames != nullptr)
        {
            delete[] programNames;
            programNames = nullptr;
        }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        if (states != nullptr)
        {
            delete[] states;
            states = nullptr;
        }
#endif

        if (bundlePath != nullptr)
        {
            std::free(bundlePath);
            bundlePath = nullptr;
        }
    }

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidiCallback(const MidiEvent& midiEvent)
    {
        if (writeMidiCallbackFunc != nullptr)
            return writeMidiCallbackFunc(callbacksPtr, midiEvent);

        return false;
    }
#endif

#if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChangeCallback(const uint32_t index, const float value)
    {
        if (requestParameterValueChangeCallbackFunc != nullptr)
            return requestParameterValueChangeCallbackFunc(callbacksPtr, index, value);

        return false;
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    bool updateStateValueCallback(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', false);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr, false);

        d_stdout("updateStateValueCallback %p", updateStateValueCallbackFunc);
        if (updateStateValueCallbackFunc != nullptr)
            return updateStateValueCallbackFunc(callbacksPtr, key, value);

        return false;
    }
#endif
};

// -----------------------------------------------------------------------
// Plugin exporter class

class PluginExporter
{
public:
    PluginExporter(void* const callbacksPtr,
                   const writeMidiFunc writeMidiCall,
                   const requestParameterValueChangeFunc requestParameterValueChangeCall,
                   const updateStateValueFunc updateStateValueCall)
        : fPlugin(createPlugin()),
          fData((fPlugin != nullptr) ? fPlugin->pData : nullptr),
          fIsActive(false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        {
            uint32_t j=0;
# if DISTRHO_PLUGIN_NUM_INPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i, ++j)
                fPlugin->initAudioPort(true, i, fData->audioPorts[j]);
# endif
# if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i, ++j)
                fPlugin->initAudioPort(false, i, fData->audioPorts[j]);
# endif
        }
#endif // DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0

        for (uint32_t i=0, count=fData->parameterCount; i < count; ++i)
            fPlugin->initParameter(i, fData->parameters[i]);

        {
            std::set<uint32_t> portGroupIndices;

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                portGroupIndices.insert(fData->audioPorts[i].groupId);
#endif
            for (uint32_t i=0, count=fData->parameterCount; i < count; ++i)
                portGroupIndices.insert(fData->parameters[i].groupId);

            portGroupIndices.erase(kPortGroupNone);

            if (const uint32_t portGroupSize = static_cast<uint32_t>(portGroupIndices.size()))
            {
                fData->portGroups = new PortGroupWithId[portGroupSize];
                fData->portGroupCount = portGroupSize;

                uint32_t index = 0;
                for (std::set<uint32_t>::iterator it = portGroupIndices.begin(); it != portGroupIndices.end(); ++it, ++index)
                {
                    PortGroupWithId& portGroup(fData->portGroups[index]);
                    portGroup.groupId = *it;

                    if (portGroup.groupId < portGroupSize)
                        fPlugin->initPortGroup(portGroup.groupId, portGroup);
                    else
                        fillInPredefinedPortGroupData(portGroup.groupId, portGroup);
                }
            }
        }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        for (uint32_t i=0; i < fData->programCount; ++i)
            fPlugin->initProgramName(i, fData->programNames[i]);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0; i < fData->stateCount; ++i)
            fPlugin->initState(i, fData->states[i]);
#endif

#if defined(DPF_RUNTIME_TESTING) && defined(__GNUC__) && !defined(__clang__)
        /* Run-time testing build.
         * Verify that virtual functions are overriden if parameters, programs or states are in use.
         * This does not work on all compilers, but we use it purely as informational check anyway. */
        if (fData->parameterCount != 0)
        {
            if ((void*)(fPlugin->*(&Plugin::initParameter)) == (void*)&Plugin::initParameter)
            {
                d_stderr2("DPF warning: Plugins with parameters must implement `initParameter`");
                abort();
            }
            if ((void*)(fPlugin->*(&Plugin::getParameterValue)) == (void*)&Plugin::getParameterValue)
            {
                d_stderr2("DPF warning: Plugins with parameters must implement `getParameterValue`");
                abort();
            }
            if ((void*)(fPlugin->*(&Plugin::setParameterValue)) == (void*)&Plugin::setParameterValue)
            {
                d_stderr2("DPF warning: Plugins with parameters must implement `setParameterValue`");
                abort();
            }
        }

# if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (fData->programCount != 0)
        {
            if ((void*)(fPlugin->*(&Plugin::initProgramName)) == (void*)&Plugin::initProgramName)
            {
                d_stderr2("DPF warning: Plugins with programs must implement `initProgramName`");
                abort();
            }
            if ((void*)(fPlugin->*(&Plugin::loadProgram)) == (void*)&Plugin::loadProgram)
            {
                d_stderr2("DPF warning: Plugins with programs must implement `loadProgram`");
                abort();
            }
        }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
        if (fData->stateCount != 0)
        {
            bool hasNonUiState = false;
            for (uint32_t i=0; i < fData->stateCount; ++i)
            {
                if ((fData->states[i].hints & kStateIsOnlyForUI) == 0)
                {
                    hasNonUiState = true;
                    break;
                }
            }

            if (hasNonUiState)
            {
                if ((void*)(fPlugin->*(static_cast<void(Plugin::*)(uint32_t,State&)>(&Plugin::initState))) ==
                    (void*)static_cast<void(Plugin::*)(uint32_t,State&)>(&Plugin::initState))
                {
                    d_stderr2("DPF warning: Plugins with state must implement `initState`");
                    abort();
                }

                if ((void*)(fPlugin->*(&Plugin::setState)) == (void*)&Plugin::setState)
                {
                    d_stderr2("DPF warning: Plugins with state must implement `setState`");
                    abort();
                }
            }
        }
# endif

# if DISTRHO_PLUGIN_WANT_FULL_STATE
        if (fData->stateCount != 0)
        {
            if ((void*)(fPlugin->*(&Plugin::getState)) == (void*)&Plugin::getState)
            {
                d_stderr2("DPF warning: Plugins with full state must implement `getState`");
                abort();
            }
        }
        else
        {
            d_stderr2("DPF warning: Plugins with full state must have at least 1 state");
            abort();
        }
# endif
#endif

        fData->callbacksPtr = callbacksPtr;
        fData->writeMidiCallbackFunc = writeMidiCall;
        fData->requestParameterValueChangeCallbackFunc = requestParameterValueChangeCall;
        fData->updateStateValueCallbackFunc = updateStateValueCall;
    }

    ~PluginExporter()
    {
        delete fPlugin;
    }

    // -------------------------------------------------------------------

    const char* getName() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->getName();
    }

    const char* getLabel() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->getLabel();
    }

    const char* getDescription() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->getDescription();
    }

    const char* getMaker() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->getMaker();
    }

    const char* getHomePage() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->getHomePage();
    }

    const char* getLicense() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, "");

        return fPlugin->getLicense();
    }

    uint32_t getVersion() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0);

        return fPlugin->getVersion();
    }

    long getUniqueId() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0);

        return fPlugin->getUniqueId();
    }

    void* getInstancePointer() const noexcept
    {
        return fPlugin;
    }

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t getLatency() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->latency;
    }
#endif

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    AudioPortWithBusId& getAudioPort(const bool input, const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, sFallbackAudioPort);

        if (input)
        {
# if DISTRHO_PLUGIN_NUM_INPUTS > 0
            DISTRHO_SAFE_ASSERT_RETURN(index < DISTRHO_PLUGIN_NUM_INPUTS,  sFallbackAudioPort);
# endif
        }
        else
        {
# if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            DISTRHO_SAFE_ASSERT_RETURN(index < DISTRHO_PLUGIN_NUM_OUTPUTS, sFallbackAudioPort);
# endif
        }

        return fData->audioPorts[index + (input ? 0 : DISTRHO_PLUGIN_NUM_INPUTS)];
    }

    uint32_t getAudioPortHints(const bool input, const uint32_t index) const noexcept
    {
        return getAudioPort(input, index).hints;
    }
    
    uint32_t getAudioPortCountWithGroupId(const bool input, const uint32_t groupId) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        uint32_t numPorts = 0;

        if (input)
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS > 0
            for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            {
                if (fData->audioPorts[i].groupId == groupId)
                    ++numPorts;
            }
           #endif
        }
        else
        {
           #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            {
                if (fData->audioPorts[i + DISTRHO_PLUGIN_NUM_INPUTS].groupId == groupId)
                    ++numPorts;
            }
           #endif
        }

        return numPorts;
    }
#endif

    uint32_t getParameterCount() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->parameterCount;
    }

    uint32_t getParameterOffset() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->parameterOffset;
    }

    uint32_t getParameterHints(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, 0x0);

        return fData->parameters[index].hints;
    }

    ParameterDesignation getParameterDesignation(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, kParameterDesignationNull);

        return fData->parameters[index].designation;
    }

    bool isParameterInput(const uint32_t index) const noexcept
    {
        return (getParameterHints(index) & kParameterIsOutput) == 0x0;
    }

    bool isParameterOutput(const uint32_t index) const noexcept
    {
        return (getParameterHints(index) & kParameterIsOutput) != 0x0;
    }

    bool isParameterInteger(const uint32_t index) const noexcept
    {
        return (getParameterHints(index) & kParameterIsInteger) != 0x0;
    }

    bool isParameterTrigger(const uint32_t index) const noexcept
    {
        return (getParameterHints(index) & kParameterIsTrigger) == kParameterIsTrigger;
    }

    bool isParameterOutputOrTrigger(const uint32_t index) const noexcept
    {
        const uint32_t hints = getParameterHints(index);

        if (hints & kParameterIsOutput)
            return true;
        if ((hints & kParameterIsTrigger) == kParameterIsTrigger)
            return true;

        return false;
    }

    const String& getParameterName(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].name;
    }

    const String& getParameterShortName(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].shortName;
    }

    const String& getParameterSymbol(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].symbol;
    }

    const String& getParameterUnit(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].unit;
    }

    const String& getParameterDescription(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackString);

        return fData->parameters[index].description;
    }

    const ParameterEnumerationValues& getParameterEnumValues(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackEnumValues);

        return fData->parameters[index].enumValues;
    }

    const ParameterRanges& getParameterRanges(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, sFallbackRanges);

        return fData->parameters[index].ranges;
    }

    uint8_t getParameterMidiCC(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, 0);

        return fData->parameters[index].midiCC;
    }

    uint32_t getParameterGroupId(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, kPortGroupNone);

        return fData->parameters[index].groupId;
    }

    float getParameterDefault(const uint32_t index) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0.0f);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, 0.0f);

        return fData->parameters[index].ranges.def;
    }

    float getParameterValue(const uint32_t index) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, 0.0f);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount, 0.0f);

        return fPlugin->getParameterValue(index);
    }

    void setParameterValue(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->parameterCount,);

        fPlugin->setParameterValue(index, value);
    }

    /*
    bool getParameterIndexForSymbol(const char* const symbol, uint32_t& index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, false);

        for (uint32_t i=0; i < fData->parameterCount; ++i)
        {
            if (fData->parameters[i].symbol == symbol)
            {
                index = i;
                return true;
            }
        }

        return false;
    }
    */

    uint32_t getPortGroupCount() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->portGroupCount;
    }

    const PortGroupWithId& getPortGroupById(const uint32_t groupId) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && fData->portGroupCount != 0, sFallbackPortGroup);

        for (uint32_t i=0; i < fData->portGroupCount; ++i)
        {
            const PortGroupWithId& portGroup(fData->portGroups[i]);

            if (portGroup.groupId == groupId)
                return portGroup;
        }

        return sFallbackPortGroup;
    }

    const PortGroupWithId& getPortGroupByIndex(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->portGroupCount, sFallbackPortGroup);

        return fData->portGroups[index];
    }

    const String& getPortGroupSymbolForId(const uint32_t groupId) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, sFallbackString);

        return getPortGroupById(groupId).symbol;
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t getProgramCount() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->programCount;
    }

    const String& getProgramName(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->programCount, sFallbackString);

        return fData->programNames[index];
    }

    void loadProgram(const uint32_t index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->programCount,);

        fPlugin->loadProgram(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    uint32_t getStateCount() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->stateCount;
    }

    uint32_t getStateHints(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, 0x0);

        return fData->states[index].hints;
    }

    const String& getStateKey(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, sFallbackString);

        return fData->states[index].key;
    }

    const String& getStateDefaultValue(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, sFallbackString);

        return fData->states[index].defaultValue;
    }

    const String& getStateLabel(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, sFallbackString);

        return fData->states[index].label;
    }

    const String& getStateDescription(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, sFallbackString);

        return fData->states[index].description;
    }

   #ifdef __MOD_DEVICES__
    const String& getStateFileTypes(const uint32_t index) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr && index < fData->stateCount, sFallbackString);

        return fData->states[index].fileTypes;
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_FULL_STATE
    String getStateValue(const char* const key) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, sFallbackString);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', sFallbackString);

        return fPlugin->getState(key);
    }
   #endif

    void setState(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

        fPlugin->setState(key, value);
    }

    bool wantStateKey(const char* const key) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', false);

        for (uint32_t i=0; i < fData->stateCount; ++i)
        {
            if (fData->states[i].key == key)
                return true;
        }

        return false;
    }
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    void setTimePosition(const TimePosition& timePosition) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        std::memcpy(&fData->timePosition, &timePosition, sizeof(TimePosition));
    }
#endif

    // -------------------------------------------------------------------

    bool isActive() const noexcept
    {
        return fIsActive;
    }

    void activate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(! fIsActive,);

        fIsActive = true;
        fPlugin->activate();
    }

    void deactivate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fIsActive,);

        fIsActive = false;
        fPlugin->deactivate();
    }

    void deactivateIfNeeded()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        if (fIsActive)
        {
            fIsActive = false;
            fPlugin->deactivate();
        }
    }

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void run(const float** const inputs, float** const outputs, const uint32_t frames,
             const MidiEvent* const midiEvents, const uint32_t midiEventCount)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        if (! fIsActive)
        {
            fIsActive = true;
            fPlugin->activate();
        }

        fData->isProcessing = true;
        fPlugin->run(inputs, outputs, frames, midiEvents, midiEventCount);
        fData->isProcessing = false;
    }
   #else
    void run(const float** const inputs, float** const outputs, const uint32_t frames)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        if (! fIsActive)
        {
            fIsActive = true;
            fPlugin->activate();
        }

        fData->isProcessing = true;
        fPlugin->run(inputs, outputs, frames);
        fData->isProcessing = false;
    }
   #endif

    // -------------------------------------------------------------------

   #ifdef DISTRHO_PLUGIN_TARGET_AU
    void setAudioPortIO(const uint16_t numInputs, const uint16_t numOutputs)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        if (fIsActive) fPlugin->deactivate();
        fPlugin->ioChanged(numInputs, numOutputs);
        if (fIsActive) fPlugin->activate();
    }
   #endif

    // -------------------------------------------------------------------

    uint32_t getBufferSize() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);
        return fData->bufferSize;
    }

    double getSampleRate() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0.0);
        return fData->sampleRate;
    }

    bool setBufferSize(const uint32_t bufferSize, const bool doCallback = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr, false);
        DISTRHO_SAFE_ASSERT(bufferSize >= 2);

        if (fData->bufferSize == bufferSize)
            return false;

        fData->bufferSize = bufferSize;

        if (doCallback)
        {
            if (fIsActive) fPlugin->deactivate();
            fPlugin->bufferSizeChanged(bufferSize);
            if (fIsActive) fPlugin->activate();
        }

        return true;
    }

    void setSampleRate(const double sampleRate, const bool doCallback = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        DISTRHO_SAFE_ASSERT(sampleRate > 0.0);

        if (d_isEqual(fData->sampleRate, sampleRate))
            return;

        fData->sampleRate = sampleRate;

        if (doCallback)
        {
            if (fIsActive) fPlugin->deactivate();
            fPlugin->sampleRateChanged(sampleRate);
            if (fIsActive) fPlugin->activate();
        }
    }

private:
    // -------------------------------------------------------------------
    // Plugin and DistrhoPlugin data

    Plugin* const fPlugin;
    Plugin::PrivateData* const fData;
    bool fIsActive;

    // -------------------------------------------------------------------
    // Static fallback data, see DistrhoPlugin.cpp

    static const String                     sFallbackString;
    static /* */ AudioPortWithBusId         sFallbackAudioPort;
    static const ParameterRanges            sFallbackRanges;
    static const ParameterEnumerationValues sFallbackEnumValues;
    static const PortGroupWithId            sFallbackPortGroup;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginExporter)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_INTERNAL_HPP_INCLUDED
