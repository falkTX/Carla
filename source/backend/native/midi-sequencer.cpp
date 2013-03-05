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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaNative.hpp"
#include "CarlaMIDI.h"
#include "CarlaMutex.hpp"
#include "RtList.hpp"

#define MAX_EVENT_DATA_SIZE          3
#define MIN_PREALLOCATED_EVENT_COUNT 100
#define MAX_PREALLOCATED_EVENT_COUNT 1000

struct RawMidiEvent {
    uint8_t  data[MAX_EVENT_DATA_SIZE];
    size_t   size;
    uint32_t time;

    RawMidiEvent()
        : data{0},
          size(0),
          time(0) {}
};

class MidiPattern
{
public:
    MidiPattern(const HostDescriptor* const host)
        : kHost(host),
          fStartTime(0),
          fDuration(0)
    {
        CARLA_ASSERT(host != nullptr);

        // TEST SONG (unsorted to test RtList API)

        uint32_t m = 44;

        addControl(0*m, 0,  7, 99);
        addControl(0*m, 0, 10, 63);
        addProgram(0*m, 0,  0, 0);

        // 6912 On ch=1 n=60 v=90
        // 7237 Off ch=1 n=60 v=90
        // 7296 On ch=1 n=62 v=90
        // 7621 Off ch=1 n=62 v=90
        // 7680 On ch=1 n=64 v=90
        // 8005 Off ch=1 n=64 v=90
        addNote(6912*m, 0, 60, 90, 325*m);
        addNote(7680*m, 0, 64, 90, 325*m);
        addNote(7296*m, 0, 62, 90, 325*m);

        // 1152 On ch=1 n=62 v=90
        // 1477 Off ch=1 n=62 v=90
        // 1536 On ch=1 n=64 v=90
        // 1861 Off ch=1 n=64 v=90
        // 1920 On ch=1 n=64 v=90
        // 2245 Off ch=1 n=64 v=90
        addNote(1152*m, 0, 62, 90, 325*m);
        addNote(1920*m, 0, 64, 90, 325*m);
        addNote(1536*m, 0, 64, 90, 325*m);

        // 3840 On ch=1 n=62 v=90
        // 4491 Off ch=1 n=62 v=90
        // 4608 On ch=1 n=64 v=90
        // 4933 Off ch=1 n=64 v=90
        // 4992 On ch=1 n=67 v=90
        // 5317 Off ch=1 n=67 v=90
        addNote(3840*m, 0, 62, 90, 650*m);
        addNote(4992*m, 0, 67, 90, 325*m);
        addNote(4608*m, 0, 64, 90, 325*m);

        // 0 On ch=1 n=64 v=90
        // 325 Off ch=1 n=64 v=90
        // 384 On ch=1 n=62 v=90
        // 709 Off ch=1 n=62 v=90
        // 768 On ch=1 n=60 v=90
        //1093 Off ch=1 n=60 v=90
        addNote(  0*m, 0, 64, 90, 325*m);
        addNote(768*m, 0, 60, 90, 325*m);
        addNote(384*m, 0, 62, 90, 325*m);

        // 10752 On ch=1 n=60 v=90
        // 12056 Off ch=1 n=60 v=90
        addNote(10752*m, 0, 60, 90, 650*m);

        // 5376 On ch=1 n=67 v=90
        // 6027 Off ch=1 n=67 v=90
        // 6144 On ch=1 n=64 v=90
        // 6469 Off ch=1 n=64 v=90
        // 6528 On ch=1 n=62 v=90
        // 6853 Off ch=1 n=62 v=90
        addNote(5376*m, 0, 67, 90, 650*m);
        addNote(6144*m, 0, 64, 90, 325*m);
        addNote(6528*m, 0, 62, 90, 325*m);

        // 8064 On ch=1 n=64 v=90
        // 8389 Off ch=1 n=64 v=90
        // 8448 On ch=1 n=64 v=90
        // 9099 Off ch=1 n=64 v=90
        // 9216 On ch=1 n=62 v=90
        // 9541 Off ch=1 n=62 v=90
        addNote(8064*m, 0, 64, 90, 325*m);
        addNote(8448*m, 0, 64, 90, 650*m);
        addNote(9216*m, 0, 62, 90, 325*m);

        // 9600 On ch=1 n=62 v=90
        // 9925 Off ch=1 n=62 v=90
        // 9984 On ch=1 n=64 v=90
        // 10309 Off ch=1 n=64 v=90
        // 10368 On ch=1 n=62 v=90
        // 10693 Off ch=1 n=62 v=90
        addNote(9600*m, 0, 62, 90, 325*m);
        addNote(9984*m, 0, 64, 90, 325*m);
        addNote(10368*m, 0, 62, 90, 325*m);

        // 2304 On ch=1 n=64 v=90
        // 2955 Off ch=1 n=64 v=90
        // 3072 On ch=1 n=62 v=90
        // 3397 Off ch=1 n=62 v=90
        // 3456 On ch=1 n=62 v=90
        // 3781 Off ch=1 n=62 v=90
        addNote(2304*m, 0, 64, 90, 650*m);
        addNote(3072*m, 0, 62, 90, 325*m);
        addNote(3456*m, 0, 62, 90, 325*m);
    }

