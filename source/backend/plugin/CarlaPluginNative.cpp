// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaNative.h"

#include "water/misc/Time.h"
#include "water/text/StringArray.h"

// -----------------------------------------------------------------------
// used in carla-base.cpp

std::size_t carla_getNativePluginCount() noexcept;
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

// -------------------------------------------------------------------
// Fallback data

static const CustomData  kCustomDataFallback = { nullptr, nullptr, nullptr };
static EngineEvent kNullEngineEvent = {
    kEngineEventTypeNull, 0, 0, {{ kEngineControlEventTypeNull, 0, -1, 0.0f, true }}
};

// -----------------------------------------------------------------------

struct NativePluginMidiOutData {
    uint32_t  count;
    uint32_t* indexes;
    CarlaEngineEventPort** ports;

    NativePluginMidiOutData() noexcept
        : count(0),
          indexes(nullptr),
          ports(nullptr) {}

    ~NativePluginMidiOutData() noexcept
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT(indexes == nullptr);
        CARLA_SAFE_ASSERT(ports == nullptr);
    }

    bool createNew(const uint32_t newCount)
    {
        CARLA_SAFE_ASSERT_INT(count == 0, count);
        CARLA_SAFE_ASSERT_RETURN(indexes == nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(ports == nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(newCount > 0, false);

        indexes       = new uint32_t[newCount];
        ports         = new CarlaEngineEventPort*[newCount];
        count         = newCount;

        carla_zeroStructs(indexes, newCount);
        carla_zeroStructs(ports, newCount);

        return true;
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

    CARLA_DECLARE_NON_COPYABLE(NativePluginMidiOutData)
};

struct NativePluginMidiInData : NativePluginMidiOutData {
    struct MultiPortData {
        uint32_t cachedEventCount;
        uint32_t usedIndex;
    };

    MultiPortData* multiportData;

    NativePluginMidiInData() noexcept
        : NativePluginMidiOutData(),
          multiportData(nullptr) {}

    ~NativePluginMidiInData() noexcept
    {
        CARLA_SAFE_ASSERT(multiportData == nullptr);
    }

    bool createNew(const uint32_t newCount)
    {
        if (! NativePluginMidiOutData::createNew(newCount))
            return false;

        multiportData = new MultiPortData[newCount];
        carla_zeroStructs(multiportData, newCount);

        return true;
    }

    void clear() noexcept
    {
        if (multiportData != nullptr)
        {
            delete[] multiportData;
            multiportData = nullptr;
        }

        NativePluginMidiOutData::clear();
    }

    void initBuffers(CarlaEngineEventPort* const port) const noexcept
    {
        if (count == 1)
        {
            CARLA_SAFE_ASSERT_RETURN(port != nullptr,);

            carla_zeroStruct(multiportData[0]);
            multiportData[0].cachedEventCount = port->getEventCount();
            return;
        }

        for (uint32_t i=0; i < count; ++i)
        {
            carla_zeroStruct(multiportData[i]);

            if (ports[i] != nullptr)
            {
                ports[i]->initBuffer();
                multiportData[i].cachedEventCount = ports[i]->getEventCount();
            }
        }
    }

    CARLA_DECLARE_NON_COPYABLE(NativePluginMidiInData)
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
          fIsOffline(engine->isOffline()),
          fIsUiAvailable(false),
          fIsUiVisible(false),
          fNeedsIdle(false),
          fInlineDisplayNeedsRedraw(false),
          fInlineDisplayLastRedrawTime(0),
          fLastProjectFilename(),
          fLastProjectFolder(),
          fAudioAndCvInBuffers(nullptr),
          fAudioAndCvOutBuffers(nullptr),
          fMidiEventInCount(0),
          fMidiEventOutCount(0),
          fCurBufferSize(engine->getBufferSize()),
          fCurSampleRate(engine->getSampleRate()),
          fMidiIn(),
          fMidiOut(),
          fTimeInfo()
    {
        carla_debug("CarlaPluginNative::CarlaPluginNative(%p, %i)", engine, id);

        carla_fill(fCurMidiProgs, 0, MAX_MIDI_CHANNELS);
        carla_zeroStructs(fMidiInEvents, kPluginMaxMidiEvents);
        carla_zeroStructs(fMidiOutEvents, kPluginMaxMidiEvents);
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

        fInlineDisplayNeedsRedraw = false;

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            if (fIsUiVisible && fDescriptor != nullptr && fDescriptor->ui_show != nullptr && fHandle != nullptr)
                fDescriptor->ui_show(fHandle, false);

#ifndef BUILD_BRIDGE
            pData->transientTryCounter = 0;
#endif
        }

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

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
            std::free(const_cast<char*>(fHost.uiName));
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
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,     0x0);

        bool hasMidiProgs = false;

        if (fDescriptor->get_midi_program_count != nullptr)
        {
            try {
                hasMidiProgs = fDescriptor->get_midi_program_count(fHandle) > 0;
            } catch (...) {}
        }

        uint options = 0x0;

        // can't disable fixed buffers if required by the plugin
        if ((fDescriptor->hints & NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS) == 0x0)
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        // can't disable forced stereo if enabled in the engine, or using CV
        if (pData->engine->getOptions().forceStereo || pData->cvIn.count != 0 || pData->cvOut.count != 0)
            pass();
        // if inputs or outputs are just 1, then yes we can force stereo
        else if (pData->audioIn.count == 1 || pData->audioOut.count == 1 || fHandle2 != nullptr)
            options |= PLUGIN_OPTION_FORCE_STEREO;

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

        if (fDescriptor->midiIns > 0)
            options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;

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

    bool getLabel(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);

        if (fDescriptor->label != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->label, STR_MAX);
            return true;
        }

        return CarlaPlugin::getLabel(strBuf);
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);

        if (fDescriptor->maker != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->maker, STR_MAX);
            return true;
        }

        return CarlaPlugin::getMaker(strBuf);
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);

        if (fDescriptor->copyright != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->copyright, STR_MAX);
            return true;
        }

        return CarlaPlugin::getCopyright(strBuf);
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);

        if (fDescriptor->name != nullptr)
        {
            std::strncpy(strBuf, fDescriptor->name, STR_MAX);
            return true;
        }

        return CarlaPlugin::getRealName(strBuf);
    }

    bool getParameterName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            if (param->name != nullptr)
            {
                std::strncpy(strBuf, param->name, STR_MAX);
                return true;
            }

            carla_safe_assert("param->name != nullptr", __FILE__, __LINE__);
            return CarlaPlugin::getParameterName(parameterId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        return CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    bool getParameterUnit(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            if (param->unit != nullptr)
            {
                std::strncpy(strBuf, param->unit, STR_MAX);
                return true;
            }

            return CarlaPlugin::getParameterUnit(parameterId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        return CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    bool getParameterComment(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            if (param->comment != nullptr)
            {
                std::strncpy(strBuf, param->comment, STR_MAX);
                return true;
            }

            return CarlaPlugin::getParameterComment(parameterId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        return CarlaPlugin::getParameterComment(parameterId, strBuf);
    }

    bool getParameterGroupName(const uint32_t parameterId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            if (param->groupName != nullptr)
            {
                std::strncpy(strBuf, param->groupName, STR_MAX);
                return true;
            }

            return CarlaPlugin::getParameterGroupName(parameterId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        return CarlaPlugin::getParameterGroupName(parameterId, strBuf);
    }

    bool getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->get_parameter_info != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(parameterId < pData->param.count, false);

        // FIXME - try
        if (const NativeParameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId))
        {
            CARLA_SAFE_ASSERT_RETURN(scalePointId < param->scalePointCount, false);

            const NativeParameterScalePoint* scalePoint(&param->scalePoints[scalePointId]);

            if (scalePoint->label != nullptr)
            {
                std::strncpy(strBuf, scalePoint->label, STR_MAX);
                return true;
            }

            carla_safe_assert("scalePoint->label != nullptr", __FILE__, __LINE__);
            return CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
        }

        carla_safe_assert("const Parameter* const param = fDescriptor->get_parameter_info(fHandle, parameterId)", __FILE__, __LINE__);
        return CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (state)

    void prepareForSave(bool) override
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

        CarlaPlugin::setName(newName);

        if (pData->uiTitle.isEmpty())
            setWindowTitle(nullptr);
    }

    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback) noexcept override
    {
        if (channel >= 0 && channel < MAX_MIDI_CHANNELS && pData->midiprog.count > 0)
            pData->midiprog.current = fCurMidiProgs[channel];

        CarlaPlugin::setCtrlChannel(channel, sendOsc, sendCallback);
    }

    void setWindowTitle(const char* const title) noexcept
    {
        String uiName;

        if (title != nullptr)
        {
            uiName = title;
        }
        else
        {
            uiName  = pData->name;
            uiName += " (GUI)";
        }

        std::free(const_cast<char*>(fHost.uiName));
        fHost.uiName = uiName.getAndReleaseBuffer();

        if (fDescriptor->dispatcher != nullptr && fIsUiVisible)
        {
            try {
                fDescriptor->dispatcher(fHandle,
                                        NATIVE_PLUGIN_OPCODE_UI_NAME_CHANGED,
                                        0, 0,
                                        const_cast<char*>(fHost.uiName),
                                        0.0f);
            } CARLA_SAFE_EXCEPTION("set custom ui title");
        }

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

    void setParameterValueRT(const uint32_t parameterId, const float value, const uint32_t frameOffset, const bool sendCallbackLater) noexcept override
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

        CarlaPlugin::setParameterValueRT(parameterId, fixedValue, frameOffset, sendCallbackLater);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
        carla_debug("CarlaPluginNative::setCustomData(\"%s\", \"%s\", ..., %s)", type, key, bool2str(sendGui));

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PROPERTY) == 0)
            return CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (std::strcmp(type, CUSTOM_DATA_TYPE_PATH) == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(key, "file") == 0,);
            CARLA_SAFE_ASSERT_RETURN(value[0] != '\0',);
        }
        else if (std::strcmp(type, CUSTOM_DATA_TYPE_STRING) != 0 && std::strcmp(type, CUSTOM_DATA_TYPE_CHUNK) != 0)
        {
            return carla_stderr2("CarlaPluginNative::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid",
                                 type, key, value, bool2str(sendGui));
        }

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
            water::StringArray midiProgramList(water::StringArray::fromTokens(value, ":", ""));

            if (midiProgramList.size() == MAX_MIDI_CHANNELS)
            {
                uint8_t channel = 0;
                for (water::String *it=midiProgramList.begin(), *end=midiProgramList.end(); it != end; ++it)
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
                            pData->engine->callback(true, true,
                                                    ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED,
                                                    pData->id,
                                                    index,
                                                    0, 0, 0.0f, nullptr);
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

    void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool doingInit) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index >= -1 && index < static_cast<int32_t>(pData->midiprog.count),);
        CARLA_SAFE_ASSERT_RETURN(sendGui || sendOsc || sendCallback || doingInit,);

        // TODO, put into check below
        if ((pData->hints & PLUGIN_IS_SYNTH) != 0 && (pData->ctrlChannel < 0 || pData->ctrlChannel >= MAX_MIDI_CHANNELS))
           return CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, doingInit);

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

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, doingInit);
    }

    // FIXME: this is never used
    void setMidiProgramRT(const uint32_t index, const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index < pData->midiprog.count,);

        // TODO, put into check below
        if ((pData->hints & PLUGIN_IS_SYNTH) != 0 && (pData->ctrlChannel < 0 || pData->ctrlChannel >= MAX_MIDI_CHANNELS))
           return CarlaPlugin::setMidiProgramRT(index, sendCallbackLater);

        const uint8_t  channel = uint8_t((pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS) ? pData->ctrlChannel : 0);
        const uint32_t bank    = pData->midiprog.data[index].bank;
        const uint32_t program = pData->midiprog.data[index].program;

        try {
            fDescriptor->set_midi_program(fHandle, channel, bank, program);
        } catch(...) {}

        if (fHandle2 != nullptr)
        {
            try {
                fDescriptor->set_midi_program(fHandle2, channel, bank, program);
            } catch(...) {}
        }

        fCurMidiProgs[channel] = static_cast<int32_t>(index);

        CarlaPlugin::setMidiProgramRT(index, sendCallbackLater);
    }

    // -------------------------------------------------------------------
    // Set ui stuff

    void setCustomUITitle(const char* const title) noexcept override
    {
        setWindowTitle(title);
        CarlaPlugin::setCustomUITitle(title);
    }

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
#ifndef BUILD_BRIDGE
            pData->transientTryCounter = 0;
