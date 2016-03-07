/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoPluginInternal.hpp"

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
# error Cannot use MIDI Output with LADSPA or DSSI
#endif

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
# include "dssi/dssi.h"
#else
# include "ladspa/ladspa.h"
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
#  error Cannot use MIDI with LADSPA
# endif
# if DISTRHO_PLUGIN_WANT_STATE
#  warning LADSPA cannot handle states
# endif
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
# warning LADSPA/DSSI does not support TimePos
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class PluginLadspaDssi
{
public:
    PluginLadspaDssi()
        : fPortControls(nullptr),
          fLastControlValues(nullptr)
    {
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            fPortAudioIns[i] = nullptr;
#else
        fPortAudioIns = nullptr;
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            fPortAudioOuts[i] = nullptr;
#else
        fPortAudioOuts = nullptr;
#endif

        if (const uint32_t count = fPlugin.getParameterCount())
        {
            fPortControls      = new LADSPA_Data*[count];
            fLastControlValues = new LADSPA_Data[count];

            for (uint32_t i=0; i < count; ++i)
            {
                fPortControls[i] = nullptr;
                fLastControlValues[i] = fPlugin.getParameterValue(i);
            }
        }
        else
        {
            fPortControls = nullptr;
            fLastControlValues = nullptr;
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        fPortLatency = nullptr;
#endif
    }

    ~PluginLadspaDssi() noexcept
    {
        if (fPortControls != nullptr)
        {
            delete[] fPortControls;
            fPortControls = nullptr;
        }

        if (fLastControlValues != nullptr)
        {
            delete[] fLastControlValues;
            fLastControlValues = nullptr;
        }
    }

    // -------------------------------------------------------------------

    void ladspa_activate()
    {
        fPlugin.activate();
    }

    void ladspa_deactivate()
    {
        fPlugin.deactivate();
    }

    // -------------------------------------------------------------------

    void ladspa_connect_port(const ulong port, LADSPA_Data* const dataLocation) noexcept
    {
        ulong index = 0;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (ulong i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioIns[i] = dataLocation;
                return;
            }
        }
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (ulong i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            if (port == index++)
            {
                fPortAudioOuts[i] = dataLocation;
                return;
            }
        }
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
        if (port == index++)
        {
            fPortLatency = dataLocation;
            return;
        }
#endif

        for (ulong i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (port == index++)
            {
                fPortControls[i] = dataLocation;
                return;
            }
        }
    }

    // -------------------------------------------------------------------

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
    void ladspa_run(const ulong sampleCount)
    {
        dssi_run_synth(sampleCount, nullptr, 0);
    }

    void dssi_run_synth(const ulong sampleCount, snd_seq_event_t* const events, const ulong eventCount)
#else
    void ladspa_run(const ulong sampleCount)
#endif
    {
        // pre-roll
        if (sampleCount == 0)
            return updateParameterOutputs();

        // Check for updated parameters
        float curValue;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPortControls[i] == nullptr)
                continue;

            curValue = *fPortControls[i];

            if (fLastControlValues[i] != curValue && ! fPlugin.isParameterOutput(i))
            {
                fLastControlValues[i] = curValue;
                fPlugin.setParameterValue(i, curValue);
            }
        }

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        // Get MIDI Events
        uint32_t  midiEventCount = 0;
        MidiEvent midiEvents[eventCount];

        for (uint32_t i=0, j; i < eventCount; ++i)
        {
            const snd_seq_event_t& seqEvent(events[i]);

            // FIXME
            if (seqEvent.data.note.channel > 0xF || seqEvent.data.control.channel > 0xF)
                continue;

            switch (seqEvent.type)
            {
            case SND_SEQ_EVENT_NOTEOFF:
                j = midiEventCount++;
                midiEvents[j].frame   = seqEvent.time.tick;
                midiEvents[j].size    = 3;
                midiEvents[j].data[0] = 0x80 + seqEvent.data.note.channel;
                midiEvents[j].data[1] = seqEvent.data.note.note;
                midiEvents[j].data[2] = 0;
                midiEvents[j].data[3] = 0;
                break;
            case SND_SEQ_EVENT_NOTEON:
                j = midiEventCount++;
                midiEvents[j].frame   = seqEvent.time.tick;
                midiEvents[j].size    = 3;
                midiEvents[j].data[0] = 0x90 + seqEvent.data.note.channel;
                midiEvents[j].data[1] = seqEvent.data.note.note;
                midiEvents[j].data[2] = seqEvent.data.note.velocity;
                midiEvents[j].data[3] = 0;
                break;
            case SND_SEQ_EVENT_KEYPRESS:
                j = midiEventCount++;
                midiEvents[j].frame   = seqEvent.time.tick;
                midiEvents[j].size    = 3;
                midiEvents[j].data[0] = 0xA0 + seqEvent.data.note.channel;
                midiEvents[j].data[1] = seqEvent.data.note.note;
                midiEvents[j].data[2] = seqEvent.data.note.velocity;
                midiEvents[j].data[3] = 0;
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                j = midiEventCount++;
                midiEvents[j].frame   = seqEvent.time.tick;
                midiEvents[j].size    = 3;
                midiEvents[j].data[0] = 0xB0 + seqEvent.data.control.channel;
                midiEvents[j].data[1] = seqEvent.data.control.param;
                midiEvents[j].data[2] = seqEvent.data.control.value;
                midiEvents[j].data[3] = 0;
                break;
            case SND_SEQ_EVENT_CHANPRESS:
                j = midiEventCount++;
                midiEvents[j].frame   = seqEvent.time.tick;
                midiEvents[j].size    = 2;
                midiEvents[j].data[0] = 0xD0 + seqEvent.data.control.channel;
                midiEvents[j].data[1] = seqEvent.data.control.value;
                midiEvents[j].data[2] = 0;
                midiEvents[j].data[3] = 0;
                break;
#if 0 // TODO
            case SND_SEQ_EVENT_PITCHBEND:
                j = midiEventCount++;
                midiEvents[j].frame   = seqEvent.time.tick;
                midiEvents[j].size    = 3;
                midiEvents[j].data[0] = 0xE0 + seqEvent.data.control.channel;
                midiEvents[j].data[1] = 0;
                midiEvents[j].data[2] = 0;
                midiEvents[j].data[3] = 0;
                break;
#endif
            }
        }

        fPlugin.run(fPortAudioIns, fPortAudioOuts, sampleCount, midiEvents, midiEventCount);
