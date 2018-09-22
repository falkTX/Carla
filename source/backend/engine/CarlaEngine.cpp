/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

/* TODO:
 * - complete processRack(): carefully add to input, sorted events
 * - implement processPatchbay()
 * - implement oscSend_control_switch_plugins()
 * - something about the peaks?
 */

#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBinaryUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaStateUtils.hpp"
#include "CarlaMIDI.h"

#include "jackbridge/JackBridge.hpp"

#include "water/files/File.h"
#include "water/streams/MemoryOutputStream.h"
#include "water/xml/XmlDocument.h"
#include "water/xml/XmlElement.h"

// FIXME Remove on 2.1 release
#include "lv2/atom.h"

using water::Array;
using water::CharPointer_UTF8;
using water::File;
using water::MemoryOutputStream;
using water::String;
using water::StringArray;
using water::XmlDocument;
using water::XmlElement;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine()
    : pData(new ProtectedData(this))
{
    carla_debug("CarlaEngine::CarlaEngine()");
}

CarlaEngine::~CarlaEngine()
{
    carla_debug("CarlaEngine::~CarlaEngine()");

    delete pData;
}

// -----------------------------------------------------------------------
// Static calls

uint CarlaEngine::getDriverCount()
{
    carla_debug("CarlaEngine::getDriverCount()");

    uint count = 0;

    if (jackbridge_is_ok())
        count += 1;

#ifndef BUILD_BRIDGE
    count += getRtAudioApiCount();
#endif

    return count;
}

const char* CarlaEngine::getDriverName(const uint index2)
{
    carla_debug("CarlaEngine::getDriverName(%i)", index2);

    uint index(index2);

    if (jackbridge_is_ok() && index-- == 0)
        return "JACK";

#ifndef BUILD_BRIDGE
    if (const uint count = getRtAudioApiCount())
    {
        if (index < count)
            return getRtAudioApiName(index);
    }
#endif

    carla_stderr("CarlaEngine::getDriverName(%i) - invalid index", index2);
    return nullptr;
}

const char* const* CarlaEngine::getDriverDeviceNames(const uint index2)
{
    carla_debug("CarlaEngine::getDriverDeviceNames(%i)", index2);

    uint index(index2);

    if (jackbridge_is_ok() && index-- == 0)
    {
        static const char* ret[3] = { "Auto-Connect ON", "Auto-Connect OFF", nullptr };
        return ret;
    }

#ifndef BUILD_BRIDGE
    if (const uint count = getRtAudioApiCount())
    {
        if (index < count)
            return getRtAudioApiDeviceNames(index);
    }
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i) - invalid index", index2);
    return nullptr;
}

const EngineDriverDeviceInfo* CarlaEngine::getDriverDeviceInfo(const uint index2, const char* const deviceName)
{
    carla_debug("CarlaEngine::getDriverDeviceInfo(%i, \"%s\")", index2, deviceName);

    uint index(index2);

    if (jackbridge_is_ok() && index-- == 0)
    {
        static EngineDriverDeviceInfo devInfo;
        devInfo.hints       = ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE;
        devInfo.bufferSizes = nullptr;
        devInfo.sampleRates = nullptr;
        return &devInfo;
    }

#ifndef BUILD_BRIDGE
    if (const uint count = getRtAudioApiCount())
    {
        if (index < count)
            return getRtAudioDeviceInfo(index, deviceName);
    }
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%i, \"%s\") - invalid index", index2, deviceName);
    return nullptr;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', nullptr);
    carla_debug("CarlaEngine::newDriverByName(\"%s\")", driverName);

    if (std::strcmp(driverName, "JACK") == 0)
        return newJack();

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // common

    if (std::strcmp(driverName, "Dummy") == 0)
        return newRtAudio(AUDIO_API_NULL);
    if (std::strncmp(driverName, "JACK ", 5) == 0)
        return newRtAudio(AUDIO_API_JACK);
    if (std::strcmp(driverName, "OSS") == 0)
        return newRtAudio(AUDIO_API_OSS);

    // -------------------------------------------------------------------
    // linux

    if (std::strcmp(driverName, "ALSA") == 0)
        return newRtAudio(AUDIO_API_ALSA);
    if (std::strcmp(driverName, "PulseAudio") == 0)
        return newRtAudio(AUDIO_API_PULSEAUDIO);

    // -------------------------------------------------------------------
    // macos

    if (std::strcmp(driverName, "CoreAudio") == 0)
        return newRtAudio(AUDIO_API_COREAUDIO);

    // -------------------------------------------------------------------
    // windows

    if (std::strcmp(driverName, "ASIO") == 0)
        return newRtAudio(AUDIO_API_ASIO);
    if (std::strcmp(driverName, "DirectSound") == 0)
        return newRtAudio(AUDIO_API_DIRECTSOUND);
    if (std::strcmp(driverName, "WASAPI") == 0)
        return newRtAudio(AUDIO_API_WASAPI);
#endif

    carla_stderr("CarlaEngine::newDriverByName(\"%s\") - invalid driver name", driverName);
    return nullptr;
}

// -----------------------------------------------------------------------
// Constant values

uint CarlaEngine::getMaxClientNameSize() const noexcept
{
    return STR_MAX/2;
}

uint CarlaEngine::getMaxPortNameSize() const noexcept
{
    return STR_MAX;
}

uint CarlaEngine::getCurrentPluginCount() const noexcept
{
    return pData->curPluginCount;
}

uint CarlaEngine::getMaxPluginNumber() const noexcept
{
    return pData->maxPluginNumber;
}

// -----------------------------------------------------------------------
// Virtual, per-engine type calls

bool CarlaEngine::close()
{
    carla_debug("CarlaEngine::close()");

    if (pData->curPluginCount != 0)
    {
        pData->aboutToClose = true;
        removeAllPlugins();
    }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    if (pData->osc.isControlRegistered())
        oscSend_control_exit();
#endif

    pData->close();

    callback(ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0f, nullptr);
    return true;
}

bool CarlaEngine::usesConstantBufferSize() const noexcept
{
    return true;
}

void CarlaEngine::idle() noexcept
{
    CARLA_SAFE_ASSERT_RETURN(pData->nextAction.opcode == kEnginePostActionNull,);
    CARLA_SAFE_ASSERT_RETURN(pData->nextPluginId == pData->maxPluginNumber,);
    CARLA_SAFE_ASSERT_RETURN(getType() != kEngineTypePlugin,);

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
        {
            const uint hints(plugin->getHints());

            if ((hints & PLUGIN_HAS_CUSTOM_UI) != 0 && (hints & PLUGIN_NEEDS_UI_MAIN_THREAD) != 0)
            {
                try {
                    plugin->uiIdle();
                } CARLA_SAFE_EXCEPTION_CONTINUE("Plugin uiIdle");
            }
        }
    }

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    pData->osc.idle();
#endif
}

CarlaEngineClient* CarlaEngine::addClient(CarlaPlugin* const)
{
    return new CarlaEngineClient(*this);
}

// -----------------------------------------------------------------------
// Plugin management

bool CarlaEngine::addPlugin(const BinaryType btype, const PluginType ptype,
                            const char* const filename, const char* const name, const char* const label, const int64_t uniqueId,
                            const void* const extra, const uint options)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId <= pData->maxPluginNumber, "Invalid engine internal data");
#endif
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(btype != BINARY_NONE, "Invalid plugin binary mode");
    CARLA_SAFE_ASSERT_RETURN_ERR(ptype != PLUGIN_NONE, "Invalid plugin type");
    CARLA_SAFE_ASSERT_RETURN_ERR((filename != nullptr && filename[0] != '\0') || (label != nullptr && label[0] != '\0'), "Invalid plugin filename and label");
    carla_debug("CarlaEngine::addPlugin(%i:%s, %i:%s, \"%s\", \"%s\", \"%s\", " P_INT64 ", %p, %u)", btype, BinaryType2Str(btype), ptype, PluginType2Str(ptype), filename, name, label, uniqueId, extra, options);

    uint id;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaPlugin* oldPlugin = nullptr;

    if (pData->nextPluginId < pData->curPluginCount)
    {
        id = pData->nextPluginId;
        pData->nextPluginId = pData->maxPluginNumber;

        oldPlugin = pData->plugins[id].plugin;

        CARLA_SAFE_ASSERT_RETURN_ERR(oldPlugin != nullptr, "Invalid replace plugin Id");
    }
    else
