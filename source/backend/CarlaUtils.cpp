/*
 * Carla Plugin Host
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

#include "CarlaUtils.h"

#include "CarlaNative.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaLv2Utils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaThread.hpp"
#include "LinkedList.hpp"

#include "juce_audio_formats.h"

#ifdef CARLA_OS_MAC
# include "juce_audio_processors.h"
#endif

#include "../native-plugins/_data.cpp"

namespace CB = CarlaBackend;

static const char* const gNullCharPtr = "";

#ifdef CARLA_OS_MAC
static juce::StringArray gCachedAuPluginResults;
#endif

// -------------------------------------------------------------------------------------------------------------------

_CarlaCachedPluginInfo::_CarlaCachedPluginInfo() noexcept
    : category(CB::PLUGIN_CATEGORY_NONE),
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

const char* carla_get_complete_license_text()
{
    carla_debug("carla_get_complete_license_text()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        retText =
        "<p>This current Carla build is using the following features and 3rd-party code:</p>"
        "<ul>"

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN) || ! defined(VESTIGE_HEADER)
# define LS_NOTE_NO "2"
#else
# define LS_NOTE_NO "1"
#endif

        // Plugin formats
        "<li>LADSPA plugin support</li>"
        "<li>DSSI plugin support</li>"
        "<li>LV2 plugin support</li>"
#ifdef VESTIGE_HEADER
        "<li>VST2 plugin support using VeSTige header by Javier Serrano Polo</li>"
#else
        "<li>VST2 plugin support using official VST SDK 2.4 [1]</li>"
#endif
#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
        "<li>VST3 plugin support using official VST SDK 3.6 [1]</li>"
#endif
#ifdef CARLA_OS_MAC
        "<li>AU plugin support</li>"
#endif

        // Sample kit libraries
#ifdef HAVE_FLUIDSYNTH
        "<li>FluidSynth library for SF2 support</li>"
#endif
#ifdef HAVE_LINUXSAMPLER
        "<li>LinuxSampler library for GIG and SFZ support [" LS_NOTE_NO "]</li>"
#endif

        // misc libs
        "<li>base64 utilities based on code by Ren\u00E9 Nyffenegger</li>"
#ifdef CARLA_OS_MAC
        "<li>sem_timedwait for Mac OS by Keith Shortridge</li>"
#endif
        "<li>liblo library for OSC support</li>"
        "<li>rtmempool library by Nedko Arnaudov"
        "<li>serd, sord, sratom and lilv libraries for LV2 discovery</li>"
#if ! (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
        "<li>RtAudio and RtMidi libraries for extra Audio and MIDI support</li>"
#endif

        // Internal plugins
#ifdef HAVE_EXPERIMENTAL_PLUGINS
        "<li>AT1, BLS1 and REV1 plugin code by Fons Adriaensen</li>"
#endif
        "<li>MIDI Sequencer UI code by Perry Nguyen</li>"
        "<li>MVerb plugin code by Martin Eastwood</li>"
        "<li>Nekobi plugin code based on nekobee by Sean Bolton and others</li>"
        "<li>NekoFilter plugin code based on lv2fil by Nedko Arnaudov and Fons Adriaensen</li>"
#ifdef HAVE_ZYN_DEPS
        "<li>ZynAddSubFX plugin code by Mark McCurry and Nasca Octavian Paul</li>"
#endif

        // end
        "</ul>"

        "<p>"
#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN) || ! defined(VESTIGE_HEADER)
        // Required by VST SDK
        "&nbsp;[1] Trademark of Steinberg Media Technologies GmbH.<br/>"
#endif
#ifdef HAVE_LINUXSAMPLER
        // LinuxSampler GPL exception
        "&nbsp;[" LS_NOTE_NO "] Using LinuxSampler code in commercial hardware or software products is not allowed without prior written authorization by the authors."
#endif
        "</p>"
        ;
    }

    return retText;
}

const char* carla_get_juce_version()
{
    carla_debug("carla_get_juce_version()");

    static CarlaString retVersion;

    if (retVersion.isEmpty())
    {
        if (const char* const version = juce::SystemStats::getJUCEVersion().toRawUTF8())
            retVersion = version+6;
        else
            retVersion = "3.0";
    }

    return retVersion;
}

const char* carla_get_supported_file_extensions()
{
    carla_debug("carla_get_supported_file_extensions()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        retText =
        // Base types
        "*.carxp;*.carxs"
        // MIDI files
        ";*.mid;*.midi"
#ifdef HAVE_FLUIDSYNTH
        // fluidsynth (sf2)
        ";*.sf2"
#endif
#ifdef HAVE_LINUXSAMPLER
        // linuxsampler (gig and sfz)
        ";*.gig;*.sfz"
#endif
#ifdef HAVE_ZYN_DEPS
        // zynaddsubfx presets
        ";*.xmz;*.xiz"
#endif
        ;

        // Audio files
        {
            using namespace juce;

            AudioFormatManager afm;
            afm.registerBasicFormats();

            String juceFormats;

            for (AudioFormat **it=afm.begin(), **end=afm.end(); it != end; ++it)
            {
                const StringArray& exts((*it)->getFileExtensions());

                for (String *eit=exts.begin(), *eend=exts.end(); eit != eend; ++eit)
                    juceFormats += String(";*" + (*eit)).toRawUTF8();
            }

            retText += juceFormats.toRawUTF8();
        }
    }

    return retText;
}

// -------------------------------------------------------------------------------------------------------------------

uint carla_get_cached_plugin_count(CB::PluginType ptype, const char* pluginPath)
{
    CARLA_SAFE_ASSERT_RETURN(ptype == CB::PLUGIN_INTERNAL || ptype == CB::PLUGIN_LV2 || ptype == CB::PLUGIN_AU, 0);
    carla_debug("carla_get_cached_plugin_count(%i:%s)", ptype, CB::PluginType2Str(ptype));

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

    case CB::PLUGIN_AU: {
#ifdef CARLA_OS_MAC
        static bool initiated = false;

        if (initiated)
            return static_cast<uint>(gCachedAuPluginResults.size());

        using namespace juce;

        initiated = true;
        AudioUnitPluginFormat auFormat;
        gCachedAuPluginResults = auFormat.searchPathsForPlugins(juce::FileSearchPath(), false);

        return static_cast<uint>(gCachedAuPluginResults.size());
#else
        return 0;
#endif
    }

    default:
        return 0;
    }
}

const CarlaCachedPluginInfo* carla_get_cached_plugin_info(CB::PluginType ptype, uint index)
{
    carla_debug("carla_get_cached_plugin_info(%i:%s, %i)", ptype, CB::PluginType2Str(ptype), index);

    static CarlaCachedPluginInfo info;

    switch (ptype)
    {
    case CB::PLUGIN_INTERNAL: {
        uint32_t count = 0;
        const NativePluginDescriptor* const descs(carla_get_native_plugins_data(&count));
        CARLA_SAFE_ASSERT_BREAK(index < count);
        CARLA_SAFE_ASSERT_BREAK(descs != nullptr);

        const NativePluginDescriptor& desc(descs[index]);

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

    case CB::PLUGIN_LV2: {
        Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());

        const LilvPlugin* const cPlugin(lv2World.getPluginFromIndex(index));
        CARLA_SAFE_ASSERT_BREAK(cPlugin != nullptr);

        Lilv::Plugin lilvPlugin(cPlugin);
        CARLA_SAFE_ASSERT_BREAK(lilvPlugin.get_uri().is_uri());

        carla_stdout("Filling info for LV2 with URI '%s'", lilvPlugin.get_uri().as_uri());

        // features
        info.hints = 0x0;

        if (lilvPlugin.get_uis().size() > 0 || lilvPlugin.get_modgui_resources_directory().as_uri() != nullptr)
            info.hints |= CB::PLUGIN_HAS_CUSTOM_UI;

        {
            Lilv::Nodes lilvFeatureNodes(lilvPlugin.get_supported_features());

            LILV_FOREACH(nodes, it, lilvFeatureNodes)
            {
                Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(it));
                const char* const featureURI(lilvFeatureNode.as_uri());
                CARLA_SAFE_ASSERT_CONTINUE(featureURI != nullptr);

                if (std::strcmp(featureURI, LV2_CORE__hardRTCapable) == 0)
                    info.hints |= CB::PLUGIN_IS_RTSAFE;
            }

            lilv_nodes_free(const_cast<LilvNodes*>(lilvFeatureNodes.me));
        }

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
                isInput = true;
            else if (lilvPort.is_a(lv2World.port_output))
                isInput = false;
            else
                continue;

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
        }

        // text data
        static CarlaString suri, sname, smaker, slicense;
        suri.clear(); sname.clear(); smaker.clear(); slicense.clear();

        suri = lilvPlugin.get_uri().as_uri();

        if (const char* const name = lilvPlugin.get_name().as_string())
            sname = name;
        else
            sname.clear();

        if (const char* const author = lilvPlugin.get_author_name().as_string())
            smaker = author;
        else
            smaker.clear();

        Lilv::Nodes licenseNodes(lilvPlugin.get_value(lv2World.doap_license));

        if (licenseNodes.size() > 0)
        {
            if (const char* const license = licenseNodes.get_first().as_string())
                slicense = license;
        }

        lilv_nodes_free(const_cast<LilvNodes*>(licenseNodes.me));

        info.name      = sname;
        info.label     = suri;
        info.maker     = smaker;
        info.copyright = slicense;

        return &info;
    }

    case CB::PLUGIN_AU: {
#ifdef CARLA_OS_MAC
        const int indexi(static_cast<int>(index));
        CARLA_SAFE_ASSERT_BREAK(indexi < gCachedAuPluginResults.size());

        using namespace juce;

        String pluginId(gCachedAuPluginResults[indexi]);
        OwnedArray<PluginDescription> results;

        AudioUnitPluginFormat auFormat;
        auFormat.findAllTypesForFile(results, pluginId);
        CARLA_SAFE_ASSERT_BREAK(results.size() > 0);
        CARLA_SAFE_ASSERT(results.size() == 1);

        PluginDescription* const desc(results[0]);
        CARLA_SAFE_ASSERT_BREAK(desc != nullptr);

        info.category = CB::getPluginCategoryFromName(desc->category.toRawUTF8());
        info.hints    = 0x0;

        if (desc->isInstrument)
            info.hints |= CB::PLUGIN_IS_SYNTH;
        if (true)
            info.hints |= CB::PLUGIN_HAS_CUSTOM_UI;

        info.audioIns  = static_cast<uint32_t>(desc->numInputChannels);
        info.audioOuts = static_cast<uint32_t>(desc->numOutputChannels);
        info.midiIns   = desc->isInstrument ? 1 : 0;
        info.midiOuts  = 0;
        info.parameterIns  = 0;
        info.parameterOuts = 0;

        static CarlaString sname, slabel, smaker;

        sname  = desc->name.toRawUTF8();
        slabel = desc->fileOrIdentifier.toRawUTF8();
        smaker = desc->manufacturerName.toRawUTF8();

        info.name      = sname;
        info.label     = slabel;
        info.maker     = smaker;
        info.copyright = gNullCharPtr;

        return &info;
#else
        break;
#endif
    }

    default:
        break;
    }

    info.category      = CB::PLUGIN_CATEGORY_NONE;
    info.hints         = 0x0;
    info.audioIns      = 0;
    info.audioOuts     = 0;
    info.midiIns       = 0;
    info.midiOuts      = 0;
    info.parameterIns  = 0;
    info.parameterOuts = 0;
    info.name          = gNullCharPtr;
    info.label         = gNullCharPtr;
    info.maker         = gNullCharPtr;
    info.copyright     = gNullCharPtr;
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_process_name(const char* name)
{
    carla_debug("carla_set_process_name(\"%s\")", name);

    CarlaThread::setCurrentThreadName(name);
    juce::Thread::setCurrentThreadName(name);
}

// -------------------------------------------------------------------------------------------------------------------

class CarlaPipeClientPlugin : public CarlaPipeClient
{
public:
    CarlaPipeClientPlugin(const CarlaPipeCallbackFunc callbackFunc, void* const callbackPtr) noexcept
        : CarlaPipeClient(),
          fCallbackFunc(callbackFunc),
          fCallbackPtr(callbackPtr)
    {
        CARLA_SAFE_ASSERT(fCallbackFunc != nullptr);
    }

    const char* readlineblock(const uint timeout) noexcept
    {
        return CarlaPipeClient::_readlineblock(timeout);
    }

    bool msgReceived(const char* const msg) noexcept
    {
        if (fCallbackFunc != nullptr)
        {
            try {
                fCallbackFunc(fCallbackPtr, msg);
            } CARLA_SAFE_EXCEPTION("msgReceived");
        }

        return true;
    }

private:
    const CarlaPipeCallbackFunc fCallbackFunc;
    void* const fCallbackPtr;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeClientPlugin)
};

CarlaPipeClientHandle carla_pipe_client_new(const char* argv[], CarlaPipeCallbackFunc callbackFunc, void* callbackPtr)
{
    carla_debug("carla_pipe_client_new(%p, %p, %p)", argv, callbackFunc, callbackPtr);

    CarlaPipeClientPlugin* const pipe(new CarlaPipeClientPlugin(callbackFunc, callbackPtr));

    if (! pipe->initPipeClient(argv))
    {
        delete pipe;
        return nullptr;
    }

    return pipe;
}

void carla_pipe_client_idle(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

    ((CarlaPipeClientPlugin*)handle)->idlePipe();
}

bool carla_pipe_client_is_running(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((CarlaPipeClientPlugin*)handle)->isPipeRunning();
}

void carla_pipe_client_lock(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

    return ((CarlaPipeClientPlugin*)handle)->lockPipe();
}

void carla_pipe_client_unlock(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

    return ((CarlaPipeClientPlugin*)handle)->unlockPipe();
}

const char* carla_pipe_client_readlineblock(CarlaPipeClientHandle handle, uint timeout)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);

    return ((CarlaPipeClientPlugin*)handle)->readlineblock(timeout);
}

bool carla_pipe_client_write_msg(CarlaPipeClientHandle handle, const char* msg)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((CarlaPipeClientPlugin*)handle)->writeMessage(msg);
}

bool carla_pipe_client_write_and_fix_msg(CarlaPipeClientHandle handle, const char* msg)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((CarlaPipeClientPlugin*)handle)->writeAndFixMessage(msg);
}

bool carla_pipe_client_flush(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    return ((CarlaPipeClientPlugin*)handle)->flushMessages();
}

bool carla_pipe_client_flush_and_unlock(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, false);

    CarlaPipeClientPlugin* const pipe((CarlaPipeClientPlugin*)handle);
    const bool ret(pipe->flushMessages());
    pipe->unlockPipe();
    return ret;
}

void carla_pipe_client_destroy(CarlaPipeClientHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
    carla_debug("carla_pipe_client_destroy(%p)", handle);

    CarlaPipeClientPlugin* const pipe((CarlaPipeClientPlugin*)handle);
    pipe->closePipeClient();
    delete pipe;
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_library_filename()
{
    carla_debug("carla_get_library_filename()");

    static CarlaString ret;

    if (ret.isEmpty())
    {
        using juce::File;
        ret = File(File::getSpecialLocation(File::currentExecutableFile)).getFullPathName().toRawUTF8();
    }

    return ret;
}

const char* carla_get_library_folder()
{
    carla_debug("carla_get_library_folder()");

    static CarlaString ret;

    if (ret.isEmpty())
    {
        using juce::File;
        ret = File(File::getSpecialLocation(File::currentExecutableFile).getParentDirectory()).getFullPathName().toRawUTF8();
    }

    return ret;
}

// -------------------------------------------------------------------------------------------------------------------

#include "CarlaPipeUtils.cpp"

// -------------------------------------------------------------------------------------------------------------------
