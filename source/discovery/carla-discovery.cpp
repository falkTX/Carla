/*
 * Carla Plugin discovery
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaBackendUtils.hpp"
//#include "CarlaJuceUtils.hpp"
//#include "CarlaLibUtils.hpp"
//#include "CarlaString.hpp"
//#include "CarlaMIDI.h"

#ifdef WANT_LADSPA
# include "CarlaLadspaUtils.hpp"
#endif
#ifdef WANT_DSSI
# include "CarlaLadspaUtils.hpp"
# include "dssi/dssi.h"
#endif
#ifdef WANT_LV2
# include <QtCore/QUrl>
# include "CarlaLv2Utils.hpp"
#endif
#ifdef WANT_VST
# include "CarlaVstUtils.hpp"
#endif
#ifdef WANT_FLUIDSYNTH
# include <fluidsynth.h>
#endif
#ifdef WANT_LINUXSAMPLER
# include <QtCore/QFileInfo>
# include "linuxsampler/EngineFactory.h"
#endif

#include <iostream>

#define DISCOVERY_OUT(x, y) std::cout << "\ncarla-discovery::" << x << "::" << y << std::endl;

CARLA_BACKEND_USE_NAMESPACE

#ifdef WANT_LV2
// --------------------------------------------------------------------------
// Our LV2 World class object

Lv2WorldClass lv2World;
#endif

// --------------------------------------------------------------------------
// Dummy values to test plugins with

const uint32_t bufferSize = 512;
const double   sampleRate = 44100.0;

// --------------------------------------------------------------------------
// Don't print ELF/EXE related errors since discovery can find multi-architecture binaries

void print_lib_error(const char* const filename)
{
    const char* const error = lib_error(filename);
    if (error && strstr(error, "wrong ELF class") == nullptr && strstr(error, "Bad EXE format") == nullptr)
        DISCOVERY_OUT("error", error);
}

// --------------------------------------------------------------------------
// VST stuff

#ifdef WANT_VST
// Check if plugin wants midi
bool gVstWantsMidi = false;

// Check if plugin is processing
bool gVstIsProcessing = false;

// Current uniqueId for vst shell plugins
intptr_t gVstCurrentUniqueId = 0;

// Supported Carla features
intptr_t vstHostCanDo(const char* const feature)
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
        return 1;
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

    // unimplemented
    carla_stderr("vstHostCanDo(\"%s\") - unknown feature", feature);
    return 0;
}

// Host-side callback
intptr_t VSTCALLBACK vstHostCallback(AEffect* const effect, const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
{
    carla_debug("vstHostCallback(%p, %s, %i, " P_INTPTR ", %p, %f)", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);

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

#if ! VST_FORCE_DEPRECATED
    case audioMasterWantMidi:
        gVstWantsMidi = true;
        ret = 1;
        break;
#endif

    case audioMasterGetTime:
        static VstTimeInfo_R timeInfo;
        carla_zeroStruct<VstTimeInfo_R>(timeInfo);
        timeInfo.sampleRate = sampleRate;

        // Tempo
        timeInfo.tempo  = 120.0;
        timeInfo.flags |= kVstTempoValid;

        // Time Signature
        timeInfo.timeSigNumerator   = 4;
        timeInfo.timeSigDenominator = 4;
        timeInfo.flags |= kVstTimeSigValid;

        ret = getAddressFromPointer(&timeInfo);
        break;

#if ! VST_FORCE_DEPRECATED
    case audioMasterTempoAt:
        ret = 120 * 10000;
        break;

    case audioMasterGetNumAutomatableParameters:
        ret = carla_min<int32_t>(effect->numParams, MAX_DEFAULT_PARAMETERS, 0);
        break;

    case audioMasterGetParameterQuantization:
        ret = 1; // full single float precision
        break;
#endif

    case audioMasterGetSampleRate:
        ret = sampleRate;
        break;

    case audioMasterGetBlockSize:
        ret = bufferSize;
        break;

#if ! VST_FORCE_DEPRECATED
    case audioMasterWillReplaceOrAccumulate:
        ret = 1; // replace
        break;
#endif

    case audioMasterGetCurrentProcessLevel:
        ret = gVstIsProcessing ? kVstProcessLevelRealtime : kVstProcessLevelUser;
        break;

    case audioMasterGetAutomationState:
        ret = kVstAutomationOff;
        break;

    case audioMasterGetVendorString:
        if (ptr)
        {
            std::strcpy((char*)ptr, "falkTX");
            ret = 1;
        }
        break;

    case audioMasterGetProductString:
        if (ptr)
        {
            std::strcpy((char*)ptr, "Carla-Discovery");
            ret = 1;
        }
        break;

    case audioMasterGetVendorVersion:
        ret = 0x050; // 0.5.0
        break;

    case audioMasterCanDo:
        if (ptr)
            ret = vstHostCanDo((const char*)ptr);
        break;

    case audioMasterGetLanguage:
        ret = kVstLangEnglish;
        break;

    default:
        carla_debug("vstHostCallback(%p, %s, %i, " P_INTPTR ", %p, %f)", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
        break;
    }

    return ret;

    // unused
    (void)index;
    (void)value;
    (void)opt;
}
#endif

// --------------------------------------------------------------------------
// LinuxSampler stuff

#ifdef WANT_LINUXSAMPLER
class LinuxSamplerScopedEngine
{
public:
    LinuxSamplerScopedEngine(const char* const filename, const char* const stype)
        : engine(nullptr),
          ins(nullptr)
    {
        using namespace LinuxSampler;

        try {
            engine = EngineFactory::Create(stype);
        }
        catch (const Exception& e)
        {
            DISCOVERY_OUT("error", e.what());
            return;
        }

        if (engine == nullptr)
            return;

        ins = engine->GetInstrumentManager();

        if (ins == nullptr)
        {
            DISCOVERY_OUT("error", "Failed to get LinuxSampler instrument manager");
            return;
        }

        std::vector<InstrumentManager::instrument_id_t> ids;

        try {
            ids = ins->GetInstrumentFileContent(filename);
        }
        catch (const Exception& e)
        {
            DISCOVERY_OUT("error", e.what());
            return;
        }

        if (ids.size() > 0)
        {
            InstrumentManager::instrument_info_t info = ins->GetInstrumentInfo(ids[0]);
            outputInfo(&info, ids.size());
        }
    }

    ~LinuxSamplerScopedEngine()
    {
        if (engine != nullptr)
            LinuxSampler::EngineFactory::Destroy(engine);
    }

    static void outputInfo(LinuxSampler::InstrumentManager::instrument_info_t* const info, const int programs, const char* const basename = nullptr)
    {
        DISCOVERY_OUT("init", "-----------");

        if (info != nullptr)
        {
            DISCOVERY_OUT("name", info->InstrumentName);
            DISCOVERY_OUT("label", info->Product);
            DISCOVERY_OUT("maker", info->Artists);
            DISCOVERY_OUT("copyright", info->Artists);
        }
        else
        {
            DISCOVERY_OUT("name", basename);
            DISCOVERY_OUT("label", basename);
        }

        DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
        DISCOVERY_OUT("audio.outs", 2);
        DISCOVERY_OUT("audio.total", 2);
        DISCOVERY_OUT("midi.ins", 1);
        DISCOVERY_OUT("midi.total", 1);
        DISCOVERY_OUT("programs.total", programs);
        //DISCOVERY_OUT("parameters.ins", 13); // defined in Carla - TODO
        //DISCOVERY_OUT("parameters.outs", 1);
        //DISCOVERY_OUT("parameters.total", 14);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }

private:
    LinuxSampler::Engine* engine;
    LinuxSampler::InstrumentManager* ins;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinuxSamplerScopedEngine)
};
#endif

// ------------------------------ Plugin Checks -----------------------------

void do_ladspa_check(void* const libHandle, const bool init)
{
#ifdef WANT_LADSPA
    const LADSPA_Descriptor_Function descFn = (LADSPA_Descriptor_Function)lib_symbol(libHandle, "ladspa_descriptor");

    if (! descFn)
    {
        DISCOVERY_OUT("error", "Not a LADSPA plugin");
        return;
    }

    unsigned long i = 0;
    const LADSPA_Descriptor* descriptor;

    while ((descriptor = descFn(i++)))
    {
        if (! descriptor->instantiate)
        {
            DISCOVERY_OUT("error", "Plugin '" << descriptor->Name << "' has no instantiate()");
            continue;
        }
        if (! descriptor->cleanup)
        {
            DISCOVERY_OUT("error", "Plugin '" << descriptor->Name << "' has no cleanup()");
            continue;
        }
        if (! descriptor->run)
        {
            DISCOVERY_OUT("error", "Plugin '" << descriptor->Name << "' has no run()");
            continue;
        }
        if (! LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties))
        {
            DISCOVERY_OUT("warning", "Plugin '" << descriptor->Name << "' is not hard real-time capable");
        }

        int hints = 0;
        int audioIns = 0;
        int audioOuts = 0;
        int audioTotal = 0;
        int parametersIns = 0;
        int parametersOuts = 0;
        int parametersTotal = 0;

        if (LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties))
            hints |= PLUGIN_IS_RTSAFE;

        for (unsigned long j=0; j < descriptor->PortCount; j++)
        {
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
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && std::strcmp(descriptor->PortNames[j], "latency") && std::strcmp(descriptor->PortNames[j], "_latency"))
                    parametersOuts += 1;

                parametersTotal += 1;
            }
        }

        if (init)
        {
            // -----------------------------------------------------------------------
            // start crash-free plugin test

            const LADSPA_Handle handle = descriptor->instantiate(descriptor, sampleRate);

            if (! handle)
            {
                DISCOVERY_OUT("error", "Failed to init LADSPA plugin");
                continue;
            }

            LADSPA_Data bufferAudio[bufferSize][audioTotal];
            LADSPA_Data bufferParams[parametersTotal];
            LADSPA_Data min, max, def;

            for (unsigned long j=0, iA=0, iC=0; j < descriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor portDescriptor = descriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  portRangeHints = descriptor->PortRangeHints[j];
                const char* const portName = descriptor->PortNames[j];

                if (LADSPA_IS_PORT_AUDIO(portDescriptor))
                {
                    carla_zeroFloat(bufferAudio[iA], bufferSize);
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
                    else if (max - min == 0.0f)
                    {
                        DISCOVERY_OUT("warning", "Parameter '" << portName << "' is broken: max - min == 0");
                        max = min + 0.1f;
                    }

                    // default value
                    def = get_default_ladspa_port_value(portRangeHints.HintDescriptor, min, max);

                    if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                    {
                        min *= sampleRate;
                        max *= sampleRate;
                        def *= sampleRate;
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

            if (descriptor->activate)
                descriptor->activate(handle);

            descriptor->run(handle, bufferSize);

            if (descriptor->deactivate)
                descriptor->deactivate(handle);

            descriptor->cleanup(handle);

            // end crash-free plugin test
            // -----------------------------------------------------------------------
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", descriptor->Name);
        DISCOVERY_OUT("label", descriptor->Label);
        DISCOVERY_OUT("maker", descriptor->Maker);
        DISCOVERY_OUT("copyright", descriptor->Copyright);
        DISCOVERY_OUT("uniqueId", descriptor->UniqueID);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("audio.total", audioTotal);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("parameters.total", parametersTotal);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
#else
    DISCOVERY_OUT("error", "LADSPA support not available");
    return;

    // unused
    (void)libHandle;
    (void)init;
#endif
}

void do_dssi_check(void* const libHandle, const bool init)
{
#ifdef WANT_DSSI
    const DSSI_Descriptor_Function descFn = (DSSI_Descriptor_Function)lib_symbol(libHandle, "dssi_descriptor");

    if (! descFn)
    {
        DISCOVERY_OUT("error", "Not a DSSI plugin");
        return;
    }

    unsigned long i = 0;
    const DSSI_Descriptor* descriptor;

    while ((descriptor = descFn(i++)))
    {
        const LADSPA_Descriptor* const ldescriptor = descriptor->LADSPA_Plugin;

        if (! ldescriptor)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' has no LADSPA interface");
            continue;
        }
        if (! ldescriptor->instantiate)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' has no instantiate()");
            continue;
        }
        if (! ldescriptor->cleanup)
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' has no cleanup()");
            continue;
        }
        if (! (ldescriptor->run || descriptor->run_synth || descriptor->run_multiple_synths))
        {
            DISCOVERY_OUT("error", "Plugin '" << ldescriptor->Name << "' has no run(), run_synth() or run_multiple_synths()");
            continue;
        }
        if (! LADSPA_IS_HARD_RT_CAPABLE(ldescriptor->Properties))
        {
            DISCOVERY_OUT("warning", "Plugin '" << ldescriptor->Name << "' is not hard real-time capable");
        }

        int hints = 0;
        int audioIns = 0;
        int audioOuts = 0;
        int audioTotal = 0;
        int midiIns = 0;
        int midiTotal = 0;
        int parametersIns = 0;
        int parametersOuts = 0;
        int parametersTotal = 0;
        int programsTotal = 0;

        if (LADSPA_IS_HARD_RT_CAPABLE(ldescriptor->Properties))
            hints |= PLUGIN_IS_RTSAFE;

        for (unsigned long j=0; j < ldescriptor->PortCount; j++)
        {
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
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && std::strcmp(ldescriptor->PortNames[j], "latency") && std::strcmp(ldescriptor->PortNames[j], "_latency"))
                    parametersOuts += 1;

                parametersTotal += 1;
            }
        }

        if (descriptor->run_synth || descriptor->run_multiple_synths)
            midiIns = midiTotal = 1;

        if (midiIns > 0 && audioIns == 0 && audioOuts > 0)
            hints |= PLUGIN_IS_SYNTH;

        if (init)
        {
            // -----------------------------------------------------------------------
            // start crash-free plugin test

            const LADSPA_Handle handle = ldescriptor->instantiate(ldescriptor, sampleRate);

            if (! handle)
            {
                DISCOVERY_OUT("error", "Failed to init DSSI plugin");
                continue;
            }

            if (descriptor->get_program && descriptor->select_program)
            {
                while (descriptor->get_program(handle, programsTotal++))
                    continue;
            }

            LADSPA_Data bufferAudio[bufferSize][audioTotal];
            LADSPA_Data bufferParams[parametersTotal];
            LADSPA_Data min, max, def;

            for (unsigned long j=0, iA=0, iC=0; j < ldescriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor portDescriptor = ldescriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  portRangeHints = ldescriptor->PortRangeHints[j];
                const char* const portName = ldescriptor->PortNames[j];

                if (LADSPA_IS_PORT_AUDIO(portDescriptor))
                {
                    carla_zeroFloat(bufferAudio[iA], bufferSize);
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
                    else if (max - min == 0.0f)
                    {
                        DISCOVERY_OUT("warning", "Parameter '" << portName << "' is broken: max - min == 0");
                        max = min + 0.1f;
                    }

                    // default value
                    def = get_default_ladspa_port_value(portRangeHints.HintDescriptor, min, max);

                    if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                    {
                        min *= sampleRate;
                        max *= sampleRate;
                        def *= sampleRate;
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
            if (programsTotal > 0)
            {
                if (const DSSI_Program_Descriptor* const pDesc = descriptor->get_program(handle, 0))
                    descriptor->select_program(handle, pDesc->Bank, pDesc->Program);
            }

            if (ldescriptor->activate)
                ldescriptor->activate(handle);

            if (descriptor->run_synth || descriptor->run_multiple_synths)
            {
                snd_seq_event_t midiEvents[2];
                memset(midiEvents, 0, sizeof(snd_seq_event_t)*2); //FIXME

                const unsigned long midiEventCount = 2;

                midiEvents[0].type = SND_SEQ_EVENT_NOTEON;
                midiEvents[0].data.note.note     = 64;
                midiEvents[0].data.note.velocity = 100;

                midiEvents[1].type = SND_SEQ_EVENT_NOTEOFF;
                midiEvents[1].data.note.note     = 64;
                midiEvents[1].data.note.velocity = 0;
                midiEvents[1].time.tick = bufferSize/2;

                if (descriptor->run_multiple_synths && ! descriptor->run_synth)
                {
                    LADSPA_Handle handlePtr[1] = { handle };
                    snd_seq_event_t* midiEventsPtr[1] = { midiEvents };
                    unsigned long midiEventCountPtr[1] = { midiEventCount };
                    descriptor->run_multiple_synths(1, handlePtr, bufferSize, midiEventsPtr, midiEventCountPtr);
                }
                else
                    descriptor->run_synth(handle, bufferSize, midiEvents, midiEventCount);
            }
            else
                ldescriptor->run(handle, bufferSize);

            if (ldescriptor->deactivate)
                ldescriptor->deactivate(handle);

            ldescriptor->cleanup(handle);

            // end crash-free plugin test
            // -----------------------------------------------------------------------
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", ldescriptor->Name);
        DISCOVERY_OUT("label", ldescriptor->Label);
        DISCOVERY_OUT("maker", ldescriptor->Maker);
        DISCOVERY_OUT("copyright", ldescriptor->Copyright);
        DISCOVERY_OUT("unique_id", ldescriptor->UniqueID);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("audio.total", audioTotal);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.total", midiTotal);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("parameters.total", parametersTotal);
        DISCOVERY_OUT("programs.total", programsTotal);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
#else
    DISCOVERY_OUT("error", "DSSI support not available");
    return;

    // unused
    (void)libHandle;
    (void)init;
#endif
}

void do_lv2_check(const char* const bundle, const bool init)
{
#ifdef WANT_LV2
    // Convert bundle filename to URI
    QString qBundle(QUrl::fromLocalFile(bundle).toString());
    if (! qBundle.endsWith(QChar(OS_SEP)))
        qBundle += QChar(OS_SEP);

    // Load bundle
    Lilv::Node lilvBundle(lv2World.new_uri(qBundle.toUtf8().constData()));
    lv2World.load_bundle(lilvBundle);

    // Load plugins in this bundle
    const Lilv::Plugins lilvPlugins = lv2World.get_all_plugins();

    // Get all plugin URIs in this bundle
    QStringList URIs;

    LILV_FOREACH(plugins, i, lilvPlugins)
    {
        Lilv::Plugin lilvPlugin(lilv_plugins_get(lilvPlugins, i));

        if (const char* const uri = lilvPlugin.get_uri().as_string())
            URIs.append(QString(uri));
    }

    if (URIs.count() == 0)
    {
        DISCOVERY_OUT("warning", "LV2 Bundle doesn't provide any plugins");
        return;
    }

    // Get & check every plugin-instance
    for (int i=0; i < URIs.count(); i++)
    {
        const LV2_RDF_Descriptor* const rdfDescriptor = lv2_rdf_new(URIs.at(i).toUtf8().constData());
        CARLA_ASSERT(rdfDescriptor && rdfDescriptor->URI);

        if (! (rdfDescriptor && rdfDescriptor->URI))
        {
            DISCOVERY_OUT("error", "Failed to find LV2 plugin '" << URIs.at(i).toUtf8().constData() << "'");
            continue;
        }

        if (init)
        {
            // test if DLL is loadable, twice
            bool isLoadable = true;

            for (int j=0; j < 2; j++)
            {
                void* const libHandle = lib_open(rdfDescriptor->Binary);

                if (! libHandle)
                {
                    isLoadable = false;
                    print_lib_error(rdfDescriptor->Binary);
                    delete rdfDescriptor;
                    break;
                }

                lib_close(libHandle);
            }

            if (! isLoadable)
                continue;
        }

        // test if we support all required ports and features
        {
            bool supported = true;

            for (uint32_t j=0; j < rdfDescriptor->PortCount && supported; j++)
            {
                const LV2_RDF_Port* const rdfPort = &rdfDescriptor->Ports[j];

                if (is_lv2_port_supported(rdfPort->Types))
                {
                    pass();
                }
                else if (! LV2_IS_PORT_OPTIONAL(rdfPort->Properties))
                {
                    DISCOVERY_OUT("error", "Plugin '" << rdfDescriptor->URI << "' requires a non-supported port type (portName: '" << rdfPort->Name << "')");
                    supported = false;
                    break;
                }
            }

            for (uint32_t j=0; j < rdfDescriptor->FeatureCount && supported; j++)
            {
                const LV2_RDF_Feature* const rdfFeature = &rdfDescriptor->Features[j];

                if (is_lv2_feature_supported(rdfFeature->URI))
                {
                    pass();
                }
                else if (LV2_IS_FEATURE_REQUIRED(rdfFeature->Type))
                {
                    DISCOVERY_OUT("error", "Plugin '" << rdfDescriptor->URI << "' requires a non-supported feature '" << rdfFeature->URI << "'");
                    supported = false;
                    break;
                }
            }

            if (! supported)
            {
                delete rdfDescriptor;
                continue;
            }
        }

        int hints = 0;
        int audioIns = 0;
        int audioOuts = 0;
        int audioTotal = 0;
        int midiIns = 0;
        int midiOuts = 0;
        int midiTotal = 0;
        int parametersIns = 0;
        int parametersOuts = 0;
        int parametersTotal = 0;
        int programsTotal = rdfDescriptor->PresetCount;

        for (uint32_t j=0; j < rdfDescriptor->FeatureCount; j++)
        {
            const LV2_RDF_Feature* const rdfFeature = &rdfDescriptor->Features[j];

            if (std::strcmp(rdfFeature->URI, LV2_CORE__hardRTCapable) == 0)
                hints |= PLUGIN_IS_RTSAFE;
        }

        for (uint32_t j=0; j < rdfDescriptor->PortCount; j++)
        {
            const LV2_RDF_Port* const rdfPort = &rdfDescriptor->Ports[j];

            if (LV2_IS_PORT_AUDIO(rdfPort->Types))
            {
                if (LV2_IS_PORT_INPUT(rdfPort->Types))
                    audioIns += 1;
                else if (LV2_IS_PORT_OUTPUT(rdfPort->Types))
                    audioOuts += 1;

                audioTotal += 1;
            }
            else if (LV2_IS_PORT_CONTROL(rdfPort->Types))
            {
                if (LV2_IS_PORT_DESIGNATION_LATENCY(rdfPort->Designation))
                {
                    pass();
                }
                else if (LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(rdfPort->Designation))
                {
                    pass();
                }
                else if (LV2_IS_PORT_DESIGNATION_FREEWHEELING(rdfPort->Designation))
                {
                    pass();
                }
                else if (LV2_IS_PORT_DESIGNATION_TIME(rdfPort->Designation))
                {
                    pass();
                }
                else
                {
                    if (LV2_IS_PORT_INPUT(rdfPort->Types))
                        parametersIns += 1;
                    else if (LV2_IS_PORT_OUTPUT(rdfPort->Types))
                        parametersOuts += 1;

                    parametersTotal += 1;
                }
            }
            else if (LV2_PORT_SUPPORTS_MIDI_EVENT(rdfPort->Types))
            {
                if (LV2_IS_PORT_INPUT(rdfPort->Types))
                    midiIns += 1;
                else if (LV2_IS_PORT_OUTPUT(rdfPort->Types))
                    midiOuts += 1;

                midiTotal += 1;
            }
        }

        if (rdfDescriptor->Type[1] & LV2_PLUGIN_INSTRUMENT)
            hints |= PLUGIN_IS_SYNTH;

        if (rdfDescriptor->UICount > 0)
            hints |= PLUGIN_HAS_GUI;

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("label", rdfDescriptor->URI);
        if (rdfDescriptor->Name)
            DISCOVERY_OUT("name", rdfDescriptor->Name);
        if (rdfDescriptor->Author)
            DISCOVERY_OUT("maker", rdfDescriptor->Author);
        if (rdfDescriptor->License)
            DISCOVERY_OUT("copyright", rdfDescriptor->License);
        DISCOVERY_OUT("unique_id", rdfDescriptor->UniqueID);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("audio.total", audioTotal);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.outs", midiOuts);
        DISCOVERY_OUT("midi.total", midiTotal);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("parameters.total", parametersTotal);
        DISCOVERY_OUT("programs.total", programsTotal);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");

        delete rdfDescriptor;
    }
#else
    DISCOVERY_OUT("error", "LV2 support not available");
    return;

    // unused
    (void)bundle;
    (void)init;
#endif
}

void do_vst_check(void* const libHandle, const bool init)
{
#ifdef WANT_VST
    VST_Function vstFn = (VST_Function)lib_symbol(libHandle, "VSTPluginMain");

    if (! vstFn)
    {
        vstFn = (VST_Function)lib_symbol(libHandle, "main");

        if (! vstFn)
        {
            DISCOVERY_OUT("error", "Not a VST plugin");
            return;
        }
    }

    AEffect* const effect = vstFn(vstHostCallback);

    if (! (effect && effect->magic == kEffectMagic))
    {
        DISCOVERY_OUT("error", "Failed to init VST plugin, or VST magic failed");
        return;
    }

    char strBuf[STR_MAX] = { 0 };
    CarlaString cName;
    CarlaString cProduct;
    CarlaString cVendor;

    effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

    intptr_t vstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

    if (vstCategory == kPlugCategShell && effect->uniqueID == 0)
    {
        if ((gVstCurrentUniqueId = effect->dispatcher(effect, effShellGetNextPlugin, 0, 0, strBuf, 0.0f)) != 0)
            cName = strBuf;
    }
    else
    {
        gVstCurrentUniqueId = effect->uniqueID;

        if (effect->dispatcher(effect, effGetEffectName, 0, 0, strBuf, 0.0f) == 1)
            cName = strBuf;
    }

    carla_zeroMem(strBuf, sizeof(char)*STR_MAX);
    if (effect->dispatcher(effect, effGetVendorString, 0, 0, strBuf, 0.0f) == 1)
        cVendor = strBuf;

    // FIXME: Waves crash during processing
    if (cVendor == "Waves")
        std::memset((void*)&init, 0, sizeof(bool)); //FIXME

    if (gVstCurrentUniqueId == 0)
    {
        DISCOVERY_OUT("error", "Plugin doesn't have an Unique ID");
        return;
    }

    while (gVstCurrentUniqueId != 0)
    {
        carla_zeroMem(strBuf, sizeof(char)*STR_MAX);
        if (effect->dispatcher(effect, effGetProductString, 0, 0, strBuf, 0.0f) == 1)
            cProduct = strBuf;
        else
            cProduct.clear();

        gVstWantsMidi = false;

        int hints = 0;
        int audioIns = effect->numInputs;
        int audioOuts = effect->numOutputs;
        int audioTotal = audioIns + audioOuts;
        int midiIns = 0;
        int midiOuts = 0;
        int midiTotal = 0;
        int parametersIns = effect->numParams;
        int parametersTotal = parametersIns;
        int programsTotal = effect->numPrograms;

        if (effect->flags & effFlagsHasEditor)
            hints |= PLUGIN_HAS_GUI;

        if (effect->flags & effFlagsIsSynth)
            hints |= PLUGIN_IS_SYNTH;

        if (vstPluginCanDo(effect, "receiveVstEvents") || vstPluginCanDo(effect, "receiveVstMidiEvent") || (effect->flags & effFlagsIsSynth) > 0)
            midiIns = 1;

        if (vstPluginCanDo(effect, "sendVstEvents") || vstPluginCanDo(effect, "sendVstMidiEvent"))
            midiOuts = 1;

        midiTotal = midiIns + midiOuts;

        // -----------------------------------------------------------------------
        // start crash-free plugin test

        if (init)
        {
#if ! VST_FORCE_DEPRECATED
            effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, bufferSize, nullptr, sampleRate);
#endif
            effect->dispatcher(effect, effSetBlockSize, 0, bufferSize, nullptr, 0.0f);
            effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
            effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

            effect->dispatcher(effect, effStopProcess,  0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

            effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
            effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);

            // Plugin might call wantMidi() during resume
            if (midiIns == 0 && gVstWantsMidi)
            {
                midiIns = 1;
                midiTotal = midiIns + midiOuts;
            }

            float* bufferAudioIn[audioIns];
            for (int j=0; j < audioIns; j++)
            {
                bufferAudioIn[j] = new float [bufferSize];
                carla_zeroFloat(bufferAudioIn[j], bufferSize);
            }

            float* bufferAudioOut[audioOuts];
            for (int j=0; j < audioOuts; j++)
            {
                bufferAudioOut[j] = new float [bufferSize];
                carla_zeroFloat(bufferAudioOut[j], bufferSize);
            }

            struct VstEventsFixed {
                int32_t numEvents;
                intptr_t reserved;
                VstEvent* data[2];

                VstEventsFixed()
                    : numEvents(0),
                      reserved(0),
                      data{0} {}
            } events;

            VstMidiEvent midiEvents[2];
            carla_zeroMem(midiEvents, sizeof(VstMidiEvent)*2);

            midiEvents[0].type = kVstMidiType;
            midiEvents[0].byteSize = sizeof(VstMidiEvent);
            midiEvents[0].midiData[0] = MIDI_STATUS_NOTE_ON;
            midiEvents[0].midiData[1] = 64;
            midiEvents[0].midiData[2] = 100;

            midiEvents[1].type = kVstMidiType;
            midiEvents[1].byteSize = sizeof(VstMidiEvent);
            midiEvents[1].midiData[0] = MIDI_STATUS_NOTE_OFF;
            midiEvents[1].midiData[1] = 64;
            midiEvents[1].deltaFrames = bufferSize/2;

            events.numEvents = 2;
            events.data[0] = (VstEvent*)&midiEvents[0];
            events.data[1] = (VstEvent*)&midiEvents[1];

            // processing
            {
                gVstIsProcessing = true;

                if (midiIns > 0)
                    effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);

#if ! VST_FORCE_DEPRECATED
                if ((effect->flags & effFlagsCanReplacing) > 0 && effect->processReplacing && effect->processReplacing != effect->process)
                    effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, bufferSize);
                else if (effect->process)
                    effect->process(effect, bufferAudioIn, bufferAudioOut, bufferSize);
                else
                    DISCOVERY_OUT("error", "Plugin doesn't have a process function");
#else
                CARLA_ASSERT(effect->flags & effFlagsCanReplacing);
                if (effect->flags & effFlagsCanReplacing)
                {
                    if (effect->processReplacing)
                        effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, bufferSize);
                    else
                        DISCOVERY_OUT("error", "Plugin doesn't have a process function");
                }
                else
                    DISCOVERY_OUT("error", "Plugin doesn't have can't do process replacing");
#endif

                gVstIsProcessing = false;
            }

            effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

            for (int j=0; j < audioIns; j++)
                delete[] bufferAudioIn[j];
            for (int j=0; j < audioOuts; j++)
                delete[] bufferAudioOut[j];
        }

        // end crash-free plugin test
        // -----------------------------------------------------------------------

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", (const char*)cName);
        DISCOVERY_OUT("label", (const char*)cProduct);
        DISCOVERY_OUT("maker", (const char*)cVendor);
        DISCOVERY_OUT("copyright", (const char*)cVendor);
        DISCOVERY_OUT("unique_id", gVstCurrentUniqueId);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("audio.total", audioTotal);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.outs", midiOuts);
        DISCOVERY_OUT("midi.total", midiTotal);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.total", parametersTotal);
        DISCOVERY_OUT("programs.total", programsTotal);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");

        if (vstCategory != kPlugCategShell)
            break;

        // request next Unique ID
        intptr_t nextUniqueId = gVstCurrentUniqueId;

        // FIXME: Waves sometimes return the same ID
        while (nextUniqueId == gVstCurrentUniqueId)
        {
            carla_zeroMem(strBuf, sizeof(char)*STR_MAX);
            gVstCurrentUniqueId = effect->dispatcher(effect, effShellGetNextPlugin, 0, 0, strBuf, 0.0f);

            if (gVstCurrentUniqueId != 0)
                cName = strBuf;
        }
    }

    effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
#else
    DISCOVERY_OUT("error", "VST support not available");
    return;

    // unused
    (void)libHandle;
    (void)init;
#endif
}

void do_fluidsynth_check(const char* const filename, const bool init)
{
#ifdef WANT_FLUIDSYNTH
    if (! fluid_is_soundfont(filename))
    {
        DISCOVERY_OUT("error", "Not a SF2 file");
        return;
    }

    int programs = 0;

    if (init)
    {
        fluid_settings_t* const f_settings = new_fluid_settings();
        fluid_synth_t* const f_synth = new_fluid_synth(f_settings);
        const int f_id = fluid_synth_sfload(f_synth, filename, 0);

        if (f_id < 0)
        {
            DISCOVERY_OUT("error", "Failed to load SF2 file");
            return;
        }

        fluid_sfont_t* f_sfont;
        fluid_preset_t f_preset;

        f_sfont = fluid_synth_get_sfont_by_id(f_synth, f_id);

        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
            programs += 1;

        delete_fluid_synth(f_synth);
        delete_fluid_settings(f_settings);
    }

#if CARLA_OS_WIN
    int sep = '\\';
#else
    int sep = '/';
#endif

    CarlaString name(std::strrchr(filename, sep)+1);
    name.truncate(name.rfind('.'));

    CarlaString label(name);

    // 2 channels
    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("name", (const char*)name);
    DISCOVERY_OUT("label", (const char*)label);
    DISCOVERY_OUT("maker", "");
    DISCOVERY_OUT("copyright", "");
    DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
    DISCOVERY_OUT("audio.outs", 2);
    DISCOVERY_OUT("audio.total", 2);
    DISCOVERY_OUT("midi.ins", 1);
    DISCOVERY_OUT("midi.total", 1);
    DISCOVERY_OUT("programs.total", programs);
    DISCOVERY_OUT("parameters.ins", 13); // defined in Carla
    DISCOVERY_OUT("parameters.outs", 1);
    DISCOVERY_OUT("parameters.total", 14);
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("end", "------------");

    // 16 channels
    if (name.isNotEmpty())
        name += " (16 outputs)";

    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("name", "");
    DISCOVERY_OUT("name", (const char*)name);
    DISCOVERY_OUT("label", (const char*)label);
    DISCOVERY_OUT("copyright", "");
    DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
    DISCOVERY_OUT("audio.outs", 32);
    DISCOVERY_OUT("audio.total", 32);
    DISCOVERY_OUT("midi.ins", 1);
    DISCOVERY_OUT("midi.total", 1);
    DISCOVERY_OUT("programs.total", programs);
    DISCOVERY_OUT("parameters.ins", 13); // defined in Carla
    DISCOVERY_OUT("parameters.outs", 1);
    DISCOVERY_OUT("parameters.total", 14);
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("end", "------------");
#else
    DISCOVERY_OUT("error", "SF2 support not available");
    return;

    // unused
    (void)filename;
    (void)init;
#endif
}

void do_linuxsampler_check(const char* const filename, const char* const stype, const bool init)
{
#ifdef WANT_LINUXSAMPLER
    const QFileInfo file(filename);

    if (! file.exists())
    {
        DISCOVERY_OUT("error", "Requested file does not exist");
        return;
    }

    if (! file.isFile())
    {
        DISCOVERY_OUT("error", "Requested file is not valid");
        return;
    }

    if (! file.isReadable())
    {
        DISCOVERY_OUT("error", "Requested file is not readable");
        return;
    }

    if (init)
        const LinuxSamplerScopedEngine engine(filename, stype);
    else
        LinuxSamplerScopedEngine::outputInfo(nullptr, 0, file.baseName().toUtf8().constData());
#else
    DISCOVERY_OUT("error", stype << " support not available");
    return;

    // unused
    (void)filename;
    (void)init;
#endif
}

// ------------------------------ main entry point ------------------------------

int main(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "-formats") == 0)
    {
        printf("Available plugin formats:\n");
        printf("LADSPA: ");
#ifdef WANT_LADSPA
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("DSSI:   ");
#ifdef WANT_DSSI
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("LV2:    ");
#ifdef WANT_LV2
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("VST:    ");
#ifdef WANT_VST
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("\n");

        printf("Available sampler formats:\n");
        printf("GIG (LinuxSampler): ");
#ifdef WANT_LINUXSAMPLER
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("SF2 (FluidSynth):   ");
#ifdef WANT_FLUIDSYNTH
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("SFZ (LinuxSampler): ");
#ifdef WANT_LINUXSAMPLER
        printf("yes\n");
#else
        printf("no\n");
#endif
        return 0;
    }

    if (argc != 3)
    {
        carla_stdout("usage: %s <type> </path/to/plugin>", argv[0]);
        return 1;
    }

    const char* const stype    = argv[1];
    const char* const filename = argv[2];

    bool openLib;
    PluginType type;
    void* handle = nullptr;

    if (std::strcmp(stype, "LADSPA") == 0)
    {
        openLib = true;
        type = PLUGIN_LADSPA;
    }
    else if (std::strcmp(stype, "DSSI") == 0)
    {
        openLib = true;
        type = PLUGIN_DSSI;
    }
    else if (std::strcmp(stype, "LV2") == 0)
    {
        openLib = false;
        type = PLUGIN_LV2;
    }
    else if (std::strcmp(stype, "VST") == 0)
    {
        openLib = true;
        type = PLUGIN_VST;
    }
    else if (std::strcmp(stype, "GIG") == 0)
    {
        openLib = false;
        type = PLUGIN_GIG;
    }
    else if (std::strcmp(stype, "SF2") == 0)
    {
        openLib = false;
        type = PLUGIN_SF2;
    }
    else if (std::strcmp(stype, "SFZ") == 0)
    {
        openLib = false;
        type = PLUGIN_SFZ;
    }
    else
    {
        DISCOVERY_OUT("error", "Invalid plugin type");
        return 1;
    }

    if (openLib)
    {
        handle = lib_open(filename);

        if (! handle)
        {
            print_lib_error(filename);
            return 1;
        }
    }

    bool doInit = ! QString(filename).endsWith("dssi-vst.so", Qt::CaseInsensitive);

    if (doInit && getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS"))
        doInit = false;

    if (doInit && handle)
    {
        // test fast loading & unloading DLL without initializing the plugin(s)

        if (! lib_close(handle))
        {
            print_lib_error(filename);
            return 1;
        }

        handle = lib_open(filename);

        if (! handle)
        {
            print_lib_error(filename);
            return 1;
        }
    }

    switch (type)
    {
    case PLUGIN_LADSPA:
        do_ladspa_check(handle, doInit);
        break;
    case PLUGIN_DSSI:
        do_dssi_check(handle, doInit);
        break;
    case PLUGIN_LV2:
        do_lv2_check(filename, doInit);
        break;
    case PLUGIN_VST:
        do_vst_check(handle, doInit);
        break;
    case PLUGIN_GIG:
        do_linuxsampler_check(filename, "gig", doInit);
        break;
    case PLUGIN_SF2:
        do_fluidsynth_check(filename, doInit);
        break;
    case PLUGIN_SFZ:
        do_linuxsampler_check(filename, "sfz", doInit);
        break;
    default:
        break;
    }

    if (openLib && handle)
        lib_close(handle);

    return 0;
}
