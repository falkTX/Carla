/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.h"

#ifdef BUILD_BRIDGE
# error This file should not be compiled if building bridge
#endif

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBase64Utils.hpp"
#include "CarlaBinaryUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaStateUtils.hpp"

#include "CarlaExternalUI.hpp"
#include "CarlaHost.h"
#include "CarlaNative.hpp"

#include "juce_audio_basics.h"

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
# include "juce_gui_basics.h"
#else
namespace juce {
# include "juce_events/messages/juce_Initialisation.h"
} // namespace juce
#endif

using juce::File;
using juce::FloatVectorOperations;
using juce::MemoryOutputStream;
using juce::ScopedPointer;
using juce::String;
using juce::XmlDocument;
using juce::XmlElement;

static bool gNeedsJuceHandling = false;
static int  gJuceReferenceCounter = 0;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

class CarlaEngineNativeUI : public CarlaExternalUI
{
public:
    CarlaEngineNativeUI(CarlaEngine* const engine)
        : fEngine(engine)
    {
        carla_debug("CarlaEngineNativeUI::CarlaEngineNativeUI(%p)", engine);
    }

    ~CarlaEngineNativeUI() noexcept override
    {
        carla_debug("CarlaEngineNativeUI::~CarlaEngineNativeUI()");
    }

protected:
    bool msgReceived(const char* const msg) noexcept override
    {
        if (CarlaExternalUI::msgReceived(msg))
            return true;

        bool ok = true;

        if (std::strcmp(msg, "set_engine_option") == 0)
        {
            uint32_t option;
            int32_t value;
            const char* valueStr = nullptr;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(option), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(value), true);
            readNextLineAsString(valueStr); // can be null

            try {
                fEngine->setOption(static_cast<EngineOption>(option), value, valueStr);
            } CARLA_SAFE_EXCEPTION("setOption");

            if (valueStr != nullptr)
                delete[] valueStr;
        }
        else if (std::strcmp(msg, "load_file") == 0)
        {
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename), true);

            try {
                ok = fEngine->loadFile(filename);
            } CARLA_SAFE_EXCEPTION("loadFile");

            delete[] filename;
        }
        else if (std::strcmp(msg, "load_project") == 0)
        {
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename), true);

            try {
                ok = fEngine->loadProject(filename);
            } CARLA_SAFE_EXCEPTION("loadProject");

            delete[] filename;
        }
        else if (std::strcmp(msg, "save_project") == 0)
        {
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename), true);

            try {
                ok = fEngine->saveProject(filename);
            } CARLA_SAFE_EXCEPTION("saveProject");

            delete[] filename;
        }
        else if (std::strcmp(msg, "patchbay_connect") == 0)
        {
            uint32_t groupA, portA, groupB, portB;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(groupA), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(portA), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(groupB), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(portB), true);

            try {
                ok = fEngine->patchbayConnect(groupA, portA, groupB, portB);
            } CARLA_SAFE_EXCEPTION("patchbayConnect");
        }
        else if (std::strcmp(msg, "patchbay_disconnect") == 0)
        {
            uint32_t connectionId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(connectionId), true);

            try {
                ok = fEngine->patchbayDisconnect(connectionId);
            } CARLA_SAFE_EXCEPTION("patchbayDisconnect");
        }
        else if (std::strcmp(msg, "patchbay_refresh") == 0)
        {
            try {
                ok = fEngine->patchbayRefresh(false);
            } CARLA_SAFE_EXCEPTION("patchbayRefresh");
        }
        else if (std::strcmp(msg, "transport_play") == 0)
        {
            fEngine->transportPlay();
        }
        else if (std::strcmp(msg, "transport_pause") == 0)
        {
            fEngine->transportPause();
        }
        else if (std::strcmp(msg, "transport_relocate") == 0)
        {
            uint64_t frame;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsULong(frame), true);

            fEngine->transportRelocate(frame);
        }
        else if (std::strcmp(msg, "add_plugin") == 0)
        {
            uint32_t btype, ptype;
            const char* filename = nullptr;
            const char* name;
            const char* label;
            int64_t uniqueId;
            uint options;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(btype), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(ptype), true);
            readNextLineAsString(filename); // can be null
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(name), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(label), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsLong(uniqueId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(options), true);

            if (filename != nullptr && std::strcmp(filename, "(null)") == 0)
            {
                delete[] filename;
                filename = nullptr;
            }

            if (std::strcmp(name, "(null)") == 0)
            {
                delete[] name;
                name = nullptr;
            }

            ok = fEngine->addPlugin(static_cast<BinaryType>(btype), static_cast<PluginType>(ptype),
                                    filename, name, label, uniqueId, nullptr, options);

            if (filename != nullptr)
                delete[] filename;
            if (name != nullptr)
                delete[] name;
            delete[] label;
        }
        else if (std::strcmp(msg, "remove_plugin") == 0)
        {
            uint32_t pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);

            ok = fEngine->removePlugin(pluginId);
        }
        else if (std::strcmp(msg, "remove_all_plugins") == 0)
        {
            ok = fEngine->removeAllPlugins();
        }
        else if (std::strcmp(msg, "rename_plugin") == 0)
        {
            uint32_t pluginId;
            const char* newName;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(newName), true);

            // TODO
            /*const char* name =*/ fEngine->renamePlugin(pluginId, newName);

            delete[] newName;
        }
        else if (std::strcmp(msg, "clone_plugin") == 0)
        {
            uint32_t pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);

            ok = fEngine->clonePlugin(pluginId);
        }
        else if (std::strcmp(msg, "replace_plugin") == 0)
        {
            uint32_t pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);

            ok = fEngine->replacePlugin(pluginId);
        }
        else if (std::strcmp(msg, "switch_plugins") == 0)
        {
            uint32_t pluginIdA, pluginIdB;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginIdA), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginIdB), true);

            ok = fEngine->switchPlugins(pluginIdA, pluginIdB);
        }
        else if (std::strcmp(msg, "load_plugin_state") == 0)
        {
            uint32_t pluginId;
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->loadStateFromFile(filename);

            delete[] filename;
        }
        else if (std::strcmp(msg, "save_plugin_state") == 0)
        {
            uint32_t pluginId;
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->saveStateToFile(filename);

            delete[] filename;
        }
        else if (std::strcmp(msg, "set_option") == 0)
        {
            uint32_t pluginId, option;
            bool yesNo;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(option), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(yesNo), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setOption(option, yesNo, false);
        }
        else if (std::strcmp(msg, "set_active") == 0)
        {
            uint32_t pluginId;
            bool onOff;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(onOff), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setActive(onOff, true, false);
        }
        else if (std::strcmp(msg, "set_drywet") == 0)
        {
            uint32_t pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setDryWet(value, true, false);
        }
        else if (std::strcmp(msg, "set_volume") == 0)
        {
            uint32_t pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setVolume(value, true, false);
        }
        else if (std::strcmp(msg, "set_balance_left") == 0)
        {
            uint32_t pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setBalanceLeft(value, true, false);
        }
        else if (std::strcmp(msg, "set_balance_right") == 0)
        {
            uint32_t pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setBalanceRight(value, true, false);
        }
        else if (std::strcmp(msg, "set_panning") == 0)
        {
            uint32_t pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setPanning(value, true, false);
        }
        else if (std::strcmp(msg, "set_ctrl_channel") == 0)
        {
            uint32_t pluginId;
            int32_t channel;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(channel), true);
            CARLA_SAFE_ASSERT_RETURN(channel >= -1 && channel < MAX_MIDI_CHANNELS, true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setCtrlChannel(int8_t(channel), true, false);
        }
        else if (std::strcmp(msg, "set_parameter_value") == 0)
        {
            uint32_t pluginId, parameterId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(parameterId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setParameterValue(parameterId, value, true, true, false);
        }
        else if (std::strcmp(msg, "set_parameter_midi_channel") == 0)
        {
            uint32_t pluginId, parameterId, channel;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(parameterId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(channel), true);
            CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setParameterMidiChannel(parameterId, static_cast<uint8_t>(channel), true, false);
        }
        else if (std::strcmp(msg, "set_parameter_midi_cc") == 0)
        {
            uint32_t pluginId, parameterId;
            int32_t cc;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(parameterId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(cc), true);
            CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < MAX_MIDI_CONTROL, true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setParameterMidiCC(parameterId, static_cast<int16_t>(cc), true, false);
        }
        else if (std::strcmp(msg, "set_program") == 0)
        {
            uint32_t pluginId;
            int32_t index;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(index), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
            {
                plugin->setProgram(index, true, true, false);
                _updateParamValues(plugin, pluginId);
            }
        }
        else if (std::strcmp(msg, "set_midi_program") == 0)
        {
            uint32_t pluginId;
            int32_t index;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(index), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
            {
                plugin->setMidiProgram(index, true, true, false);
                _updateParamValues(plugin, pluginId);
            }
        }
        else if (std::strcmp(msg, "set_custom_data") == 0)
        {
            uint32_t pluginId;
            const char* type;
            const char* key;
            const char* value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(type), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(key), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(value), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setCustomData(type, key, value, true);

            delete[] type;
            delete[] key;
            delete[] value;
        }
        else if (std::strcmp(msg, "set_chunk_data") == 0)
        {
            uint32_t pluginId;
            const char* cdata;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(cdata), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
            {
                std::vector<uint8_t> chunk(carla_getChunkFromBase64String(cdata));
                plugin->setChunkData(chunk.data(), chunk.size());
            }

            delete[] cdata;
        }
        else if (std::strcmp(msg, "prepare_for_save") == 0)
        {
            uint32_t pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->prepareForSave();
        }
        else if (std::strcmp(msg, "reset_parameters") == 0)
        {
            uint32_t pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->resetParameters();
        }
        else if (std::strcmp(msg, "randomize_parameters") == 0)
        {
            uint32_t pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->randomizeParameters();
        }
        else if (std::strcmp(msg, "send_midi_note") == 0)
        {
            uint32_t pluginId, channel, note, velocity;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(channel), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(note), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(velocity), true);
            CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, true);
            CARLA_SAFE_ASSERT_RETURN(note < MAX_MIDI_VALUE, true);
            CARLA_SAFE_ASSERT_RETURN(velocity < MAX_MIDI_VALUE, true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->sendMidiSingleNote(static_cast<uint8_t>(channel), static_cast<uint8_t>(note), static_cast<uint8_t>(velocity), true, true, false);
        }
        else if (std::strcmp(msg, "show_custom_ui") == 0)
        {
            uint32_t pluginId;
            bool yesNo;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(yesNo), true);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->showCustomUI(yesNo);
        }
        else
        {
            carla_stderr("CarlaEngineNativeUI::msgReceived : %s", msg);
            return false;
        }

        if (! ok)
        {
            const CarlaMutexLocker cml(getPipeLock());
            writeMessage("error\n", 6);
            writeAndFixMessage(fEngine->getLastError());
            flushMessages();
        }

        return true;
    }

