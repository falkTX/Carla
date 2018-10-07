/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.h"

#include "CarlaNative.h"
#include "CarlaString.hpp"
#include "CarlaLv2Utils.hpp"

#ifdef DEBUG
# include "CarlaBackendUtils.hpp"
#endif

#include "water/containers/Array.h"
#include "water/files/File.h"

namespace CB = CarlaBackend;

using water::Array;
using water::File;
using water::String;
using water::StringArray;

static const char* const gNullCharPtr = "";

static bool isCachedPluginType(const CB::PluginType ptype)
{
    switch (ptype)
    {
    case CB::PLUGIN_INTERNAL:
    case CB::PLUGIN_LV2:
    case CB::PLUGIN_SFZ:
        return true;
    default:
        return false;
    }
}

// -------------------------------------------------------------------------------------------------------------------

_CarlaCachedPluginInfo::_CarlaCachedPluginInfo() noexcept
    : valid(false),
      category(CB::PLUGIN_CATEGORY_NONE),
      hints(0x0),
      audioIns(0),
      audioOuts(0),
      midiIns(0),
      midiOuts(0),
      parameterIns(0),
      parameterOuts(0),
      name(gNullCharPtr),
      label(gNullCharPtr),
      maker(gNullCharPtr),
      copyright(gNullCharPtr) {}

// -------------------------------------------------------------------------------------------------------------------

static Array<File> gSFZs;

static void findSFZs(const char* const sfzPaths)
{
    CARLA_SAFE_ASSERT_RETURN(sfzPaths != nullptr,);

    if (sfzPaths[0] == '\0')
        return;

    const StringArray splitPaths(StringArray::fromTokens(sfzPaths, CARLA_OS_SPLIT_STR, ""));

    for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
    {
        Array<File> results;

        if (File(*it).findChildFiles(results, File::findFiles|File::ignoreHiddenFiles, true, "*.sfz") > 0)
            gSFZs.addArray(results);
    }
}

// -------------------------------------------------------------------------------------------------------------------

static const CarlaCachedPluginInfo* get_cached_plugin_internal(const NativePluginDescriptor& desc)
{
    static CarlaCachedPluginInfo info;

    info.category = static_cast<CB::PluginCategory>(desc.category);
    info.hints    = 0x0;

    if (desc.hints & NATIVE_PLUGIN_IS_RTSAFE)
        info.hints |= CB::PLUGIN_IS_RTSAFE;
    if (desc.hints & NATIVE_PLUGIN_IS_SYNTH)
        info.hints |= CB::PLUGIN_IS_SYNTH;
    if (desc.hints & NATIVE_PLUGIN_HAS_UI)
        info.hints |= CB::PLUGIN_HAS_CUSTOM_UI;
    if (desc.hints & NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS)
        info.hints |= CB::PLUGIN_NEEDS_FIXED_BUFFERS;
    if (desc.hints & NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD)
        info.hints |= CB::PLUGIN_NEEDS_UI_MAIN_THREAD;
    if (desc.hints & NATIVE_PLUGIN_USES_MULTI_PROGS)
        info.hints |= CB::PLUGIN_USES_MULTI_PROGS;

    info.valid         = true;
    info.audioIns      = desc.audioIns;
    info.audioOuts     = desc.audioOuts;
    info.midiIns       = desc.midiIns;
    info.midiOuts      = desc.midiOuts;
    info.parameterIns  = desc.paramIns;
    info.parameterOuts = desc.paramOuts;
    info.name          = desc.name;
    info.label         = desc.label;
    info.maker         = desc.maker;
    info.copyright     = desc.copyright;
    return &info;
}

