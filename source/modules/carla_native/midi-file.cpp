/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include <smf.h>

class MidiFilePlugin : public PluginClass,
                       public AbstractMidiPlayer
{
public:
    MidiFilePlugin(const HostDescriptor* const host)
        : PluginClass(host),
          fMidiOut(this),
          fWasPlayingBefore(false)
    {
    }

    ~MidiFilePlugin() override
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin state calls

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        if (std::strcmp(key, "file") != 0)
            return;

        _loadMidiFile(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float**, const uint32_t frames, const MidiEvent* const, const uint32_t) override
    {
        const TimeInfo* const timePos(getTimeInfo());

        if (timePos->playing)
        {
            fMidiOut.play(timePos->frame, frames);
        }
        else if (fWasPlayingBefore)
        {
            MidiEvent midiEvent;

            midiEvent.port    = 0;
            midiEvent.time    = 0;
            midiEvent.data[0] = MIDI_STATUS_CONTROL_CHANGE;
            midiEvent.data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            midiEvent.data[2] = 0;
            midiEvent.data[3] = 0;
            midiEvent.size    = 3;

            for (int i=0; i < MAX_MIDI_CHANNELS; ++i)
            {
                midiEvent.data[0] = MIDI_STATUS_CONTROL_CHANGE+i;
                PluginClass::writeMidiEvent(&midiEvent);
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

    void writeMidiEvent(const uint32_t timePosFrame, const RawMidiEvent* const event) override
    {
        MidiEvent midiEvent;

        midiEvent.port    = 0;
        midiEvent.time    = event->time-timePosFrame;
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];
        midiEvent.size    = event->size;

        PluginClass::writeMidiEvent(&midiEvent);
    }

private:
    MidiPattern fMidiOut;
    bool fWasPlayingBefore;

    void _loadMidiFile(const char* const filename)
    {
        fMidiOut.clear();

        if (smf_t* const smf = smf_load(filename))
        {
            smf_event_t* event;
            const double sampleRate(getSampleRate());

            while ((event = smf_get_next_event(smf)) != nullptr)
            {
                if (smf_event_is_valid(event) == 0)
                    continue;
                if (smf_event_is_metadata(event))
                    continue;
                if (smf_event_is_system_realtime(event))
                    continue;
                if (smf_event_is_system_common(event))
                    continue;
                if (event->midi_buffer_length <= 0 || event->midi_buffer_length > MAX_EVENT_DATA_SIZE)
                    continue;

                const uint32_t time(event->time_seconds*sampleRate);

#if 1
                fMidiOut.addRaw(time, event->midi_buffer, event->midi_buffer_length);
#else
                const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(event->midi_buffer);
                const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(event->midi_buffer);

                uint8_t nextBank[MAX_MIDI_CHANNELS] = { 0 };

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    const uint8_t note = event->midi_buffer[1];
                    const uint8_t velo = event->midi_buffer[2];

                    fMidiOut.addNoteOff(time, channel, note, velo);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    const uint8_t note = event->midi_buffer[1];
                    const uint8_t velo = event->midi_buffer[2];

                    fMidiOut.addNoteOn(time, channel, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    const uint8_t note     = event->midi_buffer[1];
                    const uint8_t pressure = event->midi_buffer[2];

                    fMidiOut.addNoteAftertouch(time, channel, note, pressure);
                }
                else if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
                {
                    const uint8_t control = event->midi_buffer[1];
                    const uint8_t value   = (event->midi_buffer_length > 2) ? event->midi_buffer[2] : 0;

                    if (MIDI_IS_CONTROL_BANK_SELECT(control))
                        nextBank[channel] = value;
                    else
                        fMidiOut.addControl(time, channel, control, value);
                }
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(status))
                {
                    const uint8_t program = event->midi_buffer[1];

                    fMidiOut.addProgram(time, channel, nextBank[channel], program);
                }
                else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                {
                    const uint8_t pressure = event->midi_buffer[1];

                    fMidiOut.addChannelPressure(time, channel, pressure);
                }
                else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    const uint8_t lsb = event->midi_buffer[1];
                    const uint8_t msb = event->midi_buffer[2];

                    fMidiOut.addPitchbend(time, channel, lsb, msb);
                }
#endif
            }

            smf_delete(smf);
        }
    }

    PluginClassEND(MidiFilePlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilePlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor midifileDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_GUI),
    /* supports  */ static_cast<PluginSupports>(0x0),
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
void carla_register_native_plugin_midifile()
{
    carla_register_native_plugin(&midifileDesc);
}

// -----------------------------------------------------------------------
