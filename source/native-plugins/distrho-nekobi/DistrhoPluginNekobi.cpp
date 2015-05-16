/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2004 Sean Bolton and others
 * Copyright (C) 2013-2015 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#include "DistrhoPluginNekobi.hpp"

extern "C" {

#include "nekobee-src/nekobee_synth.c"
#include "nekobee-src/nekobee_voice.c"
#include "nekobee-src/nekobee_voice_render.c"
#include "nekobee-src/minblep_tables.c"

// -----------------------------------------------------------------------
// mutual exclusion

bool dssp_voicelist_mutex_trylock(nekobee_synth_t* const synth)
{
    /* Attempt the mutex lock */
    if (pthread_mutex_trylock(&synth->voicelist_mutex) != 0)
    {
        synth->voicelist_mutex_grab_failed = 1;
        return false;
    }

    /* Clean up if a previous mutex grab failed */
    if (synth->voicelist_mutex_grab_failed)
    {
        nekobee_synth_all_voices_off(synth);
        synth->voicelist_mutex_grab_failed = 0;
    }

    return true;
}

bool dssp_voicelist_mutex_unlock(nekobee_synth_t* const synth)
{
    return (pthread_mutex_unlock(&synth->voicelist_mutex) == 0);
}

// -----------------------------------------------------------------------
// nekobee_handle_raw_event

void nekobee_handle_raw_event(nekobee_synth_t* const synth, const uint8_t size, const uint8_t* const data)
{
    if (size != 3)
        return;

    switch (data[0] & 0xf0)
    {
    case 0x80:
        nekobee_synth_note_off(synth, data[1], data[2]);
        break;
    case 0x90:
        if (data[2] > 0)
            nekobee_synth_note_on(synth, data[1], data[2]);
        else
            nekobee_synth_note_off(synth, data[1], 64); /* shouldn't happen, but... */
        break;
    case 0xB0:
        nekobee_synth_control_change(synth, data[1], data[2]);
        break;
    default:
        break;
    }
}

} /* extern "C" */

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

DistrhoPluginNekobi::DistrhoPluginNekobi()
    : Plugin(paramCount, 0, 0) // 0 programs, 0 states
{
    nekobee_init_tables();

    // init synth
    fSynth.sample_rate = d_getSampleRate();
    fSynth.deltat = 1.0f / (float)d_getSampleRate();
    fSynth.nugget_remains = 0;

    fSynth.note_id = 0;
    fSynth.polyphony = XSYNTH_DEFAULT_POLYPHONY;
    fSynth.voices = XSYNTH_DEFAULT_POLYPHONY;
    fSynth.monophonic = XSYNTH_MONO_MODE_ONCE;
    fSynth.glide = 0;
    fSynth.last_noteon_pitch = 0.0f;
    fSynth.vcf_accent = 0.0f;
    fSynth.vca_accent = 0.0f;

    for (int i=0; i<8; ++i)
        fSynth.held_keys[i] = -1;

    fSynth.voice = nekobee_voice_new();
    fSynth.voicelist_mutex_grab_failed = 0;
    pthread_mutex_init(&fSynth.voicelist_mutex, nullptr);

    fSynth.channel_pressure = 0;
    fSynth.pitch_wheel_sensitivity = 0;
    fSynth.pitch_wheel = 0;

    for (int i=0; i<128; ++i)
    {
        fSynth.key_pressure[i] = 0;
        fSynth.cc[i] = 0;
    }
    fSynth.cc[7] = 127; // full volume

    fSynth.mod_wheel  = 1.0f;
    fSynth.pitch_bend = 1.0f;
    fSynth.cc_volume  = 1.0f;

    // Default values
    fParams.waveform = 0.0f;
    fParams.tuning = 0.0f;
    fParams.cutoff = 25.0f;
    fParams.resonance = 25.0f;
    fParams.envMod = 50.0f;
    fParams.decay  = 75.0f;
    fParams.accent = 25.0f;
    fParams.volume = 75.0f;

    // Internal stuff
    fSynth.waveform  = 0.0f;
    fSynth.tuning    = 1.0f;
    fSynth.cutoff    = 5.0f;
    fSynth.resonance = 0.8f;
    fSynth.envmod    = 0.3f;
    fSynth.decay     = 0.0002f;
    fSynth.accent    = 0.3f;
    fSynth.volume    = 0.75f;

    // reset
    d_deactivate();
}

