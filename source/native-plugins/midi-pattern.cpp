// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaNativeExtUI.hpp"
#include "RtLinkedList.hpp"

#include "midi-base.hpp"
#include "midi-queue.hpp"

// matches UI side
#define TICKS_PER_BEAT 48

// -----------------------------------------------------------------------

class MidiPatternPlugin : public NativePluginAndUiClass,
                          public AbstractMidiPlayer
{
public:
    enum Parameters {
        kParameterTimeSig = 0,
        kParameterMeasures,
        kParameterDefLength,
        kParameterQuantize,
        kParameterCount
    };

    MidiPatternPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "midipattern-ui"),
          fNeedsAllNotesOff(false),
          fWasPlayingBefore(false),
          fTimeSigNum(4),
          fLastPosition(0.0),
          fLastFrame(0),
          fTicksPerFrame(0.0),
          fMaxTicksPerSigNum(0.0),
          fMidiOut(this),
          fTimeInfo(),
          fMidiQueue(),
          fMidiQueueRT()
    {
        carla_zeroStruct(fTimeInfo);

        // set default param values
        fParameters[kParameterTimeSig]   = 3.0f;
        fParameters[kParameterMeasures]  = 4.0f;
        fParameters[kParameterDefLength] = 4.0f;
        fParameters[kParameterQuantize]  = 4.0f;

        fMaxTicksPerSigNum = TICKS_PER_BEAT * fTimeSigNum * 4 /* kParameterMeasures */;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParameterCount;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterCount, nullptr);

        static NativeParameter param;
        static NativeParameterScalePoint scalePoints[10];

        int hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMATABLE|NATIVE_PARAMETER_IS_INTEGER;

        switch (index)
        {
        case 0:
            hints |= NATIVE_PARAMETER_USES_SCALEPOINTS;
            param.name = "Time Signature";
            param.ranges.def = 3.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            scalePoints[0].value = 0.0f;
            scalePoints[0].label = "1/4";
            scalePoints[1].value = 1.0f;
            scalePoints[1].label = "2/4";
            scalePoints[2].value = 2.0f;
            scalePoints[2].label = "3/4";
            scalePoints[3].value = 3.0f;
            scalePoints[3].label = "4/4";
            scalePoints[4].value = 4.0f;
            scalePoints[4].label = "5/4";
            scalePoints[5].value = 5.0f;
            scalePoints[5].label = "6/4";
            param.scalePointCount = 6;
            param.scalePoints     = scalePoints;
            break;
        case 1:
            param.name = "Measures";
            param.ranges.def = 4.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 16.0f;
            break;
        case 2:
            hints |= NATIVE_PARAMETER_USES_SCALEPOINTS;
            param.name = "Default Length";
            param.ranges.def = 4.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 9.0f;
            scalePoints[0].value = 0.0f;
            scalePoints[0].label = "1/16";
            scalePoints[1].value = 1.0f;
            scalePoints[1].label = "1/15";
            scalePoints[2].value = 2.0f;
            scalePoints[2].label = "1/12";
            scalePoints[3].value = 3.0f;
            scalePoints[3].label = "1/9";
            scalePoints[4].value = 4.0f;
            scalePoints[4].label = "1/8";
            scalePoints[5].value = 5.0f;
            scalePoints[5].label = "1/6";
            scalePoints[6].value = 6.0f;
            scalePoints[6].label = "1/4";
            scalePoints[7].value = 7.0f;
            scalePoints[7].label = "1/3";
            scalePoints[8].value = 8.0f;
            scalePoints[8].label = "1/2";
            scalePoints[9].value = 9.0f;
            scalePoints[9].label = "1";
            param.scalePointCount = 10;
            param.scalePoints     = scalePoints;
            break;
        case 3:
            hints |= NATIVE_PARAMETER_USES_SCALEPOINTS;
            param.name = "Quantize";
            param.ranges.def = 4.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 9.0f;
            scalePoints[0].value = 0.0f;
            scalePoints[0].label = "1/16";
            scalePoints[1].value = 1.0f;
            scalePoints[1].label = "1/15";
            scalePoints[2].value = 2.0f;
            scalePoints[2].label = "1/12";
            scalePoints[3].value = 3.0f;
            scalePoints[3].label = "1/9";
            scalePoints[4].value = 4.0f;
            scalePoints[4].label = "1/8";
            scalePoints[5].value = 5.0f;
            scalePoints[5].label = "1/6";
            scalePoints[6].value = 6.0f;
            scalePoints[6].label = "1/4";
            scalePoints[7].value = 7.0f;
            scalePoints[7].label = "1/3";
            scalePoints[8].value = 8.0f;
            scalePoints[8].label = "1/2";
            scalePoints[9].value = 9.0f;
            scalePoints[9].label = "1";
            param.scalePointCount = 10;
            param.scalePoints     = scalePoints;
            break;
        }

        param.hints = static_cast<NativeParameterHints>(hints);

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterCount, 0.0f);

        return fParameters[index];
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParameterCount,);

        fParameters[index] = value;

        switch (index)
        {
        case kParameterTimeSig:
            fTimeSigNum = static_cast<int>(value + 1.5f);
            // fall through
        case kParameterMeasures:
            fMaxTicksPerSigNum = TICKS_PER_BEAT * fTimeSigNum * static_cast<double>(fParameters[kParameterMeasures]);
            fNeedsAllNotesOff = true;
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(const float* const*, float**, const uint32_t frames,
                 const NativeMidiEvent* /*midiEvents*/, uint32_t /*midiEventCount*/) override
    {
        if (const NativeTimeInfo* const timeInfo = getTimeInfo())
            fTimeInfo = *timeInfo;

        if (fWasPlayingBefore != fTimeInfo.playing)
        {
            fLastFrame = 0;
            fLastPosition = 0.0;
            fNeedsAllNotesOff = true;
            fWasPlayingBefore = fTimeInfo.playing;
        }
        else if (fTimeInfo.playing && fLastFrame + frames != fTimeInfo.frame)
        {
            fNeedsAllNotesOff = true;
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

        if (fMidiQueue.isNotEmpty() && fMidiQueueRT.tryToCopyDataFrom(fMidiQueue))
        {
            uint8_t d1, d2, d3;
            NativeMidiEvent ev = { 0, 0, 3, { 0, 0, 0, 0 } };

            while (fMidiQueueRT.get(d1, d2, d3))
            {
                ev.data[0] = d1;
                ev.data[1] = d2;
                ev.data[2] = d3;
                NativePluginAndUiClass::writeMidiEvent(&ev);
            }
        }

        if (fTimeInfo.playing)
        {
            if (! fTimeInfo.bbt.valid)
                fTimeInfo.bbt.beatsPerMinute = 120.0;

            fTicksPerFrame = TICKS_PER_BEAT / (60.0 / fTimeInfo.bbt.beatsPerMinute * getSampleRate());

            double playPos;

            if (fLastFrame + frames == fTimeInfo.frame)
            {
                // continuous playback
                playPos = fLastPosition + fTicksPerFrame * static_cast<double>(frames);
            }
            else
            {
                // non-continuous, reset playPos
                playPos = fTicksPerFrame * static_cast<double>(fTimeInfo.frame);
            }

            const double endPos       = playPos + fTicksPerFrame * static_cast<double>(frames);
            const double loopedEndPos = std::fmod(endPos, fMaxTicksPerSigNum);

            for (; playPos < endPos; playPos += fMaxTicksPerSigNum)
            {
                const double loopedPlayPos = std::fmod(playPos, fMaxTicksPerSigNum);

                if (loopedEndPos >= loopedPlayPos)
                {
                    if (! fMidiOut.play(loopedPlayPos, loopedEndPos-loopedPlayPos))
                        fNeedsAllNotesOff = true;
                }
                else
                {
                    const double diff = fMaxTicksPerSigNum - loopedPlayPos;

                    if (! (fMidiOut.play(loopedPlayPos, diff) && fMidiOut.play(0.0, loopedEndPos, diff)))
                        fNeedsAllNotesOff = true;
                }
            }

            fLastPosition = playPos;
        }

        fLastFrame = fTimeInfo.frame;
    }

#ifndef CARLA_OS_WASM
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
            carla_zeroChars(strBuf, 0xff+1);

            const double beatsPerBar    = fTimeSigNum;
            const double beatsPerMinute = fTimeInfo.bbt.valid ? fTimeInfo.bbt.beatsPerMinute : 120.0;

            const double ticksPerBeat  = TICKS_PER_BEAT;
            const double fullTicks     = fLastPosition;
            const double fullBeats     = fullTicks / ticksPerBeat;

            const uint32_t tick = static_cast<uint32_t>(std::floor(std::fmod(fullTicks, ticksPerBeat)));
            const uint32_t beat = static_cast<uint32_t>(std::floor(std::fmod(fullBeats, beatsPerBar)));
            const uint32_t bar  = static_cast<uint32_t>(std::floor(fullBeats/beatsPerBar));

            const CarlaMutexLocker cml(getPipeLock());

            CARLA_SAFE_ASSERT_RETURN(writeMessage("transport\n"),);

            std::snprintf(strBuf, 0xff, "%i:" P_UINT64 ":%i:%i:%i\n", int(fTimeInfo.playing), fTimeInfo.frame, bar, beat, tick);
            CARLA_SAFE_ASSERT_RETURN(writeMessage(strBuf),);

            {
                const ScopedSafeLocale ssl;
                std::snprintf(strBuf, 0xff, "%.12g\n", beatsPerMinute);
            }

            CARLA_SAFE_ASSERT_RETURN(writeMessage(strBuf),);

            syncMessages();
        }
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        return fMidiOut.getState();
    }

    void setState(const char* const data) override
    {
        fMidiOut.setState(data);

#ifndef CARLA_OS_WASM
        if (isPipeRunning())
            _sendEventsToUI();
#endif
    }

    // -------------------------------------------------------------------
    // AbstractMidiPlayer calls

    void writeMidiEvent(const uint8_t port, const double timePosFrame, const RawMidiEvent* const event) override
    {
        NativeMidiEvent midiEvent;

        midiEvent.port    = port;
        midiEvent.time    = uint32_t(timePosFrame/fTicksPerFrame);
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];
        midiEvent.size    = event->size;

