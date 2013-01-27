/*
 * Carla LADSPA Plugin
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_plugin.hpp"

#ifdef WANT_LADSPA

#include "carla_ladspa_utils.hpp"

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendLadspaPlugin Carla Backend LADSPA Plugin
 *
 * The Carla Backend LADSPA Plugin.\n
 * http://www.ladspa.org/
 * @{
 */

class LadspaPlugin : public CarlaPlugin
{
public:
    LadspaPlugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        qDebug("LadspaPlugin::LadspaPlugin()");

        m_type = PLUGIN_LADSPA;

        handle = h2 = nullptr;
        descriptor  = nullptr;
        rdf_descriptor = nullptr;

        paramBuffers = nullptr;
    }

    ~LadspaPlugin()
    {
        qDebug("LadspaPlugin::~LadspaPlugin()");

        if (descriptor)
        {
            if (descriptor->deactivate && m_activeBefore)
            {
                if (handle)
                    descriptor->deactivate(handle);
                if (h2)
                    descriptor->deactivate(h2);
            }

            if (descriptor->cleanup)
            {
                if (handle)
                    descriptor->cleanup(handle);
                if (h2)
                    descriptor->cleanup(h2);
            }
        }

        if (rdf_descriptor)
            delete rdf_descriptor;
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        if (rdf_descriptor)
        {
            const LADSPA_Properties category = rdf_descriptor->Type;

            // Specific Types
            if (category & (LADSPA_CLASS_DELAY|LADSPA_CLASS_REVERB))
                return PLUGIN_CATEGORY_DELAY;
            if (category & (LADSPA_CLASS_PHASER|LADSPA_CLASS_FLANGER|LADSPA_CLASS_CHORUS))
                return PLUGIN_CATEGORY_MODULATOR;
            if (category & (LADSPA_CLASS_AMPLIFIER))
                return PLUGIN_CATEGORY_DYNAMICS;
            if (category & (LADSPA_CLASS_UTILITY|LADSPA_CLASS_SPECTRAL|LADSPA_CLASS_FREQUENCY_METER))
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

        return getPluginCategoryFromName(m_name);
    }

    long uniqueId()
    {
        CARLA_ASSERT(descriptor);

        return descriptor ? descriptor->UniqueID : 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t parameterScalePointCount(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

            if (port)
                return port->ScalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < param.count);

        return paramBuffers[parameterId];
    }

    double getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
    {
        CARLA_ASSERT(parameterId < param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

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
        CARLA_ASSERT(descriptor);

        if (descriptor && descriptor->Label)
            strncpy(strBuf, descriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(descriptor);

        if (rdf_descriptor && rdf_descriptor->Creator)
            strncpy(strBuf, rdf_descriptor->Creator, STR_MAX);
        else if (descriptor && descriptor->Maker)
            strncpy(strBuf, descriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(descriptor);

        if (descriptor && descriptor->Copyright)
            strncpy(strBuf, descriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(descriptor);

        if (rdf_descriptor && rdf_descriptor->Title)
            strncpy(strBuf, rdf_descriptor->Title, STR_MAX);
        else if (descriptor && descriptor->Name)
            strncpy(strBuf, descriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (descriptor && rindex < (int32_t)descriptor->PortCount)
            strncpy(strBuf, descriptor->PortNames[rindex], STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterSymbol(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

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
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

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
        CARLA_ASSERT(parameterId < param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_Port* const port = &rdf_descriptor->Ports[rindex];

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

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(parameterId < param.count);

        paramBuffers[parameterId] = fixParameterValue(value, param.ranges[parameterId]);

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("LadspaPlugin::reload() - start");
        CARLA_ASSERT(descriptor);

        const ProcessMode processMode(x_engine->getOptions().processMode);

        // Safely disable plugin for reload
        const ScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aIns, aOuts, params, j;
        aIns = aOuts = params = 0;

        const double sampleRate = x_engine->getSampleRate();
        const unsigned long portCount = descriptor->PortCount;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        for (unsigned long i=0; i < portCount; i++)
        {
            const LADSPA_PortDescriptor portType = descriptor->PortDescriptors[i];

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

        if (x_engine->getOptions().forceStereo && (aIns == 1 || aOuts == 1) && ! h2)
        {
            h2 = descriptor->instantiate(descriptor, sampleRate);

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
            aIn.ports    = new CarlaEngineAudioPort*[aIns];
            aIn.rindexes = new uint32_t[aIns];
        }

        if (aOuts > 0)
        {
            aOut.ports    = new CarlaEngineAudioPort*[aOuts];
            aOut.rindexes = new uint32_t[aOuts];
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
            paramBuffers = new float[params];
        }

        bool needsCtrlIn  = false;
        bool needsCtrlOut = false;

        const int   portNameSize = x_engine->maxPortNameSize();
        CarlaString portName;

        for (unsigned long i=0; i < portCount; i++)
        {
            const LADSPA_PortDescriptor portType = descriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portHints = descriptor->PortRangeHints[i];
            const bool hasPortRDF = (rdf_descriptor && i < rdf_descriptor->PortCount);

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = m_name;
                    portName += ":";
                }

                portName += descriptor->PortNames[i];
                portName.truncate(portNameSize);

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    j = aIn.count++;
                    aIn.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                    aIn.rindexes[j] = i;

                    if (forcedStereoIn)
                    {
                        portName += "_2";
                        aIn.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                        aIn.rindexes[1] = i;
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    j = aOut.count++;
                    aOut.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                    aOut.rindexes[j] = i;
                    needsCtrlIn = true;

                    if (forcedStereoOut)
                    {
                        portName += "_2";
                        aOut.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                        aOut.rindexes[1] = i;
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LADSPA_IS_PORT_CONTROL(portType))
            {
                j = param.count++;
                param.data[j].index  = j;
                param.data[j].rindex = i;
                param.data[j].hints  = 0;
                param.data[j].midiChannel = 0;
                param.data[j].midiCC = -1;

                double min, max, def, step, stepSmall, stepLarge;

                // min value
                if (LADSPA_IS_HINT_BOUNDED_BELOW(portHints.HintDescriptor))
                    min = portHints.LowerBound;
                else
                    min = 0.0;

                // max value
                if (LADSPA_IS_HINT_BOUNDED_ABOVE(portHints.HintDescriptor))
                    max = portHints.UpperBound;
                else
                    max = 1.0;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                if (max - min == 0.0)
                {
                    qWarning("Broken plugin parameter: max - min == 0");
                    max = min + 0.1;
                }

                // default value
                if (hasPortRDF && LADSPA_PORT_HAS_DEFAULT(rdf_descriptor->Ports[i].Hints))
                    def = rdf_descriptor->Ports[i].Default;
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
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_TOGGLED(portHints.HintDescriptor))
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(portHints.HintDescriptor))
                {
                    step = 1.0;
                    stepSmall = 1.0;
                    stepLarge = 10.0;
                    param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    stepSmall = range/1000.0;
                    stepLarge = range/10.0;
                }

                if (LADSPA_IS_PORT_INPUT(portType))
                {
                    param.data[j].type   = PARAMETER_INPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;
                    param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needsCtrlIn = true;
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    if (strcmp(descriptor->PortNames[i], "latency") == 0 || strcmp(descriptor->PortNames[i], "_latency") == 0)
                    {
                        min = 0.0;
                        max = sampleRate;
                        def = 0.0;
                        step = 1.0;
                        stepSmall = 1.0;
                        stepLarge = 1.0;

                        param.data[j].type  = PARAMETER_LATENCY;
                        param.data[j].hints = 0;
                    }
                    else if (strcmp(descriptor->PortNames[i], "_sample-rate") == 0)
                    {
                        def = sampleRate;
                        step = 1.0;
                        stepSmall = 1.0;
                        stepLarge = 1.0;

                        param.data[j].type  = PARAMETER_SAMPLE_RATE;
                        param.data[j].hints = 0;
                    }
                    else
                    {
                        param.data[j].type   = PARAMETER_OUTPUT;
                        param.data[j].hints |= PARAMETER_IS_ENABLED;
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCtrlOut = true;
                    }
                }
                else
                {
                    param.data[j].type = PARAMETER_UNKNOWN;
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LADSPA_IS_HINT_LOGARITHMIC(portHints.HintDescriptor))
                    param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                // check for scalepoints, require at least 2 to make it useful
                if (hasPortRDF && rdf_descriptor->Ports[i].ScalePointCount > 1)
                    param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].stepSmall = stepSmall;
                param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                paramBuffers[j] = def;

                descriptor->connect_port(handle, i, &paramBuffers[j]);
                if (h2) descriptor->connect_port(h2, i, &paramBuffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");
                descriptor->connect_port(handle, i, nullptr);
                if (h2) descriptor->connect_port(h2, i, nullptr);
            }
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = m_name;
                portName += ":";
            }

            portName += "control-in";
            portName.truncate(portNameSize);

            param.portCin = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = m_name;
                portName += ":";
            }

            portName += "control-out";
            portName.truncate(portNameSize);

            param.portCout = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, false);
        }

        aIn.count   = aIns;
        aOut.count  = aOuts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0))
            m_hints |= PLUGIN_CAN_FORCE_STEREO;

        // check latency
        if (m_hints & PLUGIN_CAN_DRYWET)
        {
            bool hasLatency = false;
            m_latency = 0;

            for (uint32_t i=0; i < param.count; i++)
            {
                if (param.data[i].type == PARAMETER_LATENCY)
                {
                    // pre-run so plugin can update latency control-port
                    float tmpIn[2][aIns];
                    float tmpOut[2][aOuts];

                    for (j=0; j < aIn.count; j++)
                    {
                        tmpIn[j][0] = 0.0f;
                        tmpIn[j][1] = 0.0f;

                        if (j == 0 || ! h2)
                            descriptor->connect_port(handle, aIn.rindexes[j], tmpIn[j]);
                    }

                    for (j=0; j < aOut.count; j++)
                    {
                        tmpOut[j][0] = 0.0f;
                        tmpOut[j][1] = 0.0f;

                        if (j == 0 || ! h2)
                            descriptor->connect_port(handle, aOut.rindexes[j], tmpOut[j]);
                    }

                    if (descriptor->activate)
                        descriptor->activate(handle);

                    descriptor->run(handle, 2);

                    if (descriptor->deactivate)
                        descriptor->deactivate(handle);

                    m_latency = rint(paramBuffers[i]);
                    hasLatency = true;
                    break;
                }
            }

            if (hasLatency)
            {
                x_client->setLatency(m_latency);
                recreateLatencyBuffers();
            }
        }

        x_client->activate();

        qDebug("LadspaPlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;

        double aInsPeak[2]  = { 0.0 };
        double aOutsPeak[2] = { 0.0 };

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (aIn.count > 0 && x_engine->getOptions().processMode != PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (aIn.count == 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (std::abs(inBuffer[0][k]) > aInsPeak[0])
                        aInsPeak[0] = std::abs(inBuffer[0][k]);
                }
            }
            else if (aIn.count > 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (std::abs(inBuffer[0][k]) > aInsPeak[0])
                        aInsPeak[0] = std::abs(inBuffer[0][k]);

                    if (std::abs(inBuffer[1][k]) > aInsPeak[1])
                        aInsPeak[1] = std::abs(inBuffer[1][k]);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.portCin && m_active && m_activeBefore)
        {
            const CarlaEngineControlEvent* cinEvent;
            uint32_t time, nEvents = param.portCin->getEventCount();

            for (i=0; i < nEvents; i++)
            {
                cinEvent = param.portCin->getEvent(i);

                if (! cinEvent)
                    continue;

                time = cinEvent->time - framesOffset;

                if (time >= frames)
                    continue;

                // Control change
                switch (cinEvent->type)
                {
                case CarlaEngineNullEvent:
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
                        if (descriptor->deactivate)
                        {
                            descriptor->deactivate(handle);
                            if (h2) descriptor->deactivate(h2);
                        }

                        if (descriptor->activate)
                        {
                            descriptor->activate(handle);
                            if (h2) descriptor->activate(h2);
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

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (m_latency > 0)
                {
                    for (i=0; i < aIn.count; i++)
                        memset(m_latencyBuffers[i], 0, sizeof(float)*m_latency);
                }

                if (descriptor->activate)
                {
                    descriptor->activate(handle);
                    if (h2) descriptor->activate(h2);
                }
            }

            for (i=0; i < aIn.count; i++)
            {
                if (i == 0 || ! h2) descriptor->connect_port(handle, aIn.rindexes[i], inBuffer[i]);
                else if (i == 1)    descriptor->connect_port(h2, aIn.rindexes[i], inBuffer[i]);
            }

            for (i=0; i < aOut.count; i++)
            {
                if (i == 0 || ! h2) descriptor->connect_port(handle, aOut.rindexes[i], outBuffer[i]);
                else if (i == 1)    descriptor->connect_port(h2, aOut.rindexes[i], outBuffer[i]);
            }

            descriptor->run(handle, frames);
            if (h2) descriptor->run(h2, frames);
        }
        else
        {
            if (m_activeBefore)
            {
                if (descriptor->deactivate)
                {
                    descriptor->deactivate(handle);
                    if (h2) descriptor->deactivate(h2);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            bool do_drywet  = (m_hints & PLUGIN_CAN_DRYWET) > 0 && x_dryWet != 1.0;
            bool do_volume  = (m_hints & PLUGIN_CAN_VOLUME) > 0 && x_volume != 1.0;
            bool do_balance = (m_hints & PLUGIN_CAN_BALANCE) > 0 && (x_balanceLeft != -1.0 || x_balanceRight != 1.0);

            double bal_rangeL, bal_rangeR;
            float bufValue, oldBufLeft[do_balance ? frames : 1];

            for (i=0; i < aOut.count; i++)
            {
                // Dry/Wet
                if (do_drywet)
                {
                    for (k=0; k < frames; k++)
                    {
                        if (k < m_latency && m_latency < frames)
                            bufValue = (aIn.count == 1) ? m_latencyBuffers[0][k] : m_latencyBuffers[i][k];
                        else
                            bufValue = (aIn.count == 1) ? inBuffer[0][k-m_latency] : inBuffer[i][k-m_latency];

                        outBuffer[i][k] = (outBuffer[i][k]*x_dryWet)+(bufValue*(1.0-x_dryWet));
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_balanceLeft+1.0)/2;
                    bal_rangeR = (x_balanceRight+1.0)/2;

                    for (k=0; k < frames; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            outBuffer[i][k]  = oldBufLeft[k]*(1.0-bal_rangeL);
                            outBuffer[i][k] += outBuffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k]*bal_rangeR;
                            outBuffer[i][k] += oldBufLeft[k]*bal_rangeL;
                        }
                    }
                }

                // Volume
                if (do_volume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= x_volume;
                }

                // Output VU
                if (x_engine->getOptions().processMode != PROCESS_MODE_CONTINUOUS_RACK)
                {
                    for (k=0; i < 2 && k < frames; k++)
                    {
                        if (abs(outBuffer[i][k]) > aOutsPeak[i])
                            aOutsPeak[i] = abs(outBuffer[i][k]);
                    }
                }
            }

            // Latency, save values for next callback
            if (m_latency > 0 && m_latency < frames)
            {
                for (i=0; i < aIn.count; i++)
                    memcpy(m_latencyBuffers[i], inBuffer[i] + (frames - m_latency), sizeof(float)*m_latency);
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aOut.count; i++)
                carla_zeroF(outBuffer[i], frames);

            aOutsPeak[0] = 0.0;
            aOutsPeak[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.portCout && m_active)
        {
            double value;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT)
                {
                    fixParameterValue(paramBuffers[k], param.ranges[k]);

                    if (param.data[k].midiCC > 0)
                    {
                        value = (paramBuffers[k] - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min);
                        param.portCout->writeEvent(CarlaEngineParameterChangeEvent, framesOffset, param.data[k].midiChannel, param.data[k].midiCC, value);
                    }
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        x_engine->setInputPeak(m_id, 0, aInsPeak[0]);
        x_engine->setInputPeak(m_id, 1, aInsPeak[1]);
        x_engine->setOutputPeak(m_id, 0, aOutsPeak[0]);
        x_engine->setOutputPeak(m_id, 1, aOutsPeak[1]);

        m_activeBefore = m_active;
    }

    // -------------------------------------------------------------------
    // Cleanup

    void deleteBuffers()
    {
        qDebug("LadspaPlugin::deleteBuffers() - start");

        if (param.count > 0)
            delete[] paramBuffers;

        paramBuffers = nullptr;

        CarlaPlugin::deleteBuffers();

        qDebug("LadspaPlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const LADSPA_RDF_Descriptor* const rdf_descriptor_)
    {
        // ---------------------------------------------------------------
        // open DLL

        if (! libOpen(filename))
        {
            x_engine->setLastError(libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        const LADSPA_Descriptor_Function descFn = (LADSPA_Descriptor_Function)libSymbol("ladspa_descriptor");

        if (! descFn)
        {
            x_engine->setLastError("Could not find the LASDPA Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((descriptor = descFn(i++)))
        {
            if (strcmp(descriptor->Label, label) == 0)
                break;
        }

        if (! descriptor)
        {
            x_engine->setLastError("Could not find the requested plugin Label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(filename);

        if (is_ladspa_rdf_descriptor_valid(rdf_descriptor_, descriptor))
            rdf_descriptor = ladspa_rdf_dup(rdf_descriptor_);

        if (name)
            m_name = x_engine->getUniquePluginName(name);
        else if (rdf_descriptor && rdf_descriptor->Title)
            m_name = x_engine->getUniquePluginName(rdf_descriptor->Title);
        else
            m_name = x_engine->getUniquePluginName(descriptor->Name);

        // ---------------------------------------------------------------
        // register client

        x_client = x_engine->addClient(this);

        if (! x_client->isOk())
        {
            x_engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        handle = descriptor->instantiate(descriptor, x_engine->getSampleRate());

        if (! handle)
        {
            x_engine->setLastError("Plugin failed to initialize");
            return false;
        }

        return true;
    }

private:
    LADSPA_Handle handle, h2;
    const LADSPA_Descriptor* descriptor;
    const LADSPA_RDF_Descriptor* rdf_descriptor;

    float* paramBuffers;
};

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#else // WANT_LADSPA
#  warning Building without LADSPA support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newLADSPA(const initializer& init, const void* const extra)
{
    qDebug("CarlaPlugin::newLADSPA({%p, \"%s\", \"%s\", \"%s\"}, %p)", init.engine, init.filename, init.name, init.label, extra);

#ifdef WANT_LADSPA
    short id = init.engine->getNewPluginId();

    if (id < 0 || id > init.engine->maxPluginNumber())
    {
        init.engine->setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    LadspaPlugin* const plugin = new LadspaPlugin(init.engine, id);

    if (! plugin->init(init.filename, init.name, init.label, (const LADSPA_RDF_Descriptor*)extra))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getOptions().processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (! (plugin->hints() & PLUGIN_CAN_FORCE_STEREO))
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
