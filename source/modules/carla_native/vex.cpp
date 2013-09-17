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

#include "juce_core.h"

using namespace juce;

#include "vex/cArp.h"
#include "vex/cChorus.h"
#include "vex/cDelay.h"
#include "vex/cReverb.h"
#include "vex/cSyntModule.h"

// -----------------------------------------------------------------------

class VexArpPlugin : public PluginClass
{
public:
    enum Params {
        kParamOnOff = 0,
        kParamLength,
        kParamTimeMode,
        kParamSyncMode,
        kParamFailMode,
        kParamVelMode,
        kParamCount
    };

    VexArpPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          fSettings(),
          fArp(&fSettings)
    {
        for (int i=0; i < 8; ++i)
            fSettings.grid[i*10] = true;

        fSettings.grid[1] = true;
        fSettings.grid[2] = true;
        fSettings.grid[3] = true;

        fSettings.grid[41] = true;
        fSettings.grid[42] = true;
        fSettings.grid[43] = true;
        fSettings.grid[44] = true;
        fSettings.grid[45] = true;

        fArp.setSampleRate(getSampleRate());
        fMidiInBuffer.ensureSize(512*4);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParamCount;
    }

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        static Parameter paramInfo;
        static ParameterScalePoint scalePoints[4];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE|PARAMETER_IS_INTEGER;

