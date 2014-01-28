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
#ifdef CARLA_OS_WIN
# error This file should not be compiled for Windows
#endif

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaStateUtils.hpp"

#include "CarlaNative.hpp"

#include <QtCore/QTextStream>
#include <QtXml/QDomNode>

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

class CarlaEngineNativeUI : public CarlaPipeServer
{
public:
    enum UiState {
        UiNone = 0,
        UiHide,
        UiShow,
        UiCrashed
    };

    CarlaEngineNativeUI(CarlaEngine* const engine)
        : fEngine(engine),
          fUiState(UiNone)
    {
        carla_debug("CarlaEngineNativeUI::CarlaEngineNativeUI(%p)", engine);
    }

    ~CarlaEngineNativeUI() override
    {
        CARLA_ASSERT_INT(fUiState == UiNone, fUiState);
        carla_debug("CarlaEngineNativeUI::~CarlaEngineNativeUI()");
    }

    void setData(const char* const filename, const double sampleRate, const char* const uiTitle)
    {
        fFilename   = filename;
        fSampleRate = CarlaString(sampleRate);
        fUiTitle    = uiTitle;
    }

    UiState getAndResetUiState() noexcept
    {
        const UiState uiState(fUiState);
        fUiState = UiNone;
        return uiState;
    }

