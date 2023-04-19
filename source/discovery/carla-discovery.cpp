/*
 * Carla Plugin discovery
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

#include "CarlaBackendUtils.hpp"
#include "CarlaLibUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaScopeUtils.hpp"

#include "CarlaMIDI.h"
#include "LinkedList.hpp"

#include "CarlaLadspaUtils.hpp"
#include "CarlaDssiUtils.hpp"
#include "CarlaLv2Utils.hpp"
#include "CarlaVst2Utils.hpp"
#include "CarlaVst3Utils.hpp"
#include "CarlaClapUtils.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.cpp"
# if defined(USING_JUCE) && defined(__aarch64__)
#  include <spawn.h>
# endif
#endif

#ifdef CARLA_OS_WIN
# include <pthread.h>
# include <objbase.h>
#endif

#ifdef BUILD_BRIDGE
# undef HAVE_FLUIDSYNTH
# undef HAVE_YSFX
#endif

#ifdef HAVE_FLUIDSYNTH
# include <fluidsynth.h>
#endif

#include <iostream>

#include "water/files/File.h"

#ifndef BUILD_BRIDGE
# include "CarlaDssiUtils.cpp"
# include "CarlaJsfxUtils.hpp"
# include "../backend/utils/CachedPlugins.cpp"
#endif

#ifdef USING_JUCE
# include "carla_juce/carla_juce.h"
# pragma GCC diagnostic ignored "-Wdouble-promotion"
# pragma GCC diagnostic ignored "-Wduplicated-branches"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wfloat-equal"
# include "juce_audio_processors/juce_audio_processors.h"
# if JUCE_PLUGINHOST_VST
#  define USING_JUCE_FOR_VST2
# endif
# if JUCE_PLUGINHOST_VST3
#  define USING_JUCE_FOR_VST3
# endif
# pragma GCC diagnostic pop
#endif

#define DISCOVERY_OUT(x, y) std::cout << "\ncarla-discovery::" << x << "::" << y << std::endl;

using water::File;

CARLA_BACKEND_USE_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Dummy values to test plugins with

static constexpr const uint32_t kBufferSize  = 512;
static constexpr const double   kSampleRate  = 44100.0;
static constexpr const int32_t  kSampleRatei = 44100;
static constexpr const float    kSampleRatef = 44100.0f;

// -------------------------------------------------------------------------------------------------------------------
// Don't print ELF/EXE related errors since discovery can find multi-architecture binaries

static void print_lib_error(const char* const filename)
{
    const char* const error = lib_error(filename);

    if (error != nullptr &&
        std::strstr(error, "wrong ELF class") == nullptr &&
        std::strstr(error, "invalid ELF header") == nullptr &&
        std::strstr(error, "Bad EXE format") == nullptr &&
        std::strstr(error, "no suitable image found") == nullptr &&
        std::strstr(error, "not a valid Win32 application") == nullptr)
    {
        DISCOVERY_OUT("error", error);
    }
}

// ------------------------------ Plugin Checks -----------------------------

#ifndef BUILD_BRIDGE
static void print_cached_plugin(const CarlaCachedPluginInfo* const pinfo)
{
    if (! pinfo->valid)
        return;

    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("hints", pinfo->hints);
    DISCOVERY_OUT("category", getPluginCategoryAsString(pinfo->category));
    DISCOVERY_OUT("name", pinfo->name);
    DISCOVERY_OUT("maker", pinfo->maker);
    DISCOVERY_OUT("label", pinfo->label);
    DISCOVERY_OUT("audio.ins", pinfo->audioIns);
    DISCOVERY_OUT("audio.outs", pinfo->audioOuts);
    DISCOVERY_OUT("cv.ins", pinfo->cvIns);
    DISCOVERY_OUT("cv.outs", pinfo->cvOuts);
    DISCOVERY_OUT("midi.ins", pinfo->midiIns);
    DISCOVERY_OUT("midi.outs", pinfo->midiOuts);
    DISCOVERY_OUT("parameters.ins", pinfo->parameterIns);
    DISCOVERY_OUT("parameters.outs", pinfo->parameterOuts);
    DISCOVERY_OUT("end", "------------");
}

static void do_cached_check(const PluginType type)
{
    const char* plugPath;

    switch (type)
    {
    case PLUGIN_LV2:
        plugPath = std::getenv("LV2_PATH");
        break;
    case PLUGIN_SFZ:
        plugPath = std::getenv("SFZ_PATH");
        break;
    default:
        plugPath = nullptr;
        break;
    }

   #ifdef USING_JUCE
    if (type == PLUGIN_AU)
        CarlaJUCE::initialiseJuce_GUI();
   #endif

    const uint count = carla_get_cached_plugin_count(type, plugPath);

    for (uint i=0; i<count; ++i)
    {
        const CarlaCachedPluginInfo* pinfo = carla_get_cached_plugin_info(type, i);
        CARLA_SAFE_ASSERT_CONTINUE(pinfo != nullptr);

        print_cached_plugin(pinfo);
    }

   #ifdef USING_JUCE
    if (type == PLUGIN_AU)
        CarlaJUCE::shutdownJuce_GUI();
   #endif
}
#endif

static void do_ladspa_check(lib_t& libHandle, const char* const filename, const bool doInit)
{
    LADSPA_Descriptor_Function descFn = lib_symbol<LADSPA_Descriptor_Function>(libHandle, "ladspa_descriptor");

    if (descFn == nullptr)
    {
        DISCOVERY_OUT("error", "Not a LADSPA plugin");
        return;
    }

    const LADSPA_Descriptor* descriptor;

    {
        descriptor = descFn(0);

        if (descriptor == nullptr)
        {
            DISCOVERY_OUT("error", "Binary doesn't contain any plugins");
            return;
        }

        if (doInit && descriptor->instantiate != nullptr && descriptor->cleanup != nullptr)
        {
            LADSPA_Handle handle = descriptor->instantiate(descriptor, kSampleRatei);

            if (handle == nullptr)
            {
                DISCOVERY_OUT("error", "Failed to init first LADSPA plugin");
                return;
            }

            descriptor->cleanup(handle);

            lib_close(libHandle);
            libHandle = lib_open(filename);

            if (libHandle == nullptr)
            {
                print_lib_error(filename);
                return;
            }

            descFn = lib_symbol<LADSPA_Descriptor_Function>(libHandle, "ladspa_descriptor");

            if (descFn == nullptr)
            {
                DISCOVERY_OUT("error", "Not a LADSPA plugin (#2)");
                return;
            }
        }
    }

    unsigned long i = 0;

    while ((descriptor = descFn(i++)) != nullptr)
    {
        if (descriptor->instantiate == nullptr)
        {
            DISCOVERY_OUT("error", "Plugin '" << descriptor->Name << "' has no instantiate()");
            continue;
        }
        if (descriptor->cleanup == nullptr)
        {
            DISCOVERY_OUT("error", "Plugin '" << descriptor->Name << "' has no cleanup()");
            continue;
        }
        if (descriptor->run == nullptr)
        {
            DISCOVERY_OUT("error", "Plugin '" << descriptor->Name << "' has no run()");
            continue;
        }
        if (! LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties))
        {
            DISCOVERY_OUT("warning", "Plugin '" << descriptor->Name << "' is not hard real-time capable");
        }

        uint hints = 0x0;
        uint audioIns = 0;
        uint audioOuts = 0;
        uint audioTotal = 0;
        uint parametersIns = 0;
        uint parametersOuts = 0;
        uint parametersTotal = 0;

        if (LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties))
            hints |= PLUGIN_IS_RTSAFE;

        for (unsigned long j=0; j < descriptor->PortCount; ++j)
        {
            CARLA_ASSERT(descriptor->PortNames[j] != nullptr);
            const LADSPA_PortDescriptor portDescriptor = descriptor->PortDescriptors[j];

            if (LADSPA_IS_PORT_AUDIO(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    audioIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor))
                    audioOuts += 1;

                audioTotal += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    parametersIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && std::strcmp(descriptor->PortNames[j], "latency") != 0 && std::strcmp(descriptor->PortNames[j], "_latency") != 0)
                    parametersOuts += 1;

                parametersTotal += 1;
            }
        }

        if (doInit)
        {
            // -----------------------------------------------------------------------
            // start crash-free plugin test

            LADSPA_Handle handle = descriptor->instantiate(descriptor, kSampleRatei);

            if (handle == nullptr)
            {
                DISCOVERY_OUT("error", "Failed to init LADSPA plugin");
                continue;
            }

            // Test quick init and cleanup
            descriptor->cleanup(handle);

            handle = descriptor->instantiate(descriptor, kSampleRatei);

            if (handle == nullptr)
            {
                DISCOVERY_OUT("error", "Failed to init LADSPA plugin (#2)");
                continue;
            }

            LADSPA_Data bufferAudio[kBufferSize][audioTotal];
            LADSPA_Data bufferParams[parametersTotal];
            LADSPA_Data min, max, def;

            for (unsigned long j=0, iA=0, iC=0; j < descriptor->PortCount; ++j)
            {
                const LADSPA_PortDescriptor portDescriptor = descriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  portRangeHints = descriptor->PortRangeHints[j];
                const char* const portName = descriptor->PortNames[j];

                if (LADSPA_IS_PORT_AUDIO(portDescriptor))
                {
                    carla_zeroFloats(bufferAudio[iA], kBufferSize);
                    descriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (LADSPA_IS_PORT_CONTROL(portDescriptor))
                {
                    // min value
                    if (LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHints.HintDescriptor))
                        min = portRangeHints.LowerBound;
                    else
                        min = 0.0f;

                    // max value
                    if (LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHints.HintDescriptor))
                        max = portRangeHints.UpperBound;
                    else
                        max = 1.0f;

                    if (min > max)
                    {
                        DISCOVERY_OUT("warning", "Parameter '" << portName << "' is broken: min > max");
                        max = min + 0.1f;
                    }
                    else if (carla_isEqual(min, max))
                    {
                        DISCOVERY_OUT("warning", "Parameter '" << portName << "' is broken: max == min");
                        max = min + 0.1f;
                    }

                    // default value
                    def = get_default_ladspa_port_value(portRangeHints.HintDescriptor, min, max);

                    if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                    {
                        min *= kSampleRatef;
                        max *= kSampleRatef;
                        def *= kSampleRatef;
                    }

                    if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && (std::strcmp(portName, "latency") == 0 || std::strcmp(portName, "_latency") == 0))
                    {
                        // latency parameter
                        def = 0.0f;
                    }
                    else
                    {
                        if (def < min)
                            def = min;
                        else if (def > max)
                            def = max;
                    }

                    bufferParams[iC] = def;
                    descriptor->connect_port(handle, j, &bufferParams[iC++]);
                }
            }

            if (descriptor->activate != nullptr)
                descriptor->activate(handle);

            descriptor->run(handle, kBufferSize);

            if (descriptor->deactivate != nullptr)
                descriptor->deactivate(handle);

            descriptor->cleanup(handle);

            // end crash-free plugin test
            // -----------------------------------------------------------------------
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("category", getPluginCategoryAsString(getPluginCategoryFromName(descriptor->Name)));
        DISCOVERY_OUT("name", descriptor->Name);
        DISCOVERY_OUT("label", descriptor->Label);
        DISCOVERY_OUT("maker", descriptor->Maker);
        DISCOVERY_OUT("uniqueId", descriptor->UniqueID);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("end", "------------");
    }
}

static void do_dssi_check(lib_t& libHandle, const char* const filename, const bool doInit)
{
    DSSI_Descriptor_Function descFn = lib_symbol<DSSI_Descriptor_Function>(libHandle, "dssi_descriptor");

    if (descFn == nullptr)
    {
        DISCOVERY_OUT("error", "Not a DSSI plugin");
        return;
    }

    const DSSI_Descriptor* descriptor;

    {
        descriptor = descFn(0);

        if (descriptor == nullptr)
        {
            DISCOVERY_OUT("error", "Binary doesn't contain any plugins");
            return;
        }

        const LADSPA_Descriptor* const ldescriptor(descriptor->LADSPA_Plugin);

        if (ldescriptor == nullptr)
        {
            DISCOVERY_OUT("error", "DSSI plugin doesn't provide the LADSPA interface");
            return;
        }

        if (doInit && ldescriptor->instantiate != nullptr && ldescriptor->cleanup != nullptr)
        {
            LADSPA_Handle handle = ldescriptor->instantiate(ldescriptor, kSampleRatei);

            if (handle == nullptr)
            {
                DISCOVERY_OUT("error", "Failed to init first LADSPA plugin");
                return;
            }

            ldescriptor->cleanup(handle);

            lib_close(libHandle);
            libHandle = lib_open(filename);

            if (libHandle == nullptr)
            {
                print_lib_error(filename);
                return;
            }

            descFn = lib_symbol<DSSI_Descriptor_Function>(libHandle, "dssi_descriptor");

            if (descFn == nullptr)
            {
                DISCOVERY_OUT("error", "Not a DSSI plugin (#2)");
                return;
            }
        }
    }

    unsigned long i = 0;

    while ((descriptor = descFn(i++)) != nullptr)
    {
        const LADSPA_Descriptor* const ldescriptor = descriptor->LADSPA_Plugin;

        if (ldescriptor == nullptr)
        {
            DISCOVERY_OUT("error", "Plugin has no LADSPA interface");
            continue;
        }
        if (descriptor->DSSI_API_Version != DSSI_VERSION_MAJOR)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' uses an unsupported DSSI spec version " << descriptor->DSSI_API_Version);
            continue;
        }
        if (ldescriptor->instantiate == nullptr)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' has no instantiate()");
            continue;
        }
        if (ldescriptor->cleanup == nullptr)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' has no cleanup()");
            continue;
        }
        if (ldescriptor->run == nullptr && descriptor->run_synth == nullptr)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' has no run() or run_synth()");
            continue;
        }
        if (descriptor->run_synth == nullptr && descriptor->run_multiple_synths != nullptr)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' requires run_multiple_synths which is not supported");
            continue;
        }
        if (! LADSPA_IS_HARD_RT_CAPABLE(ldescriptor->Properties))
        {
            DISCOVERY_OUT("warning", "Plugin '" << ldescriptor->Name << "' is not hard real-time capable");
        }

        uint hints = 0x0;
        uint audioIns = 0;
        uint audioOuts = 0;
        uint audioTotal = 0;
        uint midiIns = 0;
        uint parametersIns = 0;
        uint parametersOuts = 0;
        uint parametersTotal = 0;

        if (LADSPA_IS_HARD_RT_CAPABLE(ldescriptor->Properties))
            hints |= PLUGIN_IS_RTSAFE;

        for (unsigned long j=0; j < ldescriptor->PortCount; ++j)
        {
            CARLA_ASSERT(ldescriptor->PortNames[j] != nullptr);
            const LADSPA_PortDescriptor portDescriptor = ldescriptor->PortDescriptors[j];

            if (LADSPA_IS_PORT_AUDIO(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    audioIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor))
                    audioOuts += 1;

                audioTotal += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    parametersIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && std::strcmp(ldescriptor->PortNames[j], "latency") != 0 && std::strcmp(ldescriptor->PortNames[j], "_latency") != 0)
                    parametersOuts += 1;

                parametersTotal += 1;
            }
        }

        if (descriptor->run_synth != nullptr)
            midiIns = 1;

        if (midiIns > 0 && audioIns == 0 && audioOuts > 0)
            hints |= PLUGIN_IS_SYNTH;

#ifndef BUILD_BRIDGE
        if (const char* const ui = find_dssi_ui(filename, ldescriptor->Label))
        {
            hints |= PLUGIN_HAS_CUSTOM_UI;
            delete[] ui;
        }
#endif

        if (doInit)
        {
            // -----------------------------------------------------------------------
            // start crash-free plugin test

            LADSPA_Handle handle = ldescriptor->instantiate(ldescriptor, kSampleRatei);

            if (handle == nullptr)
            {
                DISCOVERY_OUT("error", "Failed to init DSSI plugin");
                continue;
            }

            // Test quick init and cleanup
            ldescriptor->cleanup(handle);

            handle = ldescriptor->instantiate(ldescriptor, kSampleRatei);

            if (handle == nullptr)
            {
                DISCOVERY_OUT("error", "Failed to init DSSI plugin (#2)");
                continue;
            }

            LADSPA_Data bufferAudio[kBufferSize][audioTotal];
            LADSPA_Data bufferParams[parametersTotal];
            LADSPA_Data min, max, def;

            for (unsigned long j=0, iA=0, iC=0; j < ldescriptor->PortCount; ++j)
            {
                const LADSPA_PortDescriptor portDescriptor = ldescriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  portRangeHints = ldescriptor->PortRangeHints[j];
                const char* const portName = ldescriptor->PortNames[j];

                if (LADSPA_IS_PORT_AUDIO(portDescriptor))
                {
                    carla_zeroFloats(bufferAudio[iA], kBufferSize);
                    ldescriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (LADSPA_IS_PORT_CONTROL(portDescriptor))
                {
                    // min value
                    if (LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHints.HintDescriptor))
                        min = portRangeHints.LowerBound;
                    else
                        min = 0.0f;

                    // max value
                    if (LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHints.HintDescriptor))
                        max = portRangeHints.UpperBound;
                    else
                        max = 1.0f;

                    if (min > max)
                    {
                        DISCOVERY_OUT("warning", "Parameter '" << portName << "' is broken: min > max");
                        max = min + 0.1f;
                    }
                    else if (carla_isEqual(min, max))
                    {
                        DISCOVERY_OUT("warning", "Parameter '" << portName << "' is broken: max == min");
                        max = min + 0.1f;
                    }

                    // default value
                    def = get_default_ladspa_port_value(portRangeHints.HintDescriptor, min, max);

                    if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                    {
                        min *= kSampleRatef;
                        max *= kSampleRatef;
                        def *= kSampleRatef;
                    }

                    if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && (std::strcmp(portName, "latency") == 0 || std::strcmp(portName, "_latency") == 0))
                    {
                        // latency parameter
                        def = 0.0f;
                    }
                    else
                    {
                        if (def < min)
                            def = min;
                        else if (def > max)
                            def = max;
                    }

                    bufferParams[iC] = def;
                    ldescriptor->connect_port(handle, j, &bufferParams[iC++]);
                }
            }

            // select first midi-program if available
            if (descriptor->get_program != nullptr && descriptor->select_program != nullptr)
            {
                if (const DSSI_Program_Descriptor* const pDesc = descriptor->get_program(handle, 0))
                    descriptor->select_program(handle, pDesc->Bank, pDesc->Program);
            }

            if (ldescriptor->activate != nullptr)
                ldescriptor->activate(handle);

            if (descriptor->run_synth != nullptr)
            {
                snd_seq_event_t midiEvents[2];
                carla_zeroStructs(midiEvents, 2);

                const unsigned long midiEventCount = 2;

                midiEvents[0].type = SND_SEQ_EVENT_NOTEON;
                midiEvents[0].data.note.note     = 64;
                midiEvents[0].data.note.velocity = 100;

                midiEvents[1].type = SND_SEQ_EVENT_NOTEOFF;
                midiEvents[1].data.note.note     = 64;
                midiEvents[1].data.note.velocity = 0;
                midiEvents[1].time.tick = kBufferSize/2;

                descriptor->run_synth(handle, kBufferSize, midiEvents, midiEventCount);
            }
            else
                ldescriptor->run(handle, kBufferSize);

            if (ldescriptor->deactivate != nullptr)
                ldescriptor->deactivate(handle);

            ldescriptor->cleanup(handle);

            // end crash-free plugin test
            // -----------------------------------------------------------------------
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("category", ((hints & PLUGIN_IS_SYNTH)
                                   ? "synth"
                                   : getPluginCategoryAsString(getPluginCategoryFromName(ldescriptor->Name))));
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("name", ldescriptor->Name);
        DISCOVERY_OUT("label", ldescriptor->Label);
        DISCOVERY_OUT("maker", ldescriptor->Maker);
        DISCOVERY_OUT("uniqueId", ldescriptor->UniqueID);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("end", "------------");
    }
}

#ifndef BUILD_BRIDGE
static void do_lv2_check(const char* const bundle, const bool doInit)
{
    Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

    Lilv::Node bundleNode(lv2World.new_file_uri(nullptr, bundle));
    CARLA_SAFE_ASSERT_RETURN(bundleNode.is_uri(),);

    CarlaString sBundle(bundleNode.as_uri());

    if (! sBundle.endsWith("/"))
        sBundle += "/";

    // Load bundle
    lv2World.load_bundle(sBundle);

    // Load plugins in this bundle
    const Lilv::Plugins lilvPlugins(lv2World.get_all_plugins());

    // Get all plugin URIs in this bundle
    CarlaStringList URIs;

    LILV_FOREACH(plugins, it, lilvPlugins)
    {
        Lilv::Plugin lilvPlugin(lilv_plugins_get(lilvPlugins, it));

        if (const char* const uri = lilvPlugin.get_uri().as_string())
            URIs.appendUnique(uri);
    }

    if (URIs.isEmpty())
    {
        DISCOVERY_OUT("warning", "LV2 Bundle doesn't provide any plugins");
        return;
    }

    // Get & check every plugin-instance
    for (CarlaStringList::Itenerator it=URIs.begin2(); it.valid(); it.next())
    {
        const char* const URI = it.getValue(nullptr);
        CARLA_SAFE_ASSERT_CONTINUE(URI != nullptr);

        CarlaScopedPointer<const LV2_RDF_Descriptor> rdfDescriptor(lv2_rdf_new(URI, false));

        if (rdfDescriptor == nullptr || rdfDescriptor->URI == nullptr)
        {
            DISCOVERY_OUT("error", "Failed to find LV2 plugin '" << URI << "'");
            continue;
        }

        if (doInit)
        {
            // test if lib is loadable, twice
            const lib_t libHandle1 = lib_open(rdfDescriptor->Binary);

            if (libHandle1 == nullptr)
            {
                print_lib_error(rdfDescriptor->Binary);
                delete rdfDescriptor;
                continue;
            }

            lib_close(libHandle1);

            const lib_t libHandle2 = lib_open(rdfDescriptor->Binary);

            if (libHandle2 == nullptr)
            {
                print_lib_error(rdfDescriptor->Binary);
                delete rdfDescriptor;
                continue;
            }

            lib_close(libHandle2);
        }

        const LilvPlugin* const cPlugin(lv2World.getPluginFromURI(URI));
        CARLA_SAFE_ASSERT_CONTINUE(cPlugin != nullptr);

        Lilv::Plugin lilvPlugin(cPlugin);
        CARLA_SAFE_ASSERT_CONTINUE(lilvPlugin.get_uri().is_uri());

        print_cached_plugin(get_cached_plugin_lv2(lv2World, lilvPlugin));
    }
}
#endif

#ifndef USING_JUCE_FOR_VST2
// -------------------------------------------------------------------------------------------------------------------
// VST stuff

// Check if plugin is currently processing
static bool gVstIsProcessing = false;

// Check if plugin needs idle
static bool gVstNeedsIdle = false;

// Check if plugin wants midi
static bool gVstWantsMidi = false;

// Check if plugin wants time
static bool gVstWantsTime = false;

// Current uniqueId for VST shell plugins
static intptr_t gVstCurrentUniqueId = 0;

// Supported Carla features
static intptr_t vstHostCanDo(const char* const feature)
{
    carla_debug("vstHostCanDo(\"%s\")", feature);

    if (std::strcmp(feature, "supplyIdle") == 0)
        return 1;
    if (std::strcmp(feature, "sendVstEvents") == 0)
        return 1;
    if (std::strcmp(feature, "sendVstMidiEvent") == 0)
        return 1;
    if (std::strcmp(feature, "sendVstMidiEventFlagIsRealtime") == 0)
        return 1;
    if (std::strcmp(feature, "sendVstTimeInfo") == 0)
    {
        gVstWantsTime = true;
        return 1;
    }
    if (std::strcmp(feature, "receiveVstEvents") == 0)
        return 1;
    if (std::strcmp(feature, "receiveVstMidiEvent") == 0)
        return 1;
    if (std::strcmp(feature, "receiveVstTimeInfo") == 0)
        return -1;
    if (std::strcmp(feature, "reportConnectionChanges") == 0)
        return -1;
    if (std::strcmp(feature, "acceptIOChanges") == 0)
        return 1;
    if (std::strcmp(feature, "sizeWindow") == 0)
        return 1;
    if (std::strcmp(feature, "offline") == 0)
        return -1;
    if (std::strcmp(feature, "openFileSelector") == 0)
        return -1;
    if (std::strcmp(feature, "closeFileSelector") == 0)
        return -1;
    if (std::strcmp(feature, "startStopProcess") == 0)
        return 1;
    if (std::strcmp(feature, "supportShell") == 0)
        return 1;
    if (std::strcmp(feature, "shellCategory") == 0)
        return 1;
    if (std::strcmp(feature, "NIMKPIVendorSpecificCallbacks") == 0)
        return -1;

    // non-official features found in some plugins:
    // "asyncProcessing"
    // "editFile"

    // unimplemented
    carla_stderr("vstHostCanDo(\"%s\") - unknown feature", feature);
    return 0;
}

// Host-side callback
static intptr_t VSTCALLBACK vstHostCallback(AEffect* const effect, const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
{
    carla_debug("vstHostCallback(%p, %i:%s, %i, " P_INTPTR ", %p, %f)",
                effect, opcode, vstMasterOpcode2str(opcode), index, value, ptr, static_cast<double>(opt));

    static VstTimeInfo timeInfo;
    intptr_t ret = 0;

    switch (opcode)
    {
    case audioMasterAutomate:
        ret = 1;
        break;

    case audioMasterVersion:
        ret = kVstVersion;
        break;

    case audioMasterCurrentId:
        ret = gVstCurrentUniqueId;
        break;

    case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
        if (gVstWantsMidi) { DISCOVERY_OUT("warning", "Plugin requested MIDI more than once"); }

        gVstWantsMidi = true;
        ret = 1;
        break;

    case audioMasterGetTime:
        if (! gVstIsProcessing) { DISCOVERY_OUT("warning", "Plugin requested timeInfo out of process"); }
        if (! gVstWantsTime)    { DISCOVERY_OUT("warning", "Plugin requested timeInfo but didn't ask if host could do \"sendVstTimeInfo\""); }

        carla_zeroStruct(timeInfo);
        timeInfo.sampleRate = kSampleRate;

        // Tempo
        timeInfo.tempo  = 120.0;
        timeInfo.flags |= kVstTempoValid;

        // Time Signature
        timeInfo.timeSigNumerator   = 4;
        timeInfo.timeSigDenominator = 4;
        timeInfo.flags |= kVstTimeSigValid;

        ret = (intptr_t)&timeInfo;
        break;

    case DECLARE_VST_DEPRECATED(audioMasterTempoAt):
        ret = 120 * 10000;
        break;

    case DECLARE_VST_DEPRECATED(audioMasterGetNumAutomatableParameters):
        ret = carla_minPositive(effect->numParams, static_cast<int>(MAX_DEFAULT_PARAMETERS));
        break;

    case DECLARE_VST_DEPRECATED(audioMasterGetParameterQuantization):
        ret = 1; // full single float precision
        break;

    case DECLARE_VST_DEPRECATED(audioMasterNeedIdle):
        if (gVstNeedsIdle) { DISCOVERY_OUT("warning", "Plugin requested idle more than once"); }

        gVstNeedsIdle = true;
        ret = 1;
        break;

    case audioMasterGetSampleRate:
        ret = kSampleRatei;
        break;

    case audioMasterGetBlockSize:
        ret = kBufferSize;
        break;

    case DECLARE_VST_DEPRECATED(audioMasterWillReplaceOrAccumulate):
        ret = 1; // replace
        break;

    case audioMasterGetCurrentProcessLevel:
        ret = gVstIsProcessing ? kVstProcessLevelRealtime : kVstProcessLevelUser;
        break;

    case audioMasterGetAutomationState:
        ret = kVstAutomationOff;
        break;

    case audioMasterGetVendorString:
        CARLA_SAFE_ASSERT_BREAK(ptr != nullptr);
        std::strcpy((char*)ptr, "falkTX");
        ret = 1;
        break;

    case audioMasterGetProductString:
        CARLA_SAFE_ASSERT_BREAK(ptr != nullptr);
        std::strcpy((char*)ptr, "Carla-Discovery");
        ret = 1;
        break;

    case audioMasterGetVendorVersion:
        ret = CARLA_VERSION_HEX;
        break;

    case audioMasterCanDo:
        CARLA_SAFE_ASSERT_BREAK(ptr != nullptr);
        ret = vstHostCanDo((const char*)ptr);
        break;

    case audioMasterGetLanguage:
        ret = kVstLangEnglish;
        break;

    default:
        carla_stdout("vstHostCallback(%p, %i:%s, %i, " P_INTPTR ", %p, %f)",
                     effect, opcode, vstMasterOpcode2str(opcode), index, value, ptr, static_cast<double>(opt));
        break;
    }

    return ret;
}

static void do_vst2_check(lib_t& libHandle, const char* const filename, const bool doInit)
{
    VST_Function vstFn = nullptr;

#ifdef CARLA_OS_MAC
    BundleLoader bundleLoader;

    if (libHandle == nullptr)
    {
        if (! bundleLoader.load(filename))
        {
            DISCOVERY_OUT("error", "Failed to load VST2 bundle executable");
            return;
        }

        vstFn = bundleLoader.getSymbol<VST_Function>(CFSTR("main_macho"));

        if (vstFn == nullptr)
            vstFn = bundleLoader.getSymbol<VST_Function>(CFSTR("VSTPluginMain"));

        if (vstFn == nullptr)
        {
            DISCOVERY_OUT("error", "Not a VST2 plugin");
            return;
        }
    }
    else
#endif
    {
        vstFn = lib_symbol<VST_Function>(libHandle, "VSTPluginMain");

        if (vstFn == nullptr)
        {
            vstFn = lib_symbol<VST_Function>(libHandle, "main");

            if (vstFn == nullptr)
            {
                DISCOVERY_OUT("error", "Not a VST plugin");
                return;
            }
        }
    }

    AEffect* effect = vstFn(vstHostCallback);

    if (effect == nullptr || effect->magic != kEffectMagic)
    {
        DISCOVERY_OUT("error", "Failed to init VST plugin, or VST magic failed");
        return;
    }

    if (effect->uniqueID == 0)
    {
        DISCOVERY_OUT("warning", "Plugin doesn't have an Unique ID when first loaded");
    }

    effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdentify), 0, 0, nullptr, 0.0f);
    effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effSetBlockSizeAndSampleRate), 0, kBufferSize, nullptr, kSampleRatef);
    effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, kSampleRatef);
    effect->dispatcher(effect, effSetBlockSize, 0, kBufferSize, nullptr, 0.0f);
    effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

    effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

    if (effect->numPrograms > 0)
        effect->dispatcher(effect, effSetProgram, 0, 0, nullptr, 0.0f);

    const bool isShell = (effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f) == kPlugCategShell);

    if (effect->uniqueID == 0 && !isShell)
    {
        DISCOVERY_OUT("error", "Plugin doesn't have an Unique ID after being open");
        effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        return;
    }

    gVstCurrentUniqueId =  effect->uniqueID;

    char strBuf[STR_MAX+1];
    CarlaString cName;
    CarlaString cProduct;
    CarlaString cVendor;
    PluginCategory category;
    LinkedList<intptr_t> uniqueIds;

    if (isShell)
    {
        for (;;)
        {
            carla_zeroChars(strBuf, STR_MAX+1);

            gVstCurrentUniqueId = effect->dispatcher(effect, effShellGetNextPlugin, 0, 0, strBuf, 0.0f);

            if (gVstCurrentUniqueId == 0)
                break;

            uniqueIds.append(gVstCurrentUniqueId);
        }

        effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        effect = nullptr;
    }
    else
    {
        uniqueIds.append(gVstCurrentUniqueId);
    }

    for (LinkedList<intptr_t>::Itenerator it = uniqueIds.begin2(); it.valid(); it.next())
    {
        gVstCurrentUniqueId = it.getValue(0);

        if (effect == nullptr)
        {
            effect = vstFn(vstHostCallback);

            effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdentify), 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effSetBlockSizeAndSampleRate), 0, kBufferSize, nullptr, kSampleRatef);
            effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, kSampleRatef);
            effect->dispatcher(effect, effSetBlockSize, 0, kBufferSize, nullptr, 0.0f);
            effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

            effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

            if (effect->numPrograms > 0)
                effect->dispatcher(effect, effSetProgram, 0, 0, nullptr, 0.0f);
        }

        // get name
        carla_zeroChars(strBuf, STR_MAX+1);

        if (effect->dispatcher(effect, effGetEffectName, 0, 0, strBuf, 0.0f) == 1)
            cName = strBuf;
        else
            cName.clear();

        // get product
        carla_zeroChars(strBuf, STR_MAX+1);

        if (effect->dispatcher(effect, effGetProductString, 0, 0, strBuf, 0.0f) == 1)
            cProduct = strBuf;
        else
            cProduct.clear();

        // get vendor
        carla_zeroChars(strBuf, STR_MAX+1);

        if (effect->dispatcher(effect, effGetVendorString, 0, 0, strBuf, 0.0f) == 1)
            cVendor = strBuf;
        else
            cVendor.clear();

        // get category
        switch (effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f))
        {
        case kPlugCategSynth:
            category = PLUGIN_CATEGORY_SYNTH;
            break;
        case kPlugCategAnalysis:
            category = PLUGIN_CATEGORY_UTILITY;
            break;
        case kPlugCategMastering:
            category = PLUGIN_CATEGORY_DYNAMICS;
            break;
        case kPlugCategRoomFx:
            category = PLUGIN_CATEGORY_DELAY;
            break;
        case kPlugCategRestoration:
            category = PLUGIN_CATEGORY_UTILITY;
            break;
        case kPlugCategGenerator:
            category = PLUGIN_CATEGORY_SYNTH;
            break;
        default:
            if (effect->flags & effFlagsIsSynth)
                category = PLUGIN_CATEGORY_SYNTH;
            else
                category = PLUGIN_CATEGORY_NONE;
            break;
        }

        // get everything else
        uint hints = 0x0;
        uint audioIns = static_cast<uint>(std::max(0, effect->numInputs));
        uint audioOuts = static_cast<uint>(std::max(0, effect->numOutputs));
        uint midiIns = 0;
        uint midiOuts = 0;
        uint parameters = static_cast<uint>(std::max(0, effect->numParams));

        if (effect->flags & effFlagsHasEditor)
            hints |= PLUGIN_HAS_CUSTOM_UI;

        if (effect->flags & effFlagsIsSynth)
        {
            hints |= PLUGIN_IS_SYNTH;
            midiIns = 1;
        }

        if (vstPluginCanDo(effect, "receiveVstEvents") || vstPluginCanDo(effect, "receiveVstMidiEvent") || (effect->flags & effFlagsIsSynth) != 0)
            midiIns = 1;

        if (vstPluginCanDo(effect, "sendVstEvents") || vstPluginCanDo(effect, "sendVstMidiEvent"))
            midiOuts = 1;

        // -----------------------------------------------------------------------
        // start crash-free plugin test

        if (doInit)
        {
            if (gVstNeedsIdle)
                effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdle), 0, 0, nullptr, 0.0f);

            effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
            effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);

            if (gVstNeedsIdle)
                effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdle), 0, 0, nullptr, 0.0f);

            // Plugin might call wantMidi() during resume
            if (midiIns == 0 && gVstWantsMidi)
            {
                midiIns = 1;
            }

            float* bufferAudioIn[std::max(1U, audioIns)];
            float* bufferAudioOut[std::max(1U, audioOuts)];

            if (audioIns == 0)
            {
                bufferAudioIn[0] = nullptr;
            }
            else
            {
                for (uint j=0; j < audioIns; ++j)
                {
                    bufferAudioIn[j] = new float[kBufferSize];
                    carla_zeroFloats(bufferAudioIn[j], kBufferSize);
                }
            }

            if (audioOuts == 0)
            {
                bufferAudioOut[0] = nullptr;
            }
            else
            {
                for (uint j=0; j < audioOuts; ++j)
                {
                    bufferAudioOut[j] = new float[kBufferSize];
                    carla_zeroFloats(bufferAudioOut[j], kBufferSize);
                }
            }

            struct VstEventsFixed {
                int32_t numEvents;
                intptr_t reserved;
                VstEvent* data[2];

                VstEventsFixed()
                    : numEvents(0),
                      reserved(0)
                {
                    data[0] = data[1] = nullptr;
                }
            } events;

            VstMidiEvent midiEvents[2];
            carla_zeroStructs(midiEvents, 2);

            midiEvents[0].type = kVstMidiType;
            midiEvents[0].byteSize = sizeof(VstMidiEvent);
            midiEvents[0].midiData[0] = char(MIDI_STATUS_NOTE_ON);
            midiEvents[0].midiData[1] = 64;
            midiEvents[0].midiData[2] = 100;

            midiEvents[1].type = kVstMidiType;
            midiEvents[1].byteSize = sizeof(VstMidiEvent);
            midiEvents[1].midiData[0] = char(MIDI_STATUS_NOTE_OFF);
            midiEvents[1].midiData[1] = 64;
            midiEvents[1].deltaFrames = kBufferSize/2;

            events.numEvents = 2;
            events.data[0] = (VstEvent*)&midiEvents[0];
            events.data[1] = (VstEvent*)&midiEvents[1];

            // processing
            gVstIsProcessing = true;

            if (midiIns > 0)
                effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);

            if ((effect->flags & effFlagsCanReplacing) > 0 && effect->processReplacing != nullptr && effect->processReplacing != effect->DECLARE_VST_DEPRECATED(process))
                effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, kBufferSize);
            else if (effect->DECLARE_VST_DEPRECATED(process) != nullptr)
                effect->DECLARE_VST_DEPRECATED(process)(effect, bufferAudioIn, bufferAudioOut, kBufferSize);
            else
                DISCOVERY_OUT("error", "Plugin doesn't have a process function");

            gVstIsProcessing = false;

            effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

            if (gVstNeedsIdle)
                effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdle), 0, 0, nullptr, 0.0f);

            for (uint j=0; j < audioIns; ++j)
                delete[] bufferAudioIn[j];
            for (uint j=0; j < audioOuts; ++j)
                delete[] bufferAudioOut[j];
        }

        // end crash-free plugin test
        // -----------------------------------------------------------------------

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("category", getPluginCategoryAsString(category));
        DISCOVERY_OUT("name", cName.buffer());
        DISCOVERY_OUT("label", cProduct.buffer());
        DISCOVERY_OUT("maker", cVendor.buffer());
        DISCOVERY_OUT("uniqueId", gVstCurrentUniqueId);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.outs", midiOuts);
        DISCOVERY_OUT("parameters.ins", parameters);
        DISCOVERY_OUT("end", "------------");

        gVstWantsMidi = false;
        gVstWantsTime = false;

        if (! isShell)
            break;

        effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        effect = nullptr;
    }

    uniqueIds.clear();

    if (effect != nullptr)
    {
        if (gVstNeedsIdle)
            effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdle), 0, 0, nullptr, 0.0f);

        effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
    }

#ifndef CARLA_OS_MAC
    return;

    // unused
    (void)filename;
#endif
}
#endif // ! USING_JUCE_FOR_VST2

#ifndef USING_JUCE_FOR_VST3
static uint32_t V3_API v3_ref_static(void*) { return 1; }
static uint32_t V3_API v3_unref_static(void*) { return 0; }

struct carla_v3_host_application : v3_host_application_cpp {
    carla_v3_host_application()
    {
        query_interface = carla_query_interface;
        ref = v3_ref_static;
        unref = v3_unref_static;
        app.get_name = carla_get_name;
        app.create_instance = carla_create_instance;
    }

    static v3_result V3_API carla_query_interface(void* const self, const v3_tuid iid, void** const iface)
    {
        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_host_application_iid))
        {
            *iface = self;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static v3_result V3_API carla_get_name(void*, v3_str_128 name)
    {
        static const char hostname[] = "Carla-Discovery\0";

        for (size_t i=0; i<sizeof(hostname); ++i)
            name[i] = hostname[i];

        return V3_OK;
    }

    static v3_result V3_API carla_create_instance(void*, v3_tuid, v3_tuid, void**) { return V3_NOT_IMPLEMENTED; }
};

struct carla_v3_param_changes : v3_param_changes_cpp {
    carla_v3_param_changes()
    {
        query_interface = carla_query_interface;
        ref = v3_ref_static;
        unref = v3_unref_static;
        changes.get_param_count = carla_get_param_count;
        changes.get_param_data = carla_get_param_data;
        changes.add_param_data = carla_add_param_data;
    }

    static v3_result V3_API carla_query_interface(void* const self, const v3_tuid iid, void** const iface)
    {
        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_param_changes_iid))
        {
            *iface = self;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static int32_t V3_API carla_get_param_count(void*) { return 0; }
    static v3_param_value_queue** V3_API carla_get_param_data(void*, int32_t) { return nullptr; }
    static v3_param_value_queue** V3_API carla_add_param_data(void*, v3_param_id*, int32_t*) { return nullptr; }
};

struct carla_v3_event_list : v3_event_list_cpp {
    carla_v3_event_list()
    {
        query_interface = carla_query_interface;
        ref = v3_ref_static;
        unref = v3_unref_static;
        list.get_event_count = carla_get_event_count;
        list.get_event = carla_get_event;
        list.add_event = carla_add_event;
    }

    static v3_result V3_API carla_query_interface(void* const self, const v3_tuid iid, void** const iface)
    {
        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_event_list_iid))
        {
            *iface = self;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static uint32_t V3_API carla_get_event_count(void*) { return 0; }
    static v3_result V3_API carla_get_event(void*, int32_t, v3_event*) { return V3_NOT_IMPLEMENTED; }
    static v3_result V3_API carla_add_event(void*, v3_event*) { return V3_NOT_IMPLEMENTED; }
};

static void do_vst3_check(lib_t& libHandle, const char* const filename, const bool doInit)
{
    V3_ENTRYFN v3_entry = nullptr;
    V3_EXITFN v3_exit = nullptr;
    V3_GETFN v3_get = nullptr;

#ifdef CARLA_OS_MAC
    BundleLoader bundleLoader;
#endif

    // if passed filename is not a plugin binary directly, inspect bundle and find one
    if (libHandle == nullptr)
    {
#ifdef CARLA_OS_MAC
        if (! bundleLoader.load(filename))
        {
            DISCOVERY_OUT("error", "Failed to load VST3 bundle executable");
            return;
        }

        v3_entry = bundleLoader.getSymbol<V3_ENTRYFN>(CFSTR(V3_ENTRYFNNAME));
        v3_exit = bundleLoader.getSymbol<V3_EXITFN>(CFSTR(V3_EXITFNNAME));
        v3_get = bundleLoader.getSymbol<V3_GETFN>(CFSTR(V3_GETFNNAME));
#else
        water::String binaryfilename = filename;

        if (!binaryfilename.endsWithChar(CARLA_OS_SEP))
            binaryfilename += CARLA_OS_SEP_STR;

        binaryfilename += "Contents" CARLA_OS_SEP_STR V3_CONTENT_DIR CARLA_OS_SEP_STR;
        binaryfilename += water::File(filename).getFileNameWithoutExtension();
# ifdef CARLA_OS_WIN
        binaryfilename += ".vst3";
# else
        binaryfilename += ".so";
# endif

        if (! water::File(binaryfilename).existsAsFile())
        {
            DISCOVERY_OUT("error", "Failed to find a suitable VST3 bundle binary");
            return;
        }

        libHandle = lib_open(binaryfilename.toRawUTF8());

        if (libHandle == nullptr)
        {
            print_lib_error(filename);
            return;
        }
#endif
    }

#ifndef CARLA_OS_MAC
    v3_entry = lib_symbol<V3_ENTRYFN>(libHandle, V3_ENTRYFNNAME);
    v3_exit = lib_symbol<V3_EXITFN>(libHandle, V3_EXITFNNAME);
    v3_get = lib_symbol<V3_GETFN>(libHandle, V3_GETFNNAME);
#endif

    // ensure entry and exit points are available
    if (v3_entry == nullptr || v3_exit == nullptr || v3_get == nullptr)
    {
        DISCOVERY_OUT("error", "Not a VST3 plugin");
        return;
    }

    // call entry point
#if defined(CARLA_OS_MAC)
    v3_entry(bundleLoader.getRef());
#elif defined(CARLA_OS_WIN)
    v3_entry();
#else
    v3_entry(libHandle);
#endif

    carla_v3_host_application hostApplication;
    carla_v3_host_application* hostApplicationPtr = &hostApplication;
    v3_funknown** const hostContext = (v3_funknown**)&hostApplicationPtr;

    // fetch initial factory
    v3_plugin_factory** factory1 = v3_get();
    CARLA_SAFE_ASSERT_RETURN(factory1 != nullptr, v3_exit());

    // get factory info
    v3_factory_info factoryInfo = {};
    CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj(factory1)->get_factory_info(factory1, &factoryInfo) == V3_OK, v3_exit());

    // get num classes
    const int32_t numClasses = v3_cpp_obj(factory1)->num_classes(factory1);
    CARLA_SAFE_ASSERT_RETURN(numClasses > 0, v3_exit());

    // query 2nd factory
    v3_plugin_factory_2** factory2 = nullptr;
    if (v3_cpp_obj_query_interface(factory1, v3_plugin_factory_2_iid, &factory2) == V3_OK)
    {
        CARLA_SAFE_ASSERT_RETURN(factory2 != nullptr, v3_exit());
    }
    else
    {
        CARLA_SAFE_ASSERT(factory2 == nullptr);
        factory2 = nullptr;
    }

    // query 3rd factory
    v3_plugin_factory_3** factory3 = nullptr;
    if (factory2 != nullptr && v3_cpp_obj_query_interface(factory2, v3_plugin_factory_3_iid, &factory3) == V3_OK)
    {
        CARLA_SAFE_ASSERT_RETURN(factory3 != nullptr, v3_exit());
    }
    else
    {
        CARLA_SAFE_ASSERT(factory3 == nullptr);
        factory3 = nullptr;
    }

    // set host context (application) if 3rd factory provided
    if (factory3 != nullptr)
        v3_cpp_obj(factory3)->set_host_context(factory3, hostContext);

    // go through all relevant classes
    for (int32_t i=0; i<numClasses; ++i)
    {
        // v3_class_info_2 is ABI compatible with v3_class_info
        union {
            v3_class_info v1;
            v3_class_info_2 v2;
        } classInfo = {};

        if (factory2 != nullptr)
            v3_cpp_obj(factory2)->get_class_info_2(factory2, i, &classInfo.v2);
        else
            v3_cpp_obj(factory1)->get_class_info(factory1, i, &classInfo.v1);

        // safety check
        CARLA_SAFE_ASSERT_CONTINUE(classInfo.v1.cardinality == 0x7FFFFFFF);

        // only check for audio plugins
        if (std::strcmp(classInfo.v1.category, "Audio Module Class") != 0)
            continue;

        // create instance
        void* instance = nullptr;
        CARLA_SAFE_ASSERT_CONTINUE(v3_cpp_obj(factory1)->create_instance(factory1, classInfo.v1.class_id, v3_component_iid, &instance) == V3_OK);
        CARLA_SAFE_ASSERT_CONTINUE(instance != nullptr);

        // initialize instance
        v3_component** const component = static_cast<v3_component**>(instance);

        CARLA_SAFE_ASSERT_CONTINUE(v3_cpp_obj_initialize(component, hostContext) == V3_OK);

        // create edit controller
        v3_edit_controller** controller = nullptr;
        bool shouldTerminateController;
        if (v3_cpp_obj_query_interface(component, v3_edit_controller_iid, &controller) != V3_OK)
            controller = nullptr;

        if (controller != nullptr)
        {
            // got edit controller from casting component, assume they belong to the same object
            shouldTerminateController = false;
        }
        else
        {
            // try to create edit controller from factory
            v3_tuid uid = {};
            if (v3_cpp_obj(component)->get_controller_class_id(component, uid) == V3_OK)
            {
                instance = nullptr;
                if (v3_cpp_obj(factory1)->create_instance(factory1, uid, v3_edit_controller_iid, &instance) == V3_OK && instance != nullptr)
                    controller = static_cast<v3_edit_controller**>(instance);
            }

            if (controller == nullptr)
            {
                DISCOVERY_OUT("warning", "Plugin '" << classInfo.v1.name << "' does not have an edit controller");
                v3_cpp_obj_terminate(component);
                v3_cpp_obj_unref(component);
                continue;
            }

            // component is separate from controller, needs its dedicated initialize and terminate
            shouldTerminateController = true;
            v3_cpp_obj_initialize(controller, hostContext);
        }

        // fill in all the details
        uint hints = 0x0;
        int audioIns = 0;
        int audioOuts = 0;
        int cvIns = 0;
        int cvOuts = 0;
        int parameterIns = 0;
        int parameterOuts = 0;

        const int32_t numAudioInputBuses = v3_cpp_obj(component)->get_bus_count(component, V3_AUDIO, V3_INPUT);
        const int32_t numEventInputBuses = v3_cpp_obj(component)->get_bus_count(component, V3_EVENT, V3_INPUT);
        const int32_t numAudioOutputBuses = v3_cpp_obj(component)->get_bus_count(component, V3_AUDIO, V3_OUTPUT);
        const int32_t numEventOutputBuses = v3_cpp_obj(component)->get_bus_count(component, V3_EVENT, V3_OUTPUT);
        const int32_t numParameters = v3_cpp_obj(controller)->get_parameter_count(controller);

        CARLA_SAFE_ASSERT(numAudioInputBuses >= 0);
        CARLA_SAFE_ASSERT(numEventInputBuses >= 0);
        CARLA_SAFE_ASSERT(numAudioOutputBuses >= 0);
        CARLA_SAFE_ASSERT(numEventOutputBuses >= 0);
        CARLA_SAFE_ASSERT(numParameters >= 0);

        for (int32_t j=0; j<numAudioInputBuses; ++j)
        {
            v3_bus_info busInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->get_bus_info(component, V3_AUDIO, V3_INPUT, j, &busInfo) == V3_OK);

            if (busInfo.flags & V3_IS_CONTROL_VOLTAGE)
                cvIns += busInfo.channel_count;
            else
                audioIns += busInfo.channel_count;
        }

        for (int32_t j=0; j<numAudioOutputBuses; ++j)
        {
            v3_bus_info busInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->get_bus_info(component, V3_AUDIO, V3_OUTPUT, j, &busInfo) == V3_OK);

            if (busInfo.flags & V3_IS_CONTROL_VOLTAGE)
                cvOuts += busInfo.channel_count;
            else
                audioOuts += busInfo.channel_count;
        }

        for (int32_t j=0; j<numParameters; ++j)
        {
            v3_param_info paramInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(controller)->get_parameter_info(controller, j, &paramInfo) == V3_OK);

            if (paramInfo.flags & (V3_PARAM_IS_BYPASS|V3_PARAM_IS_HIDDEN|V3_PARAM_PROGRAM_CHANGE))
                continue;

            if (paramInfo.flags & V3_PARAM_READ_ONLY)
                ++parameterOuts;
            else
                ++parameterIns;
        }

        if (v3_plugin_view** const view = v3_cpp_obj(controller)->create_view(controller, "view"))
        {
            if (v3_cpp_obj(view)->is_platform_type_supported(view, V3_VIEW_PLATFORM_TYPE_NATIVE) == V3_TRUE)
                hints |= PLUGIN_HAS_CUSTOM_UI;

            v3_cpp_obj_unref(view);
        }

        if (factory2 != nullptr && std::strstr(classInfo.v2.sub_categories, "Instrument") != nullptr)
            hints |= PLUGIN_IS_SYNTH;

        if (doInit)
        {
            v3_audio_processor** processor = nullptr;
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj_query_interface(component, v3_audio_processor_iid, &processor) == V3_OK);
            CARLA_SAFE_ASSERT_BREAK(processor != nullptr);

            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(processor)->can_process_sample_size(processor, V3_SAMPLE_32) == V3_OK);

            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->set_active(component, true) == V3_OK);
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->set_active(component, false) == V3_OK);

            v3_process_setup setup = { V3_REALTIME, V3_SAMPLE_32, kBufferSize, kSampleRate };
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(processor)->setup_processing(processor, &setup) == V3_OK);

            for (int32_t j=0; j<numAudioInputBuses; ++j)
            {
                v3_bus_info busInfo = {};
                CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->get_bus_info(component, V3_AUDIO, V3_INPUT, j, &busInfo) == V3_OK);

                if ((busInfo.flags & V3_DEFAULT_ACTIVE) == 0x0) {
                    CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->activate_bus(component, V3_AUDIO, V3_INPUT, j, true) == V3_OK);
                }
            }

            for (int32_t j=0; j<numAudioOutputBuses; ++j)
            {
                v3_bus_info busInfo = {};
                CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->get_bus_info(component, V3_AUDIO, V3_OUTPUT, j, &busInfo) == V3_OK);

                if ((busInfo.flags & V3_DEFAULT_ACTIVE) == 0x0) {
                    CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->activate_bus(component, V3_AUDIO, V3_OUTPUT, j, true) == V3_OK);
                }
            }

            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->set_active(component, true) == V3_OK);
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(processor)->set_processing(processor, true) == V3_OK);

            float* bufferAudioIn[(uint)std::max(1, audioIns + cvIns)];
            float* bufferAudioOut[(uint)std::max(1, audioOuts + cvOuts)];

            if (audioIns + cvIns == 0)
            {
                bufferAudioIn[0] = nullptr;
            }
            else
            {
                for (int j=0; j < audioIns + cvIns; ++j)
                {
                    bufferAudioIn[j] = new float[kBufferSize];
                    carla_zeroFloats(bufferAudioIn[j], kBufferSize);
                }
            }

            if (audioOuts + cvOuts == 0)
            {
                bufferAudioOut[0] = nullptr;
            }
            else
            {
                for (int j=0; j < audioOuts + cvOuts; ++j)
                {
                    bufferAudioOut[j] = new float[kBufferSize];
                    carla_zeroFloats(bufferAudioOut[j], kBufferSize);
                }
            }

            carla_v3_event_list eventList;
            carla_v3_event_list* eventListPtr = &eventList;

            carla_v3_param_changes paramChanges;
            carla_v3_param_changes* paramChangesPtr = &paramChanges;

            v3_audio_bus_buffers processInputs = { audioIns + cvIns, 0, { bufferAudioIn } };
            v3_audio_bus_buffers processOutputs = { audioOuts + cvOuts, 0, { bufferAudioOut } };

            v3_process_context processContext = {};
            processContext.sample_rate = kSampleRate;

            v3_process_data processData = {
                V3_REALTIME,
                V3_SAMPLE_32,
                kBufferSize,
                audioIns + cvIns,
                audioOuts + cvOuts,
                &processInputs,
                &processOutputs,
                (v3_param_changes**)&paramChangesPtr,
                (v3_param_changes**)&paramChangesPtr,
                (v3_event_list**)&eventListPtr,
                (v3_event_list**)&eventListPtr,
                &processContext
            };
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(processor)->process(processor, &processData) == V3_OK);

            for (int j=0; j < audioIns + cvIns; ++j)
                delete[] bufferAudioIn[j];
            for (int j=0; j < audioOuts + cvOuts; ++j)
                delete[] bufferAudioOut[j];

            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(processor)->set_processing(processor, false) == V3_OK);
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(component)->set_active(component, false) == V3_OK);

            v3_cpp_obj_unref(processor);
        }

        if (shouldTerminateController)
            v3_cpp_obj_terminate(controller);

        v3_cpp_obj_unref(controller);

        v3_cpp_obj_terminate(component);
        v3_cpp_obj_unref(component);

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("category", getPluginCategoryAsString(factory2 != nullptr ? getPluginCategoryFromV3SubCategories(classInfo.v2.sub_categories)
                                                                                : getPluginCategoryFromName(classInfo.v1.name)));
        DISCOVERY_OUT("name", classInfo.v1.name);
        DISCOVERY_OUT("label", tuid2str(classInfo.v1.class_id));
        DISCOVERY_OUT("maker", (factory2 != nullptr ? classInfo.v2.vendor : factoryInfo.vendor));
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("cv.ins", cvIns);
        DISCOVERY_OUT("cv.outs", cvOuts);
        DISCOVERY_OUT("midi.ins", numEventInputBuses);
        DISCOVERY_OUT("midi.outs", numEventOutputBuses);
        DISCOVERY_OUT("parameters.ins", parameterIns);
        DISCOVERY_OUT("parameters.outs", parameterOuts);
        DISCOVERY_OUT("end", "------------");
    }

    // unref interfaces
    if (factory3 != nullptr)
        v3_cpp_obj_unref(factory3);

    if (factory2 != nullptr)
        v3_cpp_obj_unref(factory2);

    v3_cpp_obj_unref(factory1);

    v3_exit();
}
#endif // ! USING_JUCE_FOR_VST3

struct carla_clap_host : clap_host_t {
    carla_clap_host()
    {
        clap_version = CLAP_VERSION;
        host_data = this;
        name = "Carla-Discovery";
        vendor = "falkTX";
        url = "https://kx.studio/carla";
        version = CARLA_VERSION_STRING;
        get_extension = carla_get_extension;
        request_restart = carla_request_restart;
        request_process = carla_request_process;
        request_callback = carla_request_callback;
    }

    static CLAP_ABI const void* carla_get_extension(const clap_host_t* const host, const char* const extension_id)
    {
        carla_stdout("carla_get_extension %p %s", host, extension_id);
        return nullptr;
    }

    static CLAP_ABI void carla_request_restart(const clap_host_t* const host)
    {
        carla_stdout("carla_request_restart %p", host);
    }

    static CLAP_ABI void carla_request_process(const clap_host_t* const host)
    {
        carla_stdout("carla_request_process %p", host);
    }

    static CLAP_ABI void carla_request_callback(const clap_host_t* const host)
    {
        carla_stdout("carla_request_callback %p", host);
    }
};

static void do_clap_check(lib_t& libHandle, const char* const filename, const bool doInit)
{
    const clap_plugin_entry_t* entry = nullptr;

   #ifdef CARLA_OS_MAC
    BundleLoader bundleLoader;

    // if passed filename is not a plugin binary directly, inspect bundle and find one
    if (libHandle == nullptr)
    {
        if (! bundleLoader.load(filename))
        {
            DISCOVERY_OUT("error", "Failed to load CLAP bundle executable");
            return;
        }

        entry = bundleLoader.getSymbol<const clap_plugin_entry_t*>(CFSTR("clap_entry"));
    }
    else
   #endif
    {
        entry = lib_symbol<const clap_plugin_entry_t*>(libHandle, "clap_entry");
    }

    // ensure entry points are available
    if (entry == nullptr || entry->init == nullptr || entry->deinit == nullptr || entry->get_factory == nullptr)
    {
        DISCOVERY_OUT("error", "Not a CLAP plugin");
        return;
    }

    // ensure compatible version
    if (!clap_version_is_compatible(entry->clap_version))
    {
        DISCOVERY_OUT("error", "Incompatible CLAP plugin");
        return;
    }

    const water::String pluginPath(water::File(filename).getParentDirectory().getFullPathName());

    if (!entry->init(pluginPath.toRawUTF8()))
    {
        DISCOVERY_OUT("error", "CLAP plugin failed to initialize");
        return;
    }

    const clap_plugin_factory_t* const factory = static_cast<const clap_plugin_factory_t*>(
        entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    CARLA_SAFE_ASSERT_RETURN(factory != nullptr
                             && factory->get_plugin_count != nullptr
                             && factory->get_plugin_descriptor != nullptr
                             && factory->create_plugin != nullptr, entry->deinit());

    if (const uint32_t count = factory->get_plugin_count(factory))
    {
        const carla_clap_host host;

        for (uint32_t i=0; i<count; ++i)
        {
            const clap_plugin_descriptor_t* const desc = factory->get_plugin_descriptor(factory, i);
            CARLA_SAFE_ASSERT_CONTINUE(desc != nullptr);

            const clap_plugin_t* const plugin = factory->create_plugin(factory, &host, desc->id);
            CARLA_SAFE_ASSERT_CONTINUE(plugin != nullptr);

            // FIXME this is not needed per spec, but JUCE-based CLAP plugins crash without it :(
            if (!plugin->init(plugin))
            {
                plugin->destroy(plugin);
                continue;
            }

            uint hints = 0x0;
            uint audioIns = 0;
            uint audioOuts = 0;
            // uint audioTotal = 0;
            uint midiIns = 0;
            uint midiOuts = 0;
            // uint midiTotal = 0;
            uint parametersIns = 0;
            uint parametersOuts = 0;
            // uint parametersTotal = 0;
            PluginCategory category = PLUGIN_CATEGORY_NONE;

            const clap_plugin_audio_ports_t* const audioPorts = static_cast<const clap_plugin_audio_ports_t*>(
                plugin->get_extension(plugin, CLAP_EXT_AUDIO_PORTS));

            const clap_plugin_note_ports_t* const notePorts = static_cast<const clap_plugin_note_ports_t*>(
                plugin->get_extension(plugin, CLAP_EXT_NOTE_PORTS));

            const clap_plugin_params_t* const params = static_cast<const clap_plugin_params_t*>(
                plugin->get_extension(plugin, CLAP_EXT_PARAMS));

           #ifdef CLAP_WINDOW_API_NATIVE
            const clap_plugin_gui_t* const gui = static_cast<const clap_plugin_gui_t*>(
                plugin->get_extension(plugin, CLAP_EXT_GUI));
           #endif

            if (audioPorts != nullptr)
            {
                clap_audio_port_info_t info;

                const uint32_t inPorts = audioPorts->count(plugin, true);
                for (uint32_t j=0; j<inPorts; ++j)
                {
                    if (!audioPorts->get(plugin, j, true, &info))
                        break;

                    audioIns += info.channel_count;
                }

                const uint32_t outPorts = audioPorts->count(plugin, false);
                for (uint32_t j=0; j<outPorts; ++j)
                {
                    if (!audioPorts->get(plugin, j, false, &info))
                        break;

                    audioOuts += info.channel_count;
                }

                // audioTotal = audioIns + audioOuts;
            }

            if (notePorts != nullptr)
            {
                clap_note_port_info_t info;

                const uint32_t inPorts = notePorts->count(plugin, true);
                for (uint32_t j=0; j<inPorts; ++j)
                {
                    if (!notePorts->get(plugin, j, true, &info))
                        break;

                    if (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
                        ++midiIns;
                }

                const uint32_t outPorts = notePorts->count(plugin, false);
                for (uint32_t j=0; j<outPorts; ++j)
                {
                    if (!notePorts->get(plugin, j, false, &info))
                        break;

                    if (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
                        ++midiOuts;
                }

                // midiTotal = midiIns + midiOuts;
            }

            if (params != nullptr)
            {
                clap_param_info_t info;

                const uint32_t numParams = params->count(plugin);
                for (uint32_t j=0; j<numParams; ++j)
                {
                    if (!params->get_info(plugin, j, &info))
                        break;

                    if (info.flags & (CLAP_PARAM_IS_HIDDEN|CLAP_PARAM_IS_BYPASS))
                        continue;

                    if (info.flags & CLAP_PARAM_IS_READONLY)
                        ++parametersOuts;
                    else
                        ++parametersIns;
                }

                // parametersTotal = parametersIns + parametersOuts;
            }

            if (desc->features != nullptr)
                category = getPluginCategoryFromClapFeatures(desc->features);

            if (category == PLUGIN_CATEGORY_SYNTH)
                hints |= PLUGIN_IS_SYNTH;

           #ifdef CLAP_WINDOW_API_NATIVE
            if (gui != nullptr)
                hints |= PLUGIN_HAS_CUSTOM_UI;
           #endif

            if (doInit)
            {
                // -----------------------------------------------------------------------
                // start crash-free plugin test

                // FIXME already initiated before, because broken plugins etc
                // plugin->init(plugin);

                // TODO

                // end crash-free plugin test
                // -----------------------------------------------------------------------
            }

            plugin->destroy(plugin);

            DISCOVERY_OUT("init", "-----------");
            DISCOVERY_OUT("build", BINARY_NATIVE);
            DISCOVERY_OUT("hints", hints);
            DISCOVERY_OUT("category", getPluginCategoryAsString(category));
            DISCOVERY_OUT("name", desc->name);
            DISCOVERY_OUT("label", desc->id);
            DISCOVERY_OUT("maker", desc->vendor);
            DISCOVERY_OUT("audio.ins", audioIns);
            DISCOVERY_OUT("audio.outs", audioOuts);
            DISCOVERY_OUT("midi.ins", midiIns);
            DISCOVERY_OUT("midi.outs", midiOuts);
            DISCOVERY_OUT("parameters.ins", parametersIns);
            DISCOVERY_OUT("parameters.outs", parametersOuts);
            DISCOVERY_OUT("end", "------------");
        }
    }

    entry->deinit();
}

#ifdef USING_JUCE
// -------------------------------------------------------------------------------------------------------------------
// find all available plugin audio ports

static void findMaxTotalChannels(juce::AudioProcessor* const filter, int& maxTotalIns, int& maxTotalOuts)
{
    filter->enableAllBuses();

    const int numInputBuses  = filter->getBusCount(true);
    const int numOutputBuses = filter->getBusCount(false);

    if (numInputBuses > 1 || numOutputBuses > 1)
    {
        maxTotalIns = maxTotalOuts = 0;

        for (int i = 0; i < numInputBuses; ++i)
            maxTotalIns += filter->getChannelCountOfBus(true, i);

        for (int i = 0; i < numOutputBuses; ++i)
            maxTotalOuts += filter->getChannelCountOfBus(false, i);
    }
    else
    {
        maxTotalIns  = numInputBuses  > 0 ? filter->getBus(true,  0)->getMaxSupportedChannels(64) : 0;
        maxTotalOuts = numOutputBuses > 0 ? filter->getBus(false, 0)->getMaxSupportedChannels(64) : 0;
    }
}

// -------------------------------------------------------------------------------------------------------------------

static bool do_juce_check(const char* const filename_, const char* const stype, const bool doInit)
{
    CARLA_SAFE_ASSERT_RETURN(stype != nullptr && stype[0] != 0, false) // FIXME
    carla_debug("do_juce_check(%s, %s, %s)", filename_, stype, bool2str(doInit));

    CarlaJUCE::initialiseJuce_GUI();

    juce::String filename;

#ifdef CARLA_OS_WIN
    // Fix for wine usage
    if (juce::File("Z:\\usr\\").isDirectory() && filename_[0] == '/')
    {
        filename = filename_;
        filename.replace("/", "\\");
        filename = "Z:" + filename;
    }
    else
#endif
    filename = juce::File(filename_).getFullPathName();

    CarlaScopedPointer<juce::AudioPluginFormat> pluginFormat;

    /* */ if (std::strcmp(stype, "VST2") == 0)
    {
#if JUCE_PLUGINHOST_VST
        pluginFormat = new juce::VSTPluginFormat();
#else
        DISCOVERY_OUT("error", "VST2 support not available");
        return false;
#endif
    }
    else if (std::strcmp(stype, "VST3") == 0)
    {
#if JUCE_PLUGINHOST_VST3
        pluginFormat = new juce::VST3PluginFormat();
#else
        DISCOVERY_OUT("error", "VST3 support not available");
        return false;
#endif
    }
    else if (std::strcmp(stype, "AU") == 0)
    {
#if JUCE_PLUGINHOST_AU
        pluginFormat = new juce::AudioUnitPluginFormat();
#else
        DISCOVERY_OUT("error", "AU support not available");
        return false;
#endif
    }

    if (pluginFormat == nullptr)
    {
        DISCOVERY_OUT("error", stype << " support not available");
        return false;
    }