#endif
    {
        id = pData->curPluginCount;

        if (id == pData->maxPluginNumber)
        {
            setLastError("Maximum number of plugins reached");
            return false;
        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins[id].plugin == nullptr, "Invalid engine internal data");
#endif
    }

    CarlaPlugin::Initializer initializer = {
        this,
        id,
        filename,
        name,
        label,
        uniqueId,
        options
    };

    CarlaPlugin* plugin = nullptr;
    CarlaString bridgeBinary(pData->options.binaryDir);

    if (bridgeBinary.isNotEmpty())
    {
#ifndef CARLA_OS_WIN
        if (btype == BINARY_NATIVE)
        {
            bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-native";
        }
        else
#endif
        {
            switch (btype)
            {
            case BINARY_POSIX32:
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-posix32";
                break;
            case BINARY_POSIX64:
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-posix64";
                break;
            case BINARY_WIN32:
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-win32.exe";
                break;
            case BINARY_WIN64:
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-win64.exe";
                break;
            default:
                bridgeBinary.clear();
                break;
            }
        }

        if (! File(bridgeBinary.buffer()).existsAsFile())
            bridgeBinary.clear();
    }

    // Prefer bridges for some specific plugins
    bool preferBridges = pData->options.preferPluginBridges;

#ifndef BUILD_BRIDGE
    if (! preferBridges)
    {
        if (ptype == PLUGIN_LV2 && label != nullptr)
        {
            if (std::strncmp(label, "http://calf.sourceforge.net/plugins/", 36) == 0 ||
                std::strcmp(label, "http://factorial.hu/plugins/lv2/ir") == 0 ||
                std::strstr(label, "v1.sourceforge.net/lv2") != nullptr)
            {
                preferBridges = true;
            }
        }
    }
#endif // ! BUILD_BRIDGE

    const bool canBeBridged = ptype != PLUGIN_INTERNAL
                           && ptype != PLUGIN_SF2
                           && ptype != PLUGIN_SFZ
                           && ptype != PLUGIN_JACK;

    if (canBeBridged && (btype != BINARY_NATIVE || (preferBridges && bridgeBinary.isNotEmpty())))
    {
        if (bridgeBinary.isNotEmpty())
        {
            plugin = CarlaPlugin::newBridge(initializer, btype, ptype, bridgeBinary);
        }
        else
        {
            setLastError("This Carla build cannot handle this binary");
            return false;
        }
    }
    else
    {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        bool use16Outs;
#endif
        setLastError("Invalid or unsupported plugin type");

        switch (ptype)
        {
        case PLUGIN_NONE:
            break;

        case PLUGIN_LADSPA:
            plugin = CarlaPlugin::newLADSPA(initializer, (const LADSPA_RDF_Descriptor*)extra);
            break;

        case PLUGIN_DSSI:
            plugin = CarlaPlugin::newDSSI(initializer);
            break;

        case PLUGIN_LV2:
            plugin = CarlaPlugin::newLV2(initializer);
            break;

        case PLUGIN_VST2:
            plugin = CarlaPlugin::newVST2(initializer);
            break;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        case PLUGIN_INTERNAL:
            plugin = CarlaPlugin::newNative(initializer);
            break;

        case PLUGIN_SF2:
            use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
            plugin = CarlaPlugin::newFluidSynth(initializer, use16Outs);
            break;

        case PLUGIN_SFZ:
            plugin = CarlaPlugin::newSFZero(initializer);
            break;

        case PLUGIN_JACK:
            plugin = CarlaPlugin::newJackApp(initializer);
            break;
#else
        case PLUGIN_INTERNAL:
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
        case PLUGIN_JACK:
            setLastError("Plugin bridges cannot handle this binary");
            break;
#endif
        }
    }

    if (plugin == nullptr)
        return false;

    plugin->reload();

    bool canRun = true;

    /**/ if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (plugin->getCVInCount() > 0 || plugin->getCVInCount() > 0)
        {
            setLastError("Carla's rack mode cannot work with plugins that have CV ports, sorry!");
            canRun = false;
        }
    }
    else if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        /**/ if (plugin->getMidiInCount() > 1 || plugin->getMidiOutCount() > 1)
        {
            setLastError("Carla's patchbay mode cannot work with plugins that have multiple MIDI ports, sorry!");
            canRun = false;
        }
        else if (plugin->getCVInCount() > 0 || plugin->getCVInCount() > 0)
        {
            setLastError("CV ports in patchbay mode is still TODO");
            canRun = false;
        }
    }

    if (! canRun)
    {
        delete plugin;
        return false;
    }

    EnginePluginData& pluginData(pData->plugins[id]);
    pluginData.plugin      = plugin;
    pluginData.insPeak[0]  = 0.0f;
    pluginData.insPeak[1]  = 0.0f;
    pluginData.outsPeak[0] = 0.0f;
    pluginData.outsPeak[1] = 0.0f;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (oldPlugin != nullptr)
    {
        CARLA_SAFE_ASSERT(! pData->loadingProject);

        const ScopedThreadStopper sts(this);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
            pData->graph.replacePlugin(oldPlugin, plugin);

        const bool  wasActive = oldPlugin->getInternalParameterValue(PARAMETER_ACTIVE) >= 0.5f;
        const float oldDryWet = oldPlugin->getInternalParameterValue(PARAMETER_DRYWET);
        const float oldVolume = oldPlugin->getInternalParameterValue(PARAMETER_VOLUME);

        delete oldPlugin;

        if (plugin->getHints() & PLUGIN_CAN_DRYWET)
            plugin->setDryWet(oldDryWet, true, true);

        if (plugin->getHints() & PLUGIN_CAN_VOLUME)
            plugin->setVolume(oldVolume, true, true);

        plugin->setActive(wasActive, true, true);
        plugin->setEnabled(true);

        callback(ENGINE_CALLBACK_RELOAD_ALL, id, 0, 0, 0.0f, nullptr);
    }
    else if (! pData->loadingProject)
#endif
    {
        plugin->setEnabled(true);

        ++pData->curPluginCount;
        callback(ENGINE_CALLBACK_PLUGIN_ADDED, id, 0, 0, 0.0f, plugin->getName());

        if (getType() != kEngineTypeBridge)
            plugin->setActive(true, false, true);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
            pData->graph.addPlugin(plugin);
#endif
    }

#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
    plugin->registerToOscClient();
#endif

    return true;
}

bool CarlaEngine::addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, const int64_t uniqueId, const void* const extra)
{
    return addPlugin(BINARY_NATIVE, ptype, filename, name, label, uniqueId, extra, 0x0);
}

bool CarlaEngine::removePlugin(const uint id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data");
#endif
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id");
    carla_debug("CarlaEngine::removePlugin(%i)", id);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to remove");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

    const ScopedThreadStopper sts(this);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        pData->graph.removePlugin(plugin);

    const ScopedActionLock sal(this, kEnginePostActionRemovePlugin, id, 0);

    /*
    for (uint i=id; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin2(pData->plugins[i].plugin);
        CARLA_SAFE_ASSERT_BREAK(plugin2 != nullptr);
        plugin2->updateOscURL();
    }
    */

# if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    if (isOscControlRegistered())
        oscSend_control_remove_plugin(id);
# endif
#else
    pData->curPluginCount = 0;
    carla_zeroStructs(pData->plugins, 1);
#endif

    delete plugin;

    callback(ENGINE_CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0.0f, nullptr);
    return true;
}

bool CarlaEngine::removeAllPlugins()
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextPluginId == pData->maxPluginNumber, "Invalid engine internal data");
#endif
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    carla_debug("CarlaEngine::removeAllPlugins()");

    if (pData->curPluginCount == 0)
        return true;

    const ScopedThreadStopper sts(this);

    const uint curPluginCount(pData->curPluginCount);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        pData->graph.removeAllPlugins();

# if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    if (isOscControlRegistered())
    {
        for (uint i=0; i < curPluginCount; ++i)
            oscSend_control_remove_plugin(curPluginCount-i-1);
    }
# endif
#endif

    const ScopedActionLock sal(this, kEnginePostActionZeroCount, 0, 0);

    callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

    for (uint i=0; i < curPluginCount; ++i)
    {
        EnginePluginData& pluginData(pData->plugins[i]);

        if (pluginData.plugin != nullptr)
        {
            delete pluginData.plugin;
            pluginData.plugin = nullptr;
        }

        pluginData.insPeak[0]  = 0.0f;
        pluginData.insPeak[1]  = 0.0f;
        pluginData.outsPeak[0] = 0.0f;
        pluginData.outsPeak[1] = 0.0f;

        callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);
    }

    return true;
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
const char* CarlaEngine::renamePlugin(const uint id, const char* const newName)
{
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->curPluginCount != 0, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERRN(id < pData->curPluginCount, "Invalid plugin Id");
    CARLA_SAFE_ASSERT_RETURN_ERRN(newName != nullptr && newName[0] != '\0', "Invalid plugin name");
    carla_debug("CarlaEngine::renamePlugin(%i, \"%s\")", id, newName);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);
    CARLA_SAFE_ASSERT_RETURN_ERRN(plugin != nullptr, "Could not find plugin to rename");
    CARLA_SAFE_ASSERT_RETURN_ERRN(plugin->getId() == id, "Invalid engine internal data");

    const char* const uniqueName(getUniquePluginName(newName));
    CARLA_SAFE_ASSERT_RETURN_ERRN(uniqueName != nullptr, "Unable to get new unique plugin name");

    plugin->setName(uniqueName);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        pData->graph.renamePlugin(plugin, uniqueName);

    delete[] uniqueName;
    return plugin->getName();
}