    void start()
    {
        CarlaPipeServer::start(fFilename, fSampleRate, fUiTitle);
        writeMsg("show\n", 5);
    }

protected:
    void msgReceived(const char* const msg) override
    {
        if (std::strcmp(msg, "exiting") == 0)
        {
            waitChildClose();
            fUiState = UiHide;
        }
        else if (std::strcmp(msg, "set_engine_option") == 0)
        {
            int option, value;
            const char* valueStr;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(option),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(value),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(valueStr),);

            fEngine->setOption((EngineOption)option, value, valueStr);

            delete[] valueStr;
        }
        else if (std::strcmp(msg, "load_file") == 0)
        {
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename),);

            fEngine->loadFile(filename);

            delete[] filename;
        }
        else if (std::strcmp(msg, "load_project") == 0)
        {
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename),);

            fEngine->loadProject(filename);

            delete[] filename;
        }
        else if (std::strcmp(msg, "save_project") == 0)
        {
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename),);

            fEngine->saveProject(filename);

            delete[] filename;
        }
        else if (std::strcmp(msg, "patchbay_connect") == 0)
        {
            int portA, portB;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(portA),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(portB),);

            fEngine->patchbayConnect(portA, portB);
        }
        else if (std::strcmp(msg, "patchbay_disconnect") == 0)
        {
            uint connectionId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(connectionId),);

            fEngine->patchbayDisconnect(connectionId);
        }
        else if (std::strcmp(msg, "patchbay_refresh") == 0)
        {
            fEngine->patchbayRefresh();
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
            long frame;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsLong(frame),);

            fEngine->transportRelocate((uint64_t)frame);
        }
        else if (std::strcmp(msg, "add_plugin") == 0)
        {
            int btype, ptype;
            const char* filename = nullptr;
            const char* name;
            const char* label;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(btype),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(ptype),);
            readNextLineAsString(filename); // can be null
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(name),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(label),);

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

            fEngine->addPlugin((BinaryType)btype, (PluginType)ptype, filename, name, label);

            if (filename != nullptr)
                delete[] filename;
            if (name != nullptr)
                delete[] name;
            delete[] label;
        }
        else if (std::strcmp(msg, "remove_plugin") == 0)
        {
            uint pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);

            fEngine->removePlugin(pluginId);
        }
        else if (std::strcmp(msg, "remove_all_plugins") == 0)
        {
            fEngine->removeAllPlugins();
        }
        else if (std::strcmp(msg, "rename_plugin") == 0)
        {
            uint pluginId;
            const char* newName;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(newName),);

            /*const char* name =*/ fEngine->renamePlugin(pluginId, newName);

            delete[] newName;
        }
        else if (std::strcmp(msg, "clone_plugin") == 0)
        {
            uint pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);

            fEngine->clonePlugin(pluginId);
        }
        else if (std::strcmp(msg, "replace_plugin") == 0)
        {
            uint pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);

            fEngine->replacePlugin(pluginId);
        }
        else if (std::strcmp(msg, "switch_plugins") == 0)
        {
            uint pluginIdA, pluginIdB;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginIdA),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginIdB),);

            fEngine->switchPlugins(pluginIdA, pluginIdB);
        }
        else if (std::strcmp(msg, "load_plugin_state") == 0)
        {
            uint pluginId;
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->loadStateFromFile(filename);

            delete[] filename;
        }
        else if (std::strcmp(msg, "save_plugin_state") == 0)
        {
            uint pluginId;
            const char* filename;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(filename),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->saveStateToFile(filename);

            delete[] filename;
        }
        else if (std::strcmp(msg, "set_option") == 0)
        {
            uint pluginId;
            uint option;
            bool yesNo;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(option),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(yesNo),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setOption(option, yesNo);
        }
        else if (std::strcmp(msg, "set_active") == 0)
        {
            uint pluginId;
            bool onOff;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(onOff),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setActive(onOff, true, false);
        }
        else if (std::strcmp(msg, "set_drywet") == 0)
        {
            uint pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setDryWet(value, true, false);
        }
        else if (std::strcmp(msg, "set_volume") == 0)
        {
            uint pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setVolume(value, true, false);
        }
        else if (std::strcmp(msg, "set_balance_left") == 0)
        {
            uint pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setBalanceLeft(value, true, false);
        }
        else if (std::strcmp(msg, "set_balance_right") == 0)
        {
            uint pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setBalanceRight(value, true, false);
        }
        else if (std::strcmp(msg, "set_panning") == 0)
        {
            uint pluginId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setPanning(value, true, false);
        }
        else if (std::strcmp(msg, "set_ctrl_channel") == 0)
        {
            uint pluginId;
            int channel;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(channel),);
            CARLA_SAFE_ASSERT_RETURN(channel >= -1 && channel < MAX_MIDI_CHANNELS,);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setCtrlChannel(int8_t(channel), true, false);
        }
        else if (std::strcmp(msg, "set_parameter_value") == 0)
        {
            uint pluginId;
            uint parameterId;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(parameterId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setParameterValue(parameterId, value, true, true, false);
        }
        else if (std::strcmp(msg, "set_parameter_midi_channel") == 0)
        {
            uint pluginId;
            uint parameterId;
            int channel;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(parameterId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(channel),);
            CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < MAX_MIDI_CHANNELS,);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setParameterMidiChannel(parameterId, uint8_t(channel), true, false);
        }
        else if (std::strcmp(msg, "set_parameter_midi_cc") == 0)
        {
            uint pluginId;
            uint parameterId;
            int cc;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(parameterId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(cc),);
            CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < 0x5F,);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setParameterMidiCC(parameterId, int16_t(cc), true, false);
        }
        else if (std::strcmp(msg, "set_program") == 0)
        {
            uint pluginId;
            int index;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(index),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setProgram(index, true, true, false);
        }
        else if (std::strcmp(msg, "set_midi_program") == 0)
        {
            uint pluginId;
            int index;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(index),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setMidiProgram(index, true, true, false);
        }
        else if (std::strcmp(msg, "set_custom_data") == 0)
        {
            uint pluginId;
            const char* type;
            const char* key;
            const char* value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(type),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(key),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(value),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setCustomData(type, key, value, true);
        }
        else if (std::strcmp(msg, "set_chunk_data") == 0)
        {
            uint pluginId;
            const char* cdata;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(cdata),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->setChunkData(cdata);
        }
        else if (std::strcmp(msg, "prepare_for_save") == 0)
        {
            uint pluginId;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->prepareForSave();
        }
        else if (std::strcmp(msg, "send_midi_note") == 0)
        {
            uint pluginId;
            int channel, note, velocity;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(channel),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(note),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(velocity),);
            CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < MAX_MIDI_CHANNELS,);
            CARLA_SAFE_ASSERT_RETURN(note >= 0 && channel < MAX_MIDI_VALUE,);
            CARLA_SAFE_ASSERT_RETURN(velocity >= 0 && channel < MAX_MIDI_VALUE,);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->sendMidiSingleNote(uint8_t(channel), uint8_t(note), uint8_t(velocity), true, true, false);
        }
        else if (std::strcmp(msg, "show_custom_ui") == 0)
        {
            uint pluginId;
            bool yesNo;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(pluginId),);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsBool(yesNo),);

            if (CarlaPlugin* const plugin = fEngine->getPlugin(pluginId))
                plugin->showCustomUI(yesNo);
        }
        else
        {
            carla_stderr("msgReceived : %s", msg);
        }
    }

