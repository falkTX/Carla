/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoPluginInternal.hpp"

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
# include "dssi/dssi.h"
#else
# include "ladspa/ladspa.h"
# if DISTRHO_PLUGIN_IS_SYNTH
#  error Cannot build synth plugin with LADSPA
# endif
# if DISTRHO_PLUGIN_WANT_STATE
#  warning LADSPA cannot handle states
# endif
#endif

#include <vector>

typedef LADSPA_Data*                LADSPA_DataPtr;
typedef std::vector<LADSPA_Data>    LADSPA_DataVector;
typedef std::vector<LADSPA_DataPtr> LADSPA_DataPtrVector;

// -------------------------------------------------

START_NAMESPACE_DISTRHO

class PluginLadspaDssi
{
public:
    PluginLadspaDssi()
    {
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            fPortAudioIns[i] = nullptr;

        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            fPortAudioOuts[i] = nullptr;

        {
            const uint32_t count(fPlugin.parameterCount());

            fPortControls.resize(count, nullptr);
            fLastControlValues.resize(count, 0.0f);

            for (uint32_t i=0; i < count; ++i)
                fLastControlValues[i] = fPlugin.parameterValue(i);
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        fPortLatency = nullptr;
#endif
    }

    ~PluginLadspaDssi()
    {
        fLastControlValues.clear();
        fPortControls.clear();
    }

    // ---------------------------------------------

    void ladspa_connect_port(const unsigned long port, const LADSPA_DataPtr dataLocation)
    {
        unsigned long i, count, index = 0;

        for (i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioIns[i] = dataLocation;
                return;
            }
        }

        for (i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioOuts[i] = dataLocation;
                return;
            }
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        if (port == index++)
        {
            fPortLatency = dataLocation;
            return;
        }
#endif

        for (i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            if (port == index++)
            {
                fPortControls[i] = dataLocation;
                return;
            }
        }
    }

    // ---------------------------------------------

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
# if DISTRHO_PLUGIN_WANT_STATE
    char* dssi_configure(const char* const key, const char* const value)
    {
        if (strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX, strlen(DSSI_RESERVED_CONFIGURE_PREFIX) == 0))
            return nullptr;
        if (strncmp(key, DSSI_GLOBAL_CONFIGURE_PREFIX, strlen(DSSI_GLOBAL_CONFIGURE_PREFIX) == 0))
            return nullptr;

        fPlugin.setState(key, value);
        return nullptr;
    }
# endif

# if DISTRHO_PLUGIN_WANT_PROGRAMS
    const DSSI_Program_Descriptor* dssi_get_program(const unsigned long index)
    {
        if (index >= fPlugin.programCount())
            return nullptr;

        static DSSI_Program_Descriptor desc;

        desc.Bank    = index / 128;
        desc.Program = index % 128;
        desc.Name    = fPlugin.programName(index);

        return &desc;
    }

    void dssi_select_program(const unsigned long bank, const unsigned long program)
    {
        const unsigned long realProgram(bank * 128 + program);

        if (realProgram >= fPlugin.programCount())
            return;

        fPlugin.setProgram(realProgram);

        // Update parameters
        for (uint32_t i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            if (! fPlugin.parameterIsOutput(i))
            {
                fLastControlValues[i] = fPlugin.parameterValue(i);

                if (fPortControls[i])
                    *fPortControls[i] = fLastControlValues[i];
            }
        }
    }
# endif
#endif

    // ---------------------------------------------

    void ladspa_activate()
    {
        fPlugin.activate();
    }

    void ladspa_deactivate()
    {
        fPlugin.deactivate();
    }

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
    void ladspa_run(const unsigned long bufferSize)
    {
        dssi_run_synth(bufferSize, nullptr, 0);
    }

    void dssi_run_synth(const unsigned long bufferSize, snd_seq_event_t* const events, const unsigned long eventCount)
#else
    void ladspa_run(const unsigned long bufferSize)