bool CarlaEngine::clonePlugin(const uint id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id");
    carla_debug("CarlaEngine::clonePlugin(%i)", id);

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to clone");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

    char label[STR_MAX+1];
    carla_zeroChars(label, STR_MAX+1);
    plugin->getLabel(label);

    const uint pluginCountBefore(pData->curPluginCount);

    if (! addPlugin(plugin->getBinaryType(), plugin->getType(),
                    plugin->getFilename(), plugin->getName(), label, plugin->getUniqueId(),
                    plugin->getExtraStuff(), plugin->getOptionsEnabled()))
        return false;

    CARLA_SAFE_ASSERT_RETURN_ERR(pluginCountBefore+1 == pData->curPluginCount, "No new plugin found");

    if (CarlaPlugin* const newPlugin = pData->plugins[pluginCountBefore].plugin)
        newPlugin->loadStateSave(plugin->getStateSave());

    return true;
}

bool CarlaEngine::replacePlugin(const uint id) noexcept
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    carla_debug("CarlaEngine::replacePlugin(%i)", id);

    // might use this to reset
    if (id == pData->maxPluginNumber)
    {
        pData->nextPluginId = pData->maxPluginNumber;
        return true;
    }

    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id");

    CarlaPlugin* const plugin(pData->plugins[id].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin != nullptr, "Could not find plugin to replace");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

    pData->nextPluginId = id;

    return true;
}

bool CarlaEngine::switchPlugins(const uint idA, const uint idB) noexcept
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount >= 2, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(idA != idB, "Invalid operation, cannot switch plugin with itself");
    CARLA_SAFE_ASSERT_RETURN_ERR(idA < pData->curPluginCount, "Invalid plugin Id");
    CARLA_SAFE_ASSERT_RETURN_ERR(idB < pData->curPluginCount, "Invalid plugin Id");
    carla_debug("CarlaEngine::switchPlugins(%i)", idA, idB);

    CarlaPlugin* const pluginA(pData->plugins[idA].plugin);
    CarlaPlugin* const pluginB(pData->plugins[idB].plugin);

    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA != nullptr, "Could not find plugin to switch");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA != nullptr, "Could not find plugin to switch");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA->getId() == idA, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginB->getId() == idB, "Invalid engine internal data");

    const ScopedThreadStopper sts(this);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        pData->graph.replacePlugin(pluginA, pluginB);

    const ScopedActionLock sal(this, kEnginePostActionSwitchPlugins, idA, idB);

    // TODO
    /*
    pluginA->updateOscURL();
    pluginB->updateOscURL();

    if (isOscControlRegistered())
        oscSend_control_switch_plugins(idA, idB);
    */

    return true;
}
#endif

CarlaPlugin* CarlaEngine::getPlugin(const uint id) const noexcept
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->curPluginCount != 0, "Invalid engine internal data");
#endif
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERRN(id < pData->curPluginCount, "Invalid plugin Id");

    return pData->plugins[id].plugin;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const uint id) const noexcept
{
    return pData->plugins[id].plugin;
}

const char* CarlaEngine::getUniquePluginName(const char* const name) const
{
    CARLA_SAFE_ASSERT_RETURN(pData->nextAction.opcode == kEnginePostActionNull, nullptr);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngine::getUniquePluginName(\"%s\")", name);

    CarlaString sname;
    sname = name;

    if (sname.isEmpty())
    {
        sname = "(No name)";
        return sname.dup();
    }

    const std::size_t maxNameSize(carla_minConstrained<uint>(getMaxClientNameSize(), 0xff, 6U) - 6); // 6 = strlen(" (10)") + 1

    if (maxNameSize == 0 || ! isRunning())
        return sname.dup();

    sname.truncate(maxNameSize);
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CARLA_SAFE_ASSERT_BREAK(pData->plugins[i].plugin != nullptr);

        // Check if unique name doesn't exist
        if (const char* const pluginName = pData->plugins[i].plugin->getName())
        {
            if (sname != pluginName)
                continue;
        }

        // Check if string has already been modified
        {
            const std::size_t len(sname.length());

            // 1 digit, ex: " (2)"
            if (sname[len-4] == ' ' && sname[len-3] == '(' && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                const int number = sname[len-2] - '0';

                if (number == 9)
                {
                    // next number is 10, 2 digits
                    sname.truncate(len-4);
                    sname += " (10)";
                    //sname.replace(" (9)", " (10)");
                }
                else
                    sname[len-2] = char('0' + number + 1);

                continue;
            }

            // 2 digits, ex: " (11)"
            if (sname[len-5] == ' ' && sname[len-4] == '(' && sname.isDigit(len-3) && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                char n2 = sname[len-2];
                char n3 = sname[len-3];

                if (n2 == '9')
                {
                    n2 = '0';
                    n3 = static_cast<char>(n3 + 1);
                }
                else
                    n2 = static_cast<char>(n2 + 1);

                sname[len-2] = n2;
                sname[len-3] = n3;

                continue;
            }
        }

        // Modify string if not
        sname += " (2)";
    }

    return sname.dup();
}

// -----------------------------------------------------------------------
// Project management

bool CarlaEngine::loadFile(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename");
    carla_debug("CarlaEngine::loadFile(\"%s\")", filename);

    const String jfilename = String(CharPointer_UTF8(filename));
    File file(jfilename);
    CARLA_SAFE_ASSERT_RETURN_ERR(file.exists(), "Requested file does not exist or is not a readable");

    CarlaString baseName(file.getFileNameWithoutExtension().toRawUTF8());
    CarlaString extension(file.getFileExtension().replace(".","").toLowerCase().toRawUTF8());

    const uint curPluginId(pData->nextPluginId < pData->curPluginCount ? pData->nextPluginId : pData->curPluginCount);

    // -------------------------------------------------------------------
    // NOTE: please keep in sync with carla_get_supported_file_extensions!!

    if (extension == "carxp" || extension == "carxs")
        return loadProject(filename);

    // -------------------------------------------------------------------

    if (extension == "sf2" || extension == "sf3")
        return addPlugin(PLUGIN_SF2, filename, baseName, baseName, 0, nullptr);

    if (extension == "sfz")
        return addPlugin(PLUGIN_SFZ, filename, baseName, baseName, 0, nullptr);

    // -------------------------------------------------------------------

    if (
#ifdef HAVE_SNDFILE
        extension == "aif"  ||
        extension == "aifc" ||
        extension == "aiff" ||
        extension == "au"   ||
        extension == "bwf"  ||
        extension == "flac" ||
        extension == "htk"  ||
        extension == "iff"  ||
        extension == "mat4" ||
        extension == "mat5" ||
        extension == "oga"  ||
        extension == "ogg"  ||
        extension == "paf"  ||
        extension == "pvf"  ||
        extension == "pvf5" ||
        extension == "sd2"  ||
        extension == "sf"   ||
        extension == "snd"  ||
        extension == "svx"  ||
        extension == "vcc"  ||
        extension == "w64"  ||
        extension == "wav"  ||
        extension == "xi"   ||
#endif
#ifdef HAVE_FFMPEG
        extension == "3g2" ||
        extension == "3gp" ||
        extension == "aac" ||
        extension == "ac3" ||
        extension == "amr" ||
        extension == "ape" ||
        extension == "mp2" ||
        extension == "mp3" ||
        extension == "mpc" ||
        extension == "wma" ||
# ifdef HAVE_SNDFILE
        // FFmpeg without sndfile
        extension == "flac" ||
        extension == "oga"  ||
        extension == "ogg"  ||
        extension == "w64"  ||
        extension == "wav"  ||
# endif
#endif
        false
       )
    {
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "audiofile", 0, nullptr))
        {
            if (CarlaPlugin* const plugin = getPlugin(curPluginId))
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------

    if (extension == "mid" || extension == "midi")
    {
        if (addPlugin(PLUGIN_INTERNAL, nullptr, baseName, "midifile", 0, nullptr))
        {
            if (CarlaPlugin* const plugin = getPlugin(curPluginId))
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "file", filename, true);
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------
    // ZynAddSubFX

    if (extension == "xmz" || extension == "xiz")
    {
#ifdef HAVE_ZYN_DEPS
        CarlaString nicerName("Zyn - ");

        const std::size_t sep(baseName.find('-')+1);

        if (sep < baseName.length())
            nicerName += baseName.buffer()+sep;
        else
            nicerName += baseName;

        //nicerName
        if (addPlugin(PLUGIN_INTERNAL, nullptr, nicerName, "zynaddsubfx", 0, nullptr))
        {
            callback(ENGINE_CALLBACK_UI_STATE_CHANGED, curPluginId, 0, 0, 0.0f, nullptr);

            if (CarlaPlugin* const plugin = getPlugin(curPluginId))
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, (extension == "xmz") ? "CarlaAlternateFile1" : "CarlaAlternateFile2", filename, true);
            return true;
        }
        return false;
#else
        setLastError("This Carla build does not have ZynAddSubFX support");
        return false;
#endif
    }

    // -------------------------------------------------------------------
    // Direct plugin binaries

#ifdef CARLA_OS_MAC
    if (extension == "vst")
        return addPlugin(PLUGIN_VST2, filename, nullptr, nullptr, 0, nullptr);
#else
    if (extension == "dll" || extension == "so")
        return addPlugin(getBinaryTypeFromFile(filename), PLUGIN_VST2, filename, nullptr, nullptr, 0, nullptr, 0x0);
#endif

    // -------------------------------------------------------------------

    setLastError("Unknown file extension");
    return false;
}

