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
    enum Parameters {
        kParameterCount = 0
    };

    MidiSequencerPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, CARLA_OS_SEP_STR "midiseq-ui"),
          fNeedsAllNotesOff(false),
          fWantInEvents(false),
          fWasPlayingBefore(false),
          fTicksPerFrame(0.0),
          fInEvents(),
          fMidiOut(this),
          fTimeInfo(),
          leakDetector_MidiSequencerPlugin()
    {
        carla_zeroStruct(fTimeInfo);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float**, const uint32_t frames, const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        if (const NativeTimeInfo* const timeInfo = getTimeInfo())
            fTimeInfo = *timeInfo;

        if (fWantInEvents)
        {
            if (midiEventCount > 0)
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
                    rawMidiEvent.time    = fTimeInfo.playing ? fTimeInfo.frame + midiEvent->time : 0;

                    fInEvents.appendRT(rawMidiEvent);
                }
            }

            fInEvents.trySplice();
        }

        if (fWasPlayingBefore != fTimeInfo.playing)
        {
            fNeedsAllNotesOff = true;
            fWasPlayingBefore = fTimeInfo.playing;
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
                NativePluginAndUiClass::writeMidiEvent(&midiEvent);
            }

            fNeedsAllNotesOff = false;
        }

        if (fTimeInfo.playing)
        {
            if (! fTimeInfo.bbt.valid)
                fTimeInfo.bbt.beatsPerMinute = 120.0;

            fTicksPerFrame = 48.0 / (60.0 / fTimeInfo.bbt.beatsPerMinute * getSampleRate());

            fMidiOut.play(fTicksPerFrame*static_cast<long double>(fTimeInfo.frame),
                          fTicksPerFrame*static_cast<double>(frames));
        }
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        NativePluginAndUiClass::uiShow(show);

        if (show)
            _sendEventsToUI();
    }

    void uiIdle() override
    {
        NativePluginAndUiClass::uiIdle();

        // send transport
        if (isPipeRunning())
        {
            char strBuf[0xff+1];
            strBuf[0xff] = '\0';

            const float  beatsPerBar    = fTimeInfo.bbt.valid ? fTimeInfo.bbt.beatsPerBar    : 4.0f;
            const double beatsPerMinute = fTimeInfo.bbt.valid ? fTimeInfo.bbt.beatsPerMinute : 120.0;
            const float  beatType       = fTimeInfo.bbt.valid ? fTimeInfo.bbt.beatType       : 4.0f;

            const double ticksPerBeat  = 48.0;
            const double ticksPerFrame = ticksPerBeat / (60.0 / beatsPerMinute * getSampleRate());
            const double fullTicks     = static_cast<double>(ticksPerFrame*static_cast<long double>(fTimeInfo.frame));
            const double fullBeats     = fullTicks/ticksPerBeat;

            const uint32_t tick = static_cast<uint32_t>(std::floor(std::fmod(fullTicks, ticksPerBeat)));
            const uint32_t beat = static_cast<uint32_t>(std::floor(std::fmod(fullBeats, beatsPerBar)));
            const uint32_t bar  = static_cast<uint32_t>(std::floor(fullBeats/beatsPerBar));

            const CarlaMutexLocker cml(getPipeLock());
            const ScopedLocale csl;

            writeAndFixMessage("transport");
            writeMessage(fTimeInfo.playing ? "true\n" : "false\n");

            std::sprintf(strBuf, P_UINT64 ":%i:%i:%i\n", fTimeInfo.frame, bar, beat, tick);
            writeMessage(strBuf);

            std::sprintf(strBuf, "%f:%f:%f\n", beatsPerMinute, beatsPerBar, beatType);
            writeMessage(strBuf);

            flushMessages();
        }
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

        if (isPipeRunning())
            _sendEventsToUI();
    }

    // -------------------------------------------------------------------
    // AbstractMidiPlayer calls

    void writeMidiEvent(const uint8_t port, const long double timePosFrame, const RawMidiEvent* const event) override
    {
        NativeMidiEvent midiEvent;

        midiEvent.port    = port;
        midiEvent.time    = uint32_t(timePosFrame/fTicksPerFrame);
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];
        midiEvent.size    = event->size;

        carla_stdout("Playing at %i :: %03X:%03i:%03i",
                     midiEvent.time, midiEvent.data[0], midiEvent.data[1], midiEvent.data[2]);

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
            fNeedsAllNotesOff = true;
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
    bool fNeedsAllNotesOff;
    bool fWantInEvents;
    bool fWasPlayingBefore;

    double fTicksPerFrame;

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

    MidiPattern    fMidiOut;
    NativeTimeInfo fTimeInfo;

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
    /* paramIns  */ MidiSequencerPlugin::kParameterCount,
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
