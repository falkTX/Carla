/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_PLUGIN_UTILS_HPP_INCLUDED
#define DISTRHO_PLUGIN_UTILS_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Plugin related utilities */

/**
   @defgroup PluginRelatedUtilities Plugin related utilities

   @{
 */

/**
   Get the absolute filename of the plugin DSP/UI binary.@n
   Under certain systems or plugin formats the binary will be inside the plugin bundle.@n
   Also, in some formats or setups, the DSP and UI binaries are in different files.
*/
const char* getBinaryFilename();

/**
   Get a string representation of the current plugin format we are building against.@n
   This can be "AudioUnit", "JACK/Standalone", "LADSPA", "DSSI", "LV2", "VST2", "VST3" or "CLAP".@n
   This string is purely informational and must not be used to tweak plugin behaviour.

   @note DO NOT CHANGE PLUGIN BEHAVIOUR BASED ON PLUGIN FORMAT.
*/
const char* getPluginFormatName() noexcept;

/**
   List of supported OS-specific directories to be used for getSpecialDir.
*/
enum SpecialDir {
    /** The user "home" directory */
    kSpecialDirHome,
    /** Directory intended to store persistent configuration data (in general) */
    kSpecialDirConfig,
    /** Directory intended to store persistent configuration data for the current plugin */
    kSpecialDirConfigForPlugin,
    /** Directory intended to store "documents" (in general) */
    kSpecialDirDocuments,
    /** Directory intended to store "documents" for the current plugin */
    kSpecialDirDocumentsForPlugin,
};

/**
   Get an OS-specific directory.@n
   Calling this function will ensure the dictory exists on the filesystem.@n
   The returned path always includes a final OS separator.
*/
const char* getSpecialDir(SpecialDir dir);

/**
   Get the path to where resources are stored within the plugin bundle.@n
   Requires a valid plugin bundle path.

   Returns a path inside the bundle where the plugin is meant to store its resources in.@n
   This path varies between systems and plugin formats, like so:

    - AU: <bundle>/Contents/Resources
    - CLAP+VST2 macOS: <bundle>/Contents/Resources
    - CLAP+VST2 non-macOS: <bundle>/resources (see note)
    - LV2: <bundle>/resources (can be stored anywhere inside the bundle really, DPF just uses this one)
    - VST3: <bundle>/Contents/Resources

   The other non-mentioned formats do not support bundles.@n

   @note For CLAP and VST2 on non-macOS systems, this assumes you have your plugin inside a dedicated directory
         rather than only shipping with the binary, like so:
         @code
         + myplugin.vst/
           - myplugin.dll
           + resources/
             - image1.png
        @endcode
*/
const char* getResourcePath(const char* bundlePath) noexcept;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Plugin helper classes */

/**
   @defgroup PluginHelperClasses Plugin helper classes

   @{
 */

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
/**
   Handy class to help keep audio buffer in sync with incoming MIDI events.
   To use it, create a local variable (on the stack) and call nextEvent() until it returns false.
   @code
    for (AudioMidiSyncHelper amsh(outputs, frames, midiEvents, midiEventCount); amsh.nextEvent();)
    {
        float* const outL = amsh.outputs[0];
        float* const outR = amsh.outputs[1];

        for (uint32_t i=0; i<amsh.midiEventCount; ++i)
        {
            const MidiEvent& ev(amsh.midiEvents[i]);
            // ... do something with the midi event
        }

        renderSynth(outL, outR, amsh.frames);
    }
   @endcode

   Some important notes when using this class:
    1. MidiEvent::frame retains its original value, but it is useless, do not use it.
    2. The class variable names are the same as the default ones in the run function.
       Keep that in mind and try to avoid typos. :)
 */
struct AudioMidiSyncHelper
{
    /** Parameters from the run function, adjusted for event sync */
    float* outputs[DISTRHO_PLUGIN_NUM_OUTPUTS];
    uint32_t frames;
    const MidiEvent* midiEvents;
    uint32_t midiEventCount;

    /**
       Constructor, using values from the run function.
    */
    AudioMidiSyncHelper(float** const o, uint32_t f, const MidiEvent* m, uint32_t mc)
        : outputs(),
          frames(0),
          midiEvents(m),
          midiEventCount(0),
          remainingFrames(f),
          remainingMidiEventCount(mc),
          totalFramesUsed(0)
    {
        for (uint i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            outputs[i] = o[i];
    }

    /**
       Process a batch of events untill no more are available.
       You must not read any more values from this class after this function returns false.
    */
    bool nextEvent()
    {
        // nothing else to do
        if (remainingFrames == 0)
            return false;

        // initial setup, need to find first MIDI event
        if (totalFramesUsed == 0)
        {
            // no MIDI events at all in this process cycle
            if (remainingMidiEventCount == 0)
            {
                frames = remainingFrames;
                remainingFrames = 0;
                totalFramesUsed += frames;
                return true;
            }

            // render audio until first midi event, if needed
            if (const uint32_t firstEventFrame = midiEvents[0].frame)
            {
                DISTRHO_SAFE_ASSERT_UINT2_RETURN(firstEventFrame < remainingFrames,
                                                 firstEventFrame, remainingFrames, false);
                frames = firstEventFrame;
                remainingFrames -= firstEventFrame;
                totalFramesUsed += firstEventFrame;
                return true;
            }
        }
        else
        {
            for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                outputs[i] += frames;
        }

        // no more MIDI events available
        if (remainingMidiEventCount == 0)
        {
            frames = remainingFrames;
            midiEvents = nullptr;
            midiEventCount = 0;
            remainingFrames = 0;
            totalFramesUsed += frames;
            return true;
        }

        // if there were midi events before, increment pointer
        if (midiEventCount != 0)
            midiEvents += midiEventCount;

        const uint32_t firstEventFrame = midiEvents[0].frame;
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(firstEventFrame >= totalFramesUsed,
                                         firstEventFrame, totalFramesUsed, false);

        midiEventCount = 1;
        while (midiEventCount < remainingMidiEventCount)
        {
            if (midiEvents[midiEventCount].frame == firstEventFrame)
                ++midiEventCount;
            else
                break;
        }

        frames = firstEventFrame - totalFramesUsed;
        remainingFrames -= frames;
        remainingMidiEventCount -= midiEventCount;
        totalFramesUsed += frames;
        return true;
    }

private:
    /** @internal */
    uint32_t remainingFrames;
    uint32_t remainingMidiEventCount;
    uint32_t totalFramesUsed;
};
#endif

/** @} */

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_UTILS_HPP_INCLUDED
