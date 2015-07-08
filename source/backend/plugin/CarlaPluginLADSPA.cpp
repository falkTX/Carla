/*
 * Carla Plugin, LADSPA implementation
 * Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaLadspaUtils.hpp"
#include "CarlaMathUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------

class CarlaPluginLADSPA : public CarlaPlugin
{
public:
    CarlaPluginLADSPA(CarlaEngine* const engine, const uint id) noexcept
        : CarlaPlugin(engine, id),
          fHandles(),
          fDescriptor(nullptr),
          fRdfDescriptor(nullptr),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fExtraStereoBuffer(),
          fParamBuffers(nullptr),
          fLatencyIndex(-1),
          fForcedStereoIn(false),
          fForcedStereoOut(false),
          fIsDssiVst(false)
    {
        carla_debug("CarlaPluginLADSPA::CarlaPluginLADSPA(%p, %i)", engine, id);

        carla_zeroPointers(fExtraStereoBuffer, 2);
    }

    ~CarlaPluginLADSPA() noexcept override
    {
        carla_debug("CarlaPluginLADSPA::~CarlaPluginLADSPA()");

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
                for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
                {
                    LADSPA_Handle const handle(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                    try {
                        fDescriptor->cleanup(handle);
                    } CARLA_SAFE_EXCEPTION("LADSPA cleanup");
                }
            }

            fHandles.clear();
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

    PluginCategory getCategory() const noexcept override
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

        return CarlaPlugin::getCategory();
    }

    int64_t getUniqueId() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0);

        return static_cast<int64_t>(fDescriptor->UniqueID);
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        if (fLatencyIndex < 0 || fParamBuffers == nullptr)
            return 0;

        const float latency(fParamBuffers[fLatencyIndex]);
        CARLA_SAFE_ASSERT_RETURN(latency >= 0.0f, 0);

        return static_cast<uint32_t>(latency);
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,                      0);

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            return static_cast<uint32_t>(port->ScalePointCount);
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        if (! fIsDssiVst)
        {
            // can't disable fixed buffers if using latency
            if (fLatencyIndex == -1)
                options |= PLUGIN_OPTION_FIXED_BUFFERS;

            // can't disable forced stereo if in rack mode
            if (pData->engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
                pass();
            // if inputs or outputs are just 1, then yes we can force stereo
            else if (pData->audioIn.count == 1 || pData->audioOut.count == 1 || fForcedStereoIn || fForcedStereoOut)
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fParamBuffers != nullptr,         0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        // bad plugins might have set output values out of bounds
        if (pData->param.data[parameterId].type == PARAMETER_OUTPUT)
            return pData->param.ranges[parameterId].getFixedValue(fParamBuffers[parameterId]);

        // not output, should be fine
        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,        0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,                      0.0f);

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            CARLA_SAFE_ASSERT_RETURN(scalePointId < port->ScalePointCount, 0.0f);

            const LADSPA_RDF_ScalePoint* const scalePoint(&port->ScalePoints[scalePointId]);
            return pData->param.ranges[parameterId].getFixedValue(scalePoint->Value);
        }

        return 0.0f;
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

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Creator != nullptr)
        {
            std::strncpy(strBuf, fRdfDescriptor->Creator, STR_MAX);
            return;
        }

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

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr)
        {
            std::strncpy(strBuf, fRdfDescriptor->Title, STR_MAX);
            return;
        }

        std::strncpy(strBuf, fDescriptor->Name, STR_MAX);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,           nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,                      nullStrBuf(strBuf));

        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames[rindex] != nullptr,             nullStrBuf(strBuf));

        if (getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, true))
            return;

        std::strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,                      nullStrBuf(strBuf));

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);

            if (LADSPA_PORT_HAS_LABEL(port->Hints))
            {
                CARLA_SAFE_ASSERT_RETURN(port->Label != nullptr, nullStrBuf(strBuf));

                std::strncpy(strBuf, port->Label, STR_MAX);
                return;
            }
        }

        nullStrBuf(strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,                      nullStrBuf(strBuf));

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

        CARLA_SAFE_ASSERT_RETURN(rindex < static_cast<int32_t>(fDescriptor->PortCount), nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->PortNames[rindex] != nullptr,             nullStrBuf(strBuf));

        if (getSeparatedParameterNameOrUnit(fDescriptor->PortNames[rindex], strBuf, false))
            return;

        nullStrBuf(strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fRdfDescriptor != nullptr,        nullStrBuf(strBuf));
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, nullStrBuf(strBuf));

        const int32_t rindex(pData->param.data[parameterId].rindex);
        CARLA_SAFE_ASSERT_RETURN(rindex >= 0,                      nullStrBuf(strBuf));

        if (rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port* const port(&fRdfDescriptor->Ports[rindex]);
            CARLA_SAFE_ASSERT_RETURN(scalePointId < port->ScalePointCount, nullStrBuf(strBuf));

            const LADSPA_RDF_ScalePoint* const scalePoint(&port->ScalePoints[scalePointId]);
            CARLA_SAFE_ASSERT_RETURN(scalePoint->Label != nullptr,         nullStrBuf(strBuf));

            std::strncpy(strBuf, scalePoint->Label, STR_MAX);
            return;
        }

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

    // -------------------------------------------------------------------
    // Misc

    // nothing

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandles.count() > 0,);
        carla_debug("CarlaPluginLADSPA::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate(static_cast<float>(pData->engine->getSampleRate()));
        const uint32_t portCount(getSafePortCount());

        uint32_t aIns, aOuts, params;
        aIns = aOuts = params = 0;

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

        if (pData->options & PLUGIN_OPTION_FORCE_STEREO)
        {
            if ((aIns == 1 || aOuts == 1) && fHandles.count() == 1 && addInstance())
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
            const bool hasPortRDF = (fRdfDescriptor != nullptr && i < fRdfDescriptor->PortCount);

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
                    pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
                    pData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        pData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, 1);
                        pData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    const uint32_t j = iAudioOut++;
                    pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
                    pData->audioOut.ports[j].rindex = i;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 1);
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

                if (LADSPA_IS_HINT_SAMPLE_RATE(portRangeHints.HintDescriptor))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    pData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (min >= max)
                {
                    carla_stderr2("WARNING - Broken plugin parameter '%s': min >= max", paramName);
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
                        CARLA_SAFE_ASSERT(fLatencyIndex == static_cast<int32_t>(j));
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

                // check for scalepoints, require at least 2 to make it useful
                if (hasPortRDF && fRdfDescriptor->Ports[i].ScalePointCount >= 2)
                    pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                pData->param.ranges[j].min = min;
                pData->param.ranges[j].max = max;
                pData->param.ranges[j].def = def;
                pData->param.ranges[j].step = step;
                pData->param.ranges[j].stepSmall = stepSmall;
                pData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                fParamBuffers[j] = def;

                for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
                {
                    LADSPA_Handle const handle(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                    try {
                        fDescriptor->connect_port(handle, i, &fParamBuffers[j]);
                    } CARLA_SAFE_EXCEPTION("LADSPA connect_port (parameter)");
                }
            }
            else
            {
                // Not Audio or Control
                carla_stderr2("ERROR - Got a broken Port (neither Audio or Control)");

                for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
                {
                    LADSPA_Handle const handle(it.getValue(nullptr));
                    CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                    try {
                        fDescriptor->connect_port(handle, i, nullptr);
                    } CARLA_SAFE_EXCEPTION("LADSPA connect_port (null)");
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

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, 0);
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

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, 0);
        }

        if (forcedStereoIn || forcedStereoOut)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else
            pData->options &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        pData->hints = 0x0;

        if (LADSPA_IS_HARD_RT_CAPABLE(fDescriptor->Properties))
            pData->hints |= PLUGIN_IS_RTSAFE;

#ifndef BUILD_BRIDGE
        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;
#endif

        // extra plugin hints
        pData->extraHints  = 0x0;
        pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        // check initial latency
        findInitialLatencyValue(aIns, aOuts);

        fForcedStereoIn  = forcedStereoIn;
        fForcedStereoOut = forcedStereoOut;

        bufferSizeChanged(pData->engine->getBufferSize());

        if (pData->active)
            activate();

        carla_debug("CarlaPluginLADSPA::reload() - end");
    }

    void findInitialLatencyValue(const uint32_t aIns, const uint32_t aOuts) const
    {
        if (fLatencyIndex < 0 || fHandles.count() == 0)
            return;

        // we need to pre-run the plugin so it can update its latency control-port
        const LADSPA_Handle handle(fHandles.getFirst(nullptr));
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

        float tmpIn [(aIns > 0)  ? aIns  : 1][2];
        float tmpOut[(aOuts > 0) ? aOuts : 1][2];

        for (uint32_t j=0; j < aIns; ++j)
        {
            tmpIn[j][0] = 0.0f;
            tmpIn[j][1] = 0.0f;

            try {
                fDescriptor->connect_port(handle, pData->audioIn.ports[j].rindex, tmpIn[j]);
            } CARLA_SAFE_EXCEPTION("LADSPA connect_port (latency input)");
        }

        for (uint32_t j=0; j < aOuts; ++j)
        {
            tmpOut[j][0] = 0.0f;
            tmpOut[j][1] = 0.0f;

            try {
                fDescriptor->connect_port(handle, pData->audioOut.ports[j].rindex, tmpOut[j]);
            } CARLA_SAFE_EXCEPTION("LADSPA connect_port (latency output)");
        }

        if (fDescriptor->activate != nullptr)
        {
            try {
                fDescriptor->activate(handle);
            } CARLA_SAFE_EXCEPTION("LADSPA latency activate");
        }

        try {
            fDescriptor->run(handle, 2);
        } CARLA_SAFE_EXCEPTION("LADSPA latency run");

        if (fDescriptor->deactivate != nullptr)
        {
            try {
                fDescriptor->deactivate(handle);
            } CARLA_SAFE_EXCEPTION("LADSPA latency deactivate");
        }

        // done, let's get the value
        if (const uint32_t latency = getLatencyInFrames())
        {
            pData->client->setLatency(latency);
            pData->latency.recreateBuffers(std::max(aIns, aOuts), latency);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->activate != nullptr)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDescriptor->activate(handle);
                } CARLA_SAFE_EXCEPTION("LADSPA activate");
            }
        }
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->deactivate != nullptr)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDescriptor->deactivate(handle);
                } CARLA_SAFE_EXCEPTION("LADSPA deactivate");
            }
        }
    }

    void process(const float** const audioIn, float** const audioOut, const float** const, float** const, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            return;
        }

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            const bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t numEvents  = pData->event.portIn->getEventCount();
            uint32_t timeOffset = 0;

            for (uint32_t i=0; i < numEvents; ++i)
            {
                const EngineEvent& event(pData->event.portIn->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (isSampleAccurate && event.time > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, event.time - timeOffset, timeOffset))
                        timeOffset = event.time;
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
                            break;
                        }
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                    case kEngineControlEventTypeMidiProgram:
                    case kEngineControlEventTypeAllSoundOff:
                    case kEngineControlEventTypeAllNotesOff:
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi:
                    break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, frames, 0);

        } // End of Plugin processing (no events)

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

    bool processSingle(const float** const audioIn, float** const audioOut, const uint32_t frames,
                       const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr, false);
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
                    audioOut[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        const int iframes(static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------
        // Handle needsReset

        if (pData->needsReset)
        {
            if (pData->latency.frames > 0)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::clear(pData->latency.buffers[i], static_cast<int>(pData->latency.frames));
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio buffers

        const bool customMonoOut   = pData->audioOut.count == 2 && fForcedStereoOut && ! fForcedStereoIn;
        const bool customStereoOut = pData->audioOut.count == 2 && fForcedStereoIn  && ! fForcedStereoOut;

        if (! customMonoOut)
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(fAudioOutBuffers[i], iframes);
        }

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            FloatVectorOperations::copy(fAudioInBuffers[i], audioIn[i]+timeOffset, iframes);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        uint instn = 0;
        for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next(), ++instn)
        {
            LADSPA_Handle const handle(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

            // ----------------------------------------------------------------------------------------------------
            // Mixdown for forced stereo

            if (customMonoOut)
                FloatVectorOperations::clear(fAudioOutBuffers[instn], iframes);

            // ----------------------------------------------------------------------------------------------------
            // Run it

            try {
                fDescriptor->run(handle, frames);
            } CARLA_SAFE_EXCEPTION("LADSPA run");

            // ----------------------------------------------------------------------------------------------------
            // Mixdown for forced stereo

            if (customMonoOut)
                FloatVectorOperations::multiply(fAudioOutBuffers[instn], 0.5f, iframes);
            else if (customStereoOut)
                FloatVectorOperations::copy(fExtraStereoBuffer[instn], fAudioOutBuffers[instn], iframes);
        }

        if (customStereoOut)
        {
            FloatVectorOperations::copy(fAudioOutBuffers[0], fExtraStereoBuffer[0], iframes);
            FloatVectorOperations::copy(fAudioOutBuffers[1], fExtraStereoBuffer[1], iframes);
        }

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    const uint32_t c = isMono ? 0 : i;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (k < pData->latency.frames)
                            bufValue = pData->latency.buffers[c][k];
                        else if (pData->latency.frames < frames)
                            bufValue = fAudioInBuffers[c][k-pData->latency.frames];
                        else
                            bufValue = fAudioInBuffers[c][k];

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
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], iframes);
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
                        audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k] * pData->postProc.volume;
                }
            }

        } // End of Post-processing

#else // BUILD_BRIDGE
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------
        // Save latency values for next callback

        if (const uint32_t latframes = pData->latency.frames)
        {
            CARLA_SAFE_ASSERT(timeOffset == 0);

            if (latframes <= frames)
            {
                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                    FloatVectorOperations::copy(pData->latency.buffers[i], audioIn[i]+(frames-latframes), static_cast<int>(latframes));
            }
            else
            {
                const uint32_t diff = pData->latency.frames-frames;

                for (uint32_t i=0, k; i<pData->audioIn.count; ++i)
                {
                    // push back buffer by 'frames'
                    for (k=0; k < diff; ++k)
                        pData->latency.buffers[i][k] = pData->latency.buffers[i][k+frames];

                    // put current input at the end
                    for (uint32_t j=0; k < latframes; ++j, ++k)
                        pData->latency.buffers[i][k] = audioIn[i][j];
                }
            }
        }

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginLADSPA::bufferSizeChanged(%i) - start", newBufferSize);

        const int iBufferSize(static_cast<int>(newBufferSize));

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];

            fAudioInBuffers[i] = new float[newBufferSize];
            FloatVectorOperations::clear(fAudioInBuffers[i], iBufferSize);
        }

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];

            fAudioOutBuffers[i] = new float[newBufferSize];
            FloatVectorOperations::clear(fAudioOutBuffers[i], iBufferSize);
        }

        if (fExtraStereoBuffer[0] != nullptr)
        {
            delete[] fExtraStereoBuffer[0];
            fExtraStereoBuffer[0] = nullptr;
        }

        if (fExtraStereoBuffer[1] != nullptr)
        {
            delete[] fExtraStereoBuffer[1];
            fExtraStereoBuffer[1] = nullptr;
        }

        if (fForcedStereoIn && pData->audioOut.count == 2)
        {
            fExtraStereoBuffer[0] = new float[newBufferSize];
            fExtraStereoBuffer[1] = new float[newBufferSize];
            FloatVectorOperations::clear(fExtraStereoBuffer[0], iBufferSize);
            FloatVectorOperations::clear(fExtraStereoBuffer[1], iBufferSize);
        }

        reconnectAudioPorts();

        carla_debug("CarlaPluginLADSPA::bufferSizeChanged(%i) - end", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_stdout("CarlaPluginLADSPA::sampleRateChanged(%g) - start", newSampleRate);

        if (pData->active)
            deactivate();

        const std::size_t instanceCount(fHandles.count());

        if (fDescriptor->cleanup == nullptr)
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                try {
                    fDescriptor->cleanup(handle);
                } CARLA_SAFE_EXCEPTION("LADSPA cleanup");
            }
        }

        fHandles.clear();

        for (std::size_t i=0; i<instanceCount; ++i)
            addInstance();

        reconnectAudioPorts();

        if (pData->active)
            activate();

        carla_stdout("CarlaPluginLADSPA::sampleRateChanged(%g) - end", newSampleRate);
    }

    void reconnectAudioPorts() const noexcept
    {
        if (fForcedStereoIn)
        {
            if (LADSPA_Handle const handle = fHandles.getAt(0, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioIn.ports[0].rindex, fAudioInBuffers[0]);
                } CARLA_SAFE_EXCEPTION("LADSPA connect_port (forced stereo input)");
            }

            if (LADSPA_Handle const handle = fHandles.getAt(1, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioIn.ports[1].rindex, fAudioInBuffers[1]);
                } CARLA_SAFE_EXCEPTION("LADSPA connect_port (forced stereo input)");
            }
        }
        else
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                for (uint32_t i=0; i < pData->audioIn.count; ++i)
                {
                    try {
                        fDescriptor->connect_port(handle, pData->audioIn.ports[i].rindex, fAudioInBuffers[i]);
                    } CARLA_SAFE_EXCEPTION("LADSPA connect_port (audio input)");
                }
            }
        }

        if (fForcedStereoOut)
        {
            if (LADSPA_Handle const handle = fHandles.getAt(0, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioOut.ports[0].rindex, fAudioOutBuffers[0]);
                } CARLA_SAFE_EXCEPTION("LADSPA connect_port (forced stereo output)");
            }

            if (LADSPA_Handle const handle = fHandles.getAt(1, nullptr))
            {
                try {
                    fDescriptor->connect_port(handle, pData->audioOut.ports[1].rindex, fAudioOutBuffers[1]);
                } CARLA_SAFE_EXCEPTION("LADSPA connect_port (forced stereo output)");
            }
        }
        else
        {
            for (LinkedList<LADSPA_Handle>::Itenerator it = fHandles.begin2(); it.valid(); it.next())
            {
                LADSPA_Handle const handle(it.getValue(nullptr));
                CARLA_SAFE_ASSERT_CONTINUE(handle != nullptr);

                for (uint32_t i=0; i < pData->audioOut.count; ++i)
                {
                    try {
                        fDescriptor->connect_port(handle, pData->audioOut.ports[i].rindex, fAudioOutBuffers[i]);
                    } CARLA_SAFE_EXCEPTION("LADSPA connect_port (audio output)");
                }
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginLADSPA::clearBuffers() - start");

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

        if (fExtraStereoBuffer[0] != nullptr)
        {
            delete[] fExtraStereoBuffer[0];
            fExtraStereoBuffer[0] = nullptr;
        }

        if (fExtraStereoBuffer[1] != nullptr)
        {
            delete[] fExtraStereoBuffer[1];
            fExtraStereoBuffer[1] = nullptr;
        }

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginLADSPA::clearBuffers() - end");
    }

    // -------------------------------------------------------------------

    const void* getNativeDescriptor() const noexcept override
    {
        return fDescriptor;
    }

    const void* getExtraStuff() const noexcept override
    {
        return fRdfDescriptor;
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const uint options,
              const LADSPA_RDF_Descriptor* const rdfDescriptor)
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

        const LADSPA_Descriptor_Function descFn = pData->libSymbol<LADSPA_Descriptor_Function>("ladspa_descriptor");

        if (descFn == nullptr)
        {
            pData->engine->setLastError("Could not find the LASDPA Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        for (ulong d=0;; ++d)
        {
            try {
                fDescriptor = descFn(d);
            }
            catch(...) {
                carla_stderr2("Caught exception when trying to get LADSPA descriptor");
                fDescriptor = nullptr;
                break;
            }

            if (fDescriptor == nullptr)
                break;

            if (fDescriptor->Label == nullptr || fDescriptor->Label[0] == '\0')
            {
                carla_stderr2("WARNING - Got an invalid label, will not use this plugin");
                fDescriptor = nullptr;
                break;
            }
            if (fDescriptor->run == nullptr)
            {
                carla_stderr2("WARNING - Plugin has no run, cannot use it");
                fDescriptor = nullptr;
                break;
            }

            if (std::strcmp(fDescriptor->Label, label) == 0)
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

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr && fRdfDescriptor->Title[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(fRdfDescriptor->Title);
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

        if (! addInstance())
            return false;

        // ---------------------------------------------------------------
        // find latency port index

        for (uint32_t i=0, iCtrl=0, count=getSafePortCount(); i<count; ++i)
        {
            const int portType(fDescriptor->PortDescriptors[i]);

            if (! LADSPA_IS_PORT_CONTROL(portType))
                continue;

            const uint32_t index(iCtrl++);

            if (! LADSPA_IS_PORT_OUTPUT(portType))
                continue;

            const char* const portName(fDescriptor->PortNames[i]);
            CARLA_SAFE_ASSERT_BREAK(portName != nullptr);

            if (std::strcmp(portName, "latency")  == 0 ||
                std::strcmp(portName, "_latency") == 0)
            {
                fLatencyIndex = static_cast<int32_t>(index);
                break;
            }
        }

        // ---------------------------------------------------------------
        // check if this is dssi-vst

        fIsDssiVst = CarlaString(filename).contains("dssi-vst", true);

        // ---------------------------------------------------------------
        // set default options

        pData->options = 0x0;

        /**/ if (fLatencyIndex >= 0 || fIsDssiVst)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
         else if (options & PLUGIN_OPTION_FIXED_BUFFERS)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        /**/ if (pData->engine->getOptions().forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
         else if (options & PLUGIN_OPTION_FORCE_STEREO)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        return true;
    }

    // -------------------------------------------------------------------

