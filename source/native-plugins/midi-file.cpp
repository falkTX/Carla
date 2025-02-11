/*
 * Carla Native Plugins
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNativePrograms.hpp"
#include "midi-base.hpp"

#include "water/files/FileInputStream.h"
#include "water/midi/MidiFile.h"

// -----------------------------------------------------------------------

class MidiFilePlugin : public NativePluginWithMidiPrograms<FileMIDI>,
                       public AbstractMidiPlayer
{
public:
    enum Parameters {
        kParameterRepeating,
        kParameterHostSync,
        kParameterEnabled,
        kParameterInfoNumTracks,
        kParameterInfoLength,
        kParameterInfoPosition,
        kParameterCount
    };

    MidiFilePlugin(const NativeHostDescriptor* const host)
        : NativePluginWithMidiPrograms<FileMIDI>(host, fPrograms, 0),
          fRepeatMode(false),
#ifdef __MOD_DEVICES__
          fHostSync(false),
#else
          fHostSync(true),
#endif
          fEnabled(true),
          fNeedsAllNotesOff(false),
          fWasPlayingBefore(false),
          fLastPosition(0.0f),
          fMidiOut(this),
          fFileLength(0.0f),
          fNumTracks(0.0f),
          fInternalTransportFrame(0),
          fMaxFrame(0),
          fLastFrame(0),
          fPrograms(hostGetFilePath("midi"), "*.mid;*.midi")
    {
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
        static NativeParameter param;

        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;
        param.unit             = nullptr;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        param.designation      = NATIVE_PARAMETER_DESIGNATION_NONE;

        switch (index)
        {
        case kParameterRepeating:
            param.name  = "Repeat Mode";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_BOOLEAN);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case kParameterHostSync:
            param.name  = "Host Sync";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_BOOLEAN);
#ifdef __MOD_DEVICES__
            param.ranges.def = 0.0f;
#else
            param.ranges.def = 1.0f;
#endif
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case kParameterEnabled:
            param.name  = "Enabled";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_BOOLEAN|
                                                            NATIVE_PARAMETER_USES_DESIGNATION);
            param.ranges.def = 1.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            param.designation = NATIVE_PARAMETER_DESIGNATION_ENABLED;
            break;
        case kParameterInfoNumTracks:
            param.name  = "Num Tracks";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 256.0f;
            break;
        case kParameterInfoLength:
            param.name  = "Length";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = (float)INT64_MAX;
            param.unit = "s";
            break;
        case kParameterInfoPosition:
            param.name  = "Position";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.unit = "%";
            break;
        default:
            return nullptr;
        }

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        switch (index)
        {
        case kParameterRepeating:
            return fRepeatMode ? 1.0f : 0.0f;
        case kParameterHostSync:
            return fHostSync ? 1.0f : 0.0f;
        case kParameterEnabled:
            return fEnabled ? 1.0f : 0.0f;
        case kParameterInfoNumTracks:
            return fNumTracks;
        case kParameterInfoLength:
            return fFileLength;
        case kParameterInfoPosition:
            return fLastPosition;
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        const bool b = (value > 0.5f);

        switch (index)
        {
        case kParameterRepeating:
            if (fRepeatMode != b)
            {
                fRepeatMode = b;
                fNeedsAllNotesOff = true;
            }
            break;
        case kParameterHostSync:
            if (fHostSync != b)
            {
                fInternalTransportFrame = 0;
                fHostSync = b;
            }
            break;
        case kParameterEnabled:
            if (fEnabled != b)
            {
                fInternalTransportFrame = 0;
                fEnabled = b;
            }
            break;
        default:
            break;
        }
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);

        if (std::strcmp(key, "file") != 0)
            return;

        invalidateNextFilename();
        _loadMidiFile(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process2(const float* const*, float**, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        const uint32_t maxFrame = fMaxFrame;
        bool playing;
        uint64_t frame;

        if (fHostSync)
        {
            const NativeTimeInfo* const timePos = getTimeInfo();
            playing = fEnabled && timePos->playing;
            frame = timePos->frame;
        }
        else
        {
            playing = fEnabled;
            frame = fInternalTransportFrame;

            if (playing)
                fInternalTransportFrame += frames;
        }

        if (fRepeatMode && maxFrame != 0 && frame >= maxFrame)
            frame %= maxFrame;

        if (fWasPlayingBefore != playing || frame < fLastFrame)
        {
            fNeedsAllNotesOff = true;
            fWasPlayingBefore = playing;
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
            if (! fMidiOut.play(static_cast<uint32_t>(frame), frames))
                fNeedsAllNotesOff = true;

        fLastFrame = frame;

        if (frame < maxFrame)
            fLastPosition = static_cast<float>(frame) / static_cast<float>(maxFrame) * 100.0f;
        else
            fLastPosition = 100.0f;
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (! show)
            return;

        if (const char* const filename = uiOpenFile(false, "Open MIDI File", "MIDI Files (*.mid *.midi);;"))
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

    void setStateFromFile(const char* const filename) override
    {
        _loadMidiFile(filename);
    }

    // -------------------------------------------------------------------
    // AbstractMidiPlayer calls

    void writeMidiEvent(const uint8_t port, const double timePosFrame, const RawMidiEvent* const event) override
    {
        NativeMidiEvent midiEvent;

        midiEvent.port    = port;
        midiEvent.time    = static_cast<uint32_t>(timePosFrame);
        midiEvent.size    = event->size;
        midiEvent.data[0] = event->data[0];
        midiEvent.data[1] = event->data[1];
        midiEvent.data[2] = event->data[2];
        midiEvent.data[3] = event->data[3];

        NativePluginClass::writeMidiEvent(&midiEvent);
    }

    // -------------------------------------------------------------------

private:
    bool fRepeatMode;
    bool fHostSync;
    bool fEnabled;
    bool fNeedsAllNotesOff;
    bool fWasPlayingBefore;
    float fLastPosition;
    MidiPattern fMidiOut;
    float fFileLength;
    float fNumTracks;
    uint32_t fInternalTransportFrame;
    uint32_t fMaxFrame;
    uint64_t fLastFrame;
    NativeMidiPrograms fPrograms;

    void _loadMidiFile(const char* const filename)
    {
        fMidiOut.clear();
        fInternalTransportFrame = 0;
        fFileLength = 0.0f;
        fNumTracks = 0.0f;
        fMaxFrame = 0;
        fLastFrame = 0;
        fLastPosition = 0.0f;

        using namespace water;

        File file(filename);

        if (! file.existsAsFile())
           return;

        FileInputStream fileStream(file);
        MidiFile        midiFile;

        if (! midiFile.readFrom(fileStream))
            return;

        midiFile.convertTimestampTicksToSeconds();

        const double sampleRate = getSampleRate();
        const size_t numTracks = midiFile.getNumTracks();

        for (size_t i=0; i<numTracks; ++i)
        {
            const MidiMessageSequence* const track = midiFile.getTrack(i);
            CARLA_SAFE_ASSERT_CONTINUE(track != nullptr);

            for (int j=0, numEvents = track->getNumEvents(); j<numEvents; ++j)
            {
                const MidiMessageSequence::MidiEventHolder* const midiEventHolder = track->getEventPointer(j);
                CARLA_SAFE_ASSERT_CONTINUE(midiEventHolder != nullptr);

                const MidiMessage& midiMessage(midiEventHolder->message);

                const int dataSize = midiMessage.getRawDataSize();
                if (dataSize <= 0 || dataSize > MAX_EVENT_DATA_SIZE)
                    continue;

                const uint8_t* const data = midiMessage.getRawData();
                if (! MIDI_IS_CHANNEL_MESSAGE(data[0]))
                    continue;

                const double time = midiMessage.getTimeStamp() * sampleRate;
                // const double time = track->getEventTime(i) * sampleRate;
                CARLA_SAFE_ASSERT_CONTINUE(time >= 0.0);

                fMidiOut.addRaw(static_cast<uint32_t>(time + 0.5),
                                midiMessage.getRawData(), static_cast<uint8_t>(dataSize));
            }
        }

        const double lastTimeStamp = midiFile.getLastTimestamp();

        fFileLength = static_cast<float>(lastTimeStamp);
        fNumTracks = static_cast<float>(numTracks);
        fNeedsAllNotesOff = true;
        fInternalTransportFrame = 0;
        fLastFrame = 0;
        fMaxFrame = static_cast<uint32_t>(lastTimeStamp * sampleRate + 0.5);
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
                                                  |NATIVE_PLUGIN_REQUESTS_IDLE
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

CARLA_API_EXPORT
void carla_register_native_plugin_midifile();

CARLA_API_EXPORT
void carla_register_native_plugin_midifile()
{
    carla_register_native_plugin(&midifileDesc);
}

// -----------------------------------------------------------------------