#endif
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
                const CustomData& cData(it.getValue(kCustomDataFallback));
                CARLA_SAFE_ASSERT_CONTINUE(cData.isValid());

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

    void idle() override
    {
        if (fNeedsIdle)
        {
            fNeedsIdle = false;
            fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_IDLE, 0, 0, nullptr, 0.0f);
        }

        if (fInlineDisplayNeedsRedraw)
        {
            // TESTING
            CARLA_SAFE_ASSERT(pData->enabled)
            CARLA_SAFE_ASSERT(!pData->engine->isAboutToClose());
            CARLA_SAFE_ASSERT(pData->client->isActive());

            if (pData->enabled && !pData->engine->isAboutToClose() && pData->client->isActive())
            {
                const int64_t timeNow = water::Time::currentTimeMillis();

                if (timeNow - fInlineDisplayLastRedrawTime > (1000 / 30))
                {
                    fInlineDisplayNeedsRedraw = false;
                    fInlineDisplayLastRedrawTime = timeNow;
                    pData->engine->callback(true, true,
                                            ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW,
                                            pData->id,
                                            0, 0, 0, 0.0f, nullptr);
                }
            }
            else
            {
                fInlineDisplayNeedsRedraw = false;
            }
        }

        CarlaPlugin::idle();
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

        const EngineProcessMode processMode = pData->engine->getProccessMode();

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        uint32_t aIns, aOuts, cvIns, cvOuts, mIns, mOuts, j;

        bool forcedStereoIn, forcedStereoOut;
        forcedStereoIn = forcedStereoOut = false;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        aIns   = fDescriptor->audioIns;
        aOuts  = fDescriptor->audioOuts;
        cvIns  = fDescriptor->cvIns;
        cvOuts = fDescriptor->cvOuts;
        mIns   = fDescriptor->midiIns;
        mOuts  = fDescriptor->midiOuts;

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
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            needsCtrlIn = true;
        }

        if (cvIns > 0)
        {
            pData->cvIn.createNew(cvIns);
        }

        if (cvOuts > 0)
        {
            pData->cvOut.createNew(cvOuts);
        }

        if (const uint32_t acIns = aIns + cvIns)
        {
            fAudioAndCvInBuffers = new float*[acIns];
            carla_zeroPointers(fAudioAndCvInBuffers, acIns);
        }

        if (const uint32_t acOuts = aOuts + cvOuts)
        {
            fAudioAndCvOutBuffers = new float*[acOuts];
            carla_zeroPointers(fAudioAndCvOutBuffers, acOuts);
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

        const uint portNameSize(pData->engine->getMaxPortNameSize());
        String portName;

        // Audio Ins
        for (j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fDescriptor->get_buffer_port_name != nullptr)
            {
                portName += fDescriptor->get_buffer_port_name(fHandle, forcedStereoIn ? 0 : j, false);
            }
            else if (aIns > 1 && ! forcedStereoIn)
            {
                portName += "input_";
                portName += String(j+1);
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

            if (fDescriptor->get_buffer_port_name != nullptr)
            {
                portName += fDescriptor->get_buffer_port_name(fHandle, forcedStereoOut ? 0 : j, true);
            }
            else if (aOuts > 1 && ! forcedStereoOut)
            {
                portName += "output_";
                portName += String(j+1);
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

        // CV Ins
        for (j=0; j < cvIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fDescriptor->get_buffer_port_name != nullptr)
            {
                portName += fDescriptor->get_buffer_port_name(fHandle, fDescriptor->audioIns + j, false);
            }
            else if (cvIns > 1)
            {
                portName += "cv_input_";
                portName += String(j+1);
            }
            else
                portName += "cv_input";

            portName.truncate(portNameSize);

            float min = -1.0f, max = 1.0f;
            if (fDescriptor->get_buffer_port_range != nullptr)
            {
                if (const NativePortRange* const range = fDescriptor->get_buffer_port_range(fHandle,
                                                                                            fDescriptor->audioIns + j,
                                                                                            false))
                {
                    min = range->minimum;
                    max = range->maximum;
                }
            }

            pData->cvIn.ports[j].port   = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, true, j);
            pData->cvIn.ports[j].rindex = j;
            pData->cvIn.ports[j].port->setRange(min, max);
        }

        // CV Outs
        for (j=0; j < cvOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (fDescriptor->get_buffer_port_name != nullptr)
            {
                portName += fDescriptor->get_buffer_port_name(fHandle, fDescriptor->audioOuts + j, true);
            }
            else if (cvOuts > 1)
            {
                portName += "cv_output_";
                portName += String(j+1);
            }
            else
                portName += "cv_output";

            portName.truncate(portNameSize);

            float min = -1.0f, max = 1.0f;
            if (fDescriptor->get_buffer_port_range != nullptr)
            {
                if (const NativePortRange* const range = fDescriptor->get_buffer_port_range(fHandle,
                                                                                            fDescriptor->audioOuts + j,
                                                                                            true))
                {
                    min = range->minimum;
                    max = range->maximum;
                }
            }

            pData->cvOut.ports[j].port   = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV, portName, false, j);
            pData->cvOut.ports[j].rindex = j;
            pData->cvOut.ports[j].port->setRange(min, max);
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
                portName += String(j+1);
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
                portName += String(j+1);
                portName.truncate(portNameSize);

                fMidiOut.ports[j]   = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent, portName, false, j);
                fMidiOut.indexes[j] = j;
            }

            pData->event.portOut = fMidiOut.ports[0];
        }

        reloadParameters(&needsCtrlIn, &needsCtrlOut);

        if (needsCtrlIn || mIns == 1)
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
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
#endif
        }

        if (needsCtrlOut || mOuts == 1)
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
        if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
            pData->hints |= PLUGIN_HAS_CUSTOM_UI_USING_FILE_OPEN;
        if (fDescriptor->hints & NATIVE_PLUGIN_USES_MULTI_PROGS)
            pData->hints |= PLUGIN_USES_MULTI_PROGS;
        if (fDescriptor->hints & NATIVE_PLUGIN_HAS_INLINE_DISPLAY)
            pData->hints |= PLUGIN_HAS_INLINE_DISPLAY;

        // extra plugin hints
        pData->extraHints = 0x0;

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();

        carla_debug("CarlaPluginNative::reload() - end");
    }

    void reloadParameters(bool* const needsCtrlIn, bool* const needsCtrlOut)
    {
        carla_debug("CarlaPluginNative::reloadParameters() - start");

        const float sampleRate = static_cast<float>(pData->engine->getSampleRate());
        const uint32_t params = (fDescriptor->get_parameter_count != nullptr && fDescriptor->get_parameter_info != nullptr)
                              ? fDescriptor->get_parameter_count(fHandle)
                              : 0;

        pData->param.clear();

        if (params > 0)
        {
            pData->param.createNew(params, true);
        }

        for (uint32_t j=0; j < params; ++j)
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
                if (needsCtrlOut != nullptr)
                    *needsCtrlOut = true;
            }
            else
            {
                pData->param.data[j].type = PARAMETER_INPUT;
                if (needsCtrlIn != nullptr)
                    *needsCtrlIn = true;
            }

            // extra parameter hints
            if (paramInfo->hints & NATIVE_PARAMETER_IS_ENABLED)
            {
                pData->param.data[j].hints |= PARAMETER_IS_ENABLED;

                if (paramInfo->hints & NATIVE_PARAMETER_IS_AUTOMATABLE)
                {
                    pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;
                    pData->param.data[j].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
                }
            }

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

        carla_debug("CarlaPluginNative::reloadParameters() - end");
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

        if (doInit)
        {
            if (count > 0)
                setMidiProgram(0, false, false, false, true);
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
                setMidiProgram(pData->midiprog.current, true, true, true, false);

            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_RELOAD_PROGRAMS,
                                    pData->id,
                                    0, 0, 0, 0.0f, nullptr);
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

    EngineEvent& findNextEvent()
    {
        if (fMidiIn.count == 1)
        {
            NativePluginMidiInData::MultiPortData& multiportData(fMidiIn.multiportData[0]);

            if (multiportData.usedIndex == multiportData.cachedEventCount)
            {
                const uint32_t eventCount = pData->event.portIn->getEventCount();
                CARLA_SAFE_ASSERT_INT2(eventCount == multiportData.cachedEventCount,
                                       eventCount, multiportData.cachedEventCount);
                return kNullEngineEvent;
            }

            return pData->event.portIn->getEvent(multiportData.usedIndex++);
        }

        uint32_t lowestSampleTime = 9999999;
        uint32_t portMatching = 0;
        bool found = false;

        // process events in order for multiple ports
        for (uint32_t m=0; m < fMidiIn.count; ++m)
        {
            CarlaEngineEventPort* const eventPort(fMidiIn.ports[m]);
            NativePluginMidiInData::MultiPortData& multiportData(fMidiIn.multiportData[m]);

            if (multiportData.usedIndex == multiportData.cachedEventCount)
                continue;

            const EngineEvent& event(eventPort->getEventUnchecked(multiportData.usedIndex));

            if (event.time < lowestSampleTime)
            {
                lowestSampleTime = event.time;
                portMatching = m;
                found = true;
            }
        }

        if (found)
        {
            CarlaEngineEventPort* const eventPort(fMidiIn.ports[portMatching]);
            NativePluginMidiInData::MultiPortData& multiportData(fMidiIn.multiportData[portMatching]);

            return eventPort->getEvent(multiportData.usedIndex++);
        }

        return kNullEngineEvent;
    }

    void process(const float* const* const audioIn, float** const audioOut,
                 const float* const* const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                carla_zeroFloats(cvOut[i], frames);
            return;
        }

        fMidiEventInCount = fMidiEventOutCount = 0;
        carla_zeroStructs(fMidiInEvents, kPluginMaxMidiEvents);
        carla_zeroStructs(fMidiOutEvents, kPluginMaxMidiEvents);

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                fMidiEventInCount = MAX_MIDI_CHANNELS*2;

                for (uint8_t k=0, i=MAX_MIDI_CHANNELS; k < MAX_MIDI_CHANNELS; ++k)
                {
                    fMidiInEvents[k].data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (k & MIDI_CHANNEL_BIT));
                    fMidiInEvents[k].data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                    fMidiInEvents[k].data[2] = 0;
                    fMidiInEvents[k].size    = 3;

                    fMidiInEvents[k+i].data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (k & MIDI_CHANNEL_BIT));
                    fMidiInEvents[k+i].data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                    fMidiInEvents[k+i].data[2] = 0;
                    fMidiInEvents[k+i].size    = 3;
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS)
            {
                fMidiEventInCount = MAX_MIDI_NOTE;

                for (uint8_t k=0; k < MAX_MIDI_NOTE; ++k)
                {
                    fMidiInEvents[k].data[0] = uint8_t(MIDI_STATUS_NOTE_OFF | (pData->ctrlChannel & MIDI_CHANNEL_BIT));
                    fMidiInEvents[k].data[1] = k;
                    fMidiInEvents[k].data[2] = 0;
                    fMidiInEvents[k].size    = 3;
                }
            }

            pData->needsReset = false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());

        fTimeInfo.playing = timeInfo.playing;
        fTimeInfo.frame   = timeInfo.frame;
        fTimeInfo.usecs   = timeInfo.usecs;

        if (timeInfo.bbt.valid)
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
        {
            fTimeInfo.bbt.valid = false;
        }

