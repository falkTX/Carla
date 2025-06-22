/*
 * Carla LV2 utils
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaMathUtils.hpp"
#include "CarlaStringList.hpp"
#include "CarlaMIDI.h"

#ifndef nullptr
# undef NULL
# define NULL nullptr
#endif

// disable -Wdocumentation for LV2 headers
#if defined(__clang__) && (__clang_major__ * 100 + __clang_minor__) > 300
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdocumentation"
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
#include "lv2/inline-display.h"
#include "lv2/instance-access.h"
#include "lv2/log.h"
// logger
#include "lv2/midi.h"
#include "lv2/midnam.h"
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
#include "lv2/lv2_kxstudio_properties.h"
#include "lv2/lv2_programs.h"
#include "lv2/lv2_rtmempool.h"

#include "lv2/control-input-port-change-request.h"

#include "lilv/lilvmm.hpp"
#include "sratom/sratom.h"
#include "lilv/config/lilv_config.h"

// enable -Wdocumentation again
#if defined(__clang__) && (__clang_major__ * 100 + __clang_minor__) > 300
# pragma clang diagnostic pop
#endif

#include "lv2_rdf.hpp"

#if defined(CARLA_UTILS_USE_QT)
# include <QtCore/QStringList>
#else
# include "water/text/StringArray.h"
#endif

// used for scalepoint sorting
#include <map>
typedef std::map<double,const LilvScalePoint*> LilvScalePointMap;

// --------------------------------------------------------------------------------------------------------------------
// Define namespaces and missing prefixes

#define NS_dct  "http://purl.org/dc/terms/"
#define NS_doap "http://usefulinc.com/ns/doap#"
#define NS_rdf  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_rdfs "http://www.w3.org/2000/01/rdf-schema#"
#define NS_llmm "http://ll-plugins.nongnu.org/lv2/ext/midimap#"
#define NS_devp "http://lv2plug.in/ns/dev/extportinfo#"

#define LV2_MIDI_Map__CC      "http://ll-plugins.nongnu.org/lv2/namespace#CC"
#define LV2_MIDI_Map__NRPN    "http://ll-plugins.nongnu.org/lv2/namespace#NRPN"

#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"

#define LV2_UI__makeResident       LV2_UI_PREFIX "makeResident"
#define LV2_UI__makeSONameResident LV2_UI_PREFIX "makeSONameResident"

// TODO: update LV2 headers once again
#define LV2_CORE__Parameter   LV2_CORE_PREFIX "Parameter"   ///< http://lv2plug.in/ns/lv2core#Parameter
#define LV2_CORE__enabled     LV2_CORE_PREFIX "enabled"     ///< http://lv2plug.in/ns/lv2core#enabled
#define LV2_CORE__isSideChain LV2_CORE_PREFIX "isSideChain" ///< http://lv2plug.in/ns/lv2core#isSideChain

// --------------------------------------------------------------------------------------------------------------------
// Custom Atom types

struct LV2_Atom_MidiEvent {
    LV2_Atom atom;    /**< Atom header. */
    uint8_t  data[4]; /**< MIDI data (body). */
};

static inline
uint32_t lv2_atom_total_size(const LV2_Atom_MidiEvent& midiEv)
{
    return static_cast<uint32_t>(sizeof(LV2_Atom)) + midiEv.atom.size;
}

// ---------------------------------------------------------------------------------------------------------------------
// -Weffc++ compat ext widget

extern "C" {

typedef struct _LV2_External_UI_Widget_Compat {
  void (*run )(struct _LV2_External_UI_Widget_Compat*);
  void (*show)(struct _LV2_External_UI_Widget_Compat*);
  void (*hide)(struct _LV2_External_UI_Widget_Compat*);

  _LV2_External_UI_Widget_Compat() noexcept
      : run(nullptr), show(nullptr), hide(nullptr) {}

} LV2_External_UI_Widget_Compat;

}

