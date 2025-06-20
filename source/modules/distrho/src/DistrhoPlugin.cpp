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

#include "DistrhoPluginInternal.hpp"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Static data, see DistrhoPluginInternal.hpp */

uint32_t    d_nextBufferSize = 0;
double      d_nextSampleRate = 0.0;
const char* d_nextBundlePath = nullptr;
bool        d_nextPluginIsDummy = false;
bool        d_nextPluginIsSelfTest = false;
bool        d_nextCanRequestParameterValueChanges = false;

/* ------------------------------------------------------------------------------------------------------------
 * Static fallback data, see DistrhoPluginInternal.hpp */

const String                     PluginExporter::sFallbackString;
/* */ AudioPortWithBusId         PluginExporter::sFallbackAudioPort;
const ParameterRanges            PluginExporter::sFallbackRanges;
const ParameterEnumerationValues PluginExporter::sFallbackEnumValues;
const PortGroupWithId            PluginExporter::sFallbackPortGroup;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin */

Plugin::Plugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
    : pData(new PrivateData())
{
   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    pData->audioPorts = new AudioPortWithBusId[DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS];
   #endif

   #if defined(DPF_ABORT_ON_ERROR) || defined(DPF_RUNTIME_TESTING)
    #define DPF_ABORT abort();
   #else
    #define DPF_ABORT
   #endif

    if (parameterCount > 0)
    {
        pData->parameterCount = parameterCount;
        pData->parameters = new Parameter[parameterCount];
    }

    if (programCount > 0)
    {
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        pData->programCount = programCount;
        pData->programNames = new String[programCount];
       #else
        d_stderr2("DPF warning: Plugins with programs must define `DISTRHO_PLUGIN_WANT_PROGRAMS` to 1");
        DPF_ABORT
       #endif
    }

    if (stateCount > 0)
    {
       #if DISTRHO_PLUGIN_WANT_STATE
        pData->stateCount = stateCount;
        pData->states = new State[stateCount];
       #else
        d_stderr2("DPF warning: Plugins with state must define `DISTRHO_PLUGIN_WANT_STATE` to 1");
        DPF_ABORT
       #endif
    }

    #undef DPF_ABORT
}

Plugin::~Plugin()
{
    delete pData;
}

/* ------------------------------------------------------------------------------------------------------------
 * Host state */

uint32_t Plugin::getBufferSize() const noexcept
{
    return pData->bufferSize;
}

double Plugin::getSampleRate() const noexcept
{
    return pData->sampleRate;
}

const char* Plugin::getBundlePath() const noexcept
{
    return pData->bundlePath;
}

bool Plugin::isDummyInstance() const noexcept
{
    return pData->isDummy;
}

bool Plugin::isSelfTestInstance() const noexcept
{
    return pData->isSelfTest;
}

#if DISTRHO_PLUGIN_WANT_TIMEPOS
const TimePosition& Plugin::getTimePosition() const noexcept
{
    return pData->timePosition;
}
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
void Plugin::setLatency(const uint32_t frames) noexcept
{
    pData->latency = frames;
}
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
bool Plugin::writeMidiEvent(const MidiEvent& midiEvent) noexcept
{
    return pData->writeMidiCallback(midiEvent);
}
#endif

#if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
bool Plugin::canRequestParameterValueChanges() const noexcept
{
    return pData->canRequestParameterValueChanges;
}

bool Plugin::requestParameterValueChange(const uint32_t index, const float value) noexcept
{
    return pData->requestParameterValueChangeCallback(index, value);
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
bool Plugin::updateStateValue(const char* const key, const char* const value) noexcept
{
    return pData->updateStateValueCallback(key, value);
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * Init */

void Plugin::initAudioPort(bool input, uint32_t index, AudioPort& port)
{
    if (port.hints & kAudioPortIsCV)
    {
        port.name    = input ? "CV Input " : "CV Output ";
        port.name   += String(index+1);
        port.symbol  = input ? "cv_in_" : "cv_out_";
        port.symbol += String(index+1);
    }
    else
    {
        port.name    = input ? "Audio Input " : "Audio Output ";
        port.name   += String(index+1);
        port.symbol  = input ? "audio_in_" : "audio_out_";
        port.symbol += String(index+1);
    }
}

void Plugin::initParameter(uint32_t, Parameter&) {}

void Plugin::initPortGroup(const uint32_t groupId, PortGroup& portGroup)
{
    fillInPredefinedPortGroupData(groupId, portGroup);
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void Plugin::initProgramName(uint32_t, String&) {}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void Plugin::initState(const uint32_t index, State& state)
{
    uint hints = 0x0;
    String stateKey, defaultStateValue;

   #if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4996)
   #elif defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
   #elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
   #endif
    initState(index, stateKey, defaultStateValue);
    if (isStateFile(index))
        hints = kStateIsFilenamePath;
   #if defined(_MSC_VER)
    #pragma warning(pop)
   #elif defined(__clang__)
    #pragma clang diagnostic pop
   #elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
    #pragma GCC diagnostic pop
   #endif

    state.hints = hints;
    state.key = stateKey;
    state.label = stateKey;
    state.defaultValue = defaultStateValue;
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * Init */

float Plugin::getParameterValue(uint32_t) const { return 0.0f; }
void Plugin::setParameterValue(uint32_t, float) {}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void Plugin::loadProgram(uint32_t) {}
#endif

#if DISTRHO_PLUGIN_WANT_FULL_STATE
String Plugin::getState(const char*) const { return String(); }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void Plugin::setState(const char*, const char*) {}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * Callbacks (optional) */

void Plugin::bufferSizeChanged(uint32_t) {}
void Plugin::sampleRateChanged(double) {}
void Plugin::ioChanged(uint16_t, uint16_t) {}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