#ifdef CARLA_OS_WIN
    CARLA_CUSTOM_SAFE_ASSERT_RETURN("Plugin file/folder does not exist", juce::File(filename).exists(), false);
#endif
    CARLA_SAFE_ASSERT_RETURN(pluginFormat->fileMightContainThisPluginType(filename), false);

    juce::OwnedArray<juce::PluginDescription> results;
    pluginFormat->findAllTypesForFile(results, filename);

    if (results.size() == 0)
    {
#if defined(CARLA_OS_MAC) && defined(__aarch64__)
        if (std::strcmp(stype, "VST2") == 0 || std::strcmp(stype, "VST3") == 0)
            return true;
#endif
        DISCOVERY_OUT("error", "No plugins found");
        return false;
    }

    for (juce::PluginDescription **it = results.begin(), **end = results.end(); it != end; ++it)
    {
        juce::PluginDescription* const desc(*it);

        uint hints = 0x0;
        int audioIns = desc->numInputChannels;
        int audioOuts = desc->numOutputChannels;
        int midiIns = 0;
        int midiOuts = 0;
        int parameters = 0;

        if (desc->isInstrument)
        {
            hints |= PLUGIN_IS_SYNTH;
            midiIns = 1;
        }

        if (doInit)
        {
            if (std::unique_ptr<juce::AudioPluginInstance> instance
                    = pluginFormat->createInstanceFromDescription(*desc, kSampleRate, kBufferSize))
            {
                CarlaJUCE::idleJuce_GUI();

                findMaxTotalChannels(instance.get(), audioIns, audioOuts);
                instance->refreshParameterList();

                parameters = instance->getParameters().size();

                if (instance->hasEditor())
                    hints |= PLUGIN_HAS_CUSTOM_UI;
                if (instance->acceptsMidi())
                    midiIns = 1;
                if (instance->producesMidi())
                    midiOuts = 1;
            }
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("category", getPluginCategoryAsString(getPluginCategoryFromName(desc->category.toRawUTF8())));
        DISCOVERY_OUT("name", desc->descriptiveName);
        DISCOVERY_OUT("label", desc->name);
        DISCOVERY_OUT("maker", desc->manufacturerName);
        DISCOVERY_OUT("uniqueId", desc->uniqueId);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.outs", midiOuts);
        DISCOVERY_OUT("parameters.ins", parameters);
        DISCOVERY_OUT("end", "------------");
    }

    CarlaJUCE::idleJuce_GUI();
    CarlaJUCE::shutdownJuce_GUI();
    return false;
}
#endif // USING_JUCE_FOR_VST2