    ~MidiPattern()
    {
        fData.clear();
    }

    void addControl(const uint32_t time, const uint8_t channel, const uint8_t control, const uint8_t value)
    {
        RawMidiEvent* ctrlEvent(new RawMidiEvent());
        ctrlEvent->data[0] = MIDI_STATUS_CONTROL_CHANGE | (channel & 0x0F);
        ctrlEvent->data[1] = control;
        ctrlEvent->data[2] = value;
        ctrlEvent->size    = 3;
        ctrlEvent->time    = time;

        append(ctrlEvent);
    }

    void addProgram(const uint32_t time, const uint8_t channel, const uint8_t bank, const uint8_t program)
    {
        RawMidiEvent* bankEvent(new RawMidiEvent());
        bankEvent->data[0] = MIDI_STATUS_CONTROL_CHANGE | (channel & 0x0F);
        bankEvent->data[1] = MIDI_CONTROL_BANK_SELECT;
        bankEvent->data[2] = bank;
        bankEvent->size    = 3;
        bankEvent->time    = time;

        RawMidiEvent* programEvent(new RawMidiEvent());
        programEvent->data[0] = MIDI_STATUS_PROGRAM_CHANGE | (channel & 0x0F);
        programEvent->data[1] = program;
        programEvent->size    = 2;
        programEvent->time    = time;

        append(bankEvent);
        append(programEvent);
    }

    void addNote(const uint32_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity, const uint32_t duration)
    {
        RawMidiEvent* noteOnEvent(new RawMidiEvent());
        noteOnEvent->data[0] = MIDI_STATUS_NOTE_ON | (channel & 0x0F);
        noteOnEvent->data[1] = pitch;
        noteOnEvent->data[2] = velocity;
        noteOnEvent->size    = 3;
        noteOnEvent->time    = time;

        RawMidiEvent* noteOffEvent(new RawMidiEvent());
        noteOffEvent->data[0] = MIDI_STATUS_NOTE_OFF | (channel & 0x0F);
        noteOffEvent->data[1] = pitch;
        noteOffEvent->data[2] = velocity;
        noteOffEvent->size    = 3;
        noteOffEvent->time    = time+duration;

        append(noteOnEvent);
        append(noteOffEvent);
    }

