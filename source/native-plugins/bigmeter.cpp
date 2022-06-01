/*
 * Carla Native Plugins
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaMathUtils.hpp"
#include "CarlaNativeExtUI.hpp"

#include "water/maths/MathsFunctions.h"

using water::roundToIntAccurate;

// -----------------------------------------------------------------------

class BigMeterPlugin : public NativePluginAndUiClass
{
public:
    BigMeterPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "bigmeter-ui"),
          fColor(1),
          fStyle(1),
          fOutLeft(0.0f),
          fOutRight(0.0f),
          fInlineDisplay() {}

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
        static NativeParameterScalePoint scalePoints[3];

        int hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMATABLE;

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
            hints |= NATIVE_PARAMETER_IS_INTEGER|NATIVE_PARAMETER_USES_SCALEPOINTS;
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
            hints |= NATIVE_PARAMETER_IS_INTEGER|NATIVE_PARAMETER_USES_SCALEPOINTS;
            param.name = "Style";
            param.ranges.def = 1.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 3.0f;
            scalePoints[0].value = 1.0f;
            scalePoints[0].label = "Default";
            scalePoints[1].value = 2.0f;
            scalePoints[1].label = "OpenAV";
            scalePoints[2].value = 3.0f;
            scalePoints[2].label = "RNCBC";
            param.scalePointCount = 3;
            param.scalePoints     = scalePoints;
            break;
        case 2:
            hints |= NATIVE_PARAMETER_IS_OUTPUT;
            param.name = "Out Left";
            break;
        case 3:
            hints |= NATIVE_PARAMETER_IS_OUTPUT;
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
            fColor = roundToIntAccurate(value);
            break;
        case 1:
            fStyle = roundToIntAccurate(value);
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

    void process(const float* const* inputs, float**, const uint32_t frames,
                 const NativeMidiEvent* const, const uint32_t) override
    {
        fOutLeft  = carla_findMaxNormalizedFloat(inputs[0], frames);
        fOutRight = carla_findMaxNormalizedFloat(inputs[1], frames);

        bool needsInlineRender = fInlineDisplay.pending < 0;

        if (carla_isNotEqual(fOutLeft, fInlineDisplay.lastLeft))
        {
            fInlineDisplay.lastLeft = fOutLeft;
            needsInlineRender = true;
        }

        if (carla_isNotEqual(fOutRight, fInlineDisplay.lastRight))
        {
            fInlineDisplay.lastRight = fOutRight;
            needsInlineRender = true;
        }

        if (needsInlineRender && fInlineDisplay.pending != 1 && fInlineDisplay.pending != 2)
        {
            fInlineDisplay.pending = 1;
            hostRequestIdle();
        }
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void idle() override
    {
        if (fInlineDisplay.pending == 1)
        {
            fInlineDisplay.pending = 2;
            hostQueueDrawInlineDisplay();
        }
    }

    const NativeInlineDisplayImageSurface* renderInlineDisplay(const uint32_t rwidth, const uint32_t height) override
    {
        CARLA_SAFE_ASSERT_RETURN(rwidth > 0 && height > 0, nullptr);

        const uint32_t width = rwidth == height ? height / 6 : rwidth;
        const size_t stride = width * 4;
        const size_t dataSize = stride * height;

        uchar* data = fInlineDisplay.data;

        if (fInlineDisplay.dataSize < dataSize || data == nullptr)
        {
            delete[] data;
            data = new uchar[dataSize];
            std::memset(data, 0, dataSize);
            fInlineDisplay.data = data;
            fInlineDisplay.dataSize = dataSize;
        }

        std::memset(data, 0, dataSize);

        fInlineDisplay.width = static_cast<int>(width);
        fInlineDisplay.height = static_cast<int>(height);
        fInlineDisplay.stride = static_cast<int>(stride);

        const uint heightValueLeft = static_cast<uint>(fInlineDisplay.lastLeft * static_cast<float>(height));
        const uint heightValueRight = static_cast<uint>(fInlineDisplay.lastRight * static_cast<float>(height));

        for (uint h=0; h < height; ++h)
        {
            for (uint w=0; w < width; ++w)
            {
//                 data[h * stride + w * 4 + 0] = 0;
//                 data[h * stride + w * 4 + 1] = 255;
//                 data[h * stride + w * 4 + 2] = 0;
                data[h * stride + w * 4 + 3] = 160;
            }
        }

        for (uint h=0; h < heightValueLeft; ++h)
        {
            const uint h2 = height - h - 1;

            for (uint w=0; w < width / 2; ++w)
            {
                data[h2 * stride + w * 4 + 0] = 200;
                data[h2 * stride + w * 4 + 1] = 0;
                data[h2 * stride + w * 4 + 2] = 0;
                data[h2 * stride + w * 4 + 3] = 255;
            }
        }

        for (uint h=0; h < heightValueRight; ++h)
        {
            const uint h2 = height - h - 1;

            for (uint w=width / 2; w < width; ++w)
            {
                data[h2 * stride + w * 4 + 0] = 200;
                data[h2 * stride + w * 4 + 1] = 0;
                data[h2 * stride + w * 4 + 2] = 0;
                data[h2 * stride + w * 4 + 3] = 255;
            }
        }

        // draw 1px border
        for (uint w=0; w < width; ++w)
        {
//             data[w * 4 + 0] = 0;
//             data[w * 4 + 1] = 0;
//             data[w * 4 + 2] = 255;
              data[w * 4 + 3] = 120;

//             data[(height - 1) * stride + w * 4 + 0] = 0;
//             data[(height - 1) * stride + w * 4 + 1] = 0;
//             data[(height - 1) * stride + w * 4 + 2] = 255;
            data[(height - 1) * stride + w * 4 + 3] = 120;
        }

        for (uint h=0; h < height; ++h)
        {
//             data[h * stride + 0] = 0;
//             data[h * stride + 1] = 0;
//             data[h * stride + 2] = 255;
            data[h * stride + 3] = 120;

            data[h * stride + (width / 2) * 4 + 0] = 0;
            data[h * stride + (width / 2) * 4 + 1] = 0;
            data[h * stride + (width / 2) * 4 + 2] = 0;
            data[h * stride + (width / 2) * 4 + 3] = 160;

//             data[h * stride + (width - 1) * 4 + 0] = 0;
//             data[h * stride + (width - 1) * 4 + 1] = 0;
//             data[h * stride + (width - 1) * 4 + 2] = 255;
            data[h * stride + (width - 1) * 4 + 3] = 120;
        }

        fInlineDisplay.pending = rwidth == height ? -1 : 0;
        return (NativeInlineDisplayImageSurface*)(NativeInlineDisplayImageSurfaceCompat*)&fInlineDisplay;
    }

private:
    int fColor, fStyle;
    float fOutLeft, fOutRight;

    struct InlineDisplay : NativeInlineDisplayImageSurfaceCompat {
        float lastLeft;
        float lastRight;
        volatile int pending;

        InlineDisplay()
            : NativeInlineDisplayImageSurfaceCompat(),
              lastLeft(0.0f),
              lastRight(0.0f),
              pending(0) {}

        ~InlineDisplay()
        {
            if (data != nullptr)
            {
                delete[] data;
                data = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPYABLE(InlineDisplay)
        CARLA_PREVENT_HEAP_ALLOCATION
    } fInlineDisplay;

    PluginClassEND(BigMeterPlugin)
    CARLA_DECLARE_NON_COPYABLE(BigMeterPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor bigmeterDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_INLINE_DISPLAY
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_REQUESTS_IDLE),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 2,
    /* paramOuts */ 2,
    /* name      */ "Big Meter",
    /* label     */ "bigmeter",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(BigMeterPlugin)
};

// -----------------------------------------------------------------------

CARLA_API_EXPORT
void carla_register_native_plugin_bigmeter();

CARLA_API_EXPORT
void carla_register_native_plugin_bigmeter()
{
    carla_register_native_plugin(&bigmeterDesc);
}

// -----------------------------------------------------------------------