private:
    CarlaEngine* const fEngine;

    void _updateParamValues(CarlaPlugin* const plugin, const uint32_t pluginId) const noexcept
    {
        for (uint32_t i=0, count=plugin->getParameterCount(); i<count; ++i)
            fEngine->callback(ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED, pluginId, i, 0, plugin->getParameterValue(i), nullptr);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineNativeUI)
};

// -----------------------------------------------------------------------

class CarlaEngineNative : public CarlaEngine
{
public:
    CarlaEngineNative(const NativeHostDescriptor* const host, const bool isPatchbay, const uint32_t inChan = 2, uint32_t outChan = 0)
        : CarlaEngine(),
          pHost(host),
          kIsPatchbay(isPatchbay),
          fIsActive(false),
          fIsRunning(false),
          fUiServer(this),
          fOptionsForced(false)
    {
        carla_debug("CarlaEngineNative::CarlaEngineNative()");

        CARLA_SAFE_ASSERT_INT(gJuceReferenceCounter >= 0, gJuceReferenceCounter);

        if (gNeedsJuceHandling && ++gJuceReferenceCounter == 1)
            juce::initialiseJuce_GUI();

        carla_zeroChars(fTmpBuf, STR_MAX+1);

        pData->bufferSize = pHost->get_buffer_size(pHost->handle);
        pData->sampleRate = pHost->get_sample_rate(pHost->handle);

        if (outChan == 0)
            outChan = inChan;

        // set-up engine
        if (kIsPatchbay)
        {
            pData->options.processMode         = ENGINE_PROCESS_MODE_PATCHBAY;
            pData->options.transportMode       = ENGINE_TRANSPORT_MODE_PLUGIN;
            pData->options.forceStereo         = false;
            pData->options.preferPluginBridges = false;
            pData->options.preferUiBridges     = false;
            init("Carla-Patchbay");
            pData->graph.create(inChan, outChan);
        }
        else
        {
            CARLA_SAFE_ASSERT(inChan == 2);
            CARLA_SAFE_ASSERT(outChan == 2);
            pData->options.processMode         = ENGINE_PROCESS_MODE_CONTINUOUS_RACK;
            pData->options.transportMode       = ENGINE_TRANSPORT_MODE_PLUGIN;
            pData->options.forceStereo         = true;
            pData->options.preferPluginBridges = false;
            pData->options.preferUiBridges     = false;
            init("Carla-Rack");
            pData->graph.create(0, 0);
        }

        if (pData->options.resourceDir != nullptr)
            delete[] pData->options.resourceDir;
        if (pData->options.binaryDir != nullptr)
            delete[] pData->options.binaryDir;

        pData->options.resourceDir = carla_strdup(pHost->resourceDir);
        pData->options.binaryDir   = carla_strdup(carla_get_library_folder());

        setCallback(_ui_server_callback, this);
    }

    ~CarlaEngineNative() override
    {
        CARLA_SAFE_ASSERT(! fIsActive);
        carla_debug("CarlaEngineNative::~CarlaEngineNative() - START");

        pData->aboutToClose = true;
        fIsRunning = false;

        removeAllPlugins();
        //runPendingRtEvents();
        close();

        pData->graph.destroy();

        if (gNeedsJuceHandling && --gJuceReferenceCounter == 0)
            juce::shutdownJuce_GUI();

        carla_debug("CarlaEngineNative::~CarlaEngineNative() - END");
    }

protected:
    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineNative::init(\"%s\")", clientName);

        fIsRunning = true;

        if (! pData->init(clientName))
        {
            close();
            setLastError("Failed to init internal data");
            return false;
        }