bool CarlaEngine::loadProject(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename");
    carla_debug("CarlaEngine::loadProject(\"%s\")", filename);

    const String jfilename = String(CharPointer_UTF8(filename));
    File file(jfilename);
    CARLA_SAFE_ASSERT_RETURN_ERR(file.existsAsFile(), "Requested file does not exist or is not a readable file");

    XmlDocument xml(file);
    return loadProjectInternal(xml);
}

bool CarlaEngine::saveProject(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename");
    carla_debug("CarlaEngine::saveProject(\"%s\")", filename);

    MemoryOutputStream out;
    saveProjectInternal(out);

    const String jfilename = String(CharPointer_UTF8(filename));
    File file(jfilename);

    if (file.replaceWithData(out.getData(), out.getDataSize()))
        return true;

    setLastError("Failed to write file");
    return false;
}

// -----------------------------------------------------------------------
// Information (base)

uint CarlaEngine::getHints() const noexcept
{
    return pData->hints;
}

uint32_t CarlaEngine::getBufferSize() const noexcept
{
    return pData->bufferSize;
}

double CarlaEngine::getSampleRate() const noexcept
{
    return pData->sampleRate;
}

const char* CarlaEngine::getName() const noexcept
{
    return pData->name;
}

EngineProcessMode CarlaEngine::getProccessMode() const noexcept
{
    return pData->options.processMode;
}

const EngineOptions& CarlaEngine::getOptions() const noexcept
{
    return pData->options;
}

EngineTimeInfo CarlaEngine::getTimeInfo() const noexcept
{
    return pData->timeInfo;
}

// -----------------------------------------------------------------------
// Information (peaks)

float CarlaEngine::getInputPeak(const uint pluginId, const bool isLeft) const noexcept
{
    if (pluginId == MAIN_CARLA_PLUGIN_ID)
    {
        // get peak from first plugin, if available
        if (pData->curPluginCount > 0)
            return pData->plugins[0].insPeak[isLeft ? 0 : 1];
        return 0.0f;
    }

    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);

    return pData->plugins[pluginId].insPeak[isLeft ? 0 : 1];
}