private:
    CarlaEngine* const fEngine;

    CarlaString fFilename;
    CarlaString fSampleRate;
    CarlaString fUiTitle;
    UiState     fUiState;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineNativeUI)
};

// -----------------------------------------------------------------------

class CarlaEngineNative : public CarlaEngine
{
public:
    CarlaEngineNative(const NativeHostDescriptor* const host, const bool isPatchbay)
        : CarlaEngine(),
          pHost(host),
          fIsPatchbay(isPatchbay),
          fIsActive(false),
          fIsRunning(false),
          fUiServer(this)
    {
        carla_debug("CarlaEngineNative::CarlaEngineNative()");

        fTmpBuf[STR_MAX] = '\0';

        // set-up engine
        if (fIsPatchbay)
        {
            pData->options.processMode         = ENGINE_PROCESS_MODE_PATCHBAY;
            pData->options.transportMode       = ENGINE_TRANSPORT_MODE_PLUGIN;
            pData->options.forceStereo         = false;
            pData->options.preferPluginBridges = false;
            pData->options.preferUiBridges     = false;
            init("Carla-Patchbay");
        }
        else
        {
            pData->options.processMode         = ENGINE_PROCESS_MODE_CONTINUOUS_RACK;
            pData->options.transportMode       = ENGINE_TRANSPORT_MODE_PLUGIN;
            pData->options.forceStereo         = true;
            pData->options.preferPluginBridges = false;
            pData->options.preferUiBridges     = false;
            init("Carla-Rack");
        }

        setCallback(_ui_server_callback, this);
    }

    ~CarlaEngineNative() override
    {
        CARLA_ASSERT(! fIsActive);
        carla_debug("CarlaEngineNative::~CarlaEngineNative() - START");

        pData->aboutToClose = true;
        fIsRunning = false;

        removeAllPlugins();
        runPendingRtEvents();
        close();

        carla_debug("CarlaEngineNative::~CarlaEngineNative() - END");
    }

protected:
    // -------------------------------------
    // CarlaEngine virtual calls