    void play(uint32_t timePosFrame, uint32_t frames)
    {
        if (! fMutex.tryLock())
            return;

        MidiEvent midiEvent;

        for (auto it = fData.begin(); it.valid(); it.next())
        {
            const RawMidiEvent* const rawMidiEvent(*it);

            if (timePosFrame > rawMidiEvent->time)
                continue;
            if (timePosFrame + frames <= rawMidiEvent->time)
                continue;

            midiEvent.port    = 0;
            midiEvent.time    = rawMidiEvent->time-timePosFrame;
            midiEvent.data[0] = rawMidiEvent->data[0];
            midiEvent.data[1] = rawMidiEvent->data[1];
            midiEvent.data[2] = rawMidiEvent->data[2];

            writeMidiEvent(&midiEvent);
        }

        fMutex.unlock();
    }

private:
    const HostDescriptor* const kHost;

    uint32_t fStartTime;
    uint32_t fDuration;

    CarlaMutex fMutex;
    NonRtList<const RawMidiEvent*> fData;

    void append(const RawMidiEvent* const event)
    {
        if (fData.isEmpty())
        {
            const CarlaMutex::ScopedLocker sl(&fMutex);
            fData.append(event);
            return;
        }

        for (auto it = fData.begin(); it.valid(); it.next())
        {
            const RawMidiEvent* const oldEvent(*it);

            if (event->time >= oldEvent->time)
                continue;

            const CarlaMutex::ScopedLocker sl(&fMutex);
            fData.insertAt(event, it);
            return;
        }

        const CarlaMutex::ScopedLocker sl(&fMutex);
        fData.append(event);
    }

    void writeMidiEvent(const MidiEvent* const event)
    {
        kHost->write_midi_event(kHost->handle, event);
    }
};

class MidiSequencerPlugin : public PluginDescriptorClass
{
public:
    MidiSequencerPlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host),
          fWantInEvents(false),
          fMidiOut(host)
    {
    }

    ~MidiSequencerPlugin()
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
    }

    void deactivate()
    {
    }

    void process(float**, float**, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents)
    {
        const TimeInfo* const timePos = getTimeInfo();

        if (fWantInEvents)
        {
            RawMidiEvent rawMidiEvent;

            for (uint32_t i=0; i < midiEventCount; i++)
            {
                const MidiEvent* const midiEvent = &midiEvents[i];

                rawMidiEvent.data[0] = midiEvent->data[0];
                rawMidiEvent.data[1] = midiEvent->data[1];
                rawMidiEvent.data[2] = midiEvent->data[2];
                rawMidiEvent.time    = timePos->frame + midiEvent->time;

                fInEvents.appendRT(rawMidiEvent);
            }

            if (fInEvents.mutex.tryLock())
            {
                fInEvents.splice();
                fInEvents.mutex.unlock();
            }
        }

        if (timePos->playing)
            fMidiOut.play(timePos->frame, frames);
    }

private:
    bool fWantInEvents;

    struct InRtEvents {
        CarlaMutex mutex;
        RtList<RawMidiEvent>::Pool dataPool;
        RtList<RawMidiEvent> data;
        RtList<RawMidiEvent> dataPendingRT;

        InRtEvents()
            : dataPool(MIN_PREALLOCATED_EVENT_COUNT, MAX_PREALLOCATED_EVENT_COUNT),
              data(&dataPool),
              dataPendingRT(&dataPool) {}

        ~InRtEvents()
        {
            clear();
        }

        void appendRT(const RawMidiEvent& event)
        {
            dataPendingRT.append(event);
        }

        void clear()
        {
            data.clear();
            dataPendingRT.clear();
        }

        void splice()
        {
            dataPendingRT.spliceAppend(data, true);
        }

    } fInEvents;

    MidiPattern fMidiOut;

    PluginDescriptorClassEND(MidiSequencerPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSequencerPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor midiSequencerDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ PLUGIN_IS_RTSAFE/*|PLUGIN_HAS_GUI*/,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Sequencer",
    /* label     */ "midiSequencer",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(MidiSequencerPlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiSequencer()
{
    carla_register_native_plugin(&midiSequencerDesc);
}

// -----------------------------------------------------------------------
