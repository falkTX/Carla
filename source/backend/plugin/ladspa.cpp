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
        qDebug("LadspaPlugin::LadspaPlugin()");

        fHandle  = nullptr;
        fHandle2 = nullptr;
        fDescriptor  = nullptr;
        fRdfDescriptor = nullptr;

        fParamBuffers = nullptr;
    }

    ~LadspaPlugin()
    {
        qDebug("LadspaPlugin::~LadspaPlugin()");

        if (fDescriptor)
        {
            if (fDescriptor->deactivate && fData->activeBefore)
            {
                if (fHandle)
                    fDescriptor->deactivate(fHandle);
                if (fHandle2)
                    fDescriptor->deactivate(fHandle2);
            }

            if (fDescriptor->cleanup)
            {
                if (fHandle)
                    fDescriptor->cleanup(fHandle);
                if (fHandle2)
                    fDescriptor->cleanup(fHandle2);
            }
        }

        delete fRdfDescriptor;
    }

    // -------------------------------------------------------------------
    // Information (base)

    virtual PluginType type() const
    {
        return PLUGIN_LADSPA;
    }

    PluginCategory category()
    {
        if (fRdfDescriptor)
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

    long uniqueId()
    {
        CARLA_ASSERT(fDescriptor);

        return fDescriptor ? fDescriptor->UniqueID : 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t parameterScalePointCount(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor && rindex < (int32_t)fRdfDescriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &fRdfDescriptor->Ports[rindex];

            if (port)
                return port->ScalePointCount;
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
        CARLA_ASSERT(parameterId < fData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor && rindex < (int32_t)fRdfDescriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &fRdfDescriptor->Ports[rindex];

            if (port && scalePointId < port->ScalePointCount)
            {
                const LADSPA_RDF_ScalePoint* const scalePoint = &port->ScalePoints[scalePointId];

                if (scalePoint)
                    return scalePoint->Value;
            }
        }

        return 0.0;
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor);

        if (fDescriptor && fDescriptor->Label)
            strncpy(strBuf, fDescriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor);

        if (fRdfDescriptor && fRdfDescriptor->Creator)
            strncpy(strBuf, fRdfDescriptor->Creator, STR_MAX);
        else if (fDescriptor && fDescriptor->Maker)
            strncpy(strBuf, fDescriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor);

        if (fDescriptor && fDescriptor->Copyright)
            strncpy(strBuf, fDescriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor);

        if (fRdfDescriptor && fRdfDescriptor->Title)
            strncpy(strBuf, fRdfDescriptor->Title, STR_MAX);
        else if (fDescriptor && fDescriptor->Name)
            strncpy(strBuf, fDescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor);
        CARLA_ASSERT(parameterId < fData->param.count);

        int32_t rindex = fData->param.data[parameterId].rindex;

        if (fDescriptor && rindex < (int32_t)fDescriptor->PortCount)
            strncpy(strBuf, fDescriptor->PortNames[rindex], STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor && rindex < (int32_t)fRdfDescriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &fRdfDescriptor->Ports[rindex];

            if (LADSPA_PORT_HAS_LABEL(port->Hints) && port->Label)
            {
                strncpy(strBuf, port->Label, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterSymbol(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < fData->param.count);

        int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor && rindex < (int32_t)fRdfDescriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &fRdfDescriptor->Ports[rindex];

            if (LADSPA_PORT_HAS_UNIT(port->Hints))
            {
                switch (port->Unit)
                {
                case LADSPA_UNIT_DB:
                    strncpy(strBuf, "dB", STR_MAX);
                    return;
                case LADSPA_UNIT_COEF:
                    strncpy(strBuf, "(coef)", STR_MAX);
                    return;
                case LADSPA_UNIT_HZ:
                    strncpy(strBuf, "Hz", STR_MAX);
                    return;
                case LADSPA_UNIT_S:
                    strncpy(strBuf, "s", STR_MAX);
                    return;
                case LADSPA_UNIT_MS:
                    strncpy(strBuf, "ms", STR_MAX);
                    return;
                case LADSPA_UNIT_MIN:
                    strncpy(strBuf, "min", STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < fData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        int32_t rindex = fData->param.data[parameterId].rindex;

        if (fRdfDescriptor && rindex < (int32_t)fRdfDescriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &fRdfDescriptor->Ports[rindex];

            if (port && scalePointId < port->ScalePointCount)
            {
                const LADSPA_RDF_ScalePoint* const scalePoint = &port->ScalePoints[scalePointId];

                if (scalePoint && scalePoint->Label)
                {
                    strncpy(strBuf, scalePoint->Label, STR_MAX);
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

        const float fixedValue = fData->param.ranges[parameterId].fixValue(value);
        fParamBuffers[parameterId] = fixedValue;

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("LadspaPlugin::reload() - start");
        CARLA_ASSERT(fDescriptor);

        const ProcessMode processMode(fData->engine->getOptions().processMode);

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (fData->client->isActive())
            fData->client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aIns, aOuts, params, j;
        aIns = aOuts = params = 0;

        const double sampleRate = fData->engine->getSampleRate();
        const unsigned long portCount = fDescriptor->PortCount;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

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

        if (fData->engine->getOptions().forceStereo && (aIns == 1 || aOuts == 1) && ! fHandle2)
        {
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
            aIns = 0;
        }

        if (aOuts > 0)
        {
            fData->audioOut.createNew(aOuts);
            aOuts = 0;
        }

        if (params > 0)
        {
            fData->param.createNew(params);
            fParamBuffers = new float[params];
            params = 0;
        }

        bool needsCtrlIn  = false;
        bool needsCtrlOut = false;

        const int   portNameSize = fData->engine->maxPortNameSize();
        CarlaString portName;

        for (unsigned long i=0; i < portCount; i++)
        {
            const LADSPA_PortDescriptor portType = fDescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portHints = fDescriptor->PortRangeHints[i];
            const bool hasPortRDF = (fRdfDescriptor && i < fRdfDescriptor->PortCount);

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
                    j = aIns++;
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
                    j = aOuts++;
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
                j = params++;
                fData->param.data[j].index  = j;
                fData->param.data[j].rindex = i;
                fData->param.data[j].hints  = 0;
                fData->param.data[j].midiChannel = 0;
                fData->param.data[j].midiCC = -1;

                float min, max, def, step, stepSmall, stepLarge;

                // min value
                if (LADSPA_IS_HINT_BOUNDED_BELOW(portHints.HintDescriptor))
                    min = portHints.LowerBound;
                else
                    min = 0.0f;

                // max value
                if (LADSPA_IS_HINT_BOUNDED_ABOVE(portHints.HintDescriptor))
                    max = portHints.UpperBound;
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
                    def = get_default_ladspa_port_value(portHints.HintDescriptor, min, max);

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LADSPA_IS_HINT_SAMPLE_RATE(portHints.HintDescriptor))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
                    fData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_TOGGLED(portHints.HintDescriptor))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    fData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(portHints.HintDescriptor))
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
                    if (strcmp(fDescriptor->PortNames[i], "latency") == 0 || strcmp(fDescriptor->PortNames[i], "_latency") == 0)
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
                    else if (strcmp(fDescriptor->PortNames[i], "_sample-rate") == 0)
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
                if (LADSPA_IS_HINT_LOGARITHMIC(portHints.HintDescriptor))
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

                if (fHandle2)
                    fDescriptor->connect_port(fHandle2, i, &fParamBuffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");
                fDescriptor->connect_port(fHandle, i, nullptr);
                if (fHandle2) fDescriptor->connect_port(fHandle2, i, nullptr);
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

            portName += "control-in";
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

            portName += "control-out";
            portName.truncate(portNameSize);

            fData->event.portOut = (CarlaEngineEventPort*)fData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        // plugin checks
        fData->hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts%2 == 0)
            fData->hints |= PLUGIN_CAN_BALANCE;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            fData->hints |= PLUGIN_CAN_FORCE_STEREO;

#if 0
        // check latency
        if (fData->hints & PLUGIN_CAN_DRYWET)
        {
            bool hasLatency = false;
            fData->latency = 0;

            for (uint32_t i=0; i < fData->param.count; i++)
            {
                if (fData->param.data[i].type == PARAMETER_LATENCY)
                {
                    // pre-run so plugin can update latency control-port
                    float tmpIn[2][aIns];
                    float tmpOut[2][aOuts];

                    for (j=0; j < fData->audioIn.count; j++)
                    {
                        tmpIn[j][0] = 0.0f;
                        tmpIn[j][1] = 0.0f;

                        //if (j == 0 || ! fHandle2)
                        //    fDescriptor->connect_port(fHandle, fData->audioIn.rindexes[j], tmpIn[j]);
                    }

                    for (j=0; j < fData->audioOut.count; j++)
                    {
                        tmpOut[j][0] = 0.0f;
                        tmpOut[j][1] = 0.0f;

                        //if (j == 0 || ! fHandle2)
                        //    fDescriptor->connect_port(fHandle, fData->audioOut.rindexes[j], tmpOut[j]);
                    }

                    if (fDescriptor->activate)
                        fDescriptor->activate(fHandle);

                    fDescriptor->run(fHandle, 2);

                    if (fDescriptor->deactivate)
                        fDescriptor->deactivate(fHandle);

                    fData->latency = rint(fParamBuffers[i]);
                    hasLatency = true;
                    break;
                }
            }

            if (hasLatency)
            {
                fData->client->setLatency(fData->latency);
                recreateLatencyBuffers();
            }
        }
#endif

        fData->client->activate();

        qDebug("LadspaPlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

#if 0
        //if (param.portCin && m_active && m_activeBefore)
        {
            const EngineEvent* cinEvent = nullptr;
            uint32_t time, nEvents = 0; //param.portCin->getEventCount();

            for (i=0; i < nEvents; i++)
            {
                //cinEvent = param.portCin->getEvent(i);

                if (! cinEvent)
                    continue;

                time = cinEvent->time - framesOffset;

                if (time >= frames)
                    continue;

                // Control change
                switch (cinEvent->type)
                {
                case kEngineControlEventTypeNull:
                    break;

                case CarlaEngineParameterChangeEvent:
                {
                    double value;

                    // Control backend stuff
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(cinEvent->parameter) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = cinEvent->value;
                            setDryWet(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_DRYWET, 0, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_CHANNEL_VOLUME(cinEvent->parameter) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = cinEvent->value*127/100;
                            setVolume(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_VOLUME, 0, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_BALANCE(cinEvent->parameter) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = cinEvent->value/0.5 - 1.0;

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
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midiChannel != cinEvent->channel)
                            continue;
                        if (param.data[k].midiCC != cinEvent->parameter)
                            continue;
                        if (param.data[k].type != PARAMETER_INPUT)
                            continue;

                        if (param.data[k].hints & PARAMETER_IS_AUTOMABLE)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = cinEvent->value < 0.5 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = cinEvent->value * (param.ranges[k].max - param.ranges[k].min) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeEvent(PluginPostEventParameterChange, k, 0, value);
                        }
                    }

                    break;
                }

                case CarlaEngineMidiBankChangeEvent:
                case CarlaEngineMidiProgramChangeEvent:
                    break;

                case CarlaEngineAllSoundOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (fDescriptor->deactivate)
                        {
                            descriptor->deactivate(fHandle);
                            if (fHandle2) descriptor->deactivate(fHandle2);
                        }

                        if (fDescriptor->activate)
                        {
                            descriptor->activate(fHandle);
                            if (fHandle2) descriptor->activate(fHandle2);
                        }

                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 0.0);
                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 1.0);
                    }
                    break;

                case CarlaEngineAllNotesOffEvent:
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;
#endif

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

#if 0
        for (k=0; k < param.count; k++)
        {
            if (param.data[k].type == PARAMETER_LATENCY)
            {
                // TODO: ladspa special params
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;
#endif

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (fData->active)
        {
            if (! fData->activeBefore)
            {
#if 0
                if (fData->latency > 0)
                {
                    for (i=0; i < fData->audioIn.count; i++)
                        memset(fData->latencyBuffers[i], 0, sizeof(float)*fData->latency);
                }
#endif

                if (fDescriptor->activate)
                {
                    fDescriptor->activate(fHandle);
                    if (fHandle2) fDescriptor->activate(fHandle2);
                }
            }

            for (i=0; i < fData->audioIn.count; i++)
            {
                //if (i == 0 || ! fHandle2)
                fDescriptor->connect_port(fHandle, fData->audioIn.ports[i].rindex, inBuffer[i]);
                //else if (i == 1)
                //    fDescriptor->connect_port(fHandle2, fData->audioIn.ports[i].rindex, inBuffer[i]);
            }

            for (i=0; i < fData->audioOut.count; i++)
            {
                //if (i == 0 || ! fHandle2)
                fDescriptor->connect_port(fHandle, fData->audioOut.ports[i].rindex, outBuffer[i]);
                //else if (i == 1)
                //fDescriptor->connect_port(fHandle2, fData->audioOut.ports[i].rindex, outBuffer[i]);
            }

            fDescriptor->run(fHandle, frames);
            if (fHandle2) fDescriptor->run(fHandle2, frames);
        }
        else
        {
            if (fData->activeBefore)
            {
                if (fDescriptor->deactivate)
                {
                    fDescriptor->deactivate(fHandle);
                    if (fHandle2) fDescriptor->deactivate(fHandle2);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (fData->active)
        {
            const bool doDryWet  = (fData->hints & PLUGIN_CAN_DRYWET) > 0 && fData->postProc.dryWet != 1.0;
            const bool doVolume  = (fData->hints & PLUGIN_CAN_VOLUME) > 0 && fData->postProc.volume != 1.0;
            const bool doBalance = (fData->hints & PLUGIN_CAN_BALANCE) > 0 && (fData->postProc.balanceLeft != -1.0 || fData->postProc.balanceRight != 1.0);

            float balRangeL, balRangeR;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < fData->audioOut.count; i++)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; k++)
                    {
                        //if (k < m_latency && m_latency < frames)
                        //    bufValue = (aIn.count == 1) ? fData->latencyBuffers[0][k] : fData->latencyBuffers[i][k];
                        //else
                        //    bufValue = (aIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        bufValue = (fData->audioIn.count == 1) ? inBuffer[0][k] : inBuffer[i][k];

                        outBuffer[i][k] = (outBuffer[i][k] * fData->postProc.dryWet) + (bufValue * (1.0f - fData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    balRangeL = (fData->postProc.balanceLeft  + 1.0f)/2.0f;
                    balRangeR = (fData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; k++)
                    {
                        if (i % 2 == 0)
                        {
                            // left output
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
            // Latency, save values for next callback
            if (fData->latency > 0 && fData->latency < frames)
            {
                for (i=0; i < aIn.count; i++)
                    memcpy(fData->latencyBuffers[i], inBuffer[i] + (frames - fData->latency), sizeof(float)*fData->latency);
            }
#endif
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < fData->audioOut.count; i++)
                carla_zeroFloat(outBuffer[i], frames);

        } // End of Post-processing


        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (fData->event.portOut && fData->active)
        {
            float value;

            for (k=0; k < fData->param.count; k++)
            {
                if (fData->param.data[k].type == PARAMETER_OUTPUT)
                {
                    fData->param.ranges[k].fixValue(fParamBuffers[k]);

                    if (fData->param.data[k].midiCC > 0)
                    {
                        value = fData->param.ranges[k].Value(fParamBuffers[k]);
                        fData->event.portOut->writeControlEvent(framesOffset, fData->param.data[k].midiChannel, kEngineControlEventTypeParameter, fData->param.data[k].midiCC, value);
                    }
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
            fData->engine->setLastError("Could not find the requested plugin Label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (is_ladspa_rdf_descriptor_valid(rdfDescriptor, fDescriptor))
            fRdfDescriptor = ladspa_rdf_dup(rdfDescriptor);

        if (name != nullptr)
            fData->name = fData->engine->getNewUniquePluginName(name);
        else if (fRdfDescriptor && fRdfDescriptor->Title)
            fData->name = fData->engine->getNewUniquePluginName(fRdfDescriptor->Title);
        else
            fData->name = fData->engine->getNewUniquePluginName(fDescriptor->Name);

        fData->filename = filename;

        // ---------------------------------------------------------------
        // register client

        fData->client = fData->engine->addClient(this);

        if (! fData->client->isOk())
        {
            fData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        fHandle = fDescriptor->instantiate(fDescriptor, fData->engine->getSampleRate());

        if (! fHandle)
        {
            fData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        return true;
    }

private:
    LADSPA_Handle fHandle;
    LADSPA_Handle fHandle2;
    const LADSPA_Descriptor* fDescriptor;
    const LADSPA_RDF_Descriptor* fRdfDescriptor;

    float* fParamBuffers;
};

CARLA_BACKEND_END_NAMESPACE

#else // WANT_LADSPA
#  warning Building without LADSPA support
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

    if (init.engine->getOptions().processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if ((plugin->hints() & PLUGIN_CAN_FORCE_STEREO) == 0)
        {
            init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo LADSPA plugins, sorry!");
            delete plugin;
            return nullptr;
        }
    }

    plugin->registerToOscClient();

    return plugin;
#else
    init.engine->setLastError("LADSPA support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