        pData->bufferSize = pHost->get_buffer_size(pHost->handle);
        pData->sampleRate = pHost->get_sample_rate(pHost->handle);

        return true;
    }

    bool close() override
    {
        fIsRunning = false;
        CarlaEngine::close();
        return true;
    }

    bool isRunning() const noexcept override
    {
        return fIsRunning;
    }

    bool isOffline() const noexcept override
    {
        return pHost->is_offline(pHost->handle);
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypePlugin;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "Plugin";
    }

    void callback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr) noexcept override
    {
        CarlaEngine::callback(action, pluginId, value1, value2, value3, valueStr);

        if (action == ENGINE_CALLBACK_IDLE && ! pData->aboutToClose)
            pHost->dispatcher(pHost->handle, NATIVE_HOST_OPCODE_HOST_IDLE, 0, 0, nullptr, 0.0f);
    }

    // -------------------------------------------------------------------

    const char* renamePlugin(const uint id, const char* const newName) override
    {
        if (const char* const retName = CarlaEngine::renamePlugin(id, newName))
        {
            uiServerCallback(ENGINE_CALLBACK_PLUGIN_RENAMED, id, 0, 0, 0.0f, retName);
            return retName;
        }

        return nullptr;
    }

    // -------------------------------------------------------------------

    void bufferSizeChanged(const uint32_t newBufferSize)
    {
        if (pData->bufferSize == newBufferSize)
            return;

        {
            const CarlaMutexLocker cml(fUiServer.getPipeLock());

            fUiServer.writeAndFixMessage("buffer-size");
            std::sprintf(fTmpBuf, "%i\n", newBufferSize);
            fUiServer.writeMessage(fTmpBuf);
            fUiServer.flushMessages();
        }

        pData->bufferSize = newBufferSize;
        CarlaEngine::bufferSizeChanged(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate)
    {
        if (carla_isEqual(pData->sampleRate, newSampleRate))
            return;

        {
            const CarlaMutexLocker cml(fUiServer.getPipeLock());
            const ScopedLocale csl;

            fUiServer.writeAndFixMessage("sample-rate");
            std::sprintf(fTmpBuf, "%f\n", newSampleRate);
            fUiServer.writeMessage(fTmpBuf);
            fUiServer.flushMessages();
        }

        pData->sampleRate = newSampleRate;
        CarlaEngine::sampleRateChanged(newSampleRate);
    }

    // -------------------------------------------------------------------

    void uiServerSendPluginInfo(CarlaPlugin* const plugin)
    {
        const CarlaMutexLocker cml(fUiServer.getPipeLock());

        const uint pluginId(plugin->getId());

        std::sprintf(fTmpBuf, "PLUGIN_INFO_%i\n", pluginId);
        fUiServer.writeMessage(fTmpBuf);

        std::sprintf(fTmpBuf, "%i:%i:%i:" P_INT64 ":%i:%i\n", plugin->getType(), plugin->getCategory(), plugin->getHints(), plugin->getUniqueId(), plugin->getOptionsAvailable(), plugin->getOptionsEnabled());
        fUiServer.writeMessage(fTmpBuf);

        if (const char* const filename = plugin->getFilename())
        {
            std::sprintf(fTmpBuf, "%s", filename);
            fUiServer.writeAndFixMessage(fTmpBuf);
        }
        else
            fUiServer.writeMessage("\n");

        if (const char* const name = plugin->getName())
        {
            std::sprintf(fTmpBuf, "%s", name);
            fUiServer.writeAndFixMessage(fTmpBuf);
        }
        else
            fUiServer.writeMessage("\n");

        if (const char* const iconName = plugin->getIconName())
        {
            std::sprintf(fTmpBuf, "%s", iconName);
            fUiServer.writeAndFixMessage(fTmpBuf);
        }
        else
            fUiServer.writeMessage("\n");

        plugin->getRealName(fTmpBuf);
        fUiServer.writeAndFixMessage(fTmpBuf);

        plugin->getLabel(fTmpBuf);
        fUiServer.writeAndFixMessage(fTmpBuf);

        plugin->getMaker(fTmpBuf);
        fUiServer.writeAndFixMessage(fTmpBuf);

        plugin->getCopyright(fTmpBuf);
        fUiServer.writeAndFixMessage(fTmpBuf);

        std::sprintf(fTmpBuf, "AUDIO_COUNT_%i:%i:%i\n", pluginId, plugin->getAudioInCount(), plugin->getAudioOutCount());
        fUiServer.writeMessage(fTmpBuf);

        std::sprintf(fTmpBuf, "MIDI_COUNT_%i:%i:%i\n", pluginId, plugin->getMidiInCount(), plugin->getMidiOutCount());
        fUiServer.writeMessage(fTmpBuf);

        fUiServer.flushMessages();
    }

    void uiServerSendPluginParameters(CarlaPlugin* const plugin)
    {
        const CarlaMutexLocker cml(fUiServer.getPipeLock());
        const ScopedLocale csl;

        const uint pluginId(plugin->getId());

        for (int32_t i=PARAMETER_ACTIVE; i>PARAMETER_MAX; --i)
        {
            std::sprintf(fTmpBuf, "PARAMVAL_%i:%i\n", pluginId, i);
            fUiServer.writeMessage(fTmpBuf);
            std::sprintf(fTmpBuf, "%f\n", plugin->getInternalParameterValue(i));
            fUiServer.writeMessage(fTmpBuf);
            fUiServer.flushMessages();
        }

        uint32_t ins, outs, count;
        plugin->getParameterCountInfo(ins, outs);
        count = plugin->getParameterCount();
        std::sprintf(fTmpBuf, "PARAMETER_COUNT_%i:%i:%i:%i\n", pluginId, ins, outs, count);
        fUiServer.writeMessage(fTmpBuf);

        for (uint32_t i=0; i<count; ++i)
        {
            const ParameterData& paramData(plugin->getParameterData(i));
            const ParameterRanges& paramRanges(plugin->getParameterRanges(i));

            std::sprintf(fTmpBuf, "PARAMETER_DATA_%i:%i\n", pluginId, i);
            fUiServer.writeMessage(fTmpBuf);
            std::sprintf(fTmpBuf, "%i:%i:%i:%i\n", paramData.type, paramData.hints, paramData.midiChannel, paramData.midiCC);
            fUiServer.writeMessage(fTmpBuf);
            plugin->getParameterName(i, fTmpBuf);
            fUiServer.writeAndFixMessage(fTmpBuf);
            plugin->getParameterUnit(i, fTmpBuf);
            fUiServer.writeAndFixMessage(fTmpBuf);

            std::sprintf(fTmpBuf, "PARAMETER_RANGES_%i:%i\n", pluginId, i);
            fUiServer.writeMessage(fTmpBuf);
            std::sprintf(fTmpBuf, "%f:%f:%f:%f:%f:%f\n", paramRanges.def, paramRanges.min, paramRanges.max, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            fUiServer.writeMessage(fTmpBuf);

            std::sprintf(fTmpBuf, "PARAMVAL_%i:%i\n", pluginId, i);
            fUiServer.writeMessage(fTmpBuf);
            std::sprintf(fTmpBuf, "%f\n", plugin->getParameterValue(i));
            fUiServer.writeMessage(fTmpBuf);
        }

        fUiServer.flushMessages();
    }

    void uiServerSendPluginPrograms(CarlaPlugin* const plugin)
    {
        const CarlaMutexLocker cml(fUiServer.getPipeLock());

        const uint pluginId(plugin->getId());

        uint32_t count = plugin->getProgramCount();
        std::sprintf(fTmpBuf, "PROGRAM_COUNT_%i:%i:%i\n", pluginId, count, plugin->getCurrentProgram());
        fUiServer.writeMessage(fTmpBuf);

        for (uint32_t i=0; i<count; ++i)
        {
            std::sprintf(fTmpBuf, "PROGRAM_NAME_%i:%i\n", pluginId, i);
            fUiServer.writeMessage(fTmpBuf);
            plugin->getProgramName(i, fTmpBuf);
            fUiServer.writeAndFixMessage(fTmpBuf);
        }

        fUiServer.flushMessages();

        count = plugin->getMidiProgramCount();
        std::sprintf(fTmpBuf, "MIDI_PROGRAM_COUNT_%i:%i:%i\n", pluginId, count, plugin->getCurrentMidiProgram());
        fUiServer.writeMessage(fTmpBuf);

        for (uint32_t i=0; i<count; ++i)
        {
            std::sprintf(fTmpBuf, "MIDI_PROGRAM_DATA_%i:%i\n", pluginId, i);
            fUiServer.writeMessage(fTmpBuf);

            const MidiProgramData& mpData(plugin->getMidiProgramData(i));
            std::sprintf(fTmpBuf, "%i:%i\n", mpData.bank, mpData.program);
            fUiServer.writeMessage(fTmpBuf);
            std::sprintf(fTmpBuf, "%s", mpData.name);
            fUiServer.writeAndFixMessage(fTmpBuf);
        }

        fUiServer.flushMessages();
    }

    void uiServerSendPluginProperties(CarlaPlugin* const plugin)
    {
        const CarlaMutexLocker cml(fUiServer.getPipeLock());

        const uint pluginId(plugin->getId());

        uint32_t count = plugin->getCustomDataCount();
        std::sprintf(fTmpBuf, "CUSTOM_DATA_COUNT_%i:%i\n", pluginId, count);
        fUiServer.writeMessage(fTmpBuf);

        for (uint32_t i=0; i<count; ++i)
        {
            const CustomData& customData(plugin->getCustomData(i));
            CARLA_SAFE_ASSERT_CONTINUE(customData.isValid());

            if (std::strcmp(customData.type, CUSTOM_DATA_TYPE_PROPERTY) != 0)
                continue;

            std::sprintf(fTmpBuf, "CUSTOM_DATA_%i:%i\n", pluginId, i);
            fUiServer.writeMessage(fTmpBuf);

            fUiServer.writeAndFixMessage(customData.type);
            fUiServer.writeAndFixMessage(customData.key);
            fUiServer.writeAndFixMessage(customData.value);
        }

        fUiServer.flushMessages();
    }

    void uiServerCallback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        if (! fIsRunning)
            return;
        if (! fUiServer.isPipeRunning())
            return;

        CarlaPlugin* plugin;

        switch (action)
        {
        case ENGINE_CALLBACK_UPDATE:
            plugin = getPlugin(pluginId);

            if (plugin != nullptr && plugin->isEnabled())
            {
                CARLA_SAFE_ASSERT_BREAK(plugin->getId() == pluginId);
                uiServerSendPluginProperties(plugin);
            }
            break;

        case ENGINE_CALLBACK_RELOAD_INFO:
            plugin = getPlugin(pluginId);

            if (plugin != nullptr && plugin->isEnabled())
            {
                CARLA_SAFE_ASSERT_BREAK(plugin->getId() == pluginId);
                uiServerSendPluginInfo(plugin);
            }
            break;

        case ENGINE_CALLBACK_RELOAD_PARAMETERS:
            plugin = getPlugin(pluginId);

            if (plugin != nullptr && plugin->isEnabled())
            {
                CARLA_SAFE_ASSERT_BREAK(plugin->getId() == pluginId);
                uiServerSendPluginParameters(plugin);
            }
            break;

        case ENGINE_CALLBACK_RELOAD_PROGRAMS:
            plugin = getPlugin(pluginId);

            if (plugin != nullptr && plugin->isEnabled())
            {
                CARLA_SAFE_ASSERT_BREAK(plugin->getId() == pluginId);
                uiServerSendPluginPrograms(plugin);
            }
            break;

        case ENGINE_CALLBACK_RELOAD_ALL:
        case ENGINE_CALLBACK_PLUGIN_ADDED:
            plugin = getPlugin(pluginId);

            if (plugin != nullptr && plugin->isEnabled())
            {
                CARLA_SAFE_ASSERT_BREAK(plugin->getId() == pluginId);
                uiServerSendPluginInfo(plugin);
                uiServerSendPluginParameters(plugin);
                uiServerSendPluginPrograms(plugin);
                uiServerSendPluginProperties(plugin);
            }
            break;

        default:
            break;
        }

        const CarlaMutexLocker cml(fUiServer.getPipeLock());
        const ScopedLocale csl;

        std::sprintf(fTmpBuf, "ENGINE_CALLBACK_%i\n", int(action));
        fUiServer.writeMessage(fTmpBuf);

        std::sprintf(fTmpBuf, "%u\n", pluginId);
        fUiServer.writeMessage(fTmpBuf);

        std::sprintf(fTmpBuf, "%i\n", value1);
        fUiServer.writeMessage(fTmpBuf);

        std::sprintf(fTmpBuf, "%i\n", value2);
        fUiServer.writeMessage(fTmpBuf);

        std::sprintf(fTmpBuf, "%f\n", value3);
        fUiServer.writeMessage(fTmpBuf);

        fUiServer.writeAndFixMessage(valueStr != nullptr ? valueStr : "");

        fUiServer.flushMessages();
    }

    void uiServerInfo()
    {
        CARLA_SAFE_ASSERT_RETURN(fIsRunning,);
        CARLA_SAFE_ASSERT_RETURN(fUiServer.isPipeRunning(),);

        const CarlaMutexLocker cml(fUiServer.getPipeLock());

#ifdef HAVE_LIBLO
        fUiServer.writeAndFixMessage("osc-urls");
        fUiServer.writeAndFixMessage(pData->osc.getServerPathTCP());
        fUiServer.writeAndFixMessage(pData->osc.getServerPathUDP());
#endif

        fUiServer.writeAndFixMessage("max-plugin-number");
        std::sprintf(fTmpBuf, "%i\n", pData->maxPluginNumber);
        fUiServer.writeMessage(fTmpBuf);

        fUiServer.writeAndFixMessage("buffer-size");
        std::sprintf(fTmpBuf, "%i\n", pData->bufferSize);
        fUiServer.writeMessage(fTmpBuf);

        const ScopedLocale csl;

        fUiServer.writeAndFixMessage("sample-rate");
        std::sprintf(fTmpBuf, "%f\n", pData->sampleRate);
        fUiServer.writeMessage(fTmpBuf);

        fUiServer.flushMessages();
    }

    void uiServerOptions()
    {
        CARLA_SAFE_ASSERT_RETURN(fIsRunning,);
        CARLA_SAFE_ASSERT_RETURN(fUiServer.isPipeRunning(),);

        const EngineOptions& options(pData->options);
        const CarlaMutexLocker cml(fUiServer.getPipeLock());

        const char* const optionsForcedStr(fOptionsForced ? "true\n" : "false\n");
        const std::size_t optionsForcedStrSize(fOptionsForced ? 5 : 6);

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_PROCESS_MODE);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        std::sprintf(fTmpBuf, "%i\n", options.processMode);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_TRANSPORT_MODE);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        std::sprintf(fTmpBuf, "%i\n", options.transportMode);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_FORCE_STEREO);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        fUiServer.writeMessage(options.forceStereo ? "true\n" : "false\n");
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_PREFER_PLUGIN_BRIDGES);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        fUiServer.writeMessage(options.preferPluginBridges ? "true\n" : "false\n");
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_PREFER_UI_BRIDGES);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        fUiServer.writeMessage(options.preferUiBridges ? "true\n" : "false\n");
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_UIS_ALWAYS_ON_TOP);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        fUiServer.writeMessage(options.uisAlwaysOnTop ? "true\n" : "false\n");
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_MAX_PARAMETERS);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        std::sprintf(fTmpBuf, "%i\n", options.maxParameters);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_UI_BRIDGES_TIMEOUT);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage(optionsForcedStr, optionsForcedStrSize);
        std::sprintf(fTmpBuf, "%i\n", options.uiBridgesTimeout);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_PATH_BINARIES);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage("true\n", 5);
        std::sprintf(fTmpBuf, "%s\n", options.binaryDir);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.flushMessages();

        std::sprintf(fTmpBuf, "ENGINE_OPTION_%i\n", ENGINE_OPTION_PATH_RESOURCES);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.writeMessage("true\n", 5);
        std::sprintf(fTmpBuf, "%s\n", options.resourceDir);
        fUiServer.writeMessage(fTmpBuf);
        fUiServer.flushMessages();
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
            return plugin->getParameterCount();

        return 0;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (index < plugin->getParameterCount())
            {
                static NativeParameter param;
                static char strBufName[STR_MAX+1];
                static char strBufUnit[STR_MAX+1];

                const ParameterData& paramData(plugin->getParameterData(index));
                const ParameterRanges& paramRanges(plugin->getParameterRanges(index));

                plugin->getParameterName(index, strBufName);
                plugin->getParameterUnit(index, strBufUnit);

                uint hints = 0x0;

                if (paramData.hints & PARAMETER_IS_BOOLEAN)
                    hints |= NATIVE_PARAMETER_IS_BOOLEAN;
                if (paramData.hints & PARAMETER_IS_INTEGER)
                    hints |= NATIVE_PARAMETER_IS_INTEGER;
                if (paramData.hints & PARAMETER_IS_LOGARITHMIC)
                    hints |= NATIVE_PARAMETER_IS_LOGARITHMIC;
                if (paramData.hints & PARAMETER_IS_AUTOMABLE)
                    hints |= NATIVE_PARAMETER_IS_AUTOMABLE;
                if (paramData.hints & PARAMETER_USES_SAMPLERATE)
                    hints |= NATIVE_PARAMETER_USES_SAMPLE_RATE;
                if (paramData.hints & PARAMETER_USES_SCALEPOINTS)
                    hints |= NATIVE_PARAMETER_USES_SCALEPOINTS;

                if (paramData.type == PARAMETER_INPUT || paramData.type == PARAMETER_OUTPUT)
                {
                    if (paramData.hints & PARAMETER_IS_ENABLED)
                        hints |= NATIVE_PARAMETER_IS_ENABLED;
                    if (paramData.type == PARAMETER_OUTPUT)
                        hints |= NATIVE_PARAMETER_IS_OUTPUT;
                }

                param.hints = static_cast<NativeParameterHints>(hints);
                param.name  = strBufName;
                param.unit  = strBufUnit;
                param.ranges.def = paramRanges.def;
                param.ranges.min = paramRanges.min;
                param.ranges.max = paramRanges.max;
                param.ranges.step = paramRanges.step;
                param.ranges.stepSmall = paramRanges.stepSmall;
                param.ranges.stepLarge = paramRanges.stepLarge;
                param.scalePointCount = 0; // TODO
                param.scalePoints = nullptr;

                return &param;
            }
        }

        return nullptr;
    }

    float getParameterValue(const uint32_t index) const
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (index < plugin->getParameterCount())
                return plugin->getParameterValue(index);
        }

        return 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() const
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
            return plugin->getMidiProgramCount();

        return 0;
    }

    const NativeMidiProgram* getMidiProgramInfo(const uint32_t index) const
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (index < plugin->getMidiProgramCount())
            {
                static NativeMidiProgram midiProg;

                {
                    const MidiProgramData& midiProgData(plugin->getMidiProgramData(index));

                    midiProg.bank    = midiProgData.bank;
                    midiProg.program = midiProgData.program;
                    midiProg.name    = midiProgData.name;
                }

                return &midiProg;
            }
        }

        return nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value)
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (index < plugin->getParameterCount())
                plugin->setParameterValue(index, value, false, false, false);
        }
    }

    void setMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program)
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
            plugin->setMidiProgramById(bank, program, false, false, false);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
