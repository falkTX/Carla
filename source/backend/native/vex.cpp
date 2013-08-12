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

using namespace juce;

#include "vex/cArp.h"

class VexArpPlugin : public PluginDescriptorClass
{
public:
    enum Params {
        kParamLength = 0,
        kParamTimeMode,
        kParamSyncMode,
        kParamFailMode,
        kParamVelMode,
        kParamCount
    };

    VexArpPlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host),
          settings(),
          arp(&settings),
          fWetOnly(false)
    {
        arp.setSampleRate(getSampleRate());
        inMidiBuffer.ensureSize(512*4);

        for (int i=0; i < 8; ++i)
            settings.grid[i*10] = true;

        settings.grid[1] = true;
        settings.grid[2] = true;
        settings.grid[3] = true;

        settings.grid[41] = true;
        settings.grid[42] = true;
        settings.grid[43] = true;
        settings.grid[44] = true;
        settings.grid[45] = true;
    }

    ~VexArpPlugin() override
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() override
    {
        return kParamCount;
    }

    const Parameter* getParameterInfo(const uint32_t index) override
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

    float getParameterValue(const uint32_t index) override
    {
        switch (index)
        {
        case kParamLength:
            return settings.length;
        case kParamTimeMode:
            return settings.timeMode;
        case kParamSyncMode:
            return settings.syncMode;
        case kParamFailMode:
            return settings.failMode;
        case kParamVelMode:
            return settings.velMode;
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value)
    {
        switch (index)
        {
        case kParamLength:
            settings.length = value;
            break;
        case kParamTimeMode:
            settings.timeMode = value;
            break;
        case kParamSyncMode:
            settings.syncMode = value;
            break;
        case kParamFailMode:
            settings.failMode = value;
            break;
        case kParamVelMode:
            settings.velMode = value;
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float**, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents) override
    {
        const TimeInfo* const timeInfo(getTimeInfo());
        double ppqPos, barStartPos, bpm;

        if (timeInfo->bbt.valid)
        {
            ppqPos = 0.0;
            barStartPos = 0.0;
            bpm = timeInfo->bbt.beatsPerMinute;
        }
        else
        {
            double ppqBar  = double(timeInfo->bbt.bar - 1) * timeInfo->bbt.beatsPerBar;
            double ppqBeat = double(timeInfo->bbt.beat - 1);
            double ppqTick = double(timeInfo->bbt.tick) / timeInfo->bbt.ticksPerBeat;

            ppqPos = ppqBar + ppqBeat + ppqTick;
            barStartPos = ppqBar;
            bpm = 120.0;
        }

        inMidiBuffer.clear();

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const MidiEvent* const midiEvent(&midiEvents[i]);
            inMidiBuffer.addEvent(MidiMessage(midiEvent->data, midiEvent->size, midiEvent->time), 0/*timeInfo->frame*/);
        }

        const MidiBuffer& outMidiBuffer(arp.processMidi(inMidiBuffer, timeInfo->playing, ppqPos, barStartPos, bpm, frames));

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

private:
    PeggySettings settings;
    cArp arp;
    MidiBuffer inMidiBuffer;
    bool fWetOnly;

    PluginDescriptorClassEND(VexArpPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexArpPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor vexArpDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(0x0),
    /* supports  */ static_cast<PluginSupports>(PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ VexArpPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "Vex Arp",
    /* label     */ "vexArp",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(VexArpPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_vex()
{
    carla_register_native_plugin(&vexArpDesc);
}

// -----------------------------------------------------------------------
