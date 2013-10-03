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

#include "vex/VexArp.h"
#include "vex/VexChorus.h"
#include "vex/VexDelay.h"
#include "vex/VexReverb.h"
#include "vex/VexSyntModule.h"

#include "vex/PeggyViewComponent.h"

// -----------------------------------------------------------------------

class HelperWindow : public DocumentWindow
{
public:
    HelperWindow()
        : DocumentWindow("PlugWindow", Colour(0, 0, 0), DocumentWindow::closeButton, false),
          fClosed(false)
    {
        setDropShadowEnabled(false);
        setOpaque(true);
        setResizable(false, false);
        //setUsingNativeTitleBar(true);
        setVisible(false);
    }

    void show(Component* const comp)
    {
        fClosed = false;

        setContentNonOwned(comp, true);
        centreWithSize(comp->getWidth(), comp->getHeight());

        if (! isOnDesktop())
            addToDesktop();

        setVisible(true);
    }

    void hide()
    {
        setVisible(false);

        if (isOnDesktop())
            removeFromDesktop();

        clearContentComponent();
    }

    bool wasClosedByUser() const
    {
        return fClosed;
    }

protected:
    void closeButtonPressed() override
    {
        fClosed = true;
    }

private:
    bool fClosed;
};

// -----------------------------------------------------------------------

class VexArpPlugin : public PluginClass,
                     public PeggyViewComponent::Callback
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
          fArp(&fSettings)
    {
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

        bool   timePlaying = false;
        double ppqPos      = 0.0;
        double barStartPos = 0.0;
        double bpm         = 120.0;

        if (timeInfo != nullptr)
        {
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
            fMidiInBuffer.addEvent(MidiMessage(midiEvent->data, midiEvent->size), midiEvent->time);
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
            tmpEvent.time = sampleNumber;

            if (tmpEvent.size > 4)
                continue;

            std::memcpy(tmpEvent.data, midiMessage.getRawData(), sizeof(uint8_t)*tmpEvent.size);
            writeMidiEvent(&tmpEvent);
        }
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (fWindow == nullptr)
            {
                fWindow = new HelperWindow();
                fWindow->setName(getUiName());
            }

            if (fView == nullptr)
            {
                fView = new PeggyViewComponent(1, fSettings, this);
                fView->setSize(207, 280);
            }

            fWindow->show(fView);
        }
        else if (fWindow != nullptr)
        {
            fWindow->hide();

            fView = nullptr;
            fWindow = nullptr;
        }
    }

    void uiIdle() override
    {
        if (fWindow == nullptr)
            return;

        if (fWindow->wasClosedByUser())
        {
            uiShow(false);
            uiClosed();
        }
    }

    void uiSetParameterValue(const uint32_t, const float) override
    {
        if (fView == nullptr)
            return;

        fView->update();
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void sampleRateChanged(const double sampleRate) override
    {
        fArp.setSampleRate(sampleRate);
    }

    void uiNameChanged(const char* const uiName) override
    {
        if (fWindow == nullptr)
            return;

        fWindow->setName(uiName);
    }

    // -------------------------------------------------------------------
    // Peggy callback

    void somethingChanged(const uint32_t id) override
    {
        uiParameterChanged(id, getParameterValue(id));
    }

private:
    VexArpSettings fSettings;
    VexArp fArp;
    MidiBuffer fMidiInBuffer;

    ScopedPointer<PeggyViewComponent> fView;
    ScopedPointer<HelperWindow> fWindow;

    PluginClassEND(VexArpPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexArpPlugin)
};

// -----------------------------------------------------------------------

class VexChorusPlugin : public PluginClass
{
public:
    enum Params {
        kParamRate = 0,
        kParamDepth,
        kParamCount
    };

    VexChorusPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          fChorus(fParameters)
    {
        std::memset(fParameters, 0, sizeof(float)*92);

        fParameters[76] = 0.3f;
        fParameters[77] = 0.6f;

        fChorus.setSampleRate(getSampleRate());
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
        paramInfo.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        paramInfo.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        paramInfo.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        switch (index)
        {
        case kParamRate:
            paramInfo.name = "Rate";
            paramInfo.ranges.def = 0.3f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 1.0f;
            break;
        case kParamDepth:
            paramInfo.name = "Depth";
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
        switch (index)
        {
        case kParamRate:
            return fParameters[76];
        case kParamDepth:
            return fParameters[77];
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
        case kParamRate:
            fParameters[76] = value;
            break;
        case kParamDepth:
            fParameters[77] = value;
            break;
        default:
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const MidiEvent* const, const uint32_t) override
    {
        if (inBuffer[0] != outBuffer[0])
            carla_copyFloat(outBuffer[0], inBuffer[0], frames);
        if (inBuffer[1] != outBuffer[1])
            carla_copyFloat(outBuffer[1], inBuffer[1], frames);

        fChorus.processBlock(outBuffer[0], outBuffer[1], frames);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void sampleRateChanged(const double sampleRate) override
    {
        fChorus.setSampleRate(sampleRate);
    }

private:
    VexChorus fChorus;
    float fParameters[92];

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
          fDelay(fParameters)
    {
        std::memset(fParameters, 0, sizeof(float)*92);

        fParameters[73] = 0.5f * 8.0f;
        fParameters[74] = 0.4f * 100.0f;

        fDelay.setSampleRate(getSampleRate());
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
        paramInfo.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        paramInfo.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        paramInfo.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
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
        switch (index)
        {
        case kParamTime:
            return fParameters[73] * 8.0f;
        case kParamFeedback:
            return fParameters[74] * 100.0f;
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
        case kParamTime:
            fParameters[73] = value/8.0f;
            break;
        case kParamFeedback:
            fParameters[74] = value/100.0f;
            break;
        default:
            break;
        }
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

        fDelay.processBlock(outBuffer[0], outBuffer[1], frames, bpm);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void sampleRateChanged(const double sampleRate) override
    {
        fDelay.setSampleRate(sampleRate);
    }

private:
    VexDelay fDelay;
    float fParameters[92];

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
          fReverb(fParameters)
    {
        std::memset(fParameters, 0, sizeof(float)*92);

        // FIXME?
        fParameters[79] = 0.6f;
        fParameters[80] = 0.6f;
        fParameters[81] = 0.7f;
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
        paramInfo.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        paramInfo.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        paramInfo.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
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
        switch (index)
        {
        case kParamSize:
            return fParameters[79];
        case kParamWidth:
            return fParameters[80];
        case kParamDamp:
            return fParameters[81];
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
        case kParamSize:
            fParameters[79] = value;
            break;
        case kParamWidth:
            fParameters[80] = value;
            break;
        case kParamDamp:
            fParameters[81] = value;
            break;
        default:
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const MidiEvent* const, const uint32_t) override
    {
        for (uint32_t i=0; i< frames; ++i)
            outBuffer[0][i] = inBuffer[0][i]/2.0f;
        for (uint32_t i=0; i< frames; ++i)
            outBuffer[1][i] = inBuffer[1][i]/2.0f;

        fReverb.processBlock(outBuffer[0], outBuffer[1], frames);
    }

private:
    VexReverb fReverb;
    float fParameters[92];

    PluginClassEND(VexReverbPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexReverbPlugin)
};

// -----------------------------------------------------------------------

class VexSynthPlugin : public PluginClass
{
public:
    static const unsigned int kParamCount = 92;

    VexSynthPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          obf(nullptr),
          abf(nullptr),
          dbf1(nullptr),
          dbf2(nullptr),
          dbf3(nullptr),
          fChorus(fParameters),
          fDelay(fParameters),
          fReverb(fParameters),
          fSynth(fParameters)
    {
        std::memset(fParameters, 0, sizeof(float)*92);

        fParameters[0] = 1.0f; // main volume

        for (int i = 0; i < 3; ++i)
        {
            const int offset = i * 24;

            fParameters[offset +  1] = 0.5f;
            fParameters[offset +  2] = 0.5f;
            fParameters[offset +  3] = 0.5f;
            fParameters[offset +  4] = 0.5f;
            fParameters[offset +  5] = 0.9f;
            fParameters[offset +  6] = 0.0f;
            fParameters[offset +  7] = 1.0f;
            fParameters[offset +  8] = 0.5f;
            fParameters[offset +  9] = 0.0f;
            fParameters[offset + 10] = 0.2f;
            fParameters[offset + 11] = 0.0f;
            fParameters[offset + 12] = 0.5f;
            fParameters[offset + 13] = 0.5f;
            fParameters[offset + 14] = 0.0f;
            fParameters[offset + 15] = 0.3f;
            fParameters[offset + 16] = 0.7f;
            fParameters[offset + 17] = 0.1f;
            fParameters[offset + 18] = 0.5f;
            fParameters[offset + 19] = 0.5f;
            fParameters[offset + 20] = 0.0f;
            fParameters[offset + 21] = 0.0f;
            fParameters[offset + 22] = 0.5f;
            fParameters[offset + 23] = 0.5f;
            fParameters[offset + 24] = 0.5f;
        }

        // ^1 - 72

        fParameters[73] = 0.5f; // Delay Time
        fParameters[74] = 0.4f; // Delay Feedback
        fParameters[75] = 0.0f; // Delay Volume

        fParameters[76] = 0.3f; // Chorus Rate
        fParameters[77] = 0.6f; // Chorus Depth
        fParameters[78] = 0.5f; // Chorus Volume

        fParameters[79] = 0.6f; // Reverb Size
        fParameters[80] = 0.7f; // Reverb Width
        fParameters[81] = 0.6f; // Reverb Damp
        fParameters[82] = 0.0f; // Reverb Volume

        fParameters[83] = 0.5f; // wave1 panning
        fParameters[84] = 0.5f; // wave2 panning
        fParameters[85] = 0.5f; // wave3 panning

        fParameters[86] = 0.5f; // wave1 volume
        fParameters[87] = 0.5f; // wave2 volume
        fParameters[88] = 0.5f; // wave3 volume

        fParameters[89] = 1.0f; // wave1 on/off
        fParameters[90] = 1.0f; // wave2 on/off
        fParameters[91] = 1.0f; // wave3 on/off

        bufferSizeChanged(getBufferSize());
        sampleRateChanged(getSampleRate());
    }

    ~VexSynthPlugin()
    {
        delete obf;
        delete abf;
        delete dbf1;
        delete dbf2;
        delete dbf3;
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
        paramInfo.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        paramInfo.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        paramInfo.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        if (index >= 1 && index <= 72)
        {
            uint32_t ri = index % 24;

            switch (ri)
            {
            case 1:
                paramInfo.name = "oct";
                break;
            case 2:
                paramInfo.name = "cent";
                break;
            case 3:
                paramInfo.name = "phaseOffset";
                break;
            case 4:
                paramInfo.name = "phaseIncOffset";
                break;
            case 5:
                paramInfo.name = "filter";
                break;
            case 6:
                paramInfo.name = "filter";
                break;
            case 7:
                paramInfo.name = "filter";
                break;
            case 8:
                paramInfo.name = "filter";
                break;
            case 9:
                paramInfo.name = "F ADSR";
                break;
            case 10:
                paramInfo.name = "F ADSR";
                break;
            case 11:
                paramInfo.name = "F ADSR";
                break;
            case 12:
                paramInfo.name = "F ADSR";
                break;
            case 13:
                paramInfo.name = "F velocity";
                break;
            case 14:
                paramInfo.name = "A ADSR";
                break;
            case 15:
                paramInfo.name = "A ADSR";
                break;
            case 16:
                paramInfo.name = "A ADSR";
                break;
            case 17:
                paramInfo.name = "A ADSR";
                break;
            case 18:
                paramInfo.name = "A velocity";
                break;
            case 19:
                paramInfo.name = "lfoC";
                break;
            case 20:
                paramInfo.name = "lfoA";
                break;
            case 21:
                paramInfo.name = "lfoF";
                break;
            case 22:
                paramInfo.name = "fx vol D";
                break;
            case 23:
                paramInfo.name = "fx vol C";
                break;
            case 24:
            case 0:
                paramInfo.name = "fx vol R";
                break;
            default:
                paramInfo.name = "unknown2";
                break;
            }

            paramInfo.hints = static_cast<ParameterHints>(hints);
            return &paramInfo;
        }

        switch (index)
        {
        case 0:
            paramInfo.name = "Main volume";
            break;
        case 73:
            paramInfo.name = "Delay Time";
            break;
        case 74:
            paramInfo.name = "Delay Feedback";
            break;
        case 75:
            paramInfo.name = "Delay Volume";
            break;
        case 76:
            paramInfo.name = "Chorus Rate";
            break;
        case 77:
            paramInfo.name = "Chorus Depth";
            break;
        case 78:
            paramInfo.name = "Chorus Volume";
            break;
        case 79:
            paramInfo.name = "Reverb Size";
            break;
        case 80:
            paramInfo.name = "Reverb Width";
            break;
        case 81:
            paramInfo.name = "Reverb Damp";
            break;
        case 82:
            paramInfo.name = "Reverb Volume";
            break;
        case 83:
            paramInfo.name = "Wave1 Panning";
            break;
        case 84:
            paramInfo.name = "Wave2 Panning";
            break;
        case 85:
            paramInfo.name = "Wave3 Panning";
            break;
        case 86:
            paramInfo.name = "Wave1 Volume";
            break;
        case 87:
            paramInfo.name = "Wave2 Volume";
            break;
        case 88:
            paramInfo.name = "Wave3 Volume";
            break;
        case 89:
            paramInfo.name = "Wave1 on/off";
            break;
        case 90:
            paramInfo.name = "Wave2 on/off";
            break;
        case 91:
            paramInfo.name = "Wave3 on/off";
            break;
        default:
            paramInfo.name = "unknown";
            break;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);
        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        return (index < kParamCount) ? fParameters[index] : 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
        {
            fParameters[index] = value;
            fSynth.update(index);
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** outBuffer, const uint32_t frames, const MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        const TimeInfo* const timeInfo(getTimeInfo());
        const double bpm((timeInfo != nullptr && timeInfo->bbt.valid) ? timeInfo->bbt.beatsPerMinute : 120.0);

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const MidiEvent* const midiEvent(&midiEvents[i]);

            const uint8_t status(MIDI_GET_STATUS_FROM_DATA(midiEvent->data));

            if (status == MIDI_STATUS_NOTE_ON)
            {
                fSynth.playNote(midiEvent->data[1], midiEvent->data[2], 0, 1);
            }
            else if (status == MIDI_STATUS_NOTE_OFF)
            {
                fSynth.releaseNote(midiEvent->data[1], 0, 1);
            }
            else if (status == MIDI_STATUS_CONTROL_CHANGE)
            {
                const uint8_t control(midiEvent->data[1]);

                if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                    fSynth.kill();
                else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                    fSynth.releaseAll(1);
            }
        }

        if (obf->getNumSamples() != (int)frames)
        {
                obf->setSize(2,  frames, 0, 0, 1);
                abf->setSize(2,  frames, 0, 0, 1);
                dbf1->setSize(2, frames, 0, 0, 1);
                dbf2->setSize(2, frames, 0, 0, 1);
                dbf3->setSize(2, frames, 0, 0, 1);
        }

        obf ->clear();
        dbf1->clear();
        dbf2->clear();
        dbf3->clear();

        fSynth.doProcess(*obf, *abf, *dbf1, *dbf2, *dbf3);

        if (fParameters[75] > 0.001f) fDelay.processBlock(dbf1, bpm);
        if (fParameters[78] > 0.001f) fChorus.processBlock(dbf2);
        if (fParameters[82] > 0.001f) fReverb.processBlock(dbf3);

        AudioSampleBuffer output(outBuffer, 2, 0, frames);
        output.clear();

        obf->addFrom(0, 0, *dbf1, 0, 0, frames, fParameters[75]);
        obf->addFrom(1, 0, *dbf1, 1, 0, frames, fParameters[75]);
        obf->addFrom(0, 0, *dbf2, 0, 0, frames, fParameters[78]);
        obf->addFrom(1, 0, *dbf2, 1, 0, frames, fParameters[78]);
        obf->addFrom(0, 0, *dbf3, 0, 0, frames, fParameters[82]);
        obf->addFrom(1, 0, *dbf3, 1, 0, frames, fParameters[82]);

        output.addFrom(0, 0, *obf, 0, 0, frames, fParameters[0]);
        output.addFrom(1, 0, *obf, 1, 0, frames, fParameters[0]);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        delete obf;
        delete abf;
        delete dbf1;
        delete dbf2;
        delete dbf3;

        obf  = new AudioSampleBuffer(2, bufferSize);
        abf  = new AudioSampleBuffer(2, bufferSize);
        dbf1 = new AudioSampleBuffer(2, bufferSize);
        dbf2 = new AudioSampleBuffer(2, bufferSize);
        dbf3 = new AudioSampleBuffer(2, bufferSize);
    }

    void sampleRateChanged(const double sampleRate) override
    {
        fChorus.setSampleRate(sampleRate);
        fDelay.setSampleRate(sampleRate);
        fSynth.setSampleRate(sampleRate);
    }

private:
    float fParameters[92];

    AudioSampleBuffer* obf;
    AudioSampleBuffer* abf;
    AudioSampleBuffer* dbf1; // delay
    AudioSampleBuffer* dbf2; // chorus
    AudioSampleBuffer* dbf3; // reverb

    VexChorus fChorus;
    VexDelay fDelay;
    VexReverb fReverb;
    VexSyntModule fSynth;

    PluginClassEND(VexSynthPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexSynthPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor vexArpDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_HAS_GUI|PLUGIN_NEEDS_SINGLE_THREAD|PLUGIN_USES_TIME),
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