float CarlaEngine::getOutputPeak(const uint pluginId, const bool isLeft) const noexcept
{
    if (pluginId == MAIN_CARLA_PLUGIN_ID)
    {
        // get peak from last plugin, if available
        if (pData->curPluginCount > 0)
            return pData->plugins[pData->curPluginCount-1].outsPeak[isLeft ? 0 : 1];
        return 0.0f;
    }

    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);

    return pData->plugins[pluginId].outsPeak[isLeft ? 0 : 1];
}

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr) noexcept
{
#ifdef DEBUG
    if (pData->isIdling)
        carla_stdout("CarlaEngine::callback [while idling] (%i:%s, %i, %i, %i, %f, \"%s\")",
                     action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
    else if (action != ENGINE_CALLBACK_IDLE && action != ENGINE_CALLBACK_NOTE_ON && action != ENGINE_CALLBACK_NOTE_OFF)
        carla_debug("CarlaEngine::callback(%i:%s, %i, %i, %i, %f, \"%s\")",
                    action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
#endif

    if (pData->callback != nullptr)
    {
        if (action == ENGINE_CALLBACK_IDLE)
            ++pData->isIdling;

        try {
            pData->callback(pData->callbackPtr, action, pluginId, value1, value2, value3, valueStr);
#if defined(CARLA_OS_LINUX) && defined(__arm__)
        } catch (__cxxabiv1::__forced_unwind&) {
            carla_stderr2("Caught forced unwind exception in callback");
            throw;
#endif
        } catch (...) {
            carla_safe_exception("callback", __FILE__, __LINE__);
        }

        if (action == ENGINE_CALLBACK_IDLE)
            --pData->isIdling;
    }
}

void CarlaEngine::setCallback(const EngineCallbackFunc func, void* const ptr) noexcept
{
    carla_debug("CarlaEngine::setCallback(%p, %p)", func, ptr);

    pData->callback    = func;
    pData->callbackPtr = ptr;
}

// -----------------------------------------------------------------------
// File Callback

const char* CarlaEngine::runFileCallback(const FileCallbackOpcode action, const bool isDir, const char* const title, const char* const filter) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(title != nullptr && title[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(filter != nullptr, nullptr);
    carla_debug("CarlaEngine::runFileCallback(%i:%s, %s, \"%s\", \"%s\")", action, FileCallbackOpcode2Str(action), bool2str(isDir), title, filter);

    const char* ret = nullptr;

    if (pData->fileCallback != nullptr)
    {
        try {
            ret = pData->fileCallback(pData->fileCallbackPtr, action, isDir, title, filter);
        } CARLA_SAFE_EXCEPTION("runFileCallback");
    }

    return ret;
}

void CarlaEngine::setFileCallback(const FileCallbackFunc func, void* const ptr) noexcept
{
    carla_debug("CarlaEngine::setFileCallback(%p, %p)", func, ptr);

    pData->fileCallback    = func;
    pData->fileCallbackPtr = ptr;
}

// -----------------------------------------------------------------------
// Transport

void CarlaEngine::transportPlay() noexcept
{
    pData->timeInfo.playing = true;
    pData->time.setNeedsReset();
}

void CarlaEngine::transportPause() noexcept
{
    if (pData->timeInfo.playing)
        pData->time.pause();
    else
        pData->time.setNeedsReset();
}

void CarlaEngine::transportBPM(const double bpm) noexcept
{
    try {
        pData->time.setBPM(bpm);
    } CARLA_SAFE_EXCEPTION("CarlaEngine::transportBPM");
}

void CarlaEngine::transportRelocate(const uint64_t frame) noexcept
{
    pData->time.relocate(frame);
}

// -----------------------------------------------------------------------
// Error handling

const char* CarlaEngine::getLastError() const noexcept
{
    return pData->lastError;
}

void CarlaEngine::setLastError(const char* const error) const noexcept
{
    pData->lastError = error;
}

// -----------------------------------------------------------------------
// Misc

bool CarlaEngine::isAboutToClose() const noexcept
{
    return pData->aboutToClose;
}

bool CarlaEngine::setAboutToClose() noexcept
{
    carla_debug("CarlaEngine::setAboutToClose()");

    pData->aboutToClose = true;

    return (pData->isIdling == 0);
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
bool CarlaEngine::isLoadingProject() const noexcept
{
    return pData->loadingProject;
}
#endif

// -----------------------------------------------------------------------
// Global options

void CarlaEngine::setOption(const EngineOption option, const int value, const char* const valueStr) noexcept
{
    carla_debug("CarlaEngine::setOption(%i:%s, %i, \"%s\")", option, EngineOption2Str(option), value, valueStr);

    if (isRunning())
    {
        switch (option)
        {
        case ENGINE_OPTION_PROCESS_MODE:
        case ENGINE_OPTION_AUDIO_TRIPLE_BUFFER:
        case ENGINE_OPTION_AUDIO_DEVICE:
            return carla_stderr("CarlaEngine::setOption(%i:%s, %i, \"%s\") - Cannot set this option while engine is running!",
                                option, EngineOption2Str(option), value, valueStr);
        default:
            break;
        }
    }

    // do not un-force stereo for rack mode
    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && option == ENGINE_OPTION_FORCE_STEREO && value != 0)
        return;

    switch (option)
    {
    case ENGINE_OPTION_DEBUG:
        break;

    case ENGINE_OPTION_PROCESS_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= ENGINE_PROCESS_MODE_SINGLE_CLIENT && value <= ENGINE_PROCESS_MODE_BRIDGE,);
        pData->options.processMode = static_cast<EngineProcessMode>(value);
        break;

    case ENGINE_OPTION_TRANSPORT_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= ENGINE_TRANSPORT_MODE_DISABLED && value <= ENGINE_TRANSPORT_MODE_BRIDGE,);
        CARLA_SAFE_ASSERT_RETURN(getType() == kEngineTypeJack || value != ENGINE_TRANSPORT_MODE_JACK,);
        pData->options.transportMode = static_cast<EngineTransportMode>(value);
        delete[] pData->options.transportExtra;
        if (value >= ENGINE_TRANSPORT_MODE_DISABLED && valueStr != nullptr)
            pData->options.transportExtra = carla_strdup_safe(valueStr);
        else
            pData->options.transportExtra = nullptr;

        pData->time.setNeedsReset();

#if defined(HAVE_HYLIA) && !defined(BUILD_BRIDGE)
        // enable link now if needed
        {
            const bool linkEnabled = pData->options.transportExtra != nullptr && std::strstr(pData->options.transportExtra, ":link:") != nullptr;
            pData->time.enableLink(linkEnabled);
        }
#endif
        break;

    case ENGINE_OPTION_FORCE_STEREO:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.forceStereo = (value != 0);
        break;

    case ENGINE_OPTION_PREFER_PLUGIN_BRIDGES:
#ifdef BUILD_BRIDGE_ALTERNATIVE_ARCH
        CARLA_SAFE_ASSERT_RETURN(value == 0,);
#else
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
#endif
        pData->options.preferPluginBridges = (value != 0);
        break;

    case ENGINE_OPTION_PREFER_UI_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.preferUiBridges = (value != 0);
        break;

    case ENGINE_OPTION_UIS_ALWAYS_ON_TOP:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.uisAlwaysOnTop = (value != 0);
        break;

    case ENGINE_OPTION_MAX_PARAMETERS:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        pData->options.maxParameters = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_UI_BRIDGES_TIMEOUT:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        pData->options.uiBridgesTimeout = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_BUFFER_SIZE:
        CARLA_SAFE_ASSERT_RETURN(value >= 8,);
        pData->options.audioBufferSize = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_SAMPLE_RATE:
        CARLA_SAFE_ASSERT_RETURN(value >= 22050,);
        pData->options.audioSampleRate = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_AUDIO_TRIPLE_BUFFER:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.audioTripleBuffer = (value != 0);
        break;

    case ENGINE_OPTION_AUDIO_DEVICE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

        if (pData->options.audioDevice != nullptr)
            delete[] pData->options.audioDevice;

        pData->options.audioDevice = carla_strdup_safe(valueStr);
        break;

    case ENGINE_OPTION_PLUGIN_PATH:
        CARLA_SAFE_ASSERT_RETURN(value > PLUGIN_NONE,);
        CARLA_SAFE_ASSERT_RETURN(value <= PLUGIN_SFZ,);

        switch (value)
        {
        case PLUGIN_LADSPA:
            if (pData->options.pathLADSPA != nullptr)
                delete[] pData->options.pathLADSPA;
            if (valueStr != nullptr)
                pData->options.pathLADSPA = carla_strdup_safe(valueStr);
            else
                pData->options.pathLADSPA = nullptr;
            break;
        case PLUGIN_DSSI:
            if (pData->options.pathDSSI != nullptr)
                delete[] pData->options.pathDSSI;
            if (valueStr != nullptr)
                pData->options.pathDSSI = carla_strdup_safe(valueStr);
            else
                pData->options.pathDSSI = nullptr;
            break;
        case PLUGIN_LV2:
            if (pData->options.pathLV2 != nullptr)
                delete[] pData->options.pathLV2;
            if (valueStr != nullptr)
                pData->options.pathLV2 = carla_strdup_safe(valueStr);
            else
                pData->options.pathLV2 = nullptr;
            break;
        case PLUGIN_VST2:
            if (pData->options.pathVST2 != nullptr)
                delete[] pData->options.pathVST2;
            if (valueStr != nullptr)
                pData->options.pathVST2 = carla_strdup_safe(valueStr);
            else
                pData->options.pathVST2 = nullptr;
            break;
        case PLUGIN_SF2:
            if (pData->options.pathSF2 != nullptr)
                delete[] pData->options.pathSF2;
            if (valueStr != nullptr)
                pData->options.pathSF2 = carla_strdup_safe(valueStr);
            else
                pData->options.pathSF2 = nullptr;
            break;
        case PLUGIN_SFZ:
            if (pData->options.pathSFZ != nullptr)
                delete[] pData->options.pathSFZ;
            if (valueStr != nullptr)
                pData->options.pathSFZ = carla_strdup_safe(valueStr);
            else
                pData->options.pathSFZ = nullptr;
            break;
        default:
            return carla_stderr("CarlaEngine::setOption(%i:%s, %i, \"%s\") - Invalid plugin type", option, EngineOption2Str(option), value, valueStr);
            break;
        }
        break;

    case ENGINE_OPTION_PATH_BINARIES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.binaryDir != nullptr)
            delete[] pData->options.binaryDir;

        pData->options.binaryDir = carla_strdup_safe(valueStr);
        break;

    case ENGINE_OPTION_PATH_RESOURCES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.resourceDir != nullptr)
            delete[] pData->options.resourceDir;

        pData->options.resourceDir = carla_strdup_safe(valueStr);
        break;

    case ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR: {
        CARLA_SAFE_ASSERT_RETURN(pData->options.binaryDir != nullptr && pData->options.binaryDir[0] != '\0',);

#ifdef CARLA_OS_LINUX
        const ScopedEngineEnvironmentLocker _seel(this);

        if (value != 0)
        {
            CarlaString interposerPath(CarlaString(pData->options.binaryDir) + "/libcarla_interposer-safe.so");
            ::setenv("LD_PRELOAD", interposerPath.buffer(), 1);
        }
        else
        {
            ::unsetenv("LD_PRELOAD");
        }
#endif
    }   break;

    case ENGINE_OPTION_FRONTEND_WIN_ID: {
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);
        const long long winId(std::strtoll(valueStr, nullptr, 16));
        CARLA_SAFE_ASSERT_RETURN(winId >= 0,);
        pData->options.frontendWinId = static_cast<uintptr_t>(winId);
    }   break;

#if !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) && !defined(CARLA_OS_WIN)
    case ENGINE_OPTION_WINE_EXECUTABLE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.wine.executable != nullptr)
            delete[] pData->options.wine.executable;

        pData->options.wine.executable = carla_strdup_safe(valueStr);
        break;

    case ENGINE_OPTION_WINE_AUTO_PREFIX:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.wine.autoPrefix = (value != 0);
        break;

    case ENGINE_OPTION_WINE_FALLBACK_PREFIX:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (pData->options.wine.fallbackPrefix != nullptr)
            delete[] pData->options.wine.fallbackPrefix;

        pData->options.wine.fallbackPrefix = carla_strdup_safe(valueStr);
        break;

    case ENGINE_OPTION_WINE_RT_PRIO_ENABLED:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.wine.rtPrio = (value != 0);
        break;

    case ENGINE_OPTION_WINE_BASE_RT_PRIO:
        CARLA_SAFE_ASSERT_RETURN(value >= 1 && value <= 89,);
        pData->options.wine.baseRtPrio = value;
        break;

    case ENGINE_OPTION_WINE_SERVER_RT_PRIO:
        CARLA_SAFE_ASSERT_RETURN(value >= 1 && value <= 99,);
        pData->options.wine.serverRtPrio = value;
        break;
#endif

    case ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT:
        break;
    }
}

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// OSC Stuff

bool CarlaEngine::isOscControlRegistered() const noexcept
{
# ifdef HAVE_LIBLO
    return pData->osc.isControlRegistered();
# else
    return false;
# endif
}

void CarlaEngine::idleOsc() const noexcept
{
# ifdef HAVE_LIBLO
    pData->osc.idle();
# endif
}

const char* CarlaEngine::getOscServerPathTCP() const noexcept
{
# ifdef HAVE_LIBLO
    return pData->osc.getServerPathTCP();
# else
    return nullptr;
# endif
}

const char* CarlaEngine::getOscServerPathUDP() const noexcept
{
# ifdef HAVE_LIBLO
    return pData->osc.getServerPathUDP();
# else
    return nullptr;
# endif
}
#endif

// -----------------------------------------------------------------------
// Helper functions

EngineEvent* CarlaEngine::getInternalEventBuffer(const bool isInput) const noexcept
{
    return isInput ? pData->events.in : pData->events.out;
}

// -----------------------------------------------------------------------
// Internal stuff

void CarlaEngine::bufferSizeChanged(const uint32_t newBufferSize)
{
    carla_debug("CarlaEngine::bufferSizeChanged(%i)", newBufferSize);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
        pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        pData->graph.setBufferSize(newBufferSize);
    }
