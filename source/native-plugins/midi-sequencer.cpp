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

// -----------------------------------------------------------------------

class MidiSequencerPlugin : public NativePluginClass,
                            public AbstractMidiPlayer
{
public:
    MidiSequencerPlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fWantInEvents(false),
          fMidiOut(this)
    {
        // TEST SONG (unsorted to test RtList API)

        uint32_t m = 44;

        fMidiOut.addControl(0*m, 0,  7, 99);
        fMidiOut.addControl(0*m, 0, 10, 63);
        fMidiOut.addProgram(0*m, 0,  0, 0);

        // 6912 On ch=1 n=60 v=90
        // 7237 Off ch=1 n=60 v=90
        // 7296 On ch=1 n=62 v=90
        // 7621 Off ch=1 n=62 v=90
        // 7680 On ch=1 n=64 v=90
        // 8005 Off ch=1 n=64 v=90
        fMidiOut.addNote(6912*m, 0, 60, 90, 325*m);
        fMidiOut.addNote(7680*m, 0, 64, 90, 325*m);
        fMidiOut.addNote(7296*m, 0, 62, 90, 325*m);

        // 1152 On ch=1 n=62 v=90
        // 1477 Off ch=1 n=62 v=90
        // 1536 On ch=1 n=64 v=90
        // 1861 Off ch=1 n=64 v=90
        // 1920 On ch=1 n=64 v=90
        // 2245 Off ch=1 n=64 v=90
        fMidiOut.addNote(1152*m, 0, 62, 90, 325*m);
        fMidiOut.addNote(1920*m, 0, 64, 90, 325*m);
        fMidiOut.addNote(1536*m, 0, 64, 90, 325*m);

        // 3840 On ch=1 n=62 v=90
        // 4491 Off ch=1 n=62 v=90
        // 4608 On ch=1 n=64 v=90
        // 4933 Off ch=1 n=64 v=90
        // 4992 On ch=1 n=67 v=90
        // 5317 Off ch=1 n=67 v=90
        fMidiOut.addNote(3840*m, 0, 62, 90, 650*m);
        fMidiOut.addNote(4992*m, 0, 67, 90, 325*m);
        fMidiOut.addNote(4608*m, 0, 64, 90, 325*m);

        // 0 On ch=1 n=64 v=90
        // 325 Off ch=1 n=64 v=90
        // 384 On ch=1 n=62 v=90
        // 709 Off ch=1 n=62 v=90
        // 768 On ch=1 n=60 v=90
        //1093 Off ch=1 n=60 v=90
        fMidiOut.addNote(  0*m, 0, 64, 90, 325*m);
        fMidiOut.addNote(768*m, 0, 60, 90, 325*m);
        fMidiOut.addNote(384*m, 0, 62, 90, 325*m);

        // 10752 On ch=1 n=60 v=90
        // 12056 Off ch=1 n=60 v=90
        fMidiOut.addNote(10752*m, 0, 60, 90, 650*m);

        // 5376 On ch=1 n=67 v=90
        // 6027 Off ch=1 n=67 v=90
        // 6144 On ch=1 n=64 v=90
        // 6469 Off ch=1 n=64 v=90
        // 6528 On ch=1 n=62 v=90
        // 6853 Off ch=1 n=62 v=90
        fMidiOut.addNote(5376*m, 0, 67, 90, 650*m);
        fMidiOut.addNote(6144*m, 0, 64, 90, 325*m);
        fMidiOut.addNote(6528*m, 0, 62, 90, 325*m);

        // 8064 On ch=1 n=64 v=90
        // 8389 Off ch=1 n=64 v=90
        // 8448 On ch=1 n=64 v=90
        // 9099 Off ch=1 n=64 v=90
        // 9216 On ch=1 n=62 v=90
        // 9541 Off ch=1 n=62 v=90
        fMidiOut.addNote(8064*m, 0, 64, 90, 325*m);
        fMidiOut.addNote(8448*m, 0, 64, 90, 650*m);
        fMidiOut.addNote(9216*m, 0, 62, 90, 325*m);

        // 9600 On ch=1 n=62 v=90
        // 9925 Off ch=1 n=62 v=90
        // 9984 On ch=1 n=64 v=90
        // 10309 Off ch=1 n=64 v=90
        // 10368 On ch=1 n=62 v=90
        // 10693 Off ch=1 n=62 v=90
        fMidiOut.addNote(9600*m, 0, 62, 90, 325*m);
        fMidiOut.addNote(9984*m, 0, 64, 90, 325*m);
        fMidiOut.addNote(10368*m, 0, 62, 90, 325*m);

        // 2304 On ch=1 n=64 v=90
        // 2955 Off ch=1 n=64 v=90
        // 3072 On ch=1 n=62 v=90
        // 3397 Off ch=1 n=62 v=90
        // 3456 On ch=1 n=62 v=90
        // 3781 Off ch=1 n=62 v=90
        fMidiOut.addNote(2304*m, 0, 64, 90, 650*m);
        fMidiOut.addNote(3072*m, 0, 62, 90, 325*m);
        fMidiOut.addNote(3456*m, 0, 62, 90, 325*m);
    }

    ~MidiSequencerPlugin() override
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
    }

    void deactivate() override
    {
    }

    void process(float**, float**, const uint32_t frames, const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        const NativeTimeInfo* const timePos = getTimeInfo();

        if (fWantInEvents)
        {
            RawMidiEvent rawMidiEvent;

            for (uint32_t i=0; i < midiEventCount; ++i)
            {
                const NativeMidiEvent* const midiEvent = &midiEvents[i];

                rawMidiEvent.data[0] = midiEvent->data[0];
                rawMidiEvent.data[1] = midiEvent->data[1];
                rawMidiEvent.data[2] = midiEvent->data[2];
                rawMidiEvent.data[3] = midiEvent->data[3];
                rawMidiEvent.size    = midiEvent->size;
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

    // -------------------------------------------------------------------
    // Plugin process calls

    void writeMidiEvent(const uint8_t port, const uint32_t timePosFrame, const RawMidiEvent* const event) override
    {
        NativeMidiEvent midiEvent;

        midiEvent.port    = port;
        midiEvent.time    = uint32_t(event->time-timePosFrame);
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];
        midiEvent.size    = event->size;

        NativePluginClass::writeMidiEvent(&midiEvent);
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
              data(dataPool),
              dataPendingRT(dataPool) {}

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
            dataPendingRT.spliceAppend(data);
        }

    } fInEvents;

    MidiPattern fMidiOut;

    PluginClassEND(MidiSequencerPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSequencerPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor midisequencerDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE/*|NATIVE_PLUGIN_HAS_GUI*/,
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Sequencer",
    /* label     */ "midisequencer",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(MidiSequencerPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_midisequencer();

CARLA_EXPORT
void carla_register_native_plugin_midisequencer()
{
    carla_register_native_plugin(&midisequencerDesc);
}

// -----------------------------------------------------------------------
