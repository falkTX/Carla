/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#ifdef DISTRHO_PLUGIN_TARGET_LV2

#include "DistrhoPluginInternal.h"

#include "lv2-sdk/lv2.h"
#include "lv2-sdk/atom.h"
#include "lv2-sdk/midi.h"
#include "lv2-sdk/patch.h"
#include "lv2-sdk/programs.h"
#include "lv2-sdk/state.h"
#include "lv2-sdk/urid.h"
#include "lv2-sdk/ui.h"
#include "lv2-sdk/units.h"
#include "lv2-sdk/worker.h"

#include <fstream>
#include <iostream>

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

#define DISTRHO_LV2_USE_EVENTS (DISTRHO_PLUGIN_IS_SYNTH || DISTRHO_PLUGIN_WANT_STATE)

// -------------------------------------------------

START_NAMESPACE_DISTRHO

void lv2_generate_ttl_func()
{
    PluginInternal plugin;

    d_string pluginLabel = plugin.label();
    d_string pluginTTL   = pluginLabel + ".ttl";

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
        manifestString += "    lv2:binary <" + pluginLabel + "." DISTRHO_DLL_EXT "> ;\n";
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
        manifestString += "    ui:binary <" + pluginLabel + "_ui." DISTRHO_DLL_EXT "> ;\n";
# if DISTRHO_LV2_USE_EVENTS
        manifestString += "    lv2:optionalFeature <" LV2_URID__map "> ,\n";
        manifestString += "                        ui:noUserResize .\n";
# else
        manifestString += "    lv2:optionalFeature ui:noUserResize .\n";
# endif
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
#if DISTRHO_LV2_USE_EVENTS
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

#if (DISTRHO_PLUGIN_IS_SYNTH && DISTRHO_PLUGIN_WANT_STATE)
        pluginString += "    lv2:optionalFeature <" LV2_CORE__hardRTCapable "> ,\n";
        pluginString += "                        <" LV2_WORKER__schedule "> ;\n";
        pluginString += "    lv2:requiredFeature <" LV2_URID__map "> ;\n";
#elif DISTRHO_PLUGIN_IS_SYNTH
        pluginString += "    lv2:optionalFeature <" LV2_CORE__hardRTCapable "> ;\n";
        pluginString += "    lv2:requiredFeature <" LV2_URID__map "> ;\n";
#elif DISTRHO_PLUGIN_WANT_STATE
        pluginString += "    lv2:optionalFeature <" LV2_CORE__hardRTCapable "> ,\n";
        pluginString += "                        <" LV2_URID__map "> ,\n";
        pluginString += "                        <" LV2_WORKER__schedule "> ;\n";
#endif

#if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_WANT_PROGRAMS)
        pluginString += "    lv2:extensionData <" LV2_STATE__interface "> ,\n";
        pluginString += "                      <" LV2_WORKER__interface "> ,\n";
        pluginString += "                      <" LV2_PROGRAMS__Interface "> ;\n";
#elif DISTRHO_PLUGIN_WANT_STATE
        pluginString += "    lv2:extensionData <" LV2_STATE__interface "> ,\n";
        pluginString += "                      <" LV2_WORKER__interface "> ;\n";
#elif DISTRHO_PLUGIN_WANT_PROGRAMS
        pluginString += "    lv2:extensionData <" LV2_PROGRAMS__Interface "> ;\n";
#endif
        pluginString += "\n";

#if DISTRHO_PLUGIN_HAS_UI
        pluginString += "    ui:ui <" DISTRHO_UI_URI "> ;\n";
        pluginString += "\n";
#endif

        {
            uint32_t i, portIndex = 0;

            for (i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; i++)
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

            for (i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; i++)
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

            for (i=0; i < plugin.parameterCount(); i++)
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
                    const ParameterRanges* ranges = plugin.parameterRanges(i);

                    if (plugin.parameterHints(i) & PARAMETER_IS_INTEGER)
                    {
                        pluginString += "        lv2:default " + d_string(int(plugin.parameterValue(i))) + " ;\n";
                        pluginString += "        lv2:minimum " + d_string(int(ranges->min)) + " ;\n";
                        pluginString += "        lv2:maximum " + d_string(int(ranges->max)) + " ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:default " + d_string(plugin.parameterValue(i)) + " ;\n";
                        pluginString += "        lv2:minimum " + d_string(ranges->min) + " ;\n";
                        pluginString += "        lv2:maximum " + d_string(ranges->max) + " ;\n";
                    }
                }

                // unit
                {
                    const d_string& unit = plugin.parameterUnit(i);

                    if (! unit.isEmpty())
                    {
                        if (unit == "db" || unit == "dB")
                            pluginString += "        unit:unit unit:db ;\n";
                        else if (unit == "hz" || unit == "Hz")
                            pluginString += "        unit:unit unit:hz ;\n";
                        else if (unit == "khz" || unit == "kHz")
                            pluginString += "        unit:unit unit:khz ;\n";
                        else if (unit == "mhz" || unit == "mHz")
                            pluginString += "        unit:unit unit:mhz ;\n";
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
                    uint32_t hints = plugin.parameterHints(i);

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

            pluginString += "    lv2:port [\n";

#if DISTRHO_LV2_USE_EVENTS
            pluginString += "        a lv2:InputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
            pluginString += "        lv2:name \"Events Input\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_in\" ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_IS_SYNTH && DISTRHO_PLUGIN_WANT_STATE)
            pluginString += "        atom:supports <" LV2_MIDI__MidiEvent "> ,\n";
            pluginString += "                      <" LV2_PATCH__Message "> ;\n";
# elif DISTRHO_PLUGIN_IS_SYNTH
            pluginString += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
# else
            pluginString += "        atom:supports <" LV2_PATCH__Message "> ;\n";
# endif
            pluginString += "    ] ,\n";
            pluginString += "    [\n";
#endif
            pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
            pluginString += "        lv2:name \"Latency\" ;\n";
            pluginString += "        lv2:symbol \"lv2_latency\" ;\n";
            pluginString += "        lv2:designation lv2:latency ;\n";
#if ! DISTRHO_PLUGIN_HAS_UI
            pluginString += "    ] ;\n\n";
#else
            pluginString += "    ] ,\n";
            pluginString += "    [\n";
            pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
            pluginString += "        lv2:index " + d_string(portIndex++) + " ;\n";
            pluginString += "        lv2:name \"Sample Rate\" ;\n";
            pluginString += "        lv2:symbol \"lv2_sample_rate\" ;\n";
            pluginString += "        lv2:designation <http://lv2plug.in/ns/ext/parameters#sampleRate> ;\n";
            pluginString += "    ] ;\n\n";
#endif
        }

        pluginString += "    doap:name \"" + d_string(plugin.name()) + "\" ;\n";
        pluginString += "    doap:maintainer [ foaf:name \"" + d_string(plugin.maker()) + "\" ] .\n";

        pluginFile << pluginString << std::endl;
        pluginFile.close();
        std::cout << " done!" << std::endl;
    }
}

// unused stuff
void d_unusedStuff()
{
    (void)d_lastBufferSize;
    (void)d_lastSampleRate;
}

END_NAMESPACE_DISTRHO

// -------------------------------------------------

int main()
{
    USE_NAMESPACE_DISTRHO
    lv2_generate_ttl_func();
}

// -------------------------------------------------

#endif // DISTRHO_PLUGIN_TARGET_LV2
