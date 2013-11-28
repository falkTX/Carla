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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPluginInternal.hpp"

#ifdef WANT_DSSI

#include "CarlaDssiUtils.hpp"
#include "CarlaLibUtils.hpp"

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
          fUsesCustomData(false),
          fGuiFilename(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fParamBuffers(nullptr)
    {
        carla_debug("DssiPlugin::DssiPlugin(%p, %i)", engine, id);

        pData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
    }

    ~DssiPlugin() override
    {
        carla_debug("DssiPlugin::~DssiPlugin()");

        // close UI
        if (fHints & PLUGIN_HAS_GUI)
        {
            showGui(false);

            pData->osc.thread.stop(pData->engine->getOptions().uiBridgesTimeout * 2);
        }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fDescriptor != nullptr)
        {
            if (fName.isNotEmpty() && fDssiDescriptor != nullptr && fDssiDescriptor->run_synth == nullptr && fDssiDescriptor->run_multiple_synths != nullptr)
                removeUniqueMultiSynth(fDescriptor->Label);

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

        if (fGuiFilename != nullptr)
        {
            delete[] fGuiFilename;
            fGuiFilename = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_DSSI;
    }

    PluginCategory getCategory() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr, PLUGIN_CATEGORY_NONE);

        if (pData->audioIn.count == 0 && pData->audioOut.count > 0 && (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr))
            return PLUGIN_CATEGORY_SYNTH;

        return getPluginCategoryFromName(fName);
    }

    long getUniqueId() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0);

        return fDescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t getChunkData(void** const dataPtr) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsesCustomData, 0);
        CARLA_SAFE_ASSERT_RETURN(fOptions & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->get_custom_data != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fHandle2 == nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        unsigned long dataSize = 0;

        if (fDssiDescriptor->get_custom_data(fHandle, dataPtr, &dataSize) != 0)
            return static_cast<int32_t>(dataSize);

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int getAvailableOptions() const override
    {
        const bool isAmSynth = fFilename.contains("amsynth", true);
        const bool isDssiVst = fFilename.contains("dssi-vst", true);

        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (! (isAmSynth || isDssiVst))
        {
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (pData->engine->getProccessMode() != ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                if (fOptions & PLUGIN_OPTION_FORCE_STEREO)
                    options |= PLUGIN_OPTION_FORCE_STEREO;
                else if (pData->audioIn.count <= 1 && pData->audioOut.count <= 1 && (pData->audioIn.count != 0 || pData->audioOut.count != 0))
                    options |= PLUGIN_OPTION_FORCE_STEREO;
            }
        }

        if (fUsesCustomData)
            options |= PLUGIN_OPTION_USE_CHUNKS;

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

    float getParameterValue(const uint32_t parameterId) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fParamBuffers[parameterId];
    }

    void getLabel(char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->Label != nullptr)
            std::strncpy(strBuf, fDescriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->Maker != nullptr)
            std::strncpy(strBuf, fDescriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->Copyright != nullptr)
            std::strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->Name != nullptr)
            std::strncpy(strBuf, fDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

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
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        carla_debug("DssiPlugin::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) != 0)
            return carla_stderr2("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (fDssiDescriptor->configure != nullptr)
        {
            fDssiDescriptor->configure(fHandle, key, value);

            if (fHandle2 != nullptr)
                fDssiDescriptor->configure(fHandle2, key, value);
        }

        if (sendGui && pData->osc.data.target != nullptr)
            osc_send_configure(pData->osc.data, key, value);

        if (std::strcmp(key, "reloadprograms") == 0 || std::strcmp(key, "load") == 0 || std::strncmp(key, "patches", 7) == 0)
        {
            const ScopedSingleProcessLocker spl(this, true);
            reloadPrograms(false);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const char* const stringData) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsesCustomData,);
        CARLA_SAFE_ASSERT_RETURN(fOptions & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->set_custom_data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle2 == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(stringData != nullptr,);

        // TODO
#if 0
        QByteArray chunk(QByteArray::fromBase64(stringData));

        CARLA_ASSERT(chunk.size() > 0);

        if (chunk.size() > 0)
        {
            const ScopedSingleProcessLocker spl(this, true);
            fDssiDescriptor->set_custom_data(fHandle, chunk.data(), chunk.size());
        }
#endif
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->select_program != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        if (index >= 0)
        {
            const uint32_t bank    = pData->midiprog.data[index].bank;
            const uint32_t program = pData->midiprog.data[index].program;

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
            pData->osc.thread.start();
        }
        else
        {
            if (pData->osc.data.target != nullptr)
            {
                osc_send_hide(pData->osc.data);
                osc_send_quit(pData->osc.data);
                pData->osc.data.free();
            }

            pData->osc.thread.stop(pData->engine->getOptions().uiBridgesTimeout);
        }
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        carla_debug("DssiPlugin::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));
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
            pData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; ++i)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; ++i)
                fAudioOutBuffers[i] = nullptr;
        }

        if (params > 0)
        {
            pData->param.createNew(params);

            fParamBuffers = new float[params];
#ifdef USE_JUCE
            FloatVectorOperations::clear(fParamBuffers, params);
#else
#endif
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];

            CARLA_ASSERT(fDescriptor->PortNames[i] != nullptr);

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fName;
                    portName += ":";
                }

                portName += fDescriptor->PortNames[i];
                portName.truncate(portNameSize);

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    j = iAudioIn++;
                    pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
                    pData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        pData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true);
                        pData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    j = iAudioOut++;
                    pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
                    pData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false);
                        pData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    carla_stderr2("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
            {
                j = iCtrl++;
                pData->param.data[j].index  = j;
                pData->param.data[j].rindex = i;
                pData->param.data[j].hints  = 0x0;
                pData->param.data[j].midiChannel = 0;
                pData->param.data[j].midiCC = -1;

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
                    pData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_TOGGLED(portRangeHints.HintDescriptor))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(portRangeHints.HintDescriptor))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
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
                    pData->param.data[j].type   = PARAMETER_INPUT;
                    pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                    pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needsCtrlIn = true;

                    // MIDI CC value
                    if (fDssiDescriptor->get_midi_controller_for_port != nullptr)
                    {
                        int controller = fDssiDescriptor->get_midi_controller_for_port(fHandle, i);
                        if (DSSI_CONTROLLER_IS_SET(controller) && DSSI_IS_CC(controller))
                        {
                            int16_t cc = DSSI_CC_NUMBER(controller);
                            if (! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                pData->param.data[j].midiCC = cc;
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

                        //pData->param.data[j].type  = PARAMETER_LATENCY;
                        pData->param.data[j].hints = 0;
                    }
                    else if (std::strcmp(fDescriptor->PortNames[i], "_sample-rate") == 0)
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        //pData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        pData->param.data[j].hints = 0;
                    }
                    else
                    {
                        pData->param.data[j].type   = PARAMETER_OUTPUT;
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    pData->param.data[j].type = PARAMETER_UNKNOWN;
                    carla_stderr2("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LADSPA_IS_HINT_LOGARITHMIC(portRangeHints.HintDescriptor))
                    pData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                pData->param.ranges[j].min = min;
                pData->param.ranges[j].max = max;
                pData->param.ranges[j].def = def;
                pData->param.ranges[j].step = step;
                pData->param.ranges[j].stepSmall = stepSmall;
                pData->param.ranges[j].stepLarge = stepLarge;

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

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        if (forcedStereoIn || forcedStereoOut)
            fOptions |= PLUGIN_OPTION_FORCE_STEREO;
        else
            fOptions &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        fHints = 0x0;

        if (LADSPA_IS_HARD_RT_CAPABLE(fDescriptor->Properties))
            fHints |= PLUGIN_IS_RTSAFE;

        if (fGuiFilename != nullptr)
            fHints |= PLUGIN_HAS_GUI;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        // check latency
        if (fHints & PLUGIN_CAN_DRYWET)
        {
            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                //if (pData->param.data[i].type != PARAMETER_LATENCY)
                //    continue;

                // we need to pre-run the plugin so it can update its latency control-port

                float tmpIn[aIns][2];
                float tmpOut[aOuts][2];

                for (j=0; j < aIns; ++j)
                {
                    tmpIn[j][0] = 0.0f;
                    tmpIn[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, pData->audioIn.ports[j].rindex, tmpIn[j]);
                }

                for (j=0; j < aOuts; ++j)
                {
                    tmpOut[j][0] = 0.0f;
                    tmpOut[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, pData->audioOut.ports[j].rindex, tmpOut[j]);
                }

                if (fDescriptor->activate != nullptr)
                    fDescriptor->activate(fHandle);

                fDescriptor->run(fHandle, 2);

                if (fDescriptor->deactivate != nullptr)
                    fDescriptor->deactivate(fHandle);

                const uint32_t latency = (uint32_t)fParamBuffers[i];

                if (pData->latency != latency)
                {
                    pData->latency = latency;
                    pData->client->setLatency(latency);
                    pData->recreateLatencyBuffers();
                }

                break;
            }
        }

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("DssiPlugin::reload() - end");
    }

    void reloadPrograms(const bool init) override
    {
        carla_debug("DssiPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount  = pData->midiprog.count;
        const int32_t current = pData->midiprog.current;

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
        {
            while (fDssiDescriptor->get_program(fHandle, count))
                count++;
        }

        if (count > 0)
        {
            pData->midiprog.createNew(count);

            // Update data
            for (i=0; i < count; ++i)
            {
                const DSSI_Program_Descriptor* const pdesc(fDssiDescriptor->get_program(fHandle, i));
                CARLA_ASSERT(pdesc != nullptr);
                CARLA_ASSERT(pdesc->Name != nullptr);

                pData->midiprog.data[i].bank    = static_cast<uint32_t>(pdesc->Bank);
                pData->midiprog.data[i].program = static_cast<uint32_t>(pdesc->Program);
                pData->midiprog.data[i].name    = carla_strdup(pdesc->Name);
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (pData->engine->isOscControlRegistered())
        {
            pData->engine->oscSend_control_set_midi_program_count(fId, count);

            for (i=0; i < count; ++i)
                pData->engine->oscSend_control_set_midi_program_data(fId, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
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
                pData->midiprog.current = oldCount;
                programChanged = true;
            }
            else if (current < 0 && count > 0)
            {
                // programs exist now, but not before
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && count == 0)
            {
                // programs existed before, but not anymore
                pData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(count))
            {
                // current midi program > count
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                pData->midiprog.current = current;
            }

            if (programChanged)
                setMidiProgram(pData->midiprog.current, true, true, true);

            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->activate != nullptr)
        {
            fDescriptor->activate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->activate(fHandle2);
        }
    }

    void deactivate() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->deactivate != nullptr)
        {
            fDescriptor->deactivate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->deactivate(fHandle2);
        }
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
#ifdef USE_JUCE
                FloatVectorOperations::clear(outBuffer[i], frames);
#else
#endif
            }

            return;
        }

        unsigned long midiEventCount = 0;

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                midiEventCount = MAX_MIDI_CHANNELS*2;
                carla_zeroStruct<snd_seq_event_t>(fMidiEvents, midiEventCount);

                for (unsigned char i=0, k=MAX_MIDI_CHANNELS; i < MAX_MIDI_CHANNELS; ++i)
                {
                    fMidiEvents[i].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[i].data.control.channel = i;
                    fMidiEvents[i].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                    fMidiEvents[k+i].type = SND_SEQ_EVENT_CONTROLLER;
                    fMidiEvents[k+i].data.control.channel = i;
                    fMidiEvents[k+i].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                midiEventCount = MAX_MIDI_NOTE;
                carla_zeroStruct<snd_seq_event_t>(fMidiEvents, midiEventCount);

                for (unsigned char i=0; i < MAX_MIDI_NOTE; ++i)
                {
                    fMidiEvents[i].type = SND_SEQ_EVENT_NOTEOFF;
                    fMidiEvents[i].data.note.channel = pData->ctrlChannel;
                    fMidiEvents[i].data.note.note    = i;
                }
            }

            if (pData->latency > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                {
#ifdef USE_JUCE
                    FloatVectorOperations::clear(pData->latencyBuffers[i], pData->latency);
#else
#endif
                }
            }

            pData->needsReset = false;

            CARLA_PROCESS_CONTINUE_CHECK;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                while (midiEventCount < kPluginMaxMidiEvents && ! pData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(pData->extNotes.data.getFirst(true));

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                    carla_zeroStruct<snd_seq_event_t>(midiEvent);

                    midiEvent.type               = (note.velo > 0) ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    midiEvent.data.note.channel  = note.channel;
                    midiEvent.data.note.note     = note.note;
                    midiEvent.data.note.velocity = note.velo;
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent  = false;
            bool isSampleAccurate = (fOptions & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t time, numEvents = pData->event.portIn->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId = 0;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                time = event.time;

                CARLA_ASSERT_INT2(time < frames, time, frames);

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && isSampleAccurate)
                {
                    if (processSingle(inBuffer, outBuffer, time - timeOffset, timeOffset, midiEventCount))
                    {
                        startTime  = 0;
                        timeOffset = time;
                        midiEventCount = 0;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;
                    }
                    else
                        startTime += timeOffset;
                }

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl:
                {
                    const EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter:
                    {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) != 0)
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
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            }
                        }
