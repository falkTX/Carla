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

#include "CarlaNativeExtUI.hpp"

#include <cmath>

// -----------------------------------------------------------------------

class BigMeterPlugin : public NativePluginAndUiClass
{
public:
    BigMeterPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "/bigmeter-ui"),
          fColor(1),
          fOutLeft(0.0f),
          fOutRight(0.0f)
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return 3;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= 3)
            return nullptr;

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
            scalePoints[0].label = "Green";
            scalePoints[1].label = "Blue";
            scalePoints[0].value = 1.0f;
            scalePoints[1].value = 2.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            break;
        case 1:
            hints |= PARAMETER_IS_OUTPUT;
            param.name = "Out Left";
            break;
        case 2:
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
            return (float)fColor;
        case 1:
            return fOutLeft;
        case 2:
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
            fColor = (int)value;
            break;
        case 1:
            fOutLeft = value;
            break;
        case 2:
            fOutRight = value;
            break;
        default:
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** inputs, float**, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        float tmp, tmpLeft, tmpRight;
        tmpLeft = tmpRight = 0.0f;

        for (uint32_t i=0; i < frames; ++i)
        {
            tmp = std::abs(inputs[0][i]);

            if (tmp > tmpLeft)
                tmpLeft = tmp;

            tmp = std::abs(inputs[1][i]);

            if (tmp > tmpRight)
                tmpRight = tmp;
        }

        fOutLeft  = tmpLeft;
        fOutRight = tmpRight;
    }

private:
    int fColor;
    float fOutLeft, fOutRight;

    PluginClassEND(BigMeterPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BigMeterPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor bigmeterDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_UI),
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
void carla_register_native_plugin_bigmeter()
{
    carla_register_native_plugin(&bigmeterDesc);
}

// -----------------------------------------------------------------------