static const CarlaCachedPluginInfo* get_cached_plugin_lv2(Lv2WorldClass& lv2World, Lilv::Plugin& lilvPlugin)
{
    static CarlaCachedPluginInfo info;

    info.valid = false;
    bool supported = true;

    // ----------------------------------------------------------------------------------------------------------------
    // text data

    {
        static CarlaString suri, sname, smaker, slicense;
        suri.clear(); sname.clear(); smaker.clear(); slicense.clear();

        suri = lilvPlugin.get_uri().as_uri();

        if (LilvNode* const nameNode = lilv_plugin_get_name(lilvPlugin.me))
        {
            if (const char* const name = lilv_node_as_string(nameNode))
                sname = name;
            lilv_node_free(nameNode);
        }

        if (const char* const author = lilvPlugin.get_author_name().as_string())
            smaker = author;

        Lilv::Nodes licenseNodes(lilvPlugin.get_value(lv2World.doap_license));

        if (licenseNodes.size() > 0)
        {
            if (const char* const license = licenseNodes.get_first().as_string())
                slicense = license;
        }

        lilv_nodes_free(const_cast<LilvNodes*>(licenseNodes.me));

        info.name      = sname.buffer();
        info.label     = suri.buffer();
        info.maker     = smaker.buffer();
        info.copyright = slicense.buffer();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // features

    info.hints = 0x0;

    if (lilvPlugin.get_uis().size() > 0)
        info.hints |= CB::PLUGIN_HAS_CUSTOM_UI;

    {
        Lilv::Nodes lilvRequiredFeatureNodes(lilvPlugin.get_required_features());

        LILV_FOREACH(nodes, it, lilvRequiredFeatureNodes)
        {
            Lilv::Node lilvFeatureNode(lilvRequiredFeatureNodes.get(it));
            const char* const featureURI(lilvFeatureNode.as_uri());
            CARLA_SAFE_ASSERT_CONTINUE(featureURI != nullptr);

            if (! is_lv2_feature_supported(featureURI))
            {
                if (std::strcmp(featureURI, LV2_DATA_ACCESS_URI) == 0
                 || std::strcmp(featureURI, LV2_INSTANCE_ACCESS_URI) == 0)
                {
                    // we give a warning about this below
                    continue;
                }

                supported = false;
                carla_stderr("LV2 plugin '%s' requires unsupported feature '%s'", info.label, featureURI);
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvRequiredFeatureNodes.me));
    }

    {
        Lilv::Nodes lilvSupportedFeatureNodes(lilvPlugin.get_supported_features());

        LILV_FOREACH(nodes, it, lilvSupportedFeatureNodes)
        {
            Lilv::Node lilvFeatureNode(lilvSupportedFeatureNodes.get(it));
            const char* const featureURI(lilvFeatureNode.as_uri());
            CARLA_SAFE_ASSERT_CONTINUE(featureURI != nullptr);

            if (std::strcmp(featureURI, LV2_CORE__hardRTCapable) == 0)
            {
                info.hints |= CB::PLUGIN_IS_RTSAFE;
            }
            else if (std::strcmp(featureURI, LV2_DATA_ACCESS_URI) == 0
                  || std::strcmp(featureURI, LV2_INSTANCE_ACCESS_URI) == 0)
            {
                carla_stderr("LV2 plugin '%s' DSP wants UI feature '%s', ignoring this", info.label, featureURI);
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(lilvSupportedFeatureNodes.me));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // category

    info.category = CB::PLUGIN_CATEGORY_NONE;

    {
        Lilv::Nodes typeNodes(lilvPlugin.get_value(lv2World.rdf_type));

        if (typeNodes.size() > 0)
        {
            if (typeNodes.contains(lv2World.class_allpass))
                info.category = CB::PLUGIN_CATEGORY_FILTER;
            if (typeNodes.contains(lv2World.class_amplifier))
                info.category = CB::PLUGIN_CATEGORY_DYNAMICS;
            if (typeNodes.contains(lv2World.class_analyzer))
                info.category = CB::PLUGIN_CATEGORY_UTILITY;
            if (typeNodes.contains(lv2World.class_bandpass))
                info.category = CB::PLUGIN_CATEGORY_FILTER;
            if (typeNodes.contains(lv2World.class_chorus))
                info.category = CB::PLUGIN_CATEGORY_MODULATOR;
            if (typeNodes.contains(lv2World.class_comb))
                info.category = CB::PLUGIN_CATEGORY_FILTER;
            if (typeNodes.contains(lv2World.class_compressor))
                info.category = CB::PLUGIN_CATEGORY_DYNAMICS;
            if (typeNodes.contains(lv2World.class_constant))
                info.category = CB::PLUGIN_CATEGORY_OTHER;
            if (typeNodes.contains(lv2World.class_converter))
                info.category = CB::PLUGIN_CATEGORY_UTILITY;
            if (typeNodes.contains(lv2World.class_delay))
                info.category = CB::PLUGIN_CATEGORY_DELAY;
            if (typeNodes.contains(lv2World.class_distortion))
                info.category = CB::PLUGIN_CATEGORY_DISTORTION;
            if (typeNodes.contains(lv2World.class_dynamics))
                info.category = CB::PLUGIN_CATEGORY_DYNAMICS;
            if (typeNodes.contains(lv2World.class_eq))
                info.category = CB::PLUGIN_CATEGORY_EQ;
            if (typeNodes.contains(lv2World.class_envelope))
                info.category = CB::PLUGIN_CATEGORY_DYNAMICS;
            if (typeNodes.contains(lv2World.class_expander))
                info.category = CB::PLUGIN_CATEGORY_DYNAMICS;
            if (typeNodes.contains(lv2World.class_filter))
                info.category = CB::PLUGIN_CATEGORY_FILTER;
            if (typeNodes.contains(lv2World.class_flanger))
                info.category = CB::PLUGIN_CATEGORY_MODULATOR;
            if (typeNodes.contains(lv2World.class_function))
                info.category = CB::PLUGIN_CATEGORY_UTILITY;
            if (typeNodes.contains(lv2World.class_gate))
                info.category = CB::PLUGIN_CATEGORY_DYNAMICS;
            if (typeNodes.contains(lv2World.class_generator))
                info.category = CB::PLUGIN_CATEGORY_OTHER;
            if (typeNodes.contains(lv2World.class_highpass))
                info.category = CB::PLUGIN_CATEGORY_FILTER;
            if (typeNodes.contains(lv2World.class_limiter))
                info.category = CB::PLUGIN_CATEGORY_DYNAMICS;
            if (typeNodes.contains(lv2World.class_lowpass))
                info.category = CB::PLUGIN_CATEGORY_FILTER;
            if (typeNodes.contains(lv2World.class_mixer))
                info.category = CB::PLUGIN_CATEGORY_UTILITY;
            if (typeNodes.contains(lv2World.class_modulator))
                info.category = CB::PLUGIN_CATEGORY_MODULATOR;
            if (typeNodes.contains(lv2World.class_multiEQ))
                info.category = CB::PLUGIN_CATEGORY_EQ;
            if (typeNodes.contains(lv2World.class_oscillator))
                info.category = CB::PLUGIN_CATEGORY_OTHER;
            if (typeNodes.contains(lv2World.class_paraEQ))
                info.category = CB::PLUGIN_CATEGORY_EQ;
            if (typeNodes.contains(lv2World.class_phaser))
                info.category = CB::PLUGIN_CATEGORY_MODULATOR;
            if (typeNodes.contains(lv2World.class_pitch))
                info.category = CB::PLUGIN_CATEGORY_OTHER;
            if (typeNodes.contains(lv2World.class_reverb))
                info.category = CB::PLUGIN_CATEGORY_DELAY;
            if (typeNodes.contains(lv2World.class_simulator))
                info.category = CB::PLUGIN_CATEGORY_OTHER;
            if (typeNodes.contains(lv2World.class_spatial))
                info.category = CB::PLUGIN_CATEGORY_OTHER;
            if (typeNodes.contains(lv2World.class_spectral))
                info.category = CB::PLUGIN_CATEGORY_OTHER;
            if (typeNodes.contains(lv2World.class_utility))
                info.category = CB::PLUGIN_CATEGORY_UTILITY;
            if (typeNodes.contains(lv2World.class_waveshaper))
                info.category = CB::PLUGIN_CATEGORY_DISTORTION;
            if (typeNodes.contains(lv2World.class_instrument))
            {
                info.category = CB::PLUGIN_CATEGORY_SYNTH;
                info.hints |= CB::PLUGIN_IS_SYNTH;
            }
        }

        lilv_nodes_free(const_cast<LilvNodes*>(typeNodes.me));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // number data

    info.audioIns      = 0;
    info.audioOuts     = 0;
    info.midiIns       = 0;
    info.midiOuts      = 0;
    info.parameterIns  = 0;
    info.parameterOuts = 0;

    for (uint i=0, count=lilvPlugin.get_num_ports(); i<count; ++i)
    {
        Lilv::Port lilvPort(lilvPlugin.get_port_by_index(i));

        bool isInput;

        /**/ if (lilvPort.is_a(lv2World.port_input))
        {
            isInput = true;
        }
        else if (lilvPort.is_a(lv2World.port_output))
        {
            isInput = false;
        }
        else
        {
            const LilvNode* const symbolNode = lilvPort.get_symbol();
            CARLA_SAFE_ASSERT_CONTINUE(symbolNode != nullptr && lilv_node_is_string(symbolNode));

            const char* const symbol = lilv_node_as_string(symbolNode);
            CARLA_SAFE_ASSERT_CONTINUE(symbol != nullptr);

            carla_stderr("LV2 plugin '%s' port '%s' is neither input or output", info.label, symbol);
            continue;
        }

        /**/ if (lilvPort.is_a(lv2World.port_control))
        {
            // skip some control ports
            if (lilvPort.has_property(lv2World.reportsLatency))
                continue;

            if (LilvNode* const designationNode = lilv_port_get(lilvPort.parent, lilvPort.me, lv2World.designation.me))
            {
                bool skip = false;

                if (const char* const designation = lilv_node_as_string(designationNode))
                {
                    /**/ if (std::strcmp(designation, LV2_CORE__control) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_CORE__freeWheeling) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_CORE__latency) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_PARAMETERS__sampleRate) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__bar) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__barBeat) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__beat) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__beatUnit) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__beatsPerBar) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__beatsPerMinute) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__frame) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__framesPerSecond) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_TIME__speed) == 0)
                        skip = true;
                    else if (std::strcmp(designation, LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat) == 0)
                        skip = true;
                }

                lilv_node_free(designationNode);

                if (skip)
                    continue;
            }

            if (isInput)
                ++(info.parameterIns);
            else
                ++(info.parameterOuts);
        }
        else if (lilvPort.is_a(lv2World.port_audio))
        {
            if (isInput)
                ++(info.audioIns);
            else
                ++(info.audioOuts);
        }
        else if (lilvPort.is_a(lv2World.port_cv))
        {
        }
        else if (lilvPort.is_a(lv2World.port_atom))
        {
            Lilv::Nodes supportNodes(lilvPort.get_value(lv2World.atom_supports));

            for (LilvIter *it = lilv_nodes_begin(supportNodes.me); ! lilv_nodes_is_end(supportNodes.me, it); it = lilv_nodes_next(supportNodes.me, it))
            {
                const Lilv::Node node(lilv_nodes_get(supportNodes.me, it));
                CARLA_SAFE_ASSERT_CONTINUE(node.is_uri());

                if (node.equals(lv2World.midi_event))
                {
                    if (isInput)
                        ++(info.midiIns);
                    else
                        ++(info.midiOuts);
                }
            }

            lilv_nodes_free(const_cast<LilvNodes*>(supportNodes.me));
        }
        else if (lilvPort.is_a(lv2World.port_event))
        {
            if (lilvPort.supports_event(lv2World.midi_event))
            {
                if (isInput)
                    ++(info.midiIns);
                else
                    ++(info.midiOuts);
            }
        }
        else if (lilvPort.is_a(lv2World.port_midi))
        {
            if (isInput)
                ++(info.midiIns);
            else
                ++(info.midiOuts);
        }
        else
        {
            const LilvNode* const symbolNode = lilvPort.get_symbol();
            CARLA_SAFE_ASSERT_CONTINUE(symbolNode != nullptr && lilv_node_is_string(symbolNode));

            const char* const symbol = lilv_node_as_string(symbolNode);
            CARLA_SAFE_ASSERT_CONTINUE(symbol != nullptr);

            supported = false;
            carla_stderr("LV2 plugin '%s' port '%s' is required but has unsupported type", info.label, symbol);
        }
    }

    if (supported)
        info.valid = true;

    return &info;
}

