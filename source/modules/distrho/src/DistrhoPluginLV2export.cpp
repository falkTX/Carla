/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "lv2/atom.h"
#include "lv2/buf-size.h"
#include "lv2/data-access.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/port-props.h"
#include "lv2/resize-port.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/ui.h"
#include "lv2/units.h"
#include "lv2/urid.h"
#include "lv2/worker.h"
#include "lv2/lv2_kxstudio_properties.h"
#include "lv2/lv2_programs.h"

#include <fstream>
#include <iostream>

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

#ifndef DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE
# define DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE 2048
#endif

#define DISTRHO_LV2_USE_EVENTS_IN  (DISTRHO_PLUGIN_HAS_MIDI_INPUT || DISTRHO_PLUGIN_WANT_TIMEPOS || (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI))
#define DISTRHO_LV2_USE_EVENTS_OUT (DISTRHO_PLUGIN_HAS_MIDI_OUTPUT || (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI))

// -----------------------------------------------------------------------

DISTRHO_PLUGIN_EXPORT
void lv2_generate_ttl(const char* const basename)
{
    USE_NAMESPACE_DISTRHO

    // Dummy plugin to get data from
    d_lastBufferSize = 512;
    d_lastSampleRate = 44100.0;
    PluginExporter plugin;
    d_lastBufferSize = 0;
    d_lastSampleRate = 0.0;

    d_string pluginDLL(basename);
    d_string pluginTTL(pluginDLL + ".ttl");

    // ---------------------------------------------

    {
        std::cout << "Writing manifest.ttl..."; std::cout.flush();
        std::fstream manifestFile("manifest.ttl", std::ios::out);

        d_string manifestString;
        manifestString += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
        manifestString += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
#if DISTRHO_PLUGIN_HAS_UI
        manifestString += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
#endif
        manifestString += "\n";

        manifestString += "<" DISTRHO_PLUGIN_URI ">\n";
        manifestString += "    a lv2:Plugin ;\n";
        manifestString += "    lv2:binary <" + pluginDLL + "." DISTRHO_DLL_EXTENSION "> ;\n";
        manifestString += "    rdfs:seeAlso <" + pluginTTL + "> .\n";
        manifestString += "\n";

#if DISTRHO_PLUGIN_HAS_UI
        manifestString += "<" DISTRHO_UI_URI ">\n";
# if DISTRHO_OS_HAIKU
        manifestString += "    a ui:BeUI ;\n";
# elif DISTRHO_OS_MAC
        manifestString += "    a ui:CocoaUI ;\n";
# elif DISTRHO_OS_WINDOWS
        manifestString += "    a ui:WindowsUI ;\n";
# else
        manifestString += "    a ui:X11UI ;\n";
# endif
# if ! DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        d_string pluginUI(pluginDLL);
        pluginUI.truncate(pluginDLL.rfind("_dsp"));
        pluginUI += "_ui";

        manifestString += "    ui:binary <" + pluginUI + "." DISTRHO_DLL_EXTENSION "> ;\n";
# else
        manifestString += "    ui:binary <" + pluginDLL + "." DISTRHO_DLL_EXTENSION "> ;\n";
#endif
        manifestString += "    lv2:extensionData ui:idleInterface ,\n";
# if DISTRHO_PLUGIN_WANT_PROGRAMS
        manifestString += "                      ui:showInterface ,\n";
        manifestString += "                      <" LV2_PROGRAMS__Interface "> ;\n";
# else
        manifestString += "                      ui:showInterface ;\n";
# endif
        manifestString += "    lv2:optionalFeature ui:noUserResize ,\n";
        manifestString += "                        ui:resize ,\n";
        manifestString += "                        ui:touch ;\n";
# if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        manifestString += "    lv2:requiredFeature <" LV2_DATA_ACCESS_URI "> ,\n";
        manifestString += "                        <" LV2_INSTANCE_ACCESS_URI "> ,\n";
        manifestString += "                        <" LV2_OPTIONS__options "> ,\n";
# else
        manifestString += "    lv2:requiredFeature <" LV2_OPTIONS__options "> ,\n";
# endif
        manifestString += "                        <" LV2_URID__map "> .\n";
#endif

        manifestFile << manifestString << std::endl;
        manifestFile.close();
        std::cout << " done!" << std::endl;
    }

    // ---------------------------------------------

    {
        std::cout << "Writing " << pluginTTL << "..."; std::cout.flush();
        std::fstream pluginFile(pluginTTL, std::ios::out);

        d_string pluginString;

        // header
#if DISTRHO_LV2_USE_EVENTS_IN
        pluginString += "@prefix atom: <" LV2_ATOM_PREFIX "> .\n";
#endif
        pluginString += "@prefix doap: <http://usefulinc.com/ns/doap#> .\n";
        pluginString += "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n";
        pluginString += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
        pluginString += "@prefix rsz:  <" LV2_RESIZE_PORT_PREFIX "> .\n";
#if DISTRHO_PLUGIN_HAS_UI
        pluginString += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
#endif
        pluginString += "@prefix unit: <" LV2_UNITS_PREFIX "> .\n";
        pluginString += "\n";

        // plugin
        pluginString += "<" DISTRHO_PLUGIN_URI ">\n";
#if DISTRHO_PLUGIN_IS_SYNTH
        pluginString += "    a lv2:InstrumentPlugin, lv2:Plugin ;\n";
#else
        pluginString += "    a lv2:Plugin ;\n";
#endif
        pluginString += "\n";

        // extensionData
        pluginString += "    lv2:extensionData <" LV2_STATE__interface "> ";
#if DISTRHO_PLUGIN_WANT_STATE
        pluginString += ",\n                      <" LV2_OPTIONS__interface "> ";
        pluginString += ",\n                      <" LV2_WORKER__interface "> ";
#endif
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        pluginString += ",\n                      <" LV2_PROGRAMS__Interface "> ";
#endif
        pluginString += ";\n\n";

        // optionalFeatures
#if DISTRHO_PLUGIN_IS_RT_SAFE
        pluginString += "    lv2:optionalFeature <" LV2_CORE__hardRTCapable "> ,\n";
        pluginString += "                        <" LV2_BUF_SIZE__boundedBlockLength "> ;\n";
#else
        pluginString += "    lv2:optionalFeature <" LV2_BUF_SIZE__boundedBlockLength "> ;\n";
#endif
        pluginString += "\n";

        // requiredFeatures
        pluginString += "    lv2:requiredFeature <" LV2_OPTIONS__options "> ";
        pluginString += ",\n                        <" LV2_URID__map "> ";
#if DISTRHO_PLUGIN_WANT_STATE
        pluginString += ",\n                        <" LV2_WORKER__schedule "> ";
#endif
        pluginString += ";\n\n";

        // UI
#if DISTRHO_PLUGIN_HAS_UI
        pluginString += "    ui:ui <" DISTRHO_UI_URI "> ;\n";
        pluginString += "\n";
#endif

        {
            uint32_t portIndex = 0;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i, ++portIndex)
            {
                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                pluginString += "        a lv2:InputPort, lv2:AudioPort ;\n";
                pluginString += "        lv2:index " + d_string(portIndex) + " ;\n";
                pluginString += "        lv2:symbol \"lv2_audio_in_" + d_string(i+1) + "\" ;\n";
                pluginString += "        lv2:name \"Audio Input " + d_string(i+1) + "\" ;\n";

                if (i+1 == DISTRHO_PLUGIN_NUM_INPUTS)
                    pluginString += "    ] ;\n\n";
                else
                    pluginString += "    ] ,\n";
            }
            pluginString += "\n";
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i, ++portIndex)
            {
                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                pluginString += "        a lv2:OutputPort, lv2:AudioPort ;\n";
                pluginString += "        lv2:index " + d_string(portIndex) + " ;\n";
                pluginString += "        lv2:symbol \"lv2_audio_out_" + d_string(i+1) + "\" ;\n";
                pluginString += "        lv2:name \"Audio Output " + d_string(i+1) + "\" ;\n";

                if (i+1 == DISTRHO_PLUGIN_NUM_OUTPUTS)
                    pluginString += "    ] ;\n\n";
                else
                    pluginString += "    ] ,\n";
            }
            pluginString += "\n";
#endif

#if DISTRHO_LV2_USE_EVENTS_IN
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:InputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Events Input\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_in\" ;\n";
            pluginString += "        rsz:minimumSize " + d_string(DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE) + " ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)
            pluginString += "        atom:supports <" LV2_ATOM__String "> ;\n";
# endif
# if DISTRHO_PLUGIN_HAS_MIDI_INPUT
            pluginString += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_TIMEPOS
            pluginString += "        atom:supports <" LV2_TIME__Position "> ;\n";
# endif
            pluginString += "    ] ;\n\n";
            ++portIndex;
#endif

#if DISTRHO_LV2_USE_EVENTS_OUT
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:OutputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Events Output\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_out\" ;\n";
            pluginString += "        rsz:minimumSize " + d_string(DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE) + " ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)
            pluginString += "        atom:supports <" LV2_ATOM__String "> ;\n";
# endif
# if DISTRHO_PLUGIN_HAS_MIDI_OUTPUT
            pluginString += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
# endif
            pluginString += "    ] ;\n\n";
            ++portIndex;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Latency\" ;\n";
            pluginString += "        lv2:symbol \"lv2_latency\" ;\n";
            pluginString += "        lv2:designation lv2:latency ;\n";
            pluginString += "        lv2:portProperty lv2:reportsLatency, lv2:integer ;\n";
            pluginString += "    ] ;\n\n";
            ++portIndex;
#endif

            for (uint32_t i=0, count=plugin.getParameterCount(); i < count; ++i, ++portIndex)
            {
                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                if (plugin.isParameterOutput(i))
                    pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
                else
                    pluginString += "        a lv2:InputPort, lv2:ControlPort ;\n";

                pluginString += "        lv2:index " + d_string(portIndex) + " ;\n";
                pluginString += "        lv2:name \"" + plugin.getParameterName(i) + "\" ;\n";

                // symbol
                {
                    d_string symbol(plugin.getParameterSymbol(i));

                    if (symbol.isEmpty())
                        symbol = "lv2_port_" + d_string(portIndex-1);

                    pluginString += "        lv2:symbol \"" + symbol + "\" ;\n";
                }

                // ranges
                {
                    const ParameterRanges& ranges(plugin.getParameterRanges(i));

                    if (plugin.getParameterHints(i) & kParameterIsInteger)
                    {
                        pluginString += "        lv2:default " + d_string(int(plugin.getParameterValue(i))) + " ;\n";
                        pluginString += "        lv2:minimum " + d_string(int(ranges.min)) + " ;\n";
                        pluginString += "        lv2:maximum " + d_string(int(ranges.max)) + " ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:default " + d_string(plugin.getParameterValue(i)) + " ;\n";
                        pluginString += "        lv2:minimum " + d_string(ranges.min) + " ;\n";
                        pluginString += "        lv2:maximum " + d_string(ranges.max) + " ;\n";
                    }
                }

                // unit
                {
                    const d_string& unit(plugin.getParameterUnit(i));

                    if (! unit.isEmpty())
                    {
                        if (unit == "db" || unit == "dB")
                        {
                            pluginString += "        unit:unit unit:db ;\n";
                        }
                        else if (unit == "hz" || unit == "Hz")
                        {
                            pluginString += "        unit:unit unit:hz ;\n";
                        }
                        else if (unit == "khz" || unit == "kHz")
                        {
                            pluginString += "        unit:unit unit:khz ;\n";
                        }
                        else if (unit == "mhz" || unit == "mHz")
                        {
                            pluginString += "        unit:unit unit:mhz ;\n";
                        }
                        else if (unit == "%")
                        {
                            pluginString += "        unit:unit unit:pc ;\n";
                        }
                        else
                        {
                            pluginString += "        unit:unit [\n";
                            pluginString += "            a unit:Unit ;\n";
                            pluginString += "            unit:name   \"" + unit + "\" ;\n";
                            pluginString += "            unit:symbol \"" + unit + "\" ;\n";
                            pluginString += "            unit:render \"%f " + unit + "\" ;\n";
                            pluginString += "        ] ;\n";
                        }
                    }
                }

                // hints
                {
                    const uint32_t hints(plugin.getParameterHints(i));

                    if (hints & kParameterIsBoolean)
                        pluginString += "        lv2:portProperty lv2:toggled ;\n";
                    if (hints & kParameterIsInteger)
                        pluginString += "        lv2:portProperty lv2:integer ;\n";
                    if (hints & kParameterIsLogarithmic)
                        pluginString += "        lv2:portProperty <" LV2_PORT_PROPS__logarithmic "> ;\n";
                    if ((hints & kParameterIsAutomable) == 0 && ! plugin.isParameterOutput(i))
                    {
                        pluginString += "        lv2:portProperty <" LV2_PORT_PROPS__expensive "> ,\n";
                        pluginString += "                         <" LV2_KXSTUDIO_PROPERTIES__NonAutomable "> ;\n";
                    }
                }

                if (i+1 == count)
                    pluginString += "    ] ;\n\n";
                else
                    pluginString += "    ] ,\n";
            }
        }

        pluginString += "    doap:name \"" + d_string(plugin.getName()) + "\" ;\n";
        pluginString += "    doap:maintainer [ foaf:name \"" + d_string(plugin.getMaker()) + "\" ] .\n";

        pluginFile << pluginString << std::endl;
        pluginFile.close();
        std::cout << " done!" << std::endl;
    }
}