#ifdef DEBUG
        carla_stdout("Playing at %f|%u :: %03X:%03i:%03i",
                     midiEvent.time,
                     static_cast<double>(midiEvent.time)*fTicksPerFrame,
                     midiEvent.data[0], midiEvent.data[1], midiEvent.data[2]);
#endif

        NativePluginAndUiClass::writeMidiEvent(&midiEvent);
    }

#ifndef CARLA_OS_WASM
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

        if (std::strcmp(msg, "midi-note") == 0)
        {
            uint8_t note;
            bool on;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(note), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(on), true);

            const uint8_t status   = on ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
            const uint8_t velocity = on ? 100 : 0;

            const CarlaMutexLocker cml(fMidiQueue.getMutex());

            fMidiQueue.put(status, note, velocity);
            return true;
        }

        if (std::strcmp(msg, "midievent-add") == 0)
        {
            uint32_t time;
            uint8_t size;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(time), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(size), true);
            CARLA_SAFE_ASSERT_RETURN(size > 0, true);

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
            uint32_t time;
            uint8_t size;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(time), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(size), true);
            CARLA_SAFE_ASSERT_RETURN(size > 0, true);

            uint8_t data[size], dvalue;

            for (uint8_t i=0; i<size; ++i)
            {
                CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(dvalue), true);
                data[i] = dvalue;
            }

            fMidiOut.removeRaw(time, data, size);

            if (MIDI_IS_STATUS_NOTE_ON(data[0]))
            {
                const uint8_t status = MIDI_STATUS_NOTE_OFF | (data[0] & MIDI_CHANNEL_BIT);

                const CarlaMutexLocker cml(fMidiQueue.getMutex());
                fMidiQueue.put(status, data[1], 0);
            }

            return true;
        }

        return false;
    }
