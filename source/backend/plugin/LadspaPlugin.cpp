/*
 * Carla LADSPA Plugin
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

#ifdef WANT_LADSPA

#include "CarlaLadspaUtils.hpp"
#include "CarlaLibUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

class LadspaPlugin : public CarlaPlugin
{
public:
    LadspaPlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fParamBuffers(nullptr)
    {
        carla_debug("LadspaPlugin::LadspaPlugin(%p, %i)", engine, id);
    }

    ~LadspaPlugin() override
    {
        carla_debug("LadspaPlugin::~LadspaPlugin()");

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
        }

        if (fRdfDescriptor != nullptr)
        {
            delete fRdfDescriptor;
            fRdfDescriptor = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_LADSPA;
    }

    PluginCategory getCategory() const override
    {
        if (fRdfDescriptor != nullptr)
        {
            const LADSPA_PluginType category(fRdfDescriptor->Type);

            // Specific Types
            if (category & (LADSPA_PLUGIN_DELAY|LADSPA_PLUGIN_REVERB))
                return PLUGIN_CATEGORY_DELAY;
            if (category & (LADSPA_PLUGIN_PHASER|LADSPA_PLUGIN_FLANGER|LADSPA_PLUGIN_CHORUS))
                return PLUGIN_CATEGORY_MODULATOR;
            if (category & (LADSPA_PLUGIN_AMPLIFIER))
                return PLUGIN_CATEGORY_DYNAMICS;
            if (category & (LADSPA_PLUGIN_UTILITY|LADSPA_PLUGIN_SPECTRAL|LADSPA_PLUGIN_FREQUENCY_METER))
                return PLUGIN_CATEGORY_UTILITY;

            // Pre-set LADSPA Types
            if (LADSPA_IS_PLUGIN_DYNAMICS(category))
                return PLUGIN_CATEGORY_DYNAMICS;
            if (LADSPA_IS_PLUGIN_AMPLITUDE(category))
                return PLUGIN_CATEGORY_MODULATOR;
            if (LADSPA_IS_PLUGIN_EQ(category))
                return PLUGIN_CATEGORY_EQ;
            if (LADSPA_IS_PLUGIN_FILTER(category))
                return PLUGIN_CATEGORY_FILTER;
            if (LADSPA_IS_PLUGIN_FREQUENCY(category))
                return PLUGIN_CATEGORY_UTILITY;
            if (LADSPA_IS_PLUGIN_SIMULATOR(category))
                return PLUGIN_CATEGORY_OTHER;
            if (LADSPA_IS_PLUGIN_TIME(category))
                return PLUGIN_CATEGORY_DELAY;
            if (LADSPA_IS_PLUGIN_GENERATOR(category))
                return PLUGIN_CATEGORY_SYNTH;
        }

        return getPluginCategoryFromName(fName);
    }

    long getUniqueId() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0);

        return fDescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            return port->ScalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int getAvailableOptions() const override
    {
        const bool isDssiVst = fFilename.contains("dssi-vst", true);

        unsigned int options = 0x0;

        if (! isDssiVst)
        {
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (pData->engine->getProccessMode() != PROCESS_MODE_CONTINUOUS_RACK)
            {
                if (fOptions & PLUGIN_OPTION_FORCE_STEREO)
                    options |= PLUGIN_OPTION_FORCE_STEREO;
                else if (pData->audioIn.count <= 1 && pData->audioOut.count <= 1 && (pData->audioIn.count != 0 || pData->audioOut.count != 0))
                    options |= PLUGIN_OPTION_FORCE_STEREO;
            }
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId), 0.0f);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);

            if (scalePointId < port->ScalePointCount)
            {
                const LADSPA_RDF_ScalePoint* const scalePoint(&port->ScalePoints[scalePointId]);
                return scalePoint->Value;
            }
        }

        return 0.0f;
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

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Creator != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Creator, STR_MAX);
        else if (fDescriptor->Maker != nullptr)
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

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Title, STR_MAX);
        else if (fDescriptor->Name != nullptr)
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

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);

            if (LADSPA_PORT_HAS_LABEL(port->Hints) && port->Label != nullptr)
            {
                std::strncpy(strBuf, port->Label, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);

            if (LADSPA_PORT_HAS_UNIT(port->Hints))
            {
                switch (port->Unit)
                {
                case LADSPA_UNIT_DB:
                    std::strncpy(strBuf, "dB", STR_MAX);
                    return;
                case LADSPA_UNIT_COEF:
                    std::strncpy(strBuf, "(coef)", STR_MAX);
                    return;
                case LADSPA_UNIT_HZ:
                    std::strncpy(strBuf, "Hz", STR_MAX);
                    return;
                case LADSPA_UNIT_S:
                    std::strncpy(strBuf, "s", STR_MAX);
                    return;
                case LADSPA_UNIT_MS:
                    std::strncpy(strBuf, "ms", STR_MAX);
                    return;
                case LADSPA_UNIT_MIN:
                    std::strncpy(strBuf, "min", STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(scalePointId < getParameterScalePointCount(parameterId),);

        const int32_t rindex(pData->param.data[parameterId].rindex);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);

            if (scalePointId < port->ScalePointCount)
            {
                const LADSPA_RDF_ScalePoint* const scalePoint(&port->ScalePoints[scalePointId]);

                if (scalePoint->Label != nullptr)
                {
                    std::strncpy(strBuf, scalePoint->Label, STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
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

    // -------------------------------------------------------------------
    // Set gui stuff

    // nothing

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("LadspaPlugin::reload() - start");
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        const ProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));
        const uint32_t portCount(static_cast<uint32_t>(fDescriptor->PortCount));

        uint32_t aIns, aOuts, params, j;
        aIns = aOuts = params = 0;

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
            FloatVectorOperations::clear(fParamBuffers, params);
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        for (uint32_t i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; ++i)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];
            const bool hasPortRDF = (fRdfDescriptor != nullptr && i < fRdfDescriptor->PortCount);

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
                if (hasPortRDF && LADSPA_PORT_HAS_DEFAULT(fRdfDescriptor->Ports[i].Hints))
                    def = fRdfDescriptor->Ports[i].Default;
                else
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

                        pData->param.data[j].type  = PARAMETER_LATENCY;
                        pData->param.data[j].hints = 0;
                    }
                    else if (std::strcmp(fDescriptor->PortNames[i], "_sample-rate") == 0)
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        pData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
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

                // check for scalepoints, require at least 2 to make it useful
                if (hasPortRDF && fRdfDescriptor->Ports[i].ScalePointCount > 1)
                    pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

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

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
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

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
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

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        // check latency
        if (fHints & PLUGIN_CAN_DRYWET)
        {
            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                if (pData->param.data[i].type != PARAMETER_LATENCY)
                    continue;

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

        if (pData->active)
            activate();

        carla_debug("LadspaPlugin::reload() - end");
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
                FloatVectorOperations::clear(outBuffer[i], frames);

            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->latency > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::clear(pData->latencyBuffers[i], pData->latency);
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool isSampleAccurate  = (fOptions & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t time, numEvents = pData->event.portIn->getEventCount();
            uint32_t timeOffset = 0;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                time = event.time;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && isSampleAccurate)
                {
                    if (processSingle(inBuffer, outBuffer, time - timeOffset, timeOffset))
                        timeOffset = time;
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
                        if (event.channel == pData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
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

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                    case kEngineControlEventTypeMidiProgram:
                    case kEngineControlEventTypeAllSoundOff:
                    case kEngineControlEventTypeAllNotesOff:
                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                    // ignored in LADSPA
                    break;
                }
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(inBuffer, outBuffer, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(inBuffer, outBuffer, frames, 0);

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
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
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
            FloatVectorOperations::copy(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            FloatVectorOperations::clear(fAudioOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fDescriptor->run(fHandle, frames);

        if (fHandle2 != nullptr)
            fDescriptor->run(fHandle2, frames);

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
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], frames);
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
        carla_debug("LadspaPlugin::bufferSizeChanged(%i) - start", newBufferSize);

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

        carla_debug("LadspaPlugin::bufferSizeChanged(%i) - end", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("LadspaPlugin::sampleRateChanged(%g) - start", newSampleRate);

        // TODO
        (void)newSampleRate;

        carla_debug("LadspaPlugin::sampleRateChanged(%g) - end", newSampleRate);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() override
    {
        carla_debug("LadspaPlugin::clearBuffers() - start");

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

        carla_debug("LadspaPlugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------

    const void* getExtraStuff() const noexcept override
    {
        return fRdfDescriptor;
    }

    bool init(const char* const filename, const char* const name, const char* const label, const LADSPA_RDF_Descriptor* const rdfDescriptor)
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

        const LADSPA_Descriptor_Function descFn = (LADSPA_Descriptor_Function)pData->libSymbol("ladspa_descriptor");

        if (descFn == nullptr)
        {
            pData->engine->setLastError("Could not find the LASDPA Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((fDescriptor = descFn(i++)) != nullptr)
        {
            if (fDescriptor->Label != nullptr && std::strcmp(fDescriptor->Label, label) == 0)
                break;
        }

        if (fDescriptor == nullptr)
        {
            pData->engine->setLastError("Could not find the requested plugin label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (is_ladspa_rdf_descriptor_valid(rdfDescriptor, fDescriptor))
            fRdfDescriptor = ladspa_rdf_dup(rdfDescriptor);

        if (name != nullptr)
            fName = pData->engine->getUniquePluginName(name);
        else if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr)
            fName = pData->engine->getUniquePluginName(fRdfDescriptor->Title);
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
        // load plugin settings

        {
            const bool isDssiVst = fFilename.contains("dssi-vst", true);

            // set default options
            fOptions = 0x0;

            if (isDssiVst)
                fOptions |= PLUGIN_OPTION_FIXED_BUFFERS;

            if (pData->engine->getOptions().forceStereo)
                fOptions |= PLUGIN_OPTION_FORCE_STEREO;

            // load settings
            pData->idStr  = "LADSPA/";
            pData->idStr += std::strrchr(filename, OS_SEP)+1;
            pData->idStr += "/";
            pData->idStr += CarlaString(getUniqueId());
            pData->idStr += "/";
            pData->idStr += label;
            fOptions = pData->loadSettings(fOptions, getAvailableOptions());

            // ignore settings, we need this anyway
            if (isDssiVst)
                fOptions |= PLUGIN_OPTION_FIXED_BUFFERS;
        }

        return true;
    }

private:
    LADSPA_Handle fHandle;
    LADSPA_Handle fHandle2;
    const LADSPA_Descriptor*     fDescriptor;
    const LADSPA_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fParamBuffers;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LadspaPlugin)
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_LADSPA
# warning Building without LADSPA support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newLADSPA(const Initializer& init, const LADSPA_RDF_Descriptor* const rdfDescriptor)
{
    carla_debug("CarlaPlugin::newLADSPA({%p, \"%s\", \"%s\", \"%s\"}, %p)", init.engine, init.filename, init.name, init.label, rdfDescriptor);

#ifdef WANT_LADSPA
    LadspaPlugin* const plugin(new LadspaPlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label, rdfDescriptor))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo LADSPA plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("LADSPA support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
