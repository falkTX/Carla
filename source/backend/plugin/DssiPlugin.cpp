/*
 * Carla DSSI Plugin
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaDssiUtils.hpp"
#include "CarlaMathUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------

class DssiPlugin : public CarlaPlugin
{
public:
    DssiPlugin(CarlaEngine* const engine, const uint id) noexcept
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fDescriptor(nullptr),
          fDssiDescriptor(nullptr),
          fUsesCustomData(false),
          fUiFilename(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fParamBuffers(nullptr),
          fLatencyChanged(false),
          fLatencyIndex(-1)
    {
        carla_debug("DssiPlugin::DssiPlugin(%p, %i)", engine, id);

        pData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
    }

    ~DssiPlugin() noexcept override
    {
        carla_debug("DssiPlugin::~DssiPlugin()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            showCustomUI(false);

            pData->osc.thread.stopThread(static_cast<int>(pData->engine->getOptions().uiBridgesTimeout * 2));
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
            if (pData->name != nullptr && fDssiDescriptor != nullptr && fDssiDescriptor->run_synth == nullptr && fDssiDescriptor->run_multiple_synths != nullptr)
                removeUniqueMultiSynth(fDescriptor->Label);

            if (fDescriptor->cleanup != nullptr)
            {
                if (fHandle != nullptr)
                {
                    try {
                        fDescriptor->cleanup(fHandle);
                    } CARLA_SAFE_EXCEPTION("DSSI cleanup");
                }

                if (fHandle2 != nullptr)
                {
                    try {
                        fDescriptor->cleanup(fHandle2);
                    } CARLA_SAFE_EXCEPTION("DSSI cleanup #2");
                }
            }

            fHandle  = nullptr;
            fHandle2 = nullptr;
            fDescriptor = nullptr;
            fDssiDescriptor = nullptr;
        }

        if (fUiFilename != nullptr)
        {
            delete[] fUiFilename;
            fUiFilename = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_DSSI;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr, PLUGIN_CATEGORY_NONE);

        if (pData->audioIn.count == 0 && pData->audioOut.count > 0 && (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr))
            return PLUGIN_CATEGORY_SYNTH;

        return CarlaPlugin::getCategory();
    }

    int64_t getUniqueId() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0);

        return static_cast<int64_t>(fDescriptor->UniqueID);
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsesCustomData, 0);
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->get_custom_data != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fHandle2 == nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        *dataPtr = nullptr;

        int ret = 0;
        ulong dataSize = 0;

        try {
            ret = fDssiDescriptor->get_custom_data(fHandle, dataPtr, &dataSize);
        } CARLA_SAFE_EXCEPTION_RETURN("DssiPlugin::getChunkData", 0);

        return (ret != 0) ? dataSize : 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
#ifdef __USE_GNU
        const bool isDssiVst(strcasestr(pData->filename, "dssi-vst") != nullptr);
#else
        const bool isDssiVst(std::strstr(pData->filename, "dssi-vst") != nullptr);
#endif

        uint options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (! isDssiVst)
        {
            if (fLatencyIndex == -1)
                options |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (pData->engine->getProccessMode() != ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                if (pData->options & PLUGIN_OPTION_FORCE_STEREO)
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

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fParamBuffers[parameterId];
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor        != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Label != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Label, STR_MAX);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor        != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Maker != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Maker, STR_MAX);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor            != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Copyright != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor       != nullptr, nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->Name != nullptr, nullStrBuf(strBuf));

        std::strncpy(strBuf, fDescriptor->Name, STR_MAX);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,           nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);

        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames[rindex] != nullptr,             nullStrBuf(strBuf));

        if (getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, true))
            return;

        std::strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);

        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), nullStrBuf(strBuf));

        if (getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, false))
            return;

        nullStrBuf(strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // nothing

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
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
            try {
                fDssiDescriptor->configure(fHandle, key, value);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fDssiDescriptor->configure(fHandle2, key, value);
                } catch(...) {}
            }
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

    void setChunkData(const void* const data, const std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsesCustomData,);
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->set_custom_data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle2 == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        {
            const ScopedSingleProcessLocker spl(this, true);

            try {
                fDssiDescriptor->set_custom_data(fHandle, const_cast<void*>(data), dataSize);
            } CARLA_SAFE_EXCEPTION("DssiPlugin::setChunkData");
        }

#ifdef BUILD_BRIDGE
        const bool sendOsc(false);
#else
        const bool sendOsc(pData->engine->isOscControlRegistered());
#endif
        pData->updateParameterValues(this, sendOsc, true, false);
    }

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDssiDescriptor->select_program != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        if (index >= 0)
        {
            const uint32_t bank(pData->midiprog.data[index].bank);
            const uint32_t program(pData->midiprog.data[index].program);

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            try {
                fDssiDescriptor->select_program(fHandle, bank, program);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fDssiDescriptor->select_program(fHandle2, bank, program);
                } catch(...) {}
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        if (yesNo)
        {
            pData->osc.data.clear();
            pData->osc.thread.startThread();
        }
        else
        {
            pData->transientTryCounter = 0;

            if (pData->osc.data.target != nullptr)
            {
                osc_send_hide(pData->osc.data);
                osc_send_quit(pData->osc.data);
                pData->osc.data.clear();
            }

            pData->osc.thread.stopThread(static_cast<int>(pData->engine->getOptions().uiBridgesTimeout * 2));
        }
    }

    void idle() override
    {
        if (fLatencyChanged && fLatencyIndex != -1)
        {
            fLatencyChanged = false;

            const int32_t latency(static_cast<int32_t>(fParamBuffers[fLatencyIndex]));

            if (latency >= 0)
            {
                const uint32_t ulatency(static_cast<uint32_t>(latency));

                if (pData->latency != ulatency)
                {
                    carla_stdout("latency changed to %i", latency);

                    const ScopedSingleProcessLocker sspl(this, true);

                    pData->latency = ulatency;
                    pData->client->setLatency(ulatency);
#ifndef BUILD_BRIDGE
                    pData->recreateLatencyBuffers();
#endif
                }
            }
            else
                carla_safe_assert_int("latency >= 0", __FILE__, __LINE__, latency);
        }

        CarlaPlugin::idle();
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
        const uint32_t portCount(getSafePortCount());

        uint32_t aIns, aOuts, mIns, params;
        aIns = aOuts = mIns = params = 0;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        for (uint32_t i=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType(fDescriptor->PortDescriptors[i]);

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

        if ((pData->options & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1))
        {
            if (fHandle2 == nullptr)
            {
                try {
                    fHandle2 = fDescriptor->instantiate(fDescriptor, static_cast<ulong>(sampleRate));
                } CARLA_SAFE_EXCEPTION("DSSI instantiate #2");
            }

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
            pData->param.createNew(params, true);

            fParamBuffers = new float[params];
            FloatVectorOperations::clear(fParamBuffers, static_cast<int>(params));
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                if (fDescriptor->PortNames[i] != nullptr && fDescriptor->PortNames[i][0] != '\0')
                {
                    portName += fDescriptor->PortNames[i];
                }
                else
                {
                    if (LADSPA_IS_PORT_INPUT(portType))
                    {
                        if (aIns > 1)
                        {
                            portName += "audio-in_";
                            portName += CarlaString(iAudioIn+1);
                        }
                        else
                            portName += "audio-in";
                    }
                    else
                    {
                        if (aOuts > 1)
                        {
                            portName += "audio-out_";
                            portName += CarlaString(iAudioOut+1);
                        }
                        else
                            portName += "audio-out";
                    }
                }

                portName.truncate(portNameSize);

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    const uint32_t j = iAudioIn++;
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
                    const uint32_t j = iAudioOut++;
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
                const uint32_t j = iCtrl++;
                pData->param.data[j].index  = static_cast<int32_t>(j);
                pData->param.data[j].rindex = static_cast<int32_t>(i);

                const char* const paramName(fDescriptor->PortNames[i] != nullptr ? fDescriptor->PortNames[i] : "unknown");

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
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': min > max", paramName);
                    min = max - 0.1f;
                }
                else if (min == max)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': min == maxf", paramName);
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
                    const float range = max - min;
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
                    pData->param.data[j].type = PARAMETER_OUTPUT;

                    if (std::strcmp(paramName, "latency") == 0 || std::strcmp(paramName, "_latency") == 0)
                    {
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;
                        pData->param.special[j] = PARAMETER_SPECIAL_LATENCY;
                        CARLA_SAFE_ASSERT(fLatencyIndex == -1);
                        fLatencyIndex = static_cast<int32_t>(j);
                    }
                    else
                    {
                        pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
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

                try {
                    fDescriptor->connect_port(fHandle, i, &fParamBuffers[j]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port parameter");

                if (fHandle2 != nullptr)
                {
                    try {
                        fDescriptor->connect_port(fHandle2, i, &fParamBuffers[j]);
                    } CARLA_SAFE_EXCEPTION("DSSI connect_port parameter #2");
                }
            }
            else
            {
                // Not Audio or Control
                carla_stderr2("ERROR - Got a broken Port (neither Audio or Control)");

                try {
                    fDescriptor->connect_port(fHandle, i, nullptr);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port null");

                if (fHandle2 != nullptr)
                {
                    try {
                        fDescriptor->connect_port(fHandle2, i, nullptr);
                    } CARLA_SAFE_EXCEPTION("DSSI connect_port null #2");
                }
            }
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
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
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        if (forcedStereoIn || forcedStereoOut)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else
            pData->options &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        pData->hints = 0x0;

        if (LADSPA_IS_HARD_RT_CAPABLE(fDescriptor->Properties))
            pData->hints |= PLUGIN_IS_RTSAFE;

        if (fUiFilename != nullptr)
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;

#ifndef BUILD_BRIDGE
        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;
#endif

        // extra plugin hints
        pData->extraHints = 0x0;

        if (mIns > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        // check latency
        if (fLatencyIndex >= 0)
        {
            // we need to pre-run the plugin so it can update its latency control-port

            float tmpIn [(aIns > 0)  ? aIns  : 1][2];
            float tmpOut[(aOuts > 0) ? aOuts : 1][2];

            for (uint32_t j=0; j < aIns; ++j)
            {
                tmpIn[j][0] = 0.0f;
                tmpIn[j][1] = 0.0f;

                try {
                    fDescriptor->connect_port(fHandle, pData->audioIn.ports[j].rindex, tmpIn[j]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port latency input");
            }

            for (uint32_t j=0; j < aOuts; ++j)
            {
                tmpOut[j][0] = 0.0f;
                tmpOut[j][1] = 0.0f;

                try {
                    fDescriptor->connect_port(fHandle, pData->audioOut.ports[j].rindex, tmpOut[j]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port latency output");
            }

            if (fDescriptor->activate != nullptr)
            {
                try {
                    fDescriptor->activate(fHandle);
                } CARLA_SAFE_EXCEPTION("DSSI latency activate");
            }

            try {
                fDescriptor->run(fHandle, 2);
            } CARLA_SAFE_EXCEPTION("DSSI latency run");

            if (fDescriptor->deactivate != nullptr)
            {
                try {
                    fDescriptor->deactivate(fHandle);
                } CARLA_SAFE_EXCEPTION("DSSI latency deactivate");
            }

            const int32_t latency(static_cast<int32_t>(fParamBuffers[fLatencyIndex]));

            if (latency >= 0)
            {
                const uint32_t ulatency(static_cast<uint32_t>(latency));

                if (pData->latency != ulatency)
                {
                    carla_stdout("latency = %i", latency);

                    pData->latency = ulatency;
                    pData->client->setLatency(ulatency);
#ifndef BUILD_BRIDGE
                    pData->recreateLatencyBuffers();
#endif
                }
            }
            else
                carla_safe_assert_int("latency >= 0", __FILE__, __LINE__, latency);

            fLatencyChanged = false;
        }

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("DssiPlugin::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("DssiPlugin::reloadPrograms(%s)", bool2str(doInit));
        const uint32_t oldCount = pData->midiprog.count;
        const int32_t  current  = pData->midiprog.current;

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t newCount = 0;
        if (fDssiDescriptor->get_program != nullptr && fDssiDescriptor->select_program != nullptr)
        {
            for (; fDssiDescriptor->get_program(fHandle, newCount) != nullptr;)
                ++newCount;
        }

        if (newCount > 0)
        {
            pData->midiprog.createNew(newCount);

            // Update data
            for (uint32_t i=0; i < newCount; ++i)
            {
                const DSSI_Program_Descriptor* const pdesc(fDssiDescriptor->get_program(fHandle, i));
                CARLA_SAFE_ASSERT_CONTINUE(pdesc != nullptr);
                CARLA_SAFE_ASSERT(pdesc->Name != nullptr);

                pData->midiprog.data[i].bank    = static_cast<uint32_t>(pdesc->Bank);
                pData->midiprog.data[i].program = static_cast<uint32_t>(pdesc->Program);
                pData->midiprog.data[i].name    = carla_strdup(pdesc->Name);
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (pData->engine->isOscControlRegistered())
        {
            pData->engine->oscSend_control_set_midi_program_count(pData->id, newCount);

            for (uint32_t i=0; i < newCount; ++i)
                pData->engine->oscSend_control_set_midi_program_data(pData->id, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
        }
#endif

        if (doInit)
        {
            if (newCount > 0)
                setMidiProgram(0, false, false, false);
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (newCount == oldCount+1)
            {
                // one midi program added, probably created by user
                pData->midiprog.current = static_cast<int32_t>(oldCount);
                programChanged = true;
            }
            else if (current < 0 && newCount > 0)
            {
                // programs exist now, but not before
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && newCount == 0)
            {
                // programs existed before, but not anymore
                pData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(newCount))
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

            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->activate != nullptr)
        {
            try {
                fDescriptor->activate(fHandle);
            } CARLA_SAFE_EXCEPTION("DSSI activate");

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->activate(fHandle2);
                } CARLA_SAFE_EXCEPTION("DSSI activate #2");
            }
        }
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->deactivate != nullptr)
        {
            try {
                fDescriptor->deactivate(fHandle);
            } CARLA_SAFE_EXCEPTION("DSSI deactivate");

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->deactivate(fHandle2);
                } CARLA_SAFE_EXCEPTION("DSSI deactivate #2");
            }
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
                FloatVectorOperations::clear(outBuffer[i], static_cast<int>(frames));
            return;
        }

        ulong midiEventCount = 0;

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                midiEventCount = MAX_MIDI_CHANNELS*2;
                carla_zeroStruct<snd_seq_event_t>(fMidiEvents, midiEventCount);

                for (uchar i=0, k=MAX_MIDI_CHANNELS; i < MAX_MIDI_CHANNELS; ++i)
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

                for (uchar i=0; i < MAX_MIDI_NOTE; ++i)
                {
                    fMidiEvents[i].type = SND_SEQ_EVENT_NOTEOFF;
                    fMidiEvents[i].data.note.channel = static_cast<uchar>(pData->ctrlChannel);
                    fMidiEvents[i].data.note.note    = i;
                }
            }

#ifndef BUILD_BRIDGE
            if (pData->latency > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::clear(pData->latencyBuffers[i], static_cast<int>(pData->latency));
            }
#endif

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                ExternalMidiNote note = { 0, 0, 0 };

                for (; midiEventCount < kPluginMaxMidiEvents && ! pData->extNotes.data.isEmpty();)
                {
                    note = pData->extNotes.data.getFirst(note, true);

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                    carla_zeroStruct<snd_seq_event_t>(midiEvent);

                    midiEvent.type               = (note.velo > 0) ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    midiEvent.data.note.channel  = static_cast<uchar>(note.channel);
                    midiEvent.data.note.note     = note.note;
                    midiEvent.data.note.velocity = note.velo;
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE
            bool       allNotesOffSent  = false;
#endif
            const bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t numEvents  = pData->event.portIn->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (isSampleAccurate && event.time > timeOffset)
                {
                    if (processSingle(inBuffer, outBuffer, event.time - timeOffset, timeOffset, midiEventCount))
                    {
                        startTime  = 0;
                        timeOffset = event.time;
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

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
#ifndef BUILD_BRIDGE
                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                break;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
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
                                break;
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
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
                            break;
                        }

                        // check if event is already handled
                        if (k != pData->param.count)
                            break;

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct<snd_seq_event_t>(midiEvent);

                            midiEvent.time.tick = isSampleAccurate ? startTime : event.time;

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = event.channel;
                            midiEvent.data.control.param   = ctrlEvent.param;
                            midiEvent.data.control.value   = int8_t(ctrlEvent.value*127.0f);
                        }
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (uint32_t k=0; k < pData->midiprog.count; ++k)
                            {
                                if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                {
                                    const int32_t index(static_cast<int32_t>(k));
                                    setMidiProgram(index, false, false, false);
                                    pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, index, 0, 0.0f);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct<snd_seq_event_t>(midiEvent);

                            midiEvent.time.tick = isSampleAccurate ? startTime : event.time;

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = event.channel;
                            midiEvent.data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

                            if (midiEventCount >= kPluginMaxMidiEvents)
                                continue;

                            snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount++]);
                            carla_zeroStruct<snd_seq_event_t>(midiEvent);

                            midiEvent.time.tick = isSampleAccurate ? startTime : event.time;

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = event.channel;
                            midiEvent.data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                            midiEventCount += 1;
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    if (midiEventCount >= kPluginMaxMidiEvents)
                        continue;

                    const EngineMidiEvent& engineEvent(event.midi);

                    uint8_t status  = uint8_t(MIDI_GET_STATUS_FROM_DATA(engineEvent.data));
                    uint8_t channel = event.channel;

                    // Fix bad note-off (per DSSI spec)
                    if (MIDI_IS_STATUS_NOTE_ON(status) && engineEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    snd_seq_event_t& midiEvent(fMidiEvents[midiEventCount]);
                    carla_zeroStruct<snd_seq_event_t>(midiEvent);

                    midiEvent.time.tick = isSampleAccurate ? startTime : event.time;

                    switch (status)
                    {
                    case MIDI_STATUS_NOTE_OFF: {
                        const uint8_t note = engineEvent.data[1];

                        midiEvent.type = SND_SEQ_EVENT_NOTEOFF;
                        midiEvent.data.note.channel = channel;
                        midiEvent.data.note.note    = note;

                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, channel, note, 0.0f);
                        break;
                    }

                    case MIDI_STATUS_NOTE_ON: {
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
                        if (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
                        {
                            const uint8_t note     = engineEvent.data[1];
                            const uint8_t pressure = engineEvent.data[2];

                            midiEvent.type = SND_SEQ_EVENT_KEYPRESS;
                            midiEvent.data.note.channel  = channel;
                            midiEvent.data.note.note     = note;
                            midiEvent.data.note.velocity = pressure;
                        }
                        break;

                    case MIDI_STATUS_CONTROL_CHANGE:
                        if (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
                        {
                            const uint8_t control = engineEvent.data[1];
                            const uint8_t value   = engineEvent.data[2];

                            midiEvent.type = SND_SEQ_EVENT_CONTROLLER;
                            midiEvent.data.control.channel = channel;
                            midiEvent.data.control.param   = control;
                            midiEvent.data.control.value   = value;
                        }
                        break;

                    case MIDI_STATUS_CHANNEL_PRESSURE:
                        if (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
                        {
                            const uint8_t pressure = engineEvent.data[1];

                            midiEvent.type = SND_SEQ_EVENT_CHANPRESS;
                            midiEvent.data.control.channel = channel;
                            midiEvent.data.control.value   = pressure;
                        }
                        break;

                    case MIDI_STATUS_PITCH_WHEEL_CONTROL:
                        if (pData->options & PLUGIN_OPTION_SEND_PITCHBEND)
                        {
                            const uint8_t lsb = engineEvent.data[1];
                            const uint8_t msb = engineEvent.data[2];

                            midiEvent.type = SND_SEQ_EVENT_PITCHBEND;
                            midiEvent.data.control.channel = channel;
                            midiEvent.data.control.value   = ((msb << 7) | lsb) - 8192;
                        }
                        break;

                    default:
                        continue;
                        break;
                    } // switch (status)

                    midiEventCount += 1;
                    break;
                } // case kEngineEventTypeMidi
                } // switch (event.type)
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

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Latency, save values for next callback

        if (fLatencyIndex != -1)
        {
            if (pData->latency != static_cast<uint32_t>(fParamBuffers[fLatencyIndex]))
            {
                fLatencyChanged = true;
            }
            else if (pData->latency > 0)
            {
                if (pData->latency <= frames)
                {
                    for (uint32_t i=0; i < pData->audioIn.count; ++i)
                        FloatVectorOperations::copy(pData->latencyBuffers[i], inBuffer[i]+(frames-pData->latency), static_cast<int>(pData->latency));
                }
                else
                {
                    for (uint32_t i=0, j, k; i < pData->audioIn.count; ++i)
                    {
                        for (k=0; k < pData->latency-frames; ++k)
                            pData->latencyBuffers[i][k] = pData->latencyBuffers[i][k+frames];
                        for (j=0; k < pData->latency; ++j, ++k)
                            pData->latencyBuffers[i][k] = inBuffer[i][j];
                    }
                }
            }
        }

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

                pData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (pData->param.data[k].midiCC > 0)
                {
                    channel = pData->param.data[k].midiChannel;
                    param   = static_cast<uint16_t>(pData->param.data[k].midiCC);
                    value   = pData->param.ranges[k].getNormalizedValue(fParamBuffers[k]);
                    pData->event.portOut->writeControlEvent(0, channel, kEngineControlEventTypeParameter, param, value);
                }
            }
        } // End of Control Output
#endif
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset, const ulong midiEventCount)
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
            FloatVectorOperations::copy(fAudioInBuffers[i], inBuffer[i]+timeOffset, static_cast<int>(frames));

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            FloatVectorOperations::clear(fAudioOutBuffers[i], static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        // TODO - try catch

        if (fDssiDescriptor->run_synth != nullptr)
        {
            fDssiDescriptor->run_synth(fHandle, frames, fMidiEvents, midiEventCount);

            if (fHandle2 != nullptr)
                fDssiDescriptor->run_synth(fHandle2, frames, fMidiEvents, midiEventCount);
        }
        else if (fDssiDescriptor->run_multiple_synths != nullptr)
        {
            ulong instances = (fHandle2 != nullptr) ? 2 : 1;
            LADSPA_Handle handlePtr[2] = { fHandle, fHandle2 };
            snd_seq_event_t* midiEventsPtr[2] = { fMidiEvents, fMidiEvents };
            ulong midiEventCountPtr[2] = { midiEventCount, midiEventCount };
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
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && pData->postProc.dryWet != 1.0f;
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && (pData->postProc.balanceLeft != -1.0f || pData->postProc.balanceRight != 1.0f);
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (k < pData->latency)
                            bufValue = pData->latencyBuffers[isMono ? 0 : i][k];
                        else if (pData->latency < frames)
                            bufValue = fAudioInBuffers[isMono ? 0 : i][k-pData->latency];
                        else
                            bufValue = fAudioInBuffers[isMono ? 0 : i][k];

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
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], static_cast<int>(frames));
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

                try {
                    fDescriptor->connect_port(fHandle, pData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port audio input");
            }

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                CARLA_ASSERT(fAudioOutBuffers[i] != nullptr);

                try {
                    fDescriptor->connect_port(fHandle, pData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port audio output");
            }
        }
        else
        {
            if (pData->audioIn.count > 0)
            {
                CARLA_ASSERT(pData->audioIn.count == 2);
                CARLA_ASSERT(fAudioInBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioInBuffers[1] != nullptr);

                try {
                    fDescriptor->connect_port(fHandle, pData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port audio input #1");

                try {
                    fDescriptor->connect_port(fHandle2, pData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port audio input #2");
            }

            if (pData->audioOut.count > 0)
            {
                CARLA_ASSERT(pData->audioOut.count == 2);
                CARLA_ASSERT(fAudioOutBuffers[0] != nullptr);
                CARLA_ASSERT(fAudioOutBuffers[1] != nullptr);

                try {
                    fDescriptor->connect_port(fHandle, pData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port audio output #1");

                try {
                    fDescriptor->connect_port(fHandle2, pData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
                } CARLA_SAFE_EXCEPTION("DSSI connect_port audio output #2");
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

    void clearBuffers() noexcept override
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
    // OSC stuff

    void updateOscURL() override
    {
        // DSSI does not support this
        if (! pData->osc.thread.isThreadRunning())
            return;

        showCustomUI(false);
        //showCustomUI(true);
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        if (pData->osc.data.target == nullptr)
            return;

        osc_send_control(pData->osc.data, pData->param.data[index].rindex, value);
    }

    void uiMidiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

        if (pData->osc.data.target == nullptr)
            return;

        osc_send_program(pData->osc.data, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

        if (pData->osc.data.target == nullptr)
            return;

#if 0
        uint8_t midiData[4];
        midiData[0] = 0;
        midiData[1] = static_cast<uint8_t>(MIDI_STATUS_NOTE_ON + channel);
        midiData[2] = note;
        midiData[3] = velo;

        osc_send_midi(pData->osc.data, midiData);
#endif
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

        if (pData->osc.data.target == nullptr)
            return;

#if 0
        uint8_t midiData[4];
        midiData[0] = 0;
        midiData[1] = static_cast<uint8_t>(MIDI_STATUS_NOTE_OFF + channel);
        midiData[2] = note;
        midiData[3] = 0;

        osc_send_midi(pData->osc.data, midiData);
#endif
    }

    // -------------------------------------------------------------------

    void* getNativeHandle() const noexcept override
    {
        return fHandle;
    }

    const void* getNativeDescriptor() const noexcept override
    {
        return fDssiDescriptor;
    }

    const void* getExtraStuff() const noexcept override
    {
        return fUiFilename;
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

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
            pData->engine->setLastError(pData->libError(filename));
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

        ulong i = 0;

        for (;;)
        {
            try {
                fDssiDescriptor = descFn(i++);
            }
            catch(...) {
                carla_stderr2("Caught exception when trying to get LADSPA descriptor");
                fDescriptor     = nullptr;
                fDssiDescriptor = nullptr;
                break;
            }

            if (fDssiDescriptor == nullptr)
                break;

            fDescriptor = fDssiDescriptor->LADSPA_Plugin;

            if (fDescriptor == nullptr)
            {
                carla_stderr2("WARNING - Missing LADSPA interface, will not use this plugin");
                fDssiDescriptor = nullptr;
                break;
            }
            if (fDescriptor->Label == nullptr || fDescriptor->Label[0] == '\0')
            {
                carla_stderr2("WARNING - Got an invalid label, will not use this plugin");
                fDescriptor     = nullptr;
                fDssiDescriptor = nullptr;
                break;
            }
            if (fDescriptor->run == nullptr)
            {
                carla_stderr2("WARNING - Plugin has no run, cannot use it");
                fDescriptor     = nullptr;
                fDssiDescriptor = nullptr;
                break;
            }

            if (std::strcmp(fDescriptor->Label, label) == 0)
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

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else if (fDescriptor->Name != nullptr && fDescriptor->Name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(fDescriptor->Name);
        else
            pData->name = pData->engine->getUniquePluginName(fDescriptor->Label);

        pData->filename = carla_strdup(filename);

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

        try {
            fHandle = fDescriptor->instantiate(fDescriptor, (ulong)pData->engine->getSampleRate());
        } CARLA_SAFE_EXCEPTION("DSSI instantiate");

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

        if (const char* const guiFilename = find_dssi_ui(filename, fDescriptor->Label))
        {
            pData->osc.thread.setOscData(guiFilename, fDescriptor->Label);
            fUiFilename = guiFilename;
        }

        // ---------------------------------------------------------------
        // set default options

#ifdef __USE_GNU
        const bool isDssiVst(strcasestr(pData->filename, "dssi-vst") != nullptr);
#else
        const bool isDssiVst(std::strstr(pData->filename, "dssi-vst") != nullptr);
#endif

        pData->options  = 0x0;
        pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (fLatencyIndex >= 0 || isDssiVst)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (pData->engine->getOptions().forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fUsesCustomData)
            pData->options |= PLUGIN_OPTION_USE_CHUNKS;

        if (fDssiDescriptor->run_synth != nullptr || fDssiDescriptor->run_multiple_synths != nullptr)
        {
            pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
            pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

            if (fDssiDescriptor->run_synth == nullptr)
                carla_stderr("WARNING: Plugin can ONLY use run_multiple_synths!");
        }

        return true;
    }

    // -------------------------------------------------------------------

private:
    LADSPA_Handle fHandle;
    LADSPA_Handle fHandle2;
    const LADSPA_Descriptor* fDescriptor;
    const DSSI_Descriptor*   fDssiDescriptor;

    bool fUsesCustomData;
    const char* fUiFilename;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;

    bool    fLatencyChanged;
    int32_t fLatencyIndex; // -1 if invalid

    snd_seq_event_t fMidiEvents[kPluginMaxMidiEvents];

    // -------------------------------------------------------------------

    uint32_t getSafePortCount() const noexcept
    {
        if (fDescriptor->PortCount == 0)
            return 0;

        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortDescriptors != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortRangeHints != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames != nullptr, 0);

        return static_cast<uint32_t>(fDescriptor->PortCount);
    }

    bool getSeparatedParameterNameOrUnit(const char* const paramName, char* const strBuf, const bool wantName) const noexcept
    {
        if (_getSeparatedParameterNameOrUnitImpl(paramName, strBuf, wantName, true))
            return true;
        if (_getSeparatedParameterNameOrUnitImpl(paramName, strBuf, wantName, false))
            return true;
        return false;
    }

    bool _getSeparatedParameterNameOrUnitImpl(const char* const paramName, char* const strBuf, const bool wantName, const bool useBracket) const noexcept
    {
        const char* const sepBracketStart(std::strstr(paramName, useBracket ? " [" : " ("));

        if (sepBracketStart == nullptr)
            return false;

        const char* const sepBracketEnd(std::strstr(sepBracketStart, useBracket ? "]" : ")"));

        if (sepBracketEnd == nullptr)
            return false;

        const size_t unitSize(static_cast<size_t>(sepBracketEnd-sepBracketStart-2));

        if (unitSize > 7) // very unlikely to have such big unit
            return false;

        const size_t sepIndex(std::strlen(paramName)-unitSize-3);

        // just in case
        if (sepIndex > STR_MAX)
            return false;

        if (wantName)
        {
            std::strncpy(strBuf, paramName, sepIndex);
            strBuf[sepIndex] = '\0';
        }
        else
        {
            std::strncpy(strBuf, paramName+(sepIndex+2), unitSize);
            strBuf[unitSize] = '\0';
        }

        return true;
    }

    // -------------------------------------------------------------------

    static LinkedList<const char*> sMultiSynthList;

    static bool addUniqueMultiSynth(const char* const label) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0', false);

        const char* dlabel = nullptr;

        try {
            dlabel = carla_strdup(label);
        } catch(...) { return false; }

        for (LinkedList<const char*>::Itenerator it = sMultiSynthList.begin(); it.valid(); it.next())
        {
            const char* const itLabel(it.getValue());

            if (std::strcmp(dlabel, itLabel) == 0)
            {
                delete[] dlabel;
                return false;
            }
        }

        return sMultiSynthList.append(dlabel);
    }

    static void removeUniqueMultiSynth(const char* const label) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0',);

        for (LinkedList<const char*>::Itenerator it = sMultiSynthList.begin(); it.valid(); it.next())
        {
            const char* const itLabel(it.getValue());

            if (std::strcmp(label, itLabel) == 0)
            {
                sMultiSynthList.remove(it);
                delete[] itLabel;
                break;
            }
        }
    }

    // -------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DssiPlugin)
};

LinkedList<const char*> DssiPlugin::sMultiSynthList;

// -------------------------------------------------------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newDSSI(const Initializer& init)
{
    carla_debug("CarlaPlugin::newDSSI({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})", init.engine, init.filename, init.name, init.label, init.uniqueId);

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
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