#if 0
        for (uint i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            plugin->setActive(true, true, false);
        }
#endif
        fIsActive = true;
    }

    void deactivate()
    {
        fIsActive = false;
#if 0
        for (uint i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            plugin->setActive(false, true, false);
        }
#endif

        // just in case
        //runPendingRtEvents();
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames,
                 const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount)
    {
        const PendingRtEventsRunner prt(this);

        // ---------------------------------------------------------------
        // Time Info

        const NativeTimeInfo* const timeInfo(pHost->get_time_info(pHost->handle));

        pData->timeInfo.playing = timeInfo->playing;
        pData->timeInfo.frame   = timeInfo->frame;
        pData->timeInfo.usecs   = timeInfo->usecs;
        pData->timeInfo.valid   = 0x0;

        if (timeInfo->bbt.valid)
        {
            pData->timeInfo.valid |= EngineTimeInfo::kValidBBT;

            pData->timeInfo.bbt.bar = timeInfo->bbt.bar;
            pData->timeInfo.bbt.beat = timeInfo->bbt.beat;
            pData->timeInfo.bbt.tick = timeInfo->bbt.tick;
            pData->timeInfo.bbt.barStartTick = timeInfo->bbt.barStartTick;

            pData->timeInfo.bbt.beatsPerBar = timeInfo->bbt.beatsPerBar;
            pData->timeInfo.bbt.beatType = timeInfo->bbt.beatType;

            pData->timeInfo.bbt.ticksPerBeat = timeInfo->bbt.ticksPerBeat;
            pData->timeInfo.bbt.beatsPerMinute = timeInfo->bbt.beatsPerMinute;
        }

        // ---------------------------------------------------------------
        // Do nothing if no plugins and rack mode

        if (pData->curPluginCount == 0 && ! kIsPatchbay)
        {
            if (outBuffer[0] != inBuffer[0])
                FloatVectorOperations::copy(outBuffer[0], inBuffer[0], static_cast<int>(frames));

            if (outBuffer[1] != inBuffer[1])
                FloatVectorOperations::copy(outBuffer[1], inBuffer[1], static_cast<int>(frames));

            for (uint32_t i=0; i < midiEventCount; ++i)
            {
                if (! pHost->write_midi_event(pHost->handle, &midiEvents[i]))
                    break;
            }
            return;
        }

        // ---------------------------------------------------------------
        // initialize events

        carla_zeroStructs(pData->events.in,  kMaxEngineEventInternalCount);
        carla_zeroStructs(pData->events.out, kMaxEngineEventInternalCount);

        // ---------------------------------------------------------------
        // events input (before processing)

        {
            uint32_t engineEventIndex = 0;

            for (uint32_t i=0; i < midiEventCount && engineEventIndex < kMaxEngineEventInternalCount; ++i)
            {
                const NativeMidiEvent& midiEvent(midiEvents[i]);
                EngineEvent&           engineEvent(pData->events.in[engineEventIndex++]);

                engineEvent.time = midiEvent.time;
                engineEvent.fillFromMidiData(midiEvent.size, midiEvent.data, 0);

                if (engineEventIndex >= kMaxEngineEventInternalCount)
                    break;
            }
        }

        if (kIsPatchbay)
        {
            // -----------------------------------------------------------
            // process

            pData->graph.process(pData, inBuffer, outBuffer, frames);
        }
        else
        {
            // -----------------------------------------------------------
            // create audio buffers

            const float* inBuf[2]  = {  inBuffer[0],  inBuffer[1] };
            /* */ float* outBuf[2] = { outBuffer[0], outBuffer[1] };

            // -----------------------------------------------------------
            // process

            pData->graph.processRack(pData, inBuf, outBuf, frames);
        }

        // ---------------------------------------------------------------
        // events output (after processing)

        carla_zeroStructs(pData->events.in, kMaxEngineEventInternalCount);

        {
            NativeMidiEvent midiEvent;

            for (uint32_t i=0; i < kMaxEngineEventInternalCount; ++i)
            {
                const EngineEvent& engineEvent(pData->events.out[i]);

                if (engineEvent.type == kEngineEventTypeNull)
                    break;

                midiEvent.time = engineEvent.time;

                if (engineEvent.type == CarlaBackend::kEngineEventTypeControl)
                {
                    midiEvent.port = 0;
                    engineEvent.ctrl.convertToMidiData(engineEvent.channel, midiEvent.size, midiEvent.data);
                }
                else if (engineEvent.type == kEngineEventTypeMidi)
                {
                    if (engineEvent.midi.size > 4 || engineEvent.midi.dataExt != nullptr)
                        continue;

                    midiEvent.port = engineEvent.midi.port;
                    midiEvent.size = engineEvent.midi.size;

                    midiEvent.data[0] = static_cast<uint8_t>(engineEvent.midi.data[0] + engineEvent.channel);

                    for (uint8_t j=1; j < midiEvent.size; ++j)
                        midiEvent.data[j] = engineEvent.midi.data[j];
                }
                else
                {
                    carla_stderr("Unknown event type...");
                    continue;
                }

                pHost->write_midi_event(pHost->handle, &midiEvent);
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show)
    {
        if (show)
        {
            if (fUiServer.isPipeRunning())
            {
                fUiServer.writeFocusMessage();
                return;
            }

            CarlaString path(pHost->resourceDir);

            if (kIsPatchbay)
                path += CARLA_OS_SEP_STR "carla-plugin-patchbay";
            else
                path += CARLA_OS_SEP_STR "carla-plugin";
// #ifdef CARLA_OS_WIN
//             path += ".exe";
// #endif
            carla_stdout("Trying to start carla-plugin using \"%s\"", path.buffer());

            fUiServer.setData(path, pData->sampleRate, pHost->uiName);

            if (! fUiServer.startPipeServer(false))
            {
                pHost->dispatcher(pHost->handle, NATIVE_HOST_OPCODE_UI_UNAVAILABLE, 0, 0, nullptr, 0.0f);
                return;
            }

            uiServerInfo();
            uiServerOptions();
            uiServerCallback(ENGINE_CALLBACK_ENGINE_STARTED, 0, pData->options.processMode, pData->options.transportMode, 0.0f, "Plugin");

            fUiServer.writeShowMessage();

            for (uint i=0; i < pData->curPluginCount; ++i)
            {
                CarlaPlugin* const plugin(pData->plugins[i].plugin);

                if (plugin != nullptr && plugin->isEnabled())
                {
                    uiServerCallback(ENGINE_CALLBACK_PLUGIN_ADDED, i, 0, 0, 0.0f, plugin->getName());
                }
            }

            if (kIsPatchbay)
                patchbayRefresh(false);
        }
        else
        {
            fUiServer.stopPipeServer(2000);

            // hide all custom uis
            for (uint i=0; i < pData->curPluginCount; ++i)
            {
                CarlaPlugin* const plugin(pData->plugins[i].plugin);

                if (plugin != nullptr && plugin->isEnabled())
                {
                    if (plugin->getHints() & PLUGIN_HAS_CUSTOM_UI)
                    {
                        try {
                            plugin->showCustomUI(false);
                        } CARLA_SAFE_EXCEPTION_CONTINUE("Plugin showCustomUI (hide)");
                    }
                }
            }
        }
    }

    void uiIdle()
    {
        for (uint i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin != nullptr && plugin->isEnabled())
            {
                if (plugin->getHints() & PLUGIN_HAS_CUSTOM_UI)
                {
                    try {
                        plugin->uiIdle();
                    } CARLA_SAFE_EXCEPTION_CONTINUE("Plugin uiIdle");
                }
            }
        }

        if (fUiServer.isPipeRunning())
        {
            fUiServer.idlePipe();

            const CarlaMutexLocker cml(fUiServer.getPipeLock());
#ifndef CARLA_OS_WIN
            const EngineTimeInfo& timeInfo(pData->timeInfo);
            const ScopedLocale csl;

            // send transport
            fUiServer.writeAndFixMessage("transport");
            fUiServer.writeMessage(timeInfo.playing ? "true\n" : "false\n");

            if (timeInfo.valid & EngineTimeInfo::kValidBBT)
            {
                std::sprintf(fTmpBuf, P_UINT64 ":%i:%i:%i\n", timeInfo.frame, timeInfo.bbt.bar, timeInfo.bbt.beat, timeInfo.bbt.tick);
                fUiServer.writeMessage(fTmpBuf);
                std::sprintf(fTmpBuf, "%f\n", timeInfo.bbt.beatsPerMinute);
                fUiServer.writeMessage(fTmpBuf);
            }
            else
            {
                std::sprintf(fTmpBuf, P_UINT64 ":0:0:0\n", timeInfo.frame);
                fUiServer.writeMessage(fTmpBuf);
                fUiServer.writeMessage("0.0\n");
            }

            fUiServer.flushMessages();
#endif

            // send peaks and param outputs for all plugins
            for (uint i=0; i < pData->curPluginCount; ++i)
            {
                const EnginePluginData& plugData(pData->plugins[i]);
                const CarlaPlugin* const plugin(pData->plugins[i].plugin);

                std::sprintf(fTmpBuf, "PEAKS_%i\n", i);
                fUiServer.writeMessage(fTmpBuf);

                std::sprintf(fTmpBuf, "%f:%f:%f:%f\n", plugData.insPeak[0], plugData.insPeak[1], plugData.outsPeak[0], plugData.outsPeak[1]);
                fUiServer.writeMessage(fTmpBuf);
                fUiServer.flushMessages();

                for (uint32_t j=0, count=plugin->getParameterCount(); j < count; ++j)
                {
                    if (! plugin->isParameterOutput(j))
                        continue;

                    std::sprintf(fTmpBuf, "PARAMVAL_%i:%i\n", i, j);
                    fUiServer.writeMessage(fTmpBuf);
                    std::sprintf(fTmpBuf, "%f\n", plugin->getParameterValue(j));
                    fUiServer.writeMessage(fTmpBuf);
                    fUiServer.flushMessages();
                }
            }
        }

        switch (fUiServer.getAndResetUiState())
        {
        case CarlaExternalUI::UiNone:
        case CarlaExternalUI::UiShow:
            break;
        case CarlaExternalUI::UiCrashed:
            pHost->dispatcher(pHost->handle, NATIVE_HOST_OPCODE_UI_UNAVAILABLE, 0, 0, nullptr, 0.0f);
            break;
        case CarlaExternalUI::UiHide:
            pHost->ui_closed(pHost->handle);
            fUiServer.stopPipeServer(1000);
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const
    {
        MemoryOutputStream out;
        saveProjectInternal(out);
        return strdup(out.toString().toRawUTF8());
    }

    void setState(const char* const data)
    {
        // remove all plugins from UI side
        for (int i=pData->curPluginCount; --i >= 0;)
            CarlaEngine::callback(ENGINE_CALLBACK_PLUGIN_REMOVED, i, 0, 0, 0.0f, nullptr);

        // remove all plugins from backend, no lock
        fIsRunning = false;
        removeAllPlugins();
        fIsRunning = true;

        // stopped during removeAllPlugins()
        if (! pData->thread.isThreadRunning())
            pData->thread.startThread();

        fOptionsForced = true;
        const String state(data);
        XmlDocument xml(state);
        loadProjectInternal(xml);
    }

    // -------------------------------------------------------------------

public:
    #define handlePtr ((CarlaEngineNative*)handle)

    static NativePluginHandle _instantiateRack(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, false);
    }

    static NativePluginHandle _instantiatePatchbay(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, true);
    }

    static NativePluginHandle _instantiatePatchbay3s(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, true, 3, 2);
    }

    static NativePluginHandle _instantiatePatchbay16(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, true, 16);
    }

    static NativePluginHandle _instantiatePatchbay32(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, true, 32);
    }

    static void _cleanup(NativePluginHandle handle)
    {
        delete handlePtr;
    }

    static uint32_t _get_parameter_count(NativePluginHandle handle)
    {
        return handlePtr->getParameterCount();
    }

    static const NativeParameter* _get_parameter_info(NativePluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterInfo(index);
    }

    static float _get_parameter_value(NativePluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterValue(index);
    }

    static uint32_t _get_midi_program_count(NativePluginHandle handle)
    {
        return handlePtr->getMidiProgramCount();
    }

    static const NativeMidiProgram* _get_midi_program_info(NativePluginHandle handle, uint32_t index)
    {
        return handlePtr->getMidiProgramInfo(index);
    }

    static void _set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
    {
        handlePtr->setParameterValue(index, value);
    }

    static void _set_midi_program(NativePluginHandle handle, uint8_t channel, uint32_t bank, uint32_t program)
    {
        handlePtr->setMidiProgram(channel, bank, program);
    }

    static void _ui_show(NativePluginHandle handle, bool show)
    {
        handlePtr->uiShow(show);
    }

    static void _ui_idle(NativePluginHandle handle)
    {
        handlePtr->uiIdle();
    }

    static void _activate(NativePluginHandle handle)
    {
        handlePtr->activate();
    }

    static void _deactivate(NativePluginHandle handle)
    {
        handlePtr->deactivate();
    }

    static void _process(NativePluginHandle handle, float** inBuffer, float** outBuffer, const uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
    {
        handlePtr->process(inBuffer, outBuffer, frames, midiEvents, midiEventCount);
    }

    static char* _get_state(NativePluginHandle handle)
    {
        return handlePtr->getState();
    }

    static void _set_state(NativePluginHandle handle, const char* data)
    {
        handlePtr->setState(data);
    }

    static intptr_t _dispatcher(NativePluginHandle handle, NativePluginDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        switch(opcode)
        {
        case NATIVE_PLUGIN_OPCODE_NULL:
            return 0;
        case NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED:
            CARLA_SAFE_ASSERT_RETURN(value > 0, 0);
            handlePtr->bufferSizeChanged(static_cast<uint32_t>(value));
            return 0;
        case NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED:
            CARLA_SAFE_ASSERT_RETURN(opt > 0.0, 0);
            handlePtr->sampleRateChanged(static_cast<double>(opt));
            return 0;
        case NATIVE_PLUGIN_OPCODE_OFFLINE_CHANGED:
            handlePtr->offlineModeChanged(value != 0);
            return 0;
        case NATIVE_PLUGIN_OPCODE_UI_NAME_CHANGED:
            //handlePtr->uiNameChanged(static_cast<const char*>(ptr));
            return 0;
        }

        return 0;

        // unused
        (void)index;
        (void)ptr;
    }

    // -------------------------------------------------------------------

    static void _ui_server_callback(void* handle, EngineCallbackOpcode action, uint pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        handlePtr->uiServerCallback(action, pluginId, value1, value2, value3, valueStr);
    }

    // -------------------------------------------------------------------

    #undef handlePtr

private:
    const NativeHostDescriptor* const pHost;

    const bool kIsPatchbay; // rack if false
    bool fIsActive, fIsRunning;
    CarlaEngineNativeUI fUiServer;

    bool fOptionsForced;
    char fTmpBuf[STR_MAX+1];

    CarlaPlugin* _getFirstPlugin() const noexcept
    {
        if (pData->curPluginCount == 0 || pData->plugins == nullptr)
            return nullptr;

        CarlaPlugin* const plugin(pData->plugins[0].plugin);

        if (plugin == nullptr || ! plugin->isEnabled())
            return nullptr;

        return pData->plugins[0].plugin;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineNative)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor carlaRackDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Rack",
    /* label     */ "carlarack",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiateRack,
    CarlaEngineNative::_cleanup,
    CarlaEngineNative::_get_parameter_count,
    CarlaEngineNative::_get_parameter_info,
    CarlaEngineNative::_get_parameter_value,
    CarlaEngineNative::_get_midi_program_count,
    CarlaEngineNative::_get_midi_program_info,
    CarlaEngineNative::_set_parameter_value,
    CarlaEngineNative::_set_midi_program,
    /* _set_custom_data        */ nullptr,
    CarlaEngineNative::_ui_show,
    CarlaEngineNative::_ui_idle,
    /* _ui_set_parameter_value */ nullptr,
    /* _ui_set_midi_program    */ nullptr,
    /* _ui_set_custom_data     */ nullptr,
    CarlaEngineNative::_activate,
    CarlaEngineNative::_deactivate,
    CarlaEngineNative::_process,
    CarlaEngineNative::_get_state,
    CarlaEngineNative::_set_state,
    CarlaEngineNative::_dispatcher
};