#endif
    {
        // Check for updated parameters
        float curValue;

        for (uint32_t i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            curValue = *fPortControls[i];

            if (fLastControlValues[i] != curValue && ! fPlugin.parameterIsOutput(i))
            {
                fLastControlValues[i] = curValue;
                fPlugin.setParameterValue(i, curValue);
            }
        }

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
# if DISTRHO_PLUGIN_IS_SYNTH
        // Get MIDI Events
        uint32_t midiEventCount = 0;

        for (uint32_t i=0, j; i < eventCount && midiEventCount < MAX_MIDI_EVENTS; ++i)
        {
            const snd_seq_event_t& seqEvent(events[i]);

            if (seqEvent.data.note.channel > 0xF)
                continue;

            switch (seqEvent.type)
            {
            case SND_SEQ_EVENT_NOTEON:
                j = midiEventCount++;
                fMidiEvents[j].frame  = seqEvent.time.tick;
                fMidiEvents[j].buf[0] = 0x90 + seqEvent.data.note.channel;
                fMidiEvents[j].buf[1] = seqEvent.data.note.note;
                fMidiEvents[j].buf[2] = seqEvent.data.note.velocity;
                fMidiEvents[j].buf[3] = 0;
                fMidiEvents[j].size   = 3;
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                j = midiEventCount++;
                fMidiEvents[j].frame  = seqEvent.time.tick;
                fMidiEvents[j].buf[0] = 0x80 + seqEvent.data.note.channel;
                fMidiEvents[j].buf[1] = seqEvent.data.note.note;
                fMidiEvents[j].buf[2] = 0;
                fMidiEvents[j].buf[3] = 0;
                fMidiEvents[j].size   = 3;
                break;
            case SND_SEQ_EVENT_KEYPRESS:
                j = midiEventCount++;
                fMidiEvents[j].frame  = seqEvent.time.tick;
                fMidiEvents[j].buf[0] = 0xA0 + seqEvent.data.note.channel;
                fMidiEvents[j].buf[1] = seqEvent.data.note.note;
                fMidiEvents[j].buf[2] = seqEvent.data.note.velocity;
                fMidiEvents[j].buf[3] = 0;
                fMidiEvents[j].size   = 3;
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                j = midiEventCount++;
                fMidiEvents[j].frame  = seqEvent.time.tick;
                fMidiEvents[j].buf[0] = 0xB0 + seqEvent.data.control.channel;
                fMidiEvents[j].buf[1] = seqEvent.data.control.param;
                fMidiEvents[j].buf[2] = seqEvent.data.control.value;
                fMidiEvents[j].buf[3] = 0;
                fMidiEvents[j].size   = 3;
                break;
            case SND_SEQ_EVENT_CHANPRESS:
                j = midiEventCount++;
                fMidiEvents[j].frame  = seqEvent.time.tick;
                fMidiEvents[j].buf[0] = 0xD0 + seqEvent.data.control.channel;
                fMidiEvents[j].buf[1] = seqEvent.data.control.value;
                fMidiEvents[j].buf[2] = 0;
                fMidiEvents[j].buf[3] = 0;
                fMidiEvents[j].size   = 2;
                break;
            case SND_SEQ_EVENT_PITCHBEND: // TODO
                j = midiEventCount++;
                fMidiEvents[j].frame  = seqEvent.time.tick;
                fMidiEvents[j].buf[0] = 0xE0 + seqEvent.data.control.channel;
                fMidiEvents[j].buf[1] = 0;
                fMidiEvents[j].buf[2] = 0;
                fMidiEvents[j].buf[3] = 0;
                fMidiEvents[j].size   = 3;
                break;
            }
        }
# else
        return;
        // unused
        (void)events;
        (void)eventCount;
# endif
#endif

        // Run plugin for this cycle
#if DISTRHO_PLUGIN_IS_SYNTH
        fPlugin.run(fPortAudioIns, fPortAudioOuts, bufferSize, midiEventCount, fMidiEvents);
#else
        fPlugin.run(fPortAudioIns, fPortAudioOuts, bufferSize, 0, nullptr);
#endif

        updateParameterOutputs();
    }

    // ---------------------------------------------

private:
    PluginInternal fPlugin;

    LADSPA_DataPtr       fPortAudioIns[DISTRHO_PLUGIN_NUM_INPUTS];
    LADSPA_DataPtr       fPortAudioOuts[DISTRHO_PLUGIN_NUM_INPUTS];
    LADSPA_DataPtrVector fPortControls;
#if DISTRHO_PLUGIN_WANT_LATENCY
    LADSPA_DataPtr       fPortLatency;
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    MidiEvent fMidiEvents[MAX_MIDI_EVENTS];
#endif

    LADSPA_DataVector fLastControlValues;

    // ---------------------------------------------

    void updateParameterOutputs()
    {
        for (uint32_t i=0, count=fPlugin.parameterCount(); i < count; ++i)
        {
            if (fPlugin.parameterIsOutput(i))
            {
                fLastControlValues[i] = fPlugin.parameterValue(i);

                if (fPortControls[i] != nullptr)
                    *fPortControls[i] = fLastControlValues[i];
            }
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        if (fPortLatency != nullptr)
            *fPortLatency = fPlugin.latency();
#endif
    }
};

// -------------------------------------------------

static LADSPA_Handle ladspa_instantiate(const LADSPA_Descriptor*, unsigned long sampleRate)
{
    if (d_lastBufferSize == 0)
        d_lastBufferSize = 2048;
    d_lastSampleRate = sampleRate;

    return new PluginLadspaDssi();
}

