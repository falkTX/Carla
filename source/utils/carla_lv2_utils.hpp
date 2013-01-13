/*
 * Carla LV2 utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __CARLA_LV2_UTILS_HPP__
#define __CARLA_LV2_UTILS_HPP__

#include "carla_utils.hpp"

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/atom-forge.h"
#include "lv2/atom-util.h"
#include "lv2/buf-size.h"
#include "lv2/data-access.h"
// dynmanifest
#include "lv2/event.h"
#include "lv2/event-helpers.h"
#include "lv2/instance-access.h"
#include "lv2/log.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/parameters.h"
#include "lv2/patch.h"
#include "lv2/port-groups.h"
#include "lv2/port-props.h"
#include "lv2/presets.h"
// resize-port
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

#include <QtCore/QMap>
#include <QtCore/QStringList>

// -------------------------------------------------
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

// -------------------------------------------------
// Custom Atom types

struct LV2_Atom_MidiEvent {
    LV2_Atom_Event event;
    uint8_t data[3];
};

// FIXME ?
struct LV2_Atom_Worker_Body {
    uint32_t    size;
    const void* data;
};

// FIXME ?
struct LV2_Atom_Worker {
    LV2_Atom atom;
    LV2_Atom_Worker_Body body;
};

// -------------------------------------------------
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

    // Misc
    Lilv::Node atom_bufferType;
    Lilv::Node atom_sequence;
    Lilv::Node atom_supports;

    Lilv::Node preset_preset;

    Lilv::Node state_state;

    Lilv::Node value_default;
    Lilv::Node value_minimum;
    Lilv::Node value_maximum;

    // Port Data Types
    Lilv::Node midi_event;
    Lilv::Node patch_message;

    // MIDI CC
    Lilv::Node mm_defaultControl;
    Lilv::Node mm_controlType;
    Lilv::Node mm_controlNumber;

    // Other
    Lilv::Node dct_replaces;
    Lilv::Node doap_license;
    Lilv::Node rdf_type;
    Lilv::Node rdfs_label;

    // ----------------------------------------------------------

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

          atom_bufferType    (new_uri(LV2_ATOM__bufferType)),
          atom_sequence      (new_uri(LV2_ATOM__Sequence)),
          atom_supports      (new_uri(LV2_ATOM__supports)),

          preset_preset      (new_uri(LV2_PRESETS__Preset)),

          state_state        (new_uri(LV2_STATE__state)),

          value_default      (new_uri(LV2_CORE__default)),
          value_minimum      (new_uri(LV2_CORE__minimum)),
          value_maximum      (new_uri(LV2_CORE__maximum)),

          midi_event         (new_uri(LV2_MIDI__MidiEvent)),
          patch_message      (new_uri(LV2_PATCH__Message)),

          mm_defaultControl  (new_uri(NS_llmm "defaultMidiController")),
          mm_controlType     (new_uri(NS_llmm "controllerType")),
          mm_controlNumber   (new_uri(NS_llmm "controllerNumber")),

          dct_replaces       (new_uri(NS_dct "replaces")),
          doap_license       (new_uri(NS_doap "license")),
          rdf_type           (new_uri(NS_rdf "type")),
          rdfs_label         (new_uri(NS_rdfs "label"))
    {
        needInit = true;
    }

    void init()
    {
        if (! needInit)
            return;

        needInit = false;
        Lilv::World::load_all();
    }

private:
    bool needInit;
};

// -------------------------------------------------
// Our LV2 World class object

extern Lv2WorldClass lv2World;

// -------------------------------------------------
// Create new RDF object (using lilv)

static inline
const LV2_RDF_Descriptor* lv2_rdf_new(const LV2_URI uri)
{
    CARLA_ASSERT(uri);

    if (! uri)
        return nullptr;

    Lilv::Plugins lilvPlugins = lv2World.get_all_plugins();

    LILV_FOREACH(plugins, i, lilvPlugins)
    {
        Lilv::Plugin lilvPlugin(lilvPlugins.get(i));

        if (strcmp(lilvPlugin.get_uri().as_string(), uri))
            continue;

        LV2_RDF_Descriptor* const rdfDescriptor = new LV2_RDF_Descriptor;

        // --------------------------------------------------
        // Set Plugin Type
        {
            Lilv::Nodes typeNodes(lilvPlugin.get_value(lv2World.rdf_type));

            if (typeNodes.size() > 0)
            {
#if 0
                if (typeNodes.contains(lv2World.class_allpass))
                    rdfDescriptor->Type |= LV2_PLUGIN_ALLPASS;
                if (typeNodes.contains(lv2World.class_amplifier))
                    rdfDescriptor->Type |= LV2_PLUGIN_AMPLIFIER;
                if (typeNodes.contains(lv2World.class_analyzer))
                    rdfDescriptor->Type |= LV2_PLUGIN_ANALYSER;
                if (typeNodes.contains(lv2World.class_bandpass))
                    rdfDescriptor->Type |= LV2_PLUGIN_BANDPASS;
                if (typeNodes.contains(lv2World.class_chorus))
                    rdfDescriptor->Type |= LV2_PLUGIN_CHORUS;
                if (typeNodes.contains(lv2World.class_comb))
                    rdfDescriptor->Type |= LV2_PLUGIN_COMB;
                if (typeNodes.contains(lv2World.class_compressor))
                    rdfDescriptor->Type |= LV2_PLUGIN_COMPRESSOR;
                if (typeNodes.contains(lv2World.class_constant))
                    rdfDescriptor->Type |= LV2_PLUGIN_CONSTANT;
                if (typeNodes.contains(lv2World.class_converter))
                    rdfDescriptor->Type |= LV2_PLUGIN_CONVERTER;
                if (typeNodes.contains(lv2World.class_delay))
                    rdfDescriptor->Type |= LV2_PLUGIN_DELAY;
                if (typeNodes.contains(lv2World.class_distortion))
                    rdfDescriptor->Type |= LV2_PLUGIN_DISTORTION;
                if (typeNodes.contains(lv2World.class_dynamics))
                    rdfDescriptor->Type |= LV2_PLUGIN_DYNAMICS;
                if (typeNodes.contains(lv2World.class_eq))
                    rdfDescriptor->Type |= LV2_PLUGIN_EQ;
                if (typeNodes.contains(lv2World.class_expander))
                    rdfDescriptor->Type |= LV2_PLUGIN_EXPANDER;
                if (typeNodes.contains(lv2World.class_filter))
                    rdfDescriptor->Type |= LV2_PLUGIN_FILTER;
                if (typeNodes.contains(lv2World.class_flanger))
                    rdfDescriptor->Type |= LV2_PLUGIN_FLANGER;
                if (typeNodes.contains(lv2World.class_function))
                    rdfDescriptor->Type |= LV2_PLUGIN_FUNCTION;
                if (typeNodes.contains(lv2World.class_gate))
                    rdfDescriptor->Type |= LV2_PLUGIN_GATE;
                if (typeNodes.contains(lv2World.class_generator))
                    rdfDescriptor->Type |= LV2_PLUGIN_GENERATOR;
                if (typeNodes.contains(lv2World.class_highpass))
                    rdfDescriptor->Type |= LV2_PLUGIN_HIGHPASS;
                if (typeNodes.contains(lv2World.class_instrument))
                    rdfDescriptor->Type |= LV2_PLUGIN_INSTRUMENT;
                if (typeNodes.contains(lv2World.class_limiter))
                    rdfDescriptor->Type |= LV2_PLUGIN_LIMITER;
                if (typeNodes.contains(lv2World.class_lowpass))
                    rdfDescriptor->Type |= LV2_PLUGIN_LOWPASS;
                if (typeNodes.contains(lv2World.class_mixer))
                    rdfDescriptor->Type |= LV2_PLUGIN_MIXER;
                if (typeNodes.contains(lv2World.class_modulator))
                    rdfDescriptor->Type |= LV2_PLUGIN_MODULATOR;
                if (typeNodes.contains(lv2World.class_multiEQ))
                    rdfDescriptor->Type |= LV2_PLUGIN_MULTI_EQ;
                if (typeNodes.contains(lv2World.class_oscillator))
                    rdfDescriptor->Type |= LV2_PLUGIN_OSCILLATOR;
                if (typeNodes.contains(lv2World.class_paraEQ))
                    rdfDescriptor->Type |= LV2_PLUGIN_PARA_EQ;
                if (typeNodes.contains(lv2World.class_phaser))
                    rdfDescriptor->Type |= LV2_PLUGIN_PHASER;
                if (typeNodes.contains(lv2World.class_pitch))
                    rdfDescriptor->Type |= LV2_PLUGIN_PITCH;
                if (typeNodes.contains(lv2World.class_reverb))
                    rdfDescriptor->Type |= LV2_PLUGIN_REVERB;
                if (typeNodes.contains(lv2World.class_simulator))
                    rdfDescriptor->Type |= LV2_PLUGIN_SIMULATOR;
                if (typeNodes.contains(lv2World.class_spatial))
                    rdfDescriptor->Type |= LV2_PLUGIN_SPATIAL;
                if (typeNodes.contains(lv2World.class_spectral))
                    rdfDescriptor->Type |= LV2_PLUGIN_SPECTRAL;
                if (typeNodes.contains(lv2World.class_utility))
                    rdfDescriptor->Type |= LV2_PLUGIN_UTILITY;
                if (typeNodes.contains(lv2World.class_waveshaper))
                    rdfDescriptor->Type |= LV2_PLUGIN_WAVESHAPER;
#endif
            }
        }

        // --------------------------------------------------
        // Set Plugin Information
        {
            rdfDescriptor->URI    = strdup(uri);
            rdfDescriptor->Binary = strdup(lilv_uri_to_path(lilvPlugin.get_library_uri().as_string()));
            rdfDescriptor->Bundle = strdup(lilv_uri_to_path(lilvPlugin.get_bundle_uri().as_string()));

            if (lilvPlugin.get_name())
                rdfDescriptor->Name = strdup(lilvPlugin.get_name().as_string());

            if (lilvPlugin.get_author_name())
                rdfDescriptor->Author = strdup(lilvPlugin.get_author_name().as_string());

            Lilv::Nodes licenseNodes(lilvPlugin.get_value(lv2World.doap_license));

            if (licenseNodes.size() > 0)
                rdfDescriptor->License = strdup(licenseNodes.get_first().as_string());
        }

        // --------------------------------------------------
        // Set Plugin UniqueID
        {
            Lilv::Nodes replaceNodes(lilvPlugin.get_value(lv2World.dct_replaces));

            if (replaceNodes.size() > 0)
            {
                Lilv::Node replaceNode(replaceNodes.get_first());

                if (replaceNode.is_uri())
                {
                    const QString replaceURI(replaceNode.as_uri());

                    if (replaceURI.startsWith("urn:"))
                    {
                        const QString replaceId(replaceURI.split(":").last());

                        bool ok;
                        long uniqueId = replaceId.toLong(&ok);

                        if (ok && uniqueId > 0)
                            rdfDescriptor->UniqueID = uniqueId;
                    }
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin Ports

        if (lilvPlugin.get_num_ports() > 0)
        {
            rdfDescriptor->PortCount = lilvPlugin.get_num_ports();
            rdfDescriptor->Ports = new LV2_RDF_Port[rdfDescriptor->PortCount];

            for (uint32_t j = 0; j < rdfDescriptor->PortCount; j++)
            {
                Lilv::Port lilvPort(lilvPlugin.get_port_by_index(j));
                LV2_RDF_Port* const rdf_port = &rdfDescriptor->Ports[j];

                // --------------------------------------
                // Set Port Information (first)
                {
                    rdf_port->Name   = strdup(Lilv::Node(lilvPort.get_name()).as_string());
                    rdf_port->Symbol = strdup(Lilv::Node(lilvPort.get_symbol()).as_string());
                }

                // --------------------------------------
                // Set Port Mode and Type
                {
#if 0
                    // Mode
                    if (lilvPort.is_a(lv2World.port_input))
                        rdf_port->Type |= LV2_PORT_INPUT;

                    else if (lilvPort.is_a(lv2World.port_output))
                        rdf_port->Type |= LV2_PORT_OUTPUT;

                    else
                        qWarning("lv2_rdf_new(\"%s\") - port '%s' is not input or output", uri, rdf_port->Name);

                    // Type
                    if (lilvPort.is_a(lv2World.port_control))
                        rdf_port->Type |= LV2_PORT_CONTROL;

                    else if (lilvPort.is_a(lv2World.port_audio))
                        rdf_port->Type |= LV2_PORT_AUDIO;

                    else if (lilvPort.is_a(lv2World.port_cv))
                        rdf_port->Type |= LV2_PORT_CV;

                    else if (lilvPort.is_a(lv2World.port_atom))
                    {
                        rdf_port->Type |= LV2_PORT_ATOM;

                        Lilv::Nodes bufferTypeNodes(lilvPort.get_value(lv2World.atom_bufferType));

                        if (bufferTypeNodes.contains(lv2World.atom_Sequence))
                            rdf_port->Type |= LV2_PORT_ATOM_SEQUENCE;

                        Lilv::Nodes supportNodes(lilvPort.get_value(lv2World.atom_supports));

                        if (supportNodes.contains(lv2World.midi_event))
                            rdf_port->Type |= LV2_PORT_DATA_MIDI_EVENT;
                        if (supportNodes.contains(lv2World.patch_message))
                            rdf_port->Type |= LV2_PORT_DATA_PATCH_MESSAGE;
                    }

                    else if (lilvPort.is_a(lv2World.port_event))
                    {
                        rdf_port->Type |= LV2_PORT_EVENT;

                        if (lilvPort.supports_event(lv2World.midi_event))
                            rdf_port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                    }

                    else if (lilvPort.is_a(lv2World.port_midi))
                    {
                        rdf_port->Type |= LV2_PORT_MIDI_LL;
                        rdf_port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                    }

                    else
#endif
                        qWarning("lv2_rdf_new(\"%s\") - port '%s' is of unkown type", uri, rdf_port->Name);
                }

                // --------------------------------------
                // Set Port Properties
                {
                    if (lilvPort.has_property(lv2World.pprop_optional))
                        rdf_port->Properties = LV2_PORT_OPTIONAL;
                    if (lilvPort.has_property(lv2World.pprop_enumeration))
                        rdf_port->Properties = LV2_PORT_ENUMERATION;
                    if (lilvPort.has_property(lv2World.pprop_integer))
                        rdf_port->Properties = LV2_PORT_INTEGER;
                    if (lilvPort.has_property(lv2World.pprop_sampleRate))
                        rdf_port->Properties = LV2_PORT_SAMPLE_RATE;
                    if (lilvPort.has_property(lv2World.pprop_toggled))
                        rdf_port->Properties = LV2_PORT_TOGGLED;

                    if (lilvPort.has_property(lv2World.pprop_artifacts))
                        rdf_port->Properties = LV2_PORT_CAUSES_ARTIFACTS;
                    if (lilvPort.has_property(lv2World.pprop_continuousCV))
                        rdf_port->Properties = LV2_PORT_CONTINUOUS_CV;
                    if (lilvPort.has_property(lv2World.pprop_discreteCV))
                        rdf_port->Properties = LV2_PORT_DISCRETE_CV;
                    if (lilvPort.has_property(lv2World.pprop_expensive))
                        rdf_port->Properties = LV2_PORT_EXPENSIVE;
                    if (lilvPort.has_property(lv2World.pprop_strictBounds))
                        rdf_port->Properties = LV2_PORT_STRICT_BOUNDS;
                    if (lilvPort.has_property(lv2World.pprop_logarithmic))
                        rdf_port->Properties = LV2_PORT_LOGARITHMIC;
                    if (lilvPort.has_property(lv2World.pprop_notAutomatic))
                        rdf_port->Properties = LV2_PORT_NOT_AUTOMATIC;
                    if (lilvPort.has_property(lv2World.pprop_notOnGUI))
                        rdf_port->Properties = LV2_PORT_NOT_ON_GUI;
                    if (lilvPort.has_property(lv2World.pprop_trigger))
                        rdf_port->Properties = LV2_PORT_TRIGGER;

                    if (lilvPort.has_property(lv2World.reportsLatency))
                        rdf_port->Designation = LV2_PORT_DESIGNATION_LATENCY;
                }

                // --------------------------------------
                // Set Port Designation
                {
                    Lilv::Nodes designationNodes(lilvPort.get_value(lv2World.designation));

                    if (designationNodes.size() > 0)
                    {
                        const char* const designation = designationNodes.get_first().as_string();

                        if (strcmp(designation, LV2_CORE__freeWheeling) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_FREEWHEELING;
                        else if (strcmp(designation, LV2_CORE__latency) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_LATENCY;
                        else if (strcmp(designation, LV2_PARAMETERS__sampleRate) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_SAMPLE_RATE;
                        else if (strcmp(designation, LV2_TIME__bar) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_BAR;
                        else if (strcmp(designation, LV2_TIME__barBeat) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_BAR_BEAT;
                        else if (strcmp(designation, LV2_TIME__beat) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_BEAT;
                        else if (strcmp(designation, LV2_TIME__beatUnit) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_BEAT_UNIT;
                        else if (strcmp(designation, LV2_TIME__beatsPerBar) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR;
                        else if (strcmp(designation, LV2_TIME__beatsPerMinute) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE;
                        else if (strcmp(designation, LV2_TIME__frame) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_FRAME;
                        else if (strcmp(designation, LV2_TIME__framesPerSecond) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND;
                        else if (strcmp(designation, LV2_TIME__position) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_POSITION;
                        else if (strcmp(designation, LV2_TIME__speed) == 0)
                            rdf_port->Designation = LV2_PORT_DESIGNATION_TIME_SPEED;
                        else if (strncmp(designation, LV2_PARAMETERS_PREFIX, strlen(LV2_PARAMETERS_PREFIX)) == 0)
                            pass();
                        else if (strncmp(designation, LV2_PORT_GROUPS_PREFIX, strlen(LV2_PORT_GROUPS_PREFIX)) == 0)
                            pass();
                        else
                            qWarning("lv2_rdf_new(\"%s\") - got unknown Port Designation '%s'", uri, designation);
                    }
                }

                // --------------------------------------
                // Set Port MIDI Map
                {
                    Lilv::Nodes midiMapNodes(lilvPort.get_value(lv2World.mm_defaultControl));

                    if (midiMapNodes.size() > 0)
                    {
                        Lilv::Node midiMapNode(midiMapNodes.get_first());

                        if (midiMapNode.is_blank())
                        {
                            Lilv::Nodes midiMapTypeNodes(lv2World.find_nodes(midiMapNode, lv2World.mm_controlType, nullptr));
                            Lilv::Nodes midiMapNumberNodes(lv2World.find_nodes(midiMapNode, lv2World.mm_controlNumber, nullptr));

                            if (midiMapTypeNodes.size() == 1 && midiMapNumberNodes.size() == 1)
                            {
                                const char* const midiMapType = midiMapTypeNodes.get_first().as_string();

                                if (strcmp(midiMapType, LV2_MIDI_Map__CC) == 0)
                                    rdf_port->MidiMap.Type = LV2_PORT_MIDI_MAP_CC;
                                else if (strcmp(midiMapType, LV2_MIDI_Map__NRPN) == 0)
                                    rdf_port->MidiMap.Type = LV2_PORT_MIDI_MAP_NRPN;
                                else
                                    qWarning("lv2_rdf_new(\"%s\") - got unknown Port Midi Map type '%s'", uri, midiMapType);

                                rdf_port->MidiMap.Number = midiMapNumberNodes.get_first().as_int();
                            }
                        }
                    }
                }

                // --------------------------------------
                // Set Port Points
                {
                    Lilv::Nodes valueNodes(lilvPort.get_value(lv2World.value_default));

                    if (valueNodes.size() > 0)
                    {
                        rdf_port->Points.Hints  |= LV2_PORT_POINT_DEFAULT;
                        rdf_port->Points.Default = valueNodes.get_first().as_float();
                    }

                    valueNodes = lilvPort.get_value(lv2World.value_minimum);

                    if (valueNodes.size() > 0)
                    {
                        rdf_port->Points.Hints  |= LV2_PORT_POINT_MINIMUM;
                        rdf_port->Points.Minimum = valueNodes.get_first().as_float();
                    }

                    valueNodes = lilvPort.get_value(lv2World.value_maximum);

                    if (valueNodes.size() > 0)
                    {
                        rdf_port->Points.Hints  |= LV2_PORT_POINT_MAXIMUM;
                        rdf_port->Points.Maximum = valueNodes.get_first().as_float();
                    }
                }

                // --------------------------------------
                // Set Port Unit
                {
                    Lilv::Nodes unitTypeNodes(lilvPort.get_value(lv2World.unit_unit));

                    if (unitTypeNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT_UNIT;

                        const char* const unitType = unitTypeNodes.get_first().as_uri();

                        if (strcmp(unitType, LV2_UNITS__bar) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_BAR;
                        else if (strcmp(unitType, LV2_UNITS__beat) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_BEAT;
                        else if (strcmp(unitType, LV2_UNITS__bpm) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_BPM;
                        else if (strcmp(unitType, LV2_UNITS__cent) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_CENT;
                        else if (strcmp(unitType, LV2_UNITS__cm) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_CM;
                        else if (strcmp(unitType, LV2_UNITS__coef) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_COEF;
                        else if (strcmp(unitType, LV2_UNITS__db) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_DB;
                        else if (strcmp(unitType, LV2_UNITS__degree) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_DEGREE;
                        else if (strcmp(unitType, LV2_UNITS__frame) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_FRAME;
                        else if (strcmp(unitType, LV2_UNITS__hz) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_HZ;
                        else if (strcmp(unitType, LV2_UNITS__inch) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_INCH;
                        else if (strcmp(unitType, LV2_UNITS__khz) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_KHZ;
                        else if (strcmp(unitType, LV2_UNITS__km) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_KM;
                        else if (strcmp(unitType, LV2_UNITS__m) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_M;
                        else if (strcmp(unitType, LV2_UNITS__mhz) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_MHZ;
                        else if (strcmp(unitType, LV2_UNITS__midiNote) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_MIDINOTE;
                        else if (strcmp(unitType, LV2_UNITS__mile) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_MILE;
                        else if (strcmp(unitType, LV2_UNITS__min) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_MIN;
                        else if (strcmp(unitType, LV2_UNITS__mm) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_MM;
                        else if (strcmp(unitType, LV2_UNITS__ms) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_MS;
                        else if (strcmp(unitType, LV2_UNITS__oct) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_OCT;
                        else if (strcmp(unitType, LV2_UNITS__pc) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_PC;
                        else if (strcmp(unitType, LV2_UNITS__s) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_S;
                        else if (strcmp(unitType, LV2_UNITS__semitone12TET) == 0)
                            rdf_port->Unit.Unit = LV2_PORT_UNIT_SEMITONE;
                        else
                            qWarning("lv2_rdf_new(\"%s\") - got unknown Unit type '%s'", uri, unitType);
                    }

                    Lilv::Nodes unitNameNodes(lilvPort.get_value(lv2World.unit_name));

                    if (unitNameNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT_NAME;
                        rdf_port->Unit.Name   = strdup(unitNameNodes.get_first().as_string());
                    }

                    Lilv::Nodes unitRenderNodes(lilvPort.get_value(lv2World.unit_render));

                    if (unitRenderNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT_RENDER;
                        rdf_port->Unit.Render = strdup(unitRenderNodes.get_first().as_string());
                    }

                    Lilv::Nodes unitSymbolNodes(lilvPort.get_value(lv2World.unit_symbol));

                    if (unitSymbolNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT_SYMBOL;
                        rdf_port->Unit.Symbol = strdup(unitSymbolNodes.get_first().as_string());
                    }
                }

                // --------------------------------------
                // Set Port Scale Points

                Lilv::ScalePoints lilvScalePoints(lilvPort.get_scale_points());

                if (lilvScalePoints.size() > 0)
                {
                    rdf_port->ScalePointCount = lilvScalePoints.size();
                    rdf_port->ScalePoints = new LV2_RDF_PortScalePoint[rdf_port->ScalePointCount];

                    uint32_t h = 0;
                    LILV_FOREACH(scale_points, j, lilvScalePoints)
                    {
                        Lilv::ScalePoint lilvScalePoint(lilvScalePoints.get(j));
                        LV2_RDF_PortScalePoint* const rdf_scalePoint = &rdf_port->ScalePoints[h++];

                        rdf_scalePoint->Label = strdup(Lilv::Node(lilvScalePoint.get_label()).as_string());
                        rdf_scalePoint->Value = Lilv::Node(lilvScalePoint.get_value()).as_float();
                    }
                }
            }
        }

#if 0
        // --------------------------------------------------
        // Set Plugin Presets (TODO)
        {
            Lilv::Nodes presetNodes(lilvPlugin.get_related(lv2World.preset_preset));

            if (presetNodes.size() > 0)
            {
                // create a list of preset URIs (for checking appliesTo, sorting and uniqueness)
                QStringList presetListURIs;

                LILV_FOREACH(nodes, j, presetNodes)
                {
                    Lilv::Node presetNode(presetNodes.get(j));
                    // FIXME - check appliesTo

                    QString presetURI(presetNode.as_uri());

                    if (! presetListURIs.contains(presetURI))
                        presetListURIs.append(presetURI);
                }

                presetListURIs.sort();

                // create presets with unique URIs
                rdfDescriptor->PresetCount = presetListURIs.count();
                rdfDescriptor->Presets = new LV2_RDF_Preset[rdfDescriptor->PresetCount];

                // set preset data
                LILV_FOREACH(nodes, j, presetNodes)
                {
                    Lilv::Node presetNode(presetNodes.get(j));
                    lv2World.load_resource(presetNode);

                    LV2_URI presetURI = presetNode.as_uri();
                    int32_t index = presetListURIs.indexOf(QString(presetURI));

                    if (index < 0)
                        continue;

                    LV2_RDF_Preset* const rdf_preset = &rdfDescriptor->Presets[index];

                    // --------------------------------------
                    // Set Preset Information
                    {
                        rdf_preset->URI = strdup(presetURI);

                        Lilv::Nodes presetLabelNodes(lv2World.find_nodes(presetNode, lv2World.rdfs_label, nullptr));

                        if (presetLabelNodes.size() > 0)
                            rdf_preset->Label = strdup(presetLabelNodes.get_first().as_string());
                    }

                    // --------------------------------------
                    // Set Preset Ports
                    {
                        Lilv::Nodes presetPortNodes(lv2World.find_nodes(presetNode, lv2World.port, nullptr));

                        if (presetPortNodes.size() > 0)
                        {
                            rdf_preset->PortCount = presetPortNodes.size();
                            rdf_preset->Ports = new LV2_RDF_PresetPort[rdf_preset->PortCount];

                            uint32_t h = 0;
                            LILV_FOREACH(nodes, k, presetPortNodes)
                            {
                                Lilv::Node presetPortNode(presetPortNodes.get(k));

                                Lilv::Nodes presetPortSymbolNodes(lv2World.find_nodes(presetPortNode, lv2World.symbol, nullptr));
                                Lilv::Nodes presetPortValueNodes(lv2World.find_nodes(presetPortNode, lv2World.preset_value, nullptr));

                                if (presetPortSymbolNodes.size() == 1 && presetPortValueNodes.size() == 1)
                                {
                                    LV2_RDF_PresetPort* const rdf_presetPort = &rdf_preset->Ports[h++];
                                    rdf_presetPort->Symbol = strdup(presetPortSymbolNodes.get_first().as_string());
                                    rdf_presetPort->Value  = presetPortValueNodes.get_first().as_float();
                                }
                            }
                        }
                    }

                    // --------------------------------------
                    // Set Preset States
                    {
                        Lilv::Nodes presetStateNodes(lv2World.find_nodes(presetNode, lv2World.state_state, nullptr));

                        if (presetStateNodes.size() > 0)
                        {
                            rdf_preset->StateCount = presetStateNodes.size();
                            rdf_preset->States = new LV2_RDF_PresetState[rdf_preset->StateCount];

                            uint32_t h = 0;
                            LILV_FOREACH(nodes, k, presetStateNodes)
                            {
                                Lilv::Node presetStateNode(presetStateNodes.get(k));

                                if (presetStateNode.is_blank())
                                {
                                    // TODO
                                    LV2_RDF_PresetState* const rdf_presetState = &rdf_preset->States[h++];
                                    rdf_presetState->Type = LV2_PRESET_STATE_NULL;
                                    rdf_presetState->Key  = nullptr;
                                }
                            }
                        }
                    }
                }
            }
        }
#endif

        // --------------------------------------------------
        // Set Plugin Features
        {
            Lilv::Nodes lilvFeatureNodes(lilvPlugin.get_supported_features());

            if (lilvFeatureNodes.size() > 0)
            {
                Lilv::Nodes lilvFeatureNodesR(lilvPlugin.get_required_features());

                rdfDescriptor->FeatureCount = lilvFeatureNodes.size();
                rdfDescriptor->Features = new LV2_RDF_Feature[rdfDescriptor->FeatureCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, lilvFeatureNodes)
                {
                    Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(j));

                    LV2_RDF_Feature* const rdf_feature = &rdfDescriptor->Features[h++];
                    rdf_feature->Type = lilvFeatureNodesR.contains(lilvFeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                    rdf_feature->URI  = strdup(lilvFeatureNode.as_uri());
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin Extensions
        {
            Lilv::Nodes lilvExtensionDataNodes(lilvPlugin.get_extension_data());

            if (lilvExtensionDataNodes.size() > 0)
            {
                rdfDescriptor->ExtensionCount = lilvExtensionDataNodes.size();
                rdfDescriptor->Extensions = new LV2_URI[rdfDescriptor->ExtensionCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, lilvExtensionDataNodes)
                {
                    Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(j));

                    LV2_URI* const rdf_extension = &rdfDescriptor->Extensions[h++];
                    *rdf_extension = strdup(lilvExtensionDataNode.as_uri());
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin UIs
        {
            Lilv::UIs lilvUIs(lilvPlugin.get_uis());

            if (lilvUIs.size() > 0)
            {
                rdfDescriptor->UICount = lilvUIs.size();
                rdfDescriptor->UIs = new LV2_RDF_UI[rdfDescriptor->UICount];

                uint32_t h = 0;
                LILV_FOREACH(uis, j, lilvUIs)
                {
                    Lilv::UI lilvUI(lilvUIs.get(j));
                    LV2_RDF_UI* const rdf_ui = &rdfDescriptor->UIs[h++];

                    // --------------------------------------
                    // Set UI Type
                    {
#if 0
                        if (lilvUI.is_a(lv2World.ui_gtk2))
                            rdf_ui->Type = LV2_UI_GTK2;
                        else if (lilvUI.is_a(lv2World.ui_gtk3))
                            rdf_ui->Type = LV2_UI_GTK3;
                        else if (lilvUI.is_a(lv2World.ui_qt4))
                            rdf_ui->Type = LV2_UI_QT4;
                        else if (lilvUI.is_a(lv2World.ui_cocoa))
                            rdf_ui->Type = LV2_UI_COCOA;
                        else if (lilvUI.is_a(lv2World.ui_windows))
                            rdf_ui->Type = LV2_UI_WINDOWS;
                        else if (lilvUI.is_a(lv2World.ui_x11))
                            rdf_ui->Type = LV2_UI_X11;
                        else if (lilvUI.is_a(lv2World.ui_external))
                            rdf_ui->Type = LV2_UI_EXTERNAL;
                        else if (lilvUI.is_a(lv2World.ui_externalOld))
                            rdf_ui->Type = LV2_UI_OLD_EXTERNAL;
                        else
#endif
                            qWarning("lv2_rdf_new(\"%s\") - got unknown UI type for '%s'", uri, lilvUI.get_uri().as_uri());
                    }

                    // --------------------------------------
                    // Set UI Information
                    {
                        rdf_ui->URI    = strdup(lilvUI.get_uri().as_uri());
                        rdf_ui->Binary = strdup(lilv_uri_to_path(lilvUI.get_binary_uri().as_string()));
                        rdf_ui->Bundle = strdup(lilv_uri_to_path(lilvUI.get_bundle_uri().as_string()));
                    }

                    // --------------------------------------
                    // Set UI Features
                    {
                        Lilv::Nodes lilvFeatureNodes(lilvUI.get_supported_features());

                        if (lilvFeatureNodes.size() > 0)
                        {
                            Lilv::Nodes lilvFeatureNodesR(lilvUI.get_required_features());

                            rdf_ui->FeatureCount = lilvFeatureNodes.size();
                            rdf_ui->Features = new LV2_RDF_Feature[rdf_ui->FeatureCount];

                            uint32_t x = 0;
                            LILV_FOREACH(nodes, k, lilvFeatureNodes)
                            {
                                Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(k));

                                LV2_RDF_Feature* const rdf_feature = &rdf_ui->Features[x++];
                                rdf_feature->Type = lilvFeatureNodesR.contains(lilvFeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                                rdf_feature->URI  = strdup(lilvFeatureNode.as_uri());
                            }
                        }
                    }

                    // --------------------------------------
                    // Set UI Extensions
                    {
                        Lilv::Nodes lilvExtensionDataNodes(lilvUI.get_extension_data());

                        if (lilvExtensionDataNodes.size() > 0)
                        {
                            rdf_ui->ExtensionCount = lilvExtensionDataNodes.size();
                            rdf_ui->Extensions = new LV2_URI[rdf_ui->ExtensionCount];

                            uint32_t x = 0;
                            LILV_FOREACH(nodes, k, lilvExtensionDataNodes)
                            {
                                Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(k));

                                LV2_URI* const rdf_extension = &rdf_ui->Extensions[x++];
                                *rdf_extension = strdup(lilvExtensionDataNode.as_uri());
                            }
                        }
                    }
                }
            }
        }

        return rdfDescriptor;
    }

    return nullptr;
}

// -------------------------------------------------
// Check if we support a plugin feature

static inline
bool is_lv2_feature_supported(const LV2_URI uri)
{
    if (strcmp(uri, LV2_CORE__hardRTCapable) == 0)
        return true;
    if (strcmp(uri, LV2_CORE__inPlaceBroken) == 0)
        return true;
    if (strcmp(uri, LV2_CORE__isLive) == 0)
        return true;
    if (strcmp(uri, LV2_EVENT_URI) == 0)
        return true;
    if (strcmp(uri, LV2_LOG__log) == 0)
        return true;
    if (strcmp(uri, LV2_OPTIONS__options) == 0)
        return true;
    if (strcmp(uri, LV2_PROGRAMS__Host) == 0)
        return true;
    if (strcmp(uri, LV2_RTSAFE_MEMORY_POOL__Pool) == 0)
        return true;
    if (strcmp(uri, LV2_STATE__makePath) == 0)
        return true;
    if (strcmp(uri, LV2_STATE__mapPath) == 0)
        return true;
    if (strcmp(uri, LV2_PORT_PROPS__supportsStrictBounds) == 0)
        return true;
    if (strcmp(uri, LV2_URI_MAP_URI) == 0)
        return true;
    if (strcmp(uri, LV2_URID__map) == 0)
        return true;
    if (strcmp(uri, LV2_URID__unmap) == 0)
        return true;
    if (strcmp(uri, LV2_WORKER__schedule) == 0)
        return true;
    return false;
}

// -------------------------------------------------
// Check if we support a plugin or UI feature

static inline
bool is_lv2_ui_feature_supported(const LV2_URI uri)
{
    if (is_lv2_feature_supported(uri))
        return true;
    if (strcmp(uri, LV2_DATA_ACCESS_URI) == 0)
        return true;
    if (strcmp(uri, LV2_INSTANCE_ACCESS_URI) == 0)
        return true;
    if (strcmp(uri, LV2_UI__fixedSize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__makeResident) == 0)
        return true;
    if (strcmp(uri, LV2_UI__noUserResize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__parent) == 0)
        return true;
    if (strcmp(uri, LV2_UI__portMap) == 0)
        return true;
    if (strcmp(uri, LV2_UI__portSubscribe) == 0)
        return false; // TODO
    if (strcmp(uri, LV2_UI__resize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__touch) == 0)
        return false; // TODO
    if (strcmp(uri, LV2_EXTERNAL_UI__Widget) == 0)
        return true;
    if (strcmp(uri, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
        return true;
    return false;
}

// -------------------------------------------------

#endif // __CARLA_LV2_UTILS_HPP__
