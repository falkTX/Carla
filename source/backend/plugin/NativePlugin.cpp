/*
 * Carla Native Plugin
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

CARLA_BACKEND_START_NAMESPACE

#if 0
struct NativePluginMidiData {
    uint32_t  count;
    uint32_t* indexes;
    CarlaEngineMidiPort** ports;

    NativePluginMidiData()
        : count(0),
          indexes(nullptr),
          ports(nullptr) {}
};
#endif

class NativePlugin : public CarlaPlugin
{
public:
    NativePlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id)
    {
        carla_debug("NativePlugin::NativePlugin(%p, %i)", engine, id);

        fHandle  = nullptr;
        fHandle2 = nullptr;
        fDescriptor = nullptr;

        fHost.handle = this;
        fHost.get_buffer_size        = carla_host_get_buffer_size;
        fHost.get_sample_rate        = carla_host_get_sample_rate;
        fHost.get_time_info          = carla_host_get_time_info;
        fHost.write_midi_event       = carla_host_write_midi_event;
        fHost.ui_parameter_changed   = carla_host_ui_parameter_changed;
        fHost.ui_custom_data_changed = carla_host_ui_custom_data_changed;
        fHost.ui_closed              = carla_host_ui_closed;

        fIsProcessing = false;

        //midiEventCount = 0;
        //memset(midiEvents, 0, sizeof(::MidiEvent) * MAX_MIDI_EVENTS * 2);
    }

    ~NativePlugin()
    {
        carla_debug("NativePlugin::~NativePlugin()");

        if (fDescriptor != nullptr)
        {
            if (fDescriptor->deactivate != nullptr && kData->activeBefore)
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
    }

    // -------------------------------------------------------------------
    // Information (base)

    virtual PluginType type() const
    {
        return PLUGIN_INTERNAL;
    }

    PluginCategory category()
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr)
            return static_cast<PluginCategory>(fDescriptor->category);

        return getPluginCategoryFromName(fName);
    }

    // -------------------------------------------------------------------
    // Information (count)

#if 0
    uint32_t midiInCount()
    {
        return mIn.count;
    }

    uint32_t midiOutCount()
    {
        return mOut.count;
    }
#endif

    uint32_t parameterScalePointCount(const uint32_t parameterId) const
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->get_parameter_info != nullptr)
        {
            if (const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
                return param->scalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    float getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->get_parameter_value != nullptr)
            return fDescriptor->get_parameter_value(fHandle, parameterId);

        return 0.0f;
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->get_parameter_info != nullptr)
        {
            if (const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
            {
                const ParameterScalePoint& scalePoint = param->scalePoints[scalePointId];

                return scalePoint.value;
            }
        }

        return 0.0f;
    }

    void getLabel(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->label != nullptr)
            std::strncpy(strBuf, fDescriptor->label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->maker != nullptr)
            std::strncpy(strBuf, fDescriptor->maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->copyright != nullptr)
            std::strncpy(strBuf, fDescriptor->copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor != nullptr && fDescriptor->name != nullptr)
            std::strncpy(strBuf, fDescriptor->name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->get_parameter_info != nullptr)
        {
            const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId);

            if (param != nullptr && param->name != nullptr)
            {
                std::strncpy(strBuf, param->name, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->get_parameter_text != nullptr)
        {
            if (const char* const text = fDescriptor->get_parameter_text(fHandle, parameterId))
            {
                std::strncpy(strBuf, text, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterText(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->get_parameter_info != nullptr)
        {
            const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId);

            if (param != nullptr && param->unit != nullptr)
            {
                std::strncpy(strBuf, param->unit, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->get_parameter_info != nullptr)
        {
            if (const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
            {
                const ParameterScalePoint& scalePoint = param->scalePoints[scalePointId];

                if (scalePoint.label != nullptr)
                {
                    std::strncpy(strBuf, scalePoint.label, STR_MAX);
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
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue = kData->param.fixValue(parameterId, value);

        if (fDescriptor != nullptr && fHandle != nullptr && parameterId < kData->param.count && fDescriptor->set_parameter_value != nullptr)
        {
            fDescriptor->set_parameter_value(fHandle, parameterId, fixedValue);

            if (fHandle2 != nullptr)
                fDescriptor->set_parameter_value(fHandle2, parameterId, fixedValue);
        }

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

#if 0
    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
    {
        CARLA_ASSERT(descriptor);
        CARLA_ASSERT(handle);
        CARLA_ASSERT(type);
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        if (! type)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (! key)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (! value)
            return carla_stderr2("Nativelugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

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
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
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
#endif

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor != nullptr && fHandle != nullptr && fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, yesNo);
    }

    void idleGui()
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor != nullptr && fHandle != nullptr && fDescriptor->ui_idle != nullptr)
            fDescriptor->ui_idle(fHandle);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        carla_debug("NativePlugin::reload() - start");
        CARLA_ASSERT(fDescriptor != nullptr);

#if 0
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

            carla_debug("Audio Out #%i", j);
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
                carla_stderr("Broken plugin parameter: max - min == 0");
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
        fHints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE | PLUGIN_CAN_FORCE_STEREO);

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts%2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && mIns <= 1 && mOuts <= 1)
            fHints |= PLUGIN_CAN_FORCE_STEREO;
#endif

        // native plugin hints
        if (fDescriptor->hints & ::PLUGIN_IS_RTSAFE)
            fHints |= PLUGIN_IS_RTSAFE;
        if (fDescriptor->hints & ::PLUGIN_IS_SYNTH)
            fHints |= PLUGIN_IS_SYNTH;
        if (fDescriptor->hints & ::PLUGIN_HAS_GUI)
            fHints |= PLUGIN_HAS_GUI;
        if (fDescriptor->hints & ::PLUGIN_USES_SINGLE_THREAD)
            fHints |= PLUGIN_USES_SINGLE_THREAD;

        reloadPrograms(true);

        kData->client->activate();

        carla_debug("NativePlugin::reload() - end");
    }

#if 0
    void reloadPrograms(const bool init)
    {
        carla_debug("NativePlugin::reloadPrograms(%s)", bool2str(init));
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
        carla_debug("NativePlugin::removeClientPorts() - start");

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

        carla_debug("NativePlugin::removeClientPorts() - end");
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
        carla_debug("NativePlugin::deleteBuffers() - start");

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

        carla_debug("NativePlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------
#endif

protected:
    uint32_t handleGetBufferSize()
    {
        return kData->engine->getBufferSize();
    }

    double handleGetSampleRate()
    {
        return kData->engine->getSampleRate();
    }

    const TimeInfo* handleGetTimeInfo()
    {
        // TODO
        return nullptr;
    }

    bool handleWriteMidiEvent(const MidiEvent* const event)
    {
        CARLA_ASSERT(fEnabled);
        //CARLA_ASSERT(mOut.count > 0);
        CARLA_ASSERT(fIsProcessing);
        CARLA_ASSERT(event != nullptr);

        if (! fEnabled)
            return false;

        //if (mOut.count == 0)
        //    return false;

        if (! fIsProcessing)
        {
            carla_stderr2("NativePlugin::handleWriteMidiEvent(%p) - received MIDI out events outside audio thread, ignoring", event);
            return false;
        }

        //if (midiEventCount >= MAX_MIDI_EVENTS*2)
        //    return false;

        //memcpy(&midiEvents[midiEventCount], event, sizeof(::MidiEvent));
        //midiEventCount += 1;

        return true;
    }

    void handleUiParameterChanged(const uint32_t index, const float value)
    {
        setParameterValue(index, value, false, true, true);
    }

    void handleUiCustomDataChanged(const char* const key, const char* const value)
    {
        setCustomData(CUSTOM_DATA_STRING, key, value, false);
    }

    void handleUiClosed()
    {
        kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
    }

public:
    static size_t getPluginCount()
    {
        maybeFirstInit();
        return sPluginDescriptors.size();
    }

    static const PluginDescriptor* getPlugin(const size_t index)
    {
        maybeFirstInit();
        CARLA_ASSERT(index < sPluginDescriptors.size());

        if (index < sPluginDescriptors.size())
            return sPluginDescriptors[index];

        return nullptr;
    }

    static void registerPlugin(const PluginDescriptor* desc)
    {
        sPluginDescriptors.push_back(desc);
    }

    static void maybeFirstInit()
    {
        if (! sFirstInit)
            return;

        sFirstInit = false;

#ifndef BUILD_BRIDGE
        carla_register_native_plugin_bypass();
        carla_register_native_plugin_midiSplit();
        carla_register_native_plugin_midiThrough();

#if 0
        carla_register_native_plugin_3BandEQ();
        carla_register_native_plugin_3BandSplitter();
        carla_register_native_plugin_PingPongPan();
#endif

# ifdef WANT_ZYNADDSUBFX
        carla_register_native_plugin_zynaddsubfx();
# endif
#endif
    }

    // -------------------------------------------------------------------

    bool init(const char* const name, const char* const label)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(label);

        // ---------------------------------------------------------------
        // initialize native-plugins descriptors

        maybeFirstInit();

        // ---------------------------------------------------------------
        // get descriptor that matches label

        for (size_t i=0; i < sPluginDescriptors.size(); i++)
        {
            fDescriptor = sPluginDescriptors[i];

            if (fDescriptor == nullptr)
                break;
            if (fDescriptor->label != nullptr && std::strcmp(fDescriptor->label, label) == 0)
                break;

            fDescriptor = nullptr;
        }

        if (fDescriptor == nullptr)
        {
            kData->engine->setLastError("Invalid internal plugin");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
            fName = kData->engine->getNewUniquePluginName(name);
        else if (fDescriptor->name != nullptr)
            fName = kData->engine->getNewUniquePluginName(fDescriptor->name);
        else
            fName = kData->engine->getNewUniquePluginName(fDescriptor->name);

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

        fHandle = fDescriptor->instantiate(fDescriptor, &fHost);

        if (fHandle == nullptr)
        {
            kData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        return true;
    }

private:
    PluginHandle   fHandle;
    PluginHandle   fHandle2;
    HostDescriptor fHost;
    const PluginDescriptor* fDescriptor;

    bool fIsProcessing;

#if 0
    NativePluginMidiData mIn;
    NativePluginMidiData mOut;

    uint32_t    midiEventCount;
    ::MidiEvent midiEvents[MAX_MIDI_EVENTS*2];
#endif

    static bool sFirstInit;
    static std::vector<const PluginDescriptor*> sPluginDescriptors;

    // -------------------------------------------------------------------

    #define handlePtr ((NativePlugin*)handle)

    static uint32_t carla_host_get_buffer_size(HostHandle handle)
    {
        return handlePtr->handleGetBufferSize();
    }

    static double carla_host_get_sample_rate(HostHandle handle)
    {
        return handlePtr->handleGetSampleRate();
    }

    static const TimeInfo* carla_host_get_time_info(HostHandle handle)
    {
        return handlePtr->handleGetTimeInfo();
    }

    static bool carla_host_write_midi_event(HostHandle handle, const MidiEvent* event)
    {
        return handlePtr->handleWriteMidiEvent(event);
    }

    static void carla_host_ui_parameter_changed(HostHandle handle, uint32_t index, float value)
    {
        handlePtr->handleUiParameterChanged(index, value);
    }

    static void carla_host_ui_custom_data_changed(HostHandle handle, const char* key, const char* value)
    {
        handlePtr->handleUiCustomDataChanged(key, value);
    }

    static void carla_host_ui_closed(HostHandle handle)
    {
        handlePtr->handleUiClosed();
    }

    #undef handlePtr
};

bool NativePlugin::sFirstInit = true;
std::vector<const PluginDescriptor*> NativePlugin::sPluginDescriptors;

// -----------------------------------------------------------------------

size_t CarlaPlugin::getNativePluginCount()
{
    return NativePlugin::getPluginCount();
}

const PluginDescriptor* CarlaPlugin::getNativePluginDescriptor(const size_t index)
{
    return NativePlugin::getPlugin(index);
}

CarlaPlugin* CarlaPlugin::newNative(const Initializer& init)
{
    carla_debug("CarlaPlugin::newNative(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

    NativePlugin* const plugin = new NativePlugin(init.engine, init.id);

    if (! plugin->init(init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && (plugin->hints() & PLUGIN_CAN_FORCE_STEREO) == 0)
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo Internal plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    plugin->registerToOscClient();

    return plugin;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

void carla_register_native_plugin(const PluginDescriptor* desc)
{
    CARLA_BACKEND_USE_NAMESPACE
    NativePlugin::registerPlugin(desc);
}

// -----------------------------------------------------------------------
