/*
 * XY Controller UI, taken from Cadence
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaMIDI.h"
#include "CarlaNativeExtUI.hpp"

#include "midi-queue.hpp"
#include "water/text/StringArray.h"

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
          params(),
          channels(),
          mqueue(),
          mqueueRT()
    {
        carla_zeroStruct(params);
        carla_zeroStruct(channels);
        channels[0] = true;
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

        int hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMATABLE;

        param.name = nullptr;
        param.unit = "%";
        param.ranges.def       = 0.0f;
        param.ranges.min       = -100.0f;
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

        return params[index];
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        switch (index)
        {
        case kParamInX:
        case kParamInY:
            params[index] = value;
            break;
        }
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        if (std::strcmp(key, "channels") == 0)
        {
            const water::StringArray chans(water::StringArray::fromTokens(value, ",", ""));

            carla_zeroStruct(channels);

            for (const water::String *it=chans.begin(), *end=chans.end(); it != end; ++it)
            {
                const int ichan = std::atoi((*it).toRawUTF8());
                CARLA_SAFE_ASSERT_INT_CONTINUE(ichan >= 1 && ichan <= 16, ichan);

                channels[ichan-1] = true;
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(const float* const*, float**, const uint32_t,
                 const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        params[kParamOutX] = params[kParamInX];
        params[kParamOutY] = params[kParamInY];

        if (mqueue.isNotEmpty() && mqueueRT.tryToCopyDataFrom(mqueue))
        {
            uint8_t d1, d2, d3;
            NativeMidiEvent ev = { 0, 0, 3, { 0, 0, 0, 0 } };

            while (mqueueRT.get(d1, d2, d3))
            {
                ev.data[0] = d1;
                ev.data[1] = d2;
                ev.data[2] = d3;
                writeMidiEvent(&ev);
            }
        }

        for (uint32_t i=0; i < midiEventCount; ++i)
            writeMidiEvent(&midiEvents[i]);
    }

#ifndef CARLA_OS_WASM
    // -------------------------------------------------------------------
    // Pipe Server calls

    bool msgReceived(const char* const msg) noexcept override
    {
        if (NativePluginAndUiClass::msgReceived(msg))
            return true;

        if (std::strcmp(msg, "cc") == 0)
        {
            uint8_t cc, value;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(cc), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(value), true);

            const CarlaMutexLocker cml(mqueue.getMutex());

            for (int i=0; i<16; ++i)
            {
                if (channels[i])
                    if (! mqueue.put(uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT)), cc, value))
                        break;
            }

            return true;
        }

        if (std::strcmp(msg, "cc2") == 0)
        {
            uint8_t cc1, value1, cc2, value2;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(cc1), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(value1), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(cc2), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(value2), true);

            const CarlaMutexLocker cml(mqueue.getMutex());

            for (int i=0; i<16; ++i)
            {
                if (channels[i])
                {
                    if (! mqueue.put(uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT)), cc1, value1))
                        break;
                    if (! mqueue.put(uint8_t(MIDI_STATUS_CONTROL_CHANGE | (i & MIDI_CHANNEL_BIT)), cc2, value2))
                        break;
                }
            }

            return true;
        }

        if (std::strcmp(msg, "note") == 0)
        {
            bool onOff;
            uint8_t note;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(onOff), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsByte(note), true);

            const uint8_t status   = onOff ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
            const uint8_t velocity = onOff ? 100 : 0;

            const CarlaMutexLocker cml(mqueue.getMutex());

            for (int i=0; i<16; ++i)
            {
                if (channels[i])
                    if (! mqueue.put(uint8_t(status | (i & MIDI_CHANNEL_BIT)), note, velocity))
                        break;
            }

            return true;
        }

        return false;
    }
#endif

private:
    float params[kParamCount];
    bool channels[16];

    MIDIEventQueue<128> mqueue, mqueueRT;

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
    /* midiIns   */ 1,
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

CARLA_API_EXPORT
void carla_register_native_plugin_xycontroller();

CARLA_API_EXPORT
void carla_register_native_plugin_xycontroller()
{
    carla_register_native_plugin(&notesDesc);
}

// -----------------------------------------------------------------------