#endif

    pData->time.updateAudioValues(newBufferSize, pData->sampleRate);

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
        {
            plugin->tryLock(true);
            plugin->bufferSizeChanged(newBufferSize);
            plugin->unlock();
        }
    }

    callback(ENGINE_CALLBACK_BUFFER_SIZE_CHANGED, 0, static_cast<int>(newBufferSize), 0, 0.0f, nullptr);
}

void CarlaEngine::sampleRateChanged(const double newSampleRate)
{
    carla_debug("CarlaEngine::sampleRateChanged(%g)", newSampleRate);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
        pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        pData->graph.setSampleRate(newSampleRate);
    }
#endif

    pData->time.updateAudioValues(pData->bufferSize, newSampleRate);

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
        {
            plugin->tryLock(true);
            plugin->sampleRateChanged(newSampleRate);
            plugin->unlock();
        }
    }

    callback(ENGINE_CALLBACK_SAMPLE_RATE_CHANGED, 0, 0, 0, static_cast<float>(newSampleRate), nullptr);
}

void CarlaEngine::offlineModeChanged(const bool isOfflineNow)
{
    carla_debug("CarlaEngine::offlineModeChanged(%s)", bool2str(isOfflineNow));

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK ||
        pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        pData->graph.setOffline(isOfflineNow);
    }
#endif

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
            plugin->offlineModeChanged(isOfflineNow);
    }
}

void CarlaEngine::setPluginPeaks(const uint pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept
{
    EnginePluginData& pluginData(pData->plugins[pluginId]);

    pluginData.insPeak[0]  = inPeaks[0];
    pluginData.insPeak[1]  = inPeaks[1];
    pluginData.outsPeak[0] = outPeaks[0];
    pluginData.outsPeak[1] = outPeaks[1];
}

void CarlaEngine::saveProjectInternal(water::MemoryOutputStream& outStream) const
{
    // send initial prepareForSave first, giving time for bridges to act
    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
        {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            // deactivate bridge client-side ping check, since some plugins block during save
            if (plugin->getHints() & PLUGIN_IS_BRIDGE)
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "__CarlaPingOnOff__", "false", false);
#endif
            plugin->prepareForSave();
        }
    }

    outStream << "<?xml version='1.0' encoding='UTF-8'?>\n";
    outStream << "<!DOCTYPE CARLA-PROJECT>\n";
    outStream << "<CARLA-PROJECT VERSION='2.0'>\n";

    const bool isPlugin(getType() == kEngineTypePlugin);
    const EngineOptions& options(pData->options);

    {
        MemoryOutputStream outSettings(1024);

        outSettings << " <EngineSettings>\n";

        outSettings << "  <ForceStereo>"         << bool2str(options.forceStereo)         << "</ForceStereo>\n";
        outSettings << "  <PreferPluginBridges>" << bool2str(options.preferPluginBridges) << "</PreferPluginBridges>\n";
        outSettings << "  <PreferUiBridges>"     << bool2str(options.preferUiBridges)     << "</PreferUiBridges>\n";
        outSettings << "  <UIsAlwaysOnTop>"      << bool2str(options.uisAlwaysOnTop)      << "</UIsAlwaysOnTop>\n";

        outSettings << "  <MaxParameters>"       << String(options.maxParameters)    << "</MaxParameters>\n";
        outSettings << "  <UIBridgesTimeout>"    << String(options.uiBridgesTimeout) << "</UIBridgesTimeout>\n";

        if (isPlugin)
        {
            outSettings << "  <LADSPA_PATH>" << xmlSafeString(options.pathLADSPA, true) << "</LADSPA_PATH>\n";
            outSettings << "  <DSSI_PATH>"   << xmlSafeString(options.pathDSSI,   true) << "</DSSI_PATH>\n";
            outSettings << "  <LV2_PATH>"    << xmlSafeString(options.pathLV2,    true) << "</LV2_PATH>\n";
            outSettings << "  <VST2_PATH>"   << xmlSafeString(options.pathVST2,   true) << "</VST2_PATH>\n";
            outSettings << "  <SF2_PATH>"    << xmlSafeString(options.pathSF2,    true) << "</SF2_PATH>\n";
            outSettings << "  <SFZ_PATH>"    << xmlSafeString(options.pathSFZ,    true) << "</SFZ_PATH>\n";
        }

        outSettings << " </EngineSettings>\n";
        outStream << outSettings;
    }

    if (pData->timeInfo.bbt.valid && ! isPlugin)
    {
        MemoryOutputStream outTransport(128);

        outTransport << "\n <Transport>\n";
        // outTransport << "  <BeatsPerBar>"    << pData->timeInfo.bbt.beatsPerBar    << "</BeatsPerBar>\n";
        outTransport << "  <BeatsPerMinute>" << pData->timeInfo.bbt.beatsPerMinute << "</BeatsPerMinute>\n";
        outTransport << " </Transport>\n";
        outStream << outTransport;
    }

    char strBuf[STR_MAX+1];

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled())
        {
            MemoryOutputStream outPlugin(4096), streamPlugin;
            plugin->getStateSave(false).dumpToMemoryStream(streamPlugin);

            outPlugin << "\n";

            strBuf[0] = '\0';
            plugin->getRealName(strBuf);

            if (strBuf[0] != '\0')
                outPlugin << " <!-- " << xmlSafeString(strBuf, true) << " -->\n";

            outPlugin << " <Plugin>\n";
            outPlugin << streamPlugin;
            outPlugin << " </Plugin>\n";
            outStream << outPlugin;
        }
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // tell bridges we're done saving
    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled() && (plugin->getHints() & PLUGIN_IS_BRIDGE) != 0)
            plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "__CarlaPingOnOff__", "true", false);
    }

    // save internal connections
    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        if (const char* const* const patchbayConns = getPatchbayConnections(false))
        {
            MemoryOutputStream outPatchbay(2048);

            outPatchbay << "\n <Patchbay>\n";

            for (int i=0; patchbayConns[i] != nullptr && patchbayConns[i+1] != nullptr; ++i, ++i )
            {
                const char* const connSource(patchbayConns[i]);
                const char* const connTarget(patchbayConns[i+1]);

                CARLA_SAFE_ASSERT_CONTINUE(connSource != nullptr && connSource[0] != '\0');
                CARLA_SAFE_ASSERT_CONTINUE(connTarget != nullptr && connTarget[0] != '\0');

                outPatchbay << "  <Connection>\n";
                outPatchbay << "   <Source>" << xmlSafeString(connSource, true) << "</Source>\n";
                outPatchbay << "   <Target>" << xmlSafeString(connTarget, true) << "</Target>\n";
                outPatchbay << "  </Connection>\n";
            }

            outPatchbay << " </Patchbay>\n";
            outStream << outPatchbay;
        }
    }

    // if we're running inside some session-manager (and using JACK), let them handle the connections
    bool saveExternalConnections;

    /**/ if (isPlugin)
        saveExternalConnections = false;
    else if (std::strcmp(getCurrentDriverName(), "JACK") != 0)
        saveExternalConnections = true;
    else if (std::getenv("CARLA_DONT_MANAGE_CONNECTIONS") != nullptr)
        saveExternalConnections = false;
    else if (std::getenv("LADISH_APP_NAME") != nullptr)
        saveExternalConnections = false;
    else if (std::getenv("NSM_URL") != nullptr)
        saveExternalConnections = false;
    else
        saveExternalConnections = true;

    if (saveExternalConnections)
    {
        if (const char* const* const patchbayConns = getPatchbayConnections(true))
        {
            MemoryOutputStream outPatchbay(2048);

            outPatchbay << "\n <ExternalPatchbay>\n";

            for (int i=0; patchbayConns[i] != nullptr && patchbayConns[i+1] != nullptr; ++i, ++i )
            {
                const char* const connSource(patchbayConns[i]);
                const char* const connTarget(patchbayConns[i+1]);

                CARLA_SAFE_ASSERT_CONTINUE(connSource != nullptr && connSource[0] != '\0');
                CARLA_SAFE_ASSERT_CONTINUE(connTarget != nullptr && connTarget[0] != '\0');

                outPatchbay << "  <Connection>\n";
                outPatchbay << "   <Source>" << xmlSafeString(connSource, true) << "</Source>\n";
                outPatchbay << "   <Target>" << xmlSafeString(connTarget, true) << "</Target>\n";
                outPatchbay << "  </Connection>\n";
            }

            outPatchbay << " </ExternalPatchbay>\n";
            outStream << outPatchbay;
        }
    }
#endif

    outStream << "</CARLA-PROJECT>\n";
}

