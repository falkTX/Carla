/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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
#include "lv2/presets.h"
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

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL)
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI
# if DISTRHO_OS_HAIKU
#  define DISTRHO_LV2_UI_TYPE "BeUI"
# elif DISTRHO_OS_MAC
#  define DISTRHO_LV2_UI_TYPE "CocoaUI"
# elif DISTRHO_OS_WINDOWS
#  define DISTRHO_LV2_UI_TYPE "WindowsUI"
# else
#  define DISTRHO_LV2_UI_TYPE "X11UI"
# endif
#endif

#define DISTRHO_LV2_USE_EVENTS_IN  (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_TIMEPOS || (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI))
#define DISTRHO_LV2_USE_EVENTS_OUT (DISTRHO_PLUGIN_WANT_MIDI_OUTPUT || (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI))

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

    String pluginDLL(basename);
    String pluginTTL(pluginDLL + ".ttl");

#if DISTRHO_PLUGIN_HAS_UI
    String pluginUI(pluginDLL);
# if ! DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    pluginUI.truncate(pluginDLL.rfind("_dsp"));
    pluginUI += "_ui";
    const String uiTTL(pluginUI + ".ttl");
# endif
#endif

    // ---------------------------------------------

    {
        std::cout << "Writing manifest.ttl..."; std::cout.flush();
        std::fstream manifestFile("manifest.ttl", std::ios::out);

        String manifestString;
        manifestString += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
        manifestString += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        manifestString += "@prefix pset: <" LV2_PRESETS_PREFIX "> .\n";
#endif
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
        manifestString += "    a ui:" DISTRHO_LV2_UI_TYPE " ;\n";
        manifestString += "    ui:binary <" + pluginUI + "." DISTRHO_DLL_EXTENSION "> ;\n";
# if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        manifestString += "\n";
        manifestString += "    lv2:extensionData ui:idleInterface ,\n";
#  if DISTRHO_PLUGIN_WANT_PROGRAMS
        manifestString += "                      ui:showInterface ,\n";
        manifestString += "                      <" LV2_PROGRAMS__Interface "> ;\n";
#  else
        manifestString += "                      ui:showInterface ;\n";
#  endif
        manifestString += "\n";
        manifestString += "    lv2:optionalFeature ui:noUserResize ,\n";
        manifestString += "                        ui:resize ,\n";
        manifestString += "                        ui:touch ;\n";
        manifestString += "\n";
        manifestString += "    lv2:requiredFeature <" LV2_DATA_ACCESS_URI "> ,\n";
        manifestString += "                        <" LV2_INSTANCE_ACCESS_URI "> ,\n";
        manifestString += "                        <" LV2_OPTIONS__options "> ,\n";
        manifestString += "                        <" LV2_URID__map "> .\n";
# else
        manifestString += "    rdfs:seeAlso <" + uiTTL + "> .\n";
# endif
        manifestString += "\n";
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        const String presetSeparator(std::strstr(DISTRHO_PLUGIN_URI, "#") != nullptr ? ":" : "#");

        char strBuf[0xff+1];
        strBuf[0xff] = '\0';

        // Presets
        for (uint32_t i = 0; i < plugin.getProgramCount(); ++i)
        {
            std::snprintf(strBuf, 0xff, "%03i", i+1);

            manifestString += "<" DISTRHO_PLUGIN_URI + presetSeparator + "preset" + strBuf + ">\n";
            manifestString += "    a pset:Preset ;\n";
            manifestString += "    lv2:appliesTo <" DISTRHO_PLUGIN_URI "> ;\n";
            manifestString += "    rdfs:seeAlso <presets.ttl> .\n";
            manifestString += "\n";
        }
#endif

        manifestFile << manifestString << std::endl;
        manifestFile.close();
        std::cout << " done!" << std::endl;
    }

    // ---------------------------------------------

    {
        std::cout << "Writing " << pluginTTL << "..."; std::cout.flush();
        std::fstream pluginFile(pluginTTL, std::ios::out);

        String pluginString;

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
#ifdef DISTRHO_PLUGIN_LV2_CATEGORY
        pluginString += "    a " DISTRHO_PLUGIN_LV2_CATEGORY ", lv2:Plugin ;\n";
#elif DISTRHO_PLUGIN_IS_SYNTH
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
                const AudioPort& port(plugin.getAudioPort(true, i));

                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                if (port.hints & kAudioPortIsCV)
                    pluginString += "        a lv2:InputPort, lv2:CVPort ;\n";
                else
                    pluginString += "        a lv2:InputPort, lv2:AudioPort ;\n";

                pluginString += "        lv2:index " + String(portIndex) + " ;\n";
                pluginString += "        lv2:symbol \"" + port.symbol + "\" ;\n";
                pluginString += "        lv2:name \"" + port.name + "\" ;\n";

                if (port.hints & kAudioPortIsSidechain)
                    pluginString += "        lv2:portProperty lv2:isSideChain;\n";

                if (i+1 == DISTRHO_PLUGIN_NUM_INPUTS)
                    pluginString += "    ] ;\n";
                else
                    pluginString += "    ] ,\n";
            }
            pluginString += "\n";
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i, ++portIndex)
            {
                const AudioPort& port(plugin.getAudioPort(false, i));

                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                if (port.hints & kAudioPortIsCV)
                    pluginString += "        a lv2:OutputPort, lv2:CVPort ;\n";
                else
                    pluginString += "        a lv2:OutputPort, lv2:AudioPort ;\n";

                pluginString += "        lv2:index " + String(portIndex) + " ;\n";
                pluginString += "        lv2:symbol \"" + port.symbol + "\" ;\n";
                pluginString += "        lv2:name \"" + port.name + "\" ;\n";

                if (port.hints & kAudioPortIsSidechain)
                    pluginString += "        lv2:portProperty lv2:isSideChain;\n";

                if (i+1 == DISTRHO_PLUGIN_NUM_OUTPUTS)
                    pluginString += "    ] ;\n";
                else
                    pluginString += "    ] ,\n";
            }
            pluginString += "\n";
