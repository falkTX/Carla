/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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
          fNeedsAllNotesOff(false),
          fWasPlayingBefore(false) {}

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

        if (fWasPlayingBefore != timePos->playing)
        {
            fNeedsAllNotesOff = true;
            fWasPlayingBefore = timePos->playing;
        }

        if (fNeedsAllNotesOff)
        {
            NativeMidiEvent midiEvent;

            midiEvent.port    = 0;
            midiEvent.time    = 0;
            midiEvent.data[0] = 0;
            midiEvent.data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            midiEvent.data[2] = 0;
            midiEvent.data[3] = 0;
            midiEvent.size    = 3;

            for (int channel=MAX_MIDI_CHANNELS; --channel >= 0;)
            {
                midiEvent.data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (channel & MIDI_CHANNEL_BIT));
                NativePluginClass::writeMidiEvent(&midiEvent);
            }

            fNeedsAllNotesOff = false;
        }

        if (fWasPlayingBefore)
            fMidiOut.play(timePos->frame, frames);
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (! show)
            return;

        if (const char* const filename = uiOpenFile(false, "Open Audio File", "MIDI Files *.mid;*.midi;;"))
            uiCustomDataChanged("file", filename);

        uiClosed();
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        return fMidiOut.getState();
    }

    void setState(const char* const data) override
    {
        fMidiOut.setState(data);
    }

    // -------------------------------------------------------------------
    // AbstractMidiPlayer calls

    void writeMidiEvent(const uint8_t port, const long double timePosFrame, const RawMidiEvent* const event) override
    {
        NativeMidiEvent midiEvent;

        midiEvent.port    = port;
        midiEvent.time    = uint32_t(timePosFrame);
        midiEvent.size    = event->size;
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];

        NativePluginClass::writeMidiEvent(&midiEvent);
    }

    // -------------------------------------------------------------------

private:
    MidiPattern fMidiOut;
    bool fNeedsAllNotesOff;
    bool fWasPlayingBefore;

    void _loadMidiFile(const char* const filename)
    {
        fMidiOut.clear();

        using namespace juce;

        const String jfilename = String(CharPointer_UTF8(filename));
        File file(jfilename);

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

        fNeedsAllNotesOff = true;
    }

    PluginClassEND(MidiFilePlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilePlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor midifileDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
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
