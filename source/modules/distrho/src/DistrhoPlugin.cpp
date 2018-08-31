/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
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

uint32_t d_lastBufferSize = 0;
double   d_lastSampleRate = 0.0;

/* ------------------------------------------------------------------------------------------------------------
 * Static fallback data, see DistrhoPluginInternal.hpp */

const String                     PluginExporter::sFallbackString;
const AudioPort                  PluginExporter::sFallbackAudioPort;
const ParameterRanges            PluginExporter::sFallbackRanges;
const ParameterEnumerationValues PluginExporter::sFallbackEnumValues;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin */

Plugin::Plugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
    : pData(new PrivateData())
{
#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    pData->audioPorts = new AudioPort[DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS];
#endif

    if (parameterCount > 0)
    {
        pData->parameterCount = parameterCount;
        pData->parameters     = new Parameter[parameterCount];
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    if (programCount > 0)
    {
        pData->programCount = programCount;
        pData->programNames = new String[programCount];
    }
#else
    DISTRHO_SAFE_ASSERT(programCount == 0);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    if (stateCount > 0)
    {
        pData->stateCount     = stateCount;
        pData->stateKeys      = new String[stateCount];
        pData->stateDefValues = new String[stateCount];
    }
#else
    DISTRHO_SAFE_ASSERT(stateCount == 0);
#endif
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

#if DISTRHO_PLUGIN_WANT_TIMEPOS
const TimePosition& Plugin::getTimePosition() const noexcept
{
    return pData->timePosition;
}
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
void Plugin::setLatency(uint32_t frames) noexcept
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

/* ------------------------------------------------------------------------------------------------------------
 * Callbacks (optional) */

void Plugin::bufferSizeChanged(uint32_t) {}
void Plugin::sampleRateChanged(double)   {}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
