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

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/buf-size.h"
// #include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/programs.h"
// #include "lv2/state.h"
#include "lv2/time.h"
// #include "lv2/ui.h"
#include "lv2/urid.h"
#include "lv2/units.h"
// #include "lv2/worker.h"

#include <fstream>
#include <iostream>

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

// TODO: UI
#undef DISTRHO_PLUGIN_HAS_UI

#undef DISTRHO_PLUGIN_WANT_LATENCY
#undef DISTRHO_PLUGIN_WANT_PROGRAMS
#undef DISTRHO_PLUGIN_WANT_STATE
#undef DISTRHO_PLUGIN_WANT_TIMEPOS
#define DISTRHO_PLUGIN_WANT_LATENCY  1
#define DISTRHO_PLUGIN_WANT_PROGRAMS 1
#define DISTRHO_PLUGIN_WANT_STATE    1
#define DISTRHO_PLUGIN_WANT_TIMEPOS  1

#define DISTRHO_LV2_USE_EVENTS_IN  (DISTRHO_PLUGIN_IS_SYNTH || DISTRHO_PLUGIN_WANT_STATE || DISTRHO_PLUGIN_WANT_TIMEPOS)
#define DISTRHO_LV2_USE_EVENTS_OUT (DISTRHO_PLUGIN_WANT_STATE)

// -------------------------------------------------

