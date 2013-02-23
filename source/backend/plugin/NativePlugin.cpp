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

#ifdef WANT_NATIVE

#include <QtGui/QFileDialog>

CARLA_BACKEND_START_NAMESPACE

struct NativePluginMidiData {
    uint32_t  count;
    uint32_t* indexes;
    CarlaEngineEventPort** ports;

    NativePluginMidiData()
        : count(0),
          indexes(nullptr),
          ports(nullptr) {}

    void createNew(const uint32_t count)
    {
        CARLA_ASSERT(ports == nullptr);
        CARLA_ASSERT(indexes == nullptr);

        if (ports == nullptr)
        {
            ports = new CarlaEngineEventPort*[count];

            for (uint32_t i=0; i < count; i++)
                ports[i] = nullptr;
        }

        if (indexes == nullptr)
        {
            indexes = new uint32_t[count];

            for (uint32_t i=0; i < count; i++)
                indexes[i] = 0;
        }

        this->count = count;
    }

    void clear()
    {
        if (ports != nullptr)
        {
            for (uint32_t i=0; i < count; i++)
            {
                if (ports[i] != nullptr)
                {
                    delete ports[i];
                    ports[i] = nullptr;
                }
            }

            delete[] ports;
            ports = nullptr;
        }

        if (indexes != nullptr)
        {
            delete[] indexes;
            indexes = nullptr;
        }

        count = 0;
    }

