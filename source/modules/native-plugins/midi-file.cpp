/*
 * Carla Native Plugins
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"
#include "midi-base.hpp"

#include "juce_audio_basics.h"

// -----------------------------------------------------------------------

class MidiFilePlugin : public NativePluginClass,
                       public AbstractMidiPlayer
{
public:
    MidiFilePlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fMidiOut(this),
          fWasPlayingBefore(false),
          leakDetector_MidiFilePlugin() {}

protected:
    // -------------------------------------------------------------------
    // Plugin state calls

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);

        if (std::strcmp(key, "file") != 0)
            return;

        _loadMidiFile(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float**, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        const NativeTimeInfo* const timePos(getTimeInfo());

        if (timePos->playing)
        {
            fMidiOut.play(timePos->frame, frames);
        }
        else if (fWasPlayingBefore)
        {
            NativeMidiEvent midiEvent;

            midiEvent.port    = 0;
            midiEvent.time    = 0;
            midiEvent.data[0] = MIDI_STATUS_CONTROL_CHANGE;
            midiEvent.data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            midiEvent.data[2] = 0;
            midiEvent.data[3] = 0;
            midiEvent.size    = 3;

            for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
            {
                midiEvent.data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE+i);
                NativePluginClass::writeMidiEvent(&midiEvent);
            }

            carla_stdout("WAS PLAYING BEFORE, NOW STOPPED");
        }

        fWasPlayingBefore = timePos->playing;
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (! show)
            return;

        if (const char* const filename = uiOpenFile(false, "Open Audio File", "MIDI Files *.mid;*.midi;;"))
        {
            uiCustomDataChanged("file", filename);
        }

        uiClosed();
    }

    // -------------------------------------------------------------------
    // AbstractMidiPlayer calls

    void writeMidiEvent(const uint64_t timePosFrame, const RawMidiEvent* const event) override
    {
        NativeMidiEvent midiEvent;

        midiEvent.port    = 0;
        midiEvent.time    = uint32_t(event->time-timePosFrame);
        midiEvent.size    = event->size;
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];

        NativePluginClass::writeMidiEvent(&midiEvent);
    }

private:
    MidiPattern fMidiOut;
    bool fWasPlayingBefore;

    void _loadMidiFile(const char* const filename)
    {
        fMidiOut.clear();

        using juce::File;
        using juce::FileInputStream;
        using juce::MidiFile;
        using juce::MidiMessage;
        using juce::MidiMessageSequence;
        using juce::ScopedPointer;

        File file(filename);

        if (! file.existsAsFile())
           return;

        FileInputStream fileStream(file);
        MidiFile        midiFile;

        if (! midiFile.readFrom(fileStream))
            return;

        midiFile.convertTimestampTicksToSeconds();

        const double sampleRate(getSampleRate());

        for (int i=0, numTracks = midiFile.getNumTracks(); i<numTracks; ++i)
        {
            const MidiMessageSequence* const track(midiFile.getTrack(i));
            CARLA_SAFE_ASSERT_CONTINUE(track != nullptr);

            for (int j=0, numEvents = track->getNumEvents(); j<numEvents; ++j)
            {
                const MidiMessageSequence::MidiEventHolder* const midiEventHolder(track->getEventPointer(j));
                CARLA_SAFE_ASSERT_CONTINUE(midiEventHolder != nullptr);

                const MidiMessage& midiMessage(midiEventHolder->message);
                //const double time(track->getEventTime(i)*sampleRate);
                const int dataSize(midiMessage.getRawDataSize());

                if (dataSize <= 0 || dataSize > MAX_EVENT_DATA_SIZE)
                    continue;
                if (midiMessage.isActiveSense())
                    continue;
                if (midiMessage.isMetaEvent())
                    continue;
                if (midiMessage.isMidiStart())
                    continue;
                if (midiMessage.isMidiContinue())
                    continue;
                if (midiMessage.isMidiStop())
                    continue;
                if (midiMessage.isMidiClock())
                    continue;
                if (midiMessage.isSongPositionPointer())
                    continue;
                if (midiMessage.isQuarterFrame())
                    continue;
                if (midiMessage.isFullFrame())
                    continue;
                if (midiMessage.isMidiMachineControlMessage())
                    continue;
                if (midiMessage.isSysEx())
                    continue;

                const double time(midiMessage.getTimeStamp()*sampleRate);
                CARLA_SAFE_ASSERT_CONTINUE(time >= 0.0);

                fMidiOut.addRaw(static_cast<uint64_t>(time), midiMessage.getRawData(), static_cast<uint8_t>(dataSize));
            }
        }
    }

    PluginClassEND(MidiFilePlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilePlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor midifileDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_UI|PLUGIN_NEEDS_UI_OPEN_SAVE),
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI File",
    /* label     */ "midifile",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(MidiFilePlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_midifile();

CARLA_EXPORT
void carla_register_native_plugin_midifile()
{
    carla_register_native_plugin(&midifileDesc);
}

// -----------------------------------------------------------------------
