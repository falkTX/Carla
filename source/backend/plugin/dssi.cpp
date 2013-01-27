/*
 * Carla DSSI Plugin
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

#ifdef WANT_DSSI

#include "carla_ladspa_utils.hpp"
#include "dssi/dssi.h"

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendDssiPlugin Carla Backend DSSI Plugin
 *
 * The Carla Backend DSSI Plugin.\n
 * http://dssi.sourceforge.net/
 * @{
 */

class DssiPlugin : public CarlaPlugin
{
public:
    DssiPlugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        qDebug("DssiPlugin::DssiPlugin()");

        m_type = PLUGIN_DSSI;

        handle = h2 = nullptr;
        descriptor  = nullptr;
        ldescriptor = nullptr;

        paramBuffers = nullptr;

        memset(midiEvents, 0, sizeof(snd_seq_event_t)*MAX_MIDI_EVENTS);
    }

    ~DssiPlugin()
    {
        qDebug("DssiPlugin::~DssiPlugin()");

        // close UI
        if (m_hints & PLUGIN_HAS_GUI)
        {
            showGui(false);

            if (osc.thread)
            {
                // Wait a bit first, try safe quit, then force kill
                if (osc.thread->isRunning() && ! osc.thread->wait(x_engine->getOptions().oscUiTimeout))
                {
                    qWarning("Failed to properly stop DSSI GUI thread");
                    osc.thread->terminate();
                }

                delete osc.thread;
            }
        }

        if (ldescriptor)
        {
            if (ldescriptor->deactivate && m_activeBefore)
            {
                if (handle)
                    ldescriptor->deactivate(handle);
                if (h2)
                    ldescriptor->deactivate(h2);
            }

            if (ldescriptor->cleanup)
            {
                if (handle)
                    ldescriptor->cleanup(handle);
                if (h2)
                    ldescriptor->cleanup(h2);
            }
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        if (m_hints & PLUGIN_IS_SYNTH)
            return PLUGIN_CATEGORY_SYNTH;

        return getPluginCategoryFromName(m_name);
    }

    long uniqueId()
    {
        CARLA_ASSERT(ldescriptor);

        return ldescriptor ? ldescriptor->UniqueID : 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr)
    {
        CARLA_ASSERT(dataPtr);
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(descriptor->get_custom_data);

        unsigned long dataSize = 0;

        if (descriptor->get_custom_data && descriptor->get_custom_data(handle, dataPtr, &dataSize))
            return dataSize;

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < param.count);

        return paramBuffers[parameterId];
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(ldescriptor);

        if (ldescriptor && ldescriptor->Label)
            strncpy(strBuf, ldescriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(ldescriptor);

        if (ldescriptor && ldescriptor->Maker)
            strncpy(strBuf, ldescriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(ldescriptor);

        if (ldescriptor && ldescriptor->Copyright)
            strncpy(strBuf, ldescriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(ldescriptor);

        if (ldescriptor && ldescriptor->Name)
            strncpy(strBuf, ldescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(ldescriptor);
        CARLA_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (ldescriptor && rindex < (int32_t)ldescriptor->PortCount)
            strncpy(strBuf, ldescriptor->PortNames[rindex], STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getGuiInfo(GuiType* const type, bool* const resizable)
    {
        CARLA_ASSERT(type);
        CARLA_ASSERT(resizable);

        *type = (m_hints & PLUGIN_HAS_GUI) ? GUI_EXTERNAL_OSC : GUI_NONE;
        *resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(parameterId < param.count);

        paramBuffers[parameterId] = fixParameterValue(value, param.ranges[parameterId]);

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
    {
        CARLA_ASSERT(type);
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        if (! type)
            return qCritical("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (strcmp(type, CUSTOM_DATA_STRING) != 0)
            return qCritical("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (! key)
            return qCritical("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (! value)
            return qCritical("DssiPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        descriptor->configure(handle, key, value);
        if (h2) descriptor->configure(h2, key, value);

        if (sendGui && osc.data.target)
            osc_send_configure(&osc.data, key, value);

        if (strcmp(key, "reloadprograms") == 0 || strcmp(key, "load") == 0 || strncmp(key, "patches", 7) == 0)
        {
            const ScopedDisabler m(this);
            reloadPrograms(false);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const char* const stringData)
    {
        CARLA_ASSERT(m_hints & PLUGIN_USES_CHUNKS);
        CARLA_ASSERT(stringData);

        static QByteArray chunk;
        chunk = QByteArray::fromBase64(stringData);

        if (x_engine->isOffline())
        {
            const CarlaEngine::ScopedLocker m(x_engine);
            descriptor->set_custom_data(handle, chunk.data(), chunk.size());
            if (h2) descriptor->set_custom_data(h2, chunk.data(), chunk.size());
        }
        else
        {
            const CarlaPlugin::ScopedDisabler m(this);
            descriptor->set_custom_data(handle, chunk.data(), chunk.size());
            if (h2) descriptor->set_custom_data(h2, chunk.data(), chunk.size());
        }
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        CARLA_ASSERT(index >= -1 && index < (int32_t)midiprog.count);

        if (index < -1)
            index = -1;
        else if (index > (int32_t)midiprog.count)
            return;

        if (index >= 0)
        {
            if (x_engine->isOffline())
            {
                const CarlaEngine::ScopedLocker m(x_engine, block);
                descriptor->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (h2) descriptor->select_program(h2, midiprog.data[index].bank, midiprog.data[index].program);
            }
            else
            {
                const ScopedDisabler m(this, block);
                descriptor->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (h2) descriptor->select_program(h2, midiprog.data[index].bank, midiprog.data[index].program);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
    {
        CARLA_ASSERT(osc.thread);

        if (! osc.thread)
        {
            qCritical("DssiPlugin::showGui(%s) - attempt to show gui, but it does not exist!", bool2str(yesNo));
            return;
        }

        if (yesNo)
        {
            osc.thread->start();
        }
        else
        {
            if (osc.data.target)
            {
                osc_send_hide(&osc.data);
                osc_send_quit(&osc.data);
                osc.data.free();
            }

            if (! osc.thread->wait(500))
                osc.thread->quit();
        }
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("DssiPlugin::reload() - start");
        CARLA_ASSERT(descriptor && ldescriptor);

        const ProcessMode processMode(x_engine->getOptions().processMode);

        // Safely disable plugin for reload
        const ScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aIns, aOuts, mIns, params, j;
        aIns = aOuts = mIns = params = 0;

        const double sampleRate = x_engine->getSampleRate();
        const unsigned long portCount = ldescriptor->PortCount;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        for (unsigned long i=0; i < portCount; i++)
        {
            const LADSPA_PortDescriptor portType = ldescriptor->PortDescriptors[i];

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
            h2 = ldescriptor->instantiate(ldescriptor, sampleRate);

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

        if (descriptor->run_synth || descriptor->run_multiple_synths)
            mIns = 1;

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
            const LADSPA_PortDescriptor portType = ldescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint portHints = ldescriptor->PortRangeHints[i];

            if (LADSPA_IS_PORT_AUDIO(portType))
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = m_name;
                    portName += ":";
                }

                portName += ldescriptor->PortNames[i];
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

                    // MIDI CC value
                    if (descriptor->get_midi_controller_for_port)
                    {
                        int controller = descriptor->get_midi_controller_for_port(handle, i);
                        if (DSSI_CONTROLLER_IS_SET(controller) && DSSI_IS_CC(controller))
                        {
                            int16_t cc = DSSI_CC_NUMBER(controller);
                            if (! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                param.data[j].midiCC = cc;
                        }
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(portType))
                {
                    if (strcmp(ldescriptor->PortNames[i], "latency") == 0 || strcmp(ldescriptor->PortNames[i], "_latency") == 0)
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
                    else if (strcmp(ldescriptor->PortNames[i], "_sample-rate") == 0)
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

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].stepSmall = stepSmall;
                param.ranges[j].stepLarge = stepLarge;

                // Start parameters in their default values
                paramBuffers[j] = def;

                ldescriptor->connect_port(handle, i, &paramBuffers[j]);
                if (h2) ldescriptor->connect_port(h2, i, &paramBuffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");
                ldescriptor->connect_port(handle, i, nullptr);
                if (h2) ldescriptor->connect_port(h2, i, nullptr);
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

        if (mIns == 1)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = m_name;
                portName += ":";
            }

            portName += "midi-in";
            portName.truncate(portNameSize);

            midi.portMin = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
        }

        aIn.count   = aIns;
        aOut.count  = aOuts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (midi.portMin && aOut.count > 0)
            m_hints |= PLUGIN_IS_SYNTH;

        if (x_engine->getOptions().useDssiVstChunks && QString(m_filename).endsWith("dssi-vst.so", Qt::CaseInsensitive))
        {
            if (descriptor->get_custom_data && descriptor->set_custom_data)
                m_hints |= PLUGIN_USES_CHUNKS;
        }

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
                            ldescriptor->connect_port(handle, aIn.rindexes[j], tmpIn[j]);
                    }

                    for (j=0; j < aOut.count; j++)
                    {
                        tmpOut[j][0] = 0.0f;
                        tmpOut[j][1] = 0.0f;

                        if (j == 0 || ! h2)
                            ldescriptor->connect_port(handle, aOut.rindexes[j], tmpOut[j]);
                    }

                    if (ldescriptor->activate)
                        ldescriptor->activate(handle);

                    ldescriptor->run(handle, 2);

                    if (ldescriptor->deactivate)
                        ldescriptor->deactivate(handle);

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

        reloadPrograms(true);

        x_client->activate();

        qDebug("DssiPlugin::reload() - end");
    }

    void reloadPrograms(const bool init)
    {
        qDebug("DssiPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount = midiprog.count;

        // Delete old programs
        if (midiprog.count > 0)
        {
            for (i=0; i < midiprog.count; i++)
            {
                if (midiprog.data[i].name)
                    free((void*)midiprog.data[i].name);
            }

            delete[] midiprog.data;
        }

        midiprog.count = 0;
        midiprog.data  = nullptr;

        // Query new programs
        if (descriptor->get_program && descriptor->select_program)
        {
            while (descriptor->get_program(handle, midiprog.count))
                midiprog.count += 1;
        }

        if (midiprog.count > 0)
            midiprog.data = new MidiProgramData[midiprog.count];

        // Update data
        for (i=0; i < midiprog.count; i++)
        {
            const DSSI_Program_Descriptor* const pdesc = descriptor->get_program(handle, i);
            CARLA_ASSERT(pdesc);
            CARLA_ASSERT(pdesc->Name);

            midiprog.data[i].bank    = pdesc->Bank;
            midiprog.data[i].program = pdesc->Program;
            midiprog.data[i].name    = strdup(pdesc->Name ? pdesc->Name : "");
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (x_engine->isOscControlRegistered())
        {
            x_engine->osc_send_control_set_midi_program_count(m_id, midiprog.count);

            for (i=0; i < midiprog.count; i++)
                x_engine->osc_send_control_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
        else
        {
            x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0, nullptr);

            // Check if current program is invalid
            bool programChanged = false;

            if (midiprog.count == oldCount+1)
            {
                // one midi program added, probably created by user
                midiprog.current = oldCount;
                programChanged   = true;
            }
            else if (midiprog.current >= (int32_t)midiprog.count)
            {
                // current midi program > count
                midiprog.current = 0;
                programChanged   = true;
            }
            else if (midiprog.current < 0 && midiprog.count > 0)
            {
                // programs exist now, but not before
                midiprog.current = 0;
                programChanged   = true;
            }
            else if (midiprog.current >= 0 && midiprog.count == 0)
            {
                // programs existed before, but not anymore
                midiprog.current = -1;
                programChanged   = true;
            }

            if (programChanged)
                setMidiProgram(midiprog.current, true, true, true, true);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;
        unsigned long midiEventCount = 0;

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
            bool allNotesOffSent = false;

            const CarlaEngineControlEvent* cinEvent;
            uint32_t time, nEvents = param.portCin->getEventCount();

            uint32_t nextBankId = 0;
            if (midiprog.current >= 0 && midiprog.count > 0)
                nextBankId = midiprog.data[midiprog.current].bank;

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
                    if (cinEvent->channel == m_ctrlInChannel)
                        nextBankId = rint(cinEvent->value);
                    break;

                case CarlaEngineMidiProgramChangeEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        uint32_t nextProgramId = rint(cinEvent->value);

                        for (k=0; k < midiprog.count; k++)
                        {
                            if (midiprog.data[k].bank == nextBankId && midiprog.data[k].program == nextProgramId)
                            {
                                setMidiProgram(k, false, false, false, false);
                                postponeEvent(PluginPostEventMidiProgramChange, k, 0, 0.0);
                                break;
                            }
                        }
                    }
                    break;

                case CarlaEngineAllSoundOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        if (ldescriptor->deactivate)
                        {
                            ldescriptor->deactivate(handle);
                            if (h2) ldescriptor->deactivate(h2);
                        }

                        if (ldescriptor->activate)
                        {
                            ldescriptor->activate(handle);
                            if (h2) ldescriptor->activate(h2);
                        }

                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 0.0);
                        postponeEvent(PluginPostEventParameterChange, PARAMETER_ACTIVE, 0, 1.0);

                        allNotesOffSent = true;
                    }
                    break;

                case CarlaEngineAllNotesOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        allNotesOffSent = true;
                    }
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input

        if (midi.portMin && m_active && m_activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            {
                engineMidiLock();

                for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    if (extMidiNotes[i].channel < 0)
                        break;

                    snd_seq_event_t* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(snd_seq_event_t));

                    midiEvent->type = extMidiNotes[i].velo ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    midiEvent->data.note.channel  = extMidiNotes[i].channel;
                    midiEvent->data.note.note     = extMidiNotes[i].note;
                    midiEvent->data.note.velocity = extMidiNotes[i].velo;

                    extMidiNotes[i].channel = -1; // mark as invalid
                    midiEventCount += 1;
                }

                engineMidiUnlock();

            } // End of MIDI Input (External)

            CARLA_PROCESS_CONTINUE_CHECK;

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (System)

            {
                const CarlaEngineMidiEvent* minEvent;
                uint32_t time, nEvents = midi.portMin->getEventCount();

                for (i=0; i < nEvents && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    minEvent = midi.portMin->getEvent(i);

                    if (! minEvent)
                        continue;

                    time = minEvent->time - framesOffset;

                    if (time >= frames)
                        continue;

                    uint8_t status  = minEvent->data[0];
                    uint8_t channel = status & 0x0F;

                    // Fix bad note-off
                    if (MIDI_IS_STATUS_NOTE_ON(status) && minEvent->data[2] == 0)
                        status -= 0x10;

                    snd_seq_event_t* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(snd_seq_event_t));

                    midiEvent->time.tick = time;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        uint8_t note = minEvent->data[1];

                        midiEvent->type = SND_SEQ_EVENT_NOTEOFF;
                        midiEvent->data.note.channel = channel;
                        midiEvent->data.note.note    = note;

                        postponeEvent(PluginPostEventNoteOff, channel, note, 0.0);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        uint8_t note = minEvent->data[1];
                        uint8_t velo = minEvent->data[2];

                        midiEvent->type = SND_SEQ_EVENT_NOTEON;
                        midiEvent->data.note.channel  = channel;
                        midiEvent->data.note.note     = note;
                        midiEvent->data.note.velocity = velo;

                        postponeEvent(PluginPostEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                    {
                        uint8_t note     = minEvent->data[1];
                        uint8_t pressure = minEvent->data[2];

                        midiEvent->type = SND_SEQ_EVENT_KEYPRESS;
                        midiEvent->data.note.channel  = channel;
                        midiEvent->data.note.note     = note;
                        midiEvent->data.note.velocity = pressure;
                    }
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                    {
                        uint8_t pressure = minEvent->data[1];

                        midiEvent->type = SND_SEQ_EVENT_CHANPRESS;
                        midiEvent->data.control.channel = channel;
                        midiEvent->data.control.value   = pressure;
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                    {
                        uint8_t lsb = minEvent->data[1];
                        uint8_t msb = minEvent->data[2];

                        midiEvent->type = SND_SEQ_EVENT_PITCHBEND;
                        midiEvent->data.control.channel = channel;
                        midiEvent->data.control.value   = ((msb << 7) | lsb) - 8192;
                    }
                    else
                        continue;

                    midiEventCount += 1;
                }
            } // End of MIDI Input (System)

        } // End of MIDI Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

#if 0
        for (k=0; k < param.count; k++)
        {
            if (param.data[k].type == PARAMETER_LATENCY)
            {
                // TODO
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
                if (midi.portMin)
                {
                    for (k=0; k < MAX_MIDI_CHANNELS; k++)
                    {
                        memset(&midiEvents[k], 0, sizeof(snd_seq_event_t));
                        midiEvents[k].type      = SND_SEQ_EVENT_CONTROLLER;
                        midiEvents[k].data.control.channel = k;
                        midiEvents[k].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;

                        memset(&midiEvents[k*2], 0, sizeof(snd_seq_event_t));
                        midiEvents[k*2].type      = SND_SEQ_EVENT_CONTROLLER;
                        midiEvents[k*2].data.control.channel = k;
                        midiEvents[k*2].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;
                    }

                    midiEventCount = MAX_MIDI_CHANNELS;
                }

                if (m_latency > 0)
                {
                    for (i=0; i < aIn.count; i++)
                        memset(m_latencyBuffers[i], 0, sizeof(float)*m_latency);
                }

                if (ldescriptor->activate)
                {
                    ldescriptor->activate(handle);
                    if (h2) ldescriptor->activate(h2);
                }
            }

            for (i=0; i < aIn.count; i++)
            {
                if (i == 0 || ! h2) ldescriptor->connect_port(handle, aIn.rindexes[i], inBuffer[i]);
                else if (i == 1)    ldescriptor->connect_port(h2, aIn.rindexes[i], inBuffer[i]);
            }

            for (i=0; i < aOut.count; i++)
            {
                if (i == 0 || ! h2) ldescriptor->connect_port(handle, aOut.rindexes[i], outBuffer[i]);
                else if (i == 1)    ldescriptor->connect_port(h2, aOut.rindexes[i], outBuffer[i]);
            }

            if (descriptor->run_synth)
            {
                descriptor->run_synth(handle, frames, midiEvents, midiEventCount);
                if (h2) descriptor->run_synth(h2, frames, midiEvents, midiEventCount);
            }
            else if (descriptor->run_multiple_synths)
            {
                LADSPA_Handle handlePtr[2] = { handle, h2 };
                snd_seq_event_t* midiEventsPtr[2] = { midiEvents, midiEvents };
                unsigned long midiEventCountPtr[2] = { midiEventCount, midiEventCount };
                descriptor->run_multiple_synths(h2 ? 2 : 1, handlePtr, frames, midiEventsPtr, midiEventCountPtr);
            }
            else
            {
                ldescriptor->run(handle, frames);
                if (h2) ldescriptor->run(h2, frames);
            }
        }
        else
        {
            if (m_activeBefore)
            {
                if (ldescriptor->deactivate)
                {
                    ldescriptor->deactivate(handle);
                    if (h2) ldescriptor->deactivate(h2);
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
                        if (std::abs(outBuffer[i][k]) > aOutsPeak[i])
                            aOutsPeak[i] = std::abs(outBuffer[i][k]);
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
    // Post-poned events

    void uiParameterChange(const uint32_t index, const double value)
    {
        CARLA_ASSERT(index < param.count);

        if (index >= param.count)
            return;
        if (! osc.data.target)
            return;

        osc_send_control(&osc.data, param.data[index].rindex, value);
    }

    void uiMidiProgramChange(const uint32_t index)
    {
        CARLA_ASSERT(index < midiprog.count);

        if (index >= midiprog.count)
            return;
        if (! osc.data.target)
            return;

        osc_send_program(&osc.data, midiprog.data[index].bank, midiprog.data[index].program);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        CARLA_ASSERT(channel < 16);
        CARLA_ASSERT(note < 128);
        CARLA_ASSERT(velo > 0 && velo < 128);

        if (! osc.data.target)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_ON + channel;
        midiData[2] = note;
        midiData[3] = velo;
        osc_send_midi(&osc.data, midiData);
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note)
    {
        CARLA_ASSERT(channel < 16);
        CARLA_ASSERT(note < 128);

        if (! osc.data.target)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;
        osc_send_midi(&osc.data, midiData);
    }

    // -------------------------------------------------------------------
    // Cleanup

    void deleteBuffers()
    {
        qDebug("DssiPlugin::deleteBuffers() - start");

        if (param.count > 0)
            delete[] paramBuffers;

        paramBuffers = nullptr;

        CarlaPlugin::deleteBuffers();

        qDebug("DssiPlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const char* const guiFilename)
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

        const DSSI_Descriptor_Function descFn = (DSSI_Descriptor_Function)libSymbol("dssi_descriptor");

        if (! descFn)
        {
            x_engine->setLastError("Could not find the DSSI Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((descriptor = descFn(i++)))
        {
            ldescriptor = descriptor->LADSPA_Plugin;
            if (ldescriptor && strcmp(ldescriptor->Label, label) == 0)
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

        if (name)
            m_name = x_engine->getUniquePluginName(name);
        else
            m_name = x_engine->getUniquePluginName(ldescriptor->Name);

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

        handle = ldescriptor->instantiate(ldescriptor, x_engine->getSampleRate());

        if (! handle)
        {
            x_engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (guiFilename)
        {
            osc.thread = new CarlaPluginThread(x_engine, this, CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
            osc.thread->setOscData(guiFilename, ldescriptor->Label);

            m_hints |= PLUGIN_HAS_GUI;
        }

        return true;
    }

private:
    LADSPA_Handle handle, h2;
    const LADSPA_Descriptor* ldescriptor;
    const DSSI_Descriptor* descriptor;
    snd_seq_event_t midiEvents[MAX_MIDI_EVENTS];

    float* paramBuffers;
};

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#else // WANT_DSSI
#  warning Building without DSSI support
#endif

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newDSSI(const initializer& init, const void* const extra)
{
    qDebug("CarlaPlugin::newDSSI(%p, \"%s\", \"%s\", \"%s\", %p)", init.engine, init.filename, init.name, init.label, extra);

#ifdef WANT_DSSI
    short id = init.engine->getNewPluginId();

    if (id < 0 || id > init.engine->maxPluginNumber())
    {
        init.engine->setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    DssiPlugin* const plugin = new DssiPlugin(init.engine, id);

    if (! plugin->init(init.filename, init.name, init.label, (const char*)extra))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getOptions().processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (! (plugin->hints() & PLUGIN_CAN_FORCE_STEREO))
        {
            init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo DSSI plugins, sorry!");
            delete plugin;
            return nullptr;
        }
    }

    plugin->registerToOscClient();

    return plugin;
#else
    init.engine->setLastError("DSSI support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