static const NativePluginDescriptor carlaPatchbayDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay",
    /* label     */ "carlapatchbay",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiatePatchbay,
    CarlaEngineNative::_cleanup,
    CarlaEngineNative::_get_parameter_count,
    CarlaEngineNative::_get_parameter_info,
    CarlaEngineNative::_get_parameter_value,
    CarlaEngineNative::_get_midi_program_count,
    CarlaEngineNative::_get_midi_program_info,
    CarlaEngineNative::_set_parameter_value,
    CarlaEngineNative::_set_midi_program,
    /* _set_custom_data        */ nullptr,
    CarlaEngineNative::_ui_show,
    CarlaEngineNative::_ui_idle,
    /* _ui_set_parameter_value */ nullptr,
    /* _ui_set_midi_program    */ nullptr,
    /* _ui_set_custom_data     */ nullptr,
    CarlaEngineNative::_activate,
    CarlaEngineNative::_deactivate,
    CarlaEngineNative::_process,
    CarlaEngineNative::_get_state,
    CarlaEngineNative::_set_state,
    CarlaEngineNative::_dispatcher
};

static const NativePluginDescriptor carlaPatchbay3sDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 3,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay (sidechain)",
    /* label     */ "carlapatchbay3s",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiatePatchbay3s,
    CarlaEngineNative::_cleanup,
    CarlaEngineNative::_get_parameter_count,
    CarlaEngineNative::_get_parameter_info,
    CarlaEngineNative::_get_parameter_value,
    CarlaEngineNative::_get_midi_program_count,
    CarlaEngineNative::_get_midi_program_info,
    CarlaEngineNative::_set_parameter_value,
    CarlaEngineNative::_set_midi_program,
    /* _set_custom_data        */ nullptr,
    CarlaEngineNative::_ui_show,
    CarlaEngineNative::_ui_idle,
    /* _ui_set_parameter_value */ nullptr,
    /* _ui_set_midi_program    */ nullptr,
    /* _ui_set_custom_data     */ nullptr,
    CarlaEngineNative::_activate,
    CarlaEngineNative::_deactivate,
    CarlaEngineNative::_process,
    CarlaEngineNative::_get_state,
    CarlaEngineNative::_set_state,
    CarlaEngineNative::_dispatcher
};

