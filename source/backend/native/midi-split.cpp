/*
 * Carla Native Plugins
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_native.hpp"

#include <cstring>

class MidiSplitPlugin : public PluginDescriptorClass
{
public:
    MidiSplitPlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host)
    {
    }

    ~MidiSplitPlugin()
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float**, const uint32_t, const uint32_t midiEventCount, const MidiEvent* const midiEvents)
    {
        for (uint32_t i=0; i < midiEventCount; i++)
        {
            memcpy(&m_midiEvent, &midiEvents[i], sizeof(MidiEvent));

            const uint8_t status  = m_midiEvent.data[0] & 0xF0;
            const uint8_t channel = status & 0x0F;

            CARLA_ASSERT(channel < 16);

            if (channel >= 16)
                continue;

            m_midiEvent.port    = channel;
            m_midiEvent.data[0] = status;

            writeMidiEvent(&m_midiEvent);
        }
    }

    // -------------------------------------------------------------------

private:
    MidiEvent m_midiEvent;

    PluginDescriptorClassEND(MidiSplitPlugin)

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSplitPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor midiSplitDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ PLUGIN_IS_RTSAFE,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 16,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Split",
    /* label     */ "midiSplit",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(MidiSplitPlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiSplit()
{
    carla_register_native_plugin(&midiSplitDesc);
}

// -----------------------------------------------------------------------