        paramInfo.name = nullptr;
        paramInfo.unit = nullptr;
        paramInfo.ranges.def       = 0.0f;
        paramInfo.ranges.min       = 0.0f;
        paramInfo.ranges.max       = 1.0f;
        paramInfo.ranges.step      = 1.0f;
        paramInfo.ranges.stepSmall = 1.0f;
        paramInfo.ranges.stepLarge = 1.0f;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        switch (index)
        {
        case kParamOnOff:
            hints |= PARAMETER_IS_BOOLEAN;
            paramInfo.name = "On/Off";
            paramInfo.ranges.def = 0.0f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 1.0f;
            break;
        case kParamLength:
            paramInfo.name = "Length";
            paramInfo.ranges.def = 8.0f;
            paramInfo.ranges.min = 1.0f;
            paramInfo.ranges.max = 16.0f;
            break;
        case kParamTimeMode:
            hints |= PARAMETER_USES_SCALEPOINTS;
            paramInfo.name = "Time Signature";
            paramInfo.ranges.def = 2.0f;
            paramInfo.ranges.min = 1.0f;
            paramInfo.ranges.max = 3.0f;
            paramInfo.scalePointCount = 3;
            paramInfo.scalePoints     = scalePoints;
            scalePoints[0].label = "8";
            scalePoints[1].label = "16";
            scalePoints[2].label = "32";
            scalePoints[0].value = 1.0f;
            scalePoints[1].value = 2.0f;
            scalePoints[2].value = 3.0f;
            break;
        case kParamSyncMode:
            hints |= PARAMETER_USES_SCALEPOINTS;
            paramInfo.name = "Sync Mode";
            paramInfo.ranges.def = 1.0f;
            paramInfo.ranges.min = 1.0f;
            paramInfo.ranges.max = 2.0f;
            paramInfo.scalePointCount = 2;
            paramInfo.scalePoints     = scalePoints;
            scalePoints[0].label = "Key Sync";
            scalePoints[1].label = "Bar Sync";
            scalePoints[0].value = 1.0f;
            scalePoints[1].value = 2.0f;
            break;
        case kParamFailMode:
            hints |= PARAMETER_USES_SCALEPOINTS;
            paramInfo.name = "Fail Mode";
            paramInfo.ranges.def = 1.0f;
            paramInfo.ranges.min = 1.0f;
            paramInfo.ranges.max = 3.0f;
            paramInfo.scalePointCount = 3;
            paramInfo.scalePoints     = scalePoints;
            scalePoints[0].label = "Silent Step";
            scalePoints[1].label = "Skip One";
            scalePoints[2].label = "Skip Two";
            scalePoints[0].value = 1.0f;
            scalePoints[1].value = 2.0f;
            scalePoints[2].value = 3.0f;
            break;
        case kParamVelMode:
            hints |= PARAMETER_USES_SCALEPOINTS;
            paramInfo.name = "Velocity Mode";
            paramInfo.ranges.def = 1.0f;
            paramInfo.ranges.min = 1.0f;
            paramInfo.ranges.max = 3.0f;
            paramInfo.scalePointCount = 3;
            paramInfo.scalePoints     = scalePoints;
            scalePoints[0].label = "Pattern Velocity";
            scalePoints[1].label = "Input Velocity";
            scalePoints[2].label = "Sum Velocities";
            scalePoints[0].value = 1.0f;
            scalePoints[1].value = 2.0f;
            scalePoints[2].value = 3.0f;
            break;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);

        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        switch (index)
        {
        case kParamOnOff:
            return fSettings.on ? 1.0f : 0.0f;
        case kParamLength:
            return fSettings.length;
        case kParamTimeMode:
            return fSettings.timeMode;
        case kParamSyncMode:
            return fSettings.syncMode;
        case kParamFailMode:
            return fSettings.failMode;
        case kParamVelMode:
            return fSettings.velMode;
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        switch (index)
        {
        case kParamOnOff:
            fSettings.on = (value >= 0.5f);
            break;
        case kParamLength:
            fSettings.length = value;
            break;
        case kParamTimeMode:
            fSettings.timeMode = value;
            break;
        case kParamSyncMode:
            fSettings.syncMode = value;
            break;
        case kParamFailMode:
            fSettings.failMode = value;
            break;
        case kParamVelMode:
            fSettings.velMode = value;
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float**, const uint32_t frames, const MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        if (! fSettings.on)
        {
            for (uint32_t i=0; i < midiEventCount; ++i)
                writeMidiEvent(&midiEvents[i]);

            return;
        }

        const TimeInfo* const timeInfo(getTimeInfo());

        int    timeFrame   = 0;
        bool   timePlaying = false;
        double ppqPos      = 0.0;
        double barStartPos = 0.0;
        double bpm         = 120.0;

        if (timeInfo != nullptr)
        {
            timeFrame   = timeInfo->frame;
            timePlaying = timeInfo->playing;

            if (timeInfo->bbt.valid)
            {
                double ppqBar  = double(timeInfo->bbt.bar - 1) * timeInfo->bbt.beatsPerBar;
                double ppqBeat = double(timeInfo->bbt.beat - 1);
                double ppqTick = double(timeInfo->bbt.tick) / timeInfo->bbt.ticksPerBeat;

                ppqPos      = ppqBar + ppqBeat + ppqTick;
                barStartPos = ppqBar;
                bpm         = timeInfo->bbt.beatsPerMinute;
            }
        }

        fMidiInBuffer.clear();

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const MidiEvent* const midiEvent(&midiEvents[i]);
            fMidiInBuffer.addEvent(MidiMessage(midiEvent->data, midiEvent->size, midiEvent->time), timeFrame);
        }

        const MidiBuffer& outMidiBuffer(fArp.processMidi(fMidiInBuffer, timePlaying, ppqPos, barStartPos, bpm, frames));

        MidiBuffer::Iterator outBufferIterator(outMidiBuffer);
        MidiMessage midiMessage(0xf4);
        int sampleNumber;

        MidiEvent tmpEvent;
        tmpEvent.port = 0;

        while (outBufferIterator.getNextEvent(midiMessage, sampleNumber))
        {
            tmpEvent.size = midiMessage.getRawDataSize();
            tmpEvent.time = midiMessage.getTimeStamp();

            if (tmpEvent.size > 4)
                continue;

            std::memcpy(tmpEvent.data, midiMessage.getRawData(), sizeof(uint8_t)*tmpEvent.size);
            writeMidiEvent(&tmpEvent);
        }
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void sampleRateChanged(const double sampleRate) override
    {
        fArp.setSampleRate(sampleRate);
    }

private:
    VexArpSettings fSettings;
    VexArp fArp;
    MidiBuffer fMidiInBuffer;

    PluginClassEND(VexArpPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexArpPlugin)
};

// -----------------------------------------------------------------------

class VexChorusPlugin : public PluginClass
{
public:
    enum Params {
        kParamDepth = 0,
        kParamRate,
        kParamCount
    };

    VexChorusPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          chorus(parameters)
    {
        parameters[0] = 0.6f;
        parameters[1] = 0.3f;

        chorus.setSampleRate(getSampleRate());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParamCount;
    }

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        static Parameter paramInfo;

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;

        paramInfo.name = nullptr;
        paramInfo.unit = nullptr;
        paramInfo.ranges.def       = 0.0f;
        paramInfo.ranges.min       = 0.0f;
        paramInfo.ranges.max       = 1.0f;
        paramInfo.ranges.step      = 1.0f;
        paramInfo.ranges.stepSmall = 1.0f;
        paramInfo.ranges.stepLarge = 1.0f;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        switch (index)
        {
        case kParamDepth:
            paramInfo.name = "Depth";
            paramInfo.ranges.def = 0.6f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 1.0f;
            break;
        case kParamRate:
            paramInfo.name = "Rate";
            paramInfo.ranges.def = 0.3f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 1.0f;
            break;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);

        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (index < kParamCount)
            return parameters[index];
        return 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
            parameters[index] = value;
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const MidiEvent* const, const uint32_t) override
    {
        if (inBuffer[0] != outBuffer[0])
            carla_copyFloat(outBuffer[0], inBuffer[0], frames);
        if (inBuffer[1] != outBuffer[1])
            carla_copyFloat(outBuffer[1], inBuffer[1], frames);

        chorus.processBlock(outBuffer[0], outBuffer[1], frames);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void sampleRateChanged(const double sampleRate) override
    {
        chorus.setSampleRate(sampleRate);
    }

private:
    VexChorus chorus;
    float parameters[2];

    PluginClassEND(VexChorusPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexChorusPlugin)
};

// -----------------------------------------------------------------------

class VexDelayPlugin : public PluginClass
{
public:
    enum Params {
        kParamTime = 0,
        kParamFeedback,
        kParamCount
    };

    VexDelayPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          delay(parameters)
    {
        parameters[0] = 4.0f;
        parameters[1] = 40.0f;

        delay.setSampleRate(getSampleRate());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParamCount;
    }

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        static Parameter paramInfo;

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;

        paramInfo.name = nullptr;
        paramInfo.unit = nullptr;
        paramInfo.ranges.def       = 0.0f;
        paramInfo.ranges.min       = 0.0f;
        paramInfo.ranges.max       = 1.0f;
        paramInfo.ranges.step      = 1.0f;
        paramInfo.ranges.stepSmall = 1.0f;
        paramInfo.ranges.stepLarge = 1.0f;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        switch (index)
        {
        case kParamTime:
            hints |= PARAMETER_IS_INTEGER;
            paramInfo.name = "Time";
            paramInfo.ranges.def = 4.0f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 8.0f;
            break;
        case kParamFeedback:
            paramInfo.name = "Feedback";
            paramInfo.unit = "%";
            paramInfo.ranges.def = 40.0f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 100.0f;
            break;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);

        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (index < kParamCount)
            return parameters[index];
        return 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
            parameters[index] = value;
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const MidiEvent* const, const uint32_t) override
    {
        if (inBuffer[0] != outBuffer[0])
            carla_copyFloat(outBuffer[0], inBuffer[0], frames);
        if (inBuffer[1] != outBuffer[1])
            carla_copyFloat(outBuffer[1], inBuffer[1], frames);

        const TimeInfo* const timeInfo(getTimeInfo());
        const double bpm((timeInfo != nullptr && timeInfo->bbt.valid) ? timeInfo->bbt.beatsPerMinute : 120.0);

        delay.processBlock(outBuffer[0], outBuffer[1], frames, bpm);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void sampleRateChanged(const double sampleRate) override
    {
        delay.setSampleRate(sampleRate);
    }

private:
    VexDelay delay;
    float parameters[2];

    PluginClassEND(VexDelayPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexDelayPlugin)
};

// -----------------------------------------------------------------------

class VexReverbPlugin : public PluginClass
{
public:
    enum Params {
        kParamSize = 0,
        kParamWidth,
        kParamDamp,
        kParamCount
    };

    VexReverbPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          reverb(parameters)
    {
        parameters[0] = 0.6f;
        parameters[1] = 0.7f;
        parameters[2] = 0.6f;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParamCount;
    }

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        static Parameter paramInfo;

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;

        paramInfo.name = nullptr;
        paramInfo.unit = nullptr;
        paramInfo.ranges.def       = 0.0f;
        paramInfo.ranges.min       = 0.0f;
        paramInfo.ranges.max       = 1.0f;
        paramInfo.ranges.step      = 1.0f;
        paramInfo.ranges.stepSmall = 1.0f;
        paramInfo.ranges.stepLarge = 1.0f;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        switch (index)
        {
        case kParamSize:
            paramInfo.name = "Size";
            paramInfo.ranges.def = 0.6f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 1.0f;
            break;
        case kParamWidth:
            paramInfo.name = "Width";
            paramInfo.ranges.def = 0.7f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 1.0f;
            break;
        case kParamDamp:
            paramInfo.name = "Damp";
            paramInfo.ranges.def = 0.6f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 1.0f;
            break;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);

        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (index < kParamCount)
            return parameters[index];
        return 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
            parameters[index] = value;
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const MidiEvent* const, const uint32_t) override
    {
        for (uint32_t i=0; i< frames; ++i)
            outBuffer[0][i] = inBuffer[0][i]/2.0f;
        for (uint32_t i=0; i< frames; ++i)
            outBuffer[1][i] = inBuffer[1][i]/2.0f;

        reverb.processBlock(outBuffer[0], outBuffer[1], frames);
    }

private:
    VexReverb reverb;
    float parameters[3];

    PluginClassEND(VexReverbPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexReverbPlugin)
};

// -----------------------------------------------------------------------

class VexSynthPlugin : public PluginClass
{
public:
    static const unsigned int kParamCount = 1;

    VexSynthPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          synth(parameters)
    {
        std::memset(parameters, 0, sizeof(float)*92);

        for (int i = 0; i < 3; ++i)
        {
            const int offset = i * 24;

            parameters[offset +  1] = 0.5f;
            parameters[offset +  2] = 0.5f;
            parameters[offset +  3] = 0.5f;
            parameters[offset +  4] = 0.5f;
            parameters[offset +  5] = 0.9f;
            parameters[offset +  6] = 0.0f;
            parameters[offset +  7] = 1.0f;
            parameters[offset +  8] = 0.5f;
            parameters[offset +  9] = 0.0f;
            parameters[offset + 10] = 0.2f;
            parameters[offset + 11] = 0.0f;
            parameters[offset + 12] = 0.5f;
            parameters[offset + 13] = 0.5f;
            parameters[offset + 14] = 0.0f;
            parameters[offset + 15] = 0.3f;
            parameters[offset + 16] = 0.7f;
            parameters[offset + 17] = 0.1f;
            parameters[offset + 18] = 0.5f;
            parameters[offset + 19] = 0.5f;
            parameters[offset + 20] = 0.0f;
            parameters[offset + 21] = 0.0f;
            parameters[offset + 22] = 0.5f;
            parameters[offset + 23] = 0.5f;
            parameters[offset + 24] = 0.5f;
        }

        parameters[89] = 1.0f;

        synth.setSampleRate(getSampleRate());
        synth.update(89);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParamCount;
    }

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        static Parameter paramInfo;

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;