    bool init(const char* const clientName) override
    {
        carla_debug("CarlaEngineNative::init(\"%s\")", clientName);

        pData->bufferSize = pHost->get_buffer_size(pHost->handle);
        pData->sampleRate = pHost->get_sample_rate(pHost->handle);

        fIsRunning = true;
        CarlaEngine::init(clientName);
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

    // -------------------------------------------------------------------

    const char* renamePlugin(const unsigned int id, const char* const newName) override
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
        pData->bufferSize = newBufferSize;
        CarlaEngine::bufferSizeChanged(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate)
    {
        pData->sampleRate = newSampleRate;
        CarlaEngine::sampleRateChanged(newSampleRate);
    }

    // -------------------------------------------------------------------

    void uiServerSendPluginInfo(CarlaPlugin* const plugin)
    {
        const uint pluginId(plugin->getId());

        std::sprintf(fTmpBuf, "PLUGIN_INFO_%i\n", pluginId);
        fUiServer.writeMsg(fTmpBuf);

        std::sprintf(fTmpBuf, "%i:%i:%i:%li:%i:%i\n", plugin->getType(), plugin->getCategory(), plugin->getHints(), plugin->getUniqueId(), plugin->getOptionsAvailable(), plugin->getOptionsEnabled());
        fUiServer.writeMsg(fTmpBuf);

        if (const char* const filename = plugin->getFilename())
        {
            std::sprintf(fTmpBuf, "%s", filename);
            fUiServer.writeAndFixMsg(fTmpBuf);
        }
        else
            fUiServer.writeMsg("\n");

        if (const char* const name = plugin->getName())
        {
            std::sprintf(fTmpBuf, "%s", name);
            fUiServer.writeAndFixMsg(fTmpBuf);
        }
        else
            fUiServer.writeMsg("\n");

        if (const char* const iconName = plugin->getIconName())
        {
            std::sprintf(fTmpBuf, "%s", iconName);
            fUiServer.writeAndFixMsg(fTmpBuf);
        }
        else
            fUiServer.writeMsg("\n");

        plugin->getRealName(fTmpBuf);
        fUiServer.writeAndFixMsg(fTmpBuf);

        plugin->getLabel(fTmpBuf);
        fUiServer.writeAndFixMsg(fTmpBuf);

        plugin->getMaker(fTmpBuf);
        fUiServer.writeAndFixMsg(fTmpBuf);

        plugin->getCopyright(fTmpBuf);
        fUiServer.writeAndFixMsg(fTmpBuf);

        std::sprintf(fTmpBuf, "AUDIO_COUNT_%i:%i:%i\n", pluginId, plugin->getAudioInCount(), plugin->getAudioOutCount());
        fUiServer.writeMsg(fTmpBuf);

        std::sprintf(fTmpBuf, "MIDI_COUNT_%i:%i:%i\n", pluginId, plugin->getMidiInCount(), plugin->getMidiOutCount());
        fUiServer.writeMsg(fTmpBuf);
    }

    void uiServerSendPluginParameters(CarlaPlugin* const plugin)
    {
        const uint pluginId(plugin->getId());

        uint32_t ins, outs, count;
        plugin->getParameterCountInfo(ins, outs);
        count = plugin->getParameterCount();
        std::sprintf(fTmpBuf, "PARAMETER_COUNT_%i:%i:%i:%i\n", pluginId, ins, outs, count);
        fUiServer.writeMsg(fTmpBuf);

        for (uint32_t i=0; i<count; ++i)
        {
            const ParameterData& paramData(plugin->getParameterData(i));
            const ParameterRanges& paramRanges(plugin->getParameterRanges(i));

            std::sprintf(fTmpBuf, "PARAMETER_DATA_%i:%i\n", pluginId, i);
            fUiServer.writeMsg(fTmpBuf);
            std::sprintf(fTmpBuf, "%i:%i:%i:%i\n", paramData.type, paramData.hints, paramData.midiChannel, paramData.midiCC);
            fUiServer.writeMsg(fTmpBuf);
            plugin->getParameterName(i, fTmpBuf);
            fUiServer.writeAndFixMsg(fTmpBuf);
            plugin->getParameterUnit(i, fTmpBuf);
            fUiServer.writeAndFixMsg(fTmpBuf);

            std::sprintf(fTmpBuf, "PARAMETER_RANGES_%i:%i\n", pluginId, i);
            fUiServer.writeMsg(fTmpBuf);
            std::sprintf(fTmpBuf, "%f:%f:%f:%f:%f:%f\n", paramRanges.def, paramRanges.min, paramRanges.max, paramRanges.step, paramRanges.stepSmall, paramRanges.stepLarge);
            fUiServer.writeMsg(fTmpBuf);

            std::sprintf(fTmpBuf, "PARAMVAL_%i:%i\n", pluginId, i);
            fUiServer.writeMsg(fTmpBuf);
            std::sprintf(fTmpBuf, "%f\n", plugin->getParameterValue(i));
            fUiServer.writeMsg(fTmpBuf);
        }
    }

    void uiServerSendPluginPrograms(CarlaPlugin* const plugin)
    {
        const uint pluginId(plugin->getId());

        uint32_t count = plugin->getProgramCount();
        std::sprintf(fTmpBuf, "PROGRAM_COUNT_%i:%i:%i\n", pluginId, count, plugin->getCurrentProgram());
        fUiServer.writeMsg(fTmpBuf);

        for (uint32_t i=0; i<count; ++i)
        {
            std::sprintf(fTmpBuf, "PROGRAM_NAME_%i:%i\n", pluginId, i);
            fUiServer.writeMsg(fTmpBuf);
            plugin->getProgramName(i, fTmpBuf);
            fUiServer.writeAndFixMsg(fTmpBuf);
        }

        count = plugin->getMidiProgramCount();
        std::sprintf(fTmpBuf, "MIDI_PROGRAM_COUNT_%i:%i:%i\n", pluginId, count, plugin->getCurrentMidiProgram());
        fUiServer.writeMsg(fTmpBuf);

        for (uint32_t i=0; i<count; ++i)
        {
            std::sprintf(fTmpBuf, "MIDI_PROGRAM_DATA_%i:%i\n", pluginId, i);
            fUiServer.writeMsg(fTmpBuf);

            const MidiProgramData& mpData(plugin->getMidiProgramData(i));
            std::sprintf(fTmpBuf, "%i:%i\n", mpData.bank, mpData.program);
            fUiServer.writeMsg(fTmpBuf);
            std::sprintf(fTmpBuf, "%s", mpData.name);
            fUiServer.writeAndFixMsg(fTmpBuf);
        }
    }

    void uiServerCallback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        if (! fIsRunning)
            return;
        if (! fUiServer.isOk())
            return;

        CarlaPlugin* plugin;

        switch (action)
        {
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
            }
            break;

        default:
            break;
        }

