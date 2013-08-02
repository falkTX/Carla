/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "carla-native-base.cpp"

#include "JuceHeader.h"

#include "lv2/atom.h"
#include "lv2/buf-size.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/ui.h"
#include "lv2/units.h"
#include "lv2/urid.h"
#include "lv2/lv2_external_ui.h"

#include <fstream>

#if JUCE_WINDOWS
# define PLUGIN_EXT ".dll"
#elif JUCE_MAC
# define PLUGIN_EXT ".dylib"
#else
# define PLUGIN_EXT ".so"
#endif

using juce::String;
using juce::StringArray;
using juce::juce_wchar;

// -----------------------------------------------------------------------
// Converts a parameter name to an LV2 compatible symbol

static StringArray gUsedSymbols;

const String nameToSymbol(const String& name, const uint32_t portIndex)
{
    String symbol, trimmedName = name.trimStart().trimEnd().toLowerCase();

    if (trimmedName.isEmpty())
    {
        symbol += "lv2_port_";
        symbol += String(portIndex+1);
    }
    else
    {
        for (int i=0; i < trimmedName.length(); ++i)
        {
            const juce_wchar c = trimmedName[i];
            if (i == 0 && std::isdigit(c))
                symbol += "_";
            else if (std::isalpha(c) || std::isdigit(c))
                symbol += c;
            else
                symbol += "_";
        }
    }

    // Do not allow identical symbols
    if (gUsedSymbols.contains(symbol))
    {
        int offset = 2;
        String offsetStr = "_2";
        symbol += offsetStr;

        while (gUsedSymbols.contains(symbol))
        {
            offset += 1;
            String newOffsetStr = "_" + String(offset);
            symbol = symbol.replace(offsetStr, newOffsetStr);
            offsetStr = newOffsetStr;
        }
    }

    gUsedSymbols.add(symbol);

    return symbol;
}

// -----------------------------------------------------------------------

void writeManifestFile()
{
    String text;

    // -------------------------------------------------------------------
    // Header

    text += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
    text += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
    text += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Plugins

    for (NonRtList<const PluginDescriptor*>::Itenerator it = sPluginDescsMgr.descs.begin(); it.valid(); it.next())
    {
        const PluginDescriptor*& pluginDesc(*it);
        const String label(pluginDesc->label);

        if (label == "carla")
            text += "<http://kxstudio.sf.net/carla>\n";
        else
            text += "<http://kxstudio.sf.net/carla/plugins/" + label + ">\n";

        text += "    a lv2:Plugin ;\n";
        text += "    lv2:binary <carla-native" PLUGIN_EXT "> ;\n";
        text += "    rdfs:seeAlso <" + label + ".ttl> .\n";
        text += "\n";
    }

    // -------------------------------------------------------------------
    // UI

    text += "<http://kxstudio.sf.net/carla#UI>\n";
    text += "    a <" LV2_EXTERNAL_UI__Widget "> ;\n";
    text += "    ui:binary <carla-native" PLUGIN_EXT "> ;\n";
    text += "    lv2:requiredFeature <" LV2_INSTANCE_ACCESS_URI "> .\n";

    // -------------------------------------------------------------------
    // Write file now

    std::fstream manifest("carla-native.lv2/manifest.ttl", std::ios::out);
    manifest << text.toRawUTF8();
    manifest.close();
}

// -----------------------------------------------------------------------

static uint32_t host_getBufferSize(HostHandle) { return 512;     }
static double   host_getSampleRate(HostHandle) { return 44100.0; }
static bool     host_isOffline(HostHandle)     { return true;    }
static intptr_t host_dispatcher(HostHandle, HostDispatcherOpcode, int32_t, intptr_t, void*, float) { return 0; }

