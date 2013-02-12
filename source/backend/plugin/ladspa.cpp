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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "carla_plugin_internal.hpp"

#ifdef WANT_LADSPA

#include "carla_ladspa_utils.hpp"

CARLA_BACKEND_START_NAMESPACE

class LadspaPlugin : public CarlaPlugin
{
public:
    LadspaPlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id)
    {
        qDebug("LadspaPlugin::LadspaPlugin(%p, %i)", engine, id);

        fHandle  = nullptr;
        fHandle2 = nullptr;
        fDescriptor = nullptr;
        fRdfDescriptor = nullptr;

        fParamBuffers = nullptr;
    }

    ~LadspaPlugin()
    {
        qDebug("LadspaPlugin::~LadspaPlugin()");

        if (fDescriptor != nullptr)
        {
            if (fDescriptor->deactivate != nullptr && fData->activeBefore)
            {
                if (fHandle != nullptr)
                    fDescriptor->deactivate(fHandle);
                if (fHandle2 != nullptr)
                    fDescriptor->deactivate(fHandle2);
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
        }

        if (fRdfDescriptor != nullptr)
        {
            delete fRdfDescriptor;
            fRdfDescriptor = nullptr;
        }

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    virtual PluginType type() const
    {
        return PLUGIN_LADSPA;
    }

    PluginCategory category() const
    {
        if (fRdfDescriptor != nullptr)
        {
            const LADSPA_Properties category = fRdfDescriptor->Type;

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

        return getPluginCategoryFromName(fData->name);
    }

    long uniqueId() const
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        return (fDescriptor != nullptr) ? fDescriptor->UniqueID : 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t parameterScalePointCount(const uint32_t parameterId) const
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        const int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port& port = fRdfDescriptor->Ports[rindex];

            return port.ScalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    float getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        return fParamBuffers[parameterId];
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < fData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        const int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port& port = fRdfDescriptor->Ports[rindex];

            if (scalePointId < port.ScalePointCount)
            {
                const LADSPA_RDF_ScalePoint& scalePoint = port.ScalePoints[scalePointId];

                return scalePoint.Value;
            }
        }

        return 0.0f;
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->Label != nullptr)
            std::strncpy(strBuf, fDescriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Creator != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Creator, STR_MAX);
        else if (fDescriptor != nullptr && fDescriptor->Maker != nullptr)
            std::strncpy(strBuf, fDescriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->Copyright != nullptr)
            std::strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr)
            std::strncpy(strBuf, fRdfDescriptor->Title, STR_MAX);
        else if (fDescriptor != nullptr && fDescriptor->Name != nullptr)
            std::strncpy(strBuf, fDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(parameterId < fData->param.count);

        const int32_t rindex = fData->param.data[parameterId].rindex;

        if (fDescriptor != nullptr && rindex < static_cast<int32_t>(fDescriptor->PortCount))
            std::strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        const int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port& port = fRdfDescriptor->Ports[rindex];

            if (LADSPA_PORT_HAS_LABEL(port.Hints) && port.Label != nullptr)
            {
                std::strncpy(strBuf, port.Label, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        const int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port& port = fRdfDescriptor->Ports[rindex];

            if (LADSPA_PORT_HAS_UNIT(port.Hints))
            {
                switch (port.Unit)
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

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
    {
        CARLA_ASSERT(fRdfDescriptor != nullptr);
        CARLA_ASSERT(parameterId < fData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        const int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor != nullptr && rindex < static_cast<int32_t>(fRdfDescriptor->PortCount))
        {
            const LADSPA_RDF_Port& port = fRdfDescriptor->Ports[rindex];

            if (scalePointId < port.ScalePointCount)
            {
                const LADSPA_RDF_ScalePoint& scalePoint = port.ScalePoints[scalePointId];

                if (scalePoint.Label != nullptr)
                {
                    std::strncpy(strBuf, scalePoint.Label, STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        const float fixedValue = fData->param.fixValue(parameterId, value);
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("LadspaPlugin::reload() - start");
        CARLA_ASSERT(fData->engine != nullptr);
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        const ProcessMode processMode(fData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (fData->client->isActive())
            fData->client->deactivate();

        deleteBuffers();

        const double sampleRate = fData->engine->getSampleRate();
        const unsigned long portCount = fDescriptor->PortCount;

        uint32_t aIns, aOuts, params, j;
        aIns = aOuts = params = 0;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        for (unsigned long i=0; i < portCount; i++)
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

        if ((fData->options & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1))
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(fDescriptor, sampleRate);

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

        if (aIns > 0)
        {
            fData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            fData->audioOut.createNew(aOuts);
        }

        if (params > 0)
        {
            fData->param.createNew(params);
            fParamBuffers = new float[params];
        }

        const int   portNameSize = fData->engine->maxPortNameSize();
        CarlaString portName;

        for (unsigned long i=0, iAudioIn=0, iAudioOut=0, iCtrl=0; i < portCount; i++)
        {
            const LADSPA_PortDescriptor portType      = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portRangeHints = fDescriptor->PortRangeHints[i];
            const bool hasPortRDF = (fRdfDescriptor != nullptr && i < fRdfDescriptor->PortCount);

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fData->name;
                    portName += ":";
                }

                portName += fDescriptor->PortNames[i];
                portName.truncate(portNameSize);

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    j = iAudioIn++;
                    fData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)fData->client->addPort(kEnginePortTypeAudio, portName, true);
                    fData->audioIn.ports[j].rindex = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        fData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)fData->client->addPort(kEnginePortTypeAudio, portName, true);
                        fData->audioIn.ports[1].rindex = i;
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    j = iAudioOut++;
                    fData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)fData->client->addPort(kEnginePortTypeAudio, portName, false);
                    fData->audioOut.ports[j].rindex = i;
                    needsCtrlIn = true;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        fData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)fData->client->addPort(kEnginePortTypeAudio, portName, false);
                        fData->audioOut.ports[1].rindex = i;
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
            {
                j = iCtrl++;
                fData->param.data[j].index  = j;
                fData->param.data[j].rindex = i;
                fData->param.data[j].hints  = 0x0;
                fData->param.data[j].midiChannel = 0;
                fData->param.data[j].midiCC = -1;

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
                    qWarning("Broken plugin parameter: max - min == 0");
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
                    fData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_TOGGLED(portRangeHints.HintDescriptor))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    fData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(portRangeHints.HintDescriptor))
                {
                    step = 1.0f;
                    stepSmall = 1.0f;
                    stepLarge = 10.0f;
                    fData->param.data[j].hints |= PARAMETER_IS_INTEGER;
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
                    fData->param.data[j].type   = PARAMETER_INPUT;
                    fData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                    fData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
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

                        fData->param.data[j].type  = PARAMETER_LATENCY;
                        fData->param.data[j].hints = 0;
                    }
                    else if (std::strcmp(fDescriptor->PortNames[i], "_sample-rate") == 0)
                    {
                        def = sampleRate;
                        step = 1.0f;
                        stepSmall = 1.0f;
                        stepLarge = 1.0f;

                        fData->param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        fData->param.data[j].hints = 0;
                    }
                    else
                    {
                        fData->param.data[j].type   = PARAMETER_OUTPUT;
                        fData->param.data[j].hints |= PARAMETER_IS_ENABLED;
                        fData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    fData->param.data[j].type = PARAMETER_UNKNOWN;
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LADSPA_IS_HINT_LOGARITHMIC(portRangeHints.HintDescriptor))
                    fData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                // check for scalepoints, require at least 2 to make it useful
                if (hasPortRDF && fRdfDescriptor->Ports[i].ScalePointCount > 1)
                    fData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                fData->param.ranges[j].min = min;
                fData->param.ranges[j].max = max;
                fData->param.ranges[j].def = def;
                fData->param.ranges[j].step = step;
                fData->param.ranges[j].stepSmall = stepSmall;
                fData->param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                fParamBuffers[j] = def;

                fDescriptor->connect_port(fHandle, i, &fParamBuffers[j]);

                if (fHandle2 != nullptr)
                    fDescriptor->connect_port(fHandle2, i, &fParamBuffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");

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
                portName  = fData->name;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            fData->event.portIn = (CarlaEngineEventPort*)fData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fData->name;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            fData->event.portOut = (CarlaEngineEventPort*)fData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // plugin checks
        fData->hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fData->hints |= PLUGIN_CAN_BALANCE;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            fData->hints |= PLUGIN_CAN_FORCE_STEREO;

        // check latency
        if (fData->hints & PLUGIN_CAN_DRYWET)
        {
            for (uint32_t i=0; i < fData->param.count; i++)
            {
                if (fData->param.data[i].type != PARAMETER_LATENCY)
                    continue;

                // we need to pre-run the plugin so it can update its latency control-port

                float tmpIn[aIns][2];
                float tmpOut[aOuts][2];

                for (j=0; j < aIns; j++)
                {
                    tmpIn[j][0] = 0.0f;
                    tmpIn[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, fData->audioIn.ports[j].rindex, tmpIn[j]);
                }

                for (j=0; j < aOuts; j++)
                {
                    tmpOut[j][0] = 0.0f;
                    tmpOut[j][1] = 0.0f;

                    fDescriptor->connect_port(fHandle, fData->audioOut.ports[j].rindex, tmpOut[j]);
                }

                if (fDescriptor->activate != nullptr)
                    fDescriptor->activate(fHandle);

                fDescriptor->run(fHandle, 2);

                if (fDescriptor->deactivate != nullptr)
                    fDescriptor->deactivate(fHandle);

                const uint32_t latency = std::rint(fParamBuffers[i]);

                if (fData->latency != latency)
                {
                    fData->latency = latency;
                    fData->client->setLatency(latency);
                    recreateLatencyBuffers();
                }

                break;
            }
        }

        fData->client->activate();

        qDebug("LadspaPlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! fData->active)
        {
            // disable any output sound
            for (i=0; i < fData->audioOut.count; i++)
                carla_zeroFloat(outBuffer[i], frames);

            if (fData->activeBefore)
            {
                if (fDescriptor->deactivate != nullptr)
                {
                    fDescriptor->deactivate(fHandle);

                    if (fHandle2 != nullptr)
                        fDescriptor->deactivate(fHandle2);
                }
            }

            fData->activeBefore = fData->active;
            return;
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (fData->event.portIn != nullptr && fData->activeBefore)
        {
            uint32_t time, nEvents = fData->event.portIn->getEventCount();

            for (i=0; i < nEvents; i++)
            {
                const EngineEvent& event = fData->event.portIn->getEvent(i);

                time = event.time - framesOffset;

                if (time >= frames)
                    continue;

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
                        // Control backend stuff
                        if (event.channel == fData->ctrlInChannel)
                        {
                            double value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fData->hints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fData->hints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127/100;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fData->hints & PLUGIN_CAN_BALANCE) > 0)
                            {
                                double left, right;
                                value = ctrlEvent.value/0.5 - 1.0;

                                if (value < 0.0)
                                {
                                    left  = -1.0;
                                    right = (value*2)+1.0;
                                }
                                else if (value > 0.0)
                                {
                                    left  = (value*2)-1.0;
                                    right = 1.0;
                                }
                                else
                                {
                                    left  = -1.0;
                                    right = 1.0;
                                }

                                setBalanceLeft(left, false, false);
                                setBalanceRight(right, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                                continue;
                            }
                        }

                        // Control plugin parameters
                        for (k=0; k < fData->param.count; k++)
                        {
                            if (fData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (fData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (fData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((fData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            double value;

                            if (fData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5) ? fData->param.ranges[k].min : fData->param.ranges[k].max;
                            }
                            else
                            {
                                // FIXME - ranges call for this
                                value = ctrlEvent.value * (fData->param.ranges[k].max - fData->param.ranges[k].min) + fData->param.ranges[k].min;

                                if (fData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, k, 0, value);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                    case kEngineControlEventTypeMidiProgram:
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (event.channel == fData->ctrlInChannel)
                        {
                            if (fDescriptor->deactivate != nullptr)
                            {
                                fDescriptor->deactivate(fHandle);

                                if (fHandle2 != nullptr)
                                    fDescriptor->deactivate(fHandle2);
                            }

                            if (fDescriptor->activate != nullptr)
                            {
                                fDescriptor->activate(fHandle);

                                if (fHandle2 != nullptr)
                                    fDescriptor->activate(fHandle2);
                            }

                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 0.0);
                            postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_ACTIVE, 0, 1.0);
                        }
                        break;

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

            fData->postRtEvents.trySplice();

        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        {
            if (! fData->activeBefore)
            {
                if (fData->latency > 0)
                {
                    for (i=0; i < fData->audioIn.count; i++)
                        carla_zeroFloat(fData->latencyBuffers[i], fData->latency);
                }

                if (fDescriptor->activate != nullptr)
                {
                    fDescriptor->activate(fHandle);

                    if (fHandle2 != nullptr)
                        fDescriptor->activate(fHandle2);
                }
            }

            if (fHandle2 == nullptr)
            {
                for (i=0; i < fData->audioIn.count; i++)
                    fDescriptor->connect_port(fHandle, fData->audioIn.ports[i].rindex, inBuffer[i]);

                for (i=0; i < fData->audioOut.count; i++)
                    fDescriptor->connect_port(fHandle, fData->audioOut.ports[i].rindex, outBuffer[i]);
            }
            else
            {
                if (fData->audioIn.count > 0)
                {
                    CARLA_ASSERT(fData->audioIn.count == 2);

                    fDescriptor->connect_port(fHandle,  fData->audioIn.ports[0].rindex, inBuffer[0]);
                    fDescriptor->connect_port(fHandle2, fData->audioIn.ports[1].rindex, inBuffer[1]);
                }

                if (fData->audioOut.count > 0)
                {
                    CARLA_ASSERT(fData->audioOut.count == 2);

                    fDescriptor->connect_port(fHandle,  fData->audioOut.ports[0].rindex, outBuffer[0]);
                    fDescriptor->connect_port(fHandle2, fData->audioOut.ports[1].rindex, outBuffer[1]);
                }
            }

            fDescriptor->run(fHandle, frames);

            if (fHandle2 != nullptr)
                fDescriptor->run(fHandle2, frames);

        } // End of Plugin processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (fData->hints & PLUGIN_CAN_DRYWET) > 0 && fData->postProc.dryWet != 1.0f;
            const bool doVolume  = (fData->hints & PLUGIN_CAN_VOLUME) > 0 && fData->postProc.volume != 1.0f;
            const bool doBalance = (fData->hints & PLUGIN_CAN_BALANCE) > 0 && (fData->postProc.balanceLeft != -1.0f || fData->postProc.balanceRight != 1.0f);

            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < fData->audioOut.count; i++)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; k++)
                    {
                        // TODO
                        //if (k < fData->latency && fData->latency < frames)
                        //    bufValue = (fData->audioIn.count == 1) ? fData->latencyBuffers[0][k] : fData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (fData->audioIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = inBuffer[ (fData->audioIn.count == 1) ? 0 : i ][k];

                        outBuffer[i][k] = (outBuffer[i][k] * fData->postProc.dryWet) + (bufValue * (1.0f - fData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        std::memcpy(oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    float balRangeL = (fData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (fData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; k++)
                    {
                        if (i % 2 == 0)
                        {
                            // left
                            outBuffer[i][k]  = oldBufLeft[k]     * (1.0f - balRangeL);
                            outBuffer[i][k] += outBuffer[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k] * balRangeR;
                            outBuffer[i][k] += oldBufLeft[k]   * balRangeL;
                        }
                    }
                }

                // Volume
                if (doVolume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= fData->postProc.volume;
                }
            }

#if 0
            // Latency, save values for next callback, TODO
            if (fData->latency > 0 && fData->latency < frames)
            {
                for (i=0; i < fData->audioIn.count; i++)
                    std::memcpy(fData->latencyBuffers[i], inBuffer[i] + (frames - fData->latency), sizeof(float)*fData->latency);
            }
#endif
        } // End of Post-processing


        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (fData->event.portOut != nullptr)
        {
            float value;

            for (k=0; k < fData->param.count; k++)
            {
                if (fData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                fData->param.ranges[k].fixValue(fParamBuffers[k]);

                if (fData->param.data[k].midiCC > 0)
                {
                    value = fData->param.ranges[k].normalizeValue(fParamBuffers[k]);
                    fData->event.portOut->writeControlEvent(framesOffset, fData->param.data[k].midiChannel, kEngineControlEventTypeParameter, fData->param.data[k].midiCC, value);
                }
            }

        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------

        fData->activeBefore = fData->active;
    }

    // -------------------------------------------------------------------
    // Cleanup

    void deleteBuffers()
    {
        qDebug("LadspaPlugin::deleteBuffers() - start");

        if (fParamBuffers != nullptr)
        {
            delete[] fParamBuffers;
            fParamBuffers = nullptr;
        }

        CarlaPlugin::deleteBuffers();

        qDebug("LadspaPlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const LADSPA_RDF_Descriptor* const rdfDescriptor)
    {
        CARLA_ASSERT(fData->engine != nullptr);
        CARLA_ASSERT(fData->client == nullptr);
        CARLA_ASSERT(filename);
        CARLA_ASSERT(label);

        // ---------------------------------------------------------------
        // open DLL

        if (! libOpen(filename))
        {
            fData->engine->setLastError(libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        const LADSPA_Descriptor_Function descFn = (LADSPA_Descriptor_Function)libSymbol("ladspa_descriptor");

        if (descFn == nullptr)
        {
            fData->engine->setLastError("Could not find the LASDPA Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((fDescriptor = descFn(i++)) != nullptr)
        {
            if (fDescriptor->Label == nullptr)
                continue;
            if (std::strcmp(fDescriptor->Label, label) == 0)
                break;
        }

        if (fDescriptor == nullptr)
        {
            fData->engine->setLastError("Could not find the requested plugin label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (is_ladspa_rdf_descriptor_valid(rdfDescriptor, fDescriptor))
            fRdfDescriptor = ladspa_rdf_dup(rdfDescriptor);

        if (name != nullptr)
            fData->name = fData->engine->getNewUniquePluginName(name);
        else if (fRdfDescriptor != nullptr && fRdfDescriptor->Title != nullptr)
            fData->name = fData->engine->getNewUniquePluginName(fRdfDescriptor->Title);
        else if (fDescriptor->Name != nullptr)
            fData->name = fData->engine->getNewUniquePluginName(fDescriptor->Name);
        else
            fData->name = fData->engine->getNewUniquePluginName(fDescriptor->Label);

        fData->filename = filename;

        // ---------------------------------------------------------------
        // register client

        fData->client = fData->engine->addClient(this);

        if (fData->client == nullptr || ! fData->client->isOk())
        {
            fData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, fData->engine->getSampleRate());

        if (fHandle == nullptr)
        {
            fData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        return true;
    }

private:
    LADSPA_Handle fHandle;
    LADSPA_Handle fHandle2;
    const LADSPA_Descriptor*     fDescriptor;
    const LADSPA_RDF_Descriptor* fRdfDescriptor;

    float* fParamBuffers;
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_LADSPA
# warning Building without LADSPA support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newLADSPA(const Initializer& init, const LADSPA_RDF_Descriptor* const rdfDescriptor)
{
    qDebug("CarlaPlugin::newLADSPA({%p, \"%s\", \"%s\", \"%s\"}, %p)", init.engine, init.filename, init.name, init.label, rdfDescriptor);

#ifdef WANT_LADSPA
    LadspaPlugin* const plugin = new LadspaPlugin(init.engine, init.id);

    if (! plugin->init(init.filename, init.name, init.label, rdfDescriptor))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && (plugin->hints() & PLUGIN_CAN_FORCE_STEREO) == 0)
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo LADSPA plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    plugin->registerToOscClient();

    return plugin;
#else
    init.engine->setLastError("LADSPA support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