    void initBuffers(CarlaEngine* const engine)
    {
        for (uint32_t i=0; i < count; i++)
        {
            if (ports[i] != nullptr)
                ports[i]->initBuffer(engine);
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT_WITH_LEAK_DETECTOR(NativePluginMidiData)
};

class NativePlugin : public CarlaPlugin
{
public:
    NativePlugin(CarlaEngine* const engine, const unsigned int id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fDescriptor(nullptr),
          fIsProcessing(false),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fMidiEventCount(0)
    {
        carla_debug("NativePlugin::NativePlugin(%p, %i)", engine, id);

        fHost.handle = this;
        fHost.get_buffer_size        = carla_host_get_buffer_size;
        fHost.get_sample_rate        = carla_host_get_sample_rate;
        fHost.get_time_info          = carla_host_get_time_info;
        fHost.write_midi_event       = carla_host_write_midi_event;
        fHost.ui_parameter_changed   = carla_host_ui_parameter_changed;
        fHost.ui_custom_data_changed = carla_host_ui_custom_data_changed;
        fHost.ui_closed              = carla_host_ui_closed;

        carla_zeroMem(fMidiEvents, sizeof(::MidiEvent)*MAX_MIDI_EVENTS*2);
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

        deleteBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const
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

    uint32_t midiInCount()
    {
        return fMidiIn.count;
    }

    uint32_t midiOutCount()
    {
        return fMidiOut.count;
    }

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

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(type != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        if (type == nullptr)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_STRING) != 0)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is not string", type, key, value, bool2str(sendGui));

        if (key == nullptr)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (value == nullptr)
            return carla_stderr2("Nativelugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        if (fDescriptor != nullptr && fHandle != nullptr)
        {
            if (fDescriptor->set_custom_data != nullptr)
            {
                fDescriptor->set_custom_data(fHandle, key, value);

                if (fHandle2)
                    fDescriptor->set_custom_data(fHandle2, key, value);
            }

            if (sendGui && fDescriptor->ui_set_custom_data != nullptr)
                fDescriptor->ui_set_custom_data(fHandle, key, value);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        // FIXME
        if (fDescriptor != nullptr && fHandle != nullptr && index >= 0)
        {
            const uint32_t bank    = kData->midiprog.data[index].bank;
            const uint32_t program = kData->midiprog.data[index].program;

            if (kData->engine->isOffline())
            {
                //const CarlaEngine::ScopedLocker m(x_engine, block);
                fDescriptor->set_midi_program(fHandle, bank, program);

                if (fHandle2 != nullptr)
                    fDescriptor->set_midi_program(fHandle2, bank, program);
            }
            else
            {
                //const ScopedDisabler m(this, block);
                fDescriptor->set_midi_program(fHandle, bank, program);

                if (fHandle2 != nullptr)
                    fDescriptor->set_midi_program(fHandle2, bank, program);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor != nullptr && fHandle != nullptr)
        {
            if (fDescriptor->name != nullptr && std::strcmp(fDescriptor->label, "audiofile") == 0)
            {
                QString filenameTry = QFileDialog::getOpenFileName(nullptr, "Open Audio File");

                if (! filenameTry.isEmpty())
                    fDescriptor->set_custom_data(fHandle, "file", filenameTry.toUtf8().constData());

                kData->engine->callback(CALLBACK_SHOW_GUI, fId, 0, 0, 0.0f, nullptr);
            }
            else if (fDescriptor->ui_show != nullptr)
                fDescriptor->ui_show(fHandle, yesNo);
        }
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
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (kData->client->isActive())
            kData->client->deactivate();

        deleteBuffers();

        const double sampleRate = kData->engine->getSampleRate();

        uint32_t aIns, aOuts, mIns, mOuts, params, j;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        aIns   = fDescriptor->audioIns;
        aOuts  = fDescriptor->audioOuts;
        mIns   = fDescriptor->midiIns;
        mOuts  = fDescriptor->midiOuts;
        params = (fDescriptor->get_parameter_count != nullptr && fDescriptor->get_parameter_info != nullptr) ? fDescriptor->get_parameter_count(fHandle) : 0;

        if ((fOptions & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1) && mIns <= 1 && mOuts <= 1)
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(fDescriptor, &fHost);

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
            kData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; i++)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            kData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; i++)
                fAudioOutBuffers[i] = nullptr;
        }

        if (mIns > 0)
        {
            fMidiIn.createNew(mIns);
            needsCtrlIn = (mIns == 1);
        }

        if (mOuts > 0)
        {
            fMidiOut.createNew(mOuts);
            needsCtrlOut = (mOuts == 1);
        }

        if (params > 0)
        {
            kData->param.createNew(params);
        }

        const int   portNameSize = kData->engine->maxPortNameSize();
        CarlaString portName;

        // Audio Ins
        for (j=0; j < aIns; j++)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            if (aIns > 1)
            {
                portName += "input_";
                portName += CarlaString(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            kData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
            kData->audioIn.ports[j].rindex = j;

            if (forcedStereoIn)
            {
                portName += "_2";
                kData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
                kData->audioIn.ports[1].rindex = j;
                break;
            }
        }

        // Audio Outs
        for (j=0; j < aOuts; j++)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            if (aOuts > 1)
            {
                portName += "output_";
                portName += CarlaString(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[j].rindex = j;

            if (forcedStereoOut)
            {
                portName += "_2";
                kData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
                kData->audioOut.ports[1].rindex = j;
                break;
            }
        }

        // MIDI Input (only if multiple)
        if (mIns > 1)
        {
            for (j=0; j < mIns; j++)
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fName;
                    portName += ":";
                }

                portName += "midi-in_";
                portName += CarlaString(j+1);
                portName.truncate(portNameSize);

                fMidiIn.ports[j]   = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
                fMidiIn.indexes[j] = j;
            }
        }

        // MIDI Output (only if multiple)
        if (mOuts > 1)
        {
            for (j=0; j < mOuts; j++)
            {
                portName.clear();

                if (processMode == PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = fName;
                    portName += ":";
                }

                portName += "midi-out_";
                portName += CarlaString(j+1);
                portName.truncate(portNameSize);

                fMidiOut.ports[j]   = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
                fMidiOut.indexes[j] = j;
            }
        }

        for (j=0; j < params; j++)
        {
            const ::Parameter* const paramInfo = fDescriptor->get_parameter_info(fHandle, j);

            kData->param.data[j].index  = j;
            kData->param.data[j].rindex = j;
            kData->param.data[j].hints  = 0x0;
            kData->param.data[j].midiChannel = 0;
            kData->param.data[j].midiCC = -1;

            float min, max, def, step, stepSmall, stepLarge;

            // min value
            min = paramInfo->ranges.min;

            // max value
            max = paramInfo->ranges.max;

            if (min > max)
                max = min;
            else if (max < min)
                min = max;

            if (max - min == 0.0f)
            {
                carla_stderr2("WARNING - Broken plugin parameter '%s': max - min == 0.0f", paramInfo->name);
                max = min + 0.1f;
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
                kData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
            }

            if (paramInfo->hints & ::PARAMETER_IS_BOOLEAN)
            {
                step = max - min;
                stepSmall = step;
                stepLarge = step;
                kData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
            }
            else if (paramInfo->hints & ::PARAMETER_IS_INTEGER)
            {
                step = 1.0f;
                stepSmall = 1.0f;
                stepLarge = 10.0f;
                kData->param.data[j].hints |= PARAMETER_IS_INTEGER;
            }
            else
            {
                float range = max - min;
                step = range/100.0f;
                stepSmall = range/1000.0f;
                stepLarge = range/10.0f;
            }

            if (paramInfo->hints & ::PARAMETER_IS_OUTPUT)
            {
                kData->param.data[j].type = PARAMETER_OUTPUT;
                needsCtrlOut = true;
            }
            else
            {
                kData->param.data[j].type = PARAMETER_INPUT;
                needsCtrlIn = true;
            }

            // extra parameter hints
            if (paramInfo->hints & ::PARAMETER_IS_ENABLED)
                kData->param.data[j].hints |= PARAMETER_IS_ENABLED;

            if (paramInfo->hints & ::PARAMETER_IS_AUTOMABLE)
                kData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

            if (paramInfo->hints & ::PARAMETER_IS_LOGARITHMIC)
                kData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

            if (paramInfo->hints & ::PARAMETER_USES_SCALEPOINTS)
                kData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

            if (paramInfo->hints & ::PARAMETER_USES_CUSTOM_TEXT)
                kData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;

            kData->param.ranges[j].min = min;
            kData->param.ranges[j].max = max;
            kData->param.ranges[j].def = def;
            kData->param.ranges[j].step = step;
            kData->param.ranges[j].stepSmall = stepSmall;
            kData->param.ranges[j].stepLarge = stepLarge;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

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

        // native plugin hints
        if (fDescriptor->hints & ::PLUGIN_IS_RTSAFE)
            fHints |= PLUGIN_IS_RTSAFE;
        if (fDescriptor->hints & ::PLUGIN_IS_SYNTH)
            fHints |= PLUGIN_IS_SYNTH;
        if (fDescriptor->hints & ::PLUGIN_HAS_GUI)
            fHints |= PLUGIN_HAS_GUI;
        if (fDescriptor->hints & ::PLUGIN_USES_SINGLE_THREAD)
            fHints |= PLUGIN_USES_SINGLE_THREAD;

        // plugin options
        kData->availOptions &= ~(PLUGIN_OPTION_FIXED_BUFFER | PLUGIN_OPTION_SELF_AUTOMATION | PLUGIN_OPTION_SEND_ALL_SOUND_OFF | PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH | PLUGIN_OPTION_SEND_PITCHBEND);

        // always available if needed
        kData->availOptions |= PLUGIN_OPTION_FIXED_BUFFER;
        kData->availOptions |= PLUGIN_OPTION_SELF_AUTOMATION;

        // only for plugins with midi input
        if (mIns > 0)
        {
            kData->availOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            kData->availOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            kData->availOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
        }

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        kData->client->activate();

        carla_debug("NativePlugin::reload() - end");
    }

    void reloadPrograms(const bool init)
    {
        carla_debug("NativePlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount = kData->midiprog.count;

        // Delete old programs
        kData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fDescriptor->get_midi_program_count != nullptr && fDescriptor->get_midi_program_info != nullptr)
            count = fDescriptor->get_midi_program_count(fHandle);

        if (count > 0)
            kData->midiprog.createNew(count);

        // Update data
        for (i=0; i < kData->midiprog.count; i++)
        {
            const ::MidiProgram* const mpDesc = fDescriptor->get_midi_program_info(fHandle, i);
            CARLA_ASSERT(mpDesc != nullptr);
            CARLA_ASSERT(mpDesc->name != nullptr);

            kData->midiprog.data[i].bank    = mpDesc->bank;
            kData->midiprog.data[i].program = mpDesc->program;
            kData->midiprog.data[i].name    = carla_strdup(mpDesc->name);
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_midi_program_count(fId, kData->midiprog.count);

            for (i=0; i < kData->midiprog.count; i++)
                kData->engine->osc_send_control_set_midi_program_data(fId, i, kData->midiprog.data[i].bank, kData->midiprog.data[i].program, kData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (kData->midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
        else
        {
            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);

            // Check if current program is invalid
            bool programChanged = false;

            if (kData->midiprog.count == oldCount+1)
            {
                // one midi program added, probably created by user
                kData->midiprog.current = oldCount;
                programChanged = true;
            }
            else if (kData->midiprog.current >= static_cast<int32_t>(kData->midiprog.count))
            {
                // current midi program > count
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else if (kData->midiprog.current < 0 && kData->midiprog.count > 0)
            {
                // programs exist now, but not before
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else if (kData->midiprog.current >= 0 && kData->midiprog.count == 0)
            {
                // programs existed before, but not anymore
                kData->midiprog.current = -1;
                programChanged = true;
            }

            if (programChanged)
                setMidiProgram(kData->midiprog.current, true, true, true, true);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames)
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; i++)
                carla_zeroFloat(outBuffer[i], frames);

            if (kData->activeBefore)
            {
                if (fDescriptor->deactivate != nullptr)
                {
                    fDescriptor->deactivate(fHandle);

                    if (fHandle2 != nullptr)
                        fDescriptor->deactivate(fHandle2);
                }
            }

            kData->activeBefore = kData->active;
            return;
        }

        fMidiEventCount = 0;
        carla_zeroMem(fMidiEvents, sizeof(::MidiEvent)*MAX_MIDI_EVENTS*2);

        // --------------------------------------------------------------------------------------------------------
        // Check if not active before

        if (! kData->activeBefore)
        {
            if (kData->event.portIn != nullptr)
            {
                for (k=0, i=MAX_MIDI_CHANNELS; k < MAX_MIDI_CHANNELS; k++)
                {
                    fMidiEvents[k].data[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                    fMidiEvents[k].data[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                    fMidiEvents[k+i].data[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                    fMidiEvents[k+i].data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                }

                fMidiEventCount = MAX_MIDI_CHANNELS*2;
            }

            if (fDescriptor->activate != nullptr)
            {
                fDescriptor->activate(fHandle);

                if (fHandle2 != nullptr)
                    fDescriptor->activate(fHandle2);
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo& timeInfo = kData->engine->getTimeInfo();

        fTimeInfo.playing = timeInfo.playing;
        fTimeInfo.frame   = timeInfo.frame;
        fTimeInfo.time    = timeInfo.time;

        if (timeInfo.valid & EngineTimeInfo::ValidBBT)
        {
            fTimeInfo.bbt.valid = true;

            fTimeInfo.bbt.bar  = timeInfo.bbt.bar;
            fTimeInfo.bbt.beat = timeInfo.bbt.beat;
            fTimeInfo.bbt.tick = timeInfo.bbt.tick;
            fTimeInfo.bbt.barStartTick = timeInfo.bbt.barStartTick;

            fTimeInfo.bbt.beatsPerBar = timeInfo.bbt.beatsPerBar;
            fTimeInfo.bbt.beatType    = timeInfo.bbt.beatType;

            fTimeInfo.bbt.ticksPerBeat   = timeInfo.bbt.ticksPerBeat;
            fTimeInfo.bbt.beatsPerMinute = timeInfo.bbt.beatsPerMinute;
        }
        else
            fTimeInfo.bbt.valid = false;

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (kData->event.portIn != nullptr && kData->activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                while (fMidiEventCount < MAX_MIDI_EVENTS*2 && ! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note = kData->extNotes.data.getFirst(true);

                    CARLA_ASSERT(note.channel >= 0);

                    fMidiEvents[fMidiEventCount].port = 0;
                    fMidiEvents[fMidiEventCount].time = 0;
                    fMidiEvents[fMidiEventCount].data[0]  = (note.velo > 0) ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    fMidiEvents[fMidiEventCount].data[0] += note.channel;
                    fMidiEvents[fMidiEventCount].data[1]  = note.note;
                    fMidiEvents[fMidiEventCount].data[2]  = note.velo;

                    fMidiEventCount += 1;
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (fHints & PLUGIN_OPTION_FIXED_BUFFER) == 0;

            uint32_t time, nEvents = kData->event.portIn->getEventCount();
            uint32_t timeOffset = 0;

            uint32_t nextBankId = 0;
            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                nextBankId = kData->midiprog.data[kData->midiprog.current].bank;

            for (i=0; i < nEvents; i++)
            {
                const EngineEvent& event = kData->event.portIn->getEvent(i);

                time = event.time;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && sampleAccurate)
                {
                    processSingle(inBuffer, outBuffer, time - timeOffset, timeOffset);

                    if (fMidiEventCount > 0)
                    {
                        carla_zeroMem(fMidiEvents, sizeof(::MidiEvent)*fMidiEventCount);
                        fMidiEventCount = 0;
                    }

                    nextBankId = 0;
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
                        // Control backend stuff
                        if (event.channel == kData->ctrlChannel)
                        {
                            double value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127/100;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                                continue;
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (fHints & PLUGIN_CAN_BALANCE) > 0)
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
                        for (k=0; k < kData->param.count; k++)
                        {
                            if (kData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (kData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (kData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((kData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            double value;

                            if (kData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                // FIXME - ranges call for this
                                value = ctrlEvent.value * (kData->param.ranges[k].max - kData->param.ranges[k].min) + kData->param.ranges[k].min;

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, k, 0, value);
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == kData->ctrlChannel)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == kData->ctrlChannel)
                        {
                            const uint32_t nextProgramId = ctrlEvent.param;

                            for (k=0; k < kData->midiprog.count; k++)
                            {
                                if (kData->midiprog.data[k].bank == nextBankId && kData->midiprog.data[k].program == nextProgramId)
                                {
                                    setMidiProgram(k, false, false, false, false);
                                    postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0);
                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                                sendMidiAllNotesOff();

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

                            allNotesOffSent = true;
                        }

                        if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
                            continue;

                        fMidiEvents[fMidiEventCount].port = 0;
                        fMidiEvents[fMidiEventCount].time = sampleAccurate ? 0 : time;
                        fMidiEvents[fMidiEventCount].data[0] = MIDI_STATUS_CONTROL_CHANGE + event.channel;
                        fMidiEvents[fMidiEventCount].data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                        fMidiEvents[fMidiEventCount].data[2] = 0;

                        fMidiEventCount += 1;

                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (event.channel == kData->ctrlChannel)
                        {
                            if (! allNotesOffSent)
                                sendMidiAllNotesOff();

                            allNotesOffSent = true;
                        }

                        if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
                            continue;

                        fMidiEvents[fMidiEventCount].port = 0;
                        fMidiEvents[fMidiEventCount].time = sampleAccurate ? 0: time;
                        fMidiEvents[fMidiEventCount].data[0] = MIDI_STATUS_CONTROL_CHANGE + event.channel;
                        fMidiEvents[fMidiEventCount].data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                        fMidiEvents[fMidiEventCount].data[2] = 0;

                        fMidiEventCount += 1;

                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
                        continue;

                    const EngineMidiEvent& midiEvent = event.midi;

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;

                    if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fHints & PLUGIN_OPTION_SELF_AUTOMATION) == 0)
                        continue;

                    // Fix bad note-off (per DSSI spec)
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midiEvent.data[2] == 0)
                        status -= 0x10;

                    fMidiEvents[fMidiEventCount].port = 0;
                    fMidiEvents[fMidiEventCount].time = sampleAccurate ? 0 : time - timeOffset;

                    fMidiEvents[fMidiEventCount].data[0] = status + channel;
                    fMidiEvents[fMidiEventCount].data[1] = midiEvent.data[1];
                    fMidiEvents[fMidiEventCount].data[2] = midiEvent.data[2];

                    fMidiEventCount += 1;

                    break;
                }
                }
            }

            kData->postRtEvents.trySplice();

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
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (fHints & PLUGIN_CAN_DRYWET) > 0 && kData->postProc.dryWet != 1.0f;
            const bool doVolume  = (fHints & PLUGIN_CAN_VOLUME) > 0 && kData->postProc.volume != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) > 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; i++)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; k++)
                    {
                        bufValue = inBuffer[(kData->audioIn.count == 1) ? 0 : i][k];
                        outBuffer[i][k] = (outBuffer[i][k] * kData->postProc.dryWet) + (bufValue * (1.0f - kData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    if (i % 2 == 0)
                        carla_copyFloat(oldBufLeft, outBuffer[i], frames);

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

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
                        outBuffer[i][k] *= kData->postProc.volume;
                }
            }

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (fMidiOut.count > 0)
        {
            // reverse lookup
            for (uint32_t i = (MAX_MIDI_EVENTS*2)-1; i >= fMidiEventCount; i--)
            {
                if (fMidiEvents[i].data[0] == 0)
                    break;

                const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(fMidiEvents[i].data);
                const uint8_t port    = fMidiEvents[i].port;

                if (port < fMidiOut.count)
                    fMidiOut.ports[port]->writeMidiEvent(fMidiEvents[i].time, channel, port, fMidiEvents[i].data, 3);
            }

        } // End of MIDI Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (kData->event.portOut != nullptr)
        {
            float value, curValue;

            for (k=0; k < kData->param.count; k++)
            {
                if (kData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                curValue = fDescriptor->get_parameter_value(fHandle, k);
                kData->param.ranges[k].fixValue(curValue);

                if (kData->param.data[k].midiCC > 0)
                {
                    value = kData->param.ranges[k].normalizeValue(curValue);
                    kData->event.portOut->writeControlEvent(0, kData->param.data[k].midiChannel, kEngineControlEventTypeParameter, kData->param.data[k].midiCC, value);
                }
            }

        } // End of Control Output

        // --------------------------------------------------------------------------------------------------------

        kData->activeBefore = kData->active;
    }

    void processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        for (uint32_t i=0; i < kData->audioIn.count; i++)
            carla_copyFloat(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);
        for (uint32_t i=0; i < kData->audioOut.count; i++)
            carla_zeroFloat(fAudioOutBuffers[i], frames);

        fIsProcessing = true;

        fDescriptor->process(fHandle, fAudioInBuffers, fAudioOutBuffers, frames, fMidiEventCount, fMidiEvents);

        if (fHandle2 != nullptr)
            fDescriptor->process(fHandle2, fAudioInBuffers, fAudioOutBuffers, frames, fMidiEventCount, fMidiEvents);

        fIsProcessing = false;
        fTimeInfo.frame += frames;

        for (uint32_t i=0, k; i < kData->audioOut.count; i++)
        {
            for (k=0; k < frames; k++)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
    }

    void bufferSizeChanged(const uint32_t newBufferSize)
    {
        for (uint32_t i=0; i < kData->audioIn.count; i++)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < kData->audioOut.count; i++)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }
    }

    // -------------------------------------------------------------------
    // Cleanup

    void initBuffers()
    {
        fMidiIn.initBuffers(kData->engine);
        fMidiOut.initBuffers(kData->engine);

        CarlaPlugin::initBuffers();
    }

    void deleteBuffers()
    {
        carla_debug("NativePlugin::deleteBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; i++)
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
            for (uint32_t i=0; i < kData->audioOut.count; i++)
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

        fMidiIn.clear();
        fMidiOut.clear();

        CarlaPlugin::deleteBuffers();

        carla_debug("NativePlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

protected:
    uint32_t handleGetBufferSize()
    {
        return kData->engine->getBufferSize();
    }

    double handleGetSampleRate()
    {
        return kData->engine->getSampleRate();
    }

    const ::TimeInfo* handleGetTimeInfo()
    {
        CARLA_ASSERT(fIsProcessing);

        return &fTimeInfo;
    }

    bool handleWriteMidiEvent(const ::MidiEvent* const event)
    {
        CARLA_ASSERT(fEnabled);
        CARLA_ASSERT(fMidiOut.count > 0);
        CARLA_ASSERT(event != nullptr);
        CARLA_ASSERT(fIsProcessing);

        if (! fEnabled)
            return false;
        if (fMidiOut.count == 0)
            return false;
        if (event == nullptr)
            return false;
        if (! fIsProcessing)
        {
            carla_stderr2("NativePlugin::handleWriteMidiEvent(%p) - received MIDI out events outside audio thread, ignoring", event);
            return false;
        }

        if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
            return false;

        // reverse-find first free event, and put it there
        for (uint32_t i=fMidiEventCount-1; i >= fMidiEventCount; i--)
        {
            if (fMidiEvents[i].data[0] == 0)
            {
                std::memcpy(&fMidiEvents[i], event, sizeof(::MidiEvent));
                break;
            }
        }

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
        return sPluginDescriptors.count();
    }

    static const PluginDescriptor* getPluginDescriptor(const size_t index)
    {
        maybeFirstInit();
        CARLA_ASSERT(index < sPluginDescriptors.count());

        if (index < sPluginDescriptors.count())
            return sPluginDescriptors.getAt(index);

        return nullptr;
    }

    static void registerPlugin(const PluginDescriptor* desc)
    {
        sPluginDescriptors.append(desc);
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

# ifdef WANT_AUDIOFILE
        carla_register_native_plugin_audiofile();
# endif

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

        // FIXME - use itenerator when available
        for (size_t i=0; i < sPluginDescriptors.count(); i++)
        {
            fDescriptor = sPluginDescriptors.getAt(i);

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

    float**     fAudioInBuffers;
    float**     fAudioOutBuffers;
    uint32_t    fMidiEventCount;
    ::MidiEvent fMidiEvents[MAX_MIDI_EVENTS*2];

    NativePluginMidiData fMidiIn;
    NativePluginMidiData fMidiOut;

    ::TimeInfo fTimeInfo;

    static bool sFirstInit;
    static NonRtList<const PluginDescriptor*> sPluginDescriptors;

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

    static const ::TimeInfo* carla_host_get_time_info(HostHandle handle)
    {
        return handlePtr->handleGetTimeInfo();
    }

    static bool carla_host_write_midi_event(HostHandle handle, const ::MidiEvent* event)
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
NonRtList<const PluginDescriptor*> NativePlugin::sPluginDescriptors;

CARLA_BACKEND_END_NAMESPACE

void carla_register_native_plugin(const PluginDescriptor* desc)
{
    CARLA_BACKEND_USE_NAMESPACE
    NativePlugin::registerPlugin(desc);
}

#else // WANT_NATIVE
# warning Building without Internal plugin support
#endif

CARLA_BACKEND_START_NAMESPACE

size_t CarlaPlugin::getNativePluginCount()
{
#ifdef WANT_NATIVE
    return NativePlugin::getPluginCount();
#else
    return 0;
#endif
}

const PluginDescriptor* CarlaPlugin::getNativePluginDescriptor(const size_t index)
{
#ifdef WANT_NATIVE
    return NativePlugin::getPluginDescriptor(index);
#else
    return nullptr;
    // unused
    (void)index;
#endif
}

// -----------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newNative(const Initializer& init)
{
    carla_debug("CarlaPlugin::newNative(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

#ifdef WANT_NATIVE
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

    return plugin;
#else
    init.engine->setLastError("Internal plugins not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