static String findBinaryInCustomPath(const char* const searchPath, const char* const binary)
{
    const StringArray searchPaths(StringArray::fromTokens(searchPath, CARLA_OS_SPLIT_STR, ""));

    // try direct filename first
    String jbinary(binary);

    // adjust for current platform
#ifdef CARLA_OS_WIN
    if (jbinary[0] == '/')
        jbinary = "C:" + jbinary.replaceCharacter('/', '\\');
#else
    if (jbinary[1] == ':' && (jbinary[2] == '\\' || jbinary[2] == '/'))
        jbinary = jbinary.substring(2).replaceCharacter('\\', '/');
#endif

    String filename = File(jbinary).getFileName();

    int searchFlags = File::findFiles|File::ignoreHiddenFiles;
#ifdef CARLA_OS_MAC
    if (filename.endsWithIgnoreCase(".vst"))
        searchFlags |= File::findDirectories;
#endif

    Array<File> results;
    for (const String *it=searchPaths.begin(), *end=searchPaths.end(); it != end; ++it)
    {
        const File path(*it);

        results.clear();
        path.findChildFiles(results, searchFlags, true, filename);

        if (results.size() > 0)
            return results.getFirst().getFullPathName();
    }

    // try changing extension
#if defined(CARLA_OS_MAC)
    if (filename.endsWithIgnoreCase(".dll") || filename.endsWithIgnoreCase(".so"))
        filename = File(jbinary).getFileNameWithoutExtension() + ".dylib";
#elif defined(CARLA_OS_WIN)
    if (filename.endsWithIgnoreCase(".dylib") || filename.endsWithIgnoreCase(".so"))
        filename = File(jbinary).getFileNameWithoutExtension() + ".dll";
#else
    if (filename.endsWithIgnoreCase(".dll") || filename.endsWithIgnoreCase(".dylib"))
        filename = File(jbinary).getFileNameWithoutExtension() + ".so";
#endif
    else
        return String();

    for (const String *it=searchPaths.begin(), *end=searchPaths.end(); it != end; ++it)
    {
        const File path(*it);

        results.clear();
        path.findChildFiles(results, searchFlags, true, filename);

        if (results.size() > 0)
            return results.getFirst().getFullPathName();
    }

    return String();
}

