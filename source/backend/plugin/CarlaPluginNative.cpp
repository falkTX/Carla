/*
 * Carla Native Plugin
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaMathUtils.hpp"
#include "CarlaNative.h"

#include "juce_core.h"

using juce::jmax;
using juce::String;
using juce::StringArray;

CARLA_EXTERN_C
std::size_t carla_getNativePluginCount() noexcept;

CARLA_EXTERN_C
const NativePluginDescriptor* carla_getNativePluginDescriptor(const std::size_t index) noexcept;

// -----------------------------------------------------------------------

static LinkedList<const NativePluginDescriptor*> gPluginDescriptors;

void carla_register_native_plugin(const NativePluginDescriptor* desc)
{
    gPluginDescriptors.append(desc);
}

// -----------------------------------------------------------------------

static
class NativePluginInitializer
{
public:
    NativePluginInitializer() noexcept
        : fNeedsInit(true) {}

    ~NativePluginInitializer() noexcept
    {
        gPluginDescriptors.clear();
    }

    void initIfNeeded() noexcept
    {
        if (! fNeedsInit)
            return;

        fNeedsInit = false;

        try {
            carla_register_all_native_plugins();
        } CARLA_SAFE_EXCEPTION("carla_register_all_native_plugins")
    }

private:
    bool fNeedsInit;

} sPluginInitializer;

// -----------------------------------------------------------------------

std::size_t carla_getNativePluginCount() noexcept
{
    sPluginInitializer.initIfNeeded();
    return gPluginDescriptors.count();
}

const NativePluginDescriptor* carla_getNativePluginDescriptor(const std::size_t index) noexcept
{
    sPluginInitializer.initIfNeeded();
    return gPluginDescriptors.getAt(index, nullptr);
}

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

struct NativePluginMidiData {
    uint32_t  count;
    uint32_t* indexes;
    CarlaEngineEventPort** ports;

    NativePluginMidiData() noexcept
        : count(0),
          indexes(nullptr),
          ports(nullptr) {}

    ~NativePluginMidiData() noexcept
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT(indexes == nullptr);
        CARLA_SAFE_ASSERT(ports == nullptr);
    }

    void createNew(const uint32_t newCount)
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT_RETURN(indexes == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(ports == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(newCount > 0,);

        indexes = new uint32_t[newCount];
        ports   = new CarlaEngineEventPort*[newCount];
        count   = newCount;

        for (uint32_t i=0; i < newCount; ++i)
            indexes[i] = 0;

        for (uint32_t i=0; i < newCount; ++i)
            ports[i] = nullptr;
    }

    void clear() noexcept
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

    void initBuffers() const noexcept
    {
        for (uint32_t i=0; i < count; ++i)
        {
            if (ports[i] != nullptr)
                ports[i]->initBuffer();
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(NativePluginMidiData)
};

// -----------------------------------------------------

class CarlaPluginNative : public CarlaPlugin
{
public:
    CarlaPluginNative(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          fHandle(nullptr),
          fHandle2(nullptr),
          fHost(),
          fDescriptor(nullptr),
          fIsProcessing(false),
          fIsOffline(false),
          fIsUiAvailable(false),
          fIsUiVisible(false),
          fAudioInBuffers(nullptr),
          fAudioOutBuffers(nullptr),
          fMidiEventCount(0),
          fCurBufferSize(engine->getBufferSize()),
          fCurSampleRate(engine->getSampleRate()),
          fMidiIn(),
          fMidiOut(),
          fTimeInfo()
    {
        carla_debug("CarlaPluginNative::CarlaPluginNative(%p, %i)", engine, id);

        carla_fill(fCurMidiProgs, 0, MAX_MIDI_CHANNELS);
        carla_zeroStructs(fMidiEvents, kPluginMaxMidiEvents*2);
        carla_zeroStruct(fTimeInfo);

        fHost.handle      = this;
        fHost.resourceDir = carla_strdup(engine->getOptions().resourceDir);
        fHost.uiName      = nullptr;
        fHost.uiParentId  = engine->getOptions().frontendWinId;

        fHost.get_buffer_size        = carla_host_get_buffer_size;
        fHost.get_sample_rate        = carla_host_get_sample_rate;
        fHost.is_offline             = carla_host_is_offline;
        fHost.get_time_info          = carla_host_get_time_info;
        fHost.write_midi_event       = carla_host_write_midi_event;
        fHost.ui_parameter_changed   = carla_host_ui_parameter_changed;
        fHost.ui_custom_data_changed = carla_host_ui_custom_data_changed;
        fHost.ui_closed              = carla_host_ui_closed;
        fHost.ui_open_file           = carla_host_ui_open_file;
        fHost.ui_save_file           = carla_host_ui_save_file;
        fHost.dispatcher             = carla_host_dispatcher;
    }

    ~CarlaPluginNative() override
    {
        carla_debug("CarlaPluginNative::~CarlaPluginNative()");

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            if (fIsUiVisible && fDescriptor != nullptr && fDescriptor->ui_show != nullptr && fHandle != nullptr)
                fDescriptor->ui_show(fHandle, false);

            pData->transientTryCounter = 0;
        }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        CARLA_ASSERT(! fIsProcessing);

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

        if (fHost.resourceDir != nullptr)
        {
            delete[] fHost.resourceDir;
            fHost.resourceDir = nullptr;
        }

        if (fHost.uiName != nullptr)
        {
            delete[] fHost.uiName;
            fHost.uiName = nullptr;
        }

        clearBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_INTERNAL;
    }

    PluginCategory getCategory() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, PLUGIN_CATEGORY_NONE);

        return static_cast<PluginCategory>(fDescriptor->category);
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t getMidiInCount() const noexcept override
    {
        return fMidiIn.count;
    }

    uint32_t getMidiOutCount() const noexcept override
    {
        return fMidiOut.count;
    }

    uint32_t getParameterScalePointCount(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
            return param->scalePointCount;

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        return 0;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0x0);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, 0);

        // FIXME - try
        const bool hasMidiProgs(fDescriptor->get_midi_program_count != nullptr && fDescriptor->get_midi_program_count(fHandle) > 0);

        uint options = 0x0;

        if (getMidiInCount() == 0 && (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS) == 0)
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (pData->engine->getProccessMode() != ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            if (pData->options & PLUGIN_OPTION_FORCE_STEREO)
                options |= PLUGIN_OPTION_FORCE_STEREO;
            else if (pData->audioIn.count <= 1 && pData->audioOut.count <= 1 && (pData->audioIn.count != 0 || pData->audioOut.count != 0))
                options |= PLUGIN_OPTION_FORCE_STEREO;
        }

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_CONTROL_CHANGES)
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_CHANNEL_PRESSURE)
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_NOTE_AFTERTOUCH)
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_PITCHBEND)
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF)
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_PROGRAM_CHANGES)
            options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;
        else if (hasMidiProgs)
            options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        return options;
    }

    float getParameterValue(const uint32_t parameterId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_value != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        // FIXME - try
        return fDescriptor->get_parameter_value(fHandle, parameterId);
    }

    float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, 0.0f);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            CARLA_SAFE_ASSERT_RETURN(scalePointId < param->scalePointCount, 0.0f);

            const NativeParameterScalePoint* scalePoint(&param->scalePoints[scalePointId]);
            return scalePoint->value;
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        return 0.0f;
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->label != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->label, STR_MAX);
            return;
        }

        CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->maker != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->maker, STR_MAX);
            return;
        }

        CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->copyright != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->copyright, STR_MAX);
            return;
        }

        CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);

        if (fDescriptor->name != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->name, STR_MAX);
            return;
        }

        CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            if (param->name != nullptr)
            {
                std::strncpy(strBuf, param->name, STR_MAX);
                return;
            }

            carla_safe_assert("param->name != nullptr", __FILE__, __LINE__);
            return CarlaPlugin::getParameterName(parameterId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            if (param->unit != nullptr)
            {
                std::strncpy(strBuf, param->unit, STR_MAX);
                return;
            }

            return CarlaPlugin::getParameterUnit(parameterId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            CARLA_SAFE_ASSERT_RETURN(scalePointId < param->scalePointCount,);

            const NativeParameterScalePoint* scalePoint(&param->scalePoints[scalePointId]);

            if (scalePoint->label != nullptr)
            {
                std::strncpy(strBuf, scalePoint->label, STR_MAX);
                return;
            }

            carla_safe_assert("scalePoint->label != nullptr", __FILE__, __LINE__);
            return CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (pData->midiprog.count > 0 && fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
        {
            char strBuf[STR_MAX+1];
            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i",
                          fCurMidiProgs[0],  fCurMidiProgs[1],  fCurMidiProgs[2],  fCurMidiProgs[3],
                          fCurMidiProgs[4],  fCurMidiProgs[5],  fCurMidiProgs[6],  fCurMidiProgs[7],
                          fCurMidiProgs[8],  fCurMidiProgs[9],  fCurMidiProgs[10], fCurMidiProgs[11],
                          fCurMidiProgs[12], fCurMidiProgs[13], fCurMidiProgs[14], fCurMidiProgs[15]);
            strBuf[STR_MAX] = '\0';

            CarlaPlugin::setCustomData(CUSTOM_DATA_TYPE_STRING, "midiPrograms", strBuf, false);
        }

        if (fDescriptor == nullptr || fDescriptor->get_state == nullptr || (fDescriptor->hints & NATIVE_PLUGIN_USES_STATE) == 0)
            return;

        if (char* data = fDescriptor->get_state(fHandle))
        {
            CarlaPlugin::setCustomData(CUSTOM_DATA_TYPE_CHUNK, "State", data, false);
            std::free(data);
        }
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setName(const char* const newName) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0',);

        char uiName[std::strlen(newName)+6+1];
        std::strcpy(uiName, newName);
        std::strcat(uiName, " (GUI)");

        if (fHost.uiName != nullptr)
            delete[] fHost.uiName;
        fHost.uiName = carla_strdup(uiName);

        if (fDescriptor->dispatcher != nullptr)
            fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_UI_NAME_CHANGED, 0, 0, uiName, 0.0f);

        CarlaPlugin::setName(newName);
    }

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        if (channel >= 0 && channel < MAX_MIDI_CHANNELS && pData->midiprog.count > 0)
            pData->midiprog.current = fCurMidiProgs[channel];

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->set_parameter_value != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count,);

        const float fixedValue(pData->param.getFixedValue(parameterId, value));

        // FIXME - try
        fDescriptor->set_parameter_value(fHandle, parameterId, fixedValue);

        if (fHandle2 != nullptr)
            fDescriptor->set_parameter_value(fHandle2, parameterId, fixedValue);

        CarlaPlugin::setParameterValue(parameterId, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        carla_debug("CarlaPluginNative::setCustomData(%s, %s, %s, %s)", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) != 0 && std::strcmp(type, CUSTOM_DATA_TYPE_CHUNK) != 0)
            return carla_stderr2("CarlaPluginNative::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_CHUNK) == 0)
        {
            if (fDescriptor->set_state != nullptr && (fDescriptor->hints & NATIVE_PLUGIN_USES_STATE) != 0)
            {
                const ScopedSingleProcessLocker spl(this, true);

                fDescriptor->set_state(fHandle, value);

                if (fHandle2 != nullptr)
                    fDescriptor->set_state(fHandle2, value);
            }
        }
        else if (std::strcmp(key, "midiPrograms") == 0 && fDescriptor->set_midi_program != nullptr)
        {
            StringArray midiProgramList(StringArray::fromTokens(value, ":", ""));

            if (midiProgramList.size() == MAX_MIDI_CHANNELS)
            {
                uint8_t channel = 0;
                for (String *it=midiProgramList.begin(), *end=midiProgramList.end(); it != end; ++it)
                {
                    const int index(it->getIntValue());

                    if (index >= 0 && index < static_cast<int>(pData->midiprog.count))
                    {
                        const uint32_t bank    = pData->midiprog.data[index].bank;
                        const uint32_t program = pData->midiprog.data[index].program;

                        fDescriptor->set_midi_program(fHandle, channel, bank, program);

                        if (fHandle2 != nullptr)
                            fDescriptor->set_midi_program(fHandle2, channel, bank, program);

                        fCurMidiProgs[channel] = index;

                        if (pData->ctrlChannel == static_cast<int32_t>(channel))
                        {
                            pData->midiprog.current = index;
                            pData->engine->callback(ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED, pData->id, index, 0, 0.0f, nullptr);
                        }
                    }

                    ++channel;
                }
                CARLA_SAFE_ASSERT(channel == MAX_MIDI_CHANNELS);
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

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);

        // TODO, put into check below
        if ((pData->hints & PLUGIN_IS_SYNTH) != 0 && (pData->ctrlChannel < 0 || pData->ctrlChannel >= MAX_MIDI_CHANNELS))
           return CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);

        if (index >= 0)
        {
            const uint8_t  channel = uint8_t((pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS) ? pData->ctrlChannel : 0);
            const uint32_t bank    = pData->midiprog.data[index].bank;
            const uint32_t program = pData->midiprog.data[index].program;

            const ScopedSingleProcessLocker spl(this, (sendGui || sendOsc || sendCallback));

            try {
                fDescriptor->set_midi_program(fHandle, channel, bank, program);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->set_midi_program(fHandle2, channel, bank, program);
                } catch(...) {}
            }

            fCurMidiProgs[channel] = index;
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void showCustomUI(const bool yesNo) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->ui_show == nullptr)
            return;

        fIsUiAvailable = true;

        fDescriptor->ui_show(fHandle, yesNo);

        // UI might not be available, see NATIVE_HOST_OPCODE_UI_UNAVAILABLE
        if (yesNo && ! fIsUiAvailable)
            return;

        fIsUiVisible = yesNo;

        if (! yesNo)
        {
            pData->transientTryCounter = 0;
            return;
        }

#ifndef BUILD_BRIDGE
        if ((fDescriptor->hints & NATIVE_PLUGIN_USES_PARENT_ID) == 0 && std::strstr(fDescriptor->label, "file") == nullptr)
            pData->tryTransient();
#endif

        if (fDescriptor->ui_set_custom_data != nullptr)
        {
            for (LinkedList<CustomData>::Itenerator it = pData->custom.begin2(); it.valid(); it.next())
            {
                const CustomData& cData(it.getValue());

                if (std::strcmp(cData.type, CUSTOM_DATA_TYPE_STRING) == 0 && std::strcmp(cData.key, "midiPrograms") != 0)
                    fDescriptor->ui_set_custom_data(fHandle, cData.key, cData.value);
            }
        }

        if (fDescriptor->ui_set_midi_program != nullptr && pData->midiprog.current >= 0 && pData->midiprog.count > 0)
        {
            const int32_t  index   = pData->midiprog.current;
            const uint8_t  channel = uint8_t((pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS) ? pData->ctrlChannel : 0);
            const uint32_t bank    = pData->midiprog.data[index].bank;
            const uint32_t program = pData->midiprog.data[index].program;

            fDescriptor->ui_set_midi_program(fHandle, channel, bank, program);
        }

        if (fDescriptor->ui_set_parameter_value != nullptr)
        {
            for (uint32_t i=0; i < pData->param.count; ++i)
                fDescriptor->ui_set_parameter_value(fHandle, i, fDescriptor->get_parameter_value(fHandle, i));
        }
    }

    void uiIdle() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fIsUiVisible && fDescriptor->ui_idle != nullptr)
            fDescriptor->ui_idle(fHandle);

        CarlaPlugin::uiIdle();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        carla_debug("CarlaPluginNative::reload() - start");

        const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const float sampleRate((float)pData->engine->getSampleRate());

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

        if ((pData->options & PLUGIN_OPTION_FORCE_STEREO) != 0 && (aIns == 1 || aOuts == 1) && mIns <= 1 && mOuts <= 1)
        {
            if (fHandle2 == nullptr)
                fHandle2 = fDescriptor->instantiate(&fHost);

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
            pData->param.createNew(params, true);
        }

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        CarlaString portName;

        // Audio Ins
        for (j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
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

            pData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, j);
            pData->audioIn.ports[j].rindex = j;

            if (forcedStereoIn)
            {
                portName += "_2";
                pData->audioIn.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, true, 1);
                pData->audioIn.ports[1].rindex = j;
                break;
            }
        }

        // Audio Outs
        for (j=0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
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

            pData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, j);
            pData->audioOut.ports[j].rindex = j;

            if (forcedStereoOut)
            {
                portName += "_2";
                pData->audioOut.ports[1].port   = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio, portName, false, 1);
                pData->audioOut.ports[1].rindex = j;
                break;
            }
        }

        // MIDI Input (only if multiple)
        if (mIns > 1)
        {
            for (j=0; j < mIns; ++j)
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                portName += "midi-in_";
                portName += CarlaString(j+1);
                portName.truncate(portNameSize);

                fMidiIn.ports[j]   = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, true, j);
                fMidiIn.indexes[j] = j;
            }

            pData->event.portIn = fMidiIn.ports[0];
        }

        // MIDI Output (only if multiple)
        if (mOuts > 1)
        {
            for (j=0; j < mOuts; ++j)
            {
                portName.clear();

                if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
                {
                    portName  = pData->name;
                    portName += ":";
                }

                portName += "midi-out_";
                portName += CarlaString(j+1);
                portName.truncate(portNameSize);

                fMidiOut.ports[j]   = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, j);
                fMidiOut.indexes[j] = j;
            }

            pData->event.portOut = fMidiOut.ports[0];
        }

        for (j=0; j < params; ++j)
        {
            const NativeParameter* const paramInfo(fDescriptor->get_parameter_info(fHandle, j));

            CARLA_SAFE_ASSERT_CONTINUE(paramInfo != nullptr);

            pData->param.data[j].type   = PARAMETER_UNKNOWN;
            pData->param.data[j].index  = static_cast<int32_t>(j);
            pData->param.data[j].rindex = static_cast<int32_t>(j);

            float min, max, def, step, stepSmall, stepLarge;

            // min value
            min = paramInfo->ranges.min;

            // max value
            max = paramInfo->ranges.max;

            if (min > max)
                max = min;
            else if (max < min)
                min = max;

            if (carla_isEqual(min, max))
            {
                carla_stderr2("WARNING - Broken plugin parameter '%s': max == min", paramInfo->name);
                max = min + 0.1f;
            }

            // default value
            def = paramInfo->ranges.def;

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            if (paramInfo->hints & NATIVE_PARAMETER_USES_SAMPLE_RATE)
            {
                min *= sampleRate;
                max *= sampleRate;
                def *= sampleRate;
                pData->param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
            }

            if (paramInfo->hints & NATIVE_PARAMETER_IS_BOOLEAN)
            {
                step = max - min;
                stepSmall = step;
                stepLarge = step;
                pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
            }
            else if (paramInfo->hints & NATIVE_PARAMETER_IS_INTEGER)
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

            if (paramInfo->hints & NATIVE_PARAMETER_IS_OUTPUT)
            {
                pData->param.data[j].type = PARAMETER_OUTPUT;
                needsCtrlOut = true;
            }
            else
            {
                pData->param.data[j].type = PARAMETER_INPUT;
                needsCtrlIn = true;
            }

            // extra parameter hints
            if (paramInfo->hints & NATIVE_PARAMETER_IS_ENABLED)
                pData->param.data[j].hints |= PARAMETER_IS_ENABLED;

            if (paramInfo->hints & NATIVE_PARAMETER_IS_AUTOMABLE)
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

            if (paramInfo->hints & NATIVE_PARAMETER_IS_LOGARITHMIC)
                pData->param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

            if (paramInfo->hints & NATIVE_PARAMETER_USES_SCALEPOINTS)
                pData->param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;
        }

        if (needsCtrlIn && mIns <= 1)
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

        if (needsCtrlOut && mOuts <= 1)
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

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // native plugin hints
        if (fDescriptor->hints & NATIVE_PLUGIN_IS_RTSAFE)
            pData->hints |= PLUGIN_IS_RTSAFE;
        if (fDescriptor->hints & NATIVE_PLUGIN_IS_SYNTH)
            pData->hints |= PLUGIN_IS_SYNTH;
        if (fDescriptor->hints & NATIVE_PLUGIN_HAS_UI)
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;
        if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS)
            pData->hints |= PLUGIN_NEEDS_FIXED_BUFFERS;
        if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD)
            pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
        if (fDescriptor->hints & NATIVE_PLUGIN_USES_MULTI_PROGS)
            pData->hints |= PLUGIN_USES_MULTI_PROGS;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (aIns <= 2 && aOuts <= 2 && (aIns == aOuts || aIns == 0 || aOuts == 0) && mIns <= 1 && mOuts <= 1)
            pData->extraHints |= PLUGIN_EXTRA_HINT_CAN_RUN_RACK;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginNative::reload() - end");
    }

    void reloadPrograms(const bool doInit) override
    {
        carla_debug("CarlaPluginNative::reloadPrograms(%s)", bool2str(doInit));
        uint32_t i, oldCount  = pData->midiprog.count;
        const int32_t current = pData->midiprog.current;

        // Delete old programs
        pData->midiprog.clear();

        // Query new programs
        uint32_t count = 0;
        if (fDescriptor->get_midi_program_count != nullptr && fDescriptor->get_midi_program_info != nullptr && fDescriptor->set_midi_program != nullptr)
            count = fDescriptor->get_midi_program_count(fHandle);

        if (count > 0)
        {
            pData->midiprog.createNew(count);

            // Update data
            for (i=0; i < count; ++i)
            {
                const NativeMidiProgram* const mpDesc(fDescriptor->get_midi_program_info(fHandle, i));
                CARLA_SAFE_ASSERT_CONTINUE(mpDesc != nullptr);

                pData->midiprog.data[i].bank    = mpDesc->bank;
                pData->midiprog.data[i].program = mpDesc->program;
                pData->midiprog.data[i].name    = carla_strdup(mpDesc->name);
            }
        }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        // Update OSC Names
        if (pData->engine->isOscControlRegistered() && pData->id < pData->engine->getCurrentPluginCount())
        {
            pData->engine->oscSend_control_set_midi_program_count(pData->id, count);

            for (i=0; i < count; ++i)
                pData->engine->oscSend_control_set_midi_program_data(pData->id, i, pData->midiprog.data[i].bank, pData->midiprog.data[i].program, pData->midiprog.data[i].name);
        }
#endif

        if (doInit)
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
                pData->midiprog.current = static_cast<int32_t>(oldCount);
                programChanged = true;
            }
            else if (current < 0 && count > 0)
            {
                // programs exist now, but not before
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else if (current >= 0 && count == 0)
            {
                // programs existed before, but not anymore
                pData->midiprog.current = -1;
                programChanged = true;
            }
            else if (current >= static_cast<int32_t>(count))
            {
                // current midi program > count
                pData->midiprog.current = 0;
                programChanged = true;
            }
            else
            {
                // no change
                pData->midiprog.current = current;
            }

            if (programChanged)
                setMidiProgram(pData->midiprog.current, true, true, true);

            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, 0, 0, 0.0f, nullptr);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->activate != nullptr)
        {
            try {
                fDescriptor->activate(fHandle);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->activate(fHandle2);
                } catch(...) {}
            }
        }
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);

        if (fDescriptor->deactivate != nullptr)
        {
            try {
                fDescriptor->deactivate(fHandle);
            } catch(...) {}

            if (fHandle2 != nullptr)
            {
                try {
                    fDescriptor->deactivate(fHandle2);
                } catch(...) {}
            }
        }
    }

    void process(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FloatVectorOperations::clear(audioOut[i], static_cast<int>(frames));
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                FloatVectorOperations::clear(cvOut[i], static_cast<int>(frames));
            return;
        }

        fMidiEventCount = 0;
        carla_zeroStructs(fMidiEvents, kPluginMaxMidiEvents*2);

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                fMidiEventCount = MAX_MIDI_CHANNELS*2;

                for (uint8_t k=0, i=MAX_MIDI_CHANNELS; k < MAX_MIDI_CHANNELS; ++k)
                {
                    fMidiEvents[k].data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (k & MIDI_CHANNEL_BIT));
                    fMidiEvents[k].data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                    fMidiEvents[k].data[2] = 0;
                    fMidiEvents[k].size    = 3;

                    fMidiEvents[k+i].data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (k & MIDI_CHANNEL_BIT));
                    fMidiEvents[k+i].data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                    fMidiEvents[k+i].data[2] = 0;
                    fMidiEvents[k+i].size    = 3;
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                fMidiEventCount = MAX_MIDI_NOTE;

                for (uint8_t k=0; k < MAX_MIDI_NOTE; ++k)
                {
                    fMidiEvents[k].data[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (pData->ctrlChannel & MIDI_CHANNEL_BIT));
                    fMidiEvents[k].data[1] = k;
                    fMidiEvents[k].data[2] = 0;
                    fMidiEvents[k].size    = 3;
                }
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo& timeInfo(pData->engine->getTimeInfo());

        fTimeInfo.playing = timeInfo.playing;
        fTimeInfo.frame   = timeInfo.frame;
        fTimeInfo.usecs   = timeInfo.usecs;

        if (timeInfo.valid & EngineTimeInfo::kValidBBT)
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

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                ExternalMidiNote note = { 0, 0, 0 };

                for (; fMidiEventCount < kPluginMaxMidiEvents*2 && ! pData->extNotes.data.isEmpty();)
                {
                    note = pData->extNotes.data.getFirst(note, true);

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);

                    nativeEvent.data[0] = uint8_t((note.velo > 0 ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | (note.channel & MIDI_CHANNEL_BIT));
                    nativeEvent.data[1] = note.note;
                    nativeEvent.data[2] = note.velo;
                    nativeEvent.size    = 3;
                }

                pData->extNotes.mutex.unlock();

            } // End of MIDI Input (External)

            // ----------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE
            bool allNotesOffSent = false;
#endif
            bool sampleAccurate  = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

            for (uint32_t m=0, max=jmax(1U, fMidiIn.count); m < max; ++m)
            {
                CarlaEngineEventPort* const eventPort(m == 0 ? pData->event.portIn : fMidiIn.ports[m]);

            for (uint32_t i=0, numEvents=eventPort->getEventCount(); i < numEvents; ++i)
            {
                const EngineEvent& event(eventPort->getEvent(i));

                if (event.time >= frames)
                    continue;

                CARLA_ASSERT_INT2(event.time >= timeOffset, event.time, timeOffset);

                if (event.time > timeOffset && sampleAccurate)
                {
                    if (processSingle(audioIn, audioOut, cvIn, cvOut, event.time - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = event.time;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;

                        if (fMidiEventCount > 0)
                        {
                            carla_zeroStructs(fMidiEvents, fMidiEventCount);
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

                case kEngineEventTypeControl: {
                    const EngineControlEvent& ctrlEvent = event.ctrl;

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

                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                value = ctrlEvent.value;
                                setDryWet(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_DRYWET, 0, value);
                            }

                            if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                value = ctrlEvent.value*127.0f/100.0f;
                                setVolume(value, false, false);
                                pData->postponeRtEvent(kPluginPostRtEventParameterChange, PARAMETER_VOLUME, 0, value);
                            }

                            if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) > 0)
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

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_CONTROL)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = sampleAccurate ? startTime : event.time;
                            nativeEvent.data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            nativeEvent.data[1] = uint8_t(ctrlEvent.param);
                            nativeEvent.data[2] = uint8_t(ctrlEvent.value*127.0f);
                            nativeEvent.size    = 3;
                        }
                        break;
                    }

                    case kEngineControlEventTypeMidiBank:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            if (event.channel == pData->ctrlChannel)
                                nextBankId = ctrlEvent.param;
                        }
                        else if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = sampleAccurate ? startTime : event.time;
                            nativeEvent.data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            nativeEvent.data[1] = MIDI_CONTROL_BANK_SELECT;
                            nativeEvent.data[2] = uint8_t(ctrlEvent.param);
                            nativeEvent.size    = 3;
                        }
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            if (event.channel < MAX_MIDI_CHANNELS)
                            {
                                const uint32_t nextProgramId(ctrlEvent.param);

                                for (uint32_t k=0; k < pData->midiprog.count; ++k)
                                {
                                    if (pData->midiprog.data[k].bank == nextBankId && pData->midiprog.data[k].program == nextProgramId)
                                    {
                                        fDescriptor->set_midi_program(fHandle, event.channel, nextBankId, nextProgramId);

                                        if (fHandle2 != nullptr)
                                            fDescriptor->set_midi_program(fHandle2, event.channel, nextBankId, nextProgramId);

                                        fCurMidiProgs[event.channel] = static_cast<int32_t>(k);

                                        if (event.channel == pData->ctrlChannel)
                                            pData->postponeRtEvent(kPluginPostRtEventMidiProgramChange, static_cast<int32_t>(k), 0, 0.0f);

                                        break;
                                    }
                                }
                            }
                        }
                        else if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = sampleAccurate ? startTime : event.time;
                            nativeEvent.data[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            nativeEvent.data[1] = uint8_t(ctrlEvent.param);
                            nativeEvent.size    = 2;
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = sampleAccurate ? startTime : event.time;
                            nativeEvent.data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            nativeEvent.data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                            nativeEvent.data[2] = 0;
                            nativeEvent.size    = 3;
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                sendMidiAllNotesOffToCallback();
                            }
#endif

                            if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = sampleAccurate ? startTime : event.time;
                            nativeEvent.data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            nativeEvent.data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                            nativeEvent.data[2] = 0;
                            nativeEvent.size    = 3;
                        }
                        break;
                    }
                    break;
                }

                case kEngineEventTypeMidi: {
                    if (fMidiEventCount >= kPluginMaxMidiEvents*2)
                        continue;

                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > 4)
                        continue;
#ifdef CARLA_PROPER_CPP11_SUPPORT
                    static_assert(4 <= EngineMidiEvent::kDataSize, "Incorrect data");
#endif

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && (pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && (pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0)
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && (pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0)
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);
                    carla_zeroStruct(nativeEvent);

                    nativeEvent.port = midiEvent.port;
                    nativeEvent.time = sampleAccurate ? startTime : event.time;
                    nativeEvent.size = midiEvent.size;

                    nativeEvent.data[0] = uint8_t(status | (event.channel & MIDI_CHANNEL_BIT));
                    nativeEvent.data[1] = midiEvent.size >= 2 ? midiEvent.data[1] : 0;
                    nativeEvent.data[2] = midiEvent.size >= 3 ? midiEvent.data[2] : 0;
                    nativeEvent.data[3] = midiEvent.size == 4 ? midiEvent.data[3] : 0;

                    if (status == MIDI_STATUS_NOTE_ON)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOn, event.channel, midiEvent.data[1], midiEvent.data[2]);
                    else if (status == MIDI_STATUS_NOTE_OFF)
                        pData->postponeRtEvent(kPluginPostRtEventNoteOff, event.channel, midiEvent.data[1], 0.0f);
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, cvIn, cvOut, frames - timeOffset, timeOffset);

            } // eventPort

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, cvIn, cvOut, frames, 0);

        } // End of Plugin processing (no events)

        // --------------------------------------------------------------------------------------------------------
        // Control and MIDI Output

        if (pData->event.portOut != nullptr)
        {
#ifndef BUILD_BRIDGE
            float value, curValue;

            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                curValue = fDescriptor->get_parameter_value(fHandle, k);
                pData->param.ranges[k].fixValue(curValue);

                if (pData->param.data[k].midiCC > 0)
                {
                    value = pData->param.ranges[k].getNormalizedValue(curValue);
                    pData->event.portOut->writeControlEvent(0, pData->param.data[k].midiChannel, kEngineControlEventTypeParameter, static_cast<uint16_t>(pData->param.data[k].midiCC), value);
                }
            }
#endif

            // reverse lookup MIDI events
            for (uint32_t k = (kPluginMaxMidiEvents*2)-1; k >= fMidiEventCount; --k)
            {
                if (fMidiEvents[k].data[0] == 0)
                    break;

                const uint8_t channel = uint8_t(MIDI_GET_CHANNEL_FROM_DATA(fMidiEvents[k].data));
                const uint8_t port    = fMidiEvents[k].port;

                if (fMidiOut.count > 1 && port < fMidiOut.count)
                    fMidiOut.ports[port]->writeMidiEvent(fMidiEvents[k].time, channel, fMidiEvents[k].size, fMidiEvents[k].data);
                else
                    pData->event.portOut->writeMidiEvent(fMidiEvents[k].time, channel, fMidiEvents[k].size, fMidiEvents[k].data);
            }

        } // End of Control and MIDI Output
    }

    bool processSingle(const float** const audioIn, float** const audioOut, const float** const cvIn, float** const cvOut, const uint32_t frames, const uint32_t timeOffset)
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
        if (pData->cvIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvIn != nullptr, false);
        }
        if (pData->cvOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(cvOut != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (fIsOffline)
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
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    cvOut[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio buffers

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            FloatVectorOperations::copy(fAudioInBuffers[i], audioIn[i]+timeOffset, static_cast<int>(frames));

        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            FloatVectorOperations::clear(fAudioOutBuffers[i], static_cast<int>(frames));

#if 0
        // --------------------------------------------------------------------------------------------------------
        // Set CV buffers

        for (uint32_t i=0; i < pData->cvIn.count; ++i)
            FloatVectorOperations::copy(fCvInBuffers[i], cvIn[i]+timeOffset, static_cast<int>(frames));

        for (uint32_t i=0; i < pData->cvOut.count; ++i)
            FloatVectorOperations::clear(fCvOutBuffers[i], static_cast<int>(frames));
#endif

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fIsProcessing = true;

        if (fHandle2 == nullptr)
        {
            fDescriptor->process(fHandle, fAudioInBuffers, fAudioOutBuffers, frames, fMidiEvents, fMidiEventCount);
        }
        else
        {
            fDescriptor->process(fHandle,
                                 (pData->audioIn.count > 0) ? &fAudioInBuffers[0] : nullptr,
                                 (pData->audioOut.count > 0) ? &fAudioOutBuffers[0] : nullptr,
                                 frames, fMidiEvents, fMidiEventCount);

            fDescriptor->process(fHandle2,
                                 (pData->audioIn.count > 0) ? &fAudioInBuffers[1] : nullptr,
                                 (pData->audioOut.count > 0) ? &fAudioOutBuffers[1] : nullptr,
                                 frames, fMidiEvents, fMidiEventCount);
        }

        fIsProcessing = false;
        fTimeInfo.frame += frames;

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));

            bool isPair;
            float bufValue, oldBufLeft[doBalance ? frames : 1];

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
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
                        FloatVectorOperations::copy(oldBufLeft, fAudioOutBuffers[i], static_cast<int>(frames));
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
#else
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                audioOut[i][k+timeOffset] = fAudioOutBuffers[i][k];
        }
#endif

#if 0
        for (uint32_t i=0; i < pData->cvOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                cvOut[i][k+timeOffset] = fCvOutBuffers[i][k];
        }
#endif

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginNative::bufferSizeChanged(%i)", newBufferSize);

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

        if (fCurBufferSize == newBufferSize)
            return;

        fCurBufferSize = newBufferSize;

        if (fDescriptor != nullptr && fDescriptor->dispatcher != nullptr)
        {
            fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, static_cast<intptr_t>(newBufferSize), nullptr, 0.0f);

            if (fHandle2 != nullptr)
                fDescriptor->dispatcher(fHandle2, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, static_cast<intptr_t>(newBufferSize), nullptr, 0.0f);
        }
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginNative::sampleRateChanged(%g)", newSampleRate);

        if (carla_isEqual(fCurSampleRate, newSampleRate))
            return;

        fCurSampleRate = newSampleRate;

        if (fDescriptor != nullptr && fDescriptor->dispatcher != nullptr)
        {
            fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0, nullptr, float(newSampleRate));

            if (fHandle2 != nullptr)
                fDescriptor->dispatcher(fHandle2, NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0, nullptr, float(newSampleRate));
        }
    }

    void offlineModeChanged(const bool isOffline) override
    {
        if (fIsOffline == isOffline)
            return;

        fIsOffline = isOffline;

        if (fDescriptor != nullptr && fDescriptor->dispatcher != nullptr)
        {
            fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_OFFLINE_CHANGED, 0, isOffline ? 1 : 0, nullptr, 0.0f);

            if (fHandle2 != nullptr)
                fDescriptor->dispatcher(fHandle2, NATIVE_PLUGIN_OPCODE_OFFLINE_CHANGED, 0, isOffline ? 1 : 0, nullptr, 0.0f);
        }
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    void initBuffers() const noexcept override
    {
        fMidiIn.initBuffers();
        fMidiOut.initBuffers();

        CarlaPlugin::initBuffers();
    }

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginNative::clearBuffers() - start");

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

        if (fMidiIn.count > 1)
            pData->event.portIn = nullptr;

        if (fMidiOut.count > 1)
            pData->event.portOut = nullptr;

        fMidiIn.clear();
        fMidiOut.clear();

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginNative::clearBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        if (! fIsUiVisible)
            return;

        if (fDescriptor->ui_set_parameter_value != nullptr)
            fDescriptor->ui_set_parameter_value(fHandle, index, value);
    }

    void uiMidiProgramChange(const uint32_t index) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

        if (! fIsUiVisible)
            return;
        if (index >= pData->midiprog.count)
            return;

        if (fDescriptor->ui_set_midi_program != nullptr)
            fDescriptor->ui_set_midi_program(fHandle, 0, pData->midiprog.data[index].bank, pData->midiprog.data[index].program);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);
        CARLA_SAFE_ASSERT_RETURN(velo > 0 && velo < MAX_MIDI_VALUE,);

        if (! fIsUiVisible)
            return;

        // TODO
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
        CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_NOTE,);

        if (! fIsUiVisible)
            return;
        if (fDescriptor == nullptr || fHandle == nullptr)
            return;

        // TODO
    }

    // -------------------------------------------------------------------

protected:
    const NativeTimeInfo* handleGetTimeInfo() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, nullptr);

        return &fTimeInfo;
    }

    bool handleWriteMidiEvent(const NativeMidiEvent* const event)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->enabled, false);
        CARLA_SAFE_ASSERT_RETURN(fIsProcessing, false);
        CARLA_SAFE_ASSERT_RETURN(fMidiOut.count > 0 || pData->event.portOut != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event->data[0] != 0, false);

        // reverse-find first free event, and put it there
        for (uint32_t i=(kPluginMaxMidiEvents*2)-1; i >= fMidiEventCount; --i)
        {
            if (fMidiEvents[i].data[0] != 0)
                continue;

            std::memcpy(&fMidiEvents[i], event, sizeof(NativeMidiEvent));
            return true;
        }

        carla_stdout("CarlaPluginNative::handleWriteMidiEvent(%p) - buffer full", event);
        return false;
    }

    void handleUiParameterChanged(const uint32_t index, const float value)
    {
        setParameterValue(index, value, false, true, true);
    }

    void handleUiCustomDataChanged(const char* const key, const char* const value)
    {
        setCustomData(CUSTOM_DATA_TYPE_STRING, key, value, false);
    }

    void handleUiClosed()
    {
        pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0.0f, nullptr);
        fIsUiVisible = false;
    }

    const char* handleUiOpenFile(const bool isDir, const char* const title, const char* const filter)
    {
        return pData->engine->runFileCallback(FILE_CALLBACK_OPEN, isDir, title, filter);
    }

    const char* handleUiSaveFile(const bool isDir, const char* const title, const char* const filter)
    {
        return pData->engine->runFileCallback(FILE_CALLBACK_SAVE, isDir, title, filter);
    }

    intptr_t handleDispatcher(const NativeHostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
        carla_debug("CarlaPluginNative::handleDispatcher(%i, %i, " P_INTPTR ", %p, %f)", opcode, index, value, ptr, opt);

        intptr_t ret = 0;

        switch (opcode)
        {
        case NATIVE_HOST_OPCODE_NULL:
            break;
        case NATIVE_HOST_OPCODE_UPDATE_PARAMETER:
            // TODO
            pData->engine->callback(ENGINE_CALLBACK_UPDATE, pData->id, -1, 0, 0.0f, nullptr);
            break;
        case NATIVE_HOST_OPCODE_UPDATE_MIDI_PROGRAM:
            // TODO
            pData->engine->callback(ENGINE_CALLBACK_UPDATE, pData->id, -1, 0, 0.0f, nullptr);
            break;
        case NATIVE_HOST_OPCODE_RELOAD_PARAMETERS:
            reload(); // FIXME
            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PARAMETERS, pData->id, -1, 0, 0.0f, nullptr);
            break;
        case NATIVE_HOST_OPCODE_RELOAD_MIDI_PROGRAMS:
            reloadPrograms(false);
            pData->engine->callback(ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, -1, 0, 0.0f, nullptr);
            break;
        case NATIVE_HOST_OPCODE_RELOAD_ALL:
            reload();
            pData->engine->callback(ENGINE_CALLBACK_RELOAD_ALL, pData->id, -1, 0, 0.0f, nullptr);
            break;
        case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
            pData->engine->callback(ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0.0f, nullptr);
            fIsUiAvailable = false;
            break;
        case NATIVE_HOST_OPCODE_HOST_IDLE:
            pData->engine->callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);
            break;
        }

        return ret;

        // unused for now
        (void)index;
        (void)value;
        (void)ptr;
        (void)opt;
    }

    // -------------------------------------------------------------------

public:
    void* getNativeHandle() const noexcept override
    {
        return fHandle;
    }

    const void* getNativeDescriptor() const noexcept override
    {
        return fDescriptor;
    }

    // -------------------------------------------------------------------

    bool init(const char* const name, const char* const label)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (label == nullptr && label[0] != '\0')
        {
            pData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        sPluginInitializer.initIfNeeded();

        for (LinkedList<const NativePluginDescriptor*>::Itenerator it = gPluginDescriptors.begin2(); it.valid(); it.next())
        {
            fDescriptor = it.getValue();

            CARLA_SAFE_ASSERT_BREAK(fDescriptor != nullptr);

            carla_debug("Check vs \"%s\"", fDescriptor->label);

            if (fDescriptor->label != nullptr && std::strcmp(fDescriptor->label, label) == 0)
                break;

            fDescriptor = nullptr;
        }

        if (fDescriptor == nullptr)
        {
            pData->engine->setLastError("Invalid internal plugin");
            return false;
        }

        // ---------------------------------------------------------------
        // set icon

        if (std::strcmp(fDescriptor->label, "audiofile") == 0)
            pData->iconName = carla_strdup_safe("file");
        else if (std::strcmp(fDescriptor->label, "midifile") == 0)
            pData->iconName = carla_strdup_safe("file");

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else if (fDescriptor->name != nullptr && fDescriptor->name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(fDescriptor->name);
        else
            pData->name = pData->engine->getUniquePluginName(label);

        {
            CARLA_ASSERT(fHost.uiName == nullptr);

            char uiName[std::strlen(pData->name)+6+1];
            std::strcpy(uiName, pData->name);
            std::strcat(uiName, " (GUI)");

            fHost.uiName = carla_strdup(uiName);
        }

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

        fHandle = fDescriptor->instantiate(&fHost);

        if (fHandle == nullptr)
        {
            pData->engine->setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // set default options

        const bool hasMidiProgs(fDescriptor->get_midi_program_count != nullptr && fDescriptor->get_midi_program_count(fHandle) > 0);

        pData->options = 0x0;

        if (getMidiInCount() > 0 || (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS) != 0)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (pData->engine->getOptions().forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_CHANNEL_PRESSURE)
            pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_NOTE_AFTERTOUCH)
            pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_PITCHBEND)
            pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF)
            pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_PROGRAM_CHANGES)
        {
            CARLA_SAFE_ASSERT(! hasMidiProgs);
            pData->options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;

            if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_CONTROL_CHANGES)
                pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
        }
        else if (hasMidiProgs && fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;

        return true;
    }


private:
    NativePluginHandle   fHandle;
    NativePluginHandle   fHandle2;
    NativeHostDescriptor fHost;
    const NativePluginDescriptor* fDescriptor;

    bool fIsProcessing;
    bool fIsOffline;
    bool fIsUiAvailable;
    bool fIsUiVisible;

    float**         fAudioInBuffers;
    float**         fAudioOutBuffers;
    uint32_t        fMidiEventCount;
    NativeMidiEvent fMidiEvents[kPluginMaxMidiEvents*2];

    int32_t  fCurMidiProgs[MAX_MIDI_CHANNELS];
    uint32_t fCurBufferSize;
    double   fCurSampleRate;

    NativePluginMidiData fMidiIn;
    NativePluginMidiData fMidiOut;

    NativeTimeInfo fTimeInfo;

    // -------------------------------------------------------------------

    #define handlePtr ((CarlaPluginNative*)handle)

    static uint32_t carla_host_get_buffer_size(NativeHostHandle handle) noexcept
    {
        return handlePtr->fCurBufferSize;
    }

    static double carla_host_get_sample_rate(NativeHostHandle handle) noexcept
    {
        return handlePtr->fCurSampleRate;
    }

    static bool carla_host_is_offline(NativeHostHandle handle) noexcept
    {
        return handlePtr->fIsOffline;
    }

    static const NativeTimeInfo* carla_host_get_time_info(NativeHostHandle handle) noexcept
    {
        return handlePtr->handleGetTimeInfo();
    }

    static bool carla_host_write_midi_event(NativeHostHandle handle, const NativeMidiEvent* event)
    {
        return handlePtr->handleWriteMidiEvent(event);
    }

    static void carla_host_ui_parameter_changed(NativeHostHandle handle, uint32_t index, float value)
    {
        handlePtr->handleUiParameterChanged(index, value);
    }

    static void carla_host_ui_custom_data_changed(NativeHostHandle handle, const char* key, const char* value)
    {
        handlePtr->handleUiCustomDataChanged(key, value);
    }

    static void carla_host_ui_closed(NativeHostHandle handle)
    {
        handlePtr->handleUiClosed();
    }

    static const char* carla_host_ui_open_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiOpenFile(isDir, title, filter);
    }

    static const char* carla_host_ui_save_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiSaveFile(isDir, title, filter);
    }

    static intptr_t carla_host_dispatcher(NativeHostHandle handle, NativeHostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return handlePtr->handleDispatcher(opcode, index, value, ptr, opt);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginNative)
};

// -----------------------------------------------------------------------

CarlaPlugin* CarlaPlugin::newNative(const Initializer& init)
{
    carla_debug("CarlaPlugin::newNative({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})", init.engine, init.filename, init.name, init.label, init.uniqueId);

    CarlaPluginNative* const plugin(new CarlaPluginNative(init.engine, init.id));

    if (! plugin->init(init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    return plugin;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