#define instancePtr ((PluginLadspaDssi*)instance)

static void ladspa_connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data* dataLocation)
{
    instancePtr->ladspa_connect_port(port, dataLocation);
}

static void ladspa_activate(LADSPA_Handle instance)
{
    instancePtr->ladspa_activate();
}

static void ladspa_run(LADSPA_Handle instance, unsigned long sampleCount)
{
    instancePtr->ladspa_run(sampleCount);
}

static void ladspa_deactivate(LADSPA_Handle instance)
{
    instancePtr->ladspa_deactivate();
}

static void ladspa_cleanup(LADSPA_Handle instance)
{
    delete instancePtr;
}

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
# if DISTRHO_PLUGIN_WANT_STATE
static char* dssi_configure(LADSPA_Handle instance, const char* key, const char* value)
{
    return instancePtr->dssi_configure(key, value);
}
# endif

# if DISTRHO_PLUGIN_WANT_PROGRAMS
static const DSSI_Program_Descriptor* dssi_get_program(LADSPA_Handle instance, unsigned long index)
{
    return instancePtr->dssi_get_program(index);
}

static void dssi_select_program(LADSPA_Handle instance, unsigned long bank, unsigned long program)
{
    instancePtr->dssi_select_program(bank, program);
}
# endif

# if DISTRHO_PLUGIN_IS_SYNTH
static void dssi_run_synth(LADSPA_Handle instance, unsigned long sampleCount, snd_seq_event_t* events, unsigned long eventCount)
{
    instancePtr->dssi_run_synth(sampleCount, events, eventCount);
}
# endif
#endif

#undef instancePtr

// -------------------------------------------------

static LADSPA_Descriptor sLadspaDescriptor = {
    /* UniqueID   */ 0,
    /* Label      */ nullptr,
    /* Properties */ LADSPA_PROPERTY_REALTIME | LADSPA_PROPERTY_HARD_RT_CAPABLE,
    /* Name       */ nullptr,
    /* Maker      */ nullptr,
    /* Copyright  */ nullptr,
    /* PortCount  */ 0,
    /* PortDescriptors    */ nullptr,
    /* PortNames          */ nullptr,
    /* PortRangeHints     */ nullptr,
    /* ImplementationData */ nullptr,
    ladspa_instantiate,
    ladspa_connect_port,
    ladspa_activate,
    ladspa_run,
    /* run_adding */          nullptr,
    /* set_run_adding_gain */ nullptr,
    ladspa_deactivate,
    ladspa_cleanup
};

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
static DSSI_Descriptor sDssiDescriptor = {
    1,
    &sLadspaDescriptor,
# if DISTRHO_PLUGIN_WANT_STATE
    dssi_configure,
# else
    /* configure                    */ nullptr,
# endif
# if DISTRHO_PLUGIN_WANT_PROGRAMS
    dssi_get_program,
    dssi_select_program,
# else
    /* get_program                  */ nullptr,
    /* select_program               */ nullptr,
# endif
    /* get_midi_controller_for_port */ nullptr,
# if DISTRHO_PLUGIN_IS_SYNTH
    dssi_run_synth,
# else
    /* run_synth                    */ nullptr,
# endif
    /* run_synth_adding             */ nullptr,
    /* run_multiple_synths          */ nullptr,
    /* run_multiple_synths_adding   */ nullptr,
    nullptr, nullptr
};
#endif

// -------------------------------------------------