DistrhoPluginNekobi::~DistrhoPluginNekobi()
{
    std::free(fSynth.voice);
}

// -----------------------------------------------------------------------
// Init

void DistrhoPluginNekobi::d_initParameter(uint32_t index, Parameter& parameter)
{
    switch (index)
    {
    case paramWaveform:
        parameter.hints      = kParameterIsAutomable|kParameterIsBoolean;
        parameter.name       = "Waveform";
        parameter.symbol     = "waveform";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    case paramTuning:
        parameter.hints      = kParameterIsAutomable; // was 0.5 <-> 2.0, log
        parameter.name       = "Tuning";
        parameter.symbol     = "tuning";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -12.0f;
        parameter.ranges.max = 12.0f;
        break;
    case paramCutoff:
        parameter.hints      = kParameterIsAutomable; // modified x2.5
        parameter.name       = "Cutoff";
        parameter.symbol     = "cutoff";
        parameter.unit       = "%";
        parameter.ranges.def = 25.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 100.0f;
        break;
    case paramResonance:
        parameter.hints      = kParameterIsAutomable; // modified x100
        parameter.name       = "VCF Resonance";
        parameter.symbol     = "resonance";
        parameter.unit       = "%";
        parameter.ranges.def = 25.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 95.0f;
        break;
    case paramEnvMod:
        parameter.hints      = kParameterIsAutomable; // modified x100
        parameter.name       = "Env Mod";
        parameter.symbol     = "env_mod";
        parameter.unit       = "%";
        parameter.ranges.def = 50.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 100.0f;
        break;
    case paramDecay:
        parameter.hints      = kParameterIsAutomable; // was 0.000009 <-> 0.0005, log
        parameter.name       = "Decay";
        parameter.symbol     = "decay";
        parameter.unit       = "%";
        parameter.ranges.def = 75.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 100.0f;
        break;
    case paramAccent:
        parameter.hints      = kParameterIsAutomable; // modified x100
        parameter.name       = "Accent";
        parameter.symbol     = "accent";
        parameter.unit       = "%";
        parameter.ranges.def = 25.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 100.0f;
        break;
    case paramVolume:
        parameter.hints      = kParameterIsAutomable; // modified x100
        parameter.name       = "Volume";
        parameter.symbol     = "volume";
        parameter.unit       = "%";
        parameter.ranges.def = 75.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 100.0f;
        break;
    }
}

// -----------------------------------------------------------------------
// Internal data

float DistrhoPluginNekobi::d_getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case paramWaveform:
        return fParams.waveform;
    case paramTuning:
        return fParams.tuning;
    case paramCutoff:
        return fParams.cutoff;
    case paramResonance:
        return fParams.resonance;
    case paramEnvMod:
        return fParams.envMod;
    case paramDecay:
        return fParams.decay;
    case paramAccent:
        return fParams.accent;
    case paramVolume:
        return fParams.volume;
    }

    return 0.0f;
}