#if 0
        // This test code has proven to be quite useful
        // So I am leaving it behind, I might need it again..
        if (pData->id == 1)
        {
            static int64_t last_frame = timeInfo.frame;
            static int64_t last_dev_frame = 0;
            static double last_val = timeInfo.bbt.barStartTick + ((timeInfo.bbt.beat-1) * timeInfo.bbt.ticksPerBeat) + timeInfo.bbt.tick;
            static double last_dev_val = 0.0;

            int64_t cur_frame = timeInfo.frame;
            int64_t cur_dev_frame = cur_frame - last_frame;

            double cur_val = timeInfo.bbt.barStartTick + ((timeInfo.bbt.beat-1) * timeInfo.bbt.ticksPerBeat) + timeInfo.bbt.tick;
            double cur_dev_val = cur_val - last_val;

            if (std::abs(last_dev_val - cur_dev_val) >= 0.0001 || last_dev_frame != cur_dev_frame)
            {
                carla_stdout("currently %u at %u => %f : DEV1: %li : DEV2: %f",
                            frames,
                            timeInfo.frame,
                            cur_val,
                            cur_dev_frame,
                            cur_dev_val);
            }

            last_val = cur_val;
            last_dev_val = cur_dev_val;
            last_frame = cur_frame;
            last_dev_frame = cur_dev_frame;
        }
