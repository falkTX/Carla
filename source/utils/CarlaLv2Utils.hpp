/*
 * Carla LV2 utils
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_LV2_UTILS_HPP_INCLUDED
#define CARLA_LV2_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#ifndef nullptr
# undef NULL
# define NULL nullptr
#endif

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/atom-forge.h"
#include "lv2/atom-helpers.h"
#include "lv2/atom-util.h"
#include "lv2/buf-size.h"
#include "lv2/data-access.h"
// dynmanifest
#include "lv2/event.h"
#include "lv2/event-helpers.h"
#include "lv2/instance-access.h"
#include "lv2/log.h"
// logger
#include "lv2/midi.h"
// morph
#include "lv2/options.h"
#include "lv2/parameters.h"
#include "lv2/patch.h"
#include "lv2/port-groups.h"
#include "lv2/port-props.h"
#include "lv2/presets.h"
#include "lv2/resize-port.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/ui.h"
#include "lv2/units.h"
#include "lv2/uri-map.h"
#include "lv2/urid.h"
#include "lv2/worker.h"

#include "lv2/lv2-miditype.h"
#include "lv2/lv2-midifunctions.h"
#include "lv2/lv2_external_ui.h"
#include "lv2/lv2_programs.h"
#include "lv2/lv2_rtmempool.h"

#include "lv2_rdf.hpp"

#include "lilv/lilvmm.hpp"
#include "sratom/sratom.h"

#ifdef USE_QT
# include <QtCore/QStringList>
#else
# include "juce_core.h"
#endif

// -----------------------------------------------------------------------
// Define namespaces and missing prefixes

#define NS_dct  "http://purl.org/dc/terms/"
#define NS_doap "http://usefulinc.com/ns/doap#"
#define NS_rdf  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_rdfs "http://www.w3.org/2000/01/rdf-schema#"
#define NS_llmm "http://ll-plugins.nongnu.org/lv2/ext/midimap#"

#define LV2_MIDI_Map__CC      "http://ll-plugins.nongnu.org/lv2/namespace#CC"
#define LV2_MIDI_Map__NRPN    "http://ll-plugins.nongnu.org/lv2/namespace#NRPN"

#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"

#define LV2_UI__Qt5UI         LV2_UI_PREFIX "Qt5UI"
#define LV2_UI__makeResident  LV2_UI_PREFIX "makeResident"

// -----------------------------------------------------------------------
// Custom Atom types

struct LV2_Atom_MidiEvent {
    LV2_Atom_Event event;
    uint8_t data[4];
};

// -----------------------------------------------------------------------
// Our LV2 World class

class Lv2WorldClass : public Lilv::World
{
public:
    // Base Types
    Lilv::Node port;
    Lilv::Node symbol;
    Lilv::Node designation;
    Lilv::Node freeWheeling;
    Lilv::Node reportsLatency;

    // Plugin Types
    Lilv::Node class_allpass;
    Lilv::Node class_amplifier;
    Lilv::Node class_analyzer;
    Lilv::Node class_bandpass;
    Lilv::Node class_chorus;
    Lilv::Node class_comb;
    Lilv::Node class_compressor;
    Lilv::Node class_constant;
    Lilv::Node class_converter;
    Lilv::Node class_delay;
    Lilv::Node class_distortion;
    Lilv::Node class_dynamics;
    Lilv::Node class_eq;
    Lilv::Node class_envelope;
    Lilv::Node class_expander;
    Lilv::Node class_filter;
    Lilv::Node class_flanger;
    Lilv::Node class_function;
    Lilv::Node class_gate;
    Lilv::Node class_generator;
    Lilv::Node class_highpass;
    Lilv::Node class_instrument;
    Lilv::Node class_limiter;
    Lilv::Node class_lowpass;
    Lilv::Node class_mixer;
    Lilv::Node class_modulator;
    Lilv::Node class_multiEQ;
    Lilv::Node class_oscillator;
    Lilv::Node class_paraEQ;
    Lilv::Node class_phaser;
    Lilv::Node class_pitch;
    Lilv::Node class_reverb;
    Lilv::Node class_simulator;
    Lilv::Node class_spatial;
    Lilv::Node class_spectral;
    Lilv::Node class_utility;
    Lilv::Node class_waveshaper;

    // Port Types
    Lilv::Node port_input;
    Lilv::Node port_output;
    Lilv::Node port_control;
    Lilv::Node port_audio;
    Lilv::Node port_cv;
    Lilv::Node port_atom;
    Lilv::Node port_event;
    Lilv::Node port_midi;

    // Port Properties
    Lilv::Node pprop_optional;
    Lilv::Node pprop_enumeration;
    Lilv::Node pprop_integer;
    Lilv::Node pprop_sampleRate;
    Lilv::Node pprop_toggled;
    Lilv::Node pprop_artifacts;
    Lilv::Node pprop_continuousCV;
    Lilv::Node pprop_discreteCV;
    Lilv::Node pprop_expensive;
    Lilv::Node pprop_strictBounds;
    Lilv::Node pprop_logarithmic;
    Lilv::Node pprop_notAutomatic;
    Lilv::Node pprop_notOnGUI;
    Lilv::Node pprop_trigger;

    // Unit Hints
    Lilv::Node unit_name;
    Lilv::Node unit_render;
    Lilv::Node unit_symbol;
    Lilv::Node unit_unit;

    // UI Types
    Lilv::Node ui_gtk2;
    Lilv::Node ui_gtk3;
    Lilv::Node ui_qt4;
    Lilv::Node ui_qt5;
    Lilv::Node ui_cocoa;
    Lilv::Node ui_windows;
    Lilv::Node ui_x11;
    Lilv::Node ui_external;
    Lilv::Node ui_externalOld;
    Lilv::Node ui_externalOld2;

    // Misc
    Lilv::Node atom_bufferType;
    Lilv::Node atom_sequence;
    Lilv::Node atom_supports;

    Lilv::Node preset_preset;

    Lilv::Node state_state;

    Lilv::Node value_default;
    Lilv::Node value_minimum;
    Lilv::Node value_maximum;

    Lilv::Node rz_asLargeAs;
    Lilv::Node rz_minSize;

    // Port Data Types
    Lilv::Node midi_event;
    Lilv::Node patch_message;
    Lilv::Node time_position;

    // MIDI CC
    Lilv::Node mm_defaultControl;
    Lilv::Node mm_controlType;
    Lilv::Node mm_controlNumber;

    // Other
    Lilv::Node dct_replaces;
    Lilv::Node doap_license;
    Lilv::Node rdf_type;
    Lilv::Node rdfs_label;

    bool needsInit;

    // -------------------------------------------------------------------

    Lv2WorldClass()
        : Lilv::World(),
          port               (new_uri(LV2_CORE__port)),
          symbol             (new_uri(LV2_CORE__symbol)),
          designation        (new_uri(LV2_CORE__designation)),
          freeWheeling       (new_uri(LV2_CORE__freeWheeling)),
          reportsLatency     (new_uri(LV2_CORE__reportsLatency)),

          class_allpass      (new_uri(LV2_CORE__AllpassPlugin)),
          class_amplifier    (new_uri(LV2_CORE__AmplifierPlugin)),
          class_analyzer     (new_uri(LV2_CORE__AnalyserPlugin)),
          class_bandpass     (new_uri(LV2_CORE__BandpassPlugin)),
          class_chorus       (new_uri(LV2_CORE__ChorusPlugin)),
          class_comb         (new_uri(LV2_CORE__CombPlugin)),
          class_compressor   (new_uri(LV2_CORE__CompressorPlugin)),
          class_constant     (new_uri(LV2_CORE__ConstantPlugin)),
          class_converter    (new_uri(LV2_CORE__ConverterPlugin)),
          class_delay        (new_uri(LV2_CORE__DelayPlugin)),
          class_distortion   (new_uri(LV2_CORE__DistortionPlugin)),
          class_dynamics     (new_uri(LV2_CORE__DynamicsPlugin)),
          class_eq           (new_uri(LV2_CORE__EQPlugin)),
          class_envelope     (new_uri(LV2_CORE__EnvelopePlugin)),
          class_expander     (new_uri(LV2_CORE__ExpanderPlugin)),
          class_filter       (new_uri(LV2_CORE__FilterPlugin)),
          class_flanger      (new_uri(LV2_CORE__FlangerPlugin)),
          class_function     (new_uri(LV2_CORE__FunctionPlugin)),
          class_gate         (new_uri(LV2_CORE__GatePlugin)),
          class_generator    (new_uri(LV2_CORE__GeneratorPlugin)),
          class_highpass     (new_uri(LV2_CORE__HighpassPlugin)),
          class_instrument   (new_uri(LV2_CORE__InstrumentPlugin)),
          class_limiter      (new_uri(LV2_CORE__LimiterPlugin)),
          class_lowpass      (new_uri(LV2_CORE__LowpassPlugin)),
          class_mixer        (new_uri(LV2_CORE__MixerPlugin)),
          class_modulator    (new_uri(LV2_CORE__ModulatorPlugin)),
          class_multiEQ      (new_uri(LV2_CORE__MultiEQPlugin)),
          class_oscillator   (new_uri(LV2_CORE__OscillatorPlugin)),
          class_paraEQ       (new_uri(LV2_CORE__ParaEQPlugin)),
          class_phaser       (new_uri(LV2_CORE__PhaserPlugin)),
          class_pitch        (new_uri(LV2_CORE__PitchPlugin)),
          class_reverb       (new_uri(LV2_CORE__ReverbPlugin)),
          class_simulator    (new_uri(LV2_CORE__SimulatorPlugin)),
          class_spatial      (new_uri(LV2_CORE__SpatialPlugin)),
          class_spectral     (new_uri(LV2_CORE__SpectralPlugin)),
          class_utility      (new_uri(LV2_CORE__UtilityPlugin)),
          class_waveshaper   (new_uri(LV2_CORE__WaveshaperPlugin)),

          port_input         (new_uri(LV2_CORE__InputPort)),
          port_output        (new_uri(LV2_CORE__OutputPort)),
          port_control       (new_uri(LV2_CORE__ControlPort)),
          port_audio         (new_uri(LV2_CORE__AudioPort)),
          port_cv            (new_uri(LV2_CORE__CVPort)),
          port_atom          (new_uri(LV2_ATOM__AtomPort)),
          port_event         (new_uri(LV2_EVENT__EventPort)),
          port_midi          (new_uri(LV2_MIDI_LL__MidiPort)),

          pprop_optional     (new_uri(LV2_CORE__connectionOptional)),
          pprop_enumeration  (new_uri(LV2_CORE__enumeration)),
          pprop_integer      (new_uri(LV2_CORE__integer)),
          pprop_sampleRate   (new_uri(LV2_CORE__sampleRate)),
          pprop_toggled      (new_uri(LV2_CORE__toggled)),
          pprop_artifacts    (new_uri(LV2_PORT_PROPS__causesArtifacts)),
          pprop_continuousCV (new_uri(LV2_PORT_PROPS__continuousCV)),
          pprop_discreteCV   (new_uri(LV2_PORT_PROPS__discreteCV)),
          pprop_expensive    (new_uri(LV2_PORT_PROPS__expensive)),
          pprop_strictBounds (new_uri(LV2_PORT_PROPS__hasStrictBounds)),
          pprop_logarithmic  (new_uri(LV2_PORT_PROPS__logarithmic)),
          pprop_notAutomatic (new_uri(LV2_PORT_PROPS__notAutomatic)),
          pprop_notOnGUI     (new_uri(LV2_PORT_PROPS__notOnGUI)),
          pprop_trigger      (new_uri(LV2_PORT_PROPS__trigger)),

          unit_name          (new_uri(LV2_UNITS__name)),
          unit_render        (new_uri(LV2_UNITS__render)),
          unit_symbol        (new_uri(LV2_UNITS__symbol)),
          unit_unit          (new_uri(LV2_UNITS__unit)),

          ui_gtk2            (new_uri(LV2_UI__GtkUI)),
          ui_gtk3            (new_uri(LV2_UI__Gtk3UI)),
          ui_qt4             (new_uri(LV2_UI__Qt4UI)),
          ui_qt5             (new_uri(LV2_UI__Qt5UI)),
          ui_cocoa           (new_uri(LV2_UI__CocoaUI)),
          ui_windows         (new_uri(LV2_UI__WindowsUI)),
          ui_x11             (new_uri(LV2_UI__X11UI)),
          ui_external        (new_uri(LV2_EXTERNAL_UI__Widget)),
          ui_externalOld     (new_uri(LV2_EXTERNAL_UI_DEPRECATED_URI)),
          ui_externalOld2    (new_uri("http://nedko.arnaudov.name/lv2/external_ui/")),

          atom_bufferType    (new_uri(LV2_ATOM__bufferType)),
          atom_sequence      (new_uri(LV2_ATOM__Sequence)),
          atom_supports      (new_uri(LV2_ATOM__supports)),

          preset_preset      (new_uri(LV2_PRESETS__Preset)),

          state_state        (new_uri(LV2_STATE__state)),

          value_default      (new_uri(LV2_CORE__default)),
          value_minimum      (new_uri(LV2_CORE__minimum)),
          value_maximum      (new_uri(LV2_CORE__maximum)),

          rz_asLargeAs       (new_uri(LV2_RESIZE_PORT__asLargeAs)),
          rz_minSize         (new_uri(LV2_RESIZE_PORT__minimumSize)),

          midi_event         (new_uri(LV2_MIDI__MidiEvent)),
          patch_message      (new_uri(LV2_PATCH__Message)),
          time_position      (new_uri(LV2_TIME__Position)),

          mm_defaultControl  (new_uri(NS_llmm "defaultMidiController")),
          mm_controlType     (new_uri(NS_llmm "controllerType")),
          mm_controlNumber   (new_uri(NS_llmm "controllerNumber")),

          dct_replaces       (new_uri(NS_dct "replaces")),
          doap_license       (new_uri(NS_doap "license")),
          rdf_type           (new_uri(NS_rdf "type")),
          rdfs_label         (new_uri(NS_rdfs "label")),

          needsInit(true)
    {
    }

    static Lv2WorldClass& getInstance()
    {
        static Lv2WorldClass lv2World;
        return lv2World;
    }

    void initIfNeeded()
    {
        if (! needsInit)
            return;

        needsInit = false;
        Lilv::World::load_all();
    }

    void load_bundle(const char* const bundle)
    {
        CARLA_SAFE_ASSERT_RETURN(bundle != nullptr && bundle[0] != '\0',);

        needsInit = false;
        Lilv::World::load_bundle(Lilv::Node(new_uri(bundle)));
    }

    const LilvPlugin* getPlugin(const LV2_URI uri) const
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', nullptr);
        CARLA_SAFE_ASSERT_RETURN(! needsInit, nullptr);

        const LilvPlugins* const cPlugins(lilv_world_get_all_plugins(this->me));

        if (cPlugins == nullptr)
        {
            carla_stderr("Lv2WorldClass::getPlugin(\"%s\") - Failed to get plugins from world", uri);
            return nullptr;
        }

        LilvNode* const uriNode(Lilv::World::new_uri(uri));

        if (uriNode == nullptr)
        {
            carla_stderr("Lv2WorldClass::getPlugin(\"%s\") - Failed to get node from uri", uri);
            return nullptr;
        }

        const LilvPlugin* const cPlugin(lilv_plugins_get_by_uri(cPlugins, uriNode));
        lilv_node_free(uriNode);

        if (cPlugin == nullptr)
        {
            carla_stderr("Lv2WorldClass::getPlugin(\"%s\") - Failed to get plugin", uri);
            return nullptr;
        }

        return cPlugin;
    }

    LilvState* getState(const LV2_URI uri, const LV2_URID_Map* const uridMap) const
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', nullptr);
        CARLA_SAFE_ASSERT_RETURN(uridMap != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(! needsInit, nullptr);

        LilvNode* const uriNode(Lilv::World::new_uri(uri));

        if (uriNode == nullptr)
        {
            carla_stderr("Lv2WorldClass::getState(\"%s\", %p) - Failed to get node from uri", uri, uridMap);
            return nullptr;
        }

        LilvState* const cState(lilv_state_new_from_world(this->me, uridMap, uriNode));
        lilv_node_free(uriNode);

        if (cState == nullptr)
        {
            carla_stderr("Lv2WorldClass::getState(\"%s\", %p) - Failed to get state", uri, uridMap);
            return nullptr;
        }

        return cState;
    }

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------
// Create new RDF object (using lilv)

static inline
const LV2_RDF_Descriptor* lv2_rdf_new(const LV2_URI uri, const bool doInit)
{
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', nullptr);

    Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

    if (doInit)
        lv2World.initIfNeeded();

    const LilvPlugin* const cPlugin(lv2World.getPlugin(uri));

    if (cPlugin == nullptr)
    {
        // Error already printed in getPlugin()
        return nullptr;
    }

    Lilv::Plugin lilvPlugin(cPlugin);
    LV2_RDF_Descriptor* const rdfDescriptor(new LV2_RDF_Descriptor());

    // -------------------------------------------------------------------
    // Set Plugin Type
    {
        Lilv::Nodes typeNodes(lilvPlugin.get_value(lv2World.rdf_type));

        if (typeNodes.size() > 0)
        {
            if (typeNodes.contains(lv2World.class_allpass))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_ALLPASS;
            if (typeNodes.contains(lv2World.class_amplifier))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_AMPLIFIER;
            if (typeNodes.contains(lv2World.class_analyzer))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_ANALYSER;
            if (typeNodes.contains(lv2World.class_bandpass))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_BANDPASS;
            if (typeNodes.contains(lv2World.class_chorus))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_CHORUS;
            if (typeNodes.contains(lv2World.class_comb))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_COMB;
            if (typeNodes.contains(lv2World.class_compressor))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_COMPRESSOR;
            if (typeNodes.contains(lv2World.class_constant))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_CONSTANT;
            if (typeNodes.contains(lv2World.class_converter))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_CONVERTER;
            if (typeNodes.contains(lv2World.class_delay))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_DELAY;
            if (typeNodes.contains(lv2World.class_distortion))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_DISTORTION;
            if (typeNodes.contains(lv2World.class_dynamics))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_DYNAMICS;
            if (typeNodes.contains(lv2World.class_eq))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_EQ;
            if (typeNodes.contains(lv2World.class_envelope))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_ENVELOPE;
            if (typeNodes.contains(lv2World.class_expander))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_EXPANDER;
            if (typeNodes.contains(lv2World.class_filter))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_FILTER;
            if (typeNodes.contains(lv2World.class_flanger))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_FLANGER;
            if (typeNodes.contains(lv2World.class_function))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_FUNCTION;
            if (typeNodes.contains(lv2World.class_gate))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_GATE;
            if (typeNodes.contains(lv2World.class_generator))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_GENERATOR;
            if (typeNodes.contains(lv2World.class_highpass))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_HIGHPASS;
            if (typeNodes.contains(lv2World.class_instrument))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_INSTRUMENT;
            if (typeNodes.contains(lv2World.class_limiter))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_LIMITER;
            if (typeNodes.contains(lv2World.class_lowpass))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_LOWPASS;
            if (typeNodes.contains(lv2World.class_mixer))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_MIXER;
            if (typeNodes.contains(lv2World.class_modulator))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_MODULATOR;
            if (typeNodes.contains(lv2World.class_multiEQ))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_MULTI_EQ;
            if (typeNodes.contains(lv2World.class_oscillator))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_OSCILLATOR;
            if (typeNodes.contains(lv2World.class_paraEQ))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_PARA_EQ;
            if (typeNodes.contains(lv2World.class_phaser))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_PHASER;
            if (typeNodes.contains(lv2World.class_pitch))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_PITCH;
            if (typeNodes.contains(lv2World.class_reverb))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_REVERB;
            if (typeNodes.contains(lv2World.class_simulator))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_SIMULATOR;
            if (typeNodes.contains(lv2World.class_spatial))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_SPATIAL;
            if (typeNodes.contains(lv2World.class_spectral))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_SPECTRAL;
            if (typeNodes.contains(lv2World.class_utility))
                rdfDescriptor->Type[1] |= LV2_PLUGIN_UTILITY;
            if (typeNodes.contains(lv2World.class_waveshaper))
                rdfDescriptor->Type[0] |= LV2_PLUGIN_WAVESHAPER;
        }

        lilv_nodes_free(const_cast<LilvNodes*>(typeNodes.me));
    }

    // -------------------------------------------------------------------
    // Set Plugin Information
    {
        rdfDescriptor->URI = carla_strdup(uri);

        if (const char* const name = lilvPlugin.get_name().as_string())
            rdfDescriptor->Name = carla_strdup(name);

        if (const char* const author = lilvPlugin.get_author_name().as_string())
            rdfDescriptor->Author = carla_strdup(author);

        if (const char* const binary = lilvPlugin.get_library_uri().as_string())
            rdfDescriptor->Binary = carla_strdup(lilv_uri_to_path(binary));

        if (const char* const bundle = lilvPlugin.get_bundle_uri().as_string())
            rdfDescriptor->Bundle = carla_strdup(lilv_uri_to_path(bundle));

        Lilv::Nodes licenseNodes(lilvPlugin.get_value(lv2World.doap_license));

        if (licenseNodes.size() > 0)
        {
            if (const char* const license = licenseNodes.get_first().as_string())
                rdfDescriptor->License = carla_strdup(license);
        }

        lilv_nodes_free(const_cast<LilvNodes*>(licenseNodes.me));
    }

    // -------------------------------------------------------------------
    // Set Plugin UniqueID
    {
        Lilv::Nodes replaceNodes(lilvPlugin.get_value(lv2World.dct_replaces));

        if (replaceNodes.size() > 0)
        {
            Lilv::Node replaceNode(replaceNodes.get_first());

            if (replaceNode.is_uri())
            {
#ifdef USE_QT
                const QString replaceURI(replaceNode.as_uri());

                if (replaceURI.startsWith("urn:"))
                {
                    const QString replaceId(replaceURI.split(":").last());

                    bool ok;
                    const ulong uniqueId(replaceId.toULong(&ok));

                    if (ok && uniqueId != 0)
                        rdfDescriptor->UniqueID = uniqueId;
                }
#else
                const juce::String replaceURI(replaceNode.as_uri());

                if (replaceURI.startsWith("urn:"))
                {
                    const int uniqueId(replaceURI.getTrailingIntValue());

                    if (uniqueId > 0)
                        rdfDescriptor->UniqueID = static_cast<ulong>(uniqueId);
                }
#endif
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(replaceNodes.me));
    }

    // -------------------------------------------------------------------
    // Set Plugin Ports

    if (lilvPlugin.get_num_ports() > 0)
    {
        rdfDescriptor->PortCount = lilvPlugin.get_num_ports();
        rdfDescriptor->Ports = new LV2_RDF_Port[rdfDescriptor->PortCount];

        for (uint32_t i = 0; i < rdfDescriptor->PortCount; ++i)
        {
            Lilv::Port lilvPort(lilvPlugin.get_port_by_index(i));
            LV2_RDF_Port* const rdfPort(&rdfDescriptor->Ports[i]);

            // -----------------------------------------------------------
            // Set Port Information
            {
                if (const char* const name = Lilv::Node(lilvPort.get_name()).as_string())
                    rdfPort->Name = carla_strdup(name);

                if (const char* const symbol = Lilv::Node(lilvPort.get_symbol()).as_string())
                    rdfPort->Symbol = carla_strdup(symbol);
            }

            // -----------------------------------------------------------
            // Set Port Mode and Type
            {
                // Input or Output
                if (lilvPort.is_a(lv2World.port_input))
                    rdfPort->Types |= LV2_PORT_INPUT;
                else if (lilvPort.is_a(lv2World.port_output))
                    rdfPort->Types |= LV2_PORT_OUTPUT;
                else
                    carla_stderr("lv2_rdf_new(\"%s\") - port '%s' is not input or output", uri, rdfPort->Name);

                // Data Type
                if (lilvPort.is_a(lv2World.port_control))
                {
                    rdfPort->Types |= LV2_PORT_CONTROL;
                }
                else if (lilvPort.is_a(lv2World.port_audio))
                {
                    rdfPort->Types |= LV2_PORT_AUDIO;
                }
                else if (lilvPort.is_a(lv2World.port_cv))
                {
                    rdfPort->Types |= LV2_PORT_CV;
                }
                else if (lilvPort.is_a(lv2World.port_atom))
                {
                    rdfPort->Types |= LV2_PORT_ATOM;

                    Lilv::Nodes bufferTypeNodes(lilvPort.get_value(lv2World.atom_bufferType));

                    for (LilvIter* it = lilv_nodes_begin(bufferTypeNodes.me); ! lilv_nodes_is_end(bufferTypeNodes.me, it); it = lilv_nodes_next(bufferTypeNodes.me, it))
                    {
                        const Lilv::Node node(lilv_nodes_get(bufferTypeNodes.me, it));
                        CARLA_SAFE_ASSERT_CONTINUE(node.is_uri());

                        if (node.equals(lv2World.atom_sequence))
                            rdfPort->Types |= LV2_PORT_ATOM_SEQUENCE;
                        else
                            carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses an unknown atom buffer type '%s'", uri, rdfPort->Name, node.as_uri());
                    }

                    Lilv::Nodes supportNodes(lilvPort.get_value(lv2World.atom_supports));

                    for (LilvIter* it = lilv_nodes_begin(supportNodes.me); ! lilv_nodes_is_end(supportNodes.me, it); it = lilv_nodes_next(supportNodes.me, it))
                    {
                        const Lilv::Node node(lilv_nodes_get(supportNodes.me, it));
                        CARLA_SAFE_ASSERT_CONTINUE(node.is_uri());

                        if (node.equals(lv2World.midi_event))
                            rdfPort->Types |= LV2_PORT_DATA_MIDI_EVENT;

                        else if (node.equals(lv2World.patch_message))
                            rdfPort->Types |= LV2_PORT_DATA_PATCH_MESSAGE;

                        else if (node.equals(lv2World.time_position))
                            rdfPort->Types |= LV2_PORT_DATA_TIME_POSITION;

#if 0
                        // something new we don't support yet?
                        else if (std::strstr(node.as_uri(), "lv2plug.in/") != nullptr)
                            carla_stderr("lv2_rdf_new(\"%s\") - port '%s' is of atom type but has unsupported data '%s'", uri, rdfPort->Name, node.as_uri());
#endif
                    }

                    lilv_nodes_free(const_cast<LilvNodes*>(bufferTypeNodes.me));
                    lilv_nodes_free(const_cast<LilvNodes*>(supportNodes.me));
                }
                else if (lilvPort.is_a(lv2World.port_event))
                {
                    rdfPort->Types |= LV2_PORT_EVENT;
                    bool supported  = false;

                    if (lilvPort.supports_event(lv2World.midi_event))
                    {
                        rdfPort->Types |= LV2_PORT_DATA_MIDI_EVENT;
                        supported = true;
                    }
                    if (lilvPort.supports_event(lv2World.patch_message))
                    {
                        rdfPort->Types |= LV2_PORT_DATA_PATCH_MESSAGE;
                        supported = true;
                    }
                    if (lilvPort.supports_event(lv2World.time_position))
                    {
                        rdfPort->Types |= LV2_PORT_DATA_TIME_POSITION;
                        supported = true;
                    }

                    if (! supported)
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' is of event type but has unsupported data", uri, rdfPort->Name);
                }
                else if (lilvPort.is_a(lv2World.port_midi))
                {
                    rdfPort->Types |= LV2_PORT_MIDI_LL;
                    rdfPort->Types |= LV2_PORT_DATA_MIDI_EVENT;
                }
                else
                    carla_stderr("lv2_rdf_new(\"%s\") - port '%s' is of unkown data type", uri, rdfPort->Name);
            }

            // -----------------------------------------------------------
            // Set Port Properties
            {
                if (lilvPort.has_property(lv2World.pprop_optional))
                    rdfPort->Properties |= LV2_PORT_OPTIONAL;
                if (lilvPort.has_property(lv2World.pprop_enumeration))
                    rdfPort->Properties |= LV2_PORT_ENUMERATION;
                if (lilvPort.has_property(lv2World.pprop_integer))
                    rdfPort->Properties |= LV2_PORT_INTEGER;
                if (lilvPort.has_property(lv2World.pprop_sampleRate))
                    rdfPort->Properties |= LV2_PORT_SAMPLE_RATE;
                if (lilvPort.has_property(lv2World.pprop_toggled))
                    rdfPort->Properties |= LV2_PORT_TOGGLED;

                if (lilvPort.has_property(lv2World.pprop_artifacts))
                    rdfPort->Properties |= LV2_PORT_CAUSES_ARTIFACTS;
                if (lilvPort.has_property(lv2World.pprop_continuousCV))
                    rdfPort->Properties |= LV2_PORT_CONTINUOUS_CV;
                if (lilvPort.has_property(lv2World.pprop_discreteCV))
                    rdfPort->Properties |= LV2_PORT_DISCRETE_CV;
                if (lilvPort.has_property(lv2World.pprop_expensive))
                    rdfPort->Properties |= LV2_PORT_EXPENSIVE;
                if (lilvPort.has_property(lv2World.pprop_strictBounds))
                    rdfPort->Properties |= LV2_PORT_STRICT_BOUNDS;
                if (lilvPort.has_property(lv2World.pprop_logarithmic))
                    rdfPort->Properties |= LV2_PORT_LOGARITHMIC;
                if (lilvPort.has_property(lv2World.pprop_notAutomatic))
                    rdfPort->Properties |= LV2_PORT_NOT_AUTOMATIC;
                if (lilvPort.has_property(lv2World.pprop_notOnGUI))
                    rdfPort->Properties |= LV2_PORT_NOT_ON_GUI;
                if (lilvPort.has_property(lv2World.pprop_trigger))
                    rdfPort->Properties |= LV2_PORT_TRIGGER;

                if (lilvPort.has_property(lv2World.reportsLatency))
                    rdfPort->Designation = LV2_PORT_DESIGNATION_LATENCY;

                // no port properties set, check if using old/invalid ones
                if (rdfPort->Properties == 0x0)
                {
                    static const Lilv::Node oldPropArtifacts(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#causesArtifacts"));
                    static const Lilv::Node oldPropContinuousCV(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#continuousCV"));
                    static const Lilv::Node oldPropDiscreteCV(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#discreteCV"));
                    static const Lilv::Node oldPropExpensive(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#expensive"));
                    static const Lilv::Node oldPropStrictBounds(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#hasStrictBounds"));
                    static const Lilv::Node oldPropLogarithmic(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#logarithmic"));
                    static const Lilv::Node oldPropNotAutomatic(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#notAutomatic"));
                    static const Lilv::Node oldPropNotOnGUI(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#notOnGUI"));
                    static const Lilv::Node oldPropTrigger(lv2World.new_uri("http://lv2plug.in/ns/dev/extportinfo#trigger"));

                    if (lilvPort.has_property(oldPropArtifacts))
                    {
                        rdfPort->Properties |= LV2_PORT_CAUSES_ARTIFACTS;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'causesArtifacts'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropContinuousCV))
                    {
                        rdfPort->Properties |= LV2_PORT_CONTINUOUS_CV;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'continuousCV'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropDiscreteCV))
                    {
                        rdfPort->Properties |= LV2_PORT_DISCRETE_CV;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'discreteCV'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropExpensive))
                    {
                        rdfPort->Properties |= LV2_PORT_EXPENSIVE;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'expensive'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropStrictBounds))
                    {
                        rdfPort->Properties |= LV2_PORT_STRICT_BOUNDS;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'hasStrictBounds'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropLogarithmic))
                    {
                        rdfPort->Properties |= LV2_PORT_LOGARITHMIC;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'logarithmic'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropNotAutomatic))
                    {
                        rdfPort->Properties |= LV2_PORT_NOT_AUTOMATIC;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'notAutomatic'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropNotOnGUI))
                    {
                        rdfPort->Properties |= LV2_PORT_NOT_ON_GUI;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'notOnGUI'", uri, rdfPort->Name);
                    }
                    if (lilvPort.has_property(oldPropTrigger))
                    {
                        rdfPort->Properties |= LV2_PORT_TRIGGER;
                        carla_stderr("lv2_rdf_new(\"%s\") - port '%s' uses old/invalid LV2 property for 'trigger'", uri, rdfPort->Name);
                    }
                }
            }

            // -----------------------------------------------------------
            // Set Port Designation
            {
                if (LilvNode* const designationNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.designation.me))
                {
                    if (const char* const designation = lilv_node_as_string(designationNode))
                    {
                        if (std::strcmp(designation, LV2_CORE__control) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_CONTROL;
                        else if (std::strcmp(designation, LV2_CORE__freeWheeling) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_FREEWHEELING;
                        else if (std::strcmp(designation, LV2_CORE__latency) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_LATENCY;
                        else if (std::strcmp(designation, LV2_PARAMETERS__sampleRate) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_SAMPLE_RATE;
                        else if (std::strcmp(designation, LV2_TIME__bar) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_BAR;
                        else if (std::strcmp(designation, LV2_TIME__barBeat) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_BAR_BEAT;
                        else if (std::strcmp(designation, LV2_TIME__beat) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_BEAT;
                        else if (std::strcmp(designation, LV2_TIME__beatUnit) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_BEAT_UNIT;
                        else if (std::strcmp(designation, LV2_TIME__beatsPerBar) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR;
                        else if (std::strcmp(designation, LV2_TIME__beatsPerMinute) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE;
                        else if (std::strcmp(designation, LV2_TIME__frame) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_FRAME;
                        else if (std::strcmp(designation, LV2_TIME__framesPerSecond) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND;
                        else if (std::strcmp(designation, LV2_TIME__speed) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_SPEED;
                        else if (std::strncmp(designation, LV2_PARAMETERS_PREFIX, std::strlen(LV2_PARAMETERS_PREFIX)) == 0)
                            pass();
                        else if (std::strncmp(designation, LV2_PORT_GROUPS_PREFIX, std::strlen(LV2_PORT_GROUPS_PREFIX)) == 0)
                            pass();
                        else
                            carla_stderr("lv2_rdf_new(\"%s\") - got unknown port designation '%s'", uri, designation);
                    }
                    lilv_node_free(designationNode);
                }
            }

            // -----------------------------------------------------------
            // Set Port MIDI Map
            {
                if (LilvNode* const midiMapNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.mm_defaultControl.me))
                {
                    if (lilv_node_is_blank(midiMapNode))
                    {
                        Lilv::Nodes midiMapTypeNodes(lv2World.find_nodes(midiMapNode, lv2World.mm_controlType, nullptr));
                        Lilv::Nodes midiMapNumberNodes(lv2World.find_nodes(midiMapNode, lv2World.mm_controlNumber, nullptr));

                        if (midiMapTypeNodes.size() == 1 && midiMapNumberNodes.size() == 1)
                        {
                            if (const char* const midiMapType = midiMapTypeNodes.get_first().as_string())
                            {
                                if (std::strcmp(midiMapType, LV2_MIDI_Map__CC) == 0)
                                    rdfPort->MidiMap.Type = LV2_PORT_MIDI_MAP_CC;
                                else if (std::strcmp(midiMapType, LV2_MIDI_Map__NRPN) == 0)
                                    rdfPort->MidiMap.Type = LV2_PORT_MIDI_MAP_NRPN;
                                else
                                    carla_stderr("lv2_rdf_new(\"%s\") - got unknown port Midi-Map type '%s'", uri, midiMapType);

                                rdfPort->MidiMap.Number = static_cast<uint32_t>(midiMapNumberNodes.get_first().as_int());
                            }
                        }

                        lilv_nodes_free(const_cast<LilvNodes*>(midiMapTypeNodes.me));
                        lilv_nodes_free(const_cast<LilvNodes*>(midiMapNumberNodes.me));
                    }

                    lilv_node_free(midiMapNode);
                }

                // TODO - also check using new official MIDI API too
            }

            // -----------------------------------------------------------
            // Set Port Points
            {
                if (LilvNode* const defNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.value_default.me))
                {
                    rdfPort->Points.Hints  |= LV2_PORT_POINT_DEFAULT;
                    rdfPort->Points.Default = lilv_node_as_float(defNode);
                    lilv_node_free(defNode);
                }

                if (LilvNode* const minNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.value_minimum.me))
                {
                    rdfPort->Points.Hints  |= LV2_PORT_POINT_MINIMUM;
                    rdfPort->Points.Minimum = lilv_node_as_float(minNode);
                    lilv_node_free(minNode);
                }

                if (LilvNode* const maxNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.value_maximum.me))
                {
                    rdfPort->Points.Hints  |= LV2_PORT_POINT_MAXIMUM;
                    rdfPort->Points.Maximum = lilv_node_as_float(maxNode);
                    lilv_node_free(maxNode);
                }
            }

            // -----------------------------------------------------------
            // Set Port Unit
            {
                if (LilvNode* const unitUnitNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.unit_unit.me))
                {
                    if (lilv_node_is_uri(unitUnitNode))
                    {
                        if (const char* const unitUnit = lilv_node_as_uri(unitUnitNode))
                        {
                            rdfPort->Unit.Hints |= LV2_PORT_UNIT_UNIT;

                            if (std::strcmp(unitUnit, LV2_UNITS__bar) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_BAR;
                            else if (std::strcmp(unitUnit, LV2_UNITS__beat) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_BEAT;
                            else if (std::strcmp(unitUnit, LV2_UNITS__bpm) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_BPM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__cent) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_CENT;
                            else if (std::strcmp(unitUnit, LV2_UNITS__cm) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_CM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__coef) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_COEF;
                            else if (std::strcmp(unitUnit, LV2_UNITS__db) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_DB;
                            else if (std::strcmp(unitUnit, LV2_UNITS__degree) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_DEGREE;
                            else if (std::strcmp(unitUnit, LV2_UNITS__frame) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_FRAME;
                            else if (std::strcmp(unitUnit, LV2_UNITS__hz) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_HZ;
                            else if (std::strcmp(unitUnit, LV2_UNITS__inch) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_INCH;
                            else if (std::strcmp(unitUnit, LV2_UNITS__khz) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_KHZ;
                            else if (std::strcmp(unitUnit, LV2_UNITS__km) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_KM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__m) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_M;
                            else if (std::strcmp(unitUnit, LV2_UNITS__mhz) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_MHZ;
                            else if (std::strcmp(unitUnit, LV2_UNITS__midiNote) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_MIDINOTE;
                            else if (std::strcmp(unitUnit, LV2_UNITS__mile) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_MILE;
                            else if (std::strcmp(unitUnit, LV2_UNITS__min) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_MIN;
                            else if (std::strcmp(unitUnit, LV2_UNITS__mm) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_MM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__ms) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_MS;
                            else if (std::strcmp(unitUnit, LV2_UNITS__oct) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_OCT;
                            else if (std::strcmp(unitUnit, LV2_UNITS__pc) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_PC;
                            else if (std::strcmp(unitUnit, LV2_UNITS__s) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_S;
                            else if (std::strcmp(unitUnit, LV2_UNITS__semitone12TET) == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_SEMITONE;
                            else
                                carla_stderr("lv2_rdf_new(\"%s\") - got unknown unit unit '%s'", uri, unitUnit);
                        }
                    }
                    lilv_node_free(unitUnitNode);
                }

                // FIXME

                if (LilvNode* const unitNameNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.unit_name.me))
                {
                    if (const char* const unitName = lilv_node_as_string(unitNameNode))
                    {
                        rdfPort->Unit.Hints |= LV2_PORT_UNIT_NAME;
                        rdfPort->Unit.Name   = carla_strdup(unitName);
                    }
                    lilv_node_free(unitNameNode);
                }

                if (LilvNode* const unitRenderNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.unit_render.me))
                {
                    if (const char* const unitRender = lilv_node_as_string(unitRenderNode))
                    {
                        rdfPort->Unit.Hints |= LV2_PORT_UNIT_RENDER;
                        rdfPort->Unit.Render = carla_strdup(unitRender);
                    }
                    lilv_node_free(unitRenderNode);
                }

                if (LilvNode* const unitSymbolNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.unit_symbol.me))
                {
                    if (const char* const unitSymbol = lilv_node_as_string(unitSymbolNode))
                    {
                        rdfPort->Unit.Hints |= LV2_PORT_UNIT_SYMBOL;
                        rdfPort->Unit.Symbol = carla_strdup(unitSymbol);
                    }
                    lilv_node_free(unitSymbolNode);
                }
            }

            // -----------------------------------------------------------
            // Set Port Minimum Size
            {
                if (LilvNode* const minimumSizeNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.rz_minSize.me))
                {
                    const int minimumSize(lilv_node_as_int(minimumSizeNode));

                    if (minimumSize > 0)
                        rdfPort->MinimumSize = static_cast<uint32_t>(minimumSize);
                    else
                        carla_safe_assert_int("minimumSize > 0", __FILE__, __LINE__, minimumSize);

                    lilv_node_free(minimumSizeNode);
                }
            }

            // -----------------------------------------------------------
            // Set Port Scale Points

            {
                Lilv::ScalePoints lilvScalePoints(lilvPort.get_scale_points());

                if (lilvScalePoints.size() > 0)
                {
                    rdfPort->ScalePointCount = lilvScalePoints.size();
                    rdfPort->ScalePoints = new LV2_RDF_PortScalePoint[rdfPort->ScalePointCount];

                    uint32_t h = 0;
                    LILV_FOREACH(scale_points, it, lilvScalePoints)
                    {
                        CARLA_SAFE_ASSERT_BREAK(h < rdfPort->ScalePointCount);

                        Lilv::ScalePoint lilvScalePoint(lilvScalePoints.get(it));
                        LV2_RDF_PortScalePoint* const rdfScalePoint(&rdfPort->ScalePoints[h++]);

                        if (const char* const label = Lilv::Node(lilvScalePoint.get_label()).as_string())
                            rdfScalePoint->Label = carla_strdup(label);

                        rdfScalePoint->Value = Lilv::Node(lilvScalePoint.get_value()).as_float();
                    }
                }

                lilv_nodes_free(const_cast<LilvNodes*>(lilvScalePoints.me));
            }
        }
    }

    // -------------------------------------------------------------------
    // Set Plugin Presets

    if (doInit)
    {
        Lilv::Nodes presetNodes(lilvPlugin.get_related(lv2World.preset_preset));

        if (presetNodes.size() > 0)
        {
            // create a list of preset URIs (for checking appliesTo, sorting and unique-ness)
            // FIXME - check appliesTo?

#ifdef USE_QT
            QStringList presetListURIs;

            LILV_FOREACH(nodes, it, presetNodes)
            {
                Lilv::Node presetNode(presetNodes.get(it));

                QString presetURI(presetNode.as_uri());

                if (! (presetURI.trimmed().isEmpty() || presetListURIs.contains(presetURI)))
                    presetListURIs.append(presetURI);
            }

            presetListURIs.sort();

            rdfDescriptor->PresetCount = static_cast<uint32_t>(presetListURIs.count());
#else
            juce::StringArray presetListURIs;

            LILV_FOREACH(nodes, it, presetNodes)
            {
                Lilv::Node presetNode(presetNodes.get(it));

                juce::String presetURI(presetNode.as_uri());

                if (presetURI.trim().isNotEmpty())
                    presetListURIs.addIfNotAlreadyThere(presetURI);
            }

            presetListURIs.sortNatural();

            rdfDescriptor->PresetCount = static_cast<uint32_t>(presetListURIs.size());
#endif

            // create presets with unique URIs
            rdfDescriptor->Presets = new LV2_RDF_Preset[rdfDescriptor->PresetCount];

            // set preset data
            LILV_FOREACH(nodes, it, presetNodes)
            {
                Lilv::Node presetNode(presetNodes.get(it));

                if (lv2World.load_resource(presetNode) == -1)
                    continue;

                if (const char* const presetURI = presetNode.as_uri())
                {
#ifdef USE_QT
                    const int index(presetListURIs.indexOf(QString(presetURI)));
#else
                    const int index(presetListURIs.indexOf(juce::String(presetURI)));
#endif
                    CARLA_SAFE_ASSERT_CONTINUE(index >= 0);

                    LV2_RDF_Preset* const rdfPreset(&rdfDescriptor->Presets[index]);

                    // ---------------------------------------------------
                    // Set Preset Information
                    {
                        rdfPreset->URI = carla_strdup(presetURI);

                        Lilv::Nodes presetLabelNodes(lv2World.find_nodes(presetNode, lv2World.rdfs_label, nullptr));

                        if (presetLabelNodes.size() > 0)
                        {
                            if (const char* const label = presetLabelNodes.get_first().as_string())
                                rdfPreset->Label = carla_strdup(label);
                        }

                        lilv_nodes_free(const_cast<LilvNodes*>(presetLabelNodes.me));
                    }
                }
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(presetNodes.me));
    }

    // -------------------------------------------------------------------
    // Set Plugin Features
    {
        Lilv::Nodes lilvFeatureNodes(lilvPlugin.get_supported_features());

        if (lilvFeatureNodes.size() > 0)
        {
            Lilv::Nodes lilvFeatureNodesR(lilvPlugin.get_required_features());

            rdfDescriptor->FeatureCount = lilvFeatureNodes.size();
            rdfDescriptor->Features = new LV2_RDF_Feature[rdfDescriptor->FeatureCount];

            uint32_t h = 0;
            LILV_FOREACH(nodes, it, lilvFeatureNodes)
            {
                CARLA_SAFE_ASSERT_BREAK(h < rdfDescriptor->FeatureCount);

                Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(it));
                LV2_RDF_Feature* const rdfFeature(&rdfDescriptor->Features[h++]);

                rdfFeature->Type = lilvFeatureNodesR.contains(lilvFeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;

                if (const char* const featureURI = lilvFeatureNode.as_uri())
                    rdfFeature->URI = carla_strdup(featureURI);
                else
                    rdfFeature->URI = nullptr;
            }

            lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodesR.me));
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodes.me));
    }

    // -------------------------------------------------------------------
    // Set Plugin Extensions
    {
        Lilv::Nodes lilvExtensionDataNodes(lilvPlugin.get_extension_data());

        if (lilvExtensionDataNodes.size() > 0)
        {
            rdfDescriptor->ExtensionCount = lilvExtensionDataNodes.size();
            rdfDescriptor->Extensions = new LV2_URI[rdfDescriptor->ExtensionCount];

            uint32_t h = 0;
            LILV_FOREACH(nodes, it, lilvExtensionDataNodes)
            {
                CARLA_SAFE_ASSERT_BREAK(h < rdfDescriptor->ExtensionCount);

                Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(it));
                LV2_URI* const rdfExtension(&rdfDescriptor->Extensions[h++]);

                if (lilvExtensionDataNode.is_uri())
                {
                    if (const char* const extURI = lilvExtensionDataNode.as_uri())
                    {
                        *rdfExtension = carla_strdup(extURI);
                        continue;
                    }
                }
                *rdfExtension = nullptr;
            }

            for (uint32_t x=h; x < rdfDescriptor->ExtensionCount; ++x)
                rdfDescriptor->Extensions[x] = nullptr;
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvExtensionDataNodes.me));
    }

    // -------------------------------------------------------------------
    // Set Plugin UIs
    {
        Lilv::UIs lilvUIs(lilvPlugin.get_uis());

        if (lilvUIs.size() > 0)
        {
            rdfDescriptor->UICount = lilvUIs.size();
            rdfDescriptor->UIs = new LV2_RDF_UI[rdfDescriptor->UICount];

            uint32_t h = 0;
            LILV_FOREACH(uis, it, lilvUIs)
            {
                CARLA_SAFE_ASSERT_BREAK(h < rdfDescriptor->UICount);

                Lilv::UI lilvUI(lilvUIs.get(it));
                LV2_RDF_UI* const rdfUI(&rdfDescriptor->UIs[h++]);

                // -------------------------------------------------------
                // Set UI Type

                if (lilvUI.is_a(lv2World.ui_gtk2))
                    rdfUI->Type = LV2_UI_GTK2;
                else if (lilvUI.is_a(lv2World.ui_gtk3))
                    rdfUI->Type = LV2_UI_GTK3;
                else if (lilvUI.is_a(lv2World.ui_qt4))
                    rdfUI->Type = LV2_UI_QT4;
                else if (lilvUI.is_a(lv2World.ui_qt5))
                    rdfUI->Type = LV2_UI_QT5;
                else if (lilvUI.is_a(lv2World.ui_cocoa))
                    rdfUI->Type = LV2_UI_COCOA;
                else if (lilvUI.is_a(lv2World.ui_windows))
                    rdfUI->Type = LV2_UI_WINDOWS;
                else if (lilvUI.is_a(lv2World.ui_x11))
                    rdfUI->Type = LV2_UI_X11;
                else if (lilvUI.is_a(lv2World.ui_external))
                    rdfUI->Type = LV2_UI_EXTERNAL;
                else if (lilvUI.is_a(lv2World.ui_externalOld))
                    rdfUI->Type = LV2_UI_OLD_EXTERNAL;
                else if (lilvUI.is_a(lv2World.ui_externalOld2))
                    pass();
                else
                    carla_stderr("lv2_rdf_new(\"%s\") - UI '%s' is of unknown type", uri, lilvUI.get_uri().as_uri());

                // -------------------------------------------------------
                // Set UI Information
                {
                    if (const char* const uiURI = lilvUI.get_uri().as_uri())
                        rdfUI->URI = carla_strdup(uiURI);

                    if (const char* const uiBinary = lilvUI.get_binary_uri().as_string())
                        rdfUI->Binary = carla_strdup(lilv_uri_to_path(uiBinary));

                    if (const char* const uiBundle = lilvUI.get_bundle_uri().as_string())
                        rdfUI->Bundle = carla_strdup(lilv_uri_to_path(uiBundle));
                }

                // -------------------------------------------------------
                // Set UI Features
                {
                    Lilv::Nodes lilvFeatureNodes(lilvUI.get_supported_features());

                    if (lilvFeatureNodes.size() > 0)
                    {
                        Lilv::Nodes lilvFeatureNodesR(lilvUI.get_required_features());

                        rdfUI->FeatureCount = lilvFeatureNodes.size();
                        rdfUI->Features = new LV2_RDF_Feature[rdfUI->FeatureCount];

                        uint32_t h2 = 0;
                        LILV_FOREACH(nodes, it2, lilvFeatureNodes)
                        {
                            CARLA_SAFE_ASSERT_BREAK(h2 < rdfUI->FeatureCount);

                            Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(it2));
                            LV2_RDF_Feature* const rdfFeature(&rdfUI->Features[h2++]);

                            rdfFeature->Type = lilvFeatureNodesR.contains(lilvFeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;

                            if (const char* const featureURI = lilvFeatureNode.as_uri())
                                rdfFeature->URI = carla_strdup(featureURI);
                            else
                                rdfFeature->URI = nullptr;
                        }

                        lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodesR.me));
                    }

                    lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodes.me));
                }

                // -------------------------------------------------------
                // Set UI Extensions
                {
                    Lilv::Nodes lilvExtensionDataNodes(lilvUI.get_extension_data());

                    if (lilvExtensionDataNodes.size() > 0)
                    {
                        rdfUI->ExtensionCount = lilvExtensionDataNodes.size();
                        rdfUI->Extensions = new LV2_URI[rdfUI->ExtensionCount];

                        uint32_t h2 = 0;
                        LILV_FOREACH(nodes, it2, lilvExtensionDataNodes)
                        {
                            CARLA_SAFE_ASSERT_BREAK(h2 < rdfUI->ExtensionCount);

                            Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(it2));
                            LV2_URI* const rdfExtension(&rdfUI->Extensions[h2++]);

                            if (lilvExtensionDataNode.is_uri())
                            {
                                if (const char* const extURI = lilvExtensionDataNode.as_uri())
                                {
                                    *rdfExtension = carla_strdup(extURI);
                                    continue;
                                }
                            }
                            *rdfExtension = nullptr;
                        }

                        for (uint32_t x2=h2; x2 < rdfUI->ExtensionCount; ++x2)
                            rdfUI->Extensions[x2] = nullptr;
                    }

                    lilv_nodes_free(const_cast<LilvNodes*>(lilvExtensionDataNodes.me));
                }
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvUIs.me));
    }

    return rdfDescriptor;
}

// -----------------------------------------------------------------------
// Check if we support a plugin port

static inline
bool is_lv2_port_supported(const LV2_Property types) noexcept
{
    if (LV2_IS_PORT_CONTROL(types))
        return true;
    if (LV2_IS_PORT_AUDIO(types))
        return true;
    if (LV2_IS_PORT_CV(types))
        return true;
    if (LV2_IS_PORT_ATOM_SEQUENCE(types))
        return true;
    if (LV2_IS_PORT_EVENT(types))
        return true;
    if (LV2_IS_PORT_MIDI_LL(types))
        return true;
    return false;
}

// -----------------------------------------------------------------------
// Check if we support a plugin feature

static inline
bool is_lv2_feature_supported(const LV2_URI uri) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', false);

    if (std::strcmp(uri, LV2_BUF_SIZE__boundedBlockLength) == 0)
        return true;
    if (std::strcmp(uri, LV2_BUF_SIZE__fixedBlockLength) == 0)
        return true;
    if (std::strcmp(uri, LV2_BUF_SIZE__powerOf2BlockLength) == 0)
        return true;
    if (std::strcmp(uri, LV2_CORE__hardRTCapable) == 0)
        return true;
    if (std::strcmp(uri, LV2_CORE__inPlaceBroken) == 0)
        return true;
    if (std::strcmp(uri, LV2_CORE__isLive) == 0)
        return true;
    if (std::strcmp(uri, LV2_EVENT_URI) == 0)
        return true;
    if (std::strcmp(uri, LV2_LOG__log) == 0)
        return true;
    if (std::strcmp(uri, LV2_OPTIONS__options) == 0)
        return true;
    if (std::strcmp(uri, LV2_PROGRAMS__Host) == 0)
        return true;
    if (std::strcmp(uri, LV2_RESIZE_PORT__resize) == 0)
        return true;
    if (std::strcmp(uri, LV2_RTSAFE_MEMORY_POOL__Pool) == 0)
        return true;
    if (std::strcmp(uri, LV2_RTSAFE_MEMORY_POOL_DEPRECATED_URI) == 0)
        return true;
    if (std::strcmp(uri, LV2_STATE__makePath) == 0)
        return true;
    if (std::strcmp(uri, LV2_STATE__mapPath) == 0)
        return true;
    if (std::strcmp(uri, LV2_PORT_PROPS__supportsStrictBounds) == 0)
        return true;
    if (std::strcmp(uri, LV2_URI_MAP_URI) == 0)
        return true;
    if (std::strcmp(uri, LV2_URID__map) == 0)
        return true;
    if (std::strcmp(uri, LV2_URID__unmap) == 0)
        return true;
    if (std::strcmp(uri, LV2_WORKER__schedule) == 0)
        return true;
    return false;
}

// -----------------------------------------------------------------------
// Check if we support a plugin or UI feature

static inline
bool is_lv2_ui_feature_supported(const LV2_URI uri) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', false);

    if (is_lv2_feature_supported(uri))
        return true;
#ifndef BRIDGE_LV2
    if (std::strcmp(uri, LV2_DATA_ACCESS_URI) == 0)
        return true;
    if (std::strcmp(uri, LV2_INSTANCE_ACCESS_URI) == 0)
        return true;
#endif
    if (std::strcmp(uri, LV2_UI__fixedSize) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__makeResident) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__noUserResize) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__parent) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__portMap) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__portSubscribe) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__resize) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__touch) == 0)
        return true;
    if (std::strcmp(uri, LV2_EXTERNAL_UI__Widget) == 0)
        return true;
    if (std::strcmp(uri, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
        return true;
    return false;
}

// -----------------------------------------------------------------------

#endif // CARLA_LV2_UTILS_HPP_INCLUDED
