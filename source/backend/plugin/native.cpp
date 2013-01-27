/*
 * Carla Native Plugin
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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
#include "carla_native.h"

CARLA_BACKEND_START_NAMESPACE

struct NativePluginMidiData {
    uint32_t  count;
    uint32_t* indexes;
    CarlaEngineMidiPort** ports;

    NativePluginMidiData()
        : count(0),
          indexes(nullptr),
          ports(nullptr) {}
};

class NativePlugin : public CarlaPlugin
{
public:
    NativePlugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        qDebug("NativePlugin::NativePlugin()");

        m_type = PLUGIN_INTERNAL;

        descriptor  = nullptr;
        handle = h2 = nullptr;

        host.handle = this;
        host.get_buffer_size  = carla_host_get_buffer_size;
        host.get_sample_rate  = carla_host_get_sample_rate;
        host.get_time_info    = carla_host_get_time_info;
        host.write_midi_event = carla_host_write_midi_event;
        host.ui_parameter_changed   = carla_host_ui_parameter_changed;
        host.ui_custom_data_changed = carla_host_ui_custom_data_changed;
        host.ui_closed = carla_host_ui_closed;

        isProcessing = false;

        midiEventCount = 0;
        memset(midiEvents, 0, sizeof(::MidiEvent) * MAX_MIDI_EVENTS * 2);
    }

    ~NativePlugin()
    {
        qDebug("NativePlugin::~NativePlugin()");

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
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        CARLA_ASSERT(descriptor);

        if (descriptor)
            return static_cast<PluginCategory>(descriptor->category);

        return getPluginCategoryFromName(m_name);
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount()
    {
        return mIn.count;
    }

    uint32_t midiOutCount()
    {
        return mOut.count;
    }

    uint32_t parameterScalePointCount(const uint32_t parameterId)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);

        if (descriptor && handle && parameterId < param.count && descriptor->get_parameter_info)
        {
            const ::Parameter* const param = descriptor->get_parameter_info(handle, parameterId);

            if (param)
                return param->scalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);

        if (descriptor && handle && parameterId < param.count && descriptor->get_parameter_value)
            return descriptor->get_parameter_value(handle, parameterId);

        return 0.0;
    }

    double getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        if (descriptor && handle && parameterId < param.count && descriptor->get_parameter_info)
        {
            const ::Parameter* const param = descriptor->get_parameter_info(handle, parameterId);

            if (param && scalePointId < param->scalePointCount && param->scalePoints)
            {
                const ::ParameterScalePoint* const scalePoint = &param->scalePoints[scalePointId];

                if (scalePoint)
                    return scalePoint->value;
            }
        }

        return 0.0;
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(descriptor);

        if (descriptor && descriptor->label)
            strncpy(strBuf, descriptor->label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(descriptor);

        if (descriptor && descriptor->maker)
            strncpy(strBuf, descriptor->maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(descriptor);

        if (descriptor && descriptor->copyright)
            strncpy(strBuf, descriptor->copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(descriptor);

        if (descriptor && descriptor->name)
            strncpy(strBuf, descriptor->name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);

        if (descriptor && handle && parameterId < param.count && descriptor->get_parameter_info)
        {
            const ::Parameter* const param = descriptor->get_parameter_info(handle, parameterId);

            if (param && param->name)
            {
                strncpy(strBuf, param->name, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);

        if (descriptor && handle && parameterId < param.count && descriptor->get_parameter_text)
        {
            const char* const text = descriptor->get_parameter_text(handle, parameterId);

            if (text)
            {
                strncpy(strBuf, text, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterText(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);

        if (descriptor && handle && parameterId < param.count && descriptor->get_parameter_info)
        {
            const ::Parameter* const param = descriptor->get_parameter_info(handle, parameterId);

            if (param && param->unit)
            {
                strncpy(strBuf, param->unit, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        if (descriptor && handle && parameterId < param.count && descriptor->get_parameter_info)
        {
            const ::Parameter* const param = descriptor->get_parameter_info(handle, parameterId);

            if (param && scalePointId < param->scalePointCount && param->scalePoints)
            {
                const ::ParameterScalePoint* const scalePoint = &param->scalePoints[scalePointId];

                if (scalePoint && scalePoint->label)
                {
                    strncpy(strBuf, scalePoint->label, STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    void getGuiInfo(GuiType* const type, bool* const resizable)
    {
        *type = GUI_EXTERNAL_LV2; // FIXME, should be as _PLUGIN
        *resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(parameterId < param.count);

        if (descriptor && handle && parameterId < param.count)
        {
            fixParameterValue(value, param.ranges[parameterId]);

            descriptor->set_parameter_value(handle, param.data[parameterId].rindex, value);
            if (h2) descriptor->set_parameter_value(h2, param.data[parameterId].rindex, value);
        }

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(type);
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        if (! type)
            return qCritical("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (! key)
            return qCritical("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (! value)
            return qCritical("Nativelugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        if (descriptor && handle)
        {
            if (descriptor->set_custom_data)
            {
                descriptor->set_custom_data(handle, key, value);
                if (h2) descriptor->set_custom_data(h2, key, value);
            }

            // FIXME - only if gui was started before
            if (sendGui && descriptor->ui_set_custom_data)
                descriptor->ui_set_custom_data(handle, key, value);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(index >= -1 && index < (int32_t)midiprog.count);

        if (index < -1)
            index = -1;
        else if (index > (int32_t)midiprog.count)
            return;

        if (descriptor && handle && index >= 0)
        {
            if (x_engine->isOffline())
            {
                const CarlaEngine::ScopedLocker m(x_engine, block);
                descriptor->set_midi_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (h2) descriptor->set_midi_program(h2, midiprog.data[index].bank, midiprog.data[index].program);
            }
            else
            {
                const ScopedDisabler m(this, block);
                descriptor->set_midi_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (h2) descriptor->set_midi_program(h2, midiprog.data[index].bank, midiprog.data[index].program);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);

        if (descriptor && handle && descriptor->ui_show)
            descriptor->ui_show(handle, yesNo);
    }

    void idleGui()
    {
        // FIXME - this should not be called if there's no GUI!
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);

        if (descriptor && handle && descriptor->ui_idle)
            descriptor->ui_idle(handle);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("NativePlugin::reload() - start");
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

        uint32_t aIns, aOuts, mIns, mOuts, params, j;
        aIns = aOuts = mIns = mOuts = params = 0;

        const double sampleRate = x_engine->getSampleRate();

        aIns   = descriptor->audioIns;
        aOuts  = descriptor->audioOuts;
        mIns   = descriptor->midiIns;
        mOuts  = descriptor->midiOuts;
        params = descriptor->get_parameter_count ? descriptor->get_parameter_count(handle) : 0;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        if (x_engine->getOptions().forceStereo && (aIns == 1 || aOuts == 1) && mIns <= 1 && mOuts <= 1 && ! h2)
        {
            h2 = descriptor->instantiate(descriptor, &host);

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

        if (mIns > 0)
        {
            mIn.ports   = new CarlaEngineMidiPort*[mIns];
            mIn.indexes = new uint32_t[mIns];
        }

        if (mOuts > 0)
        {
            mOut.ports   = new CarlaEngineMidiPort*[mOuts];
            mOut.indexes = new uint32_t[mOuts];
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
        }

        const int portNameSize = x_engine->maxPortNameSize() - 2;
        char portName[portNameSize];
        bool needsCtrlIn  = false;
        bool needsCtrlOut = false;

        // Audio Ins
        for (j=0; j < descriptor->audioIns; j++)
        {
            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                sprintf(portName, "%s:input_%02i", m_name, j+1);
            else
                sprintf(portName, "input_%02i", j+1);

            aIn.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
            aIn.rindexes[j] = j;

            if (forcedStereoIn)
            {
                strcat(portName, "_");
                aIn.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                aIn.rindexes[1] = j;
            }
        }

        // Audio Outs
        for (j=0; j < descriptor->audioOuts; j++)
        {
            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                sprintf(portName, "%s:output_%02i", m_name, j+1);
            else
                sprintf(portName, "output_%02i", j+1);

            qDebug("Audio Out #%i", j);
            aOut.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
            aOut.rindexes[j] = j;
            needsCtrlIn = true;

            if (forcedStereoOut)
            {
                strcat(portName, "_");
                aOut.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                aOut.rindexes[1] = j;
            }
        }

        // MIDI Input
        for (j=0; j < mIns; j++)
        {
            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                sprintf(portName, "%s:midi-in_%02i", m_name, j+1);
            else
                sprintf(portName, "midi-in_%02i", j+1);

            mIn.ports[j]   = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
            mIn.indexes[j] = j;
        }

        // MIDI Output
        for (j=0; j < mOuts; j++)
        {
            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                sprintf(portName, "%s:midi-out_%02i", m_name, j+1);
            else
                sprintf(portName, "midi-out_%02i", j+1);

            mOut.ports[j]   = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
            mOut.indexes[j] = j;
        }

        for (j=0; j < params; j++)
        {
            if (! descriptor->get_parameter_info)
                break;

            const ::Parameter* const paramInfo = descriptor->get_parameter_info(handle, j);

            //const uint32_t paramHints  = param->hints;
            //const bool     paramOutput = paramHins;

            param.data[j].index  = j;
            param.data[j].rindex = j;
            param.data[j].hints  = 0;
            param.data[j].midiChannel = 0;
            param.data[j].midiCC = -1;

            double min, max, def, step, stepSmall, stepLarge;

            // min value
            min = paramInfo->ranges.min;

            // max value
            max = paramInfo->ranges.max;

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
            def = paramInfo->ranges.def;

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            if (paramInfo->hints & ::PARAMETER_USES_SAMPLE_RATE)
            {
                min *= sampleRate;
                max *= sampleRate;
                def *= sampleRate;
                param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
            }

            if (paramInfo->hints & ::PARAMETER_IS_BOOLEAN)
            {
                step = max - min;
                stepSmall = step;
                stepLarge = step;
                param.data[j].hints |= PARAMETER_IS_BOOLEAN;
            }
            else if (paramInfo->hints & ::PARAMETER_IS_INTEGER)
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

            if (paramInfo->hints & ::PARAMETER_IS_OUTPUT)
            {
                param.data[j].type = PARAMETER_OUTPUT;
                needsCtrlOut = true;
            }
            else
            {
                param.data[j].type = PARAMETER_INPUT;
                needsCtrlIn = true;
            }

            // extra parameter hints
            if (paramInfo->hints & ::PARAMETER_IS_ENABLED)
                param.data[j].hints |= PARAMETER_IS_ENABLED;

            if (paramInfo->hints & ::PARAMETER_IS_AUTOMABLE)
                param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

            if (paramInfo->hints & ::PARAMETER_IS_LOGARITHMIC)
                param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

            if (paramInfo->hints & ::PARAMETER_USES_SCALEPOINTS)
                param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

            if (paramInfo->hints & ::PARAMETER_USES_CUSTOM_TEXT)
                param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;

            param.ranges[j].min = min;
            param.ranges[j].max = max;
            param.ranges[j].def = def;
            param.ranges[j].step = step;
            param.ranges[j].stepSmall = stepSmall;
            param.ranges[j].stepLarge = stepLarge;
        }

        if (needsCtrlIn)
        {
            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-in");
            }
            else
                strcpy(portName, "control-in");

            param.portCin = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, true);
        }

        if (needsCtrlOut)
        {
            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-out");
            }
            else
                strcpy(portName, "control-out");

            param.portCout = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, false);
        }

        aIn.count   = aIns;
        aOut.count  = aOuts;
        mIn.count   = mIns;
        mOut.count  = mOuts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && mIns <= 1 && mOuts <= 1)
            m_hints |= PLUGIN_CAN_FORCE_STEREO;

        // native plugin hints
        if (descriptor->hints & ::PLUGIN_IS_SYNTH)
            m_hints |= PLUGIN_IS_SYNTH;
        if (descriptor->hints & ::PLUGIN_HAS_GUI)
            m_hints |= PLUGIN_HAS_GUI;
        if (descriptor->hints & ::PLUGIN_USES_SINGLE_THREAD)
            m_hints |= PLUGIN_USES_SINGLE_THREAD;

        reloadPrograms(true);

        x_client->activate();

        qDebug("NativePlugin::reload() - end");
    }

    void reloadPrograms(const bool init)
    {
        qDebug("NativePlugin::reloadPrograms(%s)", bool2str(init));
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

        midiprog.count = (descriptor->get_midi_program_count && descriptor->get_midi_program_info) ? descriptor->get_midi_program_count(handle) : 0;
        midiprog.data  = nullptr;

        if (midiprog.count > 0)
            midiprog.data = new MidiProgramData[midiprog.count];

        // Update data
        for (i=0; i < midiprog.count; i++)
        {
            const ::MidiProgram* const mpDesc = descriptor->get_midi_program_info(handle, i);
            CARLA_ASSERT(mpDesc);
            CARLA_ASSERT(mpDesc->name);

            midiprog.data[i].bank    = mpDesc->bank;
            midiprog.data[i].program = mpDesc->program;
            midiprog.data[i].name    = strdup(mpDesc->name);
        }

        // Update OSC Names
        if (x_engine->isOscControlRegistered())
        {
            x_engine->osc_send_control_set_midi_program_count(m_id, midiprog.count);

            for (i=0; i < midiprog.count; i++)
                x_engine->osc_send_control_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);
        }

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

        double aInsPeak[2]  = { 0.0 };
        double aOutsPeak[2] = { 0.0 };

        // reset MIDI
        midiEventCount = 0;
        memset(midiEvents, 0, sizeof(::MidiEvent) * MAX_MIDI_EVENTS * 2);

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
                        if (mIn.count > 0 && ! allNotesOffSent)
                            sendMidiAllNotesOff();

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

                        allNotesOffSent = true;
                    }
                    break;

                case CarlaEngineAllNotesOffEvent:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (mIn.count > 0 && ! allNotesOffSent)
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

        if (mIn.count > 0 && m_active && m_activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            {
                engineMidiLock();

                for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    if (extMidiNotes[i].channel < 0)
                        break;

                    ::MidiEvent* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(::MidiEvent));

                    midiEvent->data[0] = uint8_t(extMidiNotes[i].velo ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) + extMidiNotes[i].channel;
                    midiEvent->data[1] = extMidiNotes[i].note;
                    midiEvent->data[2] = extMidiNotes[i].velo;

                    extMidiNotes[i].channel = -1; // mark as invalid
                    midiEventCount += 1;
                }

                engineMidiUnlock();

            } // End of MIDI Input (External)

            CARLA_PROCESS_CONTINUE_CHECK;

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (System)

            for (i=0; i < mIn.count; i++)
            {
                if (! mIn.ports[i])
                    continue;

                const CarlaEngineMidiEvent* minEvent;
                uint32_t time, nEvents = mIn.ports[i]->getEventCount();

                for (k=0; k < nEvents && midiEventCount < MAX_MIDI_EVENTS; k++)
                {
                    minEvent = mIn.ports[i]->getEvent(k);

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

                    ::MidiEvent* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(::MidiEvent));

                    midiEvent->port = i;
                    midiEvent->time = minEvent->time;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        uint8_t note = minEvent->data[1];

                        midiEvent->data[0] = status;
                        midiEvent->data[1] = note;

                        postponeEvent(PluginPostEventNoteOff, channel, note, 0.0);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        uint8_t note = minEvent->data[1];
                        uint8_t velo = minEvent->data[2];

                        midiEvent->data[0] = status;
                        midiEvent->data[1] = note;
                        midiEvent->data[2] = velo;

                        postponeEvent(PluginPostEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                    {
                        uint8_t note     = minEvent->data[1];
                        uint8_t pressure = minEvent->data[2];

                        midiEvent->data[0] = status;
                        midiEvent->data[1] = note;
                        midiEvent->data[2] = pressure;
                    }
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                    {
                        uint8_t pressure = minEvent->data[1];

                        midiEvent->data[0] = status;
                        midiEvent->data[1] = pressure;
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                    {
                        uint8_t lsb = minEvent->data[1];
                        uint8_t msb = minEvent->data[2];

                        midiEvent->data[0] = status;
                        midiEvent->data[1] = lsb;
                        midiEvent->data[2] = msb;
                    }
                    else
                        continue;

                    midiEventCount += 1;
                }
            } // End of MIDI Input (System)

        } // End of MIDI Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        uint32_t midiEventCountBefore = midiEventCount;

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (mIn.count > 0)
                {
                    for (k=0; k < MAX_MIDI_CHANNELS; k++)
                    {
                        memset(&midiEvents[k], 0, sizeof(::MidiEvent));
                        midiEvents[k].data[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                        midiEvents[k].data[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                        memset(&midiEvents[k*2], 0, sizeof(::MidiEvent));
                        midiEvents[k*2].data[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                        midiEvents[k*2].data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                    }

                    midiEventCount = MAX_MIDI_CHANNELS*2;
                }

                if (descriptor->activate)
                {
                    descriptor->activate(handle);
                    if (h2) descriptor->activate(h2);
                }
            }

            isProcessing = true;

            if (h2)
            {
                descriptor->process(handle, inBuffer? &inBuffer[0] : nullptr, outBuffer? &outBuffer[0] : nullptr, frames, midiEventCountBefore, midiEvents);
                descriptor->process(h2,     inBuffer? &inBuffer[1] : nullptr, outBuffer? &outBuffer[1] : nullptr, frames, midiEventCountBefore, midiEvents);
            }
            else
                descriptor->process(handle, inBuffer, outBuffer, frames, midiEventCountBefore, midiEvents);

            isProcessing = false;
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
            float bufValue, oldBufLeft[do_balance ? frames : 0];

            for (i=0; i < aOut.count; i++)
            {
                // Dry/Wet
                if (do_drywet)
                {
                    for (k=0; k < frames; k++)
                    {
                        bufValue = (aIn.count == 1) ? inBuffer[0][k] : inBuffer[i][k];

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
        // MIDI Output

        if (mOut.count > 0 && m_active)
        {
            uint8_t data[3] = { 0 };

            for (uint32_t i = midiEventCountBefore; i < midiEventCount; i++)
            {
                data[0] = midiEvents[i].data[0];
                data[1] = midiEvents[i].data[1];
                data[2] = midiEvents[i].data[2];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(data[0]) && data[2] == 0)
                    data[0] -= 0x10;

                const uint32_t port = midiEvents[i].port;

                if (port < mOut.count)
                    mOut.ports[port]->writeEvent(midiEvents[i].time, data, 3);
            }

        } // End of MIDI Output

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.portCout && m_active)
        {
            double value, valueControl;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT)
                {
                    value = descriptor->get_parameter_value(handle, param.data[k].rindex);

                    if (param.data[k].midiCC > 0)
                    {
                        valueControl = (value - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min);
                        param.portCout->writeEvent(CarlaEngineParameterChangeEvent, framesOffset, param.data[k].midiChannel, param.data[k].midiCC, valueControl);
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

    void removeClientPorts()
    {
        qDebug("NativePlugin::removeClientPorts() - start");

        for (uint32_t i=0; i < mIn.count; i++)
        {
            delete mIn.ports[i];
            mIn.ports[i] = nullptr;
        }

        for (uint32_t i=0; i < mOut.count; i++)
        {
            delete mOut.ports[i];
            mOut.ports[i] = nullptr;
        }

        CarlaPlugin::removeClientPorts();

        qDebug("NativePlugin::removeClientPorts() - end");
    }

    void initBuffers()
    {
        for (uint32_t i=0; i < mIn.count; i++)
        {
            if (mIn.ports[i])
                mIn.ports[i]->initBuffer(x_engine);
        }

        for (uint32_t i=0; i < mOut.count; i++)
        {
            if (mOut.ports[i])
                mOut.ports[i]->initBuffer(x_engine);
        }

        CarlaPlugin::initBuffers();
    }

    void deleteBuffers()
    {
        qDebug("NativePlugin::deleteBuffers() - start");

        if (mIn.count > 0)
        {
            delete[] mIn.ports;
            delete[] mIn.indexes;
        }

        if (mOut.count > 0)
        {
            delete[] mOut.ports;
            delete[] mOut.indexes;
        }

        mIn.count = 0;
        mIn.ports = nullptr;
        mIn.indexes = nullptr;

        mOut.count = 0;
        mOut.ports = nullptr;
        mOut.indexes = nullptr;

        CarlaPlugin::deleteBuffers();

        qDebug("NativePlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    uint32_t handleGetBufferSize()
    {
        return x_engine->getBufferSize();
    }

    double handleGetSampleRate()
    {
        return x_engine->getSampleRate();
    }

    const TimeInfo* handleGetTimeInfo()
    {
        // TODO
        return nullptr;
    }

    bool handleWriteMidiEvent(MidiEvent* event)
    {
        CARLA_ASSERT(m_enabled);
        CARLA_ASSERT(mOut.count > 0);
        CARLA_ASSERT(isProcessing);
        CARLA_ASSERT(event);

        if (! m_enabled)
            return false;

        if (mOut.count == 0)
            return false;

        if (! isProcessing)
        {
            qCritical("NativePlugin::handleWriteMidiEvent(%p) - received MIDI out events outside audio thread, ignoring", event);
            return false;
        }

        if (midiEventCount >= MAX_MIDI_EVENTS*2)
            return false;

        memcpy(&midiEvents[midiEventCount], event, sizeof(::MidiEvent));
        midiEventCount += 1;

        return true;
    }

    void handleUiParameterChanged(uint32_t index, float value)
    {
        setParameterValue(index, value, false, true, true);
    }

    void handleUiCustomDataChanged(const char* key, const char* value)
    {
        setCustomData(CUSTOM_DATA_STRING, key, value, false);
    }

    void handleUiClosed()
    {
        x_engine->callback(CALLBACK_SHOW_GUI, m_id, 0, 0, 0.0, nullptr);
    }

    // -------------------------------------------------------------------

    static uint32_t carla_host_get_buffer_size(HostHandle handle)
    {
        CARLA_ASSERT(handle);
        return ((NativePlugin*)handle)->handleGetBufferSize();
    }

    static double carla_host_get_sample_rate(HostHandle handle)
    {
        CARLA_ASSERT(handle);
        return ((NativePlugin*)handle)->handleGetSampleRate();
    }

    static const TimeInfo* carla_host_get_time_info(HostHandle handle)
    {
        CARLA_ASSERT(handle);
        return ((NativePlugin*)handle)->handleGetTimeInfo();
    }

    static bool carla_host_write_midi_event(HostHandle handle, MidiEvent* event)
    {
        CARLA_ASSERT(handle);
        return ((NativePlugin*)handle)->handleWriteMidiEvent(event);
    }

    static void carla_host_ui_parameter_changed(HostHandle handle, uint32_t index, float value)
    {
        CARLA_ASSERT(handle);
        ((NativePlugin*)handle)->handleUiParameterChanged(index, value);
    }

    static void carla_host_ui_custom_data_changed(HostHandle handle, const char* key, const char* value)
    {
        CARLA_ASSERT(handle);
        ((NativePlugin*)handle)->handleUiCustomDataChanged(key, value);
    }

    static void carla_host_ui_closed(HostHandle handle)
    {
        CARLA_ASSERT(handle);
        ((NativePlugin*)handle)->handleUiClosed();
    }

    // -------------------------------------------------------------------

    static size_t getPluginCount()
    {
        maybeFirstInit();
        return pluginDescriptors.size();
    }

    static const PluginDescriptor* getPlugin(const size_t index)
    {
        CARLA_ASSERT(index < pluginDescriptors.size());

        if (index < pluginDescriptors.size())
            return pluginDescriptors[index];

        return nullptr;
    }

    static void registerPlugin(const PluginDescriptor* desc)
    {
        pluginDescriptors.push_back(desc);
    }

    static void maybeFirstInit()
    {
        if (! firstInit)
            return;

        firstInit = false;

        carla_register_native_plugin_bypass();
        carla_register_native_plugin_midiSplit();

        carla_register_native_plugin_3BandEQ();
        carla_register_native_plugin_3BandSplitter();
        carla_register_native_plugin_PingPongPan();

#ifdef WANT_ZYNADDSUBFX
        carla_register_native_plugin_zynaddsubfx();
#endif
    }

    // -------------------------------------------------------------------

    bool init(const char* const name, const char* const label)
    {
        // ---------------------------------------------------------------
        // initialize native-plugins descriptors

        maybeFirstInit();

        // ---------------------------------------------------------------
        // get descriptor that matches label

        for (size_t i=0; i < pluginDescriptors.size(); i++)
        {
            descriptor = pluginDescriptors[i];

            if (! descriptor)
                break;
            if (descriptor->label && strcmp(descriptor->label, label) == 0)
                break;

            descriptor = nullptr;
        }

        if (! descriptor)
        {
            x_engine->setLastError("Invalid internal plugin");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name)
            m_name = x_engine->getUniquePluginName(name);
        else
            m_name = x_engine->getUniquePluginName(descriptor->name);

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

        handle = descriptor->instantiate(descriptor, &host);

        if (! handle)
        {
            x_engine->setLastError("Plugin failed to initialize");
            return false;
        }

        return true;
    }

private:
    const PluginDescriptor* descriptor;
    PluginHandle handle, h2;
    HostDescriptor host;

    bool isProcessing;

    NativePluginMidiData mIn;
    NativePluginMidiData mOut;

    uint32_t    midiEventCount;
    ::MidiEvent midiEvents[MAX_MIDI_EVENTS*2];

    static bool firstInit;
    static std::vector<const PluginDescriptor*> pluginDescriptors;
};

bool NativePlugin::firstInit = true;
std::vector<const PluginDescriptor*> NativePlugin::pluginDescriptors;

// -----------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newNative(const initializer& init)
{
    qDebug("CarlaPlugin::newNative(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

    short id = init.engine->getNewPluginId();

    if (id < 0 || id > init.engine->maxPluginNumber())
    {
        init.engine->setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    NativePlugin* const plugin = new NativePlugin(init.engine, id);

    if (! plugin->init(init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getOptions().processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (! (plugin->hints() & PLUGIN_CAN_FORCE_STEREO))
        {
            init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo Internal plugins, sorry!");
            delete plugin;
            return nullptr;
        }
    }

    plugin->registerToOscClient();

    return plugin;
}

// -----------------------------------------------------------------------

size_t CarlaPlugin::getNativePluginCount()
{
    return NativePlugin::getPluginCount();
}

const PluginDescriptor* CarlaPlugin::getNativePluginDescriptor(const size_t index)
{
    return NativePlugin::getPlugin(index);
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

void carla_register_native_plugin(const PluginDescriptor* desc)
{
    CARLA_BACKEND_USE_NAMESPACE
    NativePlugin::registerPlugin(desc);
}