        paramInfo.name = nullptr;
        paramInfo.unit = nullptr;
        paramInfo.ranges.def       = 0.0f;
        paramInfo.ranges.min       = 0.0f;
        paramInfo.ranges.max       = 1.0f;
        paramInfo.ranges.step      = 1.0f;
        paramInfo.ranges.stepSmall = 1.0f;
        paramInfo.ranges.stepLarge = 1.0f;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_INTEGER;
            paramInfo.name = "Wave";
            paramInfo.ranges.def = 0.0f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = WaveRenderer::getWaveTableSize();
            break;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);

        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (index < kParamCount)
            return parameters[index];
        return 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
        {
            parameters[index] = value;
            synth.setWaveLater(1, WaveRenderer::getWaveTableName(value));

            //synth.update(index);
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** outBuffer, const uint32_t frames, const MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const MidiEvent* const midiEvent(&midiEvents[i]);

            const uint8_t status(MIDI_GET_STATUS_FROM_DATA(midiEvent->data));

            if (status == MIDI_STATUS_NOTE_ON)
            {
                synth.playNote(midiEvent->data[1], midiEvent->data[2], 0, 1);
            }
            else if (status == MIDI_STATUS_NOTE_OFF)
            {
                synth.releaseNote(midiEvent->data[1], 0, 1);
            }
            else if (status == MIDI_STATUS_CONTROL_CHANGE)
            {
                const uint8_t control(midiEvent->data[1]);

                if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                    synth.kill();
                else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                    synth.releaseAll(1);
            }
        }

        carla_zeroFloat(outBuffer[0], frames);
        carla_zeroFloat(outBuffer[1], frames);

        synth.doProcess(outBuffer[0], outBuffer[1], frames);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void sampleRateChanged(const double sampleRate) override
    {
        synth.setSampleRate(sampleRate);
    }

private:
    VexSyntModule synth;
    float parameters[92];

    PluginClassEND(VexSynthPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexSynthPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor vexArpDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_TIME),
    /* supports  */ static_cast<PluginSupports>(PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ VexArpPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "VexArp",
    /* label     */ "vexArp",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(VexArpPlugin)
};

static const PluginDescriptor vexChorusDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ VexChorusPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "VexChorus",
    /* label     */ "vexChorus",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(VexChorusPlugin)
};

static const PluginDescriptor vexDelayDesc = {
    /* category  */ PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_USES_TIME),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ VexDelayPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "VexDelay",
    /* label     */ "vexDelay",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(VexDelayPlugin)
};

static const PluginDescriptor vexReverbDesc = {
    /* category  */ PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ VexReverbPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "VexReverb",
    /* label     */ "vexReverb",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(VexReverbPlugin)
};

static const PluginDescriptor vexSynthDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
    /* hints     */ static_cast<PluginHints>(0x0),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ VexSynthPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "VexSynth",
    /* label     */ "vexSynth",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(VexSynthPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_vex()
{
    carla_register_native_plugin(&vexArpDesc);
    carla_register_native_plugin(&vexChorusDesc);
    carla_register_native_plugin(&vexDelayDesc);
    carla_register_native_plugin(&vexReverbDesc);
    carla_register_native_plugin(&vexSynthDesc);
}

// -----------------------------------------------------------------------

#include "vex/freeverb/allpass.cpp"
#include "vex/freeverb/comb.cpp"
#include "vex/freeverb/revmodel.cpp"

#include "vex/cVoice.cpp"
#include "vex/cWaveRenderer.cpp"
#include "vex/ResourceFile.cpp"

// -----------------------------------------------------------------------
