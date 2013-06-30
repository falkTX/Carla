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

#include "CarlaNative.h"

#include <QtCore/Qt>

# if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QFileDialog>
#else
# include <QtGui/QFileDialog>
#endif

void carla_register_all_plugins()
{
    // Simple plugins
    carla_register_native_plugin_bypass();
    carla_register_native_plugin_lfo();
    //carla_register_native_plugin_midiSequencer(); // unfinished
    carla_register_native_plugin_midiSplit();
    carla_register_native_plugin_midiThrough();
    carla_register_native_plugin_midiTranspose();
    carla_register_native_plugin_nekofilter();
    //carla_register_native_plugin_sunvoxfile(); // unfinished

#ifndef BUILD_BRIDGE
    // Carla
    //carla_register_native_plugin_carla(); // kinda unfinished
#endif

#ifdef WANT_AUDIOFILE
    // AudioFile
    carla_register_native_plugin_audiofile();
#endif

#ifdef WANT_MIDIFILE
    // MidiFile
    carla_register_native_plugin_midifile();
#endif

#ifdef WANT_OPENGL
    // DISTRHO plugins (OpenGL)
    carla_register_native_plugin_3BandEQ();
    carla_register_native_plugin_3BandSplitter();
    carla_register_native_plugin_Nekobi();
    carla_register_native_plugin_PingPongPan();
    //carla_register_native_plugin_StereoEnhancer(); // unfinished
#endif

    // DISTRHO plugins (Qt)
    //carla_register_native_plugin_Notes(); // unfinished

#ifdef WANT_ZYNADDSUBFX
    // ZynAddSubFX
    carla_register_native_plugin_zynaddsubfx();
#endif
}

CARLA_BACKEND_START_NAMESPACE

#if 0
}
#endif

struct NativePluginMidiData {
    uint32_t  count;
    uint32_t* indexes;
    CarlaEngineEventPort** ports;

    NativePluginMidiData()
        : count(0),
          indexes(nullptr),
          ports(nullptr) {}

    ~NativePluginMidiData()
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(ports == nullptr);
        CARLA_ASSERT(indexes == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_ASSERT_INT(count == 0, count);
        CARLA_ASSERT(ports == nullptr);
        CARLA_ASSERT(indexes == nullptr);
        CARLA_ASSERT_INT(newCount > 0, newCount);

        if (ports != nullptr || indexes != nullptr || newCount == 0)
            return;

        ports   = new CarlaEngineEventPort*[newCount];
        indexes = new uint32_t[newCount];
        count   = newCount;

        for (uint32_t i=0; i < newCount; ++i)
            ports[i] = nullptr;

        for (uint32_t i=0; i < newCount; ++i)
            indexes[i] = 0;
    }

