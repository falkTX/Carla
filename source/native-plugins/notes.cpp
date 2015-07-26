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

#include "CarlaNativeExtUI.hpp"

// -----------------------------------------------------------------------

class NotesPlugin : public NativePluginAndUiClass
{
public:
    NotesPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "notes-ui"),
          fCurPage(1) {}

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return 1;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        if (index != 0)
            return nullptr;

        static NativeParameter param;

        param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_ENABLED
                                                       |NATIVE_PARAMETER_IS_AUTOMABLE
                                                       |NATIVE_PARAMETER_IS_INTEGER);
        param.name  = "Page";
        param.unit  = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 1.0f;
        param.ranges.max       = 100.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (index != 0)
            return 0.0f;

        return static_cast<float>(fCurPage);
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index != 0)
            return;

        fCurPage = static_cast<int>(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float**, const uint32_t, const NativeMidiEvent* const, const uint32_t) override
    {
    }

private:
    int fCurPage;

    PluginClassEND(NotesPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotesPlugin)
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
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ "Notes",
    /* label     */ "notes",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(NotesPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_notes();

CARLA_EXPORT
void carla_register_native_plugin_notes()
{
    carla_register_native_plugin(&notesDesc);
}

// -----------------------------------------------------------------------
