/*
 * Carla Native Plugins
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.h"

#ifdef CARLA_OS_WIN
# error This file should not be compiled for Windows
#endif

#include "CarlaMathUtils.hpp"
#include "CarlaNativeExtUI.hpp"

#include "juce_audio_basics.h"
using juce::FloatVectorOperations;
using juce::Range;

// -----------------------------------------------------------------------

class BigMeterPlugin : public NativePluginAndUiClass
{
public:
    BigMeterPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "/bigmeter-ui"),
          fColor(1),
          fStyle(1),
          fOutLeft(0.0f),
          fOutRight(0.0f),
          leakDetector_BigMeterPlugin() {}

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return 4;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < 4, nullptr);

        static NativeParameter param;
        static NativeParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 0.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 1.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_INTEGER|PARAMETER_USES_SCALEPOINTS;
            param.name = "Color";
            param.ranges.def = 1.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 2.0f;
            scalePoints[0].value = 1.0f;
            scalePoints[0].label = "Green";
            scalePoints[1].value = 2.0f;
            scalePoints[1].label = "Blue";
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            break;
        case 1:
            hints |= PARAMETER_IS_INTEGER|PARAMETER_USES_SCALEPOINTS;
            param.name = "Style";
            param.ranges.def = 1.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 2.0f;
            scalePoints[0].value = 1.0f;
            scalePoints[0].label = "Default";
            scalePoints[1].value = 2.0f;
            scalePoints[1].label = "OpenAV";
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            break;
        case 2:
            hints |= PARAMETER_IS_OUTPUT;
            param.name = "Out Left";
            break;
        case 3:
            hints |= PARAMETER_IS_OUTPUT;
            param.name = "Out Right";
            break;
        }

        param.hints = static_cast<NativeParameterHints>(hints);

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        switch (index)
        {
        case 0:
            return float(fColor);
        case 1:
            return float(fStyle);
        case 2:
            return fOutLeft;
        case 3:
            return fOutRight;
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
        case 0:
            fColor = int(value);
            break;
        case 1:
            fStyle = int(value);
            break;
        default:
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
        fOutLeft  = 0.0f;
        fOutRight = 0.0f;
    }

    void process(float** inputs, float**, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        Range<float> range;

        range     = FloatVectorOperations::findMinAndMax(inputs[0], static_cast<int>(frames));
        fOutLeft  = carla_max(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);

        range     = FloatVectorOperations::findMinAndMax(inputs[1], static_cast<int>(frames));
        fOutRight = carla_max(std::abs(range.getStart()), std::abs(range.getEnd()), 1.0f);
    }

private:
    int fColor, fStyle;
    float fOutLeft, fOutRight;

    PluginClassEND(BigMeterPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BigMeterPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor bigmeterDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_UI|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 2,
    /* name      */ "Big Meter",
    /* label     */ "bigmeter",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(BigMeterPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_bigmeter();

CARLA_EXPORT
void carla_register_native_plugin_bigmeter()
{
    carla_register_native_plugin(&bigmeterDesc);
}

// -----------------------------------------------------------------------
