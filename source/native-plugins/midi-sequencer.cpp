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

#include "CarlaNativeExtUI.hpp"
#include "RtLinkedList.hpp"

#include "midi-base.hpp"

// -----------------------------------------------------------------------

class MidiSequencerPlugin : public NativePluginAndUiClass,
                            public AbstractMidiPlayer
{
public:
    MidiSequencerPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, CARLA_OS_SEP_STR "midiseq-ui"),
          fWantInEvents(false),
          fWasPlayingBefore(false),
          fInEvents(),
          fMidiOut(this),
          leakDetector_MidiSequencerPlugin()
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

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

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
                rawMidiEvent.time    = timePos->playing ? timePos->frame + midiEvent->time : 0;

                fInEvents.appendRT(rawMidiEvent);
            }

            fInEvents.trySplice();
        }

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
                NativePluginAndUiClass::writeMidiEvent(&midiEvent);
            }

            carla_stdout("WAS PLAYING BEFORE, NOW STOPPED");
        }

        fWasPlayingBefore = timePos->playing;
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        NativePluginAndUiClass::uiShow(show);

        if (show)
            _sendEventsToUI();
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        // TODO: malloc list of events

        return nullptr;
    }

    void setState(const char* const data) override
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        return;

        fMidiOut.clear();

        // TODO: set events according to data

        _sendEventsToUI();
    }

    // -------------------------------------------------------------------
    // AbstractMidiPlayer calls

    void writeMidiEvent(const uint8_t port, const uint64_t timePosFrame, const RawMidiEvent* const event) override
    {
        NativeMidiEvent midiEvent;

        midiEvent.port    = port;
        midiEvent.time    = uint32_t(event->time-timePosFrame);
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];
        midiEvent.size    = event->size;

        NativePluginAndUiClass::writeMidiEvent(&midiEvent);
    }

    // -------------------------------------------------------------------
    // Pipe Server calls

    bool msgReceived(const char* const msg) noexcept override
    {
        if (NativePluginAndUiClass::msgReceived(msg))
            return true;

        if (std::strcmp(msg, "midi-clear-all") == 0)
        {
            fMidiOut.clear();
            return true;
        }

        if (std::strcmp(msg, "midievent-add") == 0)
        {
            uint64_t time;
            uint8_t size;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsULong(time), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(size), true);

            uint8_t data[size], dvalue;

            for (uint8_t i=0; i<size; ++i)
            {
                CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(dvalue), true);
                data[i] = dvalue;
            }

            fMidiOut.addRaw(time, data, size);

            return true;
        }

        if (std::strcmp(msg, "midievent-remove") == 0)
        {
            uint64_t time;
            uint8_t size;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsULong(time), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(size), true);

            uint8_t data[size], dvalue;

            for (uint8_t i=0; i<size; ++i)
            {
                CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(dvalue), true);
                data[i] = dvalue;
            }

            fMidiOut.removeRaw(time, data, size);

            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

private:
    bool fWantInEvents;
    bool fWasPlayingBefore;

    struct InRtEvents {
        CarlaMutex mutex;
        RtLinkedList<RawMidiEvent>::Pool dataPool;
        RtLinkedList<RawMidiEvent> data;
        RtLinkedList<RawMidiEvent> dataPendingRT;

        InRtEvents() noexcept
            : mutex(),
              dataPool(MIN_PREALLOCATED_EVENT_COUNT, MAX_PREALLOCATED_EVENT_COUNT),
              data(dataPool),
              dataPendingRT(dataPool) {}

        ~InRtEvents() noexcept
        {
            clear();
        }

        void appendRT(const RawMidiEvent& event) noexcept
        {
            dataPendingRT.append(event);
        }

        void clear() noexcept
        {
            mutex.lock();
            data.clear();
            dataPendingRT.clear();
            mutex.unlock();
        }

        void trySplice() noexcept
        {
            if (mutex.tryLock())
            {
                if (dataPendingRT.count() > 0)
                    dataPendingRT.moveTo(data, true);
                mutex.unlock();
            }
        }

        CARLA_DECLARE_NON_COPY_STRUCT(InRtEvents);

    } fInEvents;

    MidiPattern fMidiOut;

    void _sendEventsToUI() const noexcept
    {
        char strBuf[0xff+1];
        strBuf[0xff] = '\0';

        const CarlaMutexLocker cml1(getPipeLock());
        const CarlaMutexLocker cml2(fMidiOut.getLock());

        writeMessage("midi-clear-all\n", 15);

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fMidiOut.iteneratorBegin(); it.valid(); it.next())
        {
            const RawMidiEvent* const rawMidiEvent(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(rawMidiEvent != nullptr);

            writeMessage("midievent-add\n", 14);

            std::snprintf(strBuf, 0xff, P_INT64 "\n", rawMidiEvent->time);
            writeMessage(strBuf);

            std::snprintf(strBuf, 0xff, "%i\n", rawMidiEvent->size);
            writeMessage(strBuf);

            for (uint8_t i=0, size=rawMidiEvent->size; i<size; ++i)
            {
                std::snprintf(strBuf, 0xff, "%i\n", rawMidiEvent->data[i]);
                writeMessage(strBuf);
            }
        }
    }

    PluginClassEND(MidiSequencerPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSequencerPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor midisequencerDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Sequencer",
    /* label     */ "midisequencer",
    /* maker     */ "falkTX, tatch",
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