#endif

#if DISTRHO_LV2_USE_EVENTS_IN
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:InputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + String(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Events Input\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_in\" ;\n";
            pluginString += "        rsz:minimumSize " + String(DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE) + " ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)
            pluginString += "        atom:supports <" LV2_ATOM__String "> ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
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
            pluginString += "        lv2:index " + String(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Events Output\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_out\" ;\n";
            pluginString += "        rsz:minimumSize " + String(DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE) + " ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)
            pluginString += "        atom:supports <" LV2_ATOM__String "> ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            pluginString += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
# endif
            pluginString += "    ] ;\n\n";
            ++portIndex;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
            pluginString += "        lv2:index " + String(portIndex) + " ;\n";
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

                pluginString += "        lv2:index " + String(portIndex) + " ;\n";
                pluginString += "        lv2:name \"" + plugin.getParameterName(i) + "\" ;\n";

                // symbol
                {
                    String symbol(plugin.getParameterSymbol(i));

                    if (symbol.isEmpty())
                        symbol = "lv2_port_" + String(portIndex-1);

                    pluginString += "        lv2:symbol \"" + symbol + "\" ;\n";
                }

                // ranges
                {
                    const ParameterRanges& ranges(plugin.getParameterRanges(i));

                    if (plugin.getParameterHints(i) & kParameterIsInteger)
                    {
                        pluginString += "        lv2:default " + String(int(plugin.getParameterValue(i))) + " ;\n";
                        pluginString += "        lv2:minimum " + String(int(ranges.min)) + " ;\n";
                        pluginString += "        lv2:maximum " + String(int(ranges.max)) + " ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:default " + String(plugin.getParameterValue(i)) + " ;\n";
                        pluginString += "        lv2:minimum " + String(ranges.min) + " ;\n";
                        pluginString += "        lv2:maximum " + String(ranges.max) + " ;\n";
                    }
                }

                // unit
                {
                    const String& unit(plugin.getParameterUnit(i));

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

        pluginString += "    doap:name \"" + String(plugin.getName()) + "\" ;\n";
        pluginString += "    doap:maintainer [ foaf:name \"" + String(plugin.getMaker()) + "\" ] .\n";

        pluginFile << pluginString << std::endl;
        pluginFile.close();
        std::cout << " done!" << std::endl;
    }

    // ---------------------------------------------

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    {
        std::cout << "Writing " << uiTTL << "..."; std::cout.flush();
        std::fstream uiFile(uiTTL, std::ios::out);

        String uiString;
        uiString += "@prefix lv2: <" LV2_CORE_PREFIX "> .\n";
        uiString += "@prefix ui:  <" LV2_UI_PREFIX "> .\n";
        uiString += "\n";

        uiString += "<" DISTRHO_UI_URI ">\n";
        uiString += "    lv2:extensionData ui:idleInterface ,\n";
#  if DISTRHO_PLUGIN_WANT_PROGRAMS
        uiString += "                      ui:showInterface ,\n";
        uiString += "                      <" LV2_PROGRAMS__Interface "> ;\n";
#  else
        uiString += "                      ui:showInterface ;\n";
#  endif
        uiString += "\n";
        uiString += "    lv2:optionalFeature ui:noUserResize ,\n";
        uiString += "                        ui:resize ,\n";
        uiString += "                        ui:touch ;\n";
        uiString += "\n";
        uiString += "    lv2:requiredFeature <" LV2_OPTIONS__options "> ,\n";
        uiString += "                        <" LV2_URID__map "> .\n";

        uiFile << uiString << std::endl;
        uiFile.close();
        std::cout << " done!" << std::endl;
    }
#endif

    // ---------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    {
        std::cout << "Writing presets.ttl..."; std::cout.flush();
        std::fstream presetsFile("presets.ttl", std::ios::out);

        String presetsString;
        presetsString += "@prefix lv2:   <" LV2_CORE_PREFIX "> .\n";
        presetsString += "@prefix pset:  <" LV2_PRESETS_PREFIX "> .\n";
        presetsString += "@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n";
# if DISTRHO_PLUGIN_WANT_STATE
        presetsString += "@prefix state: <" LV2_STATE_PREFIX "> .\n";
# endif
        presetsString += "\n";

        const uint32_t numParameters = plugin.getParameterCount();
        const uint32_t numPrograms   = plugin.getProgramCount();
# if DISTRHO_PLUGIN_WANT_STATE
        const uint32_t numStates     = plugin.getStateCount();
# endif

        const String presetSeparator(std::strstr(DISTRHO_PLUGIN_URI, "#") != nullptr ? ":" : "#");

        char strBuf[0xff+1];
        strBuf[0xff] = '\0';

        String presetString;

        for (uint32_t i=0; i<numPrograms; ++i)
        {
            std::snprintf(strBuf, 0xff, "%03i", i+1);

            plugin.loadProgram(i);

            presetString  = "<" DISTRHO_PLUGIN_URI + presetSeparator + "preset" + strBuf + ">\n";
            presetString += "    rdfs:label \"" + plugin.getProgramName(i) + "\" ;\n\n";

            // TODO
# if 0 // DISTRHO_PLUGIN_WANT_STATE
            for (uint32_t j=0; j<numStates; ++j)
            {
                if (j == 0)
                    presetString += "    state:state [\n";
                else
                    presetString += "    [\n";

                presetString += "        <urn:distrho:" + plugin.getStateKey(j) + ">\n";
                presetString += "\"\"\"\n";
                presetString += plugin.getState(j);
                presetString += "\"\"\"\n";

                if (j+1 == numStates)
                {
                    if (numParameters > 0)
                        presetString += "    ] ;\n\n";
                    else
                        presetString += "    ] .\n\n";
                }
                else
                {
                    presetString += "    ] ,\n";
                }
            }
# endif

            for (uint32_t j=0; j <numParameters; ++j)
            {
                if (j == 0)
                    presetString += "    lv2:port [\n";
                else
                    presetString += "    [\n";

                presetString += "        lv2:symbol \"" + plugin.getParameterSymbol(j) + "\" ;\n";

                if (plugin.getParameterHints(j) & kParameterIsInteger)
                    presetString += "        pset:value " + String(int(plugin.getParameterValue(j))) + " ;\n";
                else
                    presetString += "        pset:value " + String(plugin.getParameterValue(j)) + " ;\n";

                if (j+1 == numParameters)
                    presetString += "    ] .\n\n";
                else
                    presetString += "    ] ,\n";
            }

            presetsString += presetString;
        }

        presetsFile << presetsString << std::endl;
        presetsFile.close();
        std::cout << " done!" << std::endl;
    }
#endif
}