    void clear()
    {
        if (ports != nullptr)
        {
            for (uint32_t i=0; i < count; ++i)
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
        for (uint32_t i=0; i < count; ++i)
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
          fIsUiVisible(false),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fMidiEventCount(0),
          fCurMidiProgs{0}
#else
          fMidiEventCount(0)
#endif
    {
        carla_debug("NativePlugin::NativePlugin(%p, %i)", engine, id);

#ifndef CARLA_PROPER_CPP11_SUPPORT
        carla_fill<int32_t>(fCurMidiProgs, MAX_MIDI_CHANNELS, 0);
#endif

        carla_zeroStruct< ::MidiEvent>(fMidiEvents, MAX_MIDI_EVENTS*2);

        fHost.handle       = this;
        fHost.resource_dir = carla_strdup((const char*)engine->getOptions().resourceDir);
        fHost.ui_name      = nullptr;

        fHost.get_buffer_size        = carla_host_get_buffer_size;
        fHost.get_sample_rate        = carla_host_get_sample_rate;
        fHost.get_time_info          = carla_host_get_time_info;
        fHost.write_midi_event       = carla_host_write_midi_event;
        fHost.ui_parameter_changed   = carla_host_ui_parameter_changed;
        fHost.ui_custom_data_changed = carla_host_ui_custom_data_changed;
        fHost.ui_closed              = carla_host_ui_closed;
        fHost.ui_open_file           = carla_host_ui_open_file;
        fHost.ui_save_file           = carla_host_ui_save_file;
        fHost.dispatcher             = carla_host_dispatcher;
    }

    ~NativePlugin() override
    {
        carla_debug("NativePlugin::~NativePlugin()");

        // close UI
        if (fHints & PLUGIN_HAS_GUI)
        {
            if (fIsUiVisible && fDescriptor != nullptr && fDescriptor->ui_show != nullptr && fHandle != nullptr)
                fDescriptor->ui_show(fHandle, false);
        }

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        if (kData->client != nullptr && kData->client->isActive())
            kData->client->deactivate();

        CARLA_ASSERT(! fIsProcessing);

        if (kData->active)
        {
            deactivate();
            kData->active = false;
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

        if (fHost.resource_dir != nullptr)
        {
            delete[] fHost.resource_dir;
            fHost.resource_dir = nullptr;
        }

        if (fHost.ui_name != nullptr)
        {
            delete[] fHost.ui_name;
            fHost.ui_name = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const override
    {
        return PLUGIN_INTERNAL;
    }

    PluginCategory category() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        return static_cast<PluginCategory>(fDescriptor->category);
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount() const override
    {
        return fMidiIn.count;
    }

    uint32_t midiOutCount() const override
    {
        return fMidiOut.count;
    }

    uint32_t parameterScalePointCount(const uint32_t parameterId) const override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor->get_parameter_info != nullptr && parameterId < kData->param.count)
        {
            if (const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
                return param->scalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    unsigned int availableOptions() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor == nullptr)
            return 0x0;

        unsigned int options = 0x0;

        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        if (midiInCount() == 0)
            options |= PLUGIN_OPTION_FIXED_BUFFER;

        if (kData->engine->getProccessMode() != PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (fOptions & PLUGIN_OPTION_FORCE_STEREO)
                options |= PLUGIN_OPTION_FORCE_STEREO;
            else if (kData->audioIn.count <= 1 && kData->audioOut.count <= 1 && (kData->audioIn.count != 0 || kData->audioOut.count != 0))
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        if (fDescriptor->midiIns > 0)
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    float getParameterValue(const uint32_t parameterId) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor->get_parameter_value != nullptr && parameterId < kData->param.count)
            return fDescriptor->get_parameter_value(fHandle, parameterId);

        return 0.0f;
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        if (fDescriptor->get_parameter_info != nullptr && parameterId < kData->param.count)
        {
            if (const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
            {
                const ParameterScalePoint& scalePoint(param->scalePoints[scalePointId]);
                return scalePoint.value;
            }
        }

        return 0.0f;
    }

    void getLabel(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->label != nullptr)
            std::strncpy(strBuf, fDescriptor->label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->maker != nullptr)
            std::strncpy(strBuf, fDescriptor->maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->copyright != nullptr)
            std::strncpy(strBuf, fDescriptor->copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);

        if (fDescriptor->name != nullptr)
            std::strncpy(strBuf, fDescriptor->name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor->get_parameter_info != nullptr && parameterId < kData->param.count)
        {
            const Parameter* const param(fDescriptor->get_parameter_info(fHandle, parameterId));

            if (param != nullptr && param->name != nullptr)
            {
                std::strncpy(strBuf, param->name, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor->get_parameter_text != nullptr && parameterId < kData->param.count)
        {
            if (const char* const text = fDescriptor->get_parameter_text(fHandle, parameterId))
            {
                std::strncpy(strBuf, text, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterText(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        if (fDescriptor->get_parameter_info != nullptr && parameterId < kData->param.count)
        {
            const Parameter* const param(fDescriptor->get_parameter_info(fHandle, parameterId));

            if (param != nullptr && param->unit != nullptr)
            {
                std::strncpy(strBuf, param->unit, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);
        CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        if (fDescriptor->get_parameter_info != nullptr && parameterId < kData->param.count)
        {
            if (const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
            {
                const ParameterScalePoint& scalePoint(param->scalePoints[scalePointId]);

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
    // Set data (state)

    void prepareForSave() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (kData->midiprog.count > 0 && (fHints & PLUGIN_IS_SYNTH) != 0)
        {
            char strBuf[STR_MAX+1];
            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i",
                          fCurMidiProgs[0],  fCurMidiProgs[1],  fCurMidiProgs[2],  fCurMidiProgs[3],
                          fCurMidiProgs[4],  fCurMidiProgs[5],  fCurMidiProgs[6],  fCurMidiProgs[7],
                          fCurMidiProgs[8],  fCurMidiProgs[9],  fCurMidiProgs[10], fCurMidiProgs[11],
                          fCurMidiProgs[12], fCurMidiProgs[13], fCurMidiProgs[14], fCurMidiProgs[15]);

            CarlaPlugin::setCustomData(CUSTOM_DATA_STRING, "midiPrograms", strBuf, false);
        }

        if (fDescriptor->get_state == nullptr || (fDescriptor->hints & ::PLUGIN_USES_STATE) == 0)
            return;

        if (char* data = fDescriptor->get_state(fHandle))
        {
            CarlaPlugin::setCustomData(CUSTOM_DATA_CHUNK, "State", data, false);
            std::free(data);
        }
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        char uiName[std::strlen(newName)+6+1];
        std::strcpy(uiName, newName);
        std::strcat(uiName, " (GUI)");

        if (fHost.ui_name != nullptr)
            delete[] fHost.ui_name;

        fHost.ui_name = carla_strdup(uiName);

        // TODO - send callback to plugin, reporting name change

        CarlaPlugin::setName(newName);
    }

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) override
    {
        if (channel < MAX_MIDI_CHANNELS && kData->midiprog.count > 0)
            kData->midiprog.current = fCurMidiProgs[channel];

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(parameterId < kData->param.count);

        const float fixedValue(kData->param.fixValue(parameterId, value));

        if (fDescriptor->set_parameter_value != nullptr && parameterId < kData->param.count)
        {
            fDescriptor->set_parameter_value(fHandle, parameterId, fixedValue);

            if (fHandle2 != nullptr)
                fDescriptor->set_parameter_value(fHandle2, parameterId, fixedValue);
        }

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(type != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);
        carla_debug("NativePlugin::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (type == nullptr)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is null", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_STRING) != 0 && std::strcmp(type, CUSTOM_DATA_CHUNK) != 0)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (key == nullptr)
            return carla_stderr2("NativePlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

        if (value == nullptr)
            return carla_stderr2("Nativelugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_CHUNK) == 0)
        {
            if (fDescriptor->set_state != nullptr && (fDescriptor->hints & ::PLUGIN_USES_STATE) != 0)
            {
                const ScopedSingleProcessLocker spl(this, true);

                fDescriptor->set_state(fHandle, value);

                if (fHandle2 != nullptr)
                    fDescriptor->set_state(fHandle2, value);
            }
        }
        else if (std::strcmp(key, "midiPrograms") == 0 && fDescriptor->set_midi_program != nullptr)
        {
            QStringList midiProgramList(QString(value).split(":", QString::SkipEmptyParts));

            if (midiProgramList.count() == MAX_MIDI_CHANNELS)
            {
                uint i = 0;
                foreach (const QString& midiProg, midiProgramList)
                {
                    bool ok;
                    uint index = midiProg.toUInt(&ok);

                    if (ok && index < kData->midiprog.count)
                    {
                        const uint32_t bank    = kData->midiprog.data[index].bank;
                        const uint32_t program = kData->midiprog.data[index].program;

                        fDescriptor->set_midi_program(fHandle, i, bank, program);

                        if (fHandle2 != nullptr)
                            fDescriptor->set_midi_program(fHandle2, i, bank, program);

                        fCurMidiProgs[i] = index;

                        if (kData->ctrlChannel == static_cast<int32_t>(i))
                        {
                            kData->midiprog.current = index;
                            kData->engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, fId, index, 0, 0.0f, nullptr);
                        }
                    }

                    ++i;
                }
            }
        }
        else
        {
            if (fDescriptor->set_custom_data != nullptr)
            {
                fDescriptor->set_custom_data(fHandle, key, value);

                if (fHandle2 != nullptr)
                    fDescriptor->set_custom_data(fHandle2, key, value);
            }

            if (sendGui && fIsUiVisible && fDescriptor->ui_set_custom_data != nullptr)
                fDescriptor->ui_set_custom_data(fHandle, key, value);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index >= -1 && index < static_cast<int32_t>(kData->midiprog.count));

        if (index < -1)
            index = -1;
        else if (index > static_cast<int32_t>(kData->midiprog.count))
            return;

        if ((fHints & PLUGIN_IS_SYNTH) != 0 && (kData->ctrlChannel < 0 || kData->ctrlChannel >= MAX_MIDI_CHANNELS))
            return;

        if (index >= 0)
        {
            const uint8_t  channel = (kData->ctrlChannel >= 0 || kData->ctrlChannel < MAX_MIDI_CHANNELS) ? kData->ctrlChannel : 0;
            const uint32_t bank    = kData->midiprog.data[index].bank;
            const uint32_t program = kData->midiprog.data[index].program;

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            fDescriptor->set_midi_program(fHandle, channel, bank, program);

            if (fHandle2 != nullptr)
                fDescriptor->set_midi_program(fHandle2, channel, bank, program);

            fCurMidiProgs[channel] = index;
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor->ui_show == nullptr)
            return;

        fDescriptor->ui_show(fHandle, yesNo);
        fIsUiVisible = yesNo;

        if (yesNo)
        {
            // Update UI values, FIXME? (remove?)
            if (fDescriptor->ui_set_custom_data != nullptr)
            {
                for (NonRtList<CustomData>::Itenerator it = kData->custom.begin(); it.valid(); it.next())
                {
                    const CustomData& cData(*it);

                    if (std::strcmp(cData.type, CUSTOM_DATA_STRING) == 0)
                        fDescriptor->ui_set_custom_data(fHandle, cData.key, cData.value);
                }
            }

            if (fDescriptor->ui_set_midi_program != nullptr && kData->midiprog.current >= 0)
            {
                const MidiProgramData& mpData(kData->midiprog.getCurrent());
                fDescriptor->ui_set_midi_program(fHandle, 0, mpData.bank, mpData.program); // TODO
            }

            if (fDescriptor->ui_set_parameter_value != nullptr)
            {
                for (uint32_t i=0; i < kData->param.count; ++i)
                    fDescriptor->ui_set_parameter_value(fHandle, i, fDescriptor->get_parameter_value(fHandle, i));
            }
        }
    }

    void idleGui() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fIsUiVisible && fDescriptor->ui_idle != nullptr)
            fDescriptor->ui_idle(fHandle);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        carla_debug("NativePlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (kData->engine == nullptr)
            return;
        if (fDescriptor == nullptr)
            return;
        if (fHandle == nullptr)
            return;

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (kData->active)
            deactivate();

        clearBuffers();

        const float sampleRate = (float)kData->engine->getSampleRate();

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
            kData->audioIn.createNew(aIns);
            fAudioInBuffers = new float*[aIns];

            for (uint32_t i=0; i < aIns; ++i)
                fAudioInBuffers[i] = nullptr;
        }

        if (aOuts > 0)
        {
            kData->audioOut.createNew(aOuts);
            fAudioOutBuffers = new float*[aOuts];
            needsCtrlIn = true;

            for (uint32_t i=0; i < aOuts; ++i)
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

        const uint portNameSize(kData->engine->maxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            if (aIns > 1 && ! forcedStereoIn)
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
        for (j=0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            if (aOuts > 1 && ! forcedStereoOut)
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
            for (j=0; j < mIns; ++j)
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
            for (j=0; j < mOuts; ++j)
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

        for (j=0; j < params; ++j)
        {
            const ::Parameter* const paramInfo(fDescriptor->get_parameter_info(fHandle, j));

            CARLA_ASSERT(paramInfo != nullptr);

            if (paramInfo == nullptr)
                continue;

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

            portName += "events-in";
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

            portName += "events-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        if (forcedStereoIn || forcedStereoOut)
            fOptions |= PLUGIN_OPTION_FORCE_STEREO;
        else
            fOptions &= ~PLUGIN_OPTION_FORCE_STEREO;

        // plugin hints
        fHints = 0x0;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            fHints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            fHints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            fHints |= PLUGIN_CAN_BALANCE;

        // native plugin hints
        if (fDescriptor->hints & ::PLUGIN_IS_RTSAFE)
            fHints |= PLUGIN_IS_RTSAFE;
        if (fDescriptor->hints & ::PLUGIN_IS_SYNTH)
            fHints |= PLUGIN_IS_SYNTH;
        if (fDescriptor->hints & ::PLUGIN_HAS_GUI)
            fHints |= PLUGIN_HAS_GUI;
        if (fDescriptor->hints & ::PLUGIN_USES_GUI_AS_FILE)
            fHints |= PLUGIN_HAS_GUI_AS_FILE;
        if (fDescriptor->hints & ::PLUGIN_USES_SINGLE_THREAD)
            fHints |= PLUGIN_HAS_SINGLE_THREAD;

        // extra plugin hints
        kData->extraHints = 0x0;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && mIns <= 1 && mOuts <= 1)
            kData->extraHints |= PLUGIN_HINT_CAN_RUN_RACK;

        bufferSizeChanged(kData->engine->getBufferSize());
        reloadPrograms(true);

        if (kData->active)
            activate();

        carla_debug("NativePlugin::reload() - end");
    }

    void reloadPrograms(const bool init) override
    {
        carla_debug("NativePlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount  = kData->midiprog.count;
        const int32_t current = kData->midiprog.current;

        // Delete old programs
        kData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fDescriptor->get_midi_program_count != nullptr && fDescriptor->get_midi_program_info != nullptr && fDescriptor->set_midi_program != nullptr)
            count = fDescriptor->get_midi_program_count(fHandle);

        if (count > 0)
        {
            kData->midiprog.createNew(count);

            // Update data
            for (i=0; i < count; ++i)
            {
                const ::MidiProgram* const mpDesc = fDescriptor->get_midi_program_info(fHandle, i);
                CARLA_ASSERT(mpDesc != nullptr);
                CARLA_ASSERT(mpDesc->name != nullptr);

                kData->midiprog.data[i].bank    = mpDesc->bank;
                kData->midiprog.data[i].program = mpDesc->program;
                kData->midiprog.data[i].name    = carla_strdup(mpDesc->name);
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        if (kData->engine->isOscControlRegistered())
        {
            kData->engine->osc_send_control_set_midi_program_count(fId, count);

            for (i=0; i < count; ++i)
                kData->engine->osc_send_control_set_midi_program_data(fId, i, kData->midiprog.data[i].bank, kData->midiprog.data[i].program, kData->midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (count > 0)
                setMidiProgram(0, false, false, false);
        }
        else
        {
            // Check if current program is invalid
            bool programChanged = false;

            if (count == oldCount+1)
            {
                // one midi program added, probably created by user
                kData->midiprog.current = oldCount;
                programChanged = true;
            }
            else if (current < 0 && count > 0)
            {
                // programs exist now, but not before
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && count == 0)
            {
                // programs existed before, but not anymore
                kData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(count))
            {
                // current midi program > count
                kData->midiprog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                kData->midiprog.current = current;
            }

            if (programChanged)
                setMidiProgram(kData->midiprog.current, true, true, true);

            kData->engine->callback(CALLBACK_RELOAD_PROGRAMS, fId, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor->activate != nullptr)
        {
            fDescriptor->activate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->activate(fHandle2);
        }
    }

    void deactivate() override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);

        if (fDescriptor->deactivate != nullptr)
        {
            fDescriptor->deactivate(fHandle);

            if (fHandle2 != nullptr)
                fDescriptor->deactivate(fHandle2);
        }
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; ++i)
                carla_zeroFloat(outBuffer[i], frames);

            return;
        }

        fMidiEventCount = 0;
        carla_zeroStruct< ::MidiEvent>(fMidiEvents, MAX_MIDI_EVENTS*2);

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (kData->needsReset)
        {
            if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (k=0, i=MAX_MIDI_CHANNELS; k < MAX_MIDI_CHANNELS; ++k)
                {
                    fMidiEvents[k].data[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                    fMidiEvents[k].data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                    fMidiEvents[k].data[2] = 0;
                    fMidiEvents[k].size    = 3;

                    fMidiEvents[k+i].data[0] = MIDI_STATUS_CONTROL_CHANGE + k;
                    fMidiEvents[k+i].data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                    fMidiEvents[k+i].data[2] = 0;
                    fMidiEvents[k+i].size    = 3;
                }

                fMidiEventCount = MAX_MIDI_CHANNELS*2;
            }
            else if (kData->ctrlChannel >= 0 && kData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                for (k=0; k < MAX_MIDI_NOTE; ++k)
                {
                    fMidiEvents[k].data[0] = MIDI_STATUS_NOTE_OFF + kData->ctrlChannel;
                    fMidiEvents[k].data[1] = k;
                    fMidiEvents[k].data[2] = 0;
                    fMidiEvents[k].size    = 3;
                }

                fMidiEventCount = MAX_MIDI_NOTE;
            }

            kData->needsReset = false;
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo& timeInfo(kData->engine->getTimeInfo());

        fTimeInfo.playing = timeInfo.playing;
        fTimeInfo.frame   = timeInfo.frame;
        fTimeInfo.usecs   = timeInfo.usecs;

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

        if (kData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (kData->extNotes.mutex.tryLock())
            {
                while (fMidiEventCount < MAX_MIDI_EVENTS*2 && ! kData->extNotes.data.isEmpty())
                {
                    const ExternalMidiNote& note(kData->extNotes.data.getFirst(true));

                    CARLA_ASSERT(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    fMidiEvents[fMidiEventCount].data[0]  = (note.velo > 0) ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    fMidiEvents[fMidiEventCount].data[0] += note.channel;
                    fMidiEvents[fMidiEventCount].data[1]  = note.note;
                    fMidiEvents[fMidiEventCount].data[2]  = note.velo;
                    fMidiEvents[fMidiEventCount].size     = 3;

                    fMidiEventCount += 1;
                }

                kData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

            bool allNotesOffSent = false;
            bool sampleAccurate  = (fOptions & PLUGIN_OPTION_FIXED_BUFFER) == 0;

            uint32_t time, nEvents = kData->event.portIn->getEventCount();
            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId = 0;

            if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                nextBankId = kData->midiprog.data[kData->midiprog.current].bank;

            for (i=0; i < nEvents; ++i)
            {
                const EngineEvent& event(kData->event.portIn->getEvent(i));

                time = event.time;

                if (time >= frames)
                    continue;

                CARLA_ASSERT_INT2(time >= timeOffset, time, timeOffset);

                if (time > timeOffset && sampleAccurate)
                {
                    if (processSingle(inBuffer, outBuffer, time - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = time;

                        if (kData->midiprog.current >= 0 && kData->midiprog.count > 0)
                            nextBankId = kData->midiprog.data[kData->midiprog.current].bank;
                        else
                            nextBankId = 0;

                        if (fMidiEventCount > 0)
                        {
                            carla_zeroStruct< ::MidiEvent>(fMidiEvents, fMidiEventCount);
                            fMidiEventCount = 0;
                        }
                    }
                    else
                        startTime += timeOffset;
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
                        if (event.channel == kData->ctrlChannel)
                        {
                            float value;

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (fHints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (fHints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
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
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                                postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
                            }
                        }
#endif

                        // Control plugin parameters
                        for (k=0; k < kData->param.count; ++k)
                        {
                            if (kData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (kData->param.data[k].midiCC != ctrlEvent.param)
                                continue;
                            if (kData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((kData->param.data[k].hints & PARAMETER_IS_AUTOMABLE) == 0)
                                continue;

                            float value;

                            if (kData->param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = (ctrlEvent.value < 0.5f) ? kData->param.ranges[k].min : kData->param.ranges[k].max;
                            }
                            else
                            {
                                value = kData->param.ranges[k].unnormalizeValue(ctrlEvent.value);

                                if (kData->param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = std::rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeRtEvent(kPluginPostRtEventParameterChange, static_cast<int32_t>(k), 0, value);
                        }

                        if ((fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param <= 0x5F)
                        {
                            if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
                                continue;

                            fMidiEvents[fMidiEventCount].port = 0;
                            fMidiEvents[fMidiEventCount].time = sampleAccurate ? startTime : time;
                            fMidiEvents[fMidiEventCount].data[0] = MIDI_STATUS_CONTROL_CHANGE + event.channel;
                            fMidiEvents[fMidiEventCount].data[1] = ctrlEvent.param;
                            fMidiEvents[fMidiEventCount].data[2] = ctrlEvent.value*127.0f;
                            fMidiEvents[fMidiEventCount].size    = 3;

                            fMidiEventCount += 1;
                        }

                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (event.channel == kData->ctrlChannel && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                            nextBankId = ctrlEvent.param;
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel < MAX_MIDI_CHANNELS && (fOptions & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            const uint32_t nextProgramId(ctrlEvent.param);

                            for (k=0; k < kData->midiprog.count; ++k)
                            {
                                if (kData->midiprog.data[k].bank == nextBankId && kData->midiprog.data[k].program == nextProgramId)
                                {
                                    fDescriptor->set_midi_program(fHandle, event.channel, nextBankId, nextProgramId);

                                    if (fHandle2 != nullptr)
                                        fDescriptor->set_midi_program(fHandle2, event.channel, nextBankId, nextProgramId);

                                    fCurMidiProgs[event.channel] = k;

                                    if (event.channel == kData->ctrlChannel)
                                        postponeRtEvent(kPluginPostRtEventMidiProgramChange, k, 0, 0.0f);

                                    break;
                                }
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
                                continue;

                            fMidiEvents[fMidiEventCount].port = 0;
                            fMidiEvents[fMidiEventCount].time = sampleAccurate ? startTime : time;
                            fMidiEvents[fMidiEventCount].data[0] = MIDI_STATUS_CONTROL_CHANGE + event.channel;
                            fMidiEvents[fMidiEventCount].data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            fMidiEvents[fMidiEventCount].data[2] = 0;
                            fMidiEvents[fMidiEventCount].size    = 3;

                            fMidiEventCount += 1;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (fOptions & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (event.channel == kData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }

                            if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
                                continue;

                            fMidiEvents[fMidiEventCount].port = 0;
                            fMidiEvents[fMidiEventCount].time = sampleAccurate ? startTime : time;
                            fMidiEvents[fMidiEventCount].data[0] = MIDI_STATUS_CONTROL_CHANGE + event.channel;
                            fMidiEvents[fMidiEventCount].data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            fMidiEvents[fMidiEventCount].data[2] = 0;
                            fMidiEvents[fMidiEventCount].size    = 3;

                            fMidiEventCount += 1;
                        }
                        break;
                    }

                    break;
                }

                case kEngineEventTypeMidi:
                {
                    if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
                        continue;

                    const EngineMidiEvent& midiEvent(event.midi);

                    uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                    uint8_t channel = event.channel;

                    if (MIDI_IS_STATUS_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (MIDI_IS_STATUS_CONTROL_CHANGE(status) && (fOptions & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) && (fOptions & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status) && (fOptions & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status -= 0x10;

                    fMidiEvents[fMidiEventCount].port = 0;
                    fMidiEvents[fMidiEventCount].time = sampleAccurate ? startTime : time;
                    fMidiEvents[fMidiEventCount].size = midiEvent.size;

                    fMidiEvents[fMidiEventCount].data[0] = status + channel;
                    fMidiEvents[fMidiEventCount].data[1] = midiEvent.data[1];
                    fMidiEvents[fMidiEventCount].data[2] = midiEvent.data[2];
                    fMidiEvents[fMidiEventCount].data[3] = midiEvent.data[3];

                    fMidiEventCount += 1;

                    if (status == MIDI_STATUS_NOTE_ON)
                        postponeRtEvent(kPluginPostRtEventNoteOn, channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        postponeRtEvent(kPluginPostRtEventNoteOff, channel, midiEvent.data[1], 0.0f);

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
        // Control and MIDI Output

        if (fMidiOut.count > 0 || kData->event.portOut != nullptr)
        {
            float value, curValue;

            for (k=0; k < kData->param.count; ++k)
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

            // reverse lookup MIDI events
            for (k = (MAX_MIDI_EVENTS*2)-1; k >= fMidiEventCount; --k)
            {
                if (fMidiEvents[k].data[0] == 0)
                    break;

                const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(fMidiEvents[k].data);
                const uint8_t port    = fMidiEvents[k].port;

                if (kData->event.portOut != nullptr)
                    kData->event.portOut->writeMidiEvent(fMidiEvents[k].time, channel, port, fMidiEvents[k].data, fMidiEvents[k].size);
                else if (port < fMidiOut.count)
                    fMidiOut.ports[port]->writeMidiEvent(fMidiEvents[k].time, channel, port, fMidiEvents[k].data, fMidiEvents[k].size);
            }

        } // End of Control and MIDI Output
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_ASSERT(frames > 0);

        if (frames == 0)
            return false;

        if (kData->audioIn.count > 0)
        {
            CARLA_ASSERT(inBuffer != nullptr);
            if (inBuffer == nullptr)
                return false;
        }
        if (kData->audioOut.count > 0)
        {
            CARLA_ASSERT(outBuffer != nullptr);
            if (outBuffer == nullptr)
                return false;
        }

        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (kData->engine->isOffline())
        {
            kData->singleMutex.lock();
        }
        else if (! kData->singleMutex.tryLock())
        {
            for (i=0; i < kData->audioOut.count; ++i)
            {
                for (k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Reset audio buffers

        for (i=0; i < kData->audioIn.count; ++i)
            carla_copyFloat(fAudioInBuffers[i], inBuffer[i]+timeOffset, frames);
        for (i=0; i < kData->audioOut.count; ++i)
            carla_zeroFloat(fAudioOutBuffers[i], frames);

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fIsProcessing = true;

        if (fHandle2 == nullptr)
        {
            fDescriptor->process(fHandle, fAudioInBuffers, fAudioOutBuffers, frames, fMidiEventCount, fMidiEvents);
        }
        else
        {
            fDescriptor->process(fHandle,
                                 (kData->audioIn.count > 0) ? &fAudioInBuffers[0] : nullptr,
                                 (kData->audioOut.count > 0) ? &fAudioOutBuffers[0] : nullptr,
                                 frames, fMidiEventCount, fMidiEvents);

            fDescriptor->process(fHandle2,
                                 (kData->audioIn.count > 0) ? &fAudioInBuffers[1] : nullptr,
                                 (kData->audioOut.count > 0) ? &fAudioOutBuffers[1] : nullptr,
                                 frames, fMidiEventCount, fMidiEvents);
        }

        fIsProcessing = false;
        fTimeInfo.frame += frames;

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (fHints & PLUGIN_CAN_DRYWET) != 0 && kData->postProc.dryWet != 1.0f;
            const bool doBalance = (fHints & PLUGIN_CAN_BALANCE) != 0 && (kData->postProc.balanceLeft != -1.0f || kData->postProc.balanceRight != 1.0f);

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (i=0; i < kData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (k=0; k < frames; ++k)
                    {
                        bufValue = fAudioInBuffers[(kData->audioIn.count == 1) ? 0 : i][k];
                        fAudioOutBuffers[i][k] = (fAudioOutBuffers[i][k] * kData->postProc.dryWet) + (bufValue * (1.0f - kData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < kData->audioOut.count);
                        carla_copyFloat(oldBufLeft, fAudioOutBuffers[i], frames);
                    }

                    float balRangeL = (kData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (kData->postProc.balanceRight + 1.0f)/2.0f;

                    for (k=0; k < frames; ++k)
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
                    for (k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k] * kData->postProc.volume;
                }
            }

        } // End of Post-processing
#else
        for (i=0; i < kData->audioOut.count; ++i)
        {
            for (k=0; k < frames; ++k)
                outBuffer[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        kData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("NativePlugin::bufferSizeChanged(%i)", newBufferSize);

        for (uint32_t i=0; i < kData->audioIn.count; ++i)
        {
            if (fAudioInBuffers[i] != nullptr)
                delete[] fAudioInBuffers[i];
            fAudioInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < kData->audioOut.count; ++i)
        {
            if (fAudioOutBuffers[i] != nullptr)
                delete[] fAudioOutBuffers[i];
            fAudioOutBuffers[i] = new float[newBufferSize];
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void initBuffers() override
    {
        fMidiIn.initBuffers(kData->engine);
        fMidiOut.initBuffers(kData->engine);

        CarlaPlugin::initBuffers();
    }

    void clearBuffers() override
    {
        carla_debug("NativePlugin::clearBuffers() - start");

        if (fAudioInBuffers != nullptr)
        {
            for (uint32_t i=0; i < kData->audioIn.count; ++i)
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
            for (uint32_t i=0; i < kData->audioOut.count; ++i)
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

        CarlaPlugin::clearBuffers();

        carla_debug("NativePlugin::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index < kData->param.count);

        if (! fIsUiVisible)
            return;
        if (fDescriptor == nullptr || fHandle == nullptr)
            return;
        if (index >= kData->param.count)
            return;

        if (fDescriptor->ui_set_parameter_value != nullptr)
            fDescriptor->ui_set_parameter_value(fHandle, index, value);
    }

    void uiMidiProgramChange(const uint32_t index) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(index < kData->midiprog.count);

        if (! fIsUiVisible)
            return;
        if (fDescriptor == nullptr || fHandle == nullptr)
            return;
        if (index >= kData->midiprog.count)
            return;

        if (fDescriptor->ui_set_midi_program != nullptr) // TODO
            fDescriptor->ui_set_midi_program(fHandle, 0, kData->midiprog.data[index].bank, kData->midiprog.data[index].program);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);
        CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);

        if (! fIsUiVisible)
            return;
        if (fDescriptor == nullptr || fHandle == nullptr)
            return;
        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (velo >= MAX_MIDI_VALUE)
            return;

        // TODO
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) override
    {
        CARLA_ASSERT(fDescriptor != nullptr);
        CARLA_ASSERT(fHandle != nullptr);
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);

        if (! fIsUiVisible)
            return;
        if (fDescriptor == nullptr || fHandle == nullptr)
            return;
        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;

        // TODO
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
        CARLA_ASSERT(fIsProcessing);
        CARLA_ASSERT(fMidiOut.count > 0 || kData->event.portOut != nullptr);
        CARLA_ASSERT(event != nullptr);
        CARLA_ASSERT(event->data[0] != 0);

        if (! fEnabled)
            return false;
        if (fMidiOut.count == 0)
            return false;
        if (event == nullptr)
            return false;
        if (event->data[0] == 0)
            return false;
        if (! fIsProcessing)
        {
            carla_stderr2("NativePlugin::handleWriteMidiEvent(%p) - received MIDI out event outside audio thread, ignoring", event);
            return false;
        }

        if (fMidiEventCount >= MAX_MIDI_EVENTS*2)
            return false;

        // reverse-find first free event, and put it there
        for (uint32_t i=(MAX_MIDI_EVENTS*2)-1; i >= fMidiEventCount; --i)
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
        fIsUiVisible = false;
    }

    const char* handleUiOpenFile(const bool isDir, const char* const title, const char* const filter)
    {
        static CarlaString retStr;
        QFileDialog::Options options(isDir ? QFileDialog::ShowDirsOnly : 0x0);

        retStr = QFileDialog::getOpenFileName(nullptr, title, "", filter, nullptr, options).toUtf8().constData();

        return retStr.isNotEmpty() ? (const char*)retStr : nullptr;
    }

    const char* handleUiSaveFile(const bool isDir, const char* const title, const char* const filter)
    {
        static CarlaString retStr;
        QFileDialog::Options options(isDir ? QFileDialog::ShowDirsOnly : 0x0);

        retStr = QFileDialog::getSaveFileName(nullptr, title, "", filter, nullptr, options).toUtf8().constData();

        return retStr.isNotEmpty() ? (const char*)retStr : nullptr;
    }

    intptr_t handleDispatcher(HostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr)
    {
        intptr_t ret = 0;

        switch (opcode)
        {
        case HOST_OPCODE_NULL:
            break;
        case HOST_OPCODE_SET_PROCESS_PRECISION:
            // TODO
            break;
        case HOST_OPCODE_UI_UNAVAILABLE:
            kData->engine->callback(CALLBACK_SHOW_GUI, fId, -1, 0, 0.0f, nullptr);
            break;
        }

        return ret;

        // unused for now
        (void)index;
        (void)value;
        (void)ptr;
    }

    // -------------------------------------------------------------------

public:
    static size_t getPluginCount()
    {
        return sPluginDescriptors.count();
    }

    static const PluginDescriptor* getPluginDescriptor(const size_t index)
    {
        CARLA_ASSERT(index < sPluginDescriptors.count());

        if (index < sPluginDescriptors.count())
            return sPluginDescriptors.getAt(index);

        return nullptr;
    }

    static void registerPlugin(const PluginDescriptor* desc)
    {
        sPluginDescriptors.append(desc);
    }

    // -------------------------------------------------------------------

    bool init(const char* const name, const char* const label)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(label != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (label == nullptr)
        {
            kData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        for (NonRtList<const PluginDescriptor*>::Itenerator it = sPluginDescriptors.begin(); it.valid(); it.next())
        {
            fDescriptor = *it;

            CARLA_ASSERT(fDescriptor != nullptr);

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
        // set icon

        if (std::strcmp(fDescriptor->label, "audiofile") == 0)
            fIconName = "file";
        else if (std::strcmp(fDescriptor->label, "midifile") == 0)
            fIconName = "file";
        else if (std::strcmp(fDescriptor->label, "sunvoxfile") == 0)
            fIconName = "file";

        else if (std::strcmp(fDescriptor->label, "3BandEQ") == 0)
            fIconName = "distrho";
        else if (std::strcmp(fDescriptor->label, "3BandSplitter") == 0)
            fIconName = "distrho";
        else if (std::strcmp(fDescriptor->label, "Nekobi") == 0)
            fIconName = "distrho";
        else if (std::strcmp(fDescriptor->label, "Notes") == 0)
            fIconName = "distrho";
        else if (std::strcmp(fDescriptor->label, "PingPongPan") == 0)
            fIconName = "distrho";
        else if (std::strcmp(fDescriptor->label, "StereoEnhancer") == 0)
            fIconName = "distrho";

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr)
            fName = kData->engine->getUniquePluginName(name);
        else if (fDescriptor->name != nullptr)
            fName = kData->engine->getUniquePluginName(fDescriptor->name);
        else
            fName = kData->engine->getUniquePluginName(label);

        {
            CARLA_ASSERT(fHost.ui_name == nullptr);

            char uiName[fName.length()+6+1];
            std::strcpy(uiName, (const char*)fName);
            std::strcat(uiName, " (GUI)");

            fHost.ui_name = carla_strdup(uiName);
        }

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

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            fOptions = 0x0;

            fOptions |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

            if (midiInCount() > 0)
                fOptions |= PLUGIN_OPTION_FIXED_BUFFER;

            if (kData->engine->getOptions().forceStereo)
                fOptions |= PLUGIN_OPTION_FORCE_STEREO;

            if (fDescriptor->midiIns > 0)
            {
                fOptions |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                fOptions |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                fOptions |= PLUGIN_OPTION_SEND_PITCHBEND;
                fOptions |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // load settings
            kData->idStr  = "Native/";
            kData->idStr += label;
            fOptions = kData->loadSettings(fOptions, availableOptions());

            // ignore settings, we need this anyway
            if (midiInCount() > 0)
                fOptions |= PLUGIN_OPTION_FIXED_BUFFER;
        }

        return true;
    }

    class ScopedInitializer
    {
    public:
        ScopedInitializer()
        {
            carla_register_all_plugins();
        }

        ~ScopedInitializer()
        {
            sPluginDescriptors.clear();
        }
    };

private:
    PluginHandle   fHandle;
    PluginHandle   fHandle2;
    HostDescriptor fHost;
    const PluginDescriptor* fDescriptor;

    bool fIsProcessing;
    bool fIsUiVisible;

    float**     fAudioInBuffers;
    float**     fAudioOutBuffers;
    uint32_t    fMidiEventCount;
    ::MidiEvent fMidiEvents[MAX_MIDI_EVENTS*2];

    int32_t fCurMidiProgs[MAX_MIDI_CHANNELS];

    NativePluginMidiData fMidiIn;
    NativePluginMidiData fMidiOut;

    ::TimeInfo fTimeInfo;

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

    static const char* carla_host_ui_open_file(HostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiOpenFile(isDir, title, filter);
    }

    static const char* carla_host_ui_save_file(HostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiSaveFile(isDir, title, filter);
    }

    static intptr_t carla_host_dispatcher(HostHandle handle, HostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr)
    {
        return handlePtr->handleDispatcher(opcode, index, value, ptr);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePlugin)
};

NonRtList<const PluginDescriptor*> NativePlugin::sPluginDescriptors;

static const NativePlugin::ScopedInitializer _si;

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

// -----------------------------------------------------------------------

#ifdef WANT_NATIVE
size_t CarlaPlugin::getNativePluginCount()
{
    return NativePlugin::getPluginCount();
}

const PluginDescriptor* CarlaPlugin::getNativePluginDescriptor(const size_t index)
{
    return NativePlugin::getPluginDescriptor(index);
}
#endif

// -----------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newNative(const Initializer& init)
{
    carla_debug("CarlaPlugin::newNative({%p, \"%s\", \"%s\", \"%s\"})", init.engine, init.filename, init.name, init.label);

#ifdef WANT_NATIVE
    NativePlugin* const plugin(new NativePlugin(init.engine, init.id));

    if (! plugin->init(init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    if (init.engine->getProccessMode() == PROCESS_MODE_CONTINUOUS_RACK && ! CarlaPluginProtectedData::canRunInRack(plugin))
    {
        init.engine->setLastError("Carla's rack mode can only work with Mono or Stereo Internal plugins, sorry!");
        delete plugin;
        return nullptr;
    }

    return plugin;
#else
    init.engine->setLastError("Internal plugins support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