void writePluginFile(const PluginDescriptor* const pluginDesc)
{
    const String pluginLabel(pluginDesc->label);
    const String pluginFile("carla-native.lv2/" + pluginLabel + ".ttl");

    uint32_t portIndex = 0;
    String text;

    gUsedSymbols.clear();

    carla_stdout("Generating data for %s...", pluginDesc->name);

    // -------------------------------------------------------------------
    // Init plugin

    HostDescriptor hostDesc;
    hostDesc.handle      = nullptr;
    hostDesc.resourceDir = "";
    hostDesc.uiName      = "";
    hostDesc.get_buffer_size  = host_getBufferSize;
    hostDesc.get_sample_rate  = host_getSampleRate;
    hostDesc.is_offline       = host_isOffline;
    hostDesc.get_time_info    = nullptr;
    hostDesc.write_midi_event = nullptr;
    hostDesc.ui_parameter_changed    = nullptr;
    hostDesc.ui_midi_program_changed = nullptr;
    hostDesc.ui_custom_data_changed  = nullptr;
    hostDesc.ui_closed               = nullptr;
    hostDesc.ui_open_file = nullptr;
    hostDesc.ui_save_file = nullptr;
    hostDesc.dispatcher   = host_dispatcher;

    PluginHandle pluginHandle = pluginDesc->instantiate(&hostDesc);

    CARLA_SAFE_ASSERT_RETURN(pluginHandle != nullptr,)

    // -------------------------------------------------------------------
    // Header

    text += "@prefix atom: <" LV2_ATOM_PREFIX "> .\n";
    text += "@prefix doap: <http://usefulinc.com/ns/doap#> .\n";
    text += "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n";
    text += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
    text += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
    text += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
    text += "@prefix unit: <" LV2_UNITS_PREFIX "> .\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Plugin

    if (pluginLabel == "carla")
        text += "<http://kxstudio.sf.net/carla>\n";
    else
        text += "<http://kxstudio.sf.net/carla/plugins/" + pluginLabel + ">\n";

    //text += "    a " + getPluginType() + " ;\n";

    text += "    lv2:requiredFeature <" LV2_BUF_SIZE__boundedBlockLength "> ,\n";
    text += "                        <" LV2_URID__map "> ;\n";
    text += "    lv2:extensionData <" LV2_OPTIONS__interface "> ,\n";
    text += "                      <" LV2_STATE__interface "> ;\n";
    text += "\n";

    // -------------------------------------------------------------------
    // UIs

    if (pluginDesc->hints & PLUGIN_HAS_GUI)
    {
        text += "    ui:ui <http://kxstudio.sf.net/carla#UI> ;\n";
        text += "\n";
    }

    // -------------------------------------------------------------------
    // MIDI inputs

    for (uint32_t i=0; i < pluginDesc->midiIns; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";
        else
            text += "    [\n";

        text += "        a lv2:InputPort, atom:AtomPort ;\n";
        text += "        atom:bufferType atom:Sequence ;\n";

        if (i == 0)
        {
            text += "        atom:supports <" LV2_MIDI__MidiEvent "> ,\n";
            text += "                      <" LV2_TIME__Position "> ;\n";
            text += "        lv2:designation lv2:control ;\n";
        }
        else
        {
            text += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
        }

        text += "        lv2:index " + String(portIndex++) + " ;\n";

        if (pluginDesc->midiIns > 1)
        {
            text += "        lv2:symbol \"lv2_events_in_" + String(i+1) + "\" ;\n";
            text += "        lv2:name \"Events Input #" + String(i+1) + "\" ;\n";
        }
        else
        {
            text += "        lv2:symbol \"lv2_events_in\" ;\n";
            text += "        lv2:name \"Events Input\" ;\n";
        }

        if (i+1 == pluginDesc->midiIns)
            text += "    ] ;\n\n";
        else
            text += "    ] ,\n";
    }

    // -------------------------------------------------------------------
    // MIDI outputs

    for (uint32_t i=0; i < pluginDesc->midiOuts; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";
        else
            text += "    [\n";

        text += "        a lv2:OutputPort, atom:AtomPort ;\n";
        text += "        atom:bufferType atom:Sequence ;\n";
        text += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";

        if (pluginDesc->midiOuts > 1)
        {
            text += "        lv2:symbol \"lv2_midi_out_" + String(i+1) + "\" ;\n";
            text += "        lv2:name \"MIDI Output #" + String(i+1) + "\" ;\n";
        }
        else
        {
            text += "        lv2:symbol \"lv2_midi_out\" ;\n";
            text += "        lv2:name \"MIDI Output\" ;\n";
        }

        if (i+1 == pluginDesc->midiOuts)
            text += "    ] ;\n\n";
        else
            text += "    ] ,\n";
    }

    // -------------------------------------------------------------------
    // Freewheel port

    text += "    lv2:port [\n";
    text += "        a lv2:InputPort, lv2:ControlPort ;\n";
    text += "        lv2:index " + String(portIndex++) + " ;\n";
    text += "        lv2:symbol \"lv2_freewheel\" ;\n";
    text += "        lv2:name \"Freewheel\" ;\n";
    text += "        lv2:default 0.0 ;\n";
    text += "        lv2:minimum 0.0 ;\n";
    text += "        lv2:maximum 1.0 ;\n";
    text += "        lv2:designation <" LV2_CORE__freeWheeling "> ;\n";
    text += "        lv2:portProperty lv2:toggled ;\n";
    text += "    ] ;\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Audio inputs

    for (uint32_t i=0; i < pluginDesc->audioIns; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";
        else
            text += "    [\n";

        text += "        a lv2:InputPort, lv2:AudioPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_audio_in_" + String(i+1) + "\" ;\n";
        text += "        lv2:name \"Audio Input " + String(i+1) + "\" ;\n";

        if (i+1 == pluginDesc->audioIns)
            text += "    ] ;\n\n";
        else
            text += "    ] ,\n";
    }

    // -------------------------------------------------------------------
    // Audio outputs

    for (uint32_t i=0; i < pluginDesc->audioOuts; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";
        else
            text += "    [\n";

        text += "        a lv2:OutputPort, lv2:AudioPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_audio_out_" + String(i+1) + "\" ;\n";
        text += "        lv2:name \"Audio Output " + String(i+1) + "\" ;\n";

        if (i+1 == pluginDesc->audioOuts)
            text += "    ] ;\n\n";
        else
            text += "    ] ,\n";
    }

    // -------------------------------------------------------------------
    // Parameters

    const uint32_t paramCount(pluginDesc->get_parameter_count != nullptr ? pluginDesc->get_parameter_count(pluginHandle) : 0);

    if (paramCount > 0)
    {
        CARLA_SAFE_ASSERT_RETURN(pluginDesc->get_parameter_info != nullptr,)
        CARLA_SAFE_ASSERT_RETURN(pluginDesc->get_parameter_value != nullptr,)
    }

    for (uint32_t i=0; i < paramCount; ++i)
    {
        const Parameter* paramInfo(pluginDesc->get_parameter_info(pluginHandle, i));
        const String     paramName(paramInfo->name);
        const String     paramUnit(paramInfo->unit != nullptr ? paramInfo->unit : "");
        const float      paramValue(pluginDesc->get_parameter_value(pluginHandle, i));

        CARLA_SAFE_ASSERT_RETURN(paramInfo != nullptr,)

        if (i == 0)
            text += "    lv2:port [\n";
        else
            text += "    [\n";

        text += "        a lv2:InputPort, lv2:ControlPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"" + nameToSymbol(paramName, i) + "\" ;\n";

        if (paramName.isNotEmpty())
            text += "        lv2:name \"" + paramName + "\" ;\n";
        else
            text += "        lv2:name \"Port " + String(i+1) + "\" ;\n";

        text += "        lv2:default " + String::formatted("%f", paramValue) + " ;\n";
        text += "        lv2:minimum 0.0 ;\n";
        text += "        lv2:maximum 1.0 ;\n";

        if (paramUnit.isNotEmpty())
        {
            text += "        units:unit [\n";
            text += "            a units:Unit ;\n";
            text += "            rdfs:label   \"" + paramUnit + "\" ;\n";
            text += "            units:symbol \"" + paramUnit + "\" ;\n";
            text += "            units:render \"%f " + paramUnit + "\" ;\n";
            text += "        ] ;\n";
        }

//         if (! filter->isParameterAutomatable(i))
//             text += "        lv2:portProperty <" LV2_PORT_PROPS__expensive "> ;\n";

        if (i+1 == paramCount)
            text += "    ] ;\n\n";
        else
            text += "    ] ,\n";
    }

    text += "    doap:name \"" + String(pluginDesc->name) + "\" ;\n";
    text += "    doap:maintainer [ foaf:name \"" + String(pluginDesc->maker) + "\" ] .\n";

    // -------------------------------------------------------------------
    // Write file now

    std::fstream pluginStream(pluginFile.toRawUTF8(), std::ios::out);
    pluginStream << text.toRawUTF8();
    pluginStream.close();

    // -------------------------------------------------------------------
    // Cleanup plugin

    if (pluginDesc->cleanup != nullptr)
        pluginDesc->cleanup(pluginHandle);
}

// -----------------------------------------------------------------------

int main()
{
    writeManifestFile();

    for (NonRtList<const PluginDescriptor*>::Itenerator it = sPluginDescsMgr.descs.begin(); it.valid(); it.next())
    {
        const PluginDescriptor*& pluginDesc(*it);
        writePluginFile(pluginDesc);
    }

    carla_stdout("Done.");

    return 0;
}

// -----------------------------------------------------------------------