static void do_fluidsynth_check(const char* const filename, const PluginType type, const bool doInit)
{
#ifdef HAVE_FLUIDSYNTH
    const water::File file(filename);

    if (! file.existsAsFile())
    {
        DISCOVERY_OUT("error", "Requested file is not valid or does not exist");
        return;
    }

    if (type == PLUGIN_SF2 && ! fluid_is_soundfont(filename))
    {
        DISCOVERY_OUT("error", "Not a SF2 file");
        return;
    }

    int programs = 0;

    if (doInit)
    {
        fluid_settings_t* const f_settings = new_fluid_settings();
        CARLA_SAFE_ASSERT_RETURN(f_settings != nullptr,);

        fluid_synth_t* const f_synth = new_fluid_synth(f_settings);
        CARLA_SAFE_ASSERT_RETURN(f_synth != nullptr,);

        const int f_id_test = fluid_synth_sfload(f_synth, filename, 0);

        if (f_id_test < 0)
        {
            DISCOVERY_OUT("error", "Failed to load SF2 file");
            return;
        }

#if FLUIDSYNTH_VERSION_MAJOR >= 2
        const int f_id = f_id_test;
#else
        const uint f_id = static_cast<uint>(f_id_test);
#endif

        if (fluid_sfont_t* const f_sfont = fluid_synth_get_sfont_by_id(f_synth, f_id))
        {
#if FLUIDSYNTH_VERSION_MAJOR >= 2
            fluid_sfont_iteration_start(f_sfont);
            for (; fluid_sfont_iteration_next(f_sfont);)
                ++programs;
#else
            fluid_preset_t f_preset;

            f_sfont->iteration_start(f_sfont);
            for (; f_sfont->iteration_next(f_sfont, &f_preset);)
                ++programs;
#endif
        }

        delete_fluid_synth(f_synth);
        delete_fluid_settings(f_settings);
    }

    CarlaString name(file.getFileNameWithoutExtension().toRawUTF8());
    CarlaString label(name);

    // 2 channels
    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
    DISCOVERY_OUT("category", "synth");
    DISCOVERY_OUT("name", name.buffer());
    DISCOVERY_OUT("label", label.buffer());
    DISCOVERY_OUT("audio.outs", 2);
    DISCOVERY_OUT("midi.ins", 1);
    DISCOVERY_OUT("parameters.ins", 13); // defined in Carla
    DISCOVERY_OUT("parameters.outs", 1);
    DISCOVERY_OUT("end", "------------");

    // 16 channels
    if (doInit && (name.isEmpty() || programs <= 1))
        return;

    name += " (16 outputs)";

    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
    DISCOVERY_OUT("category", "synth");
    DISCOVERY_OUT("name", name.buffer());
    DISCOVERY_OUT("label", label.buffer());
    DISCOVERY_OUT("audio.outs", 32);
    DISCOVERY_OUT("midi.ins", 1);
    DISCOVERY_OUT("parameters.ins", 13); // defined in Carla
    DISCOVERY_OUT("parameters.outs", 1);
    DISCOVERY_OUT("end", "------------");
#else // HAVE_FLUIDSYNTH
    DISCOVERY_OUT("error", "SF2 support not available");
    return;

    // unused
    (void)filename;
    (void)type;
    (void)doInit;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

static void do_jsfx_check(const char* const filename, bool doInit)
{
#ifdef HAVE_YSFX
    const water::File file(filename);

    ysfx_config_u config(ysfx_config_new());

    ysfx_register_builtin_audio_formats(config.get());
    ysfx_guess_file_roots(config.get(), filename);
    ysfx_set_log_reporter(config.get(), &CarlaJsfxLogging::logErrorsOnly);

    ysfx_u effect(ysfx_new(config.get()));

    uint hints = 0;

    // do not attempt to compile it, because the import path is not known
    (void)doInit;

    if (! ysfx_load_file(effect.get(), filename, 0))
    {
        DISCOVERY_OUT("error", "Cannot read the JSFX header");
        return;
    }

    const char* const name = ysfx_get_name(effect.get());

    // author and category are extracted from the pseudo-tags
    const char* const author = ysfx_get_author(effect.get());
    const CB::PluginCategory category = CarlaJsfxCategories::getFromEffect(effect.get());

    const uint32_t audioIns = ysfx_get_num_inputs(effect.get());
    const uint32_t audioOuts = ysfx_get_num_outputs(effect.get());

    const uint32_t midiIns = 1;
    const uint32_t midiOuts = 1;

    uint32_t parameters = 0;
    for (uint32_t sliderIndex = 0; sliderIndex < ysfx_max_sliders; ++sliderIndex)
    {
        if (ysfx_slider_exists(effect.get(), sliderIndex))
            ++parameters;
    }

    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("hints", hints);
    DISCOVERY_OUT("category", getPluginCategoryAsString(category));
    DISCOVERY_OUT("name", name);
    DISCOVERY_OUT("maker", author);
    DISCOVERY_OUT("label", filename);
    DISCOVERY_OUT("audio.ins", audioIns);
    DISCOVERY_OUT("audio.outs", audioOuts);
    DISCOVERY_OUT("midi.ins", midiIns);
    DISCOVERY_OUT("midi.outs", midiOuts);
    DISCOVERY_OUT("parameters.ins", parameters);
    DISCOVERY_OUT("end", "------------");
#else // HAVE_YSFX
    DISCOVERY_OUT("error", "JSFX support not available");
    return;

    // unused
    (void)filename;
    (void)doInit;
#endif
}

// ------------------------------ main entry point ------------------------------

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        carla_stdout("usage: %s <type> </path/to/plugin>", argv[0]);
        return 1;
    }

    const char* const stype    = argv[1];
    const char* const filename = argv[2];
    const PluginType  type     = getPluginTypeFromString(stype);

    CarlaString filenameCheck(filename);
    filenameCheck.toLower();

    bool openLib = false;
    lib_t handle = nullptr;

    switch (type)
    {
    case PLUGIN_LADSPA:
    case PLUGIN_DSSI:
    case PLUGIN_VST2:
        openLib = true;
        break;
    case PLUGIN_VST3:
    case PLUGIN_CLAP:
        openLib = water::File(filename).existsAsFile();
        break;
    default:
        break;
    }

    if (type != PLUGIN_SF2 && filenameCheck.contains("fluidsynth", true))
    {
        DISCOVERY_OUT("info", "skipping fluidsynth based plugin");
        return 0;
    }