        std::sprintf(fTmpBuf, "ENGINE_CALLBACK_%i\n", int(action));
        fUiServer.writeMsg(fTmpBuf);

        std::sprintf(fTmpBuf, "%u\n", pluginId);
        fUiServer.writeMsg(fTmpBuf);

        std::sprintf(fTmpBuf, "%i\n", value1);
        fUiServer.writeMsg(fTmpBuf);

        std::sprintf(fTmpBuf, "%i\n", value2);
        fUiServer.writeMsg(fTmpBuf);

        std::sprintf(fTmpBuf, "%f\n", value3);
        fUiServer.writeMsg(fTmpBuf);

        fUiServer.writeAndFixMsg(valueStr);
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

                unsigned int hints = 0x0;

                if (paramData.hints & PARAMETER_IS_BOOLEAN)
                    hints |= ::PARAMETER_IS_BOOLEAN;
                if (paramData.hints & PARAMETER_IS_INTEGER)
                    hints |= ::PARAMETER_IS_INTEGER;
                if (paramData.hints & PARAMETER_IS_LOGARITHMIC)
                    hints |= ::PARAMETER_IS_LOGARITHMIC;
                if (paramData.hints & PARAMETER_IS_AUTOMABLE)
                    hints |= ::PARAMETER_IS_AUTOMABLE;
                if (paramData.hints & PARAMETER_USES_SAMPLERATE)
                    hints |= ::PARAMETER_USES_SAMPLE_RATE;
                if (paramData.hints & PARAMETER_USES_SCALEPOINTS)
                    hints |= ::PARAMETER_USES_SCALEPOINTS;
                if (paramData.hints & PARAMETER_USES_CUSTOM_TEXT)
                    hints |= ::PARAMETER_USES_CUSTOM_TEXT;