void DistrhoPluginNekobi::d_setParameterValue(uint32_t index, float value)
{
    switch (index)
    {
    case paramWaveform:
        fParams.waveform = value;
        fSynth.waveform = value;
        DISTRHO_SAFE_ASSERT(fSynth.waveform == 0.0f || fSynth.waveform == 1.0f);
        break;
    case paramTuning:
        fParams.tuning = value;
        fSynth.tuning = (value+12.0f)/24.0f * 1.5 + 0.5f; // FIXME: log?
        DISTRHO_SAFE_ASSERT(fSynth.tuning >= 0.5f && fSynth.tuning <= 2.0f);
        break;
    case paramCutoff:
        fParams.cutoff = value;
        fSynth.cutoff = value/2.5f;
        DISTRHO_SAFE_ASSERT(fSynth.cutoff >= 0.0f && fSynth.cutoff <= 40.0f);
        break;
    case paramResonance:
        fParams.resonance = value;
        fSynth.resonance = value/100.0f;
        DISTRHO_SAFE_ASSERT(fSynth.resonance >= 0.0f && fSynth.resonance <= 0.95f);
        break;
    case paramEnvMod:
        fParams.envMod = value;
        fSynth.envmod = value/100.0f;
        DISTRHO_SAFE_ASSERT(fSynth.envmod >= 0.0f && fSynth.envmod <= 1.0f);
        break;
    case paramDecay:
        fParams.decay = value;
        fSynth.decay = value/100.0f * 0.000491f + 0.000009f; // FIXME: log?
        DISTRHO_SAFE_ASSERT(fSynth.decay >= 0.000009f && fSynth.decay <= 0.0005f);
        break;
    case paramAccent:
        fParams.accent = value;
        fSynth.accent = value/100.0f;
        DISTRHO_SAFE_ASSERT(fSynth.accent >= 0.0f && fSynth.accent <= 1.0f);
        break;
    case paramVolume:
        fParams.volume = value;
        fSynth.volume = value/100.0f;
        DISTRHO_SAFE_ASSERT(fSynth.volume >= 0.0f && fSynth.volume <= 1.0f);
        break;
    }
}

// -----------------------------------------------------------------------
// Process

void DistrhoPluginNekobi::d_activate()
{
    fSynth.nugget_remains = 0;
    fSynth.note_id = 0;

    if (fSynth.voice != nullptr)
        nekobee_synth_all_voices_off(&fSynth);
}

void DistrhoPluginNekobi::d_deactivate()
{
    if (fSynth.voice != nullptr)
        nekobee_synth_all_voices_off(&fSynth);
}

void DistrhoPluginNekobi::d_run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    uint32_t framesDone = 0;
    uint32_t curEventIndex = 0;
    uint32_t burstSize;

    float* out = outputs[0];

    if (fSynth.voice == nullptr || ! dssp_voicelist_mutex_trylock(&fSynth))
    {
        std::memset(out, 0, sizeof(float)*frames);
        return;
    }

    while (framesDone < frames)
    {
        if (fSynth.nugget_remains == 0)
            fSynth.nugget_remains = XSYNTH_NUGGET_SIZE;

        /* process any ready events */
        while (curEventIndex < midiEventCount && framesDone == midiEvents[curEventIndex].frame)
        {
            if (midiEvents[curEventIndex].size > MidiEvent::kDataSize)
                continue;

            nekobee_handle_raw_event(&fSynth, midiEvents[curEventIndex].size, midiEvents[curEventIndex].data);
            curEventIndex++;
        }

        /* calculate the sample count (burstSize) for the next nekobee_voice_render() call to be the smallest of:
         * - control calculation quantization size (XSYNTH_NUGGET_SIZE, in samples)
         * - the number of samples remaining in an already-begun nugget (synth->nugget_remains)
         * - the number of samples until the next event is ready
         * - the number of samples left in this run
         */
        burstSize = XSYNTH_NUGGET_SIZE;

        /* we're still in the middle of a nugget, so reduce the burst size
         * to end when the nugget ends */
        if (fSynth.nugget_remains < burstSize)
            burstSize = fSynth.nugget_remains;

        /* reduce burst size to end when next event is ready */
        if (curEventIndex < midiEventCount && midiEvents[curEventIndex].frame - framesDone < burstSize)
            burstSize = midiEvents[curEventIndex].frame - framesDone;

        /* reduce burst size to end at end of this run */
        if (frames - framesDone < burstSize)
            burstSize = frames - framesDone;

        /* render the burst */
        nekobee_synth_render_voices(&fSynth, out + framesDone, burstSize, (burstSize == fSynth.nugget_remains));
        framesDone += burstSize;
        fSynth.nugget_remains -= burstSize;
    }

    dssp_voicelist_mutex_unlock(&fSynth);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new DistrhoPluginNekobi();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