#else
        fPlugin.run(fPortAudioIns, fPortAudioOuts, sampleCount);
#endif

        updateParameterOutputs();

#if defined(DISTRHO_PLUGIN_TARGET_DSSI) && ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
        return; // unused
        (void)events; (void)eventCount;
#endif
    }

    // -------------------------------------------------------------------

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
# if DISTRHO_PLUGIN_WANT_STATE
    char* dssi_configure(const char* const key, const char* const value)
    {
        if (std::strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX, std::strlen(DSSI_RESERVED_CONFIGURE_PREFIX) == 0))
            return nullptr;
        if (std::strncmp(key, DSSI_GLOBAL_CONFIGURE_PREFIX, std::strlen(DSSI_GLOBAL_CONFIGURE_PREFIX) == 0))
            return nullptr;

        fPlugin.setState(key, value);
        return nullptr;
    }
# endif

# if DISTRHO_PLUGIN_WANT_PROGRAMS
    const DSSI_Program_Descriptor* dssi_get_program(const ulong index)
    {
        if (index >= fPlugin.getProgramCount())
            return nullptr;

        static DSSI_Program_Descriptor desc;

        desc.Bank    = index / 128;
        desc.Program = index % 128;
        desc.Name    = fPlugin.getProgramName(index);

        return &desc;
    }

    void dssi_select_program(const ulong bank, const ulong program)
    {
        const ulong realProgram(bank * 128 + program);

        DISTRHO_SAFE_ASSERT_RETURN(realProgram < fPlugin.getProgramCount(),);

        fPlugin.loadProgram(realProgram);

        // Update control inputs
        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPlugin.isParameterOutput(i))
                continue;

            fLastControlValues[i] = fPlugin.getParameterValue(i);

            if (fPortControls[i] != nullptr)
                *fPortControls[i] = fLastControlValues[i];
        }
    }
# endif
#endif

    // -------------------------------------------------------------------

private:
    PluginExporter fPlugin;

    // LADSPA ports
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    const LADSPA_Data*  fPortAudioIns[DISTRHO_PLUGIN_NUM_INPUTS];
#else
    const LADSPA_Data** fPortAudioIns;
#endif
#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    LADSPA_Data*  fPortAudioOuts[DISTRHO_PLUGIN_NUM_OUTPUTS];
#else
    LADSPA_Data** fPortAudioOuts;