static const NativePluginDescriptor carlaPatchbay16Desc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 16,
    /* audioOuts */ 16,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay (16chan)",
    /* label     */ "carlapatchbay16",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiatePatchbay16,
    CarlaEngineNative::_cleanup,
    CarlaEngineNative::_get_parameter_count,
    CarlaEngineNative::_get_parameter_info,
    CarlaEngineNative::_get_parameter_value,
    CarlaEngineNative::_get_midi_program_count,
    CarlaEngineNative::_get_midi_program_info,
    CarlaEngineNative::_set_parameter_value,
    CarlaEngineNative::_set_midi_program,
    /* _set_custom_data        */ nullptr,
    CarlaEngineNative::_ui_show,
    CarlaEngineNative::_ui_idle,
    /* _ui_set_parameter_value */ nullptr,
    /* _ui_set_midi_program    */ nullptr,
    /* _ui_set_custom_data     */ nullptr,
    CarlaEngineNative::_activate,
    CarlaEngineNative::_deactivate,
    CarlaEngineNative::_process,
    CarlaEngineNative::_get_state,
    CarlaEngineNative::_set_state,
    CarlaEngineNative::_dispatcher
};

static const NativePluginDescriptor carlaPatchbay32Desc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 32,
    /* audioOuts */ 32,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay (32chan)",
    /* label     */ "carlapatchbay32",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiatePatchbay32,
    CarlaEngineNative::_cleanup,
    CarlaEngineNative::_get_parameter_count,
    CarlaEngineNative::_get_parameter_info,
    CarlaEngineNative::_get_parameter_value,
    CarlaEngineNative::_get_midi_program_count,
    CarlaEngineNative::_get_midi_program_info,
    CarlaEngineNative::_set_parameter_value,
    CarlaEngineNative::_set_midi_program,
    /* _set_custom_data        */ nullptr,
    CarlaEngineNative::_ui_show,
    CarlaEngineNative::_ui_idle,
    /* _ui_set_parameter_value */ nullptr,
    /* _ui_set_midi_program    */ nullptr,
    /* _ui_set_custom_data     */ nullptr,
    CarlaEngineNative::_activate,
    CarlaEngineNative::_deactivate,
    CarlaEngineNative::_process,
    CarlaEngineNative::_get_state,
    CarlaEngineNative::_set_state,
    CarlaEngineNative::_dispatcher
};

