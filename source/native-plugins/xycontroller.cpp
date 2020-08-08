/*
 * XY Controller UI, taken from Cadence
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNativeExtUI.hpp"

// -----------------------------------------------------------------------

class XYControllerPlugin : public NativePluginAndUiClass
{
public:
    enum Parameters {
        kParamInX,
        kParamInY,
        kParamOutX,
        kParamOutY,
        kParamCount,
    };

    XYControllerPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "xycontroller-ui"),
          fParams()
    {
        carla_fill(fParams, 50.0f, kParamCount);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParamCount;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount, nullptr);

        static NativeParameter param;

        int hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMABLE;

        param.name = nullptr;
        param.unit = "%";
        param.ranges.def       = 50.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 100.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 0.01f;
        param.ranges.stepLarge = 10.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case kParamInX:
            param.name = "X";
            break;
        case kParamInY:
            param.name = "Y";
            break;
        case kParamOutX:
            hints |= NATIVE_PARAMETER_IS_OUTPUT;
            param.name = "Out X";
            break;
        case kParamOutY:
            hints |= NATIVE_PARAMETER_IS_OUTPUT;
            param.name = "Out Y";
            break;
        }

        param.hints = static_cast<NativeParameterHints>(hints);

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount, 0.0f);

        return fParams[index];
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        switch (index)
        {
        case kParamInX:
        case kParamInY:
            fParams[index] = value;
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(const float* const*, float**, const uint32_t,
                 const NativeMidiEvent* const, const uint32_t) override
    {
        fParams[kParamOutX] = fParams[kParamInX];
        fParams[kParamOutY] = fParams[kParamInY];
    }

private:
    float fParams[kParamCount];

    PluginClassEND(XYControllerPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYControllerPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor notesDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 1,
    /* paramIns  */ 2,
    /* paramOuts */ 2,
    /* name      */ "XY Controller",
    /* label     */ "xycontroller",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(XYControllerPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_xycontroller();

CARLA_EXPORT
void carla_register_native_plugin_xycontroller()
{
    carla_register_native_plugin(&notesDesc);
}

// -----------------------------------------------------------------------
