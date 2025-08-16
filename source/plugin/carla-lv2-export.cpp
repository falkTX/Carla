// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#define CARLA_NATIVE_PLUGIN_LV2
#include "carla-base.cpp"

#include "lv2/atom.h"
#include "lv2/buf-size.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/parameters.h"
#include "lv2/patch.h"
#include "lv2/port-props.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/ui.h"
#include "lv2/units.h"
#include "lv2/urid.h"
#include "lv2/worker.h"
#include "lv2/inline-display.h"
#include "lv2/lv2_external_ui.h"
#include "lv2/lv2_programs.h"

#include "extra/String.hpp"
#include "water/files/File.h"
#include "water/text/StringArray.h"

#include <fstream>

#if defined(CARLA_OS_WIN)
# define PLUGIN_EXT ".dll"
#elif defined(CARLA_OS_MAC)
# define PLUGIN_EXT ".dylib"
#else
# define PLUGIN_EXT ".so"
#endif

// -----------------------------------------------------------------------
// Converts a parameter name to an LV2 compatible symbol

static water::StringArray gUsedSymbols;

static const water::String nameToSymbol(const water::String& name, const uint32_t portIndex)
{
    water::String symbol, trimmedName = name.trim().toLowerCase();

    if (trimmedName.isEmpty())
    {
        symbol += "lv2_port_";
        symbol += String(portIndex+1);
    }
    else
    {
        if (std::isdigit(static_cast<int>(trimmedName[0])))
            symbol += "_";

        for (int i=0; i < trimmedName.length(); ++i)
        {
            const water::water_uchar c = trimmedName[i];

            if (std::isalpha(static_cast<int>(c)) || std::isdigit(static_cast<int>(c)))
                symbol += c;
            else
                symbol += "_";
        }
    }

    // Do not allow identical symbols
    if (gUsedSymbols.contains(symbol))
    {
        int offset = 2;
        water::String offsetStr = "_2";
        symbol += offsetStr;

        while (gUsedSymbols.contains(symbol))
        {
            offset += 1;
            water::String newOffsetStr = "_" + water::String(offset);
            symbol = symbol.replace(offsetStr, newOffsetStr);
            offsetStr = newOffsetStr;
        }
    }

    gUsedSymbols.add(symbol);

    return symbol;
}

// -----------------------------------------------------------------------