#endif
    LADSPA_Data** fPortControls;
#if DISTRHO_PLUGIN_WANT_LATENCY
    LADSPA_Data*  fPortLatency;
#endif

    // Temporary data
    LADSPA_Data* fLastControlValues;

    // -------------------------------------------------------------------

    void updateParameterOutputs()
    {
        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (! fPlugin.isParameterOutput(i))
                continue;

            fLastControlValues[i] = fPlugin.getParameterValue(i);

            if (fPortControls[i] != nullptr)
                *fPortControls[i] = fLastControlValues[i];
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        if (fPortLatency != nullptr)
            *fPortLatency = fPlugin.getLatency();
#endif
    }
};

// -----------------------------------------------------------------------

static LADSPA_Handle ladspa_instantiate(const LADSPA_Descriptor*, ulong sampleRate)
{
    if (d_lastBufferSize == 0)
        d_lastBufferSize = 2048;
    d_lastSampleRate = sampleRate;

    return new PluginLadspaDssi();
}

#define instancePtr ((PluginLadspaDssi*)instance)

static void ladspa_connect_port(LADSPA_Handle instance, ulong port, LADSPA_Data* dataLocation)
{
    instancePtr->ladspa_connect_port(port, dataLocation);
}

static void ladspa_activate(LADSPA_Handle instance)
{
    instancePtr->ladspa_activate();
}

static void ladspa_run(LADSPA_Handle instance, ulong sampleCount)
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
static const DSSI_Program_Descriptor* dssi_get_program(LADSPA_Handle instance, ulong index)
{
    return instancePtr->dssi_get_program(index);
}

static void dssi_select_program(LADSPA_Handle instance, ulong bank, ulong program)
{
    instancePtr->dssi_select_program(bank, program);
}
# endif

# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
static void dssi_run_synth(LADSPA_Handle instance, ulong sampleCount, snd_seq_event_t* events, ulong eventCount)
{
    instancePtr->dssi_run_synth(sampleCount, events, eventCount);
}
# endif
#endif

#undef instancePtr

// -----------------------------------------------------------------------

