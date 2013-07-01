/*
 * Carla DSSI Plugin
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

#include "CarlaPluginInternal.hpp"

#ifdef WANT_DSSI

#include "CarlaLadspaUtils.hpp"
#include "dssi/dssi.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

class DssiPlugin : public CarlaPlugin
{
public:
    DssiPlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fDescriptor(nullptr),
          fDssiDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fParamBuffers(nullptr)
    {
        carla_debug("DssiPlugin::DssiPlugin(%p, %i)", engine, id);

        carla_zeroStruct<snd_seq_event_t>(fMidiEvents, MAX_MIDI_EVENTS);

        kData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
    }

    ~DssiPlugin() override
    {
        carla_debug("DssiPlugin::~DssiPlugin()");

        // close UI
        if (fHints & PLUGIN_HAS_GUI)
        {
            showGui(false);

            // Wait a bit first, then force kill
            if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(kData->engine->getOptions().oscUiTimeout))
            {
                carla_stderr("DSSI GUI thread still running, forcing termination now");
                kData->osc.thread.terminate();
            }
        }

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        if (kData->client != nullptr && kData->client->isActive())
            kData->client->deactivate();

        if (kData->active)
        {
            deactivate();
            kData->active = false;
        }

        if (fDescriptor != nullptr)
        {
            if (fName.isNotEmpty() && fDssiDescriptor != nullptr && fDssiDescriptor->run_synth == nullptr && fDssiDescriptor->run_multiple_synths != nullptr)
            {
                removeUniqueMultiSynth(fDescriptor->Label);
            }

            if (fDescriptor->cleanup != nullptr)
            {
                if (fHandle != nullptr)
                    fDescriptor->cleanup(fHandle);
                if (fHandle2 != nullptr)
                    fDescriptor->cleanup(fHandle2);
            }

            fHandle  = nullptr;
            fHandle2 = nullptr;
            fDescriptor = nullptr;
            fDssiDescriptor = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const override
    {
        return PLUGIN_DSSI;
    }

    PluginCategory category() override
    {
        if (fHints & PLUGIN_IS_SYNTH)
            return PLUGIN_CATEGORY_SYNTH;

        return getPluginCategoryFromName(fName);
    }

    long uniqueId() const override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        return fDescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr) override
    {
        CARLA_ASSERT(fOptions & PLUGIN_OPTION_USE_CHUNKS);
        CARLA_ASSERT(fDssiDescriptor != nullptr);
        CARLA_ASSERT(fDssiDescriptor->get_custom_data != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(fHandle2 == nullptr);
        CARLA_ASSERT(dataPtr != nullptr);

        unsigned long dataSize = 0;

        if (fDssiDescriptor->get_custom_data != nullptr && fDssiDescriptor->get_custom_data(fHandle, dataPtr, &dataSize) != 0)
            return static_cast<int32_t>(dataSize);

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int availableOptions() override
    {
        CARLA_ASSERT(fDssiDescriptor != nullptr);

        if (fDssiDescriptor == nullptr)
            return 0x0;

#ifdef __USE_GNU
        const bool isDssiVst = fFilename.contains("dssi-vst", true);
#else
        const bool isDssiVst = fFilename.contains("dssi-vst");
#endif

        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (isDssiVst)
        {
            if (kData->engine->getOptions().useDssiVstChunks && fDssiDescriptor->get_custom_data != nullptr && fDssiDescriptor->set_custom_data != nullptr)
                options |= PLUGIN_OPTION_USE_CHUNKS;
        }
        else
        {
            if (kData->engine->getProccessMode() != PROCESS_MODE_CONTINUOUS_RACK)
            {
                if (fOptions & PLUGIN_OPTION_FORCE_STEREO)
                    options |= PLUGIN_OPTION_FORCE_STEREO;
                else if (kData->audioIn.count <= 1 && kData->audioOut.count <= 1 && (kData->audioIn.count != 0 || kData->audioOut.count != 0))
                    options |= PLUGIN_OPTION_FORCE_STEREO;
            }

            options |= PLUGIN_OPTION_FIXED_BUFFER;
        }

        if (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) override
    {
        CARLA_ASSERT(fParamBuffers != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        return fParamBuffers[parameterId];
    }

    void getLabel(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->Label != nullptr)
            std::strncpy(strBuf, fDescriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->Maker != nullptr)
            std::strncpy(strBuf, fDescriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->Copyright != nullptr)
            std::strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->Name != nullptr)
            std::strncpy(strBuf, fDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const int32_t rindex(kData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fDescriptor->PortCount))
            std::strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue(kData->param.fixValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(type != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        carla_debug("DssiPlugin::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (type == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_STRING) != 0)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (key == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (value == nullptr)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        if (fDssiDescriptor->configure != nullptr)
        {
            fDssiDescriptor->configure(fHandle, key, value);

            if (fHandle2)
                fDssiDescriptor->configure(fHandle2, key, value);
        }

        if (sendGui && kData->osc.data.target != nullptr)
            osc_send_configure(&kData->osc.data, key, value);

        if (std::strcmp(key, "reloadprograms") == 0 || std::strcmp(key, "load") == 0 || std::strncmp(key, "patches", 7) == 0)
        {
            const ScopedDisabler sd(this);
            reloadPrograms(false);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const char* const stringData) override
    {
        CARLA_ASSERT(fOptions & PLUGIN_OPTION_USE_CHUNKS);
        CARLA_ASSERT(fDssiDescriptor != nullptr);
        CARLA_ASSERT(fDssiDescriptor->set_custom_data != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(fHandle2 == nullptr);
        CARLA_ASSERT(stringData != nullptr);

        if (fDssiDescriptor->set_custom_data == nullptr)
            return;

        QByteArray chunk(QByteArray::fromBase64(stringData));

        CARLA_ASSERT(chunk.size() > 0);

        if (chunk.size() > 0)
        {
            const ScopedSingleProcessLocker spl(this, true);
            fDssiDescriptor->set_custom_data(fHandle, chunk.data(), chunk.size());
        }
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(fDssiDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        if (index >= 0 && fDssiDescriptor != nullptr && fDssiDescriptor->select_program != nullptr)
        {
            const uint32_t bank    = kData->midiprog.data[index].bank;
            const uint32_t program = kData->midiprog.data[index].program;

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            fDssiDescriptor->select_program(fHandle, bank, program);

            if (fHandle2 != nullptr)
                fDssiDescriptor->select_program(fHandle2, bank, program);
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo) override
    {
        if (yesNo)
        {
            kData->osc.thread.start();
        }
        else
        {
            if (kData->osc.data.target != nullptr)
            {
                osc_send_hide(&kData->osc.data);
                osc_send_quit(&kData->osc.data);
                kData->osc.data.free();
            }

            if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(kData->engine->getOptions().oscUiTimeout))
                kData->osc.thread.terminate();
        }
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("DssiPlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fDssiDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (kData->engine == nullptr)
            return;
        if (fDescriptor == nullptr)
            return;
        if (fDssiDescriptor == nullptr)
            return;
        if (fHandle == nullptr)
            return;

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (kData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(kData->engine->getSampleRate()));
        const uint32_t portCount(static_cast<uint32_t>(fDescriptor->PortCount));

        uint32_t aIns, aOuts, mIns, params, j;
        aIns = aOuts = mIns = params = 0;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        if (portCount > 0)
        {
            CARLA_ASSERT(fDescriptor->PortDescriptors != nullptr);
            CARLA_ASSERT(fDescriptor->PortRangeHints != nullptr);
            CARLA_ASSERT(fDescriptor->PortNames != nullptr);

            for (uint32_t i=0; i < portCount; ++i)
            {
                const LADSPA_PortDescriptor portType = fDescriptor->PortDescriptors[i];

                if (LADSPA_IS_PORT_AUDIO(portType))
                {
                    if (LADSPA_IS_PORT_INPUT(portType))
                        aIns += 1;
                    else if (LADSPA_IS_PORT_OUTPUT(portType))
                        aOuts += 1;
                }
                else if (LADSPA_IS_PORT_CONTROL(portType))
                    params += 1;
            }
        }

        if ((fOptions & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1))
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(fDescriptor, (unsigned long)sampleRate);

            if (fHandle2 != nullptr)
            {
                if (aIns == 1)
                {
                    aIns = 2;
                    forcedStereoIn = true;
                }

                if (aOuts == 1)
                {
                    aOuts = 2;
                    forcedStereoOut = true;
                }
            }
        }

        if (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr)
        {
            mIns = 1;
            needsCtrlIn = true;
        }

        if (aIns > 0)
        {
            kData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; ++i)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            kData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; ++i)
                fAudioOutBuffers[i] = nullptr;
        }

        if (params > 0)
        {
            kData->param.createNew(params);
            fParamBuffers = new float[params];

            for (uint32_t i=0; i < params; ++i)
                fParamBuffers[i] = 0.0f;
        }

        const uint portNameSize(kData->engine->maxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];

            CARLA_ASSERT(fDescriptor->PortNames[i] != nullptr);

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fName;
                    portName += ":";
                }

                portName += fDescriptor->PortNames[i];
                portName.truncate(portNameSize);

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    j = iAudioIn++;
                    kData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
                    kData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        kData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
                        kData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    j = iAudioOut++;
                    kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                    kData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        kData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                        kData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
            {
                j = iCtrl++;
                kData->param.data[j].index  = j;
                kData->param.data[j].rindex = i;
                kData->param.data[j].hints  = 0x0;
                kData->param.data[j].midiChannel = 0;
                kData->param.data[j].midiCC = -1;

                float min, max, def, step, stepSmall, stepLarge;

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
                    max = min;
                else if (max < min)
                    min = max;

                if (max - min == 0.0f)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': max - min == 0.0f", fDescriptor->PortNames[i]);
                    max = min + 0.1f;
                }

                // default value
                def = get_default_ladspa_port_value(portRangeHints.HintDescriptor, min, max);

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
                    kData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_TOGGLED(portRangeHints.HintDescriptor))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    kData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(portRangeHints.HintDescriptor))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    kData->param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    float range = max - min;
                    step = range/100.0f;
                    stepSmall = range/1000.0f;
                    stepLarge = range/10.0f;
                }

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    kData->param.data[j].type   = PARAMETER_INPUT;
                    kData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                    kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needsCtrlIn = true;

                    // MIDI CC value
                    if (fDssiDescriptor->get_midi_controller_for_port != nullptr)
                    {
                        int controller = fDssiDescriptor->get_midi_controller_for_port(fHandle, i);
                        if (DSSI_CONTROLLER_IS_SET(controller) && DSSI_IS_CC(controller))
                        {
                            int16_t cc = DSSI_CC_NUMBER(controller);
                            if (! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                kData->param.data[j].midiCC = cc;
                        }
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    if (std::strcmp(fDescriptor->PortNames[i], "latency") == 0 || std::strcmp(fDescriptor->PortNames[i], "_latency") == 0)
                    {
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        kData->param.data[j].type  = PARAMETER_LATENCY;
                        kData->param.data[j].hints = 0;
                    }
                    else if (std::strcmp(fDescriptor->PortNames[i], "_sample-rate") == 0)
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        kData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        kData->param.data[j].hints = 0;
                    }
                    else
                    {
                        kData->param.data[j].type   = PARAMETER_OUTPUT;
                        kData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    kData->param.data[j].type = PARAMETER_UNKNOWN;
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LADSPA_IS_HINT_LOGARITHMIC(portRangeHints.HintDescriptor))
                    kData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                kData->param.ranges[j].min = min;
                kData->param.ranges[j].max = max;
                kData->param.ranges[j].def = def;
                kData->param.ranges[j].step = step;
                kData->param.ranges[j].stepSmall = stepSmall;
                kData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                fParamBuffers[j] = def;

                fDescriptor->connect_port(fHandle, i, &fParamBuffers[j]);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, &fParamBuffers[j]);
            }
            else
            {
                // Not Audio or Control
                carla_stderr2("ERROR - Got a broken Port (neither Audio or Control)");

                fDescriptor->connect_port(fHandle, i, nullptr);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, nullptr);
            }
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        if (forcedStereoIn || forcedStereoOut)
            fOptions |= PLUGIN_OPTION_FORCE_STEREO;
        else
            fOptions &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        fHints = 0x0;

        if (LADSPA_IS_HARD_RT_CAPABLE(fDescriptor->Properties))
            fHints |= PLUGIN_IS_RTSAFE;

        if (fGuiFilename.isNotEmpty())
            fHints |= PLUGIN_HAS_GUI;

        if (mIns == 1 && aIns == 0 && aOuts > 0)
            fHints |= PLUGIN_IS_SYNTH;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        kData->extraHints = 0x0;

        if (mIns > 0)
            kData->extraHints |= PLUGIN_HINT_HAS_MIDI_IN;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            kData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;

        // check latency
        if (fHints & PLUGIN_CAN_DRYWET)
        {
            for (uint32_t i=0; i < kData->param.count; ++i)
            {
                if (kData->param.data[i].type != PARAMETER_LATENCY)
                    continue;

                // we need to pre-run the plugin so it can update its latency control-port

                float tmpIn[aIns][2];
                float tmpOut[aOuts][2];

                for (j=0; j < aIns; ++j)
                {
                    tmpIn[j][0] = 0.0f;
                    tmpIn[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, kData->audioIn.ports[j].rindex, tmpIn[j]);
                }

                for (j=0; j < aOuts; ++j)
                {
                    tmpOut[j][0] = 0.0f;
                    tmpOut[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, kData->audioOut.ports[j].rindex, tmpOut[j]);
                }

                if (fDescriptor->activate != nullptr)
                    fDescriptor->activate(fHandle);

                fDescriptor->run(fHandle, 2);

                if (fDescriptor->deactivate != nullptr)
                    fDescriptor->deactivate(fHandle);

                const uint32_t latency = (uint32_t)fParamBuffers[i];

                if (kData->latency != latency)
                {
                    kData->latency = latency;
                    kData->client->setLatency(latency);
                    kData->recreateLatencyBuffers();
                }

                break;
            }
        }

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        if (kData->active)
            activate();

        carla_debug("DssiPlugin::reload() - end");
    }

    void reloadPrograms(const bool init) override
    {
        carla_debug("DssiPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount  = kData->midiprog.count;
        const int32_t current = kData->midiprog.current;

        // Delete old programs
        kData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
        {
            while (fDssiDescriptor->get_program(fHandle, count))
                count++;
        }

        if (count > 0)
        {
            kData->midiprog.createNew(count);

            // Update data
            for (i=0; i < count; ++i)
            {
                const DSSI_Program_Descriptor* const pdesc(fDssiDescriptor->get_program(fHandle, i));
                CARLA_ASSERT(pdesc != nullptr);
                CARLA_ASSERT(pdesc->Name != nullptr);

                kData->midiprog.data[i].bank    = static_cast<uint32_t>(pdesc->Bank);
                kData->midiprog.data[i].program = static_cast<uint32_t>(pdesc->Program);
                kData->midiprog.data[i].name    = carla_strdup(pdesc->Name);
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_midi_program_count(fId, count);

            for (i=0; i < count; ++i)
                kData->engine->osc_send_control_set_midi_program_data(fId, i, kData->midiprog.data[i].bank, kData->midiprog.data[i].program, kData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (count > 0)
                setMidiProgram(0, false, false, false);
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (count == oldCount+1)
            {
                // one midi program added, probably created by user
                kData->midiprog.current = oldCount;
                programChanged = true;
            }
            else if (current < 0 && count > 0)
            {
                // programs exist now, but not before
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && count == 0)
            {
                // programs existed before, but not anymore
                kData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(count))
            {
                // current midi program > count
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                kData->midiprog.current = current;
            }

            if (programChanged)
                setMidiProgram(kData->midiprog.current, true, true, true);

            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor->activate != nullptr)
        {
            fDescriptor->activate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->activate(fHandle2);
        }
    }

    void deactivate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor->deactivate != nullptr)
        {
            fDescriptor->deactivate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->deactivate(fHandle2);
        }
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return;
        }

        unsigned long midiEventCount = 0;

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (kData->needsReset)
        {
            if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (unsigned char j=0, l=MAX_MIDI_CHANNELS; j < MAX_MIDI_CHANNELS; ++j)
                {
                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[j]);
                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[j+l]);

                    fMidiEvents[j].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[j].data.control.channel = j;
                    fMidiEvents[j].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                    fMidiEvents[j+l].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[j+l].data.control.channel = j;
                    fMidiEvents[j+l].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;
                }

                midiEventCount = MAX_MIDI_CHANNELS*2;
            }
            else if (kData->ctrlChannel >= 0 && kData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                for (unsigned char j=0; j < MAX_MIDI_NOTE; ++j)
                {
                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[j]);

                    fMidiEvents[j].type = SND_SEQ_EVENT_NOTEOFF;
                    fMidiEvents[j].data.note.channel = kData->ctrlChannel;
                    fMidiEvents[j].data.note.note    = j;
                }

                midiEventCount = MAX_MIDI_NOTE;
            }

            if (kData->latency > 0)
            {
                for (i=0; i < kData->audioIn.count; ++i)
                    carla_zeroFloat(kData->latencyBuffers[i], kData->latency);
            }

            kData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (kData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                while (midiEventCount < MAX_MIDI_EVENTS && ! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(kData->extNotes.data.getFirst(true));

                    CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    fMidiEvents[midiEventCount].type               = (note.velo > 0) ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    fMidiEvents[midiEventCount].data.note.channel  = note.channel;
                    fMidiEvents[midiEventCount].data.note.note     = note.note;
                    fMidiEvents[midiEventCount].data.note.velocity = note.velo;

                    midiEventCount += 1;
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (fOptions & PLUGIN_OPTION_FIXED_BUFFER) == 0;

            uint32_t time, nEvents = kData->event.portIn->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId = 0;

            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                nextBankId = kData->midiprog.data[kData->midiprog.current].bank;

            for (i=0; i < nEvents; ++i)
            {
                const EngineEvent& event(kData->event.portIn->getEvent(i));

                time = event.time;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && sampleAccurate)
                {
                    if (processSingle(inBuffer, outBuffer, time - timeOffset, timeOffset, midiEventCount))
                    {
                        startTime  = 0;
                        timeOffset = time;
                        midiEventCount = 0;

                        if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                            nextBankId = kData->midiprog.data[kData->midiprog.current].bank;
                        else
                            nextBankId = 0;
                    }
                    else
                        startTime += timeOffset;
                }

                // Control change
                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl:
                {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
                    {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == kData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) > 0)
                            {
                                float left, right;
                                value = ctrlEvent.value/0.5f - 1.0f;

                                if (value < 0.0f)
                                {
                                    left  = -1.0f;
                                    right = (value*2.0f)+1.0f;
                                }
                                else if (value > 0.0f)
                                {
                                    left  = (value*2.0f)-1.0f;
                                    right = 1.0f;
                                }
                                else
                                {
                                    left  = -1.0f;
                                    right = 1.0f;
                                }

                                setBalanceLeft(left, false, false);
                                setBalanceRight(right, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            }
                        }
#endif

                        // Control plugin parameters
                        for (k=0; k < kData->param.count; ++k)
                        {
                            if (kData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (kData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (kData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((kData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (kData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                value = kData->param.ranges[k].unnormalizeValue(ctrlEvent.value);

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        if ((fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            if (midiEventCount >= MAX_MIDI_EVENTS)
                                continue;

                            carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                            fMidiEvents[midiEventCount].time.tick = sampleAccurate ? startTime : time;

                            fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CONTROLLER;
                            fMidiEvents[midiEventCount].data.control.channel = event.channel;
                            fMidiEvents[midiEventCount].data.control.param   = ctrlEvent.param;
                            fMidiEvents[midiEventCount].data.control.value   = ctrlEvent.value*127.0f;

                            midiEventCount += 1;
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (k=0; k < kData->midiprog.count; ++k)
                            {
                                if (kData->midiprog.data[k].bank == nextBankId && kData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false);
                                    postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (midiEventCount >= MAX_MIDI_EVENTS)
                                continue;

                            carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                            fMidiEvents[midiEventCount].time.tick = sampleAccurate ? startTime : time;

                            fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CONTROLLER;
                            fMidiEvents[midiEventCount].data.control.channel = event.channel;
                            fMidiEvents[midiEventCount].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;

                            midiEventCount += 1;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == kData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            if (midiEventCount >= MAX_MIDI_EVENTS)
                                continue;

                            carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                            fMidiEvents[midiEventCount].time.tick = sampleAccurate ? startTime : time;

                            fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CONTROLLER;
                            fMidiEvents[midiEventCount].data.control.channel = event.channel;
                            fMidiEvents[midiEventCount].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                            midiEventCount += 1;
                        }
                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    if (midiEventCount >= MAX_MIDI_EVENTS)
                        continue;

                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;

                    // Fix bad note-off (per DSSI spec)
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status -= 0x10;

                    carla_zeroStruct<snd_seq_event_t>(fMidiEvents[midiEventCount]);

                    fMidiEvents[midiEventCount].time.tick = sampleAccurate ? startTime : time;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        const uint8_t note = midiEvent.data[1];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_NOTEOFF;
                        fMidiEvents[midiEventCount].data.note.channel = channel;
                        fMidiEvents[midiEventCount].data.note.note    = note;

                        postponeRtEvent(kPluginPostRtEventNoteOff, channel, note, 0.0f);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        const uint8_t note = midiEvent.data[1];
                        const uint8_t velo = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_NOTEON;
                        fMidiEvents[midiEventCount].data.note.channel  = channel;
                        fMidiEvents[midiEventCount].data.note.note     = note;
                        fMidiEvents[midiEventCount].data.note.velocity = velo;

                        postponeRtEvent(kPluginPostRtEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) != 0)
                    {
                        const uint8_t note     = midiEvent.data[1];
                        const uint8_t pressure = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_KEYPRESS;
                        fMidiEvents[midiEventCount].data.note.channel  = channel;
                        fMidiEvents[midiEventCount].data.note.note     = note;
                        fMidiEvents[midiEventCount].data.note.velocity = pressure;
                    }
                    else if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0)
                    {
                        const uint8_t control = midiEvent.data[1];
                        const uint8_t value   = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CONTROLLER;
                        fMidiEvents[midiEventCount].data.control.channel = channel;
                        fMidiEvents[midiEventCount].data.control.param   = control;
                        fMidiEvents[midiEventCount].data.control.value   = value;
                    }
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) != 0)
                    {
                        const uint8_t pressure = midiEvent.data[1];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_CHANPRESS;
                        fMidiEvents[midiEventCount].data.control.channel = channel;
                        fMidiEvents[midiEventCount].data.control.value   = pressure;
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status) && (fOptions & PLUGIN_OPTION_SEND_PITCHBEND) != 0)
                    {
                        const uint8_t lsb = midiEvent.data[1];
                        const uint8_t msb = midiEvent.data[2];

                        fMidiEvents[midiEventCount].type = SND_SEQ_EVENT_PITCHBEND;
                        fMidiEvents[midiEventCount].data.control.channel = channel;
                        fMidiEvents[midiEventCount].data.control.value   = ((msb << 7) | lsb) - 8192;
                    }
                    else
                        continue;

                    midiEventCount += 1;

                    break;
                }
                }
            }

            kData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(inBuffer, outBuffer, frames - timeOffset, timeOffset, midiEventCount);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(inBuffer, outBuffer, frames, 0, midiEventCount);

        } // End of Plugin processing (no events)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (kData->event.portOut != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            float    value;

            for (k=0; k < kData->param.count; ++k)
            {
                if (kData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                kData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (kData->param.data[k].midiCC > 0)
                {
                    channel = kData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(kData->param.data[k].midiCC);
                    value   = kData->param.ranges[k].normalizeValue(fParamBuffers[k]);
                    kData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter, param, value);
                }
            }

        } // End of Control Output
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset, const unsigned long midiEventCount)
    {
        CARLA_ASSERT(frames > 0);

        if (frames == 0)
            return false;

        if (kData->audioIn.count > 0)
        {
            CARLA_ASSERT(inBuffer != nullptr);
            if (inBuffer == nullptr)
                return false;
        }
        if (kData->audioOut.count > 0)
        {
            CARLA_ASSERT(outBuffer != nullptr);
            if (outBuffer == nullptr)
                return false;
        }

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (kData->engine->isOffline())
        {
            kData->singleMutex.lock();
        }
        else if (! kData->singleMutex.tryLock())
        {
            for (i=0; i < kData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (i=0; i < kData->audioIn.count; ++i)
            carla_copyFloat(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);
        for (i=0; i < kData->audioOut.count; ++i)
            carla_zeroFloat(fAudioOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        if (fDssiDescriptor->run_synth != nullptr)
        {
            fDssiDescriptor->run_synth(fHandle, frames, fMidiEvents, midiEventCount);

            if (fHandle2 != nullptr)
                fDssiDescriptor->run_synth(fHandle2, frames, fMidiEvents, midiEventCount);
        }
        else if (fDssiDescriptor->run_multiple_synths != nullptr)
        {
            unsigned long instances = (fHandle2 != nullptr) ? 2 : 1;
            LADSPA_Handle handlePtr[2] = { fHandle, fHandle2 };
            snd_seq_event_t* midiEventsPtr[2] = { fMidiEvents, fMidiEvents };
            unsigned long midiEventCountPtr[2] = { midiEventCount, midiEventCount };
            fDssiDescriptor->run_multiple_synths(instances, handlePtr, frames, midiEventsPtr, midiEventCountPtr);
        }
        else
        {
            fDescriptor->run(fHandle, frames);

            if (fHandle2 != nullptr)
                fDescriptor->run(fHandle2, frames);
        }

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (fHints & PLUGIN_CAN_DRYWET) != 0 && kData->postProc.dryWet != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) != 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; ++k)
                    {
                        // TODO
                        //if (k < kData->latency && kData->latency < frames)
                        //    bufValue = (kData->audioIn.count == 1) ? kData->latencyBuffers[0][k] : kData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (kData->audioIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = fAudioInBuffers[(kData->audioIn.count == 1) ? 0 : i][k];
                        fAudioOutBuffers[i][k] = (fAudioOutBuffers[i][k] * kData->postProc.dryWet) + (bufValue * (1.0f - kData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < kData->audioOut.count);
                        carla_copyFloat(oldBufLeft, fAudioOutBuffers[i], frames);
                    }

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            fAudioOutBuffers[i][k]  = oldBufLeft[k]            * (1.0f - balRangeL);
                            fAudioOutBuffers[i][k] += fAudioOutBuffers[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            fAudioOutBuffers[i][k]  = fAudioOutBuffers[i][k] * balRangeR;
                            fAudioOutBuffers[i][k] += oldBufLeft[k]          * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                {
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k] * kData->postProc.volume;
                }
            }

#if 0
            // Latency, save values for next callback, TODO
            if (kData->latency > 0 && kData->latency < frames)
            {
                for (i=0; i < kData->audioIn.count; ++i)
                    carla_copyFloat(kData->latencyBuffers[i], inBuffer[i] + (frames - kData->latency), kData->latency);
            }
#endif
        } // End of Post-processing
#else
        for (i=0; i < kData->audioOut.count; ++i)
        {
            for (k=0; k < frames; ++k)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        kData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("DssiPlugin::bufferSizeChanged(%i) - start", newBufferSize);

        for (uint32_t i=0; i < kData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < kData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (fHandle2 == nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; ++i)
            {
                CARLA_ASSERT(fAudioInBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
            }

            for (uint32_t i=0; i < kData->audioOut.count; ++i)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, kData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
            }
        }
        else
        {
            if (kData->audioIn.count > 0)
            {
                CARLA_ASSERT(kData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
            }

            if (kData->audioOut.count > 0)
            {
                CARLA_ASSERT(kData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  kData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                fDescriptor->connect_port(fHandle2, kData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
            }
        }

        carla_debug("DssiPlugin::bufferSizeChanged(%i) - start", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("DssiPlugin::sampleRateChanged(%g) - start", newSampleRate);

        // TODO
        (void)newSampleRate;

        carla_debug("DssiPlugin::sampleRateChanged(%g) - end", newSampleRate);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() override
    {
        carla_debug("DssiPlugin::clearBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; ++i)
            {
                if (fAudioInBuffers[i] != nullptr)
                {
                    delete[] fAudioInBuffers[i];
                    fAudioInBuffers[i] = nullptr;
                }
            }

            delete[] fAudioInBuffers;
            fAudioInBuffers = nullptr;
        }

        if (fAudioOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioOut.count; ++i)
            {
                if (fAudioOutBuffers[i] != nullptr)
                {
                    delete[] fAudioOutBuffers[i];
                    fAudioOutBuffers[i] = nullptr;
                }
            }

            delete[] fAudioOutBuffers;
            fAudioOutBuffers = nullptr;
        }

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("DssiPlugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(index < kData->param.count);

        if (index >= kData->param.count)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_control(&kData->osc.data, kData->param.data[index].rindex, value);
    }

    void uiMidiProgramChange(const uint32_t index) override
    {
        CARLA_ASSERT(index < kData->midiprog.count);

        if (index >= kData->midiprog.count)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_program(&kData->osc.data, kData->midiprog.data[index].bank, kData->midiprog.data[index].program);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) override
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);
        CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (velo >= MAX_MIDI_VALUE)
            return;
        if (kData->osc.data.target == nullptr)
            return;

#if 0
        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_ON + channel;
        midiData[2] = note;
        midiData[3] = velo;

        osc_send_midi(&kData->osc.data, midiData);
#endif
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) override
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (kData->osc.data.target == nullptr)
            return;

#if 0
        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;

        osc_send_midi(&kData->osc.data, midiData);
#endif
    }

    // -------------------------------------------------------------------

    const void* getExtraStuff() const override
    {
        return fGuiFilename.isNotEmpty() ? (const char*)fGuiFilename : nullptr;
    }

    bool init(const char* const filename, const char* const name, const char* const label, const char* const guiFilename)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(filename != nullptr);
        CARLA_ASSERT(label != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr)
        {
            kData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr)
        {
            kData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! kData->libOpen(filename))
        {
            kData->engine->setLastError(kData->libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        const DSSI_Descriptor_Function descFn = (DSSI_Descriptor_Function)kData->libSymbol("dssi_descriptor");

        if (descFn == nullptr)
        {
            kData->engine->setLastError("Could not find the DSSI Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((fDssiDescriptor = descFn(i++)) != nullptr)
        {
            fDescriptor = fDssiDescriptor->LADSPA_Plugin;
            if (fDescriptor != nullptr && fDescriptor->Label != nullptr && std::strcmp(fDescriptor->Label, label) == 0)
                break;
        }

        if (fDescriptor == nullptr || fDssiDescriptor == nullptr)
        {
            kData->engine->setLastError("Could not find the requested plugin label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check if uses global instance

        if (fDssiDescriptor->run_synth == nullptr && fDssiDescriptor->run_multiple_synths != nullptr)
        {
            if (! addUniqueMultiSynth(fDescriptor->Label))
            {
                kData->engine->setLastError("This plugin uses a global instance and can't be used more than once safely");
                return false;
            }
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
            fName = kData->engine->getUniquePluginName(name);
        else if (fDescriptor->Name != nullptr)
            fName = kData->engine->getUniquePluginName(fDescriptor->Name);
        else
            fName = kData->engine->getUniquePluginName(fDescriptor->Label);

        fFilename = filename;

        // ---------------------------------------------------------------
        // register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, (unsigned long)kData->engine->getSampleRate());

        if (fHandle == nullptr)
        {
            kData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (guiFilename != nullptr)
        {
            fGuiFilename = guiFilename;
            kData->osc.thread.setOscData(guiFilename, fDescriptor->Label);
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
#ifdef __USE_GNU
            const bool isDssiVst = fFilename.contains("dssi-vst", true);
#else
            const bool isDssiVst = fFilename.contains("dssi-vst");
#endif

            // set default options
            fOptions = 0x0;

            fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (kData->engine->getOptions().forceStereo)
                fOptions |= PLUGIN_OPTION_FORCE_STEREO;

            if (isDssiVst)
            {
                fOptions |= PLUGIN_OPTION_FIXED_BUFFER;

                if (kData->engine->getOptions().useDssiVstChunks && fDssiDescriptor->get_custom_data != nullptr && fDssiDescriptor->set_custom_data != nullptr)
                    fOptions |= PLUGIN_OPTION_USE_CHUNKS;
            }

            if (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr)
            {
                fOptions |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                fOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
                fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

                if (fDssiDescriptor->run_synth == nullptr)
                    carla_stderr2("Plugin can ONLY use run_multiple_synths!");
            }

            // load settings
            kData->idStr  = "DSSI/";
            kData->idStr += std::strrchr(filename, OS_SEP)+1;
            kData->idStr += "/";
            kData->idStr += label;
            fOptions = kData->loadSettings(fOptions, availableOptions());

            // ignore settings, we need this anyway
            if (isDssiVst)
                fOptions |= PLUGIN_OPTION_FIXED_BUFFER;
        }

        return true;
    }

private:
    LADSPA_Handle fHandle;
    LADSPA_Handle fHandle2;
    const LADSPA_Descriptor* fDescriptor;
    const DSSI_Descriptor*   fDssiDescriptor;

    CarlaString fGuiFilename;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;
    snd_seq_event_t fMidiEvents[MAX_MIDI_EVENTS];

    static NonRtList<const char*> sMultiSynthList;

    static bool addUniqueMultiSynth(const char* const label)
    {
        for (NonRtList<const char*>::Itenerator it = sMultiSynthList.begin(); it.valid(); it.next())
        {
            const char*& itLabel(*it);

            if (std::strcmp(label, itLabel) == 0)
                return false;
        }

        sMultiSynthList.append(carla_strdup(label));
        return true;
    }

    static void removeUniqueMultiSynth(const char* const label)
    {
        for (NonRtList<const char*>::Itenerator it = sMultiSynthList.begin(); it.valid(); it.next())
        {
            const char*& itLabel(*it);

            if (std::strcmp(label, itLabel) == 0)
            {
                sMultiSynthList.remove(it);
                delete[] itLabel;
                return;
            }
        }
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DssiPlugin)
};

NonRtList<const char*> DssiPlugin::sMultiSynthList;

CARLA_BACKEND_END_NAMESPACE

#else // WANT_DSSI
# warning Building without DSSI support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newDSSI(const Initializer& init, const char* const guiFilename)
{
    carla_debug("CarlaPlugin::newDSSI({%p, \"%s\", \"%s\", \"%s\"}, \"%s\")", init.engine, init.filename, init.name, init.label, guiFilename);

#ifdef WANT_DSSI
    DssiPlugin* const plugin(new DssiPlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label, guiFilename))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! CarlaPluginProtectedData::canRunInRack(plugin))
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo DSSI plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("DSSI support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