#ifdef CARLA_OS_MAC
    if (type == PLUGIN_VST2 && (filenameCheck.endsWith(".vst") || filenameCheck.endsWith(".vst/")))
        openLib = false;
#endif

    // ---------------------------------------------------------------------------------------------------------------
    // Initialize OS features

    // we want stuff in English so we can parse error messages
    ::setlocale(LC_ALL, "C");
#ifndef CARLA_OS_WIN
    carla_setenv("LC_ALL", "C");
#endif

#ifdef CARLA_OS_WIN
    OleInitialize(nullptr);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
# ifndef __WINPTHREADS_VERSION
    // (non-portable) initialization of statically linked pthread library
    pthread_win32_process_attach_np();
    pthread_win32_thread_attach_np();
# endif
#endif

    // ---------------------------------------------------------------------------------------------------------------

    if (openLib)
    {
        handle = lib_open(filename);

        if (handle == nullptr)
        {
            print_lib_error(filename);
            return 1;
        }
    }

    // never do init for dssi-vst, takes too long and it's crashy
    bool doInit = ! filenameCheck.contains("dssi-vst", true);

    if (doInit && getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS") != nullptr)
        doInit = false;

    // ---------------------------------------------------------------------------------------------------------------

    if (doInit && openLib && handle != nullptr)
    {
        // test fast loading & unloading DLL without initializing the plugin(s)
        if (! lib_close(handle))
        {
            print_lib_error(filename);
            return 1;
        }

        handle = lib_open(filename);

        if (handle == nullptr)
        {
            print_lib_error(filename);
            return 1;
        }
    }

#ifndef BUILD_BRIDGE
    if (std::strcmp(filename, ":all") == 0)
    {
        do_cached_check(type);
        return 0;
    }
#endif

#ifdef CARLA_OS_MAC
    // Plugin might be in quarentine due to Apple stupid notarization rules, let's remove that if possible
    switch (type)
    {
    case PLUGIN_LADSPA:
    case PLUGIN_DSSI:
    case PLUGIN_VST2:
    case PLUGIN_VST3:
    case PLUGIN_CLAP:
        removeFileFromQuarantine(filename);
        break;
    default:
        break;
    }
#endif
#ifdef USING_JUCE
    // some macOS plugins have not been yet ported to arm64, re-run them in x86_64 mode if discovery fails
    bool retryJucePlugin = false;
#endif

    switch (type)
    {
    case PLUGIN_LADSPA:
        do_ladspa_check(handle, filename, doInit);
        break;

    case PLUGIN_DSSI:
        do_dssi_check(handle, filename, doInit);
        break;

#ifndef BUILD_BRIDGE
    case PLUGIN_LV2:
        do_lv2_check(filename, doInit);
        break;
#endif

    case PLUGIN_VST2:
#if defined(USING_JUCE) && JUCE_PLUGINHOST_VST
        retryJucePlugin = do_juce_check(filename, "VST2", doInit);
#else
        do_vst2_check(handle, filename, doInit);
#endif
        break;

    case PLUGIN_VST3:
#if defined(USING_JUCE) && JUCE_PLUGINHOST_VST3
        retryJucePlugin = do_juce_check(filename, "VST3", doInit);
#else
        do_vst3_check(handle, filename, doInit);
#endif
        break;

    case PLUGIN_AU:
#if defined(USING_JUCE) && JUCE_PLUGINHOST_AU
        do_juce_check(filename, "AU", doInit);
#else
        DISCOVERY_OUT("error", "AU support not available");
#endif
         break;

#ifndef BUILD_BRIDGE
    case PLUGIN_JSFX:
        do_jsfx_check(filename, doInit);
        break;
#endif

    case PLUGIN_CLAP:
        do_clap_check(handle, filename, doInit);
        break;

    case PLUGIN_DLS:
    case PLUGIN_GIG:
    case PLUGIN_SF2:
        do_fluidsynth_check(filename, type, doInit);
        break;

    default:
        break;
    }

#if defined(CARLA_OS_MAC) && defined(USING_JUCE) && defined(__aarch64__)
    if (retryJucePlugin)
    {
        DISCOVERY_OUT("warning", "No plugins found while scanning in arm64 mode, will try x86_64 now");

        cpu_type_t pref = CPU_TYPE_X86_64;
        pid_t pid = -1;

        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);
        CARLA_SAFE_ASSERT_RETURN(posix_spawnattr_setbinpref_np(&attr, 1, &pref, nullptr) == 0, 1);
        CARLA_SAFE_ASSERT_RETURN(posix_spawn(&pid, argv[0], nullptr, &attr, argv, nullptr) == 0, 1);
        posix_spawnattr_destroy(&attr);

        if (pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
#endif

    if (openLib && handle != nullptr)
        lib_close(handle);

    // ---------------------------------------------------------------------------------------------------------------

#ifdef CARLA_OS_WIN
#ifndef __WINPTHREADS_VERSION
    pthread_win32_thread_detach_np();
    pthread_win32_process_detach_np();
#endif
    CoUninitialize();
    OleUninitialize();
#endif

    return 0;

#ifdef USING_JUCE
    // might be unused
    (void)retryJucePlugin;
#endif
}

// -------------------------------------------------------------------------------------------------------------------