CARLA_BACKEND_END_NAMESPACE

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_carla();

void carla_register_native_plugin_carla()
{
    CARLA_BACKEND_USE_NAMESPACE;
    carla_register_native_plugin(&carlaRackDesc);
    carla_register_native_plugin(&carlaPatchbayDesc);
    carla_register_native_plugin(&carlaPatchbay3sDesc);
    carla_register_native_plugin(&carlaPatchbay16Desc);
    carla_register_native_plugin(&carlaPatchbay32Desc);
}

// -----------------------------------------------------------------------

CARLA_EXPORT
const NativePluginDescriptor* carla_get_native_rack_plugin();
const NativePluginDescriptor* carla_get_native_rack_plugin()
{
    // if this is called then we're running as special plugin
    gNeedsJuceHandling = true;

    CARLA_BACKEND_USE_NAMESPACE;
    return &carlaRackDesc;
}

CARLA_EXPORT
const NativePluginDescriptor* carla_get_native_patchbay_plugin();
const NativePluginDescriptor* carla_get_native_patchbay_plugin()
{
    // if this is called then we're running as special plugin
    gNeedsJuceHandling = true;

    CARLA_BACKEND_USE_NAMESPACE;
    return &carlaPatchbayDesc;
}

// -----------------------------------------------------------------------
// Extra stuff for linking purposes