private:
    LinkedList<LADSPA_Handle>    fHandles;
    const LADSPA_Descriptor*     fDescriptor;
    const LADSPA_RDF_Descriptor* fRdfDescriptor;

    float** fAudioInBuffers;
    float** fAudioOutBuffers;
    float*  fExtraStereoBuffer[2]; // used only if forcedStereoIn and audioOut == 2
    float*  fParamBuffers;

    int32_t fLatencyIndex; // -1 if invalid
    bool    fForcedStereoIn;
    bool    fForcedStereoOut;
    bool    fIsDssiVst;

    // -------------------------------------------------------------------

    bool addInstance()
    {
        LADSPA_Handle handle;

        try {
            handle = fDescriptor->instantiate(fDescriptor, static_cast<ulong>(pData->engine->getSampleRate()));
        } CARLA_SAFE_EXCEPTION_RETURN_ERR("LADSPA instantiate", "Plugin failed to initialize");

        for (uint32_t i=0, count=pData->param.count; i<count; ++i)
        {
            const int32_t rindex(pData->param.data[i].rindex);
            CARLA_SAFE_ASSERT_CONTINUE(rindex >= 0);

            try {
                fDescriptor->connect_port(handle, static_cast<ulong>(rindex), &fParamBuffers[i]);
            } CARLA_SAFE_EXCEPTION("LADSPA connect_port");
        }

        if (fHandles.append(handle))
            return true;

        try {
            fDescriptor->cleanup(handle);
        } CARLA_SAFE_EXCEPTION("LADSPA cleanup");

        pData->engine->setLastError("Out of memory");
        return false;
    }

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

    static bool _getSeparatedParameterNameOrUnitImpl(const char* const paramName, char* const strBuf,
                                                     const bool wantName, const bool useBracket) noexcept
    {
        const char* const sepBracketStart(std::strstr(paramName, useBracket ? " [" : " ("));

        if (sepBracketStart == nullptr)
            return false;

        const char* const sepBracketEnd(std::strstr(sepBracketStart, useBracket ? "]" : ")"));

        if (sepBracketEnd == nullptr)
            return false;

        const std::size_t unitSize(static_cast<std::size_t>(sepBracketEnd-sepBracketStart-2));

        if (unitSize > 7) // very unlikely to have such big unit
            return false;

        const std::size_t sepIndex(std::strlen(paramName)-unitSize-3);

        // just in case
        if (sepIndex+2 >= STR_MAX)
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

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginLADSPA)
};

// -------------------------------------------------------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newLADSPA(const Initializer& init, const LADSPA_RDF_Descriptor* const rdfDescriptor)
{
    carla_debug("CarlaPlugin::newLADSPA({%p, \"%s\", \"%s\", \"%s\", " P_INT64 ", %x}, %p)",
                init.engine, init.filename, init.name, init.label, init.uniqueId, init.options, rdfDescriptor);

    CarlaPluginLADSPA* const plugin(new CarlaPluginLADSPA(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label, init.options, rdfDescriptor))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