static const CarlaCachedPluginInfo* get_cached_plugin_sfz(const File file)
{
    static CarlaCachedPluginInfo info;

    static CarlaString name, filename;

    name = file.getFileNameWithoutExtension().toRawUTF8();
    name.replace('_',' ');

    filename = file.getFullPathName().toRawUTF8();

    info.category = CB::PLUGIN_CATEGORY_SYNTH;
    info.hints    = CB::PLUGIN_IS_SYNTH;
    // CB::PLUGIN_IS_RTSAFE

    info.valid         = true;
    info.audioIns      = 0;
    info.audioOuts     = 2;
    info.midiIns       = 1;
    info.midiOuts      = 0;
    info.parameterIns  = 0;
    info.parameterOuts = 1;

    info.name      = name.buffer();
    info.label     = filename.buffer();
    info.maker     = gNullCharPtr;
    info.copyright = gNullCharPtr;
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

uint carla_get_cached_plugin_count(CB::PluginType ptype, const char* pluginPath)
{
    CARLA_SAFE_ASSERT_RETURN(isCachedPluginType(ptype), 0);
    carla_debug("carla_get_cached_plugin_count(%i:%s, %s)", ptype, CB::PluginType2Str(ptype), pluginPath);

    switch (ptype)
    {
    case CB::PLUGIN_INTERNAL: {
        uint32_t count = 0;
        carla_get_native_plugins_data(&count);
        return count;
    }

    case CB::PLUGIN_LV2: {
        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());
        lv2World.initIfNeeded(pluginPath);
        return lv2World.getPluginCount();
    }

    case CB::PLUGIN_SFZ: {
        findSFZs(pluginPath);
        return static_cast<uint>(gSFZs.size());
    }

    default:
        return 0;
    }
}