                if (paramData.type == PARAMETER_INPUT || paramData.type == PARAMETER_OUTPUT)
                {
                    if (paramData.hints & PARAMETER_IS_ENABLED)
                        hints |= ::PARAMETER_IS_ENABLED;
                    if (paramData.type == PARAMETER_OUTPUT)
                        hints |= ::PARAMETER_IS_OUTPUT;
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

    const char* getParameterText(const uint32_t index, const float value) const
    {
        if (CarlaPlugin* const plugin = _getFirstPlugin())
        {
            if (index < plugin->getParameterCount())
            {
                static char strBuf[STR_MAX+1];
                carla_zeroChar(strBuf, STR_MAX+1);

                plugin->getParameterText(index, value, strBuf);

                return strBuf;
            }
        }

        return nullptr;
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
        for (uint32_t i=0; i < pData->curPluginCount; ++i)
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
        for (uint32_t i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin == nullptr || ! plugin->isEnabled())
                continue;

            plugin->setActive(false, true, false);
        }
#endif

        // just in case
        runPendingRtEvents();
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount)
    {
        if (pData->curPluginCount == 0 && ! fIsPatchbay)
        {
            FLOAT_COPY(outBuffer[0], inBuffer[0], frames);
            FLOAT_COPY(outBuffer[1], inBuffer[1], frames);

            return runPendingRtEvents();;
        }

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
        // initialize events

        carla_zeroStruct<EngineEvent>(pData->bufEvents.in,  kMaxEngineEventInternalCount);
        carla_zeroStruct<EngineEvent>(pData->bufEvents.out, kMaxEngineEventInternalCount);

        // ---------------------------------------------------------------
        // events input (before processing)

        {
            uint32_t engineEventIndex = 0;

            for (uint32_t i=0; i < midiEventCount && engineEventIndex < kMaxEngineEventInternalCount; ++i)
            {
                const NativeMidiEvent& midiEvent(midiEvents[i]);
                EngineEvent&           engineEvent(pData->bufEvents.in[engineEventIndex++]);

                engineEvent.time = midiEvent.time;
                engineEvent.fillFromMidiData(midiEvent.size, midiEvent.data);

                if (engineEventIndex >= kMaxEngineEventInternalCount)
                    break;
            }
        }

        if (fIsPatchbay)
        {
            // -----------------------------------------------------------
            // create audio buffers

            //float*   inBuf[2]    = { inBuffer[0], inBuffer[1] };
            //float*   outBuf[2]   = { outBuffer[0], outBuffer[1] };
            //uint32_t bufCount[2] = { 2, 2 };

            // -----------------------------------------------------------
            // process

            //pData->processPatchbay(inBuf, outBuf, bufCount, frames, isOffline());
        }
        else
        {
            // -----------------------------------------------------------
            // create audio buffers

            float* inBuf[2]  = { inBuffer[0], inBuffer[1] };
            float* outBuf[2] = { outBuffer[0], outBuffer[1] };

            // -----------------------------------------------------------
            // process

            pData->processRack(inBuf, outBuf, frames, isOffline());
        }

        // ---------------------------------------------------------------
        // events output (after processing)

        carla_zeroStruct<EngineEvent>(pData->bufEvents.in, kMaxEngineEventInternalCount);

        {
            NativeMidiEvent midiEvent;

            for (uint32_t i=0; i < kMaxEngineEventInternalCount; ++i)
            {
                const EngineEvent& engineEvent(pData->bufEvents.out[i]);

                if (engineEvent.type == kEngineEventTypeNull)
                    break;

                midiEvent.time = engineEvent.time;

                if (engineEvent.type == CarlaBackend::kEngineEventTypeControl)
                {
                    midiEvent.port = 0;
                    engineEvent.ctrl.dumpToMidiData(engineEvent.channel, midiEvent.size, midiEvent.data);
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

        runPendingRtEvents();
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show)
    {
        if (show)
        {
            CarlaString path(pHost->resourceDir);
            path += "/carla-plugin";

            carla_stdout("Trying to start carla-plugin using \"%s\"", path.getBuffer());

            fUiServer.setData(path.getBuffer(), pData->sampleRate, pHost->uiName);
            fUiServer.start();

            for (uint i=0; i < pData->curPluginCount; ++i)
            {
                CarlaPlugin* const plugin(pData->plugins[i].plugin);

                if (plugin != nullptr && plugin->isEnabled())
                {
                    uiServerCallback(ENGINE_CALLBACK_PLUGIN_ADDED, i, 0, 0, 0.0f, plugin->getName());
                }
            }
        }
        else
        {
            fUiServer.stop();
        }
    }

    void uiIdle()
    {
        CarlaEngine::idle();
        fUiServer.idle();

        if (! fUiServer.isOk())
            return;

        for (uint i=0; i < pData->curPluginCount; ++i)
        {
            const EnginePluginData& plugData(pData->plugins[i]);
            const CarlaPlugin* const plugin(pData->plugins[i].plugin);

            std::sprintf(fTmpBuf, "PEAKS_%i\n", i);
            fUiServer.writeMsg(fTmpBuf);

            std::sprintf(fTmpBuf, "%f:%f:%f:%f\n", plugData.insPeak[0], plugData.insPeak[1], plugData.outsPeak[0], plugData.outsPeak[1]);
            fUiServer.writeMsg(fTmpBuf);

            for (uint32_t j=0, count=plugin->getParameterCount(); j < count; ++j)
            {
                if (plugin->isParameterOutput(j))
                    continue;

                std::sprintf(fTmpBuf, "PARAMVAL_%i:%i\n", i, j);
                fUiServer.writeMsg(fTmpBuf);
                std::sprintf(fTmpBuf, "%f\n", plugin->getParameterValue(j));
                fUiServer.writeMsg(fTmpBuf);
            }
        }

        switch (fUiServer.getAndResetUiState())
        {
        case CarlaEngineNativeUI::UiNone:
        case CarlaEngineNativeUI::UiShow:
            break;
        case CarlaEngineNativeUI::UiCrashed:
            pHost->dispatcher(pHost->handle, HOST_OPCODE_UI_UNAVAILABLE, 0, 0, nullptr, 0.0f);
            break;
        case CarlaEngineNativeUI::UiHide:
            pHost->ui_closed(pHost->handle);
            break;
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const
    {
        QString string;
        QTextStream out(&string);
        out << "<?xml version='1.0' encoding='UTF-8'?>\n";
        out << "<!DOCTYPE CARLA-PROJECT>\n";
        out << "<CARLA-PROJECT VERSION='2.0'>\n";

        bool firstPlugin = true;
        char strBuf[STR_MAX+1];

        for (unsigned int i=0; i < pData->curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(pData->plugins[i].plugin);

            if (plugin != nullptr && plugin->isEnabled())
            {
                if (! firstPlugin)
                    out << "\n";

                strBuf[0] = '\0';

                plugin->getRealName(strBuf);

                //if (strBuf[0] != '\0')
                //    out << QString(" <!-- %1 -->\n").arg(xmlSafeString(strBuf, true));

                QString content;
                fillXmlStringFromSaveState(content, plugin->getSaveState());

                out << " <Plugin>\n";
                out << content;
                out << " </Plugin>\n";

                firstPlugin = false;
            }
        }

        out << "</CARLA-PROJECT>\n";

        return strdup(string.toUtf8().constData());
    }

    void setState(const char* const data)
    {
        QDomDocument xml;
        xml.setContent(QString(data));

        QDomNode xmlNode(xml.documentElement());

        if (xmlNode.toElement().tagName().compare("carla-project", Qt::CaseInsensitive) != 0)
        {
            carla_stderr2("Not a valid Carla project");
            return;
        }

        //bool pluginsAdded = false;

        for (QDomNode node = xmlNode.firstChild(); ! node.isNull(); node = node.nextSibling())
        {
            if (node.toElement().tagName().compare("plugin", Qt::CaseInsensitive) == 0)
            {
                SaveState saveState;
                fillSaveStateFromXmlNode(saveState, node);

                CARLA_SAFE_ASSERT_CONTINUE(saveState.type != nullptr)

                const void* extraStuff = nullptr;

                // check if using GIG, SF2 or SFZ 16outs
                static const char kUse16OutsSuffix[] = " (16 outs)";

                if (CarlaString(saveState.label).endsWith(kUse16OutsSuffix))
                {
                    if (std::strcmp(saveState.type, "GIG") == 0 || std::strcmp(saveState.type, "SF2") == 0)
                        extraStuff = "true";
                }

                // TODO - proper find&load plugins

                if (addPlugin(getPluginTypeFromString(saveState.type), saveState.binary, saveState.name, saveState.label, extraStuff))
                {
                    if (CarlaPlugin* const plugin = getPlugin(pData->curPluginCount-1))
                        plugin->loadSaveState(saveState);
                }

                //pluginsAdded = true;
            }
        }

        //if (pluginsAdded)
        //    pHost->dispatcher(pHost->handle, HOST_OPCODE_RELOAD_ALL, 0, 0, nullptr, 0.0f);
    }

    // -------------------------------------------------------------------

public:
    #define handlePtr ((CarlaEngineNative*)handle)

    static NativePluginHandle _instantiateRack(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, false);
    }

#ifdef HAVE_JUCE
    static NativePluginHandle _instantiatePatchbay(const NativeHostDescriptor* host)
    {
        return new CarlaEngineNative(host, true);
    }
#endif

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

    static const char* _get_parameter_text(NativePluginHandle handle, uint32_t index, float value)
    {
        return handlePtr->getParameterText(index, value);
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
        case PLUGIN_OPCODE_NULL:
            return 0;
        case PLUGIN_OPCODE_BUFFER_SIZE_CHANGED:
            CARLA_SAFE_ASSERT_RETURN(value > 0, 0);
            handlePtr->bufferSizeChanged(static_cast<uint32_t>(value));
            return 0;
        case PLUGIN_OPCODE_SAMPLE_RATE_CHANGED:
            handlePtr->sampleRateChanged(static_cast<double>(opt));
            return 0;
        case PLUGIN_OPCODE_OFFLINE_CHANGED:
            handlePtr->offlineModeChanged(value != 0);
            return 0;
        case PLUGIN_OPCODE_UI_NAME_CHANGED:
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

    const bool fIsPatchbay; // rack if false
    bool fIsActive, fIsRunning;
    CarlaEngineNativeUI fUiServer;

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
    /* category  */ ::PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(::PLUGIN_IS_SYNTH|::PLUGIN_HAS_UI|::PLUGIN_NEEDS_FIXED_BUFFERS|::PLUGIN_NEEDS_SINGLE_THREAD|::PLUGIN_USES_STATE|::PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(::PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Rack",
    /* label     */ "carla-rack",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiateRack,
    CarlaEngineNative::_cleanup,
    CarlaEngineNative::_get_parameter_count,
    CarlaEngineNative::_get_parameter_info,
    CarlaEngineNative::_get_parameter_value,
    CarlaEngineNative::_get_parameter_text,
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

#if 0 //def HAVE_JUCE
static const NativePluginDescriptor carlaPatchbayDesc = {
    /* category  */ ::PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(::PLUGIN_IS_SYNTH|::PLUGIN_HAS_UI|::PLUGIN_NEEDS_FIXED_BUFFERS|::PLUGIN_NEEDS_SINGLE_THREAD|::PLUGIN_USES_STATE|::PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(::PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay",
    /* label     */ "carla-patchbay",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    CarlaEngineNative::_instantiatePatchbay,
    CarlaEngineNative::_cleanup,
    CarlaEngineNative::_get_parameter_count,
    CarlaEngineNative::_get_parameter_info,
    CarlaEngineNative::_get_parameter_value,
    CarlaEngineNative::_get_parameter_text,
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
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

CARLA_EXPORT
void carla_register_native_plugin_carla();

void carla_register_native_plugin_carla()
{
    CARLA_BACKEND_USE_NAMESPACE
    carla_register_native_plugin(&carlaRackDesc);
#if 0 //def HAVE_JUCE
    carla_register_native_plugin(&carlaPatchbayDesc);
#endif
}

// -----------------------------------------------------------------------
// Extra stuff for linking purposes

#ifdef CARLA_PLUGIN_EXPORT

CARLA_BACKEND_START_NAMESPACE

CarlaEngine* CarlaEngine::newJack() { return nullptr; }

CarlaEngine*       CarlaEngine::newRtAudio(const AudioApi) { return nullptr; }
unsigned int       CarlaEngine::getRtAudioApiCount() { return 0; }
const char*        CarlaEngine::getRtAudioApiName(const unsigned int) { return nullptr; }
const char* const* CarlaEngine::getRtAudioApiDeviceNames(const unsigned int) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getRtAudioDeviceInfo(const unsigned int, const char* const) { return nullptr; }

# ifdef HAVE_JUCE
CarlaEngine*       CarlaEngine::newJuce(const AudioApi) { return nullptr; }
unsigned int       CarlaEngine::getJuceApiCount() { return 0; }
const char*        CarlaEngine::getJuceApiName(const unsigned int) { return nullptr; }
const char* const* CarlaEngine::getJuceApiDeviceNames(const unsigned int) { return nullptr; }
const EngineDriverDeviceInfo* CarlaEngine::getJuceDeviceInfo(const unsigned int, const char* const) { return nullptr; }
# endif

CARLA_BACKEND_END_NAMESPACE

#include "CarlaStateUtils.cpp"

#endif

// -----------------------------------------------------------------------