#ifdef CARLA_PLUGIN_EXPORT

CARLA_BACKEND_START_NAMESPACE

CarlaEngine* CarlaEngine::newJack() { return nullptr; }

# if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
CarlaEngine*       CarlaEngine::newJuce(const AudioApi)           { return nullptr; }
uint               CarlaEngine::getJuceApiCount()                 { return 0;       }
const char*        CarlaEngine::getJuceApiName(const uint)        { return nullptr; }
const char* const* CarlaEngine::getJuceApiDeviceNames(const uint) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getJuceDeviceInfo(const uint, const char* const) { return nullptr; }
# else
CarlaEngine*       CarlaEngine::newRtAudio(const AudioApi)           { return nullptr; }
uint               CarlaEngine::getRtAudioApiCount()                 { return 0;       }
const char*        CarlaEngine::getRtAudioApiName(const uint)        { return nullptr; }
const char* const* CarlaEngine::getRtAudioApiDeviceNames(const uint) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getRtAudioDeviceInfo(const uint, const char* const) { return nullptr; }
# endif

CARLA_BACKEND_END_NAMESPACE

#include "CarlaHostCommon.cpp"
#include "CarlaPluginUI.cpp"
#include "CarlaDssiUtils.cpp"
#include "CarlaPatchbayUtils.cpp"
#include "CarlaPipeUtils.cpp"
#include "CarlaStateUtils.cpp"
#include "CarlaJuceEvents.cpp"

#endif

// -----------------------------------------------------------------------