const CarlaCachedPluginInfo* carla_get_cached_plugin_info(CB::PluginType ptype, uint index)
{
    carla_debug("carla_get_cached_plugin_info(%i:%s, %i)", ptype, CB::PluginType2Str(ptype), index);

    switch (ptype)
    {
    case CB::PLUGIN_INTERNAL: {
        uint32_t count = 0;
        const NativePluginDescriptor* const descs(carla_get_native_plugins_data(&count));
        CARLA_SAFE_ASSERT_BREAK(index < count);
        CARLA_SAFE_ASSERT_BREAK(descs != nullptr);

        const NativePluginDescriptor& desc(descs[index]);
        return get_cached_plugin_internal(desc);
    }

    case CB::PLUGIN_LV2: {
        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

        const LilvPlugin* const cPlugin(lv2World.getPluginFromIndex(index));
        CARLA_SAFE_ASSERT_BREAK(cPlugin != nullptr);

        Lilv::Plugin lilvPlugin(cPlugin);
        CARLA_SAFE_ASSERT_BREAK(lilvPlugin.get_uri().is_uri());

        return get_cached_plugin_lv2(lv2World, lilvPlugin);
    }

    case CB::PLUGIN_SFZ: {
        CARLA_SAFE_ASSERT_BREAK(index < static_cast<uint>(gSFZs.size()));
        return get_cached_plugin_sfz(gSFZs.getUnchecked(static_cast<int>(index)));
    }

    default:
        break;
    }

    static CarlaCachedPluginInfo info;
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

#include "../native-plugins/_data.cpp"

// -------------------------------------------------------------------------------------------------------------------