static LADSPA_Descriptor sLadspaDescriptor = {
    /* UniqueID   */ 0,
    /* Label      */ nullptr,
#if DISTRHO_PLUGIN_IS_RT_SAFE
    /* Properties */ LADSPA_PROPERTY_HARD_RT_CAPABLE,
#else
    /* Properties */ 0x0,
#endif
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
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
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

// -----------------------------------------------------------------------

class DescriptorInitializer
{
public:
    DescriptorInitializer()
    {
        // Create dummy plugin to get data from
        d_lastBufferSize = 512;
        d_lastSampleRate = 44100.0;
        PluginExporter plugin;
        d_lastBufferSize = 0;
        d_lastSampleRate = 0.0;

        // Get port count, init
        ulong port = 0;
        ulong portCount = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS + plugin.getParameterCount();
#if DISTRHO_PLUGIN_WANT_LATENCY
        portCount += 1;
#endif
        const char** const     portNames       = new const char*[portCount];
        LADSPA_PortDescriptor* portDescriptors = new LADSPA_PortDescriptor[portCount];
        LADSPA_PortRangeHint*  portRangeHints  = new LADSPA_PortRangeHint [portCount];

        // Set ports
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (ulong i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i, ++port)
        {
            const AudioPort& aport(plugin.getAudioPort(true, i));

            portNames[port]       = strdup(aport.name);
            portDescriptors[port] = LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT;

            portRangeHints[port].HintDescriptor = 0x0;
            portRangeHints[port].LowerBound = 0.0f;
            portRangeHints[port].UpperBound = 1.0f;
        }
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (ulong i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i, ++port)
        {
            const AudioPort& aport(plugin.getAudioPort(false, i));

            portNames[port]       = strdup(aport.name);
            portDescriptors[port] = LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT;

            portRangeHints[port].HintDescriptor = 0x0;
            portRangeHints[port].LowerBound = 0.0f;
            portRangeHints[port].UpperBound = 1.0f;
        }
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
        // Set latency port
        portNames[port]       = strdup("_latency");
        portDescriptors[port] = LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT;
        portRangeHints[port].HintDescriptor = LADSPA_HINT_SAMPLE_RATE|LADSPA_HINT_INTEGER;
        portRangeHints[port].LowerBound     = 0.0f;
        portRangeHints[port].UpperBound     = 1.0f;
        ++port;
#endif

        for (ulong i=0, count=plugin.getParameterCount(); i < count; ++i, ++port)
        {
            portNames[port]       = strdup((const char*)plugin.getParameterName(i));
            portDescriptors[port] = LADSPA_PORT_CONTROL;

            if (plugin.isParameterOutput(i))
                portDescriptors[port] |= LADSPA_PORT_OUTPUT;
            else
                portDescriptors[port] |= LADSPA_PORT_INPUT;

            {
                const ParameterRanges& ranges(plugin.getParameterRanges(i));
                const float defValue(ranges.def);

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
                    const float middleValue =  ranges.min/2.0f + ranges.max/2.0f;
                    const float middleLow   = (ranges.min/2.0f + middleValue/2.0f)/2.0f + middleValue/2.0f;
                    const float middleHigh  = (ranges.max/2.0f + middleValue/2.0f)/2.0f + middleValue/2.0f;

                    if (defValue < middleLow)
                        portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_LOW;
                    else if (defValue > middleHigh)
                        portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_HIGH;
                    else
                        portRangeHints[port].HintDescriptor |= LADSPA_HINT_DEFAULT_MIDDLE;
                }
            }

            {
                const uint32_t hints(plugin.getParameterHints(i));

                if (hints & kParameterIsBoolean)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_TOGGLED;
                if (hints & kParameterIsInteger)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_INTEGER;
                if (hints & kParameterIsLogarithmic)
                    portRangeHints[port].HintDescriptor |= LADSPA_HINT_LOGARITHMIC;
            }
        }

        // Set data
        sLadspaDescriptor.UniqueID  = plugin.getUniqueId();
        sLadspaDescriptor.Label     = strdup(plugin.getLabel());
        sLadspaDescriptor.Name      = strdup(plugin.getName());
        sLadspaDescriptor.Maker     = strdup(plugin.getMaker());
        sLadspaDescriptor.Copyright = strdup(plugin.getLicense());
        sLadspaDescriptor.PortCount = portCount;
        sLadspaDescriptor.PortNames = portNames;
        sLadspaDescriptor.PortDescriptors = portDescriptors;
        sLadspaDescriptor.PortRangeHints  = portRangeHints;
    }

    ~DescriptorInitializer()
    {
        if (sLadspaDescriptor.Label != nullptr)
        {
            std::free((void*)sLadspaDescriptor.Label);
            sLadspaDescriptor.Label = nullptr;
        }

        if (sLadspaDescriptor.Name != nullptr)
        {
            std::free((void*)sLadspaDescriptor.Name);
            sLadspaDescriptor.Name = nullptr;
        }

        if (sLadspaDescriptor.Maker != nullptr)
        {
            std::free((void*)sLadspaDescriptor.Maker);
            sLadspaDescriptor.Maker = nullptr;
        }

        if (sLadspaDescriptor.Copyright != nullptr)
        {
            std::free((void*)sLadspaDescriptor.Copyright);
            sLadspaDescriptor.Copyright = nullptr;
        }

        if (sLadspaDescriptor.PortDescriptors != nullptr)
        {
            delete[] sLadspaDescriptor.PortDescriptors;
            sLadspaDescriptor.PortDescriptors = nullptr;
        }

        if (sLadspaDescriptor.PortRangeHints != nullptr)
        {
            delete[] sLadspaDescriptor.PortRangeHints;
            sLadspaDescriptor.PortRangeHints = nullptr;
        }

        if (sLadspaDescriptor.PortNames != nullptr)
        {
            for (ulong i=0; i < sLadspaDescriptor.PortCount; ++i)
            {
                if (sLadspaDescriptor.PortNames[i] != nullptr)
                    std::free((void*)sLadspaDescriptor.PortNames[i]);
            }

            delete[] sLadspaDescriptor.PortNames;
            sLadspaDescriptor.PortNames = nullptr;
        }
    }
};

static DescriptorInitializer sDescInit;

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
const LADSPA_Descriptor* ladspa_descriptor(ulong index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sLadspaDescriptor : nullptr;
}

#ifdef DISTRHO_PLUGIN_TARGET_DSSI
DISTRHO_PLUGIN_EXPORT
const DSSI_Descriptor* dssi_descriptor(ulong index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sDssiDescriptor : nullptr;
}
#endif

// -----------------------------------------------------------------------
