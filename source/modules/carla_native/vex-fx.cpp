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

#include "juce_gui_basics.h"

using namespace juce;

#include "vex/VexArp.h"
#include "vex/VexChorus.h"
#include "vex/VexDelay.h"
#include "vex/VexReverb.h"

#include "vex/PeggyViewComponent.h"

// -----------------------------------------------------------------------

class HelperWindow : public DocumentWindow
{
public:
    HelperWindow()
        : DocumentWindow("PlugWindow", Colour(50, 50, 200), DocumentWindow::closeButton, false),
          fClosed(false)
    {
        setVisible(false);
        setAlwaysOnTop(true);
        setOpaque(true);
        setResizable(false, false);
        setUsingNativeTitleBar(true);
    }

    void show(Component* const comp)
    {
        fClosed = false;

        centreWithSize(comp->getWidth(), comp->getHeight());
        setContentNonOwned(comp, true);

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

    bool wasClosedByUser() const noexcept
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelperWindow)
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
        kParamLast,
        kParamCount = kParamLast + VexArpSettings::kVelocitiesSize + VexArpSettings::kGridSize
    };

    VexArpPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          fArp(&fSettings),
          fNeedsUpdate(true)
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
        static char bufName[24+1];

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

        if (index < kParamLast)
        {
            hints |= PARAMETER_IS_INTEGER;
            paramInfo.ranges.step      = 1.0f;
            paramInfo.ranges.stepSmall = 1.0f;
            paramInfo.ranges.stepLarge = 1.0f;

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
                scalePoints[0].label = "1/8";
                scalePoints[1].label = "1/16";
                scalePoints[2].label = "1/32";
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
        }
        else if (index < kParamLast + VexArpSettings::kVelocitiesSize)
        {
            carla_zeroChar(bufName, 24+1);
            std::snprintf(bufName, 24, "Grid Velocity %i", index - kParamLast);
            paramInfo.name = bufName;
        }
        else
        {
            carla_zeroChar(bufName, 24+1);
            hints |= PARAMETER_IS_BOOLEAN|PARAMETER_IS_INTEGER;
            paramInfo.ranges.step      = 1.0f;
            paramInfo.ranges.stepSmall = 1.0f;
            paramInfo.ranges.stepLarge = 1.0f;

            carla_zeroChar(bufName, 24+1);
            std::snprintf(bufName, 24, "Grid on/off %i", index - (kParamLast + VexArpSettings::kVelocitiesSize));
            paramInfo.name = bufName;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);
        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (index < kParamLast)
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
        else if (index < kParamLast + VexArpSettings::kVelocitiesSize)
        {
            return fSettings.velocities[index-kParamLast];
        }
        else
        {
            return fSettings.grid[index-(kParamLast+VexArpSettings::kVelocitiesSize)] ? 1.0f : 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamLast)
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
        else if (index < kParamLast + VexArpSettings::kVelocitiesSize)
        {
            fSettings.velocities[index-kParamLast] = value;
        }
        else
        {
            fSettings.grid[index-(kParamLast+VexArpSettings::kVelocitiesSize)] = (value >= 0.5f);
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
            fMidiInBuffer.addEvent(midiEvent->data, midiEvent->size, midiEvent->time);
        }

        const MidiBuffer& outMidiBuffer(fArp.processMidi(fMidiInBuffer, timePlaying, ppqPos, barStartPos, bpm, frames));

        MidiBuffer::Iterator outBufferIterator(outMidiBuffer);
        const uint8_t* midiData;
        int numBytes;
        int sampleNumber;

        MidiEvent tmpEvent;
        tmpEvent.port = 0;

        while (outBufferIterator.getNextEvent(midiData, numBytes, sampleNumber))
        {
            if (numBytes <= 0 || numBytes > 4)
                continue;

            tmpEvent.size = numBytes;
            tmpEvent.time = sampleNumber;

            std::memcpy(tmpEvent.data, midiData, sizeof(uint8_t)*tmpEvent.size);
            writeMidiEvent(&tmpEvent);
        }
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        MessageManagerLock mmLock;

        if (show)
        {
            if (fWindow == nullptr)
            {
                fWindow = new HelperWindow();
                fWindow->setName(getUiName());
            }

            if (fView == nullptr)
            {
                fView = new PeggyViewComponent(fSettings, this, true);
                fView->setSize(202, 275);
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

        if (fNeedsUpdate)
        {
            if (fView != nullptr)
            {
                MessageManagerLock mmLock;
                fView->update();
            }

            fNeedsUpdate = false;
        }

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

        fNeedsUpdate = true;
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

        MessageManagerLock mmLock;
        fWindow->setName(uiName);
    }

    // -------------------------------------------------------------------
    // Peggy callback

    void arpParameterChanged(const uint32_t id) override
    {
        uiParameterChanged(id, getParameterValue(id));
    }

private:
    VexArpSettings fSettings;
    VexArp fArp;
    MidiBuffer fMidiInBuffer;

    volatile bool fNeedsUpdate;

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
            break;
        case kParamDepth:
            paramInfo.name = "Depth";
            paramInfo.ranges.def = 0.6f;
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
            FloatVectorOperations::copy(outBuffer[0], inBuffer[0], frames);
        if (inBuffer[1] != outBuffer[1])
            FloatVectorOperations::copy(outBuffer[1], inBuffer[1], frames);

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

        fParameters[73] = 0.5f;
        fParameters[74] = 0.4f;

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
            paramInfo.unit = "steps";
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
            FloatVectorOperations::copy(outBuffer[0], inBuffer[0], frames);
        if (inBuffer[1] != outBuffer[1])
            FloatVectorOperations::copy(outBuffer[1], inBuffer[1], frames);

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

        fParameters[79] = 0.6f;
        fParameters[80] = 0.7f;
        fParameters[81] = 0.6f;
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
            break;
        case kParamWidth:
            paramInfo.name = "Width";
            paramInfo.ranges.def = 0.7f;
            break;
        case kParamDamp:
            paramInfo.name = "Damp";
            paramInfo.ranges.def = 0.6f;
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
            FloatVectorOperations::copyWithMultiply(outBuffer[0], inBuffer[0], 0.5f, frames);
        for (uint32_t i=0; i< frames; ++i)
            FloatVectorOperations::copyWithMultiply(outBuffer[1], inBuffer[1], 0.5f, frames);

        fReverb.processBlock(outBuffer[0], outBuffer[1], frames);
    }

private:
    VexReverb fReverb;
    float fParameters[92];

    PluginClassEND(VexReverbPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexReverbPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor vexArpDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_HAS_GUI|PLUGIN_NEEDS_UI_JUCE|PLUGIN_USES_TIME),
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

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_vex_fx()
{
    carla_register_native_plugin(&vexArpDesc);
    carla_register_native_plugin(&vexChorusDesc);
    carla_register_native_plugin(&vexDelayDesc);
    carla_register_native_plugin(&vexReverbDesc);
}

// -----------------------------------------------------------------------