#endif

    // -------------------------------------------------------------------

private:
    bool fNeedsAllNotesOff;
    bool fWasPlayingBefore;
    int  fTimeSigNum;

    double   fLastPosition;
    uint64_t fLastFrame;

    double fTicksPerFrame;
    double fMaxTicksPerSigNum;

    MidiPattern    fMidiOut;
    NativeTimeInfo fTimeInfo;

    MIDIEventQueue<32> fMidiQueue, fMidiQueueRT;

    float fParameters[kParameterCount];

#ifndef CARLA_OS_WASM
    void _sendEventsToUI() const noexcept
    {
        char strBuf[0xff+1];
        carla_zeroChars(strBuf, 0xff);

        const CarlaMutexLocker cml1(getPipeLock());
        const CarlaMutexLocker cml2(fMidiOut.getWriteMutex());

        writeMessage("midi-clear-all\n", 15);

        writeMessage("parameters\n", 11);
        std::snprintf(strBuf, 0xff, "%i:%i:%i:%i\n",
                      static_cast<int>(fParameters[kParameterTimeSig]),
                      static_cast<int>(fParameters[kParameterMeasures]),
                      static_cast<int>(fParameters[kParameterDefLength]),
                      static_cast<int>(fParameters[kParameterQuantize]));
        writeMessage(strBuf);

        for (LinkedList<const RawMidiEvent*>::Itenerator it = fMidiOut.iteneratorBegin(); it.valid(); it.next())
        {
            const RawMidiEvent* const rawMidiEvent(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(rawMidiEvent != nullptr);

            writeMessage("midievent-add\n", 14);

            std::snprintf(strBuf, 0xff, "%u\n", rawMidiEvent->time);
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
#endif

    PluginClassEND(MidiPatternPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPatternPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor midipatternDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 1,
    /* paramIns  */ MidiPatternPlugin::kParameterCount,
    /* paramOuts */ 0,
    /* name      */ "MIDI Pattern",
    /* label     */ "midipattern",
    /* maker     */ "falkTX, tatch",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(MidiPatternPlugin)
};

// -----------------------------------------------------------------------

CARLA_API_EXPORT
void carla_register_native_plugin_midipattern();

CARLA_API_EXPORT
void carla_register_native_plugin_midipattern()
{
    carla_register_native_plugin(&midipatternDesc);
}

// -----------------------------------------------------------------------