static void writeManifestFile(PluginListManager& plm, const uint32_t microVersion, const uint32_t minorVersion)
{
    water::String text;

    // -------------------------------------------------------------------
    // Header

    text += "@prefix atom: <" LV2_ATOM_PREFIX "> .\n";
    text += "@prefix doap: <http://usefulinc.com/ns/doap#> .\n";
    text += "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n";
    text += "@prefix idpy: <http://harrisonconsoles.com/lv2/inlinedisplay#> .\n";
    text += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
    text += "@prefix mod:  <http://moddevices.com/ns/mod#> .\n";
    text += "@prefix opts: <" LV2_OPTIONS_PREFIX "> .\n";
    text += "@prefix owl:  <http://www.w3.org/2002/07/owl#> .\n";
    text += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
    text += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Project

    text += "<https://kx.studio/carla>\n";
    text += "    a owl:Ontology, doap:Project ;\n";
    text += "    doap:homepage <https://kx.studio/carla> ;\n";
    text += "    doap:maintainer [\n";
    text += "        foaf:homepage <https://falktx.com/> ;\n";
    text += "        foaf:mbox <mailto:falktx@falktx.com> ;\n";
    text += "        foaf:name \"Filipe Coelho\" ;\n";
    text += "    ] ;\n";
    text += "    doap:name \"Carla\" ;\n";
    text += "    doap:shortdesc \"fully-featured audio plugin host.\" ;\n";
    text += "    lv2:microVersion " + String(microVersion) + " ;\n";
    text += "    lv2:minorVersion " + String(minorVersion) + " ;\n";
    text += "    lv2:symbol \"carla\" .\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Extensions

    text += "idpy:interface\n";
    text += "    a lv2:ExtensionData .\n";
    text += "\n";

    text += "idpy:queue_draw\n";
    text += "    a lv2:Feature .\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Plugins

    for (LinkedList<const NativePluginDescriptor*>::Itenerator it = plm.descs.begin2(); it.valid(); it.next())
    {
        const NativePluginDescriptor* const& pluginDesc(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(pluginDesc != nullptr);

        const String label(pluginDesc->label);

        text += "<http://kxstudio.sf.net/carla/plugins/" + label + ">\n";
        text += "    a lv2:Plugin ;\n";
        text += "    lv2:binary <carla" PLUGIN_EXT "> ;\n";
        text += "    lv2:project <https://kx.studio/carla> ;\n";
        text += "    rdfs:seeAlso <" + label + ".ttl> .\n";
        text += "\n";
    }

    // -------------------------------------------------------------------
    // UI

#ifdef HAVE_PYQT
    text += "<http://kxstudio.sf.net/carla/ui-ext>\n";
    text += "    a <" LV2_EXTERNAL_UI__Widget "> ;\n";
    text += "    ui:binary <carla" PLUGIN_EXT "> ;\n";
    text += "    lv2:extensionData <" LV2_UI__idleInterface "> ,\n";
    text += "                      <" LV2_UI__showInterface "> ,\n";
    text += "                      <" LV2_PROGRAMS__UIInterface "> ;\n";
    text += "    lv2:optionalFeature <" LV2_UI__idleInterface "> ;\n";
    text += "    lv2:requiredFeature <" LV2_INSTANCE_ACCESS_URI "> ;\n";
    text += "    opts:supportedOption <" LV2_PARAMETERS__sampleRate "> .\n";
    text += "\n";

#  if 0
    text += "<http://kxstudio.sf.net/carla/ui-bridge-ext>\n";
    text += "    a <" LV2_EXTERNAL_UI__Widget "> ;\n";
    text += "    ui:binary <carla-ui" PLUGIN_EXT "> ;\n";
    text += "    lv2:extensionData <" LV2_UI__idleInterface "> ,\n";
    text += "                      <" LV2_UI__showInterface "> ,\n";
    text += "                      <" LV2_PROGRAMS__UIInterface "> .\n";
#  endif
#endif

    // -------------------------------------------------------------------
    // File handling

    text += "<http://kxstudio.sf.net/carla/file>\n";
    text += "    a lv2:Parameter ;\n";
    text += "    rdfs:label \"file\" ;\n";
    text += "    rdfs:range atom:Path .\n";
    text += "\n";

    text += "<http://kxstudio.sf.net/carla/file/audio>\n";
    text += "    a lv2:Parameter ;\n";
    text += "    mod:fileTypes \"audioloop,audiorecording,audiotrack\" ;\n";
    text += "    rdfs:label \"audio file\" ;\n";
    text += "    rdfs:range atom:Path .\n";
    text += "\n";

    text += "<http://kxstudio.sf.net/carla/file/midi>\n";
    text += "    a lv2:Parameter ;\n";
    text += "    mod:fileTypes \"midisong\" ;\n";
    text += "    rdfs:label \"midi file\" ;\n";
    text += "    rdfs:range atom:Path .\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Write file now

    std::fstream manifest("carla.lv2/manifest.ttl", std::ios::out);
    manifest << text.toRawUTF8();
    manifest.close();
}

// -----------------------------------------------------------------------

static uint32_t host_getBufferSize(NativeHostHandle) { return 512;     }
static double   host_getSampleRate(NativeHostHandle) { return 44100.0; }
static bool     host_isOffline(NativeHostHandle)     { return true;    }
static intptr_t host_dispatcher(NativeHostHandle, NativeHostDispatcherOpcode, int32_t, intptr_t, void*, float) { return 0; }

static void writePluginFile(const NativePluginDescriptor* const pluginDesc,
                            const uint32_t microVersion, const uint32_t minorVersion)
{
    const water::String pluginLabel(pluginDesc->label);
    const water::String pluginFile("carla.lv2/" + pluginLabel + ".ttl");

    uint32_t portIndex = 0;
    water::String text;

    gUsedSymbols.clear();

    carla_stdout("Generating data for %s...", pluginDesc->name);

    // -------------------------------------------------------------------
    // Init plugin

    NativeHostDescriptor hostDesc;
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

    NativePluginHandle pluginHandle = nullptr;

    if (! pluginLabel.startsWithIgnoreCase("carla"))
    {
        pluginHandle = pluginDesc->instantiate(&hostDesc);
        CARLA_SAFE_ASSERT_RETURN(pluginHandle != nullptr,)
    }

    // -------------------------------------------------------------------
    // Header

    text += "@prefix atom:  <" LV2_ATOM_PREFIX "> .\n";
    text += "@prefix doap:  <http://usefulinc.com/ns/doap#> .\n";
    text += "@prefix foaf:  <http://xmlns.com/foaf/0.1/> .\n";
    text += "@prefix idpy: <http://harrisonconsoles.com/lv2/inlinedisplay#> .\n";
    text += "@prefix lv2:   <" LV2_CORE_PREFIX "> .\n";
    text += "@prefix opts:  <" LV2_OPTIONS_PREFIX "> .\n";
    text += "@prefix patch: <" LV2_PATCH_PREFIX "> .\n";
    text += "@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n";
    text += "@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n";
    text += "@prefix ui:    <" LV2_UI_PREFIX "> .\n";
    text += "@prefix unit:  <" LV2_UNITS_PREFIX "> .\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Plugin URI

    text += "<http://kxstudio.sf.net/carla/plugins/" + pluginLabel + ">\n";

    // -------------------------------------------------------------------
    // Category

    switch (pluginDesc->category)
    {
    case NATIVE_PLUGIN_CATEGORY_SYNTH:
        text += "    a lv2:InstrumentPlugin, lv2:Plugin, doap:Project ;\n";
        break;
    case NATIVE_PLUGIN_CATEGORY_DELAY:
        text += "    a lv2:DelayPlugin, lv2:Plugin, doap:Project ;\n";
        break;
    case NATIVE_PLUGIN_CATEGORY_EQ:
        text += "    a lv2:EQPlugin, lv2:Plugin, doap:Project ;\n";
        break;
    case NATIVE_PLUGIN_CATEGORY_FILTER:
        text += "    a lv2:FilterPlugin, lv2:Plugin, doap:Project ;\n";
        break;
    case NATIVE_PLUGIN_CATEGORY_DYNAMICS:
        text += "    a lv2:DynamicsPlugin, lv2:Plugin, doap:Project ;\n";
        break;
    case NATIVE_PLUGIN_CATEGORY_MODULATOR:
        text += "    a lv2:ModulatorPlugin, lv2:Plugin, doap:Project ;\n";
        break;
    case NATIVE_PLUGIN_CATEGORY_UTILITY:
        text += "    a lv2:UtilityPlugin, lv2:Plugin, doap:Project ;\n";
        break;
    default:
        if (pluginDesc->midiIns >= 1 && pluginDesc->midiOuts >= 1 && pluginDesc->audioIns + pluginDesc->audioOuts == 0)
            text += "    a lv2:MIDIPlugin, lv2:Plugin, doap:Project ;\n";
        else
            text += "    a lv2:Plugin, doap:Project ;\n";
        break;
    }

    text += "\n";

    // -------------------------------------------------------------------
    // Features

    // optional
    if (pluginDesc->hints & NATIVE_PLUGIN_IS_RTSAFE)
        text += "    lv2:optionalFeature <" LV2_CORE__hardRTCapable "> ;\n";
    if (pluginDesc->hints & NATIVE_PLUGIN_HAS_INLINE_DISPLAY)
        text += "    lv2:optionalFeature idpy:queue_draw ;\n";
    if ((pluginDesc->hints & NATIVE_PLUGIN_USES_STATE) || (pluginDesc->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE))
        text += "    lv2:optionalFeature <" LV2_STATE__threadSafeRestore "> ;\n";
    text += "\n";

    // required
    text += "    lv2:requiredFeature <" LV2_BUF_SIZE__boundedBlockLength "> ,\n";

    if (pluginDesc->hints & NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS)
        text += "                        <" LV2_BUF_SIZE__fixedBlockLength "> ,\n";

    if (pluginDesc->hints & NATIVE_PLUGIN_REQUESTS_IDLE)
        text += "                        <" LV2_WORKER__schedule "> ,\n";

    text += "                        <" LV2_OPTIONS__options "> ,\n";
    text += "                        <" LV2_URID__map "> ;\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Extensions

    text += "    lv2:extensionData <" LV2_OPTIONS__interface "> ;\n";

    if ((pluginDesc->hints & NATIVE_PLUGIN_USES_STATE) || (pluginDesc->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE))
        text += "    lv2:extensionData <" LV2_STATE__interface "> ;\n";

    if (pluginDesc->category != NATIVE_PLUGIN_CATEGORY_SYNTH)
        text += "    lv2:extensionData <" LV2_PROGRAMS__Interface "> ;\n";

    if (pluginDesc->hints & NATIVE_PLUGIN_HAS_UI)
        text += "    lv2:extensionData <" LV2_WORKER__interface "> ;\n";

    if (pluginDesc->hints & NATIVE_PLUGIN_HAS_INLINE_DISPLAY)
        text += "    lv2:extensionData idpy:interface ;\n";

    text += "\n";

    // -------------------------------------------------------------------
    // Options

    text += "    opts:supportedOption <" LV2_BUF_SIZE__nominalBlockLength "> ,\n";
    text += "                         <" LV2_BUF_SIZE__maxBlockLength "> ,\n";
    text += "                         <" LV2_PARAMETERS__sampleRate "> ;\n";
    text += "\n";

    // -------------------------------------------------------------------
    // UIs

#ifdef HAVE_PYQT
    if ((pluginDesc->hints & NATIVE_PLUGIN_HAS_UI) != 0 && (pluginDesc->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE) == 0)
    {
        text += "    ui:ui <http://kxstudio.sf.net/carla/ui-ext> ;\n";
        text += "\n";
    }
#endif

    // -------------------------------------------------------------------
    // First input MIDI/Time/UI port

    const bool hasEventInPort = (pluginDesc->hints & NATIVE_PLUGIN_USES_TIME) != 0
                             || (pluginDesc->hints & NATIVE_PLUGIN_HAS_UI) != 0;

    if (pluginDesc->midiIns > 0 || hasEventInPort)
    {
        text += "    lv2:port [\n";
        text += "        a lv2:InputPort, atom:AtomPort ;\n";
        text += "        atom:bufferType atom:Sequence ;\n";

        if (pluginDesc->midiIns > 0 && hasEventInPort)
        {
            text += "        atom:supports <" LV2_MIDI__MidiEvent "> ,\n";
            text += "                      <" LV2_TIME__Position "> ;\n";
        }
        else if (pluginDesc->midiIns > 0)
        {
            text += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
        }
        else
        {
            text += "        atom:supports <" LV2_TIME__Position "> ;\n";
        }
        if (pluginDesc->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
            text += "        atom:supports <" LV2_PATCH__Message "> ;\n";

        text += "        lv2:designation lv2:control ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";

        if (hasEventInPort)
        {
            if (pluginDesc->midiIns > 1)
            {
                text += "        lv2:symbol \"lv2_events_in_1\" ;\n";
                text += "        lv2:name \"Events Input #1\" ;\n";
            }
            else
            {
                text += "        lv2:symbol \"lv2_events_in\" ;\n";
                text += "        lv2:name \"Events Input\" ;\n";
            }
        }
        else
        {
            if (pluginDesc->midiIns > 1)
            {
                text += "        lv2:symbol \"lv2_midi_in_1\" ;\n";
                text += "        lv2:name \"MIDI Input #1\" ;\n";
            }
            else
            {
                text += "        lv2:symbol \"lv2_midi_in\" ;\n";
                text += "        lv2:name \"MIDI Input\" ;\n";
            }
        }

        text += "    ] ;\n\n";
    }

    if (pluginDesc->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
    {
        /**/ if (pluginLabel == "audiofile")
            text += "    patch:writable <http://kxstudio.sf.net/carla/file/audio> ;\n\n";
        else if (pluginLabel == "midifile")
            text += "    patch:writable <http://kxstudio.sf.net/carla/file/midi> ;\n\n";
        else
            text += "    patch:writable <http://kxstudio.sf.net/carla/file> ;\n\n";
    }

    // -------------------------------------------------------------------
    // MIDI inputs

    for (uint32_t i=1; i < pluginDesc->midiIns; ++i)
    {
        if (i == 1)
            text += "    lv2:port [\n";

        text += "        a lv2:InputPort, atom:AtomPort ;\n";
        text += "        atom:bufferType atom:Sequence ;\n";
        text += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_midi_in_" + String(i+1) + "\" ;\n";
        text += "        lv2:name \"MIDI Input #" + String(i+1) + "\" ;\n";

        if (i+1 == pluginDesc->midiIns)
            text += "    ] ;\n\n";
        else
            text += "    ] , [\n";
    }

    // -------------------------------------------------------------------
    // First output MIDI/UI port

    const bool hasEventOutPort = (pluginDesc->hints & NATIVE_PLUGIN_HAS_UI) != 0;

    if (pluginDesc->midiIns > 0 || hasEventOutPort)
    {
        text += "    lv2:port [\n";
        text += "        a lv2:OutputPort, atom:AtomPort ;\n";
        text += "        atom:bufferType atom:Sequence ;\n";

        if (pluginDesc->midiOuts > 0)
            text += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
        if (pluginDesc->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
            text += "        atom:supports <" LV2_PATCH__Message "> ;\n";

        text += "        lv2:index " + String(portIndex++) + " ;\n";

        if (hasEventOutPort)
        {
            if (pluginDesc->midiOuts > 1)
            {
                text += "        lv2:symbol \"lv2_events_out_1\" ;\n";
                text += "        lv2:name \"Events Input #1\" ;\n";
            }
            else
            {
                text += "        lv2:symbol \"lv2_events_out\" ;\n";
                text += "        lv2:name \"Events Output\" ;\n";
            }
        }
        else
        {
            if (pluginDesc->midiOuts > 1)
            {
                text += "        lv2:symbol \"lv2_midi_out_1\" ;\n";
                text += "        lv2:name \"MIDI Output #1\" ;\n";
            }
            else
            {
                text += "        lv2:symbol \"lv2_midi_out\" ;\n";
                text += "        lv2:name \"MIDI Output\" ;\n";
            }
        }

        text += "    ] ;\n\n";
    }

    // -------------------------------------------------------------------
    // MIDI outputs

    for (uint32_t i=1; i < pluginDesc->midiOuts; ++i)
    {
        if (i == 1)
            text += "    lv2:port [\n";

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
            text += "    ] , [\n";
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
    text += "        lv2:portProperty lv2:toggled, <" LV2_PORT_PROPS__notOnGUI "> ;\n";
    text += "    ] ;\n";
    text += "\n";

    // -------------------------------------------------------------------
    // Audio inputs

    for (uint32_t i=0; i < pluginDesc->audioIns; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";

        text += "        a lv2:InputPort, lv2:AudioPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_audio_in_" + String(i+1) + "\" ;\n";
        text += "        lv2:name \"Audio Input " + String(i+1) + "\" ;\n";

        if (i+1 == pluginDesc->audioIns)
            text += "    ] ;\n\n";
        else
            text += "    ] , [\n";
    }

    // -------------------------------------------------------------------
    // Audio outputs

    for (uint32_t i=0; i < pluginDesc->audioOuts; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";

        text += "        a lv2:OutputPort, lv2:AudioPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_audio_out_" + String(i+1) + "\" ;\n";
        text += "        lv2:name \"Audio Output " + String(i+1) + "\" ;\n";

        if (i+1 == pluginDesc->audioOuts)
            text += "    ] ;\n\n";
        else
            text += "    ] , [\n";
    }

    // -------------------------------------------------------------------
    // CV inputs

    for (uint32_t i=0; i < pluginDesc->cvIns; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";

        text += "        a lv2:InputPort, lv2:CVPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_cv_in_" + String(i+1) + "\" ;\n";
        text += "        lv2:name \"CV Input " + String(i+1) + "\" ;\n";

        if (i+1 == pluginDesc->cvIns)
            text += "    ] ;\n\n";
        else
            text += "    ] , [\n";
    }

    // -------------------------------------------------------------------
    // CV outputs

    for (uint32_t i=0; i < pluginDesc->cvOuts; ++i)
    {
        if (i == 0)
            text += "    lv2:port [\n";

        text += "        a lv2:OutputPort, lv2:CVPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_cv_out_" + String(i+1) + "\" ;\n";
        text += "        lv2:name \"CV Output " + String(i+1) + "\" ;\n";

        if (i+1 == pluginDesc->cvOuts)
            text += "    ] ;\n\n";
        else
            text += "    ] , [\n";
    }

    // -------------------------------------------------------------------
    // Parameters

    const uint32_t paramCount = (pluginHandle != nullptr && pluginDesc->get_parameter_count != nullptr)
                              ? pluginDesc->get_parameter_count(pluginHandle)
                              : 0;
    if (paramCount > 0)
    {
        CARLA_SAFE_ASSERT_RETURN(pluginDesc->get_parameter_info != nullptr,)
        CARLA_SAFE_ASSERT_RETURN(pluginDesc->get_parameter_value != nullptr,)
    }

    for (uint32_t i=0; i < paramCount; ++i)
    {
        const NativeParameter* paramInfo(pluginDesc->get_parameter_info(pluginHandle, i));
        const water::String paramName(paramInfo->name != nullptr ? paramInfo->name : "");
        const water::String paramUnit(paramInfo->unit != nullptr ? paramInfo->unit : "");

        CARLA_SAFE_ASSERT_RETURN(paramInfo != nullptr,)

        if (i == 0)
            text += "    lv2:port [\n";

        if (paramInfo->hints & NATIVE_PARAMETER_IS_OUTPUT)
            text += "        a lv2:OutputPort, lv2:ControlPort ;\n";
        else
            text += "        a lv2:InputPort, lv2:ControlPort ;\n";

        text += "        lv2:index " + water::String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"" + nameToSymbol(paramName, i) + "\" ;\n";

        if (paramName.isNotEmpty())
            text += "        lv2:name \"" + paramName + "\" ;\n";
        else
            text += "        lv2:name \"Port " + water::String(i+1) + "\" ;\n";

        if ((paramInfo->hints & NATIVE_PARAMETER_IS_OUTPUT) == 0)
            text += "        lv2:default " + water::String::formatted("%f", static_cast<double>(paramInfo->ranges.def)) + " ;\n";

        text += "        lv2:minimum " + water::String::formatted("%f", static_cast<double>(paramInfo->ranges.min)) + " ;\n";
        text += "        lv2:maximum " + water::String::formatted("%f", static_cast<double>(paramInfo->ranges.max)) + " ;\n";

        if ((paramInfo->hints & NATIVE_PARAMETER_IS_AUTOMATABLE) == 0)
            text += "        lv2:portProperty <" LV2_PORT_PROPS__expensive "> ;\n";
        if (paramInfo->hints & NATIVE_PARAMETER_IS_BOOLEAN)
            text += "        lv2:portProperty lv2:toggled ;\n";
        if (paramInfo->hints & NATIVE_PARAMETER_IS_INTEGER)
            text += "        lv2:portProperty lv2:integer ;\n";
        if (paramInfo->hints & NATIVE_PARAMETER_IS_LOGARITHMIC)
            text += "        lv2:portProperty <" LV2_PORT_PROPS__logarithmic "> ;\n";
        if (paramInfo->hints & NATIVE_PARAMETER_USES_SAMPLE_RATE)
            text += "        lv2:portProperty lv2:toggled ;\n";
        if (paramInfo->hints & NATIVE_PARAMETER_USES_SCALEPOINTS)
            text += "        lv2:portProperty lv2:enumeration ;\n";
        if ((paramInfo->hints & NATIVE_PARAMETER_IS_ENABLED) == 0)
            text += "        lv2:portProperty <" LV2_PORT_PROPS__notOnGUI "> ;\n";

        for (uint32_t j=0; j < paramInfo->scalePointCount; ++j)
        {
            const NativeParameterScalePoint* const scalePoint(&paramInfo->scalePoints[j]);

            if (j == 0)
                text += "        lv2:scalePoint [ ";
            else
                text += "                       [ ";

            text += "rdfs:label \"" + water::String(scalePoint->label) + "\" ;\n";
            text += "                         rdf:value  " + water::String::formatted("%f", static_cast<double>(scalePoint->value)) + " ";

            if (j+1 == paramInfo->scalePointCount)
                text += "] ;\n";
            else
                text += "] ,\n";
        }

        if (paramUnit.isNotEmpty())
        {
            text += "        unit:unit [\n";
            text += "            a unit:Unit ;\n";
            text += "            rdfs:label  \"" + paramUnit + "\" ;\n";
            text += "            unit:symbol \"" + paramUnit + "\" ;\n";
            text += "            unit:render \"%f " + paramUnit + "\" ;\n";
            text += "        ] ;\n";
        }

        if (i+1 == paramCount)
            text += "    ] ;\n\n";
        else
            text += "    ] , [\n";
    }

    text += "    doap:developer [ foaf:name \"" + String(pluginDesc->maker) + "\" ] ;\n";

    if (std::strcmp(pluginDesc->copyright, "GNU GPL v2+") == 0)
        text += "    doap:license <http://opensource.org/licenses/GPL-2.0> ;\n";
    if (std::strcmp(pluginDesc->copyright, "LGPL") == 0)
        text += "    doap:license <http://opensource.org/licenses/LGPL-2.0> ;\n";
    if (std::strcmp(pluginDesc->copyright, "ISC") == 0)
        text += "    doap:license <http://opensource.org/licenses/ISC> ;\n";

    text += "    doap:name \"" + String(pluginDesc->name) + "\" ;\n\n";

    text += "    lv2:microVersion " + String(microVersion) + " ;\n";
    text += "    lv2:minorVersion " + String(minorVersion) + " ;\n";
    text += "    lv2:symbol \"" + String(pluginDesc->label).toBasic() + "\" .\n";

#if 0
    // -------------------------------------------------------------------
    // Presets

    if (pluginDesc->get_midi_program_count != nullptr && pluginDesc->get_midi_program_info != nullptr && pluginHandle != nullptr)
    {
        if (const uint32_t presetCount = pluginDesc->get_midi_program_count(pluginHandle))
        {
            const String presetsFile("carla.lv2/" + pluginLabel + "-presets.ttl");
            std::fstream presetsStream(presetsFile.toRawUTF8(), std::ios::out);

            water::String presetId, presetText;

            presetText += "@prefix lv2: <http://lv2plug.in/ns/lv2core#> .\n";
            presetText += "@prefix pset: <http://lv2plug.in/ns/ext/presets#> .\n";
            presetText += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";

            for (uint32_t i=0; i<presetCount; ++i)
            {
                const NativeMidiProgram* const midiProg(pluginDesc->get_midi_program_info(pluginHandle, i));
                pluginDesc->set_midi_program(pluginHandle, 0, midiProg->bank, midiProg->program);

                presetId = String::formatted("%03i", i+1);

                text += "\n<http://kxstudio.sf.net/carla/plugins/" + pluginLabel + "#preset" + presetId + ">\n";
                text += "    a pset:Preset ;\n";
                text += "    lv2:appliesTo <http://kxstudio.sf.net/carla/plugins/" + pluginLabel + "> ;\n";
                text += "    rdfs:seeAlso <" + pluginLabel + "-presets.ttl> .\n";

                presetText += "\n<http://kxstudio.sf.net/carla/plugins/" + pluginLabel + "#preset" + presetId + ">\n";
                presetText += "    a pset:Preset ;\n";
                presetText += "    lv2:appliesTo <http://kxstudio.sf.net/carla/plugins/" + pluginLabel + "> ;\n";
                presetText += "    rdfs:label \"" + String(midiProg->name) + "\" ;\n";

                for (uint32_t j=0; j < paramCount; ++j)
                {
                    const NativeParameter* paramInfo(pluginDesc->get_parameter_info(pluginHandle, j));
                    const String           paramName(paramInfo->name != nullptr ? paramInfo->name : "");
                    const String           paramUnit(paramInfo->unit != nullptr ? paramInfo->unit : "");

                    CARLA_SAFE_ASSERT_RETURN(paramInfo != nullptr,)

                    if (j == 0)
                        presetText += "    lv2:port [\n";

                    presetText += "        lv2:symbol \"" + nameToSymbol(paramName, j) + "\" ;\n";
                    presetText += "        pset:value " + String::formatted("%f", static_cast<double>(pluginDesc->get_parameter_value(pluginHandle, j))) + " ;\n";

                    if (j+1 == paramCount)
                        presetText += "    ] ;\n\n";
                    else
                        presetText += "    ] , [\n";
                }

                presetsStream << presetText.toRawUTF8();
                presetText.clear();
            }

            presetsStream.close();
        }
    }
#endif

    // -------------------------------------------------------------------
    // Write file now

    std::fstream pluginStream(pluginFile.toRawUTF8(), std::ios::out);
    pluginStream << text.toRawUTF8();
    pluginStream.close();

    // -------------------------------------------------------------------
    // Cleanup plugin

    if (pluginHandle != nullptr && pluginDesc->cleanup != nullptr)
        pluginDesc->cleanup(pluginHandle);
}

// -----------------------------------------------------------------------

int main()
{
    PluginListManager& plm(PluginListManager::getInstance());

    const uint32_t majorVersion = (CARLA_VERSION_HEX & 0xFF0000) >> 16;
    const uint32_t microVersion = (CARLA_VERSION_HEX & 0x00FF00) >> 8;
    const uint32_t minorVersion = (CARLA_VERSION_HEX & 0x0000FF) >> 0;

    const uint32_t lv2MicroVersion = majorVersion * 10 + microVersion;

    writeManifestFile(plm, lv2MicroVersion, minorVersion);

    for (LinkedList<const NativePluginDescriptor*>::Itenerator it = plm.descs.begin2(); it.valid(); it.next())
    {
        const NativePluginDescriptor* const& pluginDesc(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(pluginDesc != nullptr);

        writePluginFile(pluginDesc, lv2MicroVersion, minorVersion);
    }

    carla_stdout("Done.");

    return 0;
}

// -----------------------------------------------------------------------
