/*
 * Carla Plugin discovery
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#ifdef BUILD_BRIDGE
# undef HAVE_FLUIDSYNTH
#endif

#ifdef USING_JUCE
# if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#  pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
# elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wdouble-promotion"
#  pragma GCC diagnostic ignored "-Weffc++"
#  pragma GCC diagnostic ignored "-Wfloat-equal"
# endif
# include "../backend/utils/JUCE.cpp"
# include "AppConfig.h"
# include "juce_audio_processors/juce_audio_processors.h"
# if JUCE_PLUGINHOST_VST
#  define USING_JUCE_FOR_VST2
# endif
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
#endif

#include "CarlaLadspaUtils.hpp"
#include "CarlaLv2Utils.hpp"

#ifndef USING_JUCE_FOR_VST2
# include "CarlaVst2Utils.hpp"
#endif

#ifdef CARLA_OS_MAC
# define Component CocoaComponent
# define MemoryBlock CocoaMemoryBlock
# define Point CocoaPoint
# import <Foundation/Foundation.h>
# undef Component
# undef MemoryBlock
# undef Point
# include "CarlaMacUtils.cpp"
# include <spawn.h>
# if defined(USING_JUCE) && defined(__aarch64__)
#  include <spawn.h>
# endif
#endif

#ifdef CARLA_OS_WIN
# include <pthread.h>
# include <objbase.h>
#endif

#ifdef HAVE_FLUIDSYNTH
# include <fluidsynth.h>
#endif

#include <iostream>

#include "water/files/File.h"

#ifndef BUILD_BRIDGE
# include "water/text/StringArray.h"
# include "CarlaDssiUtils.cpp"
# include "CarlaJsfxUtils.hpp"
# include "../backend/utils/CachedPlugins.cpp"
#else
# include "CarlaDssiUtils.hpp"
#endif

#define DISCOVERY_OUT(x, y) std::cout << "\ncarla-discovery::" << x << "::" << y << std::endl;

using water::CharPointer_UTF8;
using water::File;
using water::StringArray;

CARLA_BACKEND_USE_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Dummy values to test plugins with

static const uint32_t kBufferSize  = 512;
static const double   kSampleRate  = 44100.0;
static const int32_t  kSampleRatei = 44100;
static const float    kSampleRatef = 44100.0f;

// -------------------------------------------------------------------------------------------------------------------
// Don't print ELF/EXE related errors since discovery can find multi-architecture binaries

static void print_lib_error(const char* const filename)
{
    const char* const error(lib_error(filename));

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

# ifdef USING_JUCE
    if (type == PLUGIN_AU)
        carla_juce_init();
# endif

    const uint count = carla_get_cached_plugin_count(type, plugPath);

    for (uint i=0; i<count; ++i)
    {
        const CarlaCachedPluginInfo* pinfo(carla_get_cached_plugin_info(type, i));
        CARLA_SAFE_ASSERT_CONTINUE(pinfo != nullptr);

        print_cached_plugin(pinfo);
    }

# ifdef USING_JUCE
    if (type == PLUGIN_AU)
        carla_juce_cleanup();
# endif
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
    water::StringArray URIs;

    LILV_FOREACH(plugins, it, lilvPlugins)
    {
        Lilv::Plugin lilvPlugin(lilv_plugins_get(lilvPlugins, it));

        if (const char* const uri = lilvPlugin.get_uri().as_string())
            URIs.addIfNotAlreadyThere(water::String(uri));
    }

    if (URIs.size() == 0)
    {
        DISCOVERY_OUT("warning", "LV2 Bundle doesn't provide any plugins");
        return;
    }

    // Get & check every plugin-instance
    for (int i=0, count=URIs.size(); i < count; ++i)
    {
        const char* const URI = URIs[i].toRawUTF8();
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

static void do_vst_check(lib_t& libHandle, const char* const filename, const bool doInit)
{
    VST_Function vstFn = nullptr;

#ifdef CARLA_OS_MAC
    CFBundleRef bundleRef = nullptr;
    CFBundleRefNum resFileId = 0;

    if (libHandle == nullptr)
    {
        const CFURLRef urlRef = CFURLCreateFromFileSystemRepresentation(0, (const UInt8*)filename, (CFIndex)strlen(filename), true);
        CARLA_SAFE_ASSERT_RETURN(urlRef != nullptr,);

        bundleRef = CFBundleCreate(kCFAllocatorDefault, urlRef);
        CFRelease(urlRef);
        CARLA_SAFE_ASSERT_RETURN(bundleRef != nullptr,);

        if (! CFBundleLoadExecutable(bundleRef))
        {
            CFRelease(bundleRef);
            DISCOVERY_OUT("error", "Failed to load VST bundle executable");
            return;
        }

        vstFn = (VST_Function)CFBundleGetFunctionPointerForName(bundleRef, CFSTR("main_macho"));

        if (vstFn == nullptr)
            vstFn = (VST_Function)CFBundleGetFunctionPointerForName(bundleRef, CFSTR("VSTPluginMain"));

        if (vstFn == nullptr)
        {
            CFBundleUnloadExecutable(bundleRef);
            CFRelease(bundleRef);
            DISCOVERY_OUT("error", "Not a VST plugin");
            return;
        }

        resFileId = CFBundleOpenBundleResourceMap(bundleRef);
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

#ifdef CARLA_OS_MAC
    if (bundleRef != nullptr)
    {
        CFBundleCloseBundleResourceMap(bundleRef, resFileId);
        CFBundleUnloadExecutable(bundleRef);
        CFRelease(bundleRef);
    }
#else
    return;

    // unused
    (void)filename;
#endif
}
#endif // ! USING_JUCE_FOR_VST2

#ifdef USING_JUCE
// -------------------------------------------------------------------------------------------------------------------
// find all available plugin audio ports

static void findMaxTotalChannels(juce::AudioProcessor* const filter, int& maxTotalIns, int& maxTotalOuts)
{
    filter->enableAllBuses();

    int numInputBuses  = filter->getBusCount(true);
    int numOutputBuses = filter->getBusCount(false);

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

    carla_juce_init();

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
                carla_juce_idle();

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
        DISCOVERY_OUT("uniqueId", desc->uid);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.outs", midiOuts);
        DISCOVERY_OUT("parameters.ins", parameters);
        DISCOVERY_OUT("end", "------------");
    }

    carla_juce_idle();
    carla_juce_cleanup();
    return false;
}
#endif // USING_JUCE_FOR_VST2

static void do_fluidsynth_check(const char* const filename, const PluginType type, const bool doInit)
{
#ifdef HAVE_FLUIDSYNTH
    const water::String jfilename = water::String(water::CharPointer_UTF8(filename));
    const water::File file(jfilename);

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

#ifndef BUILD_BRIDGE
static void do_jsfx_check(const char* const filename, bool doInit)
{
    JsusFx::init();

    // TODO(jsfx) determinine the import search path correctly

    water::File file(filename);
    CarlaJsusFxPathLibrary pathLibrary(file);

    CarlaJsusFx effect(pathLibrary);
    effect.setQuiet(true);

    uint hints = 0;

    ///
    JsusFxFileAPI_Basic fileAPI;
    effect.fileAPI = &fileAPI;
    fileAPI.init(effect.m_vm);

    JsusFxGfx gfxAPI;
    effect.gfx = &gfxAPI;
    gfxAPI.init(effect.m_vm);

    ///
    if (doInit)
    {
        int compileFlags =
            JsusFx::kCompileFlag_CompileSerializeSection |
            JsusFx::kCompileFlag_CompileGraphicsSection;

        {
            const CarlaScopedLocale csl;
            if (!effect.compile(pathLibrary, filename, compileFlags))
            {
                DISCOVERY_OUT("error", "Cannot compile the JSFX plugin");
                return;
            }
        }

#if 0 // TODO(jsfx) when supporting custom graphics
        if (effect.hasGraphicsSection())
            hints |= PLUGIN_HAS_CUSTOM_UI;
        // TODO(jsfx) there should be a way to check this without compiling
#endif
    }
    else
    {
        const CarlaScopedLocale csl;
        if (!effect.readHeader(pathLibrary, filename))
        {
            DISCOVERY_OUT("error", "Cannot read the JSFX header");
            return;
        }
    }

    // jsusfx lacks validity checks currently,
    // ensure we have at least the description (required)
    if (effect.desc[0] == '\0')
    {
        DISCOVERY_OUT("error", "The JSFX header is invalid");
        return;
    }

    water::String baseName = water::File(filename).getFileNameWithoutExtension();

    // NOTE: count can be -1 in case of "none"
    uint32_t audioIns = (effect.numInputs == -1) ? 0 : (uint32_t)effect.numInputs;
    uint32_t audioOuts = (effect.numOutputs == -1) ? 0 : (uint32_t)effect.numOutputs;

    uint32_t midiIns = 1;
    uint32_t midiOuts = 1;

    uint32_t parameters = 0;
    for (uint32_t sliderIndex = 0; sliderIndex < JsusFx::kMaxSliders; ++sliderIndex)
    {
        if (effect.sliders[sliderIndex].exists)
            ++parameters;
    }

    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("hints", hints);
    DISCOVERY_OUT("name", baseName.toRawUTF8());
    DISCOVERY_OUT("label", filename);
    DISCOVERY_OUT("audio.ins", audioIns);
    DISCOVERY_OUT("audio.outs", audioOuts);
    DISCOVERY_OUT("midi.ins", midiIns);
    DISCOVERY_OUT("midi.outs", midiOuts);
    DISCOVERY_OUT("parameters.ins", parameters);
    DISCOVERY_OUT("end", "------------");
}
#endif

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
        do_vst_check(handle, filename, doInit);
#endif
        break;

    case PLUGIN_VST3:
#if defined(USING_JUCE) && JUCE_PLUGINHOST_VST3
        retryJucePlugin = do_juce_check(filename, "VST3", doInit);
#else
        DISCOVERY_OUT("error", "VST3 support not available");
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