class DescriptorInitializer
{
public:
    DescriptorInitializer()
    {
        // Create dummy plugin to get data from
        d_lastBufferSize = 512;
        d_lastSampleRate = 44100.0;
        PluginInternal plugin;
        d_lastBufferSize = 0;
        d_lastSampleRate = 0.0;

        // Get port count, init
        unsigned long i, port = 0;
        unsigned long portCount = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS + plugin.parameterCount();
#if DISTRHO_PLUGIN_WANT_LATENCY
        portCount += 1;
#endif
        const char** const portNames = new const char* [portCount];
        LADSPA_PortDescriptor* portDescriptors = new LADSPA_PortDescriptor [portCount];
        LADSPA_PortRangeHint*  portRangeHints  = new LADSPA_PortRangeHint  [portCount];

        // Set ports
        for (i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i, ++port)
        {
            char portName[24] = { '\0' };
            sprintf(portName, "Audio Input %lu", i+1);

            portNames[port]       = strdup(portName);
            portDescriptors[port] = LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT;

            portRangeHints[port].HintDescriptor = 0;
            portRangeHints[port].LowerBound = 0.0f;
            portRangeHints[port].UpperBound = 1.0f;
        }

        for (i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i, ++port)
        {
            char portName[24] = { '\0' };
            sprintf(portName, "Audio Output %lu", i+1);

            portNames[port]       = strdup(portName);
            portDescriptors[port] = LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT;

            portRangeHints[port].HintDescriptor = 0;
            portRangeHints[port].LowerBound = 0.0f;
            portRangeHints[port].UpperBound = 1.0f;
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        // Set latency port
        portNames[port]       = strdup("_latency");
        portDescriptors[port] = LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT;
        portRangeHints[port].HintDescriptor = LADSPA_HINT_SAMPLE_RATE;
        portRangeHints[port].LowerBound     = 0.0f;
        portRangeHints[port].UpperBound     = 1.0f;
        ++port;
#endif

        for (i=0; i < plugin.parameterCount(); ++i, ++port)
        {
            portNames[port]       = strdup((const char*)plugin.parameterName(i));
            portDescriptors[port] = LADSPA_PORT_CONTROL;

            if (plugin.parameterIsOutput(i))
                portDescriptors[port] |= LADSPA_PORT_OUTPUT;
            else
                portDescriptors[port] |= LADSPA_PORT_INPUT;

            {
                const ParameterRanges& ranges(plugin.parameterRanges(i));
                const float defValue = ranges.def;

                portRangeHints[port].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
                portRangeHints[port].LowerBound     = ranges.min;
                portRangeHints[port].UpperBound     = ranges.max;

                if (defValue == 0.0f)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_0;
                else if (defValue == 1.0f)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_1;
                else if (defValue == 100.0f)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_100;
                else if (defValue == 440.0f)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_440;
                else if (ranges.min == defValue)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_MINIMUM;
                else if (ranges.max == defValue)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_MAXIMUM;
                else
                {
                    const float middleValue = ranges.min/2 + ranges.max/2;
                    const float middleLow   = (ranges.min/2 + middleValue/2)/2 + middleValue/2;
                    const float middleHigh  = (ranges.max/2 + middleValue/2)/2 + middleValue/2;

                    if (defValue < middleLow)
                        portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_LOW;
                    else if (defValue > middleHigh)
                        portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_HIGH;
                    else
                        portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_MIDDLE;
                }
            }

            {
                const uint32_t hints(plugin.parameterHints(i));

                if (hints & PARAMETER_IS_BOOLEAN)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_TOGGLED;
                if (hints & PARAMETER_IS_INTEGER)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_INTEGER;
                if (hints & PARAMETER_IS_LOGARITHMIC)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_LOGARITHMIC;
            }
        }

        // Set data
        sLadspaDescriptor.UniqueID  = plugin.uniqueId();
        sLadspaDescriptor.Label     = strdup(plugin.label());
        sLadspaDescriptor.Name      = strdup(plugin.name());
        sLadspaDescriptor.Maker     = strdup(plugin.maker());
        sLadspaDescriptor.Copyright = strdup(plugin.license());
        sLadspaDescriptor.PortCount = portCount;
        sLadspaDescriptor.PortNames = portNames;
        sLadspaDescriptor.PortDescriptors = portDescriptors;
        sLadspaDescriptor.PortRangeHints  = portRangeHints;
    }

    ~DescriptorInitializer()
    {
        if (sLadspaDescriptor.Label != nullptr)
            free((void*)sLadspaDescriptor.Label);

        if (sLadspaDescriptor.Name != nullptr)
            free((void*)sLadspaDescriptor.Name);

        if (sLadspaDescriptor.Maker != nullptr)
            free((void*)sLadspaDescriptor.Maker);

        if (sLadspaDescriptor.Copyright != nullptr)
            free((void*)sLadspaDescriptor.Copyright);

        if (sLadspaDescriptor.PortDescriptors != nullptr)
            delete[] sLadspaDescriptor.PortDescriptors;

        if (sLadspaDescriptor.PortRangeHints != nullptr)
            delete[] sLadspaDescriptor.PortRangeHints;

        if (sLadspaDescriptor.PortNames != nullptr)
        {
            for (unsigned long i=0; i < sLadspaDescriptor.PortCount; ++i)
            {
                if (sLadspaDescriptor.PortNames[i] != nullptr)
                    free((void*)sLadspaDescriptor.PortNames[i]);
            }

            delete[] sLadspaDescriptor.PortNames;
        }
    }
};

static DescriptorInitializer sInit;

END_NAMESPACE_DISTRHO

// -------------------------------------------------

DISTRHO_PLUGIN_EXPORT
const LADSPA_Descriptor* ladspa_descriptor(unsigned long index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sLadspaDescriptor : nullptr;
}

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
DISTRHO_PLUGIN_EXPORT
const DSSI_Descriptor* dssi_descriptor(unsigned long index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sDssiDescriptor : nullptr;
}
#endif