#endif

        // --------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                ExternalMidiNote note = { 0, 0, 0 };

                for (; fMidiEventInCount < kPluginMaxMidiEvents && ! pData->extNotes.data.isEmpty();)
                {
                    note = pData->extNotes.data.getFirst(note, true);

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    NativeMidiEvent& nativeEvent(fMidiInEvents[fMidiEventInCount++]);

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
            const bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime  = 0;
            uint32_t timeOffset = 0;
            uint32_t nextBankId;

            if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
            else
                nextBankId = 0;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (cvIn != nullptr && pData->event.cvSourcePorts != nullptr)
                pData->event.cvSourcePorts->initPortBuffers(cvIn + pData->cvIn.count, frames, isSampleAccurate, pData->event.portIn);
#endif

            for (;;)
            {
                EngineEvent& event(findNextEvent());

                if (event.type == kEngineEventTypeNull)
                    break;

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < timeOffset)
                {
                    carla_stderr2("Timing error, eventTime:%u < timeOffset:%u for '%s'",
                                  eventTime, timeOffset, pData->name);
                    eventTime = timeOffset;
                }

                if (isSampleAccurate && eventTime > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, cvIn, cvOut, eventTime - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = eventTime;

                        if (pData->midiprog.current >= 0 && pData->midiprog.count > 0)
                            nextBankId = pData->midiprog.data[pData->midiprog.current].bank;
                        else
                            nextBankId = 0;

                        if (fMidiEventInCount > 0)
                        {
                            carla_zeroStructs(fMidiInEvents, fMidiEventInCount);
                            fMidiEventInCount = 0;
                        }

                        if (fMidiEventOutCount > 0)
                        {
                            carla_zeroStructs(fMidiOutEvents, fMidiEventOutCount);
                            fMidiEventOutCount = 0;
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
                    EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // non-midi
                        if (event.channel == kEngineEventNonMidiChannel)
                        {
                            const uint32_t k = ctrlEvent.param;
                            CARLA_SAFE_ASSERT_CONTINUE(k < pData->param.count);

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                            continue;
                        }

                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) > 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) > 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) > 0)
                            {
                                float left, right;
                                value = ctrlEvent.normalizedValue/0.5f - 1.0f;

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

                                ctrlEvent.handled = true;
                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        for (uint32_t k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMATABLE) == 0)
                                continue;

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            if (fMidiEventInCount >= kPluginMaxMidiEvents)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiInEvents[fMidiEventInCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = isSampleAccurate ? startTime : eventTime;
                            nativeEvent.data[0] = uint8_t(MIDI_STATUS_CONTROL_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            nativeEvent.data[1] = uint8_t(ctrlEvent.param);
                            nativeEvent.data[2] = uint8_t(ctrlEvent.normalizedValue*127.0f + 0.5f);
                            nativeEvent.size    = 3;
                        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        if (! ctrlEvent.handled)
                            checkForMidiLearn(event);
#endif
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        if (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
                        {
                            if (event.channel == pData->ctrlChannel)
                                nextBankId = ctrlEvent.param;
                        }
                        else if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            if (fMidiEventInCount >= kPluginMaxMidiEvents)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiInEvents[fMidiEventInCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = isSampleAccurate ? startTime : eventTime;
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
                                        {
                                            pData->postponeMidiProgramChangeRtEvent(true, k);
                                        }

                                        break;
                                    }
                                }
                            }
                        }
                        else if (pData->options & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)
                        {
                            if (fMidiEventInCount >= kPluginMaxMidiEvents)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiInEvents[fMidiEventInCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = isSampleAccurate ? startTime : eventTime;
                            nativeEvent.data[0] = uint8_t(MIDI_STATUS_PROGRAM_CHANGE | (event.channel & MIDI_CHANNEL_BIT));
                            nativeEvent.data[1] = uint8_t(ctrlEvent.param);
                            nativeEvent.size    = 2;
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            if (fMidiEventInCount >= kPluginMaxMidiEvents)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiInEvents[fMidiEventInCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = isSampleAccurate ? startTime : eventTime;
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
                                postponeRtAllNotesOff();
                            }
#endif

                            if (fMidiEventInCount >= kPluginMaxMidiEvents)
                                continue;

                            NativeMidiEvent& nativeEvent(fMidiInEvents[fMidiEventInCount++]);
                            carla_zeroStruct(nativeEvent);

                            nativeEvent.time    = isSampleAccurate ? startTime : eventTime;
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
                    if (fMidiEventInCount >= kPluginMaxMidiEvents)
                        continue;

                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > 4)
                        continue;
#ifdef CARLA_PROPER_CPP11_SUPPORT
                    static_assert(4 <= EngineMidiEvent::kDataSize, "Incorrect data");
#endif

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    if ((status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON) && (pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        continue;
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

                    NativeMidiEvent& nativeEvent(fMidiInEvents[fMidiEventInCount++]);
                    carla_zeroStruct(nativeEvent);

                    nativeEvent.port = midiEvent.port;
                    nativeEvent.time = isSampleAccurate ? startTime : eventTime;
                    nativeEvent.size = midiEvent.size;

                    nativeEvent.data[0] = uint8_t(status | (event.channel & MIDI_CHANNEL_BIT));
                    nativeEvent.data[1] = midiEvent.size >= 2 ? midiEvent.data[1] : 0;
                    nativeEvent.data[2] = midiEvent.size >= 3 ? midiEvent.data[2] : 0;
                    nativeEvent.data[3] = midiEvent.size == 4 ? midiEvent.data[3] : 0;

                    if (status == MIDI_STATUS_NOTE_ON)
                    {
                        pData->postponeNoteOnRtEvent(true, event.channel, midiEvent.data[1], midiEvent.data[2]);
                    }
                    else if (status == MIDI_STATUS_NOTE_OFF)
                    {
                        pData->postponeNoteOffRtEvent(true, event.channel, midiEvent.data[1]);
                    }
                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, cvIn, cvOut, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, cvIn, cvOut, frames, 0);

        } // End of Plugin processing (no events)

#ifndef BUILD_BRIDGE
        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (pData->event.portOut != nullptr)
        {
            float value, curValue;

            for (uint32_t k=0; k < pData->param.count; ++k)
            {
                if (pData->param.data[k].type != PARAMETER_OUTPUT)
                    continue;

                curValue = fDescriptor->get_parameter_value(fHandle, k);
                pData->param.ranges[k].fixValue(curValue);

                if (pData->param.data[k].mappedControlIndex > 0)
                {
                    value = pData->param.ranges[k].getNormalizedValue(curValue);
                    pData->event.portOut->writeControlEvent(0,
                                                            pData->param.data[k].midiChannel,
                                                            kEngineControlEventTypeParameter,
                                                            static_cast<uint16_t>(pData->param.data[k].mappedControlIndex),
                                                            -1,
                                                            value);
                }
            }
        } // End of Control Output
#endif
    }

    bool processSingle(const float* const* const audioIn, float** const audioOut,
                       const float* const* const cvIn, float** const cvOut,
                       const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0) {
            CARLA_SAFE_ASSERT_RETURN(audioIn != nullptr, false);
        }
        if (pData->audioOut.count > 0) {
            CARLA_SAFE_ASSERT_RETURN(audioOut != nullptr, false);
        }
        if (pData->cvIn.count > 0) {
            CARLA_SAFE_ASSERT_RETURN(cvIn != nullptr, false);
        }
        if (pData->cvOut.count > 0) {
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

        {
            for (uint32_t i=0; i < pData->audioIn.count; ++i)
                carla_copyFloats(fAudioAndCvInBuffers[i], audioIn[i]+timeOffset, frames);
            for (uint32_t i=0; i < pData->cvIn.count; ++i)
                carla_copyFloats(fAudioAndCvInBuffers[pData->audioIn.count+i], cvIn[i]+timeOffset, frames);

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(fAudioAndCvOutBuffers[i], frames);
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                carla_zeroFloats(fAudioAndCvOutBuffers[pData->audioOut.count+i], frames);
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        fIsProcessing = true;

        if (fHandle2 == nullptr)
        {
            fDescriptor->process(fHandle,
                                 fAudioAndCvInBuffers, fAudioAndCvOutBuffers, frames,
                                 fMidiInEvents, fMidiEventInCount);
        }
        else
        {
            fDescriptor->process(fHandle,
                                 (fAudioAndCvInBuffers != nullptr)  ? &fAudioAndCvInBuffers[0]  : nullptr,
                                 (fAudioAndCvOutBuffers != nullptr) ? &fAudioAndCvOutBuffers[0] : nullptr,
                                 frames, fMidiInEvents, fMidiEventInCount);

            fDescriptor->process(fHandle2,
                                 (fAudioAndCvInBuffers != nullptr)  ? &fAudioAndCvInBuffers[1]  : nullptr,
                                 (fAudioAndCvOutBuffers != nullptr) ? &fAudioAndCvOutBuffers[1] : nullptr,
                                 frames, fMidiInEvents, fMidiEventInCount);
        }

        fIsProcessing = false;

        if (fTimeInfo.playing)
            fTimeInfo.frame += frames;

        uint32_t i=0;
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0 && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0 && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f) && carla_isEqual(pData->postProc.balanceRight, 1.0f));

            bool isPair;
            float bufValue;
            float* const oldBufLeft = pData->postProc.extraBuffer;

            for (; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue = fAudioAndCvInBuffers[(pData->audioIn.count == 1) ? 0 : i][k];
                        fAudioAndCvOutBuffers[i][k] = (fAudioAndCvOutBuffers[i][k] * pData->postProc.dryWet) + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        carla_copyFloats(oldBufLeft, fAudioAndCvOutBuffers[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            fAudioAndCvOutBuffers[i][k]  = oldBufLeft[k]            * (1.0f - balRangeL);
                            fAudioAndCvOutBuffers[i][k] += fAudioAndCvOutBuffers[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            fAudioAndCvOutBuffers[i][k]  = fAudioAndCvOutBuffers[i][k] * balRangeR;
                            fAudioAndCvOutBuffers[i][k] += oldBufLeft[k]          * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        audioOut[i][k+timeOffset] = fAudioAndCvOutBuffers[i][k] * pData->postProc.volume;
                }
            }

        } // End of Post-processing
#else
        for (; i < pData->audioOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                audioOut[i][k+timeOffset] = fAudioAndCvOutBuffers[i][k];
        }
#endif
        // CV stuff too
        for (; i < pData->cvOut.count; ++i)
        {
            for (uint32_t k=0; k < frames; ++k)
                cvOut[i][k+timeOffset] = fAudioAndCvOutBuffers[pData->audioOut.count+i][k];
        }

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (pData->event.portOut != nullptr)
        {
            for (uint32_t k = 0; k < fMidiEventOutCount; ++k)
            {
                const uint8_t channel = uint8_t(MIDI_GET_CHANNEL_FROM_DATA(fMidiOutEvents[k].data));
                const uint8_t port    = fMidiOutEvents[k].port;

                if (fMidiOut.count > 1 && port < fMidiOut.count)
                    fMidiOut.ports[port]->writeMidiEvent(fMidiOutEvents[k].time+timeOffset, channel, fMidiOutEvents[k].size, fMidiOutEvents[k].data);
                else
                    pData->event.portOut->writeMidiEvent(fMidiOutEvents[k].time+timeOffset, channel, fMidiOutEvents[k].size, fMidiOutEvents[k].data);
            }
        }

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginNative::bufferSizeChanged(%i)", newBufferSize);

        for (uint32_t i=0; i < (pData->audioIn.count+pData->cvIn.count); ++i)
        {
            if (fAudioAndCvInBuffers[i] != nullptr)
                delete[] fAudioAndCvInBuffers[i];
            fAudioAndCvInBuffers[i] = new float[newBufferSize];
        }

        for (uint32_t i=0; i < (pData->audioOut.count+pData->cvOut.count); ++i)
        {
            if (fAudioAndCvOutBuffers[i] != nullptr)
                delete[] fAudioAndCvOutBuffers[i];
            fAudioAndCvOutBuffers[i] = new float[newBufferSize];
        }

        if (fCurBufferSize != newBufferSize)
        {
            fCurBufferSize = newBufferSize;

            if (fDescriptor != nullptr && fDescriptor->dispatcher != nullptr)
            {
                fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, static_cast<intptr_t>(newBufferSize), nullptr, 0.0f);

                if (fHandle2 != nullptr)
                    fDescriptor->dispatcher(fHandle2, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, static_cast<intptr_t>(newBufferSize), nullptr, 0.0f);
            }
        }

        CarlaPlugin::bufferSizeChanged(newBufferSize);
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
        CarlaPlugin::initBuffers();

        fMidiIn.initBuffers(pData->event.portIn);
        fMidiOut.initBuffers();
    }

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginNative::clearBuffers() - start");

        if (fAudioAndCvInBuffers != nullptr)
        {
            for (uint32_t i=0; i < (pData->audioIn.count+pData->cvIn.count); ++i)
            {
                if (fAudioAndCvInBuffers[i] != nullptr)
                {
                    delete[] fAudioAndCvInBuffers[i];
                    fAudioAndCvInBuffers[i] = nullptr;
                }
            }

            delete[] fAudioAndCvInBuffers;
            fAudioAndCvInBuffers = nullptr;
        }

        if (fAudioAndCvOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < (pData->audioOut.count+pData->cvOut.count); ++i)
            {
                if (fAudioAndCvOutBuffers[i] != nullptr)
                {
                    delete[] fAudioAndCvOutBuffers[i];
                    fAudioAndCvOutBuffers[i] = nullptr;
                }
            }

            delete[] fAudioAndCvOutBuffers;
            fAudioAndCvOutBuffers = nullptr;
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

        if (fDescriptor->dispatcher != nullptr)
        {
            uint8_t data[3] = {
                uint8_t(MIDI_STATUS_NOTE_ON | (channel & MIDI_CHANNEL_BIT)),
                note,
                velo
            };
            fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_UI_MIDI_EVENT,
                                    3, 0,
                                    data,
                                    0.0f);
        }
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

        if (fDescriptor->dispatcher != nullptr)
        {
            uint8_t data[3] = {
                uint8_t(MIDI_STATUS_NOTE_OFF | (channel & MIDI_CHANNEL_BIT)),
                note,
                0
            };
            fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_UI_MIDI_EVENT,
                                    3, 0,
                                    data,
                                    0.0f);
        }
    }

    // -------------------------------------------------------------------

    const NativeInlineDisplayImageSurface* renderInlineDisplay(const uint32_t width, const uint32_t height) const
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->hints & NATIVE_PLUGIN_HAS_INLINE_DISPLAY, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->render_inline_display, nullptr);
        CARLA_SAFE_ASSERT_RETURN(width > 0, nullptr);
        CARLA_SAFE_ASSERT_RETURN(height > 0, nullptr);

        return fDescriptor->render_inline_display(fHandle, width, height);
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

        if (fMidiEventOutCount == kPluginMaxMidiEvents)
        {
            carla_stdout("CarlaPluginNative::handleWriteMidiEvent(%p) - buffer full", event);
            return false;
        }

        std::memcpy(&fMidiOutEvents[fMidiEventOutCount++], event, sizeof(NativeMidiEvent));
        return true;
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
        pData->engine->callback(true, true, ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, 0, 0, 0, 0.0f, nullptr);
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

    intptr_t handleDispatcher(const NativeHostDispatcherOpcode opcode,
                              const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#ifdef DEBUG
        if (opcode != NATIVE_HOST_OPCODE_QUEUE_INLINE_DISPLAY && opcode != NATIVE_HOST_OPCODE_REQUEST_IDLE) {
            carla_debug("CarlaPluginNative::handleDispatcher(%i, %i, " P_INTPTR ", %p, %f)",
                        opcode, index, value, ptr, static_cast<double>(opt));
        }
#endif

        switch (opcode)
        {
        case NATIVE_HOST_OPCODE_NULL:
            break;

        case NATIVE_HOST_OPCODE_UPDATE_PARAMETER:
            // TODO
            pData->engine->callback(true, true, ENGINE_CALLBACK_UPDATE, pData->id, -1, 0, 0, 0.0f, nullptr);
            break;

        case NATIVE_HOST_OPCODE_UPDATE_MIDI_PROGRAM:
            // TODO
            pData->engine->callback(true, true, ENGINE_CALLBACK_UPDATE, pData->id, -1, 0, 0, 0.0f, nullptr);
            break;

        case NATIVE_HOST_OPCODE_RELOAD_PARAMETERS:
            reloadParameters(nullptr, nullptr);
            pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_PARAMETERS, pData->id, -1, 0, 0, 0.0f, nullptr);
            break;

        case NATIVE_HOST_OPCODE_RELOAD_MIDI_PROGRAMS:
            reloadPrograms(false);
            pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_PROGRAMS, pData->id, -1, 0, 0, 0.0f, nullptr);
            break;

        case NATIVE_HOST_OPCODE_RELOAD_ALL:
            reload();
            pData->engine->callback(true, true, ENGINE_CALLBACK_RELOAD_ALL, pData->id, -1, 0, 0, 0.0f, nullptr);
            break;

        case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
            pData->engine->callback(true, true, ENGINE_CALLBACK_UI_STATE_CHANGED, pData->id, -1, 0, 0, 0.0f, nullptr);
            fIsUiAvailable = false;
            break;

        case NATIVE_HOST_OPCODE_HOST_IDLE:
            pData->engine->callback(true, false, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
            break;

        case NATIVE_HOST_OPCODE_INTERNAL_PLUGIN:
            return 1;

        case NATIVE_HOST_OPCODE_QUEUE_INLINE_DISPLAY:
            switch (pData->engine->getProccessMode())
            {
            case ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
            case ENGINE_PROCESS_MODE_PATCHBAY:
                fInlineDisplayNeedsRedraw = true;
                break;
            default:
                break;
            }
            break;

        case NATIVE_HOST_OPCODE_UI_TOUCH_PARAMETER:
            CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);
            pData->engine->touchPluginParameter(pData->id, static_cast<uint32_t>(index), value != 0);
            break;

        case NATIVE_HOST_OPCODE_REQUEST_IDLE:
            fNeedsIdle = true;
            break;

        case NATIVE_HOST_OPCODE_GET_FILE_PATH:
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            {
                const EngineOptions& opts(pData->engine->getOptions());
                const char* const filetype = (const char*)ptr;
                const char* ret = nullptr;

                if (std::strcmp(filetype, "carla") == 0)
                {
                    ret = pData->engine->getCurrentProjectFilename();

                    if (fLastProjectFilename != ret)
                    {
                        fLastProjectFilename = ret;

                        bool found;
                        const size_t r = fLastProjectFilename.rfind(CARLA_OS_SEP, &found);
                        if (found)
                        {
                            fLastProjectFolder = ret;
                            fLastProjectFolder[r] = '\0';
                        }
                        else
                        {
                            fLastProjectFolder.clear();
                        }
                    }

                    ret = fLastProjectFolder.buffer();
                }

                else if (std::strcmp(filetype, "audio") == 0)
                    ret = opts.pathAudio;
                else if (std::strcmp(filetype, "midi") == 0)
                    ret = opts.pathMIDI;

                return static_cast<intptr_t>((uintptr_t)ret);
            }
            break;

        case NATIVE_HOST_OPCODE_UI_RESIZE:
        case NATIVE_HOST_OPCODE_PREVIEW_BUFFER_DATA:
            // unused here
            break;
        }

        return 0;

        // unused for now
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

    bool init(const CarlaPluginPtr plugin,
              const char* const name, const char* const label, const uint options)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (label == nullptr || label[0] == '\0')
        {
            pData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        sPluginInitializer.initIfNeeded();

        for (LinkedList<const NativePluginDescriptor*>::Itenerator it = gPluginDescriptors.begin2(); it.valid(); it.next())
        {
            fDescriptor = it.getValue(nullptr);
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

        /**/ if (std::strcmp(fDescriptor->label, "audiofile") == 0)
            pData->iconName = carla_strdup_safe("file");
        else if (std::strcmp(fDescriptor->label, "midifile") == 0)
            pData->iconName = carla_strdup_safe("file");

        else if (std::strcmp(fDescriptor->label, "3bandeq") == 0)
            pData->iconName = carla_strdup_safe("distrho");
        else if (std::strcmp(fDescriptor->label, "3bandsplitter") == 0)
            pData->iconName = carla_strdup_safe("distrho");
        else if (std::strcmp(fDescriptor->label, "kars") == 0)
            pData->iconName = carla_strdup_safe("distrho");
        else if (std::strcmp(fDescriptor->label, "nekobi") == 0)
            pData->iconName = carla_strdup_safe("distrho");
        else if (std::strcmp(fDescriptor->label, "pingpongpan") == 0)
            pData->iconName = carla_strdup_safe("distrho");

        // ---------------------------------------------------------------
        // set info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else if (fDescriptor->name != nullptr && fDescriptor->name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(fDescriptor->name);
        else
            pData->name = pData->engine->getUniquePluginName(label);

        {
            CARLA_ASSERT(fHost.uiName == nullptr);

            String uiName;

            if (pData->uiTitle.isNotEmpty())
            {
                uiName = pData->uiTitle;
            }
            else
            {
                uiName  = pData->name;
                uiName += " (GUI)";
            }

            fHost.uiName = uiName.getAndReleaseBuffer();
        }

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

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
        // set options

        bool hasMidiProgs = false;

        if (fDescriptor->get_midi_program_count != nullptr)
        {
            try {
                hasMidiProgs = fDescriptor->get_midi_program_count(fHandle) > 0;
            } catch (...) {}
        }

        pData->options = 0x0;

        if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
         else if (options & PLUGIN_OPTION_FIXED_BUFFERS)
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (pData->engine->getOptions().forceStereo)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;
        else if (options & PLUGIN_OPTION_FORCE_STEREO)
            pData->options |= PLUGIN_OPTION_FORCE_STEREO;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_CONTROL_CHANGES)
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
                pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_CHANNEL_PRESSURE)
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_NOTE_AFTERTOUCH)
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_PITCHBEND)
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF)
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        if (fDescriptor->midiIns > 0)
            if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
                pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;

        if (fDescriptor->supports & NATIVE_PLUGIN_SUPPORTS_PROGRAM_CHANGES)
        {
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PROGRAM_CHANGES))
                pData->options |= PLUGIN_OPTION_SEND_PROGRAM_CHANGES;

            // makes no sense for a plugin to set program changes supported, but it has no midi programs
            CARLA_SAFE_ASSERT(! hasMidiProgs);
        }
        else if (hasMidiProgs)
        {
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
                pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        }

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
    volatile bool fNeedsIdle;

    bool fInlineDisplayNeedsRedraw;
    int64_t fInlineDisplayLastRedrawTime;

    String fLastProjectFilename;
    String fLastProjectFolder;

    float**         fAudioAndCvInBuffers;
    float**         fAudioAndCvOutBuffers;
    uint32_t        fMidiEventInCount;
    uint32_t        fMidiEventOutCount;
    NativeMidiEvent fMidiInEvents[kPluginMaxMidiEvents];
    NativeMidiEvent fMidiOutEvents[kPluginMaxMidiEvents];

    int32_t  fCurMidiProgs[MAX_MIDI_CHANNELS];
    uint32_t fCurBufferSize;
    double   fCurSampleRate;

    NativePluginMidiInData  fMidiIn;
    NativePluginMidiOutData fMidiOut;

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

CarlaPluginPtr CarlaPlugin::newNative(const Initializer& init)
{
    carla_debug("CarlaPlugin::newNative({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.name, init.label, init.uniqueId);

    std::shared_ptr<CarlaPluginNative> plugin(new CarlaPluginNative(init.engine, init.id));

    if (! plugin->init(plugin, init.name, init.label, init.options))
        return nullptr;

    return plugin;
}

// used in CarlaStandalone.cpp
const void* carla_render_inline_display_internal(const CarlaPluginPtr& plugin, uint32_t width, uint32_t height);

const void* carla_render_inline_display_internal(const CarlaPluginPtr& plugin, uint32_t  width, uint32_t height)
{
    const std::shared_ptr<CarlaPluginNative>& nativePlugin((const std::shared_ptr<CarlaPluginNative>&)plugin);

    return nativePlugin->renderInlineDisplay(width, height);
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
