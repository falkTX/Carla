/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

const d_string        PluginExporter::sFallbackString;
const ParameterRanges PluginExporter::sFallbackRanges;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin */

Plugin::Plugin(const uint32_t parameterCount, const uint32_t programCount, const uint32_t stateCount)
    : pData(new PrivateData())
{
    if (parameterCount > 0)
    {
        pData->parameterCount = parameterCount;
        pData->parameters     = new Parameter[parameterCount];
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    if (programCount > 0)
    {
        pData->programCount = programCount;
        pData->programNames = new d_string[programCount];
    }
#else
    DISTRHO_SAFE_ASSERT(programCount == 0);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    if (stateCount > 0)
    {
        pData->stateCount     = stateCount;
        pData->stateKeys      = new d_string[stateCount];
        pData->stateDefValues = new d_string[stateCount];
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

uint32_t Plugin::d_getBufferSize() const noexcept
{
    return pData->bufferSize;
}

double Plugin::d_getSampleRate() const noexcept
{
    return pData->sampleRate;
}

#if DISTRHO_PLUGIN_WANT_TIMEPOS
const TimePosition& Plugin::d_getTimePosition() const noexcept
{
    return pData->timePosition;
}
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
void Plugin::d_setLatency(const uint32_t frames) noexcept
{
    pData->latency = frames;
}
#endif

#if DISTRHO_PLUGIN_HAS_MIDI_OUTPUT
bool Plugin::d_writeMidiEvent(const MidiEvent& /*midiEvent*/) noexcept
{
    // TODO
    return false;
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * Callbacks (optional) */

void Plugin::d_bufferSizeChanged(uint32_t) {}
void Plugin::d_sampleRateChanged(double)   {}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