// --------------------------------------------------------------------------------------------------------------------
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
    Lilv::Node pprop_isSideChain;
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
    Lilv::Node pprop_nonAutomatable;

    // Unit Hints
    Lilv::Node unit_name;
    Lilv::Node unit_render;
    Lilv::Node unit_symbol;
    Lilv::Node unit_unit;

    // UI Types
    Lilv::Node ui;
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

    Lilv::Node lv2_name;
    Lilv::Node lv2_symbol;
    Lilv::Node patch_readable;
    Lilv::Node patch_writable;
    Lilv::Node pg_group;
    Lilv::Node pg_sideChainOf;
    Lilv::Node preset_preset;
    Lilv::Node state_state;

    Lilv::Node ui_portIndex;
    Lilv::Node ui_portNotif;
    Lilv::Node ui_protocol;

    Lilv::Node value_default;
    Lilv::Node value_minimum;
    Lilv::Node value_maximum;

    Lilv::Node rz_asLargeAs;
    Lilv::Node rz_minSize;

    // Port Data Types
    Lilv::Node midi_binding;
    Lilv::Node midi_ctlrNumber;
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
    Lilv::Node rdfs_comment;
    Lilv::Node rdfs_label;
    Lilv::Node rdfs_range;

    bool needsInit;

    const LilvPlugins* allPlugins;
    const LilvPlugin** cachedPlugins;
    uint pluginCount;

    // ----------------------------------------------------------------------------------------------------------------

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
          pprop_isSideChain  (new_uri(LV2_CORE__isSideChain)),
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
          pprop_nonAutomatable (new_uri(LV2_KXSTUDIO_PROPERTIES__NonAutomatable)),

          unit_name          (new_uri(LV2_UNITS__name)),
          unit_render        (new_uri(LV2_UNITS__render)),
          unit_symbol        (new_uri(LV2_UNITS__symbol)),
          unit_unit          (new_uri(LV2_UNITS__unit)),

          ui                 (new_uri(LV2_UI__UI)),
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

          lv2_name           (new_uri(LV2_CORE__name)),
          lv2_symbol         (new_uri(LV2_CORE__symbol)),
          patch_readable     (new_uri(LV2_PATCH__readable)),
          patch_writable     (new_uri(LV2_PATCH__writable)),
          pg_group           (new_uri(LV2_PORT_GROUPS__group)),
          pg_sideChainOf     (new_uri(LV2_PORT_GROUPS__sideChainOf)),
          preset_preset      (new_uri(LV2_PRESETS__Preset)),
          state_state        (new_uri(LV2_STATE__state)),

          ui_portIndex       (new_uri(LV2_UI__portIndex)),
          ui_portNotif       (new_uri(LV2_UI__portNotification)),
          ui_protocol        (new_uri(LV2_UI__protocol)),

          value_default      (new_uri(LV2_CORE__default)),
          value_minimum      (new_uri(LV2_CORE__minimum)),
          value_maximum      (new_uri(LV2_CORE__maximum)),

          rz_asLargeAs       (new_uri(LV2_RESIZE_PORT__asLargeAs)),
          rz_minSize         (new_uri(LV2_RESIZE_PORT__minimumSize)),

          midi_binding       (new_uri(LV2_MIDI__binding)),
          midi_ctlrNumber    (new_uri(LV2_MIDI__controllerNumber)),
          midi_event         (new_uri(LV2_MIDI__MidiEvent)),
          patch_message      (new_uri(LV2_PATCH__Message)),
          time_position      (new_uri(LV2_TIME__Position)),

          mm_defaultControl  (new_uri(NS_llmm "defaultMidiController")),
          mm_controlType     (new_uri(NS_llmm "controllerType")),
          mm_controlNumber   (new_uri(NS_llmm "controllerNumber")),

          dct_replaces       (new_uri(NS_dct "replaces")),
          doap_license       (new_uri(NS_doap "license")),
          rdf_type           (new_uri(NS_rdf "type")),
          rdfs_comment       (new_uri(NS_rdfs "comment")),
          rdfs_label         (new_uri(NS_rdfs "label")),
          rdfs_range         (new_uri(NS_rdfs "range")),

          needsInit(true),
          allPlugins(nullptr),
          cachedPlugins(nullptr),
          pluginCount(0) {}

    ~Lv2WorldClass() override
    {
        pluginCount = 0;
        allPlugins = nullptr;

        if (cachedPlugins != nullptr)
        {
            delete[] cachedPlugins;
            cachedPlugins = nullptr;
        }
    }

    // FIXME - remove this
    static Lv2WorldClass& getInstance()
    {
        static Lv2WorldClass lv2World;
        return lv2World;
    }

    void initIfNeeded(const char* LV2_PATH)
    {
        if (LV2_PATH == nullptr || LV2_PATH[0] == '\0')
        {
            static const char* const DEFAULT_LV2_PATH = LILV_DEFAULT_LV2_PATH;
            LV2_PATH = DEFAULT_LV2_PATH;
        }

        if (! needsInit)
            return;

        needsInit = false;

        Lilv::World::load_all(LV2_PATH);

        allPlugins = lilv_world_get_all_plugins(this->me);
        CARLA_SAFE_ASSERT_RETURN(allPlugins != nullptr,);

        if ((pluginCount = lilv_plugins_size(allPlugins)))
        {
            cachedPlugins = new const LilvPlugin*[pluginCount+1];
            carla_zeroPointers(cachedPlugins, pluginCount+1);

            int i = 0;
            for (LilvIter* it = lilv_plugins_begin(allPlugins); ! lilv_plugins_is_end(allPlugins, it); it = lilv_plugins_next(allPlugins, it))
                cachedPlugins[i++] = lilv_plugins_get(allPlugins, it);
        }
    }

    void load_bundle(const char* const bundle)
    {
        CARLA_SAFE_ASSERT_RETURN(bundle != nullptr && bundle[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(needsInit,);

        needsInit = false;
        Lilv::World::load_bundle(Lilv::Node(new_uri(bundle)));

        allPlugins = lilv_world_get_all_plugins(this->me);
        CARLA_SAFE_ASSERT_RETURN(allPlugins != nullptr,);

        if ((pluginCount = lilv_plugins_size(allPlugins)))
        {
            cachedPlugins = new const LilvPlugin*[pluginCount+1];
            carla_zeroPointers(cachedPlugins, pluginCount+1);

            int i = 0;
            for (LilvIter* it = lilv_plugins_begin(allPlugins); ! lilv_plugins_is_end(allPlugins, it); it = lilv_plugins_next(allPlugins, it))
                cachedPlugins[i++] = lilv_plugins_get(allPlugins, it);
        }
    }

    uint getPluginCount() const
    {
        CARLA_SAFE_ASSERT_RETURN(! needsInit, 0);

        return pluginCount;
    }

    const LilvPlugin* getPluginFromIndex(const uint index) const
    {
        CARLA_SAFE_ASSERT_RETURN(! needsInit, nullptr);
        CARLA_SAFE_ASSERT_RETURN(cachedPlugins != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(index < pluginCount, nullptr);

        return cachedPlugins[index];
    }

    const LilvPlugin* getPluginFromURI(const LV2_URI uri) const
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', nullptr);
        CARLA_SAFE_ASSERT_RETURN(! needsInit, nullptr);
        CARLA_SAFE_ASSERT_RETURN(allPlugins != nullptr, nullptr);

        LilvNode* const uriNode(lilv_new_uri(this->me, uri));
        CARLA_SAFE_ASSERT_RETURN(uriNode != nullptr, nullptr);

        const LilvPlugin* const cPlugin(lilv_plugins_get_by_uri(allPlugins, uriNode));
        lilv_node_free(uriNode);

        return cPlugin;
    }

    LilvState* getStateFromURI(const LV2_URI uri, const LV2_URID_Map* const uridMap) const
    {
        CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', nullptr);
        CARLA_SAFE_ASSERT_RETURN(uridMap != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(! needsInit, nullptr);

        LilvNode* const uriNode(lilv_new_uri(this->me, uri));
        CARLA_SAFE_ASSERT_RETURN(uriNode != nullptr, nullptr);

        CARLA_SAFE_ASSERT(lilv_world_load_resource(this->me, uriNode) >= 0);

        LilvState* const cState(lilv_state_new_from_world(this->me, uridMap, uriNode));
        lilv_node_free(uriNode);

        return cState;
    }

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(Lv2WorldClass)
};

// --------------------------------------------------------------------------------------------------------------------
// Our LV2 Plugin base class

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Weffc++"
# pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
template<class TimeInfoStruct>
class Lv2PluginBaseClass : public LV2_External_UI_Widget_Compat
{
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif
public:
    Lv2PluginBaseClass(const double sampleRate, const LV2_Feature* const* const features)
        : fIsActive(false),
          fIsOffline(false),
          fUsingNominal(false),
          fBufferSize(0),
          fSampleRate(sampleRate),
          fFreePath(nullptr),
          fUridMap(nullptr),
          fUridUnmap(nullptr),
          fWorker(nullptr),
          fInlineDisplay(nullptr),
          fTimeInfo(),
          fLastPositionData(),
          fPorts(),
          fURIs(),
          fUI()
    {
        run  = extui_run;
        show = extui_show;
        hide = extui_hide;

        if (fSampleRate < 1.0)
        {
            carla_stderr("Host doesn't provide a valid sample rate");
            return;
        }

        const LV2_State_Free_Path* freePath  = nullptr;
        const LV2_Options_Option*  options   = nullptr;
        const LV2_URID_Map*        uridMap   = nullptr;
        const LV2_URID_Unmap*      uridUnmap = nullptr;
        const LV2_Worker_Schedule* worker    = nullptr;
        const LV2_Inline_Display*  idisp     = nullptr;

        for (int i=0; features[i] != nullptr; ++i)
        {
            /**/ if (std::strcmp(features[i]->URI, LV2_STATE__freePath) == 0)
                freePath = (const LV2_State_Free_Path*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
                options = (const LV2_Options_Option*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
                uridMap = (const LV2_URID_Map*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_URID__unmap) == 0)
                uridUnmap = (const LV2_URID_Unmap*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_WORKER__schedule) == 0)
                worker = (const LV2_Worker_Schedule*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_INLINEDISPLAY__queue_draw) == 0)
                idisp = (const LV2_Inline_Display*)features[i]->data;
        }

        if (options == nullptr || uridMap == nullptr)
        {
            carla_stderr("Host doesn't provide option and urid-map features");
            return;
        }

        for (int i=0; options[i].key != 0; ++i)
        {
            if (uridUnmap != nullptr) {
                carla_debug("Host option %i:\"%s\"", i, uridUnmap->unmap(uridUnmap->handle, options[i].key));
            }

            if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__nominalBlockLength))
            {
                if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Int))
                {
                    const int32_t value(*(const int32_t*)options[i].value);
                    CARLA_SAFE_ASSERT_CONTINUE(value > 0);

                    fBufferSize = static_cast<uint32_t>(value);
                    fUsingNominal = true;
                }
                else
                {
                    carla_stderr("Host provides nominalBlockLength but has wrong value type");
                }
                break;
            }

            if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__maxBlockLength))
            {
                if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Int))
                {
                    const int32_t value(*(const int32_t*)options[i].value);
                    CARLA_SAFE_ASSERT_CONTINUE(value > 0);

                    fBufferSize = static_cast<uint32_t>(value);
                }
                else
                {
                    carla_stderr("Host provides maxBlockLength but has wrong value type");
                }
                // no break, continue in case host supports nominalBlockLength
            }
        }

        if (fBufferSize == 0)
        {
            carla_stderr("Host doesn't provide buffer-size feature");
            //return;
            // as testing, continue for now
            fBufferSize = 1024;
        }

        fUridMap = uridMap;
        fURIs.map(uridMap);

        fFreePath = freePath;
        fUridUnmap = uridUnmap;
        fWorker = worker;
        fInlineDisplay = idisp;

        clearTimeData();
    }

    virtual ~Lv2PluginBaseClass() {}

    bool loadedInProperHost() const noexcept
    {
        return fUridMap != nullptr && fBufferSize != 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2_connect_port(const uint32_t port, void* const dataLocation) noexcept
    {
        fPorts.connectPort(port, dataLocation);
    }

    bool lv2_pre_run(const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(fIsActive, false);

        fIsOffline = (fPorts.freewheel != nullptr && *fPorts.freewheel >= 0.5f);

        // cache midi events and time information first
        if (fPorts.usesTime)
        {
            LV2_ATOM_SEQUENCE_FOREACH(fPorts.eventsIn[0], event)
            {
                if (event == nullptr)
                    continue;
                if (event->body.type != fURIs.atomBlank && event->body.type != fURIs.atomObject)
                    continue;

                const LV2_Atom_Object* const obj((const LV2_Atom_Object*)&event->body);

                if (obj->body.otype != fURIs.timePos)
                    continue;

                LV2_Atom* bar = nullptr;
                LV2_Atom* barBeat = nullptr;
                LV2_Atom* beatUnit = nullptr;
                LV2_Atom* beatsPerBar = nullptr;
                LV2_Atom* beatsPerMinute = nullptr;
                LV2_Atom* frame = nullptr;
                LV2_Atom* speed = nullptr;
                LV2_Atom* ticksPerBeat = nullptr;

                lv2_atom_object_get(obj,
                                    fURIs.timeBar, &bar,
                                    fURIs.timeBarBeat, &barBeat,
                                    fURIs.timeBeatUnit, &beatUnit,
                                    fURIs.timeBeatsPerBar, &beatsPerBar,
                                    fURIs.timeBeatsPerMinute, &beatsPerMinute,
                                    fURIs.timeFrame, &frame,
                                    fURIs.timeSpeed, &speed,
                                    fURIs.timeTicksPerBeat, &ticksPerBeat,
                                    0);

                // need to handle this first as other values depend on it
                if (ticksPerBeat != nullptr)
                {
                    double ticksPerBeatValue = -1.0;

                    /**/ if (ticksPerBeat->type == fURIs.atomDouble)
                        ticksPerBeatValue = ((LV2_Atom_Double*)ticksPerBeat)->body;
                    else if (ticksPerBeat->type == fURIs.atomFloat)
                        ticksPerBeatValue = ((LV2_Atom_Float*)ticksPerBeat)->body;
                    else if (ticksPerBeat->type == fURIs.atomInt)
                        ticksPerBeatValue = static_cast<double>(((LV2_Atom_Int*)ticksPerBeat)->body);
                    else if (ticksPerBeat->type == fURIs.atomLong)
                        ticksPerBeatValue = static_cast<double>(((LV2_Atom_Long*)ticksPerBeat)->body);
                    else
                        carla_stderr("Unknown lv2 ticksPerBeat value type");

                    if (ticksPerBeatValue > 0.0)
                        fTimeInfo.bbt.ticksPerBeat = fLastPositionData.ticksPerBeat = ticksPerBeatValue;
                    else
                        carla_stderr("Invalid lv2 ticksPerBeat value");
                }

                // same
                if (speed != nullptr)
                {
                    /**/ if (speed->type == fURIs.atomDouble)
                        fLastPositionData.speed = ((LV2_Atom_Double*)speed)->body;
                    else if (speed->type == fURIs.atomFloat)
                        fLastPositionData.speed = ((LV2_Atom_Float*)speed)->body;
                    else if (speed->type == fURIs.atomInt)
                        fLastPositionData.speed = static_cast<double>(((LV2_Atom_Int*)speed)->body);
                    else if (speed->type == fURIs.atomLong)
                        fLastPositionData.speed = static_cast<double>(((LV2_Atom_Long*)speed)->body);
                    else
                        carla_stderr("Unknown lv2 speed value type");

                    fTimeInfo.playing = carla_isNotZero(fLastPositionData.speed);

                    if (fTimeInfo.playing && fLastPositionData.beatsPerMinute > 0.0)
                    {
                        fTimeInfo.bbt.beatsPerMinute = fLastPositionData.beatsPerMinute*
                                                        std::abs(fLastPositionData.speed);
                    }
                }

                if (bar != nullptr)
                {
                    int64_t barValue = -1;

                    /**/ if (bar->type == fURIs.atomDouble)
                        barValue = static_cast<int64_t>(((LV2_Atom_Double*)bar)->body);
                    else if (bar->type == fURIs.atomFloat)
                        barValue = static_cast<int64_t>(((LV2_Atom_Float*)bar)->body);
                    else if (bar->type == fURIs.atomInt)
                        barValue = ((LV2_Atom_Int*)bar)->body;
                    else if (bar->type == fURIs.atomLong)
                        barValue = ((LV2_Atom_Long*)bar)->body;
                    else
                        carla_stderr("Unknown lv2 bar value type");

                    if (barValue >= 0 && barValue < INT32_MAX)
                    {
                        fLastPositionData.bar   = static_cast<int32_t>(barValue);
                        fLastPositionData.bar_f = static_cast<float>(barValue);
                        fTimeInfo.bbt.bar = fLastPositionData.bar + 1;
                    }
                    else
                    {
                        carla_stderr("Invalid lv2 bar value");
                    }
                }

                if (barBeat != nullptr)
                {
                    double barBeatValue = -1.0;

                    /**/ if (barBeat->type == fURIs.atomDouble)
                        barBeatValue = ((LV2_Atom_Double*)barBeat)->body;
                    else if (barBeat->type == fURIs.atomFloat)
                        barBeatValue = ((LV2_Atom_Float*)barBeat)->body;
                    else if (barBeat->type == fURIs.atomInt)
                        barBeatValue = static_cast<float>(((LV2_Atom_Int*)barBeat)->body);
                    else if (barBeat->type == fURIs.atomLong)
                        barBeatValue = static_cast<float>(((LV2_Atom_Long*)barBeat)->body);
                    else
                        carla_stderr("Unknown lv2 barBeat value type");

                    if (barBeatValue >= 0.0)
                    {
                        fLastPositionData.barBeat = static_cast<float>(barBeatValue);

                        const double rest  = std::fmod(barBeatValue, 1.0);
                        fTimeInfo.bbt.beat = static_cast<int32_t>(barBeatValue-rest+1.0);
                        fTimeInfo.bbt.tick = static_cast<int32_t>(rest*fTimeInfo.bbt.ticksPerBeat+0.5);
                    }
                    else
                    {
                        carla_stderr("Invalid lv2 barBeat value");
                    }
                }

                if (beatUnit != nullptr)
                {
                    int64_t beatUnitValue = -1;

                    /**/ if (beatUnit->type == fURIs.atomDouble)
                        beatUnitValue = static_cast<int64_t>(((LV2_Atom_Double*)beatUnit)->body);
                    else if (beatUnit->type == fURIs.atomFloat)
                        beatUnitValue = static_cast<int64_t>(((LV2_Atom_Float*)beatUnit)->body);
                    else if (beatUnit->type == fURIs.atomInt)
                        beatUnitValue = ((LV2_Atom_Int*)beatUnit)->body;
                    else if (beatUnit->type == fURIs.atomLong)
                        beatUnitValue = ((LV2_Atom_Long*)beatUnit)->body;
                    else
                        carla_stderr("Unknown lv2 beatUnit value type");

                    if (beatUnitValue > 0 && beatUnitValue < UINT32_MAX)
                    {
                        fLastPositionData.beatUnit = static_cast<uint32_t>(beatUnitValue);
                        fTimeInfo.bbt.beatType     = static_cast<float>(beatUnitValue);
                    }
                    else
                    {
                        carla_stderr("Invalid lv2 beatUnit value");
                    }
                }

                if (beatsPerBar != nullptr)
                {
                    float beatsPerBarValue = -1.0f;

                    /**/ if (beatsPerBar->type == fURIs.atomDouble)
                        beatsPerBarValue = static_cast<float>(((LV2_Atom_Double*)beatsPerBar)->body);
                    else if (beatsPerBar->type == fURIs.atomFloat)
                        beatsPerBarValue = ((LV2_Atom_Float*)beatsPerBar)->body;
                    else if (beatsPerBar->type == fURIs.atomInt)
                        beatsPerBarValue = static_cast<float>(((LV2_Atom_Int*)beatsPerBar)->body);
                    else if (beatsPerBar->type == fURIs.atomLong)
                        beatsPerBarValue = static_cast<float>(((LV2_Atom_Long*)beatsPerBar)->body);
                    else
                        carla_stderr("Unknown lv2 beatsPerBar value type");

                    if (beatsPerBarValue > 0.0f)
                        fTimeInfo.bbt.beatsPerBar = fLastPositionData.beatsPerBar = beatsPerBarValue;
                    else
                        carla_stderr("Invalid lv2 beatsPerBar value");
                }

                if (beatsPerMinute != nullptr)
                {
                    double beatsPerMinuteValue = -1.0;

                    /**/ if (beatsPerMinute->type == fURIs.atomDouble)
                        beatsPerMinuteValue = ((LV2_Atom_Double*)beatsPerMinute)->body;
                    else if (beatsPerMinute->type == fURIs.atomFloat)
                        beatsPerMinuteValue = ((LV2_Atom_Float*)beatsPerMinute)->body;
                    else if (beatsPerMinute->type == fURIs.atomInt)
                        beatsPerMinuteValue = static_cast<double>(((LV2_Atom_Int*)beatsPerMinute)->body);
                    else if (beatsPerMinute->type == fURIs.atomLong)
                        beatsPerMinuteValue = static_cast<double>(((LV2_Atom_Long*)beatsPerMinute)->body);
                    else
                        carla_stderr("Unknown lv2 beatsPerMinute value type");

                    if (beatsPerMinuteValue >= 12.0 && beatsPerMinuteValue <= 999.0)
                    {
                        fTimeInfo.bbt.beatsPerMinute = fLastPositionData.beatsPerMinute = beatsPerMinuteValue;

                        if (carla_isNotZero(fLastPositionData.speed))
                            fTimeInfo.bbt.beatsPerMinute *= std::abs(fLastPositionData.speed);
                    }
                    else
                    {
                        carla_stderr("Invalid lv2 beatsPerMinute value");
                    }
                }

                if (frame != nullptr)
                {
                    int64_t frameValue = -1;

                    /**/ if (frame->type == fURIs.atomDouble)
                        frameValue = static_cast<int64_t>(((LV2_Atom_Double*)frame)->body);
                    else if (frame->type == fURIs.atomFloat)
                        frameValue = static_cast<int64_t>(((LV2_Atom_Float*)frame)->body);
                    else if (frame->type == fURIs.atomInt)
                        frameValue = ((LV2_Atom_Int*)frame)->body;
                    else if (frame->type == fURIs.atomLong)
                        frameValue = ((LV2_Atom_Long*)frame)->body;
                    else
                        carla_stderr("Unknown lv2 frame value type");

                    if (frameValue >= 0)
                        fTimeInfo.frame = fLastPositionData.frame = static_cast<uint64_t>(frameValue);
                    else
                        carla_stderr("Invalid lv2 frame value");
                }

                fTimeInfo.bbt.barStartTick = static_cast<double>(fTimeInfo.bbt.ticksPerBeat) *
                                             static_cast<double>(fTimeInfo.bbt.beatsPerBar) *
                                             (fTimeInfo.bbt.bar-1);

                fTimeInfo.bbt.valid = (fLastPositionData.beatsPerMinute > 0.0 &&
                                       fLastPositionData.beatUnit > 0 &&
                                       fLastPositionData.beatsPerBar > 0.0f);
            }
        }

        // Check for updated parameters
        float curValue;

        for (uint32_t i=0; i < fPorts.numParams; ++i)
        {
            if (fPorts.paramsOut[i])
                continue;

            CARLA_SAFE_ASSERT_CONTINUE(fPorts.paramsPtr[i] != nullptr)

            curValue = *fPorts.paramsPtr[i];

            if (carla_isEqual(fPorts.paramsLast[i], curValue))
                continue;

            fPorts.paramsLast[i] = curValue;
            handleParameterValueChanged(i, curValue);
        }

        // init event out data
        if (fPorts.numMidiOuts > 0 || fPorts.hasUI)
        {
            const uint32_t count = fPorts.numMidiOuts > 0 ? fPorts.numMidiOuts : 1;

            for (uint32_t i=0; i < count; ++i)
            {
                LV2_Atom_Sequence* const seq(fPorts.eventsOut[i]);
                CARLA_SAFE_ASSERT_CONTINUE(seq != nullptr);

                fPorts.eventsOutData[i].capacity = seq->atom.size;
                fPorts.eventsOutData[i].offset   = 0;

                seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
                seq->atom.type = fURIs.atomSequence;
                seq->body.unit = 0;
                seq->body.pad  = 0;
            }
        }

        if (frames == 0)
            return false;

        return true;
    }

    void lv2_post_run(const uint32_t frames)
    {
        // update timePos for next callback

        if (carla_isZero(fLastPositionData.speed))
            return;

        if (fLastPositionData.speed > 0.0)
        {
            // playing forwards
            fLastPositionData.frame += frames;
        }
        else
        {
            // playing backwards
            if (frames >= fLastPositionData.frame)
                fLastPositionData.frame = 0;
            else
                fLastPositionData.frame -= frames;
        }

        fTimeInfo.frame = fLastPositionData.frame;

        if (fTimeInfo.bbt.valid)
        {
            const double beatsPerMinute = fLastPositionData.beatsPerMinute * fLastPositionData.speed;
            const double framesPerBeat  = 60.0 * fSampleRate / beatsPerMinute;
            const double addedBarBeats  = double(frames) / framesPerBeat;

            if (fLastPositionData.barBeat >= 0.0f)
            {
                fLastPositionData.barBeat = std::fmod(fLastPositionData.barBeat+static_cast<float>(addedBarBeats),
                                                      fLastPositionData.beatsPerBar);

                const double rest  = std::fmod(fLastPositionData.barBeat, 1.0f);
                fTimeInfo.bbt.beat = static_cast<int32_t>(static_cast<double>(fLastPositionData.barBeat)-rest+1.0);
                fTimeInfo.bbt.tick = rest * fTimeInfo.bbt.ticksPerBeat;

                if (fLastPositionData.bar_f >= 0.0f)
                {
                    fLastPositionData.bar_f += std::floor((fLastPositionData.barBeat+static_cast<float>(addedBarBeats))/
                                                           fLastPositionData.beatsPerBar);

                    if (fLastPositionData.bar_f <= 0.0f)
                    {
                        fLastPositionData.bar   = 0;
                        fLastPositionData.bar_f = 0.0f;
                    }
                    else
                    {
                        fLastPositionData.bar = static_cast<int32_t>(fLastPositionData.bar_f+0.5f);
                    }

                    fTimeInfo.bbt.bar = fLastPositionData.bar + 1;

                    fTimeInfo.bbt.barStartTick = static_cast<double>(fTimeInfo.bbt.ticksPerBeat) *
                                                 static_cast<double>(fTimeInfo.bbt.beatsPerBar) *
                                                 (fTimeInfo.bbt.bar-1);
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    uint32_t lv2_get_options(LV2_Options_Option* const /*options*/) const
    {
        // currently unused
        return LV2_OPTIONS_SUCCESS;
    }

    uint32_t lv2_set_options(const LV2_Options_Option* const options)
    {
        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == fUridMap->map(fUridMap->handle, LV2_BUF_SIZE__nominalBlockLength))
            {
                if (options[i].type == fURIs.atomInt)
                {
                    const int32_t value(*(const int32_t*)options[i].value);
                    CARLA_SAFE_ASSERT_CONTINUE(value > 0);

                    const uint32_t newBufferSize = static_cast<uint32_t>(value);

                    if (fBufferSize != newBufferSize)
                    {
                        fBufferSize = newBufferSize;
                        handleBufferSizeChanged(newBufferSize);
                    }
                }
                else
                {
                    carla_stderr("Host changed nominalBlockLength but with wrong value type");
                }
            }
            else if (options[i].key == fUridMap->map(fUridMap->handle, LV2_BUF_SIZE__maxBlockLength) && ! fUsingNominal)
            {
                if (options[i].type == fURIs.atomInt)
                {
                    const int32_t value(*(const int32_t*)options[i].value);
                    CARLA_SAFE_ASSERT_CONTINUE(value > 0);

                    const uint32_t newBufferSize = static_cast<uint32_t>(value);

                    if (fBufferSize != newBufferSize)
                    {
                        fBufferSize = newBufferSize;
                        handleBufferSizeChanged(newBufferSize);
                    }
                }
                else
                {
                    carla_stderr("Host changed maxBlockLength but with wrong value type");
                }
            }
            else if (options[i].key == fUridMap->map(fUridMap->handle, LV2_PARAMETERS__sampleRate))
            {
                if (options[i].type == fURIs.atomFloat)
                {
                    const double value(*(const float*)options[i].value);
                    CARLA_SAFE_ASSERT_CONTINUE(value > 0.0);

                    if (carla_isNotEqual(fSampleRate, value))
                    {
                        fSampleRate = value;
                        handleSampleRateChanged(value);
                    }
                }
                else
                {
                    carla_stderr("Host changed sampleRate but with wrong value type");
                }
            }
        }

        return LV2_OPTIONS_SUCCESS;
    }

    // ----------------------------------------------------------------------------------------------------------------

    int lv2ui_idle() const
    {
        if (! fUI.isVisible)
            return 1;

        handleUiRun();
        return 0;
    }

    int lv2ui_show()
    {
        handleUiShow();
        return 0;
    }

    int lv2ui_hide()
    {
        handleUiHide();
        return 0;
    }

    void lv2ui_cleanup()
    {
        if (fUI.isVisible)
            handleUiHide();

        fUI.host = nullptr;
        fUI.touch = nullptr;
        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    virtual void handleUiRun() const = 0;
    virtual void handleUiShow() = 0;
    virtual void handleUiHide() = 0;

    virtual void handleParameterValueChanged(const uint32_t index, const float value) = 0;
    virtual void handleBufferSizeChanged(const uint32_t bufferSize) = 0;
    virtual void handleSampleRateChanged(const double sampleRate) = 0;

    void resetTimeInfo() noexcept
    {
        clearTimeData();

        // hosts may not send all values, resulting on some invalid data
        fTimeInfo.bbt.bar   = 1;
        fTimeInfo.bbt.beat  = 1;
        fTimeInfo.bbt.beatsPerBar    = 4;
        fTimeInfo.bbt.beatType       = 4;
        fTimeInfo.bbt.ticksPerBeat   = fLastPositionData.ticksPerBeat   = 960.0;
        fTimeInfo.bbt.beatsPerMinute = fLastPositionData.beatsPerMinute = 120.0;
    }

    // LV2 host data
    bool     fIsActive : 1;
    bool     fIsOffline : 1;
    bool     fUsingNominal : 1;
    uint32_t fBufferSize;
    double   fSampleRate;

    // LV2 host features
    const LV2_State_Free_Path* fFreePath;
    const LV2_URID_Map* fUridMap;
    const LV2_URID_Unmap* fUridUnmap;
    const LV2_Worker_Schedule* fWorker;
    const LV2_Inline_Display* fInlineDisplay;

    // Time info stuff
    TimeInfoStruct fTimeInfo;

    struct Lv2PositionData {
        int32_t  bar;
        float    bar_f;
        float    barBeat;
        uint32_t beatUnit;
        float    beatsPerBar;
        double   beatsPerMinute;
        uint64_t frame;
        double   speed;
        double   ticksPerBeat;

        Lv2PositionData()
            : bar(-1),
              bar_f(-1.0f),
              barBeat(-1.0f),
              beatUnit(0),
              beatsPerBar(0.0f),
              beatsPerMinute(-1.0),
              frame(0),
              speed(0.0),
              ticksPerBeat(-1.0) {}

        void clear()
        {
            bar = -1;
            bar_f = -1.0f;
            barBeat = -1.0f;
            beatUnit = 0;
            beatsPerBar = 0.0f;
            beatsPerMinute = -1.0;
            frame = 0;
            speed = 0.0;
            ticksPerBeat = -1.0;
        }

    } fLastPositionData;

    // Port stuff
    struct Ports {
        // need to save current state
        struct EventsOutData {
            uint32_t capacity;
            uint32_t offset;

            EventsOutData()
                : capacity(0),
                  offset(0) {}
        };

        // store port info
        uint32_t indexOffset;
        uint32_t numAudioIns;
        uint32_t numAudioOuts;
        uint32_t numCVIns;
        uint32_t numCVOuts;
        uint32_t numMidiIns;
        uint32_t numMidiOuts;
        uint32_t numParams;
        bool hasUI;
        bool usesTime;

        // port buffers
        const LV2_Atom_Sequence** eventsIn;
        /* */ LV2_Atom_Sequence** eventsOut;
        /* */ EventsOutData*      eventsOutData;
        /* */ float** audioCVIns;
        /* */ float** audioCVOuts;
        /* */ float*  freewheel;

        // cached parameter values
        float*  paramsLast;
        float** paramsPtr;
        bool*   paramsOut;

        Ports()
            : indexOffset(0),
              numAudioIns(0),
              numAudioOuts(0),
              numCVIns(0),
              numCVOuts(0),
              numMidiIns(0),
              numMidiOuts(0),
              numParams(0),
              hasUI(false),
              usesTime(false),
              eventsIn(nullptr),
              eventsOut(nullptr),
              eventsOutData(nullptr),
              audioCVIns(nullptr),
              audioCVOuts(nullptr),
              freewheel(nullptr),
              paramsLast(nullptr),
              paramsPtr(nullptr),
              paramsOut(nullptr) {}

        ~Ports()
        {
            if (eventsIn != nullptr)
            {
                delete[] eventsIn;
                eventsIn = nullptr;
            }

            if (eventsOut != nullptr)
            {
                delete[] eventsOut;
                eventsOut = nullptr;
            }

            if (eventsOutData != nullptr)
            {
                delete[] eventsOutData;
                eventsOutData = nullptr;
            }

            if (audioCVIns != nullptr)
            {
                delete[] audioCVIns;
                audioCVIns = nullptr;
            }

            if (audioCVOuts != nullptr)
            {
                delete[] audioCVOuts;
                audioCVOuts = nullptr;
            }

            if (paramsLast != nullptr)
            {
                delete[] paramsLast;
                paramsLast = nullptr;
            }

            if (paramsPtr != nullptr)
            {
                delete[] paramsPtr;
                paramsPtr = nullptr;
            }

            if (paramsOut != nullptr)
            {
                delete[] paramsOut;
                paramsOut = nullptr;
            }
        }

        // NOTE: assumes num* has been filled by parent class
        void init()
        {
            if (numMidiIns > 0)
            {
                eventsIn = new const LV2_Atom_Sequence*[numMidiIns];

                for (uint32_t i=0; i < numMidiIns; ++i)
                    eventsIn[i] = nullptr;
            }
            else if (usesTime || hasUI)
            {
                eventsIn = new const LV2_Atom_Sequence*[1];
                eventsIn[0] = nullptr;
            }

            if (numMidiOuts > 0)
            {
                eventsOut = new LV2_Atom_Sequence*[numMidiOuts];
                eventsOutData = new EventsOutData[numMidiOuts];

                for (uint32_t i=0; i < numMidiOuts; ++i)
                    eventsOut[i] = nullptr;
            }
            else if (hasUI)
            {
                eventsOut = new LV2_Atom_Sequence*[1];
                eventsOut[0] = nullptr;
                eventsOutData = new EventsOutData[1];
            }

            if (const uint32_t numAudioCVIns = numAudioIns+numCVIns)
            {
                audioCVIns = new float*[numAudioCVIns];
                carla_zeroPointers(audioCVIns, numAudioCVIns);
            }

            if (const uint32_t numAudioCVOuts = numAudioOuts+numCVOuts)
            {
                audioCVOuts = new float*[numAudioCVOuts];
                carla_zeroPointers(audioCVOuts, numAudioCVOuts);
            }

            if (numParams > 0)
            {
                paramsLast = new float[numParams];
                paramsPtr  = new float*[numParams];
                paramsOut  = new bool[numParams];

                carla_zeroFloats(paramsLast, numParams);
                carla_zeroPointers(paramsPtr, numParams);
                carla_zeroStructs(paramsOut, numParams);

                // NOTE: need to be filled in by the parent class
            }

            indexOffset  = numAudioIns + numAudioOuts + numCVIns + numCVOuts;
            // 1 event port for time or ui if no midi input is used
            indexOffset += numMidiIns > 0 ? numMidiIns : ((usesTime || hasUI) ? 1 : 0);
            // 1 event port for ui if no midi output is used
            indexOffset += numMidiOuts > 0 ? numMidiOuts : (hasUI ? 1 : 0);
            // 1 extra for freewheel port
            indexOffset += 1;
        }

        void connectPort(const uint32_t port, void* const dataLocation)
        {
            uint32_t index = 0;

            if (numMidiIns > 0 || usesTime || hasUI)
            {
                if (port == index++)
                {
                    eventsIn[0] = (LV2_Atom_Sequence*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=1; i < numMidiIns; ++i)
            {
                if (port == index++)
                {
                    eventsIn[i] = (LV2_Atom_Sequence*)dataLocation;
                    return;
                }
            }

            if (numMidiOuts > 0 || hasUI)
            {
                if (port == index++)
                {
                    eventsOut[0] = (LV2_Atom_Sequence*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=1; i < numMidiOuts; ++i)
            {
                if (port == index++)
                {
                    eventsOut[i] = (LV2_Atom_Sequence*)dataLocation;
                    return;
                }
            }

            if (port == index++)
            {
                freewheel = (float*)dataLocation;
                return;
            }

            for (uint32_t i=0; i < numAudioIns; ++i)
            {
                if (port == index++)
                {
                    audioCVIns[i] = (float*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < numAudioOuts; ++i)
            {
                if (port == index++)
                {
                    audioCVOuts[i] = (float*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < numCVIns; ++i)
            {
                if (port == index++)
                {
                    audioCVIns[numAudioIns+i] = (float*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < numCVOuts; ++i)
            {
                if (port == index++)
                {
                    audioCVOuts[numAudioOuts+i] = (float*)dataLocation;
                    return;
                }
            }

            for (uint32_t i=0; i < numParams; ++i)
            {
                if (port == index++)
                {
                    paramsPtr[i] = (float*)dataLocation;
                    return;
                }
            }
        }

        CARLA_DECLARE_NON_COPYABLE(Ports);
    } fPorts;

    // Rest of host<->plugin support
    struct URIDs {
        LV2_URID atomBlank;
        LV2_URID atomBool;
        LV2_URID atomObject;
        LV2_URID atomDouble;
        LV2_URID atomFloat;
        LV2_URID atomInt;
        LV2_URID atomLong;
        LV2_URID atomPath;
        LV2_URID atomSequence;
        LV2_URID atomString;
        LV2_URID atomURID;
        LV2_URID carlaFile;
        LV2_URID carlaFileAudio;
        LV2_URID carlaFileMIDI;
        LV2_URID carlaPreview;
        LV2_URID midiEvent;
        LV2_URID patchProperty;
        LV2_URID patchGet;
        LV2_URID patchSet;
        LV2_URID patchValue;
        LV2_URID timePos;
        LV2_URID timeBar;
        LV2_URID timeBarBeat;
        LV2_URID timeBeatsPerBar;
        LV2_URID timeBeatsPerMinute;
        LV2_URID timeBeatUnit;
        LV2_URID timeFrame;
        LV2_URID timeSpeed;
        LV2_URID timeTicksPerBeat;
        LV2_URID carlaRequestIdle;
        LV2_URID carlaUiEvents;

        URIDs()
            : atomBlank(0),
              atomBool(0),
              atomObject(0),
              atomDouble(0),
              atomFloat(0),
              atomInt(0),
              atomLong(0),
              atomPath(0),
              atomSequence(0),
              atomString(0),
              atomURID(0),
              carlaFile(0),
              carlaFileAudio(0),
              carlaFileMIDI(0),
              carlaPreview(0),
              midiEvent(0),
              patchProperty(0),
              patchGet(0),
              patchSet(0),
              patchValue(0),
              timePos(0),
              timeBar(0),
              timeBarBeat(0),
              timeBeatsPerBar(0),
              timeBeatsPerMinute(0),
              timeBeatUnit(0),
              timeFrame(0),
              timeSpeed(0),
              timeTicksPerBeat(0),
              carlaRequestIdle(0),
              carlaUiEvents(0) {}

        void map(const LV2_URID_Map* const uridMap)
        {
            atomBlank          = uridMap->map(uridMap->handle, LV2_ATOM__Blank);
            atomBool           = uridMap->map(uridMap->handle, LV2_ATOM__Bool);
            atomObject         = uridMap->map(uridMap->handle, LV2_ATOM__Object);
            atomDouble         = uridMap->map(uridMap->handle, LV2_ATOM__Double);
            atomFloat          = uridMap->map(uridMap->handle, LV2_ATOM__Float);
            atomInt            = uridMap->map(uridMap->handle, LV2_ATOM__Int);
            atomLong           = uridMap->map(uridMap->handle, LV2_ATOM__Long);
            atomPath           = uridMap->map(uridMap->handle, LV2_ATOM__Path);
            atomSequence       = uridMap->map(uridMap->handle, LV2_ATOM__Sequence);
            atomString         = uridMap->map(uridMap->handle, LV2_ATOM__String);
            atomURID           = uridMap->map(uridMap->handle, LV2_ATOM__URID);
            carlaFile          = uridMap->map(uridMap->handle, "http://kxstudio.sf.net/carla/file");
            carlaFileAudio     = uridMap->map(uridMap->handle, "http://kxstudio.sf.net/carla/file/audio");
            carlaFileMIDI      = uridMap->map(uridMap->handle, "http://kxstudio.sf.net/carla/file/midi");
            carlaPreview       = uridMap->map(uridMap->handle, "http://kxstudio.sf.net/carla/preview");
            midiEvent          = uridMap->map(uridMap->handle, LV2_MIDI__MidiEvent);
            patchProperty      = uridMap->map(uridMap->handle, LV2_PATCH__property);
            patchGet           = uridMap->map(uridMap->handle, LV2_PATCH__Get);
            patchSet           = uridMap->map(uridMap->handle, LV2_PATCH__Set);
            patchValue         = uridMap->map(uridMap->handle, LV2_PATCH__value);
            timePos            = uridMap->map(uridMap->handle, LV2_TIME__Position);
            timeBar            = uridMap->map(uridMap->handle, LV2_TIME__bar);
            timeBarBeat        = uridMap->map(uridMap->handle, LV2_TIME__barBeat);
            timeBeatUnit       = uridMap->map(uridMap->handle, LV2_TIME__beatUnit);
            timeFrame          = uridMap->map(uridMap->handle, LV2_TIME__frame);
            timeSpeed          = uridMap->map(uridMap->handle, LV2_TIME__speed);
            timeBeatsPerBar    = uridMap->map(uridMap->handle, LV2_TIME__beatsPerBar);
            timeBeatsPerMinute = uridMap->map(uridMap->handle, LV2_TIME__beatsPerMinute);
            timeTicksPerBeat   = uridMap->map(uridMap->handle, LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat);
            carlaRequestIdle   = uridMap->map(uridMap->handle, "urn:carla:idle");
            carlaUiEvents      = uridMap->map(uridMap->handle, "urn:carla:uiEvents");
        }
    } fURIs;

    struct UI {
        const LV2_External_UI_Host* host;
        const LV2UI_Touch* touch;
        LV2UI_Write_Function writeFunction;
        LV2UI_Controller controller;
        bool isVisible;

        UI()
            : host(nullptr),
              touch(nullptr),
              writeFunction(nullptr),
              controller(nullptr),
              isVisible(false) {}
    } fUI;

private:
    // ----------------------------------------------------------------------------------------------------------------

    #define handlePtr ((Lv2PluginBaseClass*)handle)

    static void extui_run(LV2_External_UI_Widget_Compat* handle)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

        handlePtr->handleUiRun();
    }

    static void extui_show(LV2_External_UI_Widget_Compat* handle)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        carla_debug("extui_show(%p)", handle);

        handlePtr->handleUiShow();
    }

    static void extui_hide(LV2_External_UI_Widget_Compat* handle)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
        carla_debug("extui_hide(%p)", handle);

        handlePtr->handleUiHide();
    }

    #undef handlePtr

    // ----------------------------------------------------------------------------------------------------------------

    void clearTimeData() noexcept;

    // ----------------------------------------------------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE(Lv2PluginBaseClass)
};

// --------------------------------------------------------------------------------------------------------------------
// Create new RDF object (using lilv)

static inline
const LV2_RDF_Descriptor* lv2_rdf_new(const LV2_URI uri, const bool loadPresets)
{
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', nullptr);

    Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

    const LilvPlugin* const cPlugin(lv2World.getPluginFromURI(uri));
    CARLA_SAFE_ASSERT_RETURN(cPlugin != nullptr, nullptr);

    Lilv::Plugin lilvPlugin(cPlugin);
    LV2_RDF_Descriptor* const rdfDescriptor(new LV2_RDF_Descriptor());

    CarlaStringList portGroupURIs(false); // does not allocate own elements
    LinkedList<LilvNode*> portGroupNodes;

    // ----------------------------------------------------------------------------------------------------------------
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

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin Information
    {
        rdfDescriptor->URI = carla_strdup(uri);

        if (LilvNode* const nameNode = lilv_plugin_get_name(lilvPlugin.me))
        {
            if (const char* const name = lilv_node_as_string(nameNode))
                rdfDescriptor->Name = carla_strdup(name);
            lilv_node_free(nameNode);
        }

        if (const char* const author = lilvPlugin.get_author_name().as_string())
            rdfDescriptor->Author = carla_strdup(author);

        if (const char* const binary = lilvPlugin.get_library_uri().as_string())
            rdfDescriptor->Binary = carla_strdup_free(lilv_file_uri_parse(binary, nullptr));

        if (const char* const bundle = lilvPlugin.get_bundle_uri().as_string())
            rdfDescriptor->Bundle = carla_strdup_free(lilv_file_uri_parse(bundle, nullptr));

        Lilv::Nodes licenseNodes(lilvPlugin.get_value(lv2World.doap_license));

        if (licenseNodes.size() > 0)
        {
            if (const char* const license = licenseNodes.get_first().as_string())
                rdfDescriptor->License = carla_strdup(license);
        }

        lilv_nodes_free(const_cast<LilvNodes*>(licenseNodes.me));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin UniqueID
    {
        Lilv::Nodes replaceNodes(lilvPlugin.get_value(lv2World.dct_replaces));

        if (replaceNodes.size() > 0)
        {
            Lilv::Node replaceNode(replaceNodes.get_first());

            if (replaceNode.is_uri())
            {
               #if defined(CARLA_UTILS_USE_QT)
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
                const water::String replaceURI(replaceNode.as_uri());

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

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin Ports

    if (const uint numPorts = lilvPlugin.get_num_ports())
    {
        rdfDescriptor->PortCount = numPorts;
        rdfDescriptor->Ports = new LV2_RDF_Port[numPorts];

        for (uint i = 0; i < numPorts; ++i)
        {
            Lilv::Port lilvPort(lilvPlugin.get_port_by_index(i));
            CARLA_SAFE_ASSERT_CONTINUE(lilvPort.me != nullptr);

            LV2_RDF_Port* const rdfPort(&rdfDescriptor->Ports[i]);

            // --------------------------------------------------------------------------------------------------------
            // Set Port Information
            {
                if (LilvNode* const nameNode = lilv_port_get_name(lilvPlugin.me, lilvPort.me))
                {
                    if (const char* const name = lilv_node_as_string(nameNode))
                        rdfPort->Name = carla_strdup(name);
                    lilv_node_free(nameNode);
                }

                if (const char* const symbol = lilv_node_as_string(lilvPort.get_symbol()))
                    rdfPort->Symbol = carla_strdup(symbol);

                if (LilvNode* const commentNode = lilvPort.get(lv2World.rdfs_comment.me))
                {
                    rdfPort->Comment = carla_strdup(lilv_node_as_string(commentNode));
                    lilv_node_free(commentNode);
                }

                if (LilvNode* const groupNode = lilvPort.get(lv2World.pg_group.me))
                {
                    rdfPort->GroupURI = carla_strdup(lilv_node_as_uri(groupNode));

                    if (portGroupURIs.appendUnique(rdfPort->GroupURI))
                        portGroupNodes.append(groupNode);
                    else
                        lilv_node_free(groupNode);
                }
            }

            // --------------------------------------------------------------------------------------------------------
            // Set Port Mode and Type
            {
                // Input or Output
                /**/ if (lilvPort.is_a(lv2World.port_input))
                    rdfPort->Types |= LV2_PORT_INPUT;
                else if (lilvPort.is_a(lv2World.port_output))
                    rdfPort->Types |= LV2_PORT_OUTPUT;
                else
                    carla_stderr("lv2_rdf_new(\"%s\") - port '%s' is not input or output", uri, rdfPort->Name);

                // Data Type
                /**/ if (lilvPort.is_a(lv2World.port_control))
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

                        /**/ if (node.equals(lv2World.midi_event))
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
                    carla_stderr("lv2_rdf_new(\"%s\") - port '%s' is of unknown data type", uri, rdfPort->Name);
            }

            // --------------------------------------------------------------------------------------------------------
            // Set Port Properties
            {
                if (lilvPort.has_property(lv2World.pprop_optional))
                    rdfPort->Properties |= LV2_PORT_OPTIONAL;
                if (lilvPort.has_property(lv2World.pprop_enumeration))
                    rdfPort->Properties |= LV2_PORT_ENUMERATION;
                if (lilvPort.has_property(lv2World.pprop_integer))
                    rdfPort->Properties |= LV2_PORT_INTEGER;
                if (lilvPort.has_property(lv2World.pprop_isSideChain))
                    rdfPort->Properties |= LV2_PORT_SIDECHAIN;
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
                if (lilvPort.has_property(lv2World.pprop_nonAutomatable))
                    rdfPort->Properties |= LV2_PORT_NON_AUTOMATABLE;

                if (lilvPort.has_property(lv2World.reportsLatency))
                    rdfPort->Designation = LV2_PORT_DESIGNATION_LATENCY;

                // check if sidechain (some plugins use sidechain groups instead of isSidechain)
                if (LilvNode* const portGroupNode = lilvPort.get(lv2World.pg_group.me))
                {
                    if (LilvNode* const portSideChainOfNode = lilv_world_get(lv2World.me, portGroupNode,
                                                                             lv2World.pg_sideChainOf.me, nullptr))
                    {
                        rdfPort->Properties |= LV2_PORT_SIDECHAIN;
                        lilv_node_free(portSideChainOfNode);
                    }
                    lilv_node_free(portGroupNode);
                }

                // no port properties set, check if using old/invalid ones
                if (rdfPort->Properties == 0x0)
                {
                    const Lilv::Node oldPropArtifacts(lv2World.new_uri(NS_devp "causesArtifacts"));
                    const Lilv::Node oldPropContinuousCV(lv2World.new_uri(NS_devp "continuousCV"));
                    const Lilv::Node oldPropDiscreteCV(lv2World.new_uri(NS_devp "discreteCV"));
                    const Lilv::Node oldPropExpensive(lv2World.new_uri(NS_devp "expensive"));
                    const Lilv::Node oldPropStrictBounds(lv2World.new_uri(NS_devp "hasStrictBounds"));
                    const Lilv::Node oldPropLogarithmic(lv2World.new_uri(NS_devp "logarithmic"));
                    const Lilv::Node oldPropNotAutomatic(lv2World.new_uri(NS_devp "notAutomatic"));
                    const Lilv::Node oldPropNotOnGUI(lv2World.new_uri(NS_devp "notOnGUI"));
                    const Lilv::Node oldPropTrigger(lv2World.new_uri(NS_devp "trigger"));

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

            // --------------------------------------------------------------------------------------------------------
            // Set Port Designation
            {
                if (LilvNode* const designationNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.designation.me))
                {
                    if (const char* const designation = lilv_node_as_string(designationNode))
                    {
                        /**/ if (std::strcmp(designation, LV2_CORE__control) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_CONTROL;
                        else if (std::strcmp(designation, LV2_CORE__enabled) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_ENABLED;
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
                        else if (std::strcmp(designation, LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat) == 0)
                            rdfPort->Designation = LV2_PORT_DESIGNATION_TIME_TICKS_PER_BEAT;
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

            // --------------------------------------------------------------------------------------------------------
            // Set Port MIDI Map

            if (LilvNode* const bindingNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.midi_binding.me))
            {
                if (lilv_node_is_string(bindingNode))
                {
                    if (const char* const bindingAsString = lilv_node_as_string(bindingNode))
                    {
                        if (std::strncmp(bindingAsString, "B0", 2) == 0 && std::strlen(bindingAsString) == 6)
                        {
                            const char binding[3] = { bindingAsString[2], bindingAsString[3], '\0' };
                            const long number = std::strtol(binding, nullptr, 16);

                            if (number >= 0 && number <= 0xff)
                            {
                                rdfPort->MidiMap.Type = LV2_PORT_MIDI_MAP_CC;
                                rdfPort->MidiMap.Number = static_cast<uint32_t>(number);
                            }
                        }
                    }
                }
                else
                {
                    if (lilv_node_is_blank(bindingNode))
                    {
                        Lilv::Nodes ctrlNumNodes(lv2World.find_nodes(bindingNode, lv2World.midi_ctlrNumber, nullptr));

                        if (ctrlNumNodes.size() == 1)
                        {
                            const int midiCC = ctrlNumNodes.get_first().as_int();

                            if (midiCC >= 0 && midiCC <= 0xff)
                            {
                                rdfPort->MidiMap.Type = LV2_PORT_MIDI_MAP_CC;
                                rdfPort->MidiMap.Number = static_cast<uint32_t>(midiCC);
                            }
                        }

                        lilv_nodes_free(const_cast<LilvNodes*>(ctrlNumNodes.me));
                    }
                }

                lilv_node_free(bindingNode);
            }
            else if (LilvNode* const midiMapNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.mm_defaultControl.me))
            {
                if (lilv_node_is_blank(midiMapNode))
                {
                    Lilv::Nodes midiMapTypeNodes(lv2World.find_nodes(midiMapNode, lv2World.mm_controlType, nullptr));
                    Lilv::Nodes midiMapNumberNodes(lv2World.find_nodes(midiMapNode, lv2World.mm_controlNumber, nullptr));

                    if (midiMapTypeNodes.size() == 1 && midiMapNumberNodes.size() == 1)
                    {
                        if (const char* const midiMapType = midiMapTypeNodes.get_first().as_string())
                        {
                            /**/ if (std::strcmp(midiMapType, LV2_MIDI_Map__CC) == 0)
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

            // --------------------------------------------------------------------------------------------------------
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

            // --------------------------------------------------------------------------------------------------------
            // Set Port Unit
            {
                if (LilvNode* const unitUnitNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.unit_unit.me))
                {
                    if (lilv_node_is_uri(unitUnitNode))
                    {
                        if (const char* const unitUnit = lilv_node_as_uri(unitUnitNode))
                        {
                            rdfPort->Unit.Hints |= LV2_PORT_UNIT_UNIT;

                            /**/ if (std::strcmp(unitUnit, LV2_UNITS__bar) == 0)
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
                            else if (std::strcmp(unitUnit, "http://moddevices.com/ns/mod#volts") == 0)
                                rdfPort->Unit.Unit = LV2_PORT_UNIT_VOLTS;
                            else
                                carla_stderr("lv2_rdf_new(\"%s\") - got unknown unit '%s'", uri, unitUnit);
                        }
                    }

                    if (LilvNode* const unitNameNode = lilv_world_get(lv2World.me, unitUnitNode, lv2World.unit_name.me, nullptr))
                    {
                        if (const char* const unitName = lilv_node_as_string(unitNameNode))
                        {
                            rdfPort->Unit.Hints |= LV2_PORT_UNIT_NAME;
                            rdfPort->Unit.Name   = carla_strdup(unitName);
                        }
                        lilv_node_free(unitNameNode);
                    }

                    if (LilvNode* const unitRenderNode = lilv_world_get(lv2World.me, unitUnitNode, lv2World.unit_render.me, nullptr))
                    {
                        if (const char* const unitRender = lilv_node_as_string(unitRenderNode))
                        {
                            rdfPort->Unit.Hints |= LV2_PORT_UNIT_RENDER;
                            rdfPort->Unit.Render = carla_strdup(unitRender);
                        }
                        lilv_node_free(unitRenderNode);
                    }

                    if (LilvNode* const unitSymbolNode = lilv_world_get(lv2World.me, unitUnitNode, lv2World.unit_symbol.me, nullptr))
                    {
                        if (const char* const unitSymbol = lilv_node_as_string(unitSymbolNode))
                        {
                            rdfPort->Unit.Hints |= LV2_PORT_UNIT_SYMBOL;
                            rdfPort->Unit.Symbol = carla_strdup(unitSymbol);
                        }
                        lilv_node_free(unitSymbolNode);
                    }

                    lilv_node_free(unitUnitNode);
                }
            }

            // --------------------------------------------------------------------------------------------------------
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

            // --------------------------------------------------------------------------------------------------------
            // Set Port Scale Points
            {
                Lilv::ScalePoints lilvScalePoints(lilvPort.get_scale_points());

                if (const uint numScalePoints = lilvScalePoints.size())
                {
                    rdfPort->ScalePoints = new LV2_RDF_PortScalePoint[numScalePoints];

                    // get all scalepoints and sort them by value
                    LilvScalePointMap sortedpoints;

                    LILV_FOREACH(scale_points, it, lilvScalePoints)
                    {
                        Lilv::ScalePoint lilvScalePoint(lilvScalePoints.get(it));

                        CARLA_SAFE_ASSERT_CONTINUE(lilvScalePoint.get_label() != nullptr);

                        if (const LilvNode* const valuenode = lilvScalePoint.get_value())
                        {
                            const double valueid = lilv_node_as_float(valuenode);
                            sortedpoints[valueid] = lilvScalePoint.me;
                        }
                    }

                    // now safe to store, sorted by using std::map
                    uint numUsed = 0;
                    for (LilvScalePointMap::iterator it=sortedpoints.begin(), end=sortedpoints.end(); it != end; ++it)
                    {
                        CARLA_SAFE_ASSERT_BREAK(numUsed < numScalePoints);
                        LV2_RDF_PortScalePoint* const rdfScalePoint(&rdfPort->ScalePoints[numUsed++]);

                        const LilvScalePoint* const scalepoint = it->second;

                        const LilvNode* const xlabel = lilv_scale_point_get_label(scalepoint);
                        const LilvNode* const xvalue = lilv_scale_point_get_value(scalepoint);

                        rdfScalePoint->Label = carla_strdup(lilv_node_as_string(xlabel));
                        rdfScalePoint->Value = lilv_node_as_float(xvalue);
                    }

                    rdfPort->ScalePointCount = numUsed;
                }

                lilv_nodes_free(const_cast<LilvNodes*>(lilvScalePoints.me));
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin Parameters
    {
        std::map<std::string, LV2_RDF_Parameter> parameters;
        Lilv::Nodes patchReadableNodes(lilvPlugin.get_value(lv2World.patch_readable));
        Lilv::Nodes patchWritableNodes(lilvPlugin.get_value(lv2World.patch_writable));

        if (const uint numParameters = patchWritableNodes.size())
        {
            uint numUsed = 0;
            LILV_FOREACH(nodes, it, patchWritableNodes)
            {
                CARLA_SAFE_ASSERT_BREAK(numUsed < numParameters);

                Lilv::Node patchWritableNode(patchWritableNodes.get(it));

                if (LilvNode* const typeNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                              lv2World.rdf_type.me, nullptr))
                {
                    const char* const type = lilv_node_as_uri(typeNode);

                    if (std::strcmp(type, LV2_CORE__Parameter) != 0)
                    {
                        lilv_node_free(typeNode);
                        continue;
                    }

                    lilv_node_free(typeNode);
                }
                else
                {
                    continue;
                }

                CARLA_SAFE_ASSERT_CONTINUE(patchWritableNode.is_uri());

                ++numUsed;
                LV2_RDF_Parameter rdfParam;
                rdfParam.URI = carla_strdup(patchWritableNode.as_uri());
                rdfParam.Flags = LV2_PARAMETER_FLAG_INPUT;

                if (patchReadableNodes.contains(patchWritableNode))
                    rdfParam.Flags |= LV2_PARAMETER_FLAG_OUTPUT;

                // ----------------------------------------------------------------------------------------------------
                // Set Basics

                if (LilvNode* const rangeNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                               lv2World.rdfs_range.me, nullptr))
                {
                    const char* const rangeURI = lilv_node_as_string(rangeNode);

                    /**/ if (std::strcmp(rangeURI, LV2_ATOM__Bool) == 0)
                        rdfParam.Type = LV2_PARAMETER_TYPE_BOOL;
                    else if (std::strcmp(rangeURI, LV2_ATOM__Int) == 0)
                        rdfParam.Type = LV2_PARAMETER_TYPE_INT;
                    else if (std::strcmp(rangeURI, LV2_ATOM__Long) == 0)
                        rdfParam.Type = LV2_PARAMETER_TYPE_LONG;
                    else if (std::strcmp(rangeURI, LV2_ATOM__Float) == 0)
                        rdfParam.Type = LV2_PARAMETER_TYPE_FLOAT;
                    else if (std::strcmp(rangeURI, LV2_ATOM__Double) == 0)
                        rdfParam.Type = LV2_PARAMETER_TYPE_DOUBLE;
                    else if (std::strcmp(rangeURI, LV2_ATOM__Path) == 0)
                        rdfParam.Type = LV2_PARAMETER_TYPE_PATH;
                    else if (std::strcmp(rangeURI, LV2_ATOM__String) == 0)
                        rdfParam.Type = LV2_PARAMETER_TYPE_STRING;
                    else
                        carla_stderr("lv2_rdf_new(\"%s\") - got unknown parameter type '%s'", uri, rangeURI);

                    lilv_node_free(rangeNode);
                }

                if (LilvNode* const labelNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                               lv2World.rdfs_label.me, nullptr))
                {
                    rdfParam.Label = carla_strdup(lilv_node_as_string(labelNode));
                    lilv_node_free(labelNode);
                }

                if (LilvNode* const commentNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                                 lv2World.rdfs_comment.me, nullptr))
                {
                    rdfParam.Comment = carla_strdup_safe(lilv_node_as_string(commentNode));
                    lilv_node_free(commentNode);
                }

                if (LilvNode* const groupNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                               lv2World.pg_group.me, nullptr))
                {
                    rdfParam.GroupURI = carla_strdup_safe(lilv_node_as_uri(groupNode));

                    if (portGroupURIs.appendUnique(rdfParam.GroupURI))
                        portGroupNodes.append(groupNode);
                    else
                        lilv_node_free(groupNode);
                }

                // ----------------------------------------------------------------------------------------------------
                // Set Port MIDI Map

                if (LilvNode* const bindingNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                                 lv2World.midi_binding.me, nullptr))
                {
                    if (const char* const bindingAsString = lilv_node_as_string(bindingNode))
                    {
                        if (std::strncmp(bindingAsString, "B0", 2) == 0 && std::strlen(bindingAsString) == 6)
                        {
                            const char binding[3] = { bindingAsString[2], bindingAsString[3], '\0' };
                            const long number = std::strtol(binding, nullptr, 16);

                            if (number >= 0 && number <= 0xff)
                            {
                                rdfParam.MidiMap.Type = LV2_PORT_MIDI_MAP_CC;
                                rdfParam.MidiMap.Number = static_cast<uint32_t>(number);
                            }
                        }
                    }

                    lilv_node_free(bindingNode);
                }

                // ----------------------------------------------------------------------------------------------------
                // Set Port Points

                if (LilvNode* const defNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                             lv2World.value_default.me, nullptr))
                {
                    rdfParam.Points.Hints  |= LV2_PORT_POINT_DEFAULT;
                    rdfParam.Points.Default = lilv_node_as_float(defNode);
                    lilv_node_free(defNode);
                }

                if (LilvNode* const minNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                             lv2World.value_minimum.me, nullptr))
                {
                    rdfParam.Points.Hints  |= LV2_PORT_POINT_MINIMUM;
                    rdfParam.Points.Minimum = lilv_node_as_float(minNode);
                    lilv_node_free(minNode);
                }

                if (LilvNode* const maxNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                             lv2World.value_maximum.me, nullptr))
                {
                    rdfParam.Points.Hints  |= LV2_PORT_POINT_MAXIMUM;
                    rdfParam.Points.Maximum = lilv_node_as_float(maxNode);
                    lilv_node_free(maxNode);
                }

                // ----------------------------------------------------------------------------------------------------
                // Set Port Unit

                if (LilvNode* const unitUnitNode = lilv_world_get(lv2World.me, patchWritableNode,
                                                                  lv2World.unit_unit.me, nullptr))
                {
                    if (lilv_node_is_uri(unitUnitNode))
                    {
                        if (const char* const unitUnit = lilv_node_as_uri(unitUnitNode))
                        {
                            rdfParam.Unit.Hints |= LV2_PORT_UNIT_UNIT;

                            /**/ if (std::strcmp(unitUnit, LV2_UNITS__bar) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_BAR;
                            else if (std::strcmp(unitUnit, LV2_UNITS__beat) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_BEAT;
                            else if (std::strcmp(unitUnit, LV2_UNITS__bpm) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_BPM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__cent) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_CENT;
                            else if (std::strcmp(unitUnit, LV2_UNITS__cm) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_CM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__coef) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_COEF;
                            else if (std::strcmp(unitUnit, LV2_UNITS__db) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_DB;
                            else if (std::strcmp(unitUnit, LV2_UNITS__degree) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_DEGREE;
                            else if (std::strcmp(unitUnit, LV2_UNITS__frame) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_FRAME;
                            else if (std::strcmp(unitUnit, LV2_UNITS__hz) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_HZ;
                            else if (std::strcmp(unitUnit, LV2_UNITS__inch) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_INCH;
                            else if (std::strcmp(unitUnit, LV2_UNITS__khz) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_KHZ;
                            else if (std::strcmp(unitUnit, LV2_UNITS__km) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_KM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__m) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_M;
                            else if (std::strcmp(unitUnit, LV2_UNITS__mhz) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_MHZ;
                            else if (std::strcmp(unitUnit, LV2_UNITS__midiNote) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_MIDINOTE;
                            else if (std::strcmp(unitUnit, LV2_UNITS__mile) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_MILE;
                            else if (std::strcmp(unitUnit, LV2_UNITS__min) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_MIN;
                            else if (std::strcmp(unitUnit, LV2_UNITS__mm) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_MM;
                            else if (std::strcmp(unitUnit, LV2_UNITS__ms) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_MS;
                            else if (std::strcmp(unitUnit, LV2_UNITS__oct) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_OCT;
                            else if (std::strcmp(unitUnit, LV2_UNITS__pc) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_PC;
                            else if (std::strcmp(unitUnit, LV2_UNITS__s) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_S;
                            else if (std::strcmp(unitUnit, LV2_UNITS__semitone12TET) == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_SEMITONE;
                            else if (std::strcmp(unitUnit, "http://moddevices.com/ns/mod#volts") == 0)
                                rdfParam.Unit.Unit = LV2_PORT_UNIT_VOLTS;
                            else
                                carla_stderr("lv2_rdf_new(\"%s\") - got unknown unit '%s'", uri, unitUnit);
                        }
                    }

                    if (LilvNode* const unitNameNode = lilv_world_get(lv2World.me, unitUnitNode,
                                                                      lv2World.unit_name.me, nullptr))
                    {
                        if (const char* const unitName = lilv_node_as_string(unitNameNode))
                        {
                            rdfParam.Unit.Hints |= LV2_PORT_UNIT_NAME;
                            rdfParam.Unit.Name   = carla_strdup(unitName);
                        }
                        lilv_node_free(unitNameNode);
                    }

                    if (LilvNode* const unitRenderNode = lilv_world_get(lv2World.me, unitUnitNode,
                                                                        lv2World.unit_render.me, nullptr))
                    {
                        if (const char* const unitRender = lilv_node_as_string(unitRenderNode))
                        {
                            rdfParam.Unit.Hints |= LV2_PORT_UNIT_RENDER;
                            rdfParam.Unit.Render = carla_strdup(unitRender);
                        }
                        lilv_node_free(unitRenderNode);
                    }

                    if (LilvNode* const unitSymbolNode = lilv_world_get(lv2World.me, unitUnitNode,
                                                                        lv2World.unit_symbol.me, nullptr))
                    {
                        if (const char* const unitSymbol = lilv_node_as_string(unitSymbolNode))
                        {
                            rdfParam.Unit.Hints |= LV2_PORT_UNIT_SYMBOL;
                            rdfParam.Unit.Symbol = carla_strdup(unitSymbol);
                        }
                        lilv_node_free(unitSymbolNode);
                    }

                    lilv_node_free(unitUnitNode);
                }

                parameters[rdfParam.URI].copyAndReplace(rdfParam);
            }

            CARLA_SAFE_ASSERT_UINT2(parameters.size() == numUsed, parameters.size(), numUsed);
            rdfDescriptor->Parameters = new LV2_RDF_Parameter[numUsed];
            rdfDescriptor->ParameterCount = numUsed;

            numUsed = 0;
            for (std::map<std::string, LV2_RDF_Parameter>::iterator it = parameters.begin(), end = parameters.end();
                 it != end; ++it)
            {
                rdfDescriptor->Parameters[numUsed++].copyAndReplace(it->second);
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(patchReadableNodes.me));
        lilv_nodes_free(const_cast<LilvNodes*>(patchWritableNodes.me));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin Port Groups

    if (const size_t portGroupCount = portGroupURIs.count())
    {
        rdfDescriptor->PortGroups = new LV2_RDF_PortGroup[portGroupCount];

        uint32_t count = 0;
        CarlaStringList::Itenerator itu = portGroupURIs.begin2();
        LinkedList<LilvNode*>::Itenerator itn = portGroupNodes.begin2();
        for (; itu.valid() && itn.valid(); itu.next(), itn.next())
        {
            const char* const portGroupURI = itu.getValue(nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(portGroupURI != nullptr);

            LilvNode* const portGroupNode = itn.getValue(nullptr);
            CARLA_SAFE_ASSERT_CONTINUE(portGroupNode != nullptr);

            LV2_RDF_PortGroup& portGroup(rdfDescriptor->PortGroups[count]);
            portGroup.URI = portGroupURI;

            if (LilvNode* const portGroupNameNode = lilv_world_get(lv2World.me, portGroupNode,
                                                                   lv2World.lv2_name.me, nullptr))
            {
                portGroup.Name = carla_strdup_safe(lilv_node_as_string(portGroupNameNode));
                lilv_node_free(portGroupNameNode);
            }
            // some plugins use rdfs:label, spec was not clear which one to use
            else if (LilvNode* const portGroupLabelNode = lilv_world_get(lv2World.me, portGroupNode,
                                                                         lv2World.rdfs_label.me, nullptr))
            {
                portGroup.Name = carla_strdup_safe(lilv_node_as_string(portGroupLabelNode));
                lilv_node_free(portGroupLabelNode);
            }

            if (LilvNode* const portGroupSymbolNode = lilv_world_get(lv2World.me, portGroupNode,
                                                                     lv2World.lv2_symbol.me, nullptr))
            {
                portGroup.Symbol = carla_strdup_safe(lilv_node_as_string(portGroupSymbolNode));
                lilv_node_free(portGroupSymbolNode);
            }

            ++count;
            lilv_node_free(portGroupNode);
        }

        rdfDescriptor->PortGroupCount = count;
        portGroupNodes.clear();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin Presets

    if (loadPresets)
    {
        Lilv::Nodes presetNodes(lilvPlugin.get_related(lv2World.preset_preset));

        if (presetNodes.size() > 0)
        {
            // create a list of preset URIs (for sorting and unique-ness)
           #if defined(CARLA_UTILS_USE_QT)
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
            water::StringArray presetListURIs;

            LILV_FOREACH(nodes, it, presetNodes)
            {
                Lilv::Node presetNode(presetNodes.get(it));

                water::String presetURI(presetNode.as_uri());

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

                const char* const presetURI(presetNode.as_uri());
                CARLA_SAFE_ASSERT_CONTINUE(presetURI != nullptr && presetURI[0] != '\0');

                // try to find label without loading the preset resource first
                Lilv::Nodes presetLabelNodes(lv2World.find_nodes(presetNode, lv2World.rdfs_label, nullptr));

                // failed, try loading resource
                if (presetLabelNodes.size() == 0)
                {
                    // if loading resource fails, skip this preset
                    if (lv2World.load_resource(presetNode) == -1)
                        continue;

                    // ok, let's try again
                    presetLabelNodes = lv2World.find_nodes(presetNode, lv2World.rdfs_label, nullptr);
                }

                if (presetLabelNodes.size() > 0)
                {
                   #if defined(CARLA_UTILS_USE_QT)
                    const int index = presetListURIs.indexOf(QString(presetURI));
                   #else
                    const int index = presetListURIs.indexOf(water::String(presetURI));
                   #endif
                    CARLA_SAFE_ASSERT_CONTINUE(index >= 0 && index < static_cast<int>(rdfDescriptor->PresetCount));

                    LV2_RDF_Preset* const rdfPreset(&rdfDescriptor->Presets[index]);

                    // ---------------------------------------------------
                    // Set Preset Information

                    rdfPreset->URI = carla_strdup(presetURI);

                    if (const char* const label = presetLabelNodes.get_first().as_string())
                        rdfPreset->Label = carla_strdup(label);

                    lilv_nodes_free(const_cast<LilvNodes*>(presetLabelNodes.me));
                }
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(presetNodes.me));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin Features
    {
        Lilv::Nodes lilvFeatureNodes(lilvPlugin.get_supported_features());

        if (const uint numFeatures = lilvFeatureNodes.size())
        {
            Lilv::Nodes lilvFeatureNodesR(lilvPlugin.get_required_features());

            rdfDescriptor->Features = new LV2_RDF_Feature[numFeatures];

            uint numUsed = 0;
            LILV_FOREACH(nodes, it, lilvFeatureNodes)
            {
                CARLA_SAFE_ASSERT_BREAK(numUsed < numFeatures);

                Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(it));
                LV2_RDF_Feature* const rdfFeature(&rdfDescriptor->Features[numUsed++]);

                rdfFeature->Required = lilvFeatureNodesR.contains(lilvFeatureNode);

                if (const char* const featureURI = lilvFeatureNode.as_uri())
                    rdfFeature->URI = carla_strdup(featureURI);
                else
                    rdfFeature->URI = nullptr;
            }

            rdfDescriptor->FeatureCount = numUsed;
            lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodesR.me));
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodes.me));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin Extensions
    {
        Lilv::Nodes lilvExtensionDataNodes(lilvPlugin.get_extension_data());

        if (const uint numExtensions = lilvExtensionDataNodes.size())
        {
            rdfDescriptor->Extensions = new LV2_URI[numExtensions];

            uint numUsed = 0;
            LILV_FOREACH(nodes, it, lilvExtensionDataNodes)
            {
                CARLA_SAFE_ASSERT_BREAK(numUsed < numExtensions);

                Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(it));
                LV2_URI* const rdfExtension(&rdfDescriptor->Extensions[numUsed++]);

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

            for (uint32_t x=numUsed; x < rdfDescriptor->ExtensionCount; ++x)
                rdfDescriptor->Extensions[x] = nullptr;

            rdfDescriptor->ExtensionCount = numUsed;
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvExtensionDataNodes.me));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set Plugin UIs
    {
        Lilv::UIs lilvUIs(lilvPlugin.get_uis());

        const uint numUIs = lilvUIs.size();

        if (numUIs > 0)
        {
            rdfDescriptor->UIs = new LV2_RDF_UI[numUIs];

            uint numUsed = 0;
            LILV_FOREACH(uis, it, lilvUIs)
            {
                CARLA_SAFE_ASSERT_BREAK(numUsed < numUIs);

                Lilv::UI lilvUI(lilvUIs.get(it));
                LV2_RDF_UI* const rdfUI(&rdfDescriptor->UIs[numUsed++]);

                lv2World.load_resource(lilvUI.get_uri());

                // ----------------------------------------------------------------------------------------------------
                // Set UI Type

                /**/ if (lilvUI.is_a(lv2World.ui_gtk2))
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
                else if (lilvUI.is_a(lv2World.ui))
                    rdfUI->Type = LV2_UI_NONE;
                else
                    carla_stderr("lv2_rdf_new(\"%s\") - UI '%s' is of unknown type", uri, lilvUI.get_uri().as_uri());

                // ----------------------------------------------------------------------------------------------------
                // Set UI Information
                {
                    if (const char* const uiURI = lilvUI.get_uri().as_uri())
                        rdfUI->URI = carla_strdup(uiURI);

                    if (const char* const uiBinary = lilvUI.get_binary_uri().as_string())
                        rdfUI->Binary = carla_strdup_free(lilv_file_uri_parse(uiBinary, nullptr));

                    if (const char* const uiBundle = lilvUI.get_bundle_uri().as_string())
                        rdfUI->Bundle = carla_strdup_free(lilv_file_uri_parse(uiBundle, nullptr));
                }

                // ----------------------------------------------------------------------------------------------------
                // Set UI Features
                {
                    Lilv::Nodes lilvFeatureNodes(lilvUI.get_supported_features());

                    if (const uint numFeatures = lilvFeatureNodes.size())
                    {
                        Lilv::Nodes lilvFeatureNodesR(lilvUI.get_required_features());

                        rdfUI->Features = new LV2_RDF_Feature[numFeatures];

                        uint numUsed2 = 0;
                        LILV_FOREACH(nodes, it2, lilvFeatureNodes)
                        {
                            CARLA_SAFE_ASSERT_UINT2_BREAK(numUsed2 < numFeatures, numUsed2, numFeatures);

                            Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(it2));
                            LV2_RDF_Feature* const rdfFeature(&rdfUI->Features[numUsed2++]);

                            rdfFeature->Required = lilvFeatureNodesR.contains(lilvFeatureNode);

                            if (const char* const featureURI = lilvFeatureNode.as_uri())
                                rdfFeature->URI = carla_strdup(featureURI);
                            else
                                rdfFeature->URI = nullptr;
                        }

                        rdfUI->FeatureCount = numUsed2;
                        lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodesR.me));
                    }

                    lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodes.me));
                }

                // ----------------------------------------------------------------------------------------------------
                // Set UI Extensions
                {
                    Lilv::Nodes lilvExtensionDataNodes(lilvUI.get_extension_data());

                    if (const uint numExtensions = lilvExtensionDataNodes.size())
                    {
                        rdfUI->Extensions = new LV2_URI[numExtensions];

                        uint numUsed2 = 0;
                        LILV_FOREACH(nodes, it2, lilvExtensionDataNodes)
                        {
                            CARLA_SAFE_ASSERT_UINT2_BREAK(numUsed2 < numExtensions, numUsed2, numExtensions);

                            Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(it2));
                            LV2_URI* const rdfExtension(&rdfUI->Extensions[numUsed2++]);

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

                        for (uint x2=numUsed2; x2 < rdfUI->ExtensionCount; ++x2)
                            rdfUI->Extensions[x2] = nullptr;

                        rdfUI->ExtensionCount = numUsed2;
                    }

                    lilv_nodes_free(const_cast<LilvNodes*>(lilvExtensionDataNodes.me));
                }

                // ----------------------------------------------------------------------------------------------------
                // Set UI Port Notifications
                {
                    Lilv::Nodes portNotifNodes(lv2World.find_nodes(lilvUI.get_uri(), lv2World.ui_portNotif.me, nullptr));

                    if (const uint portNotifCount = portNotifNodes.size())
                    {
                        rdfUI->PortNotificationCount = portNotifCount;
                        rdfUI->PortNotifications = new LV2_RDF_UI_PortNotification[portNotifCount];

                        uint numUsed2 = 0;
                        LILV_FOREACH(nodes, it2, portNotifNodes)
                        {
                            CARLA_SAFE_ASSERT_UINT2_BREAK(numUsed2 < portNotifCount, numUsed2, portNotifCount);

                            Lilv::Node portNotifNode(portNotifNodes.get(it2));
                            LV2_RDF_UI_PortNotification* const rdfPortNotif(&rdfUI->PortNotifications[numUsed2++]);

                            LilvNode* const protocolNode = lilv_world_get(lv2World.me, portNotifNode,
                                                                          lv2World.ui_protocol.me, nullptr);

                            if (protocolNode != nullptr)
                            {
                                CARLA_SAFE_ASSERT_CONTINUE(lilv_node_is_uri(protocolNode));

                                const char* const protocol = lilv_node_as_uri(protocolNode);
                                CARLA_SAFE_ASSERT_CONTINUE(protocol != nullptr && protocol[0] != '\0');

                                /**/ if (std::strcmp(protocol, LV2_UI__floatProtocol) == 0)
                                    rdfPortNotif->Protocol = LV2_UI_PORT_PROTOCOL_FLOAT;
                                else if (std::strcmp(protocol, LV2_UI__peakProtocol) == 0)
                                    rdfPortNotif->Protocol = LV2_UI_PORT_PROTOCOL_PEAK;
                            }
                            else
                            {
                                rdfPortNotif->Protocol = LV2_UI_PORT_PROTOCOL_FLOAT;
                            }

                            /**/ if (LilvNode* const symbolNode = lilv_world_get(lv2World.me, portNotifNode,
                                                                                 lv2World.symbol.me, nullptr))
                            {
                                CARLA_SAFE_ASSERT_CONTINUE(lilv_node_is_string(symbolNode));

                                const char* const symbol = lilv_node_as_string(symbolNode);
                                CARLA_SAFE_ASSERT_CONTINUE(symbol != nullptr && symbol[0] != '\0');

                                rdfPortNotif->Symbol = carla_strdup(symbol);

                                lilv_node_free(symbolNode);
                            }
                            else if (LilvNode* const indexNode = lilv_world_get(lv2World.me, portNotifNode,
                                                                                lv2World.ui_portIndex.me, nullptr))
                            {
                                CARLA_SAFE_ASSERT_CONTINUE(lilv_node_is_int(indexNode));

                                const int index = lilv_node_as_int(indexNode);
                                CARLA_SAFE_ASSERT_CONTINUE(index >= 0);

                                rdfPortNotif->Index = static_cast<uint32_t>(index);

                                lilv_node_free(indexNode);
                            }

                            lilv_node_free(protocolNode);
                        }
                    }

                    lilv_nodes_free(const_cast<LilvNodes*>(portNotifNodes.me));
                }
            }

            rdfDescriptor->UICount = numUsed;
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvUIs.me));
    }

    return rdfDescriptor;
}

// --------------------------------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------------------------------
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
    if (std::strcmp(uri, LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI) == 0)
        return true;
    if (std::strcmp(uri, LV2_CORE__hardRTCapable) == 0)
        return true;
    if (std::strcmp(uri, LV2_CORE__inPlaceBroken) == 0)
        return true;
    if (std::strcmp(uri, LV2_CORE__isLive) == 0)
        return true;
    if (std::strcmp(uri, LV2_EVENT_URI) == 0)
        return true;
    if (std::strcmp(uri, LV2_INLINEDISPLAY__queue_draw) == 0)
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
    if (std::strcmp(uri, LV2_STATE__freePath) == 0)
        return true;
    if (std::strcmp(uri, LV2_STATE__loadDefaultState) == 0)
        return true;
    if (std::strcmp(uri, LV2_STATE__makePath) == 0)
        return true;
    if (std::strcmp(uri, LV2_STATE__mapPath) == 0)
        return true;
    if (std::strcmp(uri, LV2_STATE__threadSafeRestore) == 0)
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

// --------------------------------------------------------------------------------------------------------------------
// Check if we support a plugin or UI feature

static inline
bool is_lv2_ui_feature_supported(const LV2_URI uri) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(uri != nullptr && uri[0] != '\0', false);

    if (is_lv2_feature_supported(uri))
        return true;
#ifndef BUILD_BRIDGE_UI
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
    if (std::strcmp(uri, LV2_UI__makeSONameResident) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__noUserResize) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__parent) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__portMap) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__portSubscribe) == 0)
        return true;
    if (std::strcmp(uri, LV2_UI__requestValue) == 0)
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

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_LV2_UTILS_HPP_INCLUDED