DISTRHO_PLUGIN_EXPORT
void lv2_generate_ttl(const char* const basename)
{
    USE_NAMESPACE_DISTRHO

    // Dummy plugin to get data from
    d_lastBufferSize = 512;
    d_lastSampleRate = 44100.0;
    PluginInternal plugin;
    d_lastBufferSize = 0;
    d_lastSampleRate = 0.0;

    d_string pluginLabel(basename);
    d_string pluginTTL(pluginLabel + ".ttl");

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
        manifestString += "    lv2:binary <" + pluginLabel + "." DISTRHO_DLL_EXTENSION "> ;\n";
        manifestString += "    rdfs:seeAlso <" + pluginTTL + "> .\n";
        manifestString += "\n";

#if DISTRHO_PLUGIN_HAS_UI
        manifestString += "<" DISTRHO_UI_URI ">\n";
# if DISTRHO_OS_HAIKU
        manifestString += "    a ui:BeUI ;\n";
# elif DISTRHO_OS_MACOS
        manifestString += "    a ui:CocoaUI ;\n";
# elif DISTRHO_OS_WINDOWS
        manifestString += "    a ui:WindowsUI ;\n";
# else
        manifestString += "    a ui:X11UI ;\n";
# endif
        manifestString += "    ui:binary <" + pluginLabel + "_ui." DISTRHO_DLL_EXTENSION "> .\n";
// # if DISTRHO_LV2_USE_EVENTS
//         manifestString += "    lv2:optionalFeature <" LV2_URID__map "> ,\n";
//         manifestString += "                        ui:noUserResize .\n";
// # else
//         manifestString += "    lv2:optionalFeature ui:noUserResize .\n";
// # endif
        manifestString += "\n";
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
#if DISTRHO_LV2_USE_EVENTS_IN
        pluginString += "@prefix atom: <" LV2_ATOM_PREFIX "> .\n";
#endif
        pluginString += "@prefix doap: <http://usefulinc.com/ns/doap#> .\n";
        pluginString += "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n";
        pluginString += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
#if DISTRHO_PLUGIN_HAS_UI
        pluginString += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
#endif
        pluginString += "@prefix unit: <" LV2_UNITS_PREFIX "> .\n";
        pluginString += "\n";

        pluginString += "<" DISTRHO_PLUGIN_URI ">\n";
#if DISTRHO_PLUGIN_IS_SYNTH
        pluginString += "    a lv2:InstrumentPlugin, lv2:Plugin ;\n";
#else
        pluginString += "    a lv2:Plugin ;\n";
#endif
        pluginString += "\n";

#if (DISTRHO_PLUGIN_WANT_STATE || DISTRHO_PLUGIN_WANT_PROGRAMS)
        pluginString += "    lv2:extensionData <" LV2_OPTIONS__interface "> ,\n";
# if DISTRHO_PLUGIN_WANT_STATE
        pluginString += "                      <" LV2_STATE__interface "> ,\n";
        pluginString += "                      <" LV2_WORKER__interface "> \n";
#  if DISTRHO_PLUGIN_WANT_PROGRAMS
        pluginString += ",\n";
#  else
        pluginString += ";\n";
#  endif
# endif
# if DISTRHO_PLUGIN_WANT_PROGRAMS
        pluginString += "                      <" LV2_PROGRAMS__Interface "> ;\n";
# endif
#endif
        pluginString += "\n";

#if DISTRHO_PLUGIN_WANT_STATE
        pluginString += "    lv2:optionalFeature <" LV2_CORE__hardRTCapable "> ,\n";
        pluginString += "                        <" LV2_WORKER__schedule "> ;\n";
#else
        pluginString += "    lv2:optionalFeature <" LV2_CORE__hardRTCapable "> ;\n";
#endif
        pluginString += "\n";

        pluginString += "    lv2:requiredFeature <" LV2_BUF_SIZE__boundedBlockLength "> ,\n";
        pluginString += "                        <" LV2_URID__map "> ;\n";
        pluginString += "\n";

#if DISTRHO_PLUGIN_HAS_UI
        pluginString += "    ui:ui <" DISTRHO_UI_URI "> ;\n";
        pluginString += "\n";
#endif

        {
            uint32_t portIndex = 0;

            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            {
                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                pluginString += "        a lv2:InputPort, lv2:AudioPort ;\n";
                pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
                pluginString += "        lv2:symbol \"lv2_audio_in_" + d_string(i+1) + "\" ;\n";
                pluginString += "        lv2:name \"Audio Input " + d_string(i+1) + "\" ;\n";

                if (i+1 == DISTRHO_PLUGIN_NUM_INPUTS)
                    pluginString += "    ] ;\n\n";
                else
                    pluginString += "    ] ,\n";
            }

            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            {
                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                pluginString += "        a lv2:OutputPort, lv2:AudioPort ;\n";
                pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
                pluginString += "        lv2:symbol \"lv2_audio_out_" + d_string(i+1) + "\" ;\n";
                pluginString += "        lv2:name \"Audio Output " + d_string(i+1) + "\" ;\n";

                if (i+1 == DISTRHO_PLUGIN_NUM_OUTPUTS)
                    pluginString += "    ] ;\n\n";
                else
                    pluginString += "    ] ,\n";
            }

#if DISTRHO_LV2_USE_EVENTS_IN
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:InputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
            pluginString += "        lv2:name \"Events Input\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_in\" ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_IS_SYNTH && DISTRHO_PLUGIN_WANT_TIMEPOS) // TODO
            pluginString += "        atom:supports <" LV2_MIDI__MidiEvent "> ,\n";
            pluginString += "                      <" LV2_TIME__Position "> ;\n";
# elif DISTRHO_PLUGIN_IS_SYNTH
            pluginString += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
# else
            pluginString += "        atom:supports <" LV2_TIME__Position "> ;\n";
# endif
            pluginString += "    ] ;\n";
#endif

#if DISTRHO_LV2_USE_EVENTS_OUT
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:OutputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
            pluginString += "        lv2:name \"Events Output\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_out\" ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
            pluginString += "        atom:supports <something_here> ;\n"; // TODO
            pluginString += "    ] ;\n";
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
            pluginString += "        lv2:name \"Latency\" ;\n";
            pluginString += "        lv2:symbol \"lv2_latency\" ;\n";
            pluginString += "        lv2:designation lv2:latency ;\n";
            pluginString += "    ] ;\n";
#endif

            for (uint32_t i=0; i < plugin.parameterCount(); ++i)
            {
                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                if (plugin.parameterIsOutput(i))
                    pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
                else
                    pluginString += "        a lv2:InputPort, lv2:ControlPort ;\n";

                pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
                pluginString += "        lv2:name \"" + plugin.parameterName(i) + "\" ;\n";

                // symbol
                {
                    d_string symbol(plugin.parameterSymbol(i));

                    if (symbol.isEmpty())
                        symbol = "lv2_port_" + d_string(portIndex-1);

                    pluginString += "        lv2:symbol \"" + symbol + "\" ;\n";
                }

                // ranges
                {
                    const ParameterRanges& ranges(plugin.parameterRanges(i));

                    if (plugin.parameterHints(i) & PARAMETER_IS_INTEGER)
                    {
                        pluginString += "        lv2:default " + d_string(int(plugin.parameterValue(i))) + " ;\n";
                        pluginString += "        lv2:minimum " + d_string(int(ranges.min)) + " ;\n";
                        pluginString += "        lv2:maximum " + d_string(int(ranges.max)) + " ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:default " + d_string(plugin.parameterValue(i)) + " ;\n";
                        pluginString += "        lv2:minimum " + d_string(ranges.min) + " ;\n";
                        pluginString += "        lv2:maximum " + d_string(ranges.max) + " ;\n";
                    }
                }

                // unit
                {
                    const d_string& unit(plugin.parameterUnit(i));

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
                        else
                        {
                            pluginString += "        unit:unit [\n";
                            pluginString += "            a unit:Unit ;\n";
                            pluginString += "            unit:name   \"" + unit + "\" ;\n";
                            pluginString += "            unit:symbol \"" + unit + "\" ;\n";
                            pluginString += "            unit:render \"%f f\" ;\n";
                            pluginString += "        ] ;\n";
                        }
                    }
                }

                // hints
                {
                    const uint32_t hints(plugin.parameterHints(i));

                    if (hints & PARAMETER_IS_BOOLEAN)
                        pluginString += "        lv2:portProperty lv2:toggled ;\n";
                    if (hints & PARAMETER_IS_INTEGER)
                        pluginString += "        lv2:portProperty lv2:integer ;\n";
                    if (hints & PARAMETER_IS_LOGARITHMIC)
                        pluginString += "        lv2:portProperty <http://lv2plug.in/ns/ext/port-props#logarithmic> ;\n";
                }

                if (i+1 == plugin.parameterCount())
                    pluginString += "    ] ;\n\n";
                else
                    pluginString += "    ] ,\n";
            }
        }

        pluginString += "    doap:name \"" + d_string(plugin.name()) + "\" ;\n";
        pluginString += "    doap:maintainer [ foaf:name \"" + d_string(plugin.maker()) + "\" ] .\n";

        pluginFile << pluginString << std::endl;
        pluginFile.close();
        std::cout << " done!" << std::endl;
    }
}