#endif

                        // Control plugin parameters
                        for (uint32_t k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (pData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? pData->param.ranges[k].min : pData->param.ranges[k].max;
                            }
                            else
                            {
                                value = pData->param.ranges[k].getUnnormalizedValue(ctrlEvent.value);

                                if (pData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            pData->postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        if ((fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct<snd_seq_event_t>(midiEvent);

                            midiEvent.time.tick = isSampleAccurate ? startTime : time;

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = event.channel;
                            midiEvent.data.control.param   = ctrlEvent.param;
                            midiEvent.data.control.value   = ctrlEvent.value*127.0f;
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == pData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (uint32_t k=0; k < pData->midiprog.count; ++k)
                            {
                                if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false);
                                    pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct<snd_seq_event_t>(midiEvent);

                            midiEvent.time.tick = isSampleAccurate ? startTime : time;

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = event.channel;
                            midiEvent.data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct<snd_seq_event_t>(midiEvent);

                            midiEvent.time.tick = isSampleAccurate ? startTime : time;

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = event.channel;
                            midiEvent.data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                            midiEventCount += 1;
                        }
                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    if (midiEventCount >= kPluginMaxMidiEvents)
                        continue;

                    const EngineMidiEvent& engineEvent(event.midi);

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(engineEvent.data);
                    uint8_t channel = event.channel;

                    // Fix bad note-off (per DSSI spec)
                    if (MIDI_IS_STATUS_NOTE_ON(status) && engineEvent.data[2] == 0)
                        status -= 0x10;

                    snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount]);
                    carla_zeroStruct<snd_seq_event_t>(midiEvent);

                    midiEvent.time.tick = isSampleAccurate ? startTime : time;

                    switch (status)
                    {
                    case MIDI_STATUS_NOTE_OFF:
                    {
                        const uint8_t note = engineEvent.data[1];

                        midiEvent.type = SND_SEQ_EVENT_NOTEOFF;
                        midiEvent.data.note.channel = channel;
                        midiEvent.data.note.note    = note;

                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, channel, note, 0.0f);
                        break;
                    }
                    case MIDI_STATUS_NOTE_ON:
                    {
                        const uint8_t note = engineEvent.data[1];
                        const uint8_t velo = engineEvent.data[2];

                        midiEvent.type = SND_SEQ_EVENT_NOTEON;
                        midiEvent.data.note.channel  = channel;
                        midiEvent.data.note.note     = note;
                        midiEvent.data.note.velocity = velo;

                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, channel, note, velo);
                        break;
                    }
                    case MIDI_STATUS_POLYPHONIC_AFTERTOUCH:
                    {
                        if (fOptions & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
                        {
                            const uint8_t note     = engineEvent.data[1];
                            const uint8_t pressure = engineEvent.data[2];

                            midiEvent.type = SND_SEQ_EVENT_KEYPRESS;
                            midiEvent.data.note.channel  = channel;
                            midiEvent.data.note.note     = note;
                            midiEvent.data.note.velocity = pressure;
                        }
                        break;
                    }
                    case MIDI_STATUS_CONTROL_CHANGE:
                    {
                        if (fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
                        {
                            const uint8_t control = engineEvent.data[1];
                            const uint8_t value   = engineEvent.data[2];

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = channel;
                            midiEvent.data.control.param   = control;
                            midiEvent.data.control.value   = value;
                        }
                        break;
                    }
                    case MIDI_STATUS_CHANNEL_PRESSURE:
                    {
                        if (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
                        {
                            const uint8_t pressure = engineEvent.data[1];

                            midiEvent.type = SND_SEQ_EVENT_CHANPRESS;
                            midiEvent.data.control.channel = channel;
                            midiEvent.data.control.value   = pressure;
                        }
                        break;
                    }
                    case MIDI_STATUS_PITCH_WHEEL_CONTROL:
                    {
                        if (fOptions & PLUGIN_OPTION_SEND_PITCHBEND)
                        {
                            const uint8_t lsb = engineEvent.data[1];
                            const uint8_t msb = engineEvent.data[2];

                            midiEvent.type = SND_SEQ_EVENT_PITCHBEND;
                            midiEvent.data.control.channel = channel;
                            midiEvent.data.control.value   = ((msb << 7) | lsb) - 8192;
                        }
                        break;
                    }
                    default:
                        continue;
                        break;
                    }

                    midiEventCount += 1;
                    break;
                }
                }
            }

            pData->postRtEvents.trySplice();

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

        if (pData->event.portOut != nullptr)
        {
            uint8_t  channel;
            uint16_t param;
            float    value;

            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                if (pData->param.data[k].midiCC > 0)
                {
                    channel = pData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(pData->param.data[k].midiCC);
                    value   = pData->param.ranges[k].getFixedAndNormalizedValue(fParamBuffers[k]);
                    pData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter, param, value);
                }
            }

        } // End of Control Output
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset, const unsigned long midiEventCount)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
        {
#ifdef USE_JUCE
            FloatVectorOperations::copy(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);
#else
#endif
        }

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
#ifdef USE_JUCE
            FloatVectorOperations::clear(fAudioOutBuffers[i], frames);