bool CarlaEngine::loadProjectInternal(water::XmlDocument& xmlDoc)
{
    ScopedPointer<XmlElement> xmlElement(xmlDoc.getDocumentElement(true));
    CARLA_SAFE_ASSERT_RETURN_ERR(xmlElement != nullptr, "Failed to parse project file");

    const String& xmlType(xmlElement->getTagName());
    const bool isPreset(xmlType.equalsIgnoreCase("carla-preset"));

    if (! (xmlType.equalsIgnoreCase("carla-project") || isPreset))
    {
        callback(ENGINE_CALLBACK_PROJECT_LOAD_FINISHED, 0, 0, 0, 0.0f, nullptr);
        setLastError("Not a valid Carla project or preset file");
        return false;
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    const ScopedValueSetter<bool> _svs(pData->loadingProject, true, false);
#endif

    // completely load file
    xmlElement = xmlDoc.getDocumentElement(false);
    CARLA_SAFE_ASSERT_RETURN_ERR(xmlElement != nullptr, "Failed to completely parse project file");

    if (pData->aboutToClose)
        return true;

    const bool isPlugin(getType() == kEngineTypePlugin);

    // load engine settings first of all
    if (XmlElement* const elem = isPreset ? nullptr : xmlElement->getChildByName("EngineSettings"))
    {
        for (XmlElement* settElem = elem->getFirstChildElement(); settElem != nullptr; settElem = settElem->getNextElement())
        {
            const String& tag(settElem->getTagName());
            const String  text(settElem->getAllSubText().trim());

            /** some settings might be incorrect or require extra work,
                so we call setOption rather than modifying them direly */

            int option = -1;
            int value  = 0;
            const char* valueStr = nullptr;

            /**/ if (tag == "ForceStereo")
            {
                option = ENGINE_OPTION_FORCE_STEREO;
                value  = text == "true" ? 1 : 0;
            }
            else if (tag == "PreferPluginBridges")
            {
                option = ENGINE_OPTION_PREFER_PLUGIN_BRIDGES;
                value  = text == "true" ? 1 : 0;
            }
            else if (tag == "PreferUiBridges")
            {
                option = ENGINE_OPTION_PREFER_UI_BRIDGES;
                value  = text == "true" ? 1 : 0;
            }
            else if (tag == "UIsAlwaysOnTop")
            {
                option = ENGINE_OPTION_UIS_ALWAYS_ON_TOP;
                value  = text == "true" ? 1 : 0;
            }
            else if (tag == "MaxParameters")
            {
                option = ENGINE_OPTION_MAX_PARAMETERS;
                value  = text.getIntValue();
            }
            else if (tag == "UIBridgesTimeout")
            {
                option = ENGINE_OPTION_UI_BRIDGES_TIMEOUT;
                value  = text.getIntValue();
            }
            else if (isPlugin)
            {
                /**/ if (tag == "LADSPA_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_LADSPA;
                    valueStr = text.toRawUTF8();
                }
                else if (tag == "DSSI_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_DSSI;
                    valueStr = text.toRawUTF8();
                }
                else if (tag == "LV2_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_LV2;
                    valueStr = text.toRawUTF8();
                }
                else if (tag == "VST2_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_VST2;
                    valueStr = text.toRawUTF8();
                }
                else if (tag == "SF2_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_SF2;
                    valueStr = text.toRawUTF8();
                }
                else if (tag == "SFZ_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_SFZ;
                    valueStr = text.toRawUTF8();
                }
            }

            if (option == -1)
            {
                // check old stuff, unhandled now
                if (tag == "AU_PATH" || tag == "VST3_PATH" || tag == "GIG_PATH")
                    continue;

                // hmm something is wrong..
                carla_stderr2("CarlaEngine::loadProjectInternal() - Unhandled option '%s'", tag.toRawUTF8());
                continue;
            }

            setOption(static_cast<EngineOption>(option), value, valueStr);
        }
    }

    if (pData->aboutToClose)
        return true;

    // now setup transport
    if (XmlElement* const elem = (isPreset || isPlugin) ? nullptr : xmlElement->getChildByName("Transport"))
    {
        if (XmlElement* const bpmElem = elem->getChildByName("BeatsPerMinute"))
        {
            const String bpmText(bpmElem->getAllSubText().trim());
            const double bpm = bpmText.getDoubleValue();

            // some sane limits
            if (bpm >= 20.0 && bpm < 400.0)
                pData->time.setBPM(bpm);
        }
    }

    if (pData->aboutToClose)
        return true;

    // and we handle plugins
    for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
    {
        const String& tagName(elem->getTagName());

        if (isPreset || tagName == "Plugin")
        {
            CarlaStateSave stateSave;
            stateSave.fillFromXmlElement(isPreset ? xmlElement.get() : elem);

            callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

            if (pData->aboutToClose)
                return true;

            CARLA_SAFE_ASSERT_CONTINUE(stateSave.type != nullptr);

#ifndef BUILD_BRIDGE
            // compatibility code to load projects with GIG files
            // FIXME Remove on 2.1 release
            if (std::strcmp(stateSave.type, "GIG") == 0)
            {
                if (addPlugin(PLUGIN_LV2, "", stateSave.name, "http://linuxsampler.org/plugins/linuxsampler", 0, nullptr))
                {
                    const uint pluginId = pData->curPluginCount;

                    if (CarlaPlugin* const plugin = pData->plugins[pluginId].plugin)
                    {
                        callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

                        if (pData->aboutToClose)
                            return true;

                        String lsState;
                        lsState << "0.35\n";
                        lsState << "18 0 Chromatic\n";
                        lsState << "18 1 Drum Kits\n";
                        lsState << "20 0\n";
                        lsState << "0 1 " << stateSave.binary << "\n";
                        lsState << "0 0 0 0 1 0 GIG\n";

                        plugin->setCustomData(LV2_ATOM__String, "http://linuxsampler.org/schema#state-string", lsState.toRawUTF8(), true);
                        plugin->restoreLV2State();

                        plugin->setDryWet(stateSave.dryWet, true, true);
                        plugin->setVolume(stateSave.volume, true, true);
                        plugin->setBalanceLeft(stateSave.balanceLeft, true, true);
                        plugin->setBalanceRight(stateSave.balanceRight, true, true);
                        plugin->setPanning(stateSave.panning, true, true);
                        plugin->setCtrlChannel(stateSave.ctrlChannel, true, true);
                        plugin->setActive(stateSave.active, true, true);

                        ++pData->curPluginCount;

                        plugin->setEnabled(true);
                        callback(ENGINE_CALLBACK_PLUGIN_ADDED, pluginId, 0, 0, 0.0f, plugin->getName());

                        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
                            pData->graph.addPlugin(plugin);
                    }
                    else
                    {
                        carla_stderr2("Failed to get new plugin, state will not be restored correctly\n");
                    }
                }
                else
                {
                    carla_stderr2("Failed to load a linuxsampler LV2 plugin, GIG file won't be loaded");
                }

                continue;
            }
#endif

            const void* extraStuff    = nullptr;
            static const char kTrue[] = "true";

            const PluginType ptype(getPluginTypeFromString(stateSave.type));

            switch (ptype)
            {
            case PLUGIN_SF2:
                if (CarlaString(stateSave.label).endsWith(" (16 outs)"))
                    extraStuff = kTrue;
                // fall through
            case PLUGIN_LADSPA:
            case PLUGIN_DSSI:
            case PLUGIN_VST2:
            case PLUGIN_SFZ:
                if (stateSave.binary != nullptr && stateSave.binary[0] != '\0' &&
                    ! (File::isAbsolutePath(stateSave.binary) && File(stateSave.binary).exists()))
                {
                    const char* searchPath;

                    switch (ptype)
                    {
                    case PLUGIN_LADSPA: searchPath = pData->options.pathLADSPA; break;
                    case PLUGIN_DSSI:   searchPath = pData->options.pathDSSI;   break;
                    case PLUGIN_VST2:   searchPath = pData->options.pathVST2;   break;
                    case PLUGIN_SF2:    searchPath = pData->options.pathSF2;    break;
                    case PLUGIN_SFZ:    searchPath = pData->options.pathSFZ;    break;
                    default:            searchPath = nullptr;                   break;
                    }

                    if (searchPath != nullptr && searchPath[0] != '\0')
                    {
                        carla_stderr("Plugin binary '%s' doesn't exist on this filesystem, let's look for it...",
                                     stateSave.binary);

                        String result = findBinaryInCustomPath(searchPath, stateSave.binary);

                        if (result.isEmpty())
                        {
                            switch (ptype)
                            {
                            case PLUGIN_LADSPA: searchPath = std::getenv("LADSPA_PATH"); break;
                            case PLUGIN_DSSI:   searchPath = std::getenv("DSSI_PATH");   break;
                            case PLUGIN_VST2:   searchPath = std::getenv("VST_PATH");    break;
                            case PLUGIN_SF2:    searchPath = std::getenv("SF2_PATH");    break;
                            case PLUGIN_SFZ:    searchPath = std::getenv("SFZ_PATH");    break;
                            default:            searchPath = nullptr;                    break;
                            }

                            if (searchPath != nullptr && searchPath[0] != '\0')
                                result = findBinaryInCustomPath(searchPath, stateSave.binary);
                        }

                        if (result.isNotEmpty())
                        {
                            delete[] stateSave.binary;
                            stateSave.binary = carla_strdup(result.toRawUTF8());
                            carla_stderr("Found it! :)");
                        }
                        else
                        {
                            carla_stderr("Damn, we failed... :(");
                        }
                    }
                }
                break;
            default:
                break;
            }

            BinaryType btype;

            switch (ptype)
            {
            case PLUGIN_LADSPA:
            case PLUGIN_DSSI:
            case PLUGIN_LV2:
            case PLUGIN_VST2:
                btype = getBinaryTypeFromFile(stateSave.binary);
                break;
            default:
                btype = BINARY_NATIVE;
                break;
            }

            if (addPlugin(btype, ptype, stateSave.binary,
                          stateSave.name, stateSave.label, stateSave.uniqueId, extraStuff, stateSave.options))
            {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                const uint pluginId = pData->curPluginCount;
#else
                const uint pluginId = 0;
#endif

                if (CarlaPlugin* const plugin = pData->plugins[pluginId].plugin)
                {
                    callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

                    if (pData->aboutToClose)
                        return true;

                    // deactivate bridge client-side ping check, since some plugins block during load
                    if ((plugin->getHints() & PLUGIN_IS_BRIDGE) != 0 && ! isPreset)
                        plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "__CarlaPingOnOff__", "false", false);

                    plugin->loadStateSave(stateSave);

                    /* NOTE: The following code is the same as the end of addPlugin().
                     *       When project is loading we do not enable the plugin right away,
                     *        as we want to load state first.
                     */
                    plugin->setEnabled(true);

                    ++pData->curPluginCount;
                    callback(ENGINE_CALLBACK_PLUGIN_ADDED, pluginId, 0, 0, 0.0f, plugin->getName());

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
                        pData->graph.addPlugin(plugin);
#endif
                }
                else
                {
                    carla_stderr2("Failed to get new plugin, state will not be restored correctly\n");
                }
            }
            else
            {
                carla_stderr2("Failed to load a plugin, error was:\n%s", getLastError());
            }
        }

        if (isPreset)
        {
            callback(ENGINE_CALLBACK_PROJECT_LOAD_FINISHED, 0, 0, 0, 0.0f, nullptr);
            return true;
        }
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // tell bridges we're done loading
    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        CarlaPlugin* const plugin(pData->plugins[i].plugin);

        if (plugin != nullptr && plugin->isEnabled() && (plugin->getHints() & PLUGIN_IS_BRIDGE) != 0)
            plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "__CarlaPingOnOff__", "true", false);
    }

    callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

    if (pData->aboutToClose)
        return true;

    bool hasInternalConnections = false;

    // and now we handle connections (internal)
    if (XmlElement* const elem = xmlElement->getChildByName("Patchbay"))
    {
        hasInternalConnections = true;

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        {
            CarlaString sourcePort, targetPort;
            const bool isUsingExternal(pData->graph.isUsingExternal());

            for (XmlElement* patchElem = elem->getFirstChildElement(); patchElem != nullptr; patchElem = patchElem->getNextElement())
            {
                const String& patchTag(patchElem->getTagName());

                if (patchTag != "Connection")
                    continue;

                sourcePort.clear();
                targetPort.clear();

                for (XmlElement* connElem = patchElem->getFirstChildElement(); connElem != nullptr; connElem = connElem->getNextElement())
                {
                    const String& tag(connElem->getTagName());
                    const String  text(connElem->getAllSubText().trim());

                    /**/ if (tag == "Source")
                        sourcePort = xmlSafeString(text, false).toRawUTF8();
                    else if (tag == "Target")
                        targetPort = xmlSafeString(text, false).toRawUTF8();
                }

                if (sourcePort.isNotEmpty() && targetPort.isNotEmpty())
                    restorePatchbayConnection(false, sourcePort, targetPort, !isUsingExternal);
            }

            callback(ENGINE_CALLBACK_IDLE, 0, 0, 0, 0.0f, nullptr);

            if (pData->aboutToClose)
                return true;
        }
    }

    // if we're running inside some session-manager (and using JACK), let them handle the external connections
    bool loadExternalConnections;

    /**/ if (std::strcmp(getCurrentDriverName(), "JACK") != 0)
        loadExternalConnections = true;
    else if (std::getenv("CARLA_DONT_MANAGE_CONNECTIONS") != nullptr)
        loadExternalConnections = false;
    else if (std::getenv("LADISH_APP_NAME") != nullptr)
        loadExternalConnections = false;
    else if (std::getenv("NSM_URL") != nullptr)
        loadExternalConnections = false;
    else
        loadExternalConnections = true;

    // plus external connections too
    if (loadExternalConnections)
    {
        const bool isUsingExternal = pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY ||
                                     pData->graph.isUsingExternal();
        const bool loadingAsExternal = pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY &&
                                       hasInternalConnections;

        for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
        {
            const String& tagName(elem->getTagName());

            // check if we want to load patchbay-mode connections into an external (multi-client) graph
            if (tagName == "Patchbay")
            {
                if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
                    continue;
            }
            // or load external patchbay connections
            else if (tagName != "ExternalPatchbay")
            {
                continue;
            }

            CarlaString sourcePort, targetPort;

            for (XmlElement* patchElem = elem->getFirstChildElement(); patchElem != nullptr; patchElem = patchElem->getNextElement())
            {
                const String& patchTag(patchElem->getTagName());

                if (patchTag != "Connection")
                    continue;

                sourcePort.clear();
                targetPort.clear();

                for (XmlElement* connElem = patchElem->getFirstChildElement(); connElem != nullptr; connElem = connElem->getNextElement())
                {
                    const String& tag(connElem->getTagName());
                    const String  text(connElem->getAllSubText().trim());

                    /**/ if (tag == "Source")
                        sourcePort = xmlSafeString(text, false).toRawUTF8();
                    else if (tag == "Target")
                        targetPort = xmlSafeString(text, false).toRawUTF8();
                }

                if (sourcePort.isNotEmpty() && targetPort.isNotEmpty())
                    restorePatchbayConnection(loadingAsExternal, sourcePort, targetPort, isUsingExternal);
            }
            break;
        }
    }

#endif

    callback(ENGINE_CALLBACK_PROJECT_LOAD_FINISHED, 0, 0, 0, 0.0f, nullptr);
    return true;
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