#else
#endif
        }

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
            const bool doDryWet  = (fHints & PLUGIN_CAN_DRYWET) != 0 && pData->postProc.dryWet != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) != 0 && (pData->postProc.balanceLeft != -1.0f || pData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
                        // TODO
                        //if (k < pData->latency && pData->latency < frames)
                        //    bufValue = (pData->audioIn.count == 1) ? pData->latencyBuffers[0][k] : pData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (pData->audioIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = fAudioInBuffers[(pData->audioIn.count == 1) ? 0 : i][k];
                        fAudioOutBuffers[i][k] = (fAudioOutBuffers[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
#ifdef USE_JUCE
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], frames);
#else
#endif
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
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
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }

#if 0
            // Latency, save values for next callback, TODO
            if (pData->latency > 0 && pData->latency < frames)
            {
                for (i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::copy(pData->latencyBuffers[i], inBuffer[i] + (frames - pData->latency), pData->latency);
            }
#endif
        } // End of Post-processing

#else // BUILD_BRIDGE
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("DssiPlugin::bufferSizeChanged(%i) - start", newBufferSize);

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }

        if (fHandle2 == nullptr)
        {
            for (uint32_t i=0; i < pData->audioIn.count; ++i)
            {
                CARLA_ASSERT(fAudioInBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, pData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
            }

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);
                fDescriptor->connect_port(fHandle, pData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
            }
        }
        else
        {
            if (pData->audioIn.count > 0)
            {
                CARLA_ASSERT(pData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  pData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                fDescriptor->connect_port(fHandle2, pData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
            }

            if (pData->audioOut.count > 0)
            {
                CARLA_ASSERT(pData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                fDescriptor->connect_port(fHandle,  pData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                fDescriptor->connect_port(fHandle2, pData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
            }
        }

        carla_debug("DssiPlugin::bufferSizeChanged(%i) - end", newBufferSize);
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
            for (uint32_t i=0; i < pData->audioIn.count; ++i)
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
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
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
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        if (pData->osc.data.target == nullptr)
            return;

        osc_send_control(pData->osc.data, pData->param.data[index].rindex, value);
    }

    void uiMidiProgramChange(const uint32_t index) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

        if (pData->osc.data.target == nullptr)
            return;

        osc_send_program(pData->osc.data, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

        if (pData->osc.data.target == nullptr)
            return;

#if 0
        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_ON + channel;
        midiData[2] = note;
        midiData[3] = velo;

        osc_send_midi(pData->osc.data, midiData);
#endif
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

        if (pData->osc.data.target == nullptr)
            return;

#if 0
        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;

        osc_send_midi(pData->osc.data, midiData);
#endif
    }

    // -------------------------------------------------------------------

    const void* getExtraStuff() const noexcept override
    {
        return fGuiFilename;
    }

    bool init(const char* const filename, const char* const name, const char* const label)
    {
        // ---------------------------------------------------------------
        // first checks

        if (pData->engine == nullptr)
        {
            return false;
        }

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr || label[0] == '\0')
        {
            pData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! pData->libOpen(filename))
        {
            pData->engine->setLastError(lib_error(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        const DSSI_Descriptor_Function descFn = (DSSI_Descriptor_Function)pData->libSymbol("dssi_descriptor");

        if (descFn == nullptr)
        {
            pData->engine->setLastError("Could not find the DSSI Descriptor in the plugin library");
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
            pData->engine->setLastError("Could not find the requested plugin label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check if uses global instance

        if (fDssiDescriptor->run_synth == nullptr && fDssiDescriptor->run_multiple_synths != nullptr)
        {
            if (! addUniqueMultiSynth(fDescriptor->Label))
            {
                pData->engine->setLastError("This plugin uses a global instance and can't be used more than once safely");
                return false;
            }
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
            fName = pData->engine->getUniquePluginName(name);
        else if (fDescriptor->Name != nullptr)
            fName = pData->engine->getUniquePluginName(fDescriptor->Name);
        else
            fName = pData->engine->getUniquePluginName(fDescriptor->Label);

        fFilename = filename;

        CARLA_ASSERT(fName.isNotEmpty());
        CARLA_ASSERT(fFilename.isNotEmpty());

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, (unsigned long)pData->engine->getSampleRate());

        if (fHandle == nullptr)
        {
            pData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // check for custom data extension

        if (fDssiDescriptor->configure != nullptr)
        {
            if (char* const error = fDssiDescriptor->configure(fHandle, DSSI_CUSTOMDATA_EXTENSION_KEY, ""))
            {
                if (std::strcmp(error, "true") == 0 && fDssiDescriptor->get_custom_data != nullptr && fDssiDescriptor->set_custom_data != nullptr)
                    fUsesCustomData = true;

                std::free(error);
            }
        }

        // ---------------------------------------------------------------
        // gui stuff

        //if (const char* const guiFilename = find_dssi_ui(filename, fDescriptor->Label))
        {
            //pData->osc.thread.setOscData(guiFilename, fDescriptor->Label);
            //fGuiFilename = guiFilename;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            const bool isAmSynth = fFilename.contains("amsynth", true);
            const bool isDssiVst = fFilename.contains("dssi-vst", true);

            // set default options
            fOptions = 0x0;

            fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (isAmSynth || isDssiVst)
                fOptions |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (pData->engine->getOptions().forceStereo)
                fOptions |= PLUGIN_OPTION_FORCE_STEREO;

            if (fUsesCustomData)
                fOptions |= PLUGIN_OPTION_USE_CHUNKS;

            if (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr)
            {
                fOptions |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                fOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
                fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

                if (fDssiDescriptor->run_synth == nullptr)
                    carla_stderr2("WARNING: Plugin can ONLY use run_multiple_synths!");
            }

            // load settings
            pData->idStr  = "DSSI/";
            pData->idStr += std::strrchr(filename, OS_SEP)+1;
            pData->idStr += "/";
            pData->idStr += label;
            fOptions = pData->loadSettings(fOptions, getAvailableOptions());

            // ignore settings, we need this anyway
            if (isAmSynth || isDssiVst)
                fOptions |= PLUGIN_OPTION_FIXED_BUFFERS;
        }

        return true;
    }

private:
    LADSPA_Handle fHandle;
    LADSPA_Handle fHandle2;
    const LADSPA_Descriptor* fDescriptor;
    const DSSI_Descriptor*   fDssiDescriptor;

    bool fUsesCustomData;
    const char* fGuiFilename;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;
    snd_seq_event_t fMidiEvents[kPluginMaxMidiEvents];

    static List<const char*> sMultiSynthList;

    static bool addUniqueMultiSynth(const char* const label)
    {
        CARLA_SAFE_ASSERT_RETURN(label != nullptr, true);

        for (List<const char*>::Itenerator it = sMultiSynthList.begin(); it.valid(); it.next())
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
        CARLA_SAFE_ASSERT_RETURN(label != nullptr,);

        for (List<const char*>::Itenerator it = sMultiSynthList.begin(); it.valid(); it.next())
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

List<const char*> DssiPlugin::sMultiSynthList;

CARLA_BACKEND_END_NAMESPACE

#else // WANT_DSSI
# warning Building without DSSI support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newDSSI(const Initializer& init)
{
    carla_debug("CarlaPlugin::newDSSI({%p, \"%s\", \"%s\", \"%s\"})", init.engine, init.filename, init.name, init.label);

#ifdef WANT_DSSI
    DssiPlugin* const plugin(new DssiPlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
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
