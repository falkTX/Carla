// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

/* TODO:
 * - complete processRack(): carefully add to input, sorted events
 * - implement processPatchbay()
 * - implement oscSend_control_switch_plugins()
 * - something about the peaks?
 */

#include "CarlaEngineClient.hpp"
#include "CarlaEngineInit.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBinaryUtils.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPipeUtils.hpp"
#include "CarlaProcessUtils.hpp"
#include "CarlaScopeUtils.hpp"
#include "CarlaStateUtils.hpp"
#include "CarlaMIDI.h"

#include "jackbridge/JackBridge.hpp"

#include "water/files/File.h"
#include "water/streams/MemoryOutputStream.h"
#include "water/xml/XmlDocument.h"
#include "water/xml/XmlElement.h"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# if defined(CARLA_OS_64BIT) && defined(HAVE_LIBMAGIC) && ! defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
#  define ADAPT_FOR_APPLE_SILLICON
# endif
#endif

#include <map>

// FIXME Remove on 2.1 release
#include "lv2/atom.h"

using water::Array;
using water::CharPointer_UTF8;
using water::File;
using water::MemoryOutputStream;
using water::StringArray;
using water::XmlDocument;
using water::XmlElement;

// #define SFZ_FILES_USING_SFIZZ

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
    using namespace EngineInit;

    uint count = 0;

#ifdef HAVE_JACK
    if (jackbridge_is_ok())
        ++count;
#endif

#ifdef USING_RTAUDIO
    count += getRtAudioApiCount();
#endif

#ifdef HAVE_SDL
    ++count;
#endif

    return count;
}

const char* CarlaEngine::getDriverName(const uint index)
{
    carla_debug("CarlaEngine::getDriverName(%u)", index);
    using namespace EngineInit;

    uint index2 = index;

#ifdef HAVE_JACK
    if (jackbridge_is_ok())
    {
        if (index2 == 0)
            return "JACK";
        --index2;
    }
#endif

#ifdef USING_RTAUDIO
    if (const uint count = getRtAudioApiCount())
    {
        if (index2 < count)
            return getRtAudioApiName(index2);
        index2 -= count;
    }
#endif

#ifdef HAVE_SDL
    if (index2 == 0)
        return "SDL";
    --index2;
#endif

    carla_stderr("CarlaEngine::getDriverName(%u) - invalid index %u", index, index2);
    return nullptr;
}

const char* const* CarlaEngine::getDriverDeviceNames(const uint index)
{
    carla_debug("CarlaEngine::getDriverDeviceNames(%u)", index);
    using namespace EngineInit;

    uint index2 = index;

#ifdef HAVE_JACK
    if (jackbridge_is_ok())
    {
        if (index2 == 0)
        {
            static const char* ret[3] = { "Auto-Connect ON", "Auto-Connect OFF", nullptr };
            return ret;
        }
        --index2;
    }
#endif

#ifdef USING_RTAUDIO
    if (const uint count = getRtAudioApiCount())
    {
        if (index2 < count)
            return getRtAudioApiDeviceNames(index2);
        index2 -= count;
    }
#endif

#ifdef HAVE_SDL
    if (index2 == 0)
        return getSDLDeviceNames();
    --index2;
#endif

    carla_stderr("CarlaEngine::getDriverDeviceNames(%u) - invalid index %u", index, index2);
    return nullptr;
}

const EngineDriverDeviceInfo* CarlaEngine::getDriverDeviceInfo(const uint index, const char* const deviceName)
{
    carla_debug("CarlaEngine::getDriverDeviceInfo(%u, \"%s\")", index, deviceName);
    using namespace EngineInit;

    uint index2 = index;

#ifdef HAVE_JACK
    if (jackbridge_is_ok())
    {
        if (index2 == 0)
        {
            static EngineDriverDeviceInfo devInfo;
            devInfo.hints       = ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE;
            devInfo.bufferSizes = nullptr;
            devInfo.sampleRates = nullptr;
            return &devInfo;
        }
        --index2;
    }
#endif

#ifdef USING_RTAUDIO
    if (const uint count = getRtAudioApiCount())
    {
        if (index2 < count)
            return getRtAudioDeviceInfo(index2, deviceName);
        index2 -= count;
    }
#endif

#ifdef HAVE_SDL
    if (index2 == 0)
    {
        static uint32_t sdlBufferSizes[] = { 512, 1024, 2048, 4096, 8192, 0 };
        static double   sdlSampleRates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 0.0 };
        static EngineDriverDeviceInfo devInfo;
        devInfo.hints       = 0x0;
        devInfo.bufferSizes = sdlBufferSizes;
        devInfo.sampleRates = sdlSampleRates;
        return &devInfo;
    }
    --index2;
#endif

    carla_stderr("CarlaEngine::getDriverDeviceInfo(%u, \"%s\") - invalid index %u", index, deviceName, index2);
    return nullptr;
}

bool CarlaEngine::showDriverDeviceControlPanel(const uint index, const char* const deviceName)
{
    carla_debug("CarlaEngine::showDriverDeviceControlPanel(%u, \"%s\")", index, deviceName);
    using namespace EngineInit;

    uint index2 = index;

#ifdef HAVE_JACK
    if (jackbridge_is_ok())
    {
        if (index2 == 0)
            return false;
        --index2;
    }
#endif

#ifdef USING_RTAUDIO
    if (const uint count = getRtAudioApiCount())
    {
        if (index2 < count)
            return false;
        index2 -= count;
    }
#endif

#ifdef HAVE_SDL
    if (index2 == 0)
        return false;
    --index2;
#endif

    carla_stderr("CarlaEngine::showDriverDeviceControlPanel(%u, \"%s\") - invalid index %u", index, deviceName, index2);
    return false;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', nullptr);
    carla_debug("CarlaEngine::newDriverByName(\"%s\")", driverName);
    using namespace EngineInit;

#ifdef HAVE_JACK
    if (std::strcmp(driverName, "JACK") == 0)
        return newJack();
#endif

#if !(defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) || defined(CARLA_OS_WASM) || defined(CARLA_PLUGIN_BUILD) || defined(STATIC_PLUGIN_TARGET))
    if (std::strcmp(driverName, "Dummy") == 0)
        return newDummy();
#endif

#ifdef USING_RTAUDIO
    // -------------------------------------------------------------------
    // common

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

#ifdef HAVE_SDL
    if (std::strcmp(driverName, "SDL") == 0)
        return newSDL();
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

    pData->close();

    callback(true, true, ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0, 0.0f, nullptr);
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

    const bool engineNotRunning = !isRunning();
    const bool engineHasIdleOnMainThread = hasIdleOnMainThread();

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
        {
            if (plugin->isEnabled())
            {
                const uint hints = plugin->getHints();

                if (engineNotRunning)
                {
                    try {
                        plugin->idle();
                    } CARLA_SAFE_EXCEPTION_CONTINUE("Plugin idle");

                    if (hints & PLUGIN_HAS_CUSTOM_UI)
                    {
                        try {
                            plugin->uiIdle();
                        } CARLA_SAFE_EXCEPTION_CONTINUE("Plugin uiIdle");
                    }
                }
                else
                {
                    if (engineHasIdleOnMainThread && (hints & PLUGIN_NEEDS_MAIN_THREAD_IDLE) != 0)
                    {
                        try {
                            plugin->idle();
                        } CARLA_SAFE_EXCEPTION_CONTINUE("Plugin idle");
                    }

                    if ((hints & PLUGIN_HAS_CUSTOM_UI) != 0 && (hints & PLUGIN_NEEDS_UI_MAIN_THREAD) != 0)
                    {
                        try {
                            plugin->uiIdle();
                        } CARLA_SAFE_EXCEPTION_CONTINUE("Plugin uiIdle");
                    }
                }
            }
        }
    }

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    pData->osc.idle();
#endif

    pData->deletePluginsAsNeeded();
}

CarlaEngineClient* CarlaEngine::addClient(CarlaPluginPtr plugin)
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    return new CarlaEngineClientForStandalone(*this, pData->graph, plugin);
#else
    return new CarlaEngineClientForBridge(*this);

    // unused
    (void)plugin;
#endif
}

float CarlaEngine::getDSPLoad() const noexcept
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    return pData->dspLoad;
#else
    return 0.0f;
#endif
}

uint32_t CarlaEngine::getTotalXruns() const noexcept
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    return pData->xruns;
#else
    return 0;
#endif
}

void CarlaEngine::clearXruns() const noexcept
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    pData->xruns = 0;
#endif
}

bool CarlaEngine::showDeviceControlPanel() const noexcept
{
    return false;
}

bool CarlaEngine::setBufferSizeAndSampleRate(const uint, const double)
{
    return false;
}

// -----------------------------------------------------------------------
// Plugin management

bool CarlaEngine::addPlugin(const BinaryType btype,
                            const PluginType ptype,
                            const char* const filename,
                            const char* const name,
                            const char* const label,
                            const int64_t uniqueId,
                            const void* const extra,
                            const uint options)
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
    carla_debug("CarlaEngine::addPlugin(%i:%s, %i:%s, \"%s\", \"%s\", \"%s\", " P_INT64 ", %p, %u)",
                btype, BinaryType2Str(btype), ptype, PluginType2Str(ptype), filename, name, label, uniqueId, extra, options);

#ifndef CARLA_OS_WIN
    if (ptype != PLUGIN_JACK && ptype != PLUGIN_LV2 && filename != nullptr && filename[0] != '\0') {
        CARLA_SAFE_ASSERT_RETURN_ERR(filename[0] == CARLA_OS_SEP || filename[0] == '.' || filename[0] == '~', "Invalid plugin filename");
    }
#endif

    uint id;

   #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CarlaPluginPtr oldPlugin;

    if (pData->nextPluginId < pData->curPluginCount)
    {
        id = pData->nextPluginId;
        pData->nextPluginId = pData->maxPluginNumber;

        oldPlugin = pData->plugins[id].plugin;

        CARLA_SAFE_ASSERT_RETURN_ERR(oldPlugin.get() != nullptr, "Invalid replace plugin Id");
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
        CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins[id].plugin.get() == nullptr, "Invalid engine internal data");
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

    CarlaPluginPtr plugin;
    String bridgeBinary(pData->options.binaryDir);

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
               #if defined(CARLA_OS_WIN) && !defined(CARLA_OS_64BIT)
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-native.exe";
               #else
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-win32.exe";
               #endif
                break;
            case BINARY_WIN64:
               #if defined(CARLA_OS_WIN) && defined(CARLA_OS_64BIT)
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-native.exe";
               #else
                bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-win64.exe";
               #endif
                break;
            default:
                bridgeBinary.clear();
                break;
            }
        }

        if (! File(bridgeBinary.buffer()).existsAsFile())
            bridgeBinary.clear();
    }

    const bool canBeBridged = ptype != PLUGIN_INTERNAL
                           && ptype != PLUGIN_DLS
                           && ptype != PLUGIN_GIG
                           && ptype != PLUGIN_SF2
                           && ptype != PLUGIN_SFZ
                           && ptype != PLUGIN_JSFX
                           && ptype != PLUGIN_JACK;

    // Prefer bridges for some specific plugins
    bool preferBridges = pData->options.preferPluginBridges;
    const char* needsArchBridge = nullptr;

   #ifdef CARLA_OS_MAC
    // Plugin might be in quarentine due to Apple stupid notarization rules, let's remove that if possible
    if (canBeBridged && ptype != PLUGIN_LV2 && ptype != PLUGIN_AU)
        removeFileFromQuarantine(filename);
   #endif

   #ifndef BUILD_BRIDGE
    if (canBeBridged && ! preferBridges)
    {
        /*
        if (ptype == PLUGIN_LV2 && label != nullptr)
        {
            if (std::strncmp(label, "http://calf.sourceforge.net/plugins/", 36) == 0 ||
                std::strcmp(label, "http://factorial.hu/plugins/lv2/ir") == 0 ||
                std::strstr(label, "v1.sourceforge.net/lv2") != nullptr)
            {
                preferBridges = true;
            }
        }
        */
       #ifdef ADAPT_FOR_APPLE_SILLICON
        // see if this binary needs bridging
        if (ptype == PLUGIN_VST2 || ptype == PLUGIN_VST3)
        {
            if (const char* const vst2Binary = findBinaryInBundle(filename))
            {
                const CarlaMagic magic;
                if (const char* const output = magic.getFileDescription(vst2Binary))
                {
                    carla_stdout("VST binary magic output is '%s'", output);
                   #ifdef __aarch64__
                    if (std::strstr(output, "arm64") == nullptr && std::strstr(output, "x86_64") != nullptr)
                        needsArchBridge = "x86_64";
                   #else
                    if (std::strstr(output, "x86_64") == nullptr && std::strstr(output, "arm64") != nullptr)
                        needsArchBridge = "arm64";
                   #endif
                }
                else
                {
                    carla_stdout("VST binary magic output is null");
                }
            }
            else
            {
                carla_stdout("Search for binary in VST bundle failed");
            }
        }
       #endif // ADAPT_FOR_APPLE_SILLICON
    }
   #endif // ! BUILD_BRIDGE

   #if defined(CARLA_PLUGIN_ONLY_BRIDGE)
    if (bridgeBinary.isNotEmpty())
    {
        plugin = CarlaPlugin::newBridge(initializer, btype, ptype, needsArchBridge, bridgeBinary);
    }
    else
    {
        setLastError("Cannot load plugin, the required plugin bridge is not available");
        return false;
    }
   #elif !defined(CARLA_OS_WASM)
    if (canBeBridged && (needsArchBridge || btype != BINARY_NATIVE || (preferBridges && bridgeBinary.isNotEmpty())))
    {
        if (bridgeBinary.isNotEmpty())
        {
            plugin = CarlaPlugin::newBridge(initializer, btype, ptype, needsArchBridge, bridgeBinary);
        }
        else
        {
            setLastError("This Carla build cannot handle this binary");
            return false;
        }
    }
    else
   #endif
   #ifndef CARLA_PLUGIN_ONLY_BRIDGE
    {
       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        bool use16Outs;
       #endif
        setLastError("Invalid or unsupported plugin type");

        // Some stupid plugins mess up with global signals, err!!
        const CarlaSignalRestorer csr;

        switch (ptype)
        {
        case PLUGIN_NONE:
        case PLUGIN_TYPE_COUNT:
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

        case PLUGIN_VST3:
            plugin = CarlaPlugin::newVST3(initializer);
            break;

        case PLUGIN_CLAP:
            plugin = CarlaPlugin::newCLAP(initializer);
            break;

        case PLUGIN_AU:
            plugin = CarlaPlugin::newAU(initializer);
            break;

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        case PLUGIN_INTERNAL:
            plugin = CarlaPlugin::newNative(initializer);
            break;

        case PLUGIN_DLS:
        case PLUGIN_GIG:
        case PLUGIN_SF2:
            use16Outs = (extra != nullptr && std::strcmp((const char*)extra, "true") == 0);
            plugin = CarlaPlugin::newFluidSynth(initializer, ptype, use16Outs);
            break;

        case PLUGIN_SFZ:
           #ifdef SFZ_FILES_USING_SFIZZ
            {
                CarlaPlugin::Initializer sfizzInitializer = {
                    this,
                    id,
                    name,
                    "",
                    "http://sfztools.github.io/sfizz",
                    0,
                    options
                };

                plugin = CarlaPlugin::newLV2(sfizzInitializer);
            }
           #else
            plugin = CarlaPlugin::newSFZero(initializer);
           #endif
            break;

        case PLUGIN_JSFX:
            plugin = CarlaPlugin::newJSFX(initializer);
            break;

        case PLUGIN_JACK:
           #ifdef HAVE_JACK
            plugin = CarlaPlugin::newJackApp(initializer);
           #else
            setLastError("JACK plugin target is not available");
           #endif
            break;
       #else
        case PLUGIN_INTERNAL:
        case PLUGIN_DLS:
        case PLUGIN_GIG:
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
        case PLUGIN_JACK:
        case PLUGIN_JSFX:
            setLastError("Plugin bridges cannot handle this binary");
            break;
       #endif // BUILD_BRIDGE_ALTERNATIVE_ARCH
        }
    }
   #endif // CARLA_PLUGIN_ONLY_BRIDGE

    if (plugin.get() == nullptr)
        return false;

    plugin->reload();

   #ifdef SFZ_FILES_USING_SFIZZ
    if (ptype == PLUGIN_SFZ && plugin->getType() == PLUGIN_LV2)
    {
        plugin->setCustomData(LV2_ATOM__Path,
                              "http://sfztools.github.io/sfizz:sfzfile",
                              filename,
                              false);

        plugin->restoreLV2State(true);
    }
   #endif

    bool canRun = true;

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        /**/ if (plugin->getMidiInCount() > 1 || plugin->getMidiOutCount() > 1)
        {
            setLastError("Carla's patchbay mode cannot work with plugins that have multiple MIDI ports, sorry!");
            canRun = false;
        }
    }

    if (! canRun)
    {
        return false;
    }

    EnginePluginData& pluginData(pData->plugins[id]);
    pluginData.plugin = plugin;
    carla_zeroFloats(pluginData.peaks, 4);

   #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (oldPlugin.get() != nullptr)
    {
        CARLA_SAFE_ASSERT(! pData->loadingProject);

        const ScopedRunnerStopper srs(this);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
            pData->graph.replacePlugin(oldPlugin, plugin);

        const bool  wasActive = oldPlugin->getInternalParameterValue(PARAMETER_ACTIVE) >= 0.5f;
        const float oldDryWet = oldPlugin->getInternalParameterValue(PARAMETER_DRYWET);
        const float oldVolume = oldPlugin->getInternalParameterValue(PARAMETER_VOLUME);

        oldPlugin->prepareForDeletion();
        {
            const CarlaMutexLocker cml(pData->pluginsToDeleteMutex);
            pData->pluginsToDelete.push_back(oldPlugin);
        }

        if (plugin->getHints() & PLUGIN_CAN_DRYWET)
            plugin->setDryWet(oldDryWet, true, true);

        if (plugin->getHints() & PLUGIN_CAN_VOLUME)
            plugin->setVolume(oldVolume, true, true);

        plugin->setActive(wasActive, true, true);
        plugin->setEnabled(true);

        callback(true, true, ENGINE_CALLBACK_RELOAD_ALL, id, 0, 0, 0, 0.0f, nullptr);
    }
    else if (! pData->loadingProject)
   #endif
    {
        plugin->setEnabled(true);

        ++pData->curPluginCount;
        callback(true, true, ENGINE_CALLBACK_PLUGIN_ADDED, id, plugin->getType(), 0, 0, 0.0f, plugin->getName());

        if (getType() != kEngineTypeBridge)
            plugin->setActive(true, true, true);

       #ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
            pData->graph.addPlugin(plugin);
       #endif
    }

    return true;

   #if defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) || defined(CARLA_PLUGIN_ONLY_BRIDGE)
    // unused
    (void)extra;
   #endif
}

bool CarlaEngine::addPlugin(const PluginType ptype,
                            const char* const filename,
                            const char* const name,
                            const char* const label,
                            const int64_t uniqueId,
                            const void* const extra)
{
    return addPlugin(BINARY_NATIVE, ptype, filename, name, label, uniqueId, extra, PLUGIN_OPTIONS_NULL);
}

bool CarlaEngine::removePlugin(const uint id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data");
#else
    CARLA_SAFE_ASSERT_RETURN_ERR(id == 0, "Invalid engine internal data");
#endif
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id");
    carla_debug("CarlaEngine::removePlugin(%i)", id);

    const CarlaPluginPtr plugin = pData->plugins[id].plugin;

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin.get() != nullptr, "Could not find plugin to remove");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

    const ScopedRunnerStopper srs(this);

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
#else
    pData->curPluginCount = 0;
    pData->plugins[0].plugin.reset();
    carla_zeroStruct(pData->plugins[0].peaks);
#endif

    plugin->prepareForDeletion();
    {
        const CarlaMutexLocker cml(pData->pluginsToDeleteMutex);
        pData->pluginsToDelete.push_back(plugin);
    }

    callback(true, true, ENGINE_CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0, 0.0f, nullptr);
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

    const ScopedRunnerStopper srs(this);

    const uint curPluginCount = pData->curPluginCount;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        pData->graph.removeAllPlugins(pData->aboutToClose);
#endif

    const ScopedActionLock sal(this, kEnginePostActionZeroCount, 0, 0);

    callback(true, false, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);

    for (uint i=0; i < curPluginCount; ++i)
    {
        const uint id = curPluginCount - i - 1;
        EnginePluginData& pluginData(pData->plugins[id]);

        pluginData.plugin->prepareForDeletion();
        {
            const CarlaMutexLocker cml(pData->pluginsToDeleteMutex);
            pData->pluginsToDelete.push_back(pluginData.plugin);
        }

        pluginData.plugin.reset();
        carla_zeroStruct(pluginData.peaks);

        callback(true, true, ENGINE_CALLBACK_PLUGIN_REMOVED, id, 0, 0, 0, 0.0f, nullptr);
        callback(true, false, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
    }

    return true;
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
bool CarlaEngine::renamePlugin(const uint id, const char* const newName)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id");
    CARLA_SAFE_ASSERT_RETURN_ERR(newName != nullptr && newName[0] != '\0', "Invalid plugin name");
    carla_debug("CarlaEngine::renamePlugin(%i, \"%s\")", id, newName);

    const CarlaPluginPtr plugin = pData->plugins[id].plugin;
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin.get() != nullptr, "Could not find plugin to rename");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

    const char* const uniqueName(getUniquePluginName(newName));
    CARLA_SAFE_ASSERT_RETURN_ERR(uniqueName != nullptr, "Unable to get new unique plugin name");

    plugin->setName(uniqueName);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        pData->graph.renamePlugin(plugin, uniqueName);

    callback(true, true, ENGINE_CALLBACK_PLUGIN_RENAMED, id, 0, 0, 0, 0.0f, uniqueName);

    delete[] uniqueName;
    return true;
}

bool CarlaEngine::clonePlugin(const uint id)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->curPluginCount != 0, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(id < pData->curPluginCount, "Invalid plugin Id");
    carla_debug("CarlaEngine::clonePlugin(%i)", id);

    const CarlaPluginPtr plugin = pData->plugins[id].plugin;

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin.get() != nullptr, "Could not find plugin to clone");
    CARLA_SAFE_ASSERT_RETURN_ERR(plugin->getId() == id, "Invalid engine internal data");

    char label[STR_MAX+1];
    carla_zeroChars(label, STR_MAX+1);

    if (! plugin->getLabel(label))
        label[0] = '\0';

    const uint pluginCountBefore(pData->curPluginCount);

    if (! addPlugin(plugin->getBinaryType(), plugin->getType(),
                    plugin->getFilename(), plugin->getName(), label, plugin->getUniqueId(),
                    plugin->getExtraStuff(), plugin->getOptionsEnabled()))
        return false;

    CARLA_SAFE_ASSERT_RETURN_ERR(pluginCountBefore+1 == pData->curPluginCount, "No new plugin found");

    if (const CarlaPluginPtr newPlugin = pData->plugins[pluginCountBefore].plugin)
    {
        if (newPlugin->getType() == PLUGIN_LV2)
            newPlugin->cloneLV2Files(*plugin);
        newPlugin->loadStateSave(plugin->getStateSave(true));
    }

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

    const CarlaPluginPtr plugin = pData->plugins[id].plugin;

    CARLA_SAFE_ASSERT_RETURN_ERR(plugin.get() != nullptr, "Could not find plugin to replace");
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

    const CarlaPluginPtr pluginA = pData->plugins[idA].plugin;
    const CarlaPluginPtr pluginB = pData->plugins[idB].plugin;

    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA.get() != nullptr, "Could not find plugin to switch");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginB.get() != nullptr, "Could not find plugin to switch");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginA->getId() == idA, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERR(pluginB->getId() == idB, "Invalid engine internal data");

    const ScopedRunnerStopper srs(this);

    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        pData->graph.switchPlugins(pluginA, pluginB);

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

void CarlaEngine::touchPluginParameter(const uint, const uint32_t, const bool) noexcept
{
}

CarlaPluginPtr CarlaEngine::getPlugin(const uint id) const noexcept
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->plugins != nullptr, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->curPluginCount != 0, "Invalid engine internal data");
#endif
    CARLA_SAFE_ASSERT_RETURN_ERRN(pData->nextAction.opcode == kEnginePostActionNull, "Invalid engine internal data");
    CARLA_SAFE_ASSERT_RETURN_ERRN(id < pData->curPluginCount, "Invalid plugin Id");

    return pData->plugins[id].plugin;
}

CarlaPluginPtr CarlaEngine::getPluginUnchecked(const uint id) const noexcept
{
    return pData->plugins[id].plugin;
}

const char* CarlaEngine::getUniquePluginName(const char* const name) const
{
    CARLA_SAFE_ASSERT_RETURN(pData->nextAction.opcode == kEnginePostActionNull, nullptr);
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);
    carla_debug("CarlaEngine::getUniquePluginName(\"%s\")", name);

    String sname;
    sname = name;

    if (sname.isEmpty())
    {
        sname = "(No name)";
        return carla_strdup(sname);
    }

    const std::size_t maxNameSize(carla_minConstrained<uint>(getMaxClientNameSize(), 0xff, 6U) - 6); // 6 = strlen(" (10)") + 1

    if (maxNameSize == 0 || ! isRunning())
        return carla_strdup(sname);

    sname.truncate(maxNameSize);
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names
    sname.replace('/', '.'); // '/' is used by us for client name prefix

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        const CarlaPluginPtr plugin = pData->plugins[i].plugin;
        CARLA_SAFE_ASSERT_BREAK(plugin.use_count() > 0);

        // Check if unique name doesn't exist
        if (const char* const pluginName = plugin->getName())
        {
            if (sname != pluginName)
                continue;
        }

        // Check if string has already been modified
        {
            const std::size_t len(sname.length());

            // 1 digit, ex: " (2)"
            if (len > 4 && sname[len-4] == ' ' && sname[len-3] == '(' && sname.isDigit(len-2) && sname[len-1] == ')')
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
            if (len > 5 && sname[len-5] == ' ' && sname[len-4] == '(' && sname.isDigit(len-3) && sname.isDigit(len-2) && sname[len-1] == ')')
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

    return carla_strdup(sname);
}

// -----------------------------------------------------------------------
// Project management

bool CarlaEngine::loadFile(const char* const filename)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename");
    carla_debug("CarlaEngine::loadFile(\"%s\")", filename);

    File file(filename);
    CARLA_SAFE_ASSERT_RETURN_ERR(file.exists(), "Requested file does not exist or is not a readable");

    String baseName(file.getFileNameWithoutExtension().toRawUTF8());
    String extension(file.getFileExtension().replace(".","").toLowerCase().toRawUTF8());

    const uint curPluginId(pData->nextPluginId < pData->curPluginCount ? pData->nextPluginId : pData->curPluginCount);

    // -------------------------------------------------------------------
    // NOTE: please keep in sync with carla_get_supported_file_extensions!!

    if (extension == "carxp" || extension == "carxs")
        return loadProject(filename, false);

    // -------------------------------------------------------------------

    if (extension == "dls")
        return addPlugin(PLUGIN_DLS, filename, baseName, baseName, 0, nullptr);

    if (extension == "gig")
        return addPlugin(PLUGIN_GIG, filename, baseName, baseName, 0, nullptr);

    if (extension == "sf2" || extension == "sf3")
        return addPlugin(PLUGIN_SF2, filename, baseName, baseName, 0, nullptr);

    if (extension == "sfz")
        return addPlugin(PLUGIN_SFZ, filename, baseName, baseName, 0, nullptr);

    if (extension == "jsfx")
        return addPlugin(PLUGIN_JSFX, filename, baseName, baseName, 0, nullptr);

    // -------------------------------------------------------------------

    if (
        extension == "mp3"  ||
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
        extension == "opus" ||
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
        extension == "mpc" ||
        extension == "wma" ||
# ifndef HAVE_SNDFILE
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
            if (const CarlaPluginPtr plugin = getPlugin(curPluginId))
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
            if (const CarlaPluginPtr plugin = getPlugin(curPluginId))
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
        String nicerName("Zyn - ");

        const std::size_t sep(baseName.find('-')+1);

        if (sep < baseName.length())
            nicerName += baseName.buffer()+sep;
        else
            nicerName += baseName;

        if (addPlugin(PLUGIN_INTERNAL, nullptr, nicerName, "zynaddsubfx", 0, nullptr))
        {
            callback(true, true, ENGINE_CALLBACK_UI_STATE_CHANGED, curPluginId, 0, 0, 0, 0.0f, nullptr);

            if (const CarlaPluginPtr plugin = getPlugin(curPluginId))
            {
                const char* const ext = (extension == "xmz") ? "CarlaAlternateFile1" : "CarlaAlternateFile2";
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, ext, filename, true);
            }

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
    if (extension == "component")
        return addPlugin(PLUGIN_AU, filename, nullptr, nullptr, 0, nullptr);

    if (extension == "vst")
        return addPlugin(PLUGIN_VST2, filename, nullptr, nullptr, 0, nullptr);
#else
    if (extension == "dll" || extension == "so")
        return addPlugin(getBinaryTypeFromFile(filename), PLUGIN_VST2, filename, nullptr, nullptr, 0, nullptr);
#endif

    if (extension == "vst3")
        return addPlugin(getBinaryTypeFromFile(filename), PLUGIN_VST3, filename, nullptr, nullptr, 0, nullptr);

    if (extension == "clap")
        return addPlugin(getBinaryTypeFromFile(filename), PLUGIN_CLAP, filename, nullptr, nullptr, 0, nullptr);

    // -------------------------------------------------------------------

    setLastError("Unknown file extension");
    return false;
}

bool CarlaEngine::loadProject(const char* const filename, const bool setAsCurrentProject)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(pData->isIdling == 0, "An operation is still being processed, please wait for it to finish");
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename");
    carla_debug("CarlaEngine::loadProject(\"%s\")", filename);

    const File file(filename);
    CARLA_SAFE_ASSERT_RETURN_ERR(file.existsAsFile(), "Requested file does not exist or is not a readable file");

    if (setAsCurrentProject)
    {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (pData->currentProjectFilename != filename)
        {
            pData->currentProjectFilename = filename;

            bool found;
            const size_t r = pData->currentProjectFilename.rfind(CARLA_OS_SEP, &found);

            if (found)
            {
                pData->currentProjectFolder = filename;
                pData->currentProjectFolder[r] = '\0';
            }
            else
            {
                pData->currentProjectFolder.clear();
            }
        }
#endif
    }

    XmlDocument xml(file);
    return loadProjectInternal(xml, !setAsCurrentProject);
}

bool CarlaEngine::saveProject(const char* const filename, const bool setAsCurrentProject)
{
    CARLA_SAFE_ASSERT_RETURN_ERR(filename != nullptr && filename[0] != '\0', "Invalid filename");
    carla_debug("CarlaEngine::saveProject(\"%s\")", filename);

    if (setAsCurrentProject)
    {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        if (pData->currentProjectFilename != filename)
        {
            pData->currentProjectFilename = filename;

            bool found;
            const size_t r = pData->currentProjectFilename.rfind(CARLA_OS_SEP, &found);

            if (found)
            {
                pData->currentProjectFolder = filename;
                pData->currentProjectFolder[r] = '\0';
            }
            else
            {
                pData->currentProjectFolder.clear();
            }
        }
#endif
    }

    MemoryOutputStream out;
    saveProjectInternal(out);

    File file(filename);

    if (file.replaceWithData(out.getData(), out.getDataSize()))
        return true;

    setLastError("Failed to write file");
    return false;
}

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
const char* CarlaEngine::getCurrentProjectFolder() const noexcept
{
    return pData->currentProjectFolder.isNotEmpty() ? pData->currentProjectFolder.buffer()
                                                    : nullptr;
}

const char* CarlaEngine::getCurrentProjectFilename() const noexcept
{
    return pData->currentProjectFilename;
}

void CarlaEngine::clearCurrentProjectFilename() noexcept
{
    pData->currentProjectFilename.clear();
    pData->currentProjectFolder.clear();
}
#endif

// -----------------------------------------------------------------------
// Information (base)

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

const float* CarlaEngine::getPeaks(const uint pluginId) const noexcept
{
    static const float kFallback[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (pluginId == MAIN_CARLA_PLUGIN_ID)
    {
        // get peak from first plugin, if available
        if (const uint count = pData->curPluginCount)
        {
            pData->peaks[0] = pData->plugins[0].peaks[0];
            pData->peaks[1] = pData->plugins[0].peaks[1];
            pData->peaks[2] = pData->plugins[count-1].peaks[2];
            pData->peaks[3] = pData->plugins[count-1].peaks[3];
        }
        else
        {
            carla_zeroFloats(pData->peaks, 4);
        }

        return pData->peaks;
    }

    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, kFallback);

    return pData->plugins[pluginId].peaks;
}

float CarlaEngine::getInputPeak(const uint pluginId, const bool isLeft) const noexcept
{
    if (pluginId == MAIN_CARLA_PLUGIN_ID)
    {
        // get peak from first plugin, if available
        if (pData->curPluginCount > 0)
            return pData->plugins[0].peaks[isLeft ? 0 : 1];
        return 0.0f;
    }

    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);

    return pData->plugins[pluginId].peaks[isLeft ? 0 : 1];
}

float CarlaEngine::getOutputPeak(const uint pluginId, const bool isLeft) const noexcept
{
    if (pluginId == MAIN_CARLA_PLUGIN_ID)
    {
        // get peak from last plugin, if available
        if (pData->curPluginCount > 0)
            return pData->plugins[pData->curPluginCount-1].peaks[isLeft ? 2 : 3];
        return 0.0f;
    }

    CARLA_SAFE_ASSERT_RETURN(pluginId < pData->curPluginCount, 0.0f);

    return pData->plugins[pluginId].peaks[isLeft ? 2 : 3];
}

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const bool sendHost, const bool sendOSC,
                           const EngineCallbackOpcode action, const uint pluginId,
                           const int value1, const int value2, const int value3,
                           const float valuef, const char* const valueStr) noexcept
{
#ifdef DEBUG
    if (pData->isIdling)
        carla_stdout("CarlaEngine::callback [while idling] (%s, %s, %i:%s, %i, %i, %i, %i, %f, \"%s\")",
                     bool2str(sendHost), bool2str(sendOSC),
                     action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3,
                     static_cast<double>(valuef), valueStr);
    else if (action != ENGINE_CALLBACK_IDLE && action != ENGINE_CALLBACK_NOTE_ON && action != ENGINE_CALLBACK_NOTE_OFF)
        carla_debug("CarlaEngine::callback(%s, %s, %i:%s, %i, %i, %i, %i, %f, \"%s\")",
                    bool2str(sendHost), bool2str(sendOSC),
                    action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3,
                    static_cast<double>(valuef), valueStr);
#endif

    if (sendHost && pData->callback != nullptr)
    {
        if (action == ENGINE_CALLBACK_IDLE)
            ++pData->isIdling;

        try {
            pData->callback(pData->callbackPtr, action, pluginId, value1, value2, value3, valuef, valueStr);
        } CARLA_SAFE_EXCEPTION("callback")

        if (action == ENGINE_CALLBACK_IDLE)
            --pData->isIdling;
    }

    if (sendOSC)
    {
#if defined(HAVE_LIBLO) && ! defined(BUILD_BRIDGE)
        if (pData->osc.isControlRegisteredForTCP())
        {
            switch (action)
            {
            case ENGINE_CALLBACK_RELOAD_INFO:
            {
                CarlaPluginPtr plugin = pData->plugins[pluginId].plugin;
                CARLA_SAFE_ASSERT_BREAK(plugin != nullptr);

                pData->osc.sendPluginInfo(plugin);
                break;
            }

            case ENGINE_CALLBACK_RELOAD_PARAMETERS:
            {
                CarlaPluginPtr plugin = pData->plugins[pluginId].plugin;
                CARLA_SAFE_ASSERT_BREAK(plugin != nullptr);

                pData->osc.sendPluginPortCount(plugin);

                if (const uint32_t count = plugin->getParameterCount())
                {
                    for (uint32_t i=0; i<count; ++i)
                      pData->osc.sendPluginParameterInfo(plugin, i);
                }
                break;
            }

            case ENGINE_CALLBACK_RELOAD_PROGRAMS:
            {
                CarlaPluginPtr plugin = pData->plugins[pluginId].plugin;
                CARLA_SAFE_ASSERT_BREAK(plugin != nullptr);

                pData->osc.sendPluginProgramCount(plugin);

                if (const uint32_t count = plugin->getProgramCount())
                {
                    for (uint32_t i=0; i<count; ++i)
                      pData->osc.sendPluginProgram(plugin, i);
                }

                if (const uint32_t count = plugin->getMidiProgramCount())
                {
                    for (uint32_t i=0; i<count; ++i)
                      pData->osc.sendPluginMidiProgram(plugin, i);
                }
                break;
            }

            case ENGINE_CALLBACK_PLUGIN_ADDED:
            case ENGINE_CALLBACK_RELOAD_ALL:
            {
                CarlaPluginPtr plugin = pData->plugins[pluginId].plugin;
                CARLA_SAFE_ASSERT_BREAK(plugin != nullptr);

                pData->osc.sendPluginInfo(plugin);
                pData->osc.sendPluginPortCount(plugin);
                pData->osc.sendPluginDataCount(plugin);

                if (const uint32_t count = plugin->getParameterCount())
                {
                    for (uint32_t i=0; i<count; ++i)
                      pData->osc.sendPluginParameterInfo(plugin, i);
                }

                if (const uint32_t count = plugin->getProgramCount())
                {
                    for (uint32_t i=0; i<count; ++i)
                      pData->osc.sendPluginProgram(plugin, i);
                }

                if (const uint32_t count = plugin->getMidiProgramCount())
                {
                    for (uint32_t i=0; i<count; ++i)
                      pData->osc.sendPluginMidiProgram(plugin, i);
                }

                if (const uint32_t count = plugin->getCustomDataCount())
                {
                    for (uint32_t i=0; i<count; ++i)
                      pData->osc.sendPluginCustomData(plugin, i);
                }

                pData->osc.sendPluginInternalParameterValues(plugin);
                break;
            }

            case ENGINE_CALLBACK_IDLE:
                return;

            default:
                break;
            }

            pData->osc.sendCallback(action, pluginId, value1, value2, value3, valuef, valueStr);
        }
#endif
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
    CARLA_SAFE_ASSERT_RETURN(bpm >= 20.0,)

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

void CarlaEngine::setActionCanceled(const bool canceled) noexcept
{
    pData->actionCanceled = canceled;
}

bool CarlaEngine::wasActionCanceled() const noexcept
{
    return pData->actionCanceled;
}

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
        case ENGINE_OPTION_AUDIO_DRIVER:
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

    case ENGINE_OPTION_RESET_XRUNS:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.resetXruns = (value != 0);
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

    case ENGINE_OPTION_AUDIO_DRIVER:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

        if (pData->options.audioDriver != nullptr)
            delete[] pData->options.audioDriver;

        pData->options.audioDriver = carla_strdup_safe(valueStr);
        break;

    case ENGINE_OPTION_AUDIO_DEVICE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

        if (pData->options.audioDevice != nullptr)
            delete[] pData->options.audioDevice;

        pData->options.audioDevice = carla_strdup_safe(valueStr);
        break;

#ifndef BUILD_BRIDGE
    case ENGINE_OPTION_OSC_ENABLED:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.oscEnabled = (value != 0);
        break;

    case ENGINE_OPTION_OSC_PORT_TCP:
        CARLA_SAFE_ASSERT_RETURN(value <= 0 || value >= 1024,);
        pData->options.oscPortTCP = value;
        break;

    case ENGINE_OPTION_OSC_PORT_UDP:
        CARLA_SAFE_ASSERT_RETURN(value <= 0 || value >= 1024,);
        pData->options.oscPortUDP = value;
        break;
#endif

    case ENGINE_OPTION_FILE_PATH:
        CARLA_SAFE_ASSERT_RETURN(value > FILE_NONE,);
        CARLA_SAFE_ASSERT_RETURN(value <= FILE_MIDI,);

        switch (value)
        {
        case FILE_AUDIO:
            if (pData->options.pathAudio != nullptr)
                delete[] pData->options.pathAudio;
            if (valueStr != nullptr)
                pData->options.pathAudio = carla_strdup_safe(valueStr);
            else
                pData->options.pathAudio = nullptr;
            break;
        case FILE_MIDI:
            if (pData->options.pathMIDI != nullptr)
                delete[] pData->options.pathMIDI;
            if (valueStr != nullptr)
                pData->options.pathMIDI = carla_strdup_safe(valueStr);
            else
                pData->options.pathMIDI = nullptr;
            break;
        default:
            return carla_stderr("CarlaEngine::setOption(%i:%s, %i, \"%s\") - Invalid file type",
                                option, EngineOption2Str(option), value, valueStr);
            break;
        }
        break;
    case ENGINE_OPTION_PLUGIN_PATH:
        CARLA_SAFE_ASSERT_RETURN(value > PLUGIN_NONE,);
        CARLA_SAFE_ASSERT_RETURN(value <= PLUGIN_TYPE_COUNT,);

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
        case PLUGIN_VST3:
            if (pData->options.pathVST3 != nullptr)
                delete[] pData->options.pathVST3;
            if (valueStr != nullptr)
                pData->options.pathVST3 = carla_strdup_safe(valueStr);
            else
                pData->options.pathVST3 = nullptr;
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
        case PLUGIN_JSFX:
            if (pData->options.pathJSFX != nullptr)
                delete[] pData->options.pathJSFX;
            if (valueStr != nullptr)
                pData->options.pathJSFX = carla_strdup_safe(valueStr);
            else
                pData->options.pathJSFX = nullptr;
            break;
        case PLUGIN_CLAP:
            if (pData->options.pathCLAP != nullptr)
                delete[] pData->options.pathCLAP;
            if (valueStr != nullptr)
                pData->options.pathCLAP = carla_strdup_safe(valueStr);
            else
                pData->options.pathCLAP = nullptr;
            break;
        default:
            return carla_stderr("CarlaEngine::setOption(%i:%s, %i, \"%s\") - Invalid plugin type",
                                option, EngineOption2Str(option), value, valueStr);
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
            String interposerPath(String(pData->options.binaryDir) + "/libcarla_interposer-safe.so");
            ::setenv("LD_PRELOAD", interposerPath.buffer(), 1);
        }
        else
        {
            ::unsetenv("LD_PRELOAD");
        }
#endif
    }   break;

    case ENGINE_OPTION_FRONTEND_BACKGROUND_COLOR:
        pData->options.bgColor = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_FRONTEND_FOREGROUND_COLOR:
        pData->options.fgColor = static_cast<uint>(value);
        break;

    case ENGINE_OPTION_FRONTEND_UI_SCALE:
        CARLA_SAFE_ASSERT_RETURN(value > 0,);
        pData->options.uiScale = static_cast<float>(value) / 1000;
        break;

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

#ifndef BUILD_BRIDGE
    case ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT:
        break;
#endif

    case ENGINE_OPTION_CLIENT_NAME_PREFIX:
        if (pData->options.clientNamePrefix != nullptr)
            delete[] pData->options.clientNamePrefix;

        pData->options.clientNamePrefix = valueStr != nullptr && valueStr[0] != '\0'
                                        ? carla_strdup_safe(valueStr)
                                        : nullptr;
        break;

    case ENGINE_OPTION_PLUGINS_ARE_STANDALONE:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        pData->options.pluginsAreStandalone = (value != 0);
        break;
    }
}

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// OSC Stuff

bool CarlaEngine::isOscControlRegistered() const noexcept
{
# ifdef HAVE_LIBLO
    return pData->osc.isControlRegisteredForTCP();
# else
    return false;
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
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
        {
            if (plugin->isEnabled() && plugin->tryLock(true))
            {
                plugin->bufferSizeChanged(newBufferSize);
                plugin->unlock();
            }
        }
    }

    callback(true, true, ENGINE_CALLBACK_BUFFER_SIZE_CHANGED, 0, static_cast<int>(newBufferSize), 0, 0, 0.0f, nullptr);
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
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
        {
            if (plugin->isEnabled() && plugin->tryLock(true))
            {
                plugin->sampleRateChanged(newSampleRate);
                plugin->unlock();
            }
        }
    }

    callback(true, true, ENGINE_CALLBACK_SAMPLE_RATE_CHANGED, 0, 0, 0, 0, static_cast<float>(newSampleRate), nullptr);
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
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
            if (plugin->isEnabled())
                plugin->offlineModeChanged(isOfflineNow);
    }
}

void CarlaEngine::setPluginPeaksRT(const uint pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept
{
    EnginePluginData& pluginData(pData->plugins[pluginId]);

    pluginData.peaks[0]  = inPeaks[0];
    pluginData.peaks[1]  = inPeaks[1];
    pluginData.peaks[2] = outPeaks[0];
    pluginData.peaks[3] = outPeaks[1];
}

void CarlaEngine::saveProjectInternal(water::MemoryOutputStream& outStream) const
{
    // send initial prepareForSave first, giving time for bridges to act
    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
        {
            if (plugin->isEnabled())
            {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                // deactivate bridge client-side ping check, since some plugins block during save
                if (plugin->getHints() & PLUGIN_IS_BRIDGE)
                    plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "__CarlaPingOnOff__", "false", false);
#endif
                plugin->prepareForSave(false);
            }
        }
    }

    outStream << "<?xml version='1.0' encoding='UTF-8'?>\n";
    outStream << "<!DOCTYPE CARLA-PROJECT>\n";
    outStream << "<CARLA-PROJECT VERSION='" CARLA_VERSION_STRMIN "'";

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->ignoreClientPrefix)
        outStream << " IgnoreClientPrefix='true'";
#endif

    outStream << ">\n";

    const bool isPlugin(getType() == kEngineTypePlugin);
    const EngineOptions& options(pData->options);

    {
        MemoryOutputStream outSettings(1024);

        outSettings << " <EngineSettings>\n";

        outSettings << "  <ForceStereo>"         << bool2str(options.forceStereo)         << "</ForceStereo>\n";
        outSettings << "  <PreferPluginBridges>" << bool2str(options.preferPluginBridges) << "</PreferPluginBridges>\n";
        outSettings << "  <PreferUiBridges>"     << bool2str(options.preferUiBridges)     << "</PreferUiBridges>\n";
        outSettings << "  <UIsAlwaysOnTop>"      << bool2str(options.uisAlwaysOnTop)      << "</UIsAlwaysOnTop>\n";

        outSettings << "  <MaxParameters>"       << water::String(options.maxParameters)    << "</MaxParameters>\n";
        outSettings << "  <UIBridgesTimeout>"    << water::String(options.uiBridgesTimeout) << "</UIBridgesTimeout>\n";

        if (isPlugin)
        {
            outSettings << "  <LADSPA_PATH>" << xmlSafeString(options.pathLADSPA, true) << "</LADSPA_PATH>\n";
            outSettings << "  <DSSI_PATH>"   << xmlSafeString(options.pathDSSI,   true) << "</DSSI_PATH>\n";
            outSettings << "  <LV2_PATH>"    << xmlSafeString(options.pathLV2,    true) << "</LV2_PATH>\n";
            outSettings << "  <VST2_PATH>"   << xmlSafeString(options.pathVST2,   true) << "</VST2_PATH>\n";
            outSettings << "  <VST3_PATH>"   << xmlSafeString(options.pathVST3,   true) << "</VST3_PATH>\n";
            outSettings << "  <SF2_PATH>"    << xmlSafeString(options.pathSF2,    true) << "</SF2_PATH>\n";
            outSettings << "  <SFZ_PATH>"    << xmlSafeString(options.pathSFZ,    true) << "</SFZ_PATH>\n";
            outSettings << "  <JSFX_PATH>"   << xmlSafeString(options.pathJSFX,   true) << "</JSFX_PATH>\n";
            outSettings << "  <CLAP_PATH>"   << xmlSafeString(options.pathCLAP,   true) << "</CLAP_PATH>\n";
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
    carla_zeroChars(strBuf, STR_MAX+1);

    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
        {
            if (plugin->isEnabled())
            {
                MemoryOutputStream outPlugin(4096), streamPlugin;
                plugin->getStateSave(false).dumpToMemoryStream(streamPlugin);

                outPlugin << "\n";

                if (plugin->getRealName(strBuf))
                    outPlugin << " <!-- " << xmlSafeString(strBuf, true) << " -->\n";

                outPlugin << " <Plugin>\n";
                outPlugin << streamPlugin;
                outPlugin << " </Plugin>\n";
                outStream << outPlugin;
            }
        }
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // tell bridges we're done saving
    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
            if (plugin->isEnabled() && (plugin->getHints() & PLUGIN_IS_BRIDGE) != 0)
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "__CarlaPingOnOff__", "true", false);
    }

    // save internal connections
    if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
    {
        uint posCount = 0;
        const char* const* const patchbayConns = getPatchbayConnections(false);
        const PatchbayPosition* const patchbayPos = getPatchbayPositions(false, posCount);

        if (patchbayConns != nullptr || patchbayPos != nullptr)
        {
            MemoryOutputStream outPatchbay(2048);

            outPatchbay << "\n <Patchbay>\n";

            if (patchbayConns != nullptr)
            {
                for (int i=0; patchbayConns[i] != nullptr && patchbayConns[i+1] != nullptr; ++i, ++i)
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
            }

            if (patchbayPos != nullptr && posCount != 0)
            {
                outPatchbay << "  <Positions>\n";

                for (uint i=0; i<posCount; ++i)
                {
                    const PatchbayPosition& ppos(patchbayPos[i]);

                    CARLA_SAFE_ASSERT_CONTINUE(ppos.name != nullptr && ppos.name[0] != '\0');

                    outPatchbay << "   <Position x1=\"" << ppos.x1 << "\" y1=\"" << ppos.y1;
                    if (ppos.x2 != 0 || ppos.y2 != 0)
                        outPatchbay << "\" x2=\"" << ppos.x2 << "\" y2=\"" << ppos.y2;
                    if (ppos.pluginId >= 0)
                        outPatchbay << "\" pluginId=\"" << ppos.pluginId;
                    outPatchbay << "\">\n";
                    outPatchbay << "    <Name>" << xmlSafeString(ppos.name, true) << "</Name>\n";
                    outPatchbay << "   </Position>\n";

                    if (ppos.dealloc)
                        delete[] ppos.name;
                }

                outPatchbay << "  </Positions>\n";
            }

            outPatchbay << " </Patchbay>\n";
            outStream << outPatchbay;

            delete[] patchbayPos;
        }

    }

    // if we're running inside some session-manager (and using JACK), let them handle the connections
    bool saveExternalConnections, saveExternalPositions = true;

    /**/ if (isPlugin)
    {
        saveExternalConnections = false;
        saveExternalPositions = false;
    }
    else if (std::strcmp(getCurrentDriverName(), "JACK") != 0)
    {
        saveExternalConnections = true;
    }
    else if (std::getenv("CARLA_DONT_MANAGE_CONNECTIONS") != nullptr)
    {
        saveExternalConnections = false;
    }
    else
    {
        saveExternalConnections = true;
    }

    if (saveExternalConnections || saveExternalPositions)
    {
        uint posCount = 0;
        const char* const* const patchbayConns = saveExternalConnections
                                               ? getPatchbayConnections(true)
                                               : nullptr;
        const PatchbayPosition* const patchbayPos = saveExternalPositions
                                                  ? getPatchbayPositions(true, posCount)
                                                  : nullptr;

        if (patchbayConns != nullptr || patchbayPos != nullptr)
        {
            MemoryOutputStream outPatchbay(2048);

            outPatchbay << "\n <ExternalPatchbay>\n";

            if (patchbayConns != nullptr)
            {
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
            }

            if (patchbayPos != nullptr && posCount != 0)
            {
                outPatchbay << "  <Positions>\n";

                for (uint i=0; i<posCount; ++i)
                {
                    const PatchbayPosition& ppos(patchbayPos[i]);

                    CARLA_SAFE_ASSERT_CONTINUE(ppos.name != nullptr && ppos.name[0] != '\0');

                    outPatchbay << "   <Position x1=\"" << ppos.x1 << "\" y1=\"" << ppos.y1;
                    if (ppos.x2 != 0 || ppos.y2 != 0)
                        outPatchbay << "\" x2=\"" << ppos.x2 << "\" y2=\"" << ppos.y2;
                    if (ppos.pluginId >= 0)
                        outPatchbay << "\" pluginId=\"" << ppos.pluginId;
                    outPatchbay << "\">\n";
                    outPatchbay << "    <Name>" << xmlSafeString(ppos.name, true) << "</Name>\n";
                    outPatchbay << "   </Position>\n";

                    if (ppos.dealloc)
                        delete[] ppos.name;
                }

                outPatchbay << "  </Positions>\n";
            }

            outPatchbay << " </ExternalPatchbay>\n";
            outStream << outPatchbay;
        }
    }
#endif

    outStream << "</CARLA-PROJECT>\n";
}

static water::String findBinaryInCustomPath(const char* const searchPath, const char* const binary)
{
    const StringArray searchPaths(StringArray::fromTokens(searchPath, CARLA_OS_SPLIT_STR, ""));

    // try direct filename first
    water::String jbinary(binary);

    // adjust for current platform
#ifdef CARLA_OS_WIN
    if (jbinary[0] == '/')
        jbinary = "C:" + jbinary.replaceCharacter('/', '\\');
#else
    if (jbinary[1] == ':' && (jbinary[2] == '\\' || jbinary[2] == '/'))
        jbinary = jbinary.substring(2).replaceCharacter('\\', '/');
#endif

    water::String filename = File(jbinary.toRawUTF8()).getFileName();

    int searchFlags = File::findFiles|File::ignoreHiddenFiles;

    if (filename.endsWithIgnoreCase(".vst3"))
        searchFlags |= File::findDirectories;
#ifdef CARLA_OS_MAC
    else if (filename.endsWithIgnoreCase(".vst"))
        searchFlags |= File::findDirectories;
#endif

    std::vector<File> results;
    for (const water::String *it=searchPaths.begin(), *end=searchPaths.end(); it != end; ++it)
    {
        const File path(it->toRawUTF8());

        results.clear();
        path.findChildFiles(results, searchFlags, true, filename.toRawUTF8());

        if (!results.empty())
            return results.front().getFullPathName();
    }

    // try changing extension
#if defined(CARLA_OS_MAC)
    if (filename.endsWithIgnoreCase(".dll") || filename.endsWithIgnoreCase(".so"))
        filename = File(jbinary.toRawUTF8()).getFileNameWithoutExtension() + ".dylib";
#elif defined(CARLA_OS_WIN)
    if (filename.endsWithIgnoreCase(".dylib") || filename.endsWithIgnoreCase(".so"))
        filename = File(jbinary.toRawUTF8()).getFileNameWithoutExtension() + ".dll";
#else
    if (filename.endsWithIgnoreCase(".dll") || filename.endsWithIgnoreCase(".dylib"))
        filename = File(jbinary.toRawUTF8()).getFileNameWithoutExtension() + ".so";
#endif
    else
        return {};

    for (const water::String *it=searchPaths.begin(), *end=searchPaths.end(); it != end; ++it)
    {
        const File path(it->toRawUTF8());

        results.clear();
        path.findChildFiles(results, searchFlags, true, filename.toRawUTF8());

        if (!results.empty())
            return results.front().getFullPathName();
    }

    return {};
}

bool CarlaEngine::loadProjectInternal(water::XmlDocument& xmlDoc, const bool alwaysLoadConnections)
{
    carla_debug("CarlaEngine::loadProjectInternal(%p, %s) - START", &xmlDoc, bool2str(alwaysLoadConnections));

    ScopedPointer<XmlElement> xmlElement(xmlDoc.getDocumentElement(true));
    CARLA_SAFE_ASSERT_RETURN_ERR(xmlElement != nullptr, "Failed to parse project file");

    const water::String& xmlType(xmlElement->getTagName());
    const bool isPreset(xmlType.equalsIgnoreCase("carla-preset"));

    if (! (xmlType.equalsIgnoreCase("carla-project") || isPreset))
    {
        callback(true, true, ENGINE_CALLBACK_PROJECT_LOAD_FINISHED, 0, 0, 0, 0, 0.0f, nullptr);
        setLastError("Not a valid Carla project or preset file");
        return false;
    }

    pData->actionCanceled = false;
    callback(true, true, ENGINE_CALLBACK_CANCELABLE_ACTION, 0, 1, 0, 0, 0.0f, "Loading project");

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    if (pData->options.clientNamePrefix != nullptr)
    {
        if (carla_isEqual(xmlElement->getDoubleAttribute("VERSION", 0.0), 2.0) ||
            xmlElement->getBoolAttribute("IgnoreClientPrefix", false))
        {
            carla_stdout("Loading project in compatibility mode, will ignore client name prefix");
            pData->ignoreClientPrefix = true;
            setOption(ENGINE_OPTION_CLIENT_NAME_PREFIX, 0, "");
        }
    }

    const CarlaScopedValueSetter<bool> csvs(pData->loadingProject, true, false);
#endif

    // completely load file
    xmlElement = xmlDoc.getDocumentElement(false);
    CARLA_SAFE_ASSERT_RETURN_ERR(xmlElement != nullptr, "Failed to completely parse project file");

    if (pData->aboutToClose)
        return true;

    if (pData->actionCanceled)
    {
        setLastError("Project load canceled");
        return false;
    }

    callback(true, false, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    const bool isMultiClient = pData->options.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
    const bool isPatchbay    = pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY;
#endif
    const bool isPlugin = getType() == kEngineTypePlugin;

    // load engine settings first of all
    if (XmlElement* const elem = isPreset ? nullptr : xmlElement->getChildByName("EngineSettings"))
    {
        for (XmlElement* settElem = elem->getFirstChildElement(); settElem != nullptr; settElem = settElem->getNextElement())
        {
            const water::String& tag(settElem->getTagName());
            const water::String  text(settElem->getAllSubText().trim());

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
                else if (tag.equalsIgnoreCase("VST3_PATH"))
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_VST3;
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
                else if (tag == "JSFX_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_JSFX;
                    valueStr = text.toRawUTF8();
                }
                else if (tag == "CLAP_PATH")
                {
                    option   = ENGINE_OPTION_PLUGIN_PATH;
                    value    = PLUGIN_CLAP;
                    valueStr = text.toRawUTF8();
                }
            }

            if (option == -1)
            {
                // check old stuff, unhandled now
                if (tag == "GIG_PATH")
                    continue;
                // ignored tags
                if (tag == "LADSPA_PATH" || tag == "DSSI_PATH" || tag == "LV2_PATH" || tag == "VST2_PATH")
                    continue;
                if (tag == "VST3_PATH" || tag == "AU_PATH")
                    continue;
                if (tag == "SF2_PATH" || tag == "SFZ_PATH" || tag == "JSFX_PATH" || tag == "CLAP_PATH")
                    continue;

                // hmm something is wrong..
                carla_stderr2("CarlaEngine::loadProjectInternal() - Unhandled option '%s'", tag.toRawUTF8());
                continue;
            }

            setOption(static_cast<EngineOption>(option), value, valueStr);
        }

        if (pData->aboutToClose)
            return true;

        if (pData->actionCanceled)
        {
            setLastError("Project load canceled");
            return false;
        }
    }

    // now setup transport
    if (XmlElement* const elem = (isPreset || isPlugin) ? nullptr : xmlElement->getChildByName("Transport"))
    {
        if (XmlElement* const bpmElem = elem->getChildByName("BeatsPerMinute"))
        {
            const water::String bpmText(bpmElem->getAllSubText().trim());
            const double bpm = bpmText.getDoubleValue();

            // some sane limits
            if (bpm >= 20.0 && bpm < 400.0)
                pData->time.setBPM(bpm);

            if (pData->aboutToClose)
                return true;

            if (pData->actionCanceled)
            {
                setLastError("Project load canceled");
                return false;
            }
        }
    }

    // and we handle plugins
    for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
    {
        const water::String& tagName(elem->getTagName());

        if (isPreset || tagName == "Plugin")
        {
            CarlaStateSave stateSave;
            stateSave.fillFromXmlElement(isPreset ? xmlElement.get() : elem);

            if (pData->aboutToClose)
                return true;

            if (pData->actionCanceled)
            {
                setLastError("Project load canceled");
                return false;
            }

            CARLA_SAFE_ASSERT_CONTINUE(stateSave.type != nullptr);

          #if !(defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) || defined(CARLA_PLUGIN_ONLY_BRIDGE))
            // compatibility code to load projects with GIG files
            // FIXME Remove on 2.1 release
            if (std::strcmp(stateSave.type, "GIG") == 0)
            {
                if (addPlugin(PLUGIN_LV2, "", stateSave.name, "http://linuxsampler.org/plugins/linuxsampler", 0, nullptr))
                {
                    const uint pluginId = pData->curPluginCount;

                    if (const CarlaPluginPtr plugin = pData->plugins[pluginId].plugin)
                    {
                        if (pData->aboutToClose)
                            return true;

                        if (pData->actionCanceled)
                        {
                            setLastError("Project load canceled");
                            return false;
                        }

                        water::String lsState;
                        lsState << "0.35\n";
                        lsState << "18 0 Chromatic\n";
                        lsState << "18 1 Drum Kits\n";
                        lsState << "20 0\n";
                        lsState << "0 1 " << stateSave.binary << "\n";
                        lsState << "0 0 0 0 1 0 GIG\n";

                        plugin->setCustomData(LV2_ATOM__String, "http://linuxsampler.org/schema#state-string", lsState.toRawUTF8(), true);
                        plugin->restoreLV2State(true);

                        plugin->setDryWet(stateSave.dryWet, true, true);
                        plugin->setVolume(stateSave.volume, true, true);
                        plugin->setBalanceLeft(stateSave.balanceLeft, true, true);
                        plugin->setBalanceRight(stateSave.balanceRight, true, true);
                        plugin->setPanning(stateSave.panning, true, true);
                        plugin->setCtrlChannel(stateSave.ctrlChannel, true, true);
                        plugin->setActive(stateSave.active, true, true);
                        plugin->setEnabled(true);

                        ++pData->curPluginCount;
                        callback(true, true, ENGINE_CALLBACK_PLUGIN_ADDED, pluginId, plugin->getType(),
                                 0, 0, 0.0f,
                                 plugin->getName());

                        if (isPatchbay)
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

                callback(true, true, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
                continue;
            }
           #ifdef SFZ_FILES_USING_SFIZZ
            if (std::strcmp(stateSave.type, "SFZ") == 0)
            {
                if (addPlugin(PLUGIN_LV2, "", stateSave.name, "http://sfztools.github.io/sfizz", 0, nullptr))
                {
                    const uint pluginId = pData->curPluginCount;

                    if (const CarlaPluginPtr plugin = pData->plugins[pluginId].plugin)
                    {
                        if (pData->aboutToClose)
                            return true;

                        if (pData->actionCanceled)
                        {
                            setLastError("Project load canceled");
                            return false;
                        }

                        plugin->setCustomData(LV2_ATOM__Path,
                                              "http://sfztools.github.io/sfizz:sfzfile",
                                              stateSave.binary,
                                              false);

                        plugin->restoreLV2State(true);

                        plugin->setDryWet(stateSave.dryWet, true, true);
                        plugin->setVolume(stateSave.volume, true, true);
                        plugin->setBalanceLeft(stateSave.balanceLeft, true, true);
                        plugin->setBalanceRight(stateSave.balanceRight, true, true);
                        plugin->setPanning(stateSave.panning, true, true);
                        plugin->setCtrlChannel(stateSave.ctrlChannel, true, true);
                        plugin->setActive(stateSave.active, true, true);
                        plugin->setEnabled(true);

                        ++pData->curPluginCount;
                        callback(true, true, ENGINE_CALLBACK_PLUGIN_ADDED, pluginId, plugin->getType(),
                                 0, 0, 0.0f,
                                 plugin->getName());

                        if (isPatchbay)
                            pData->graph.addPlugin(plugin);
                    }
                    else
                    {
                        carla_stderr2("Failed to get new plugin, state will not be restored correctly\n");
                    }
                }
                else
                {
                    carla_stderr2("Failed to load a sfizz LV2 plugin, SFZ file won't be loaded");
                }

                callback(true, true, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
                continue;
            }
           #endif
          #endif

            const void* extraStuff    = nullptr;
            static const char kTrue[] = "true";

            const PluginType ptype = getPluginTypeFromString(stateSave.type);

            switch (ptype)
            {
            case PLUGIN_SF2:
                if (String(stateSave.label).endsWith(" (16 outs)"))
                    extraStuff = kTrue;
                // fall through
            case PLUGIN_LADSPA:
            case PLUGIN_DSSI:
            case PLUGIN_VST2:
            case PLUGIN_VST3:
            case PLUGIN_SFZ:
            case PLUGIN_JSFX:
            case PLUGIN_CLAP:
                if (stateSave.binary != nullptr && stateSave.binary[0] != '\0' &&
                    ! (File::isAbsolutePath(stateSave.binary) && File(stateSave.binary).exists()))
                {
                    const char* searchPath;

                    switch (ptype)
                    {
                    case PLUGIN_LADSPA: searchPath = pData->options.pathLADSPA; break;
                    case PLUGIN_DSSI:   searchPath = pData->options.pathDSSI;   break;
                    case PLUGIN_VST2:   searchPath = pData->options.pathVST2;   break;
                    case PLUGIN_VST3:   searchPath = pData->options.pathVST3;   break;
                    case PLUGIN_SF2:    searchPath = pData->options.pathSF2;    break;
                    case PLUGIN_SFZ:    searchPath = pData->options.pathSFZ;    break;
                    case PLUGIN_JSFX:   searchPath = pData->options.pathJSFX;   break;
                    case PLUGIN_CLAP:   searchPath = pData->options.pathCLAP;   break;
                    default:            searchPath = nullptr;                   break;
                    }

                    if (searchPath != nullptr && searchPath[0] != '\0')
                    {
                        carla_stderr("Plugin binary '%s' doesn't exist on this filesystem, let's look for it...",
                                     stateSave.binary);

                        water::String result = findBinaryInCustomPath(searchPath, stateSave.binary);

                        if (result.isEmpty())
                        {
                            switch (ptype)
                            {
                            case PLUGIN_LADSPA: searchPath = std::getenv("LADSPA_PATH"); break;
                            case PLUGIN_DSSI:   searchPath = std::getenv("DSSI_PATH");   break;
                            case PLUGIN_VST2:   searchPath = std::getenv("VST_PATH");    break;
                            case PLUGIN_VST3:   searchPath = std::getenv("VST3_PATH");   break;
                            case PLUGIN_SF2:    searchPath = std::getenv("SF2_PATH");    break;
                            case PLUGIN_SFZ:    searchPath = std::getenv("SFZ_PATH");    break;
                            case PLUGIN_JSFX:   searchPath = std::getenv("JSFX_PATH");   break;
                            case PLUGIN_CLAP:   searchPath = std::getenv("CLAP_PATH");   break;
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

                        callback(true, true, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
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
            case PLUGIN_VST3:
            case PLUGIN_CLAP:
            case PLUGIN_AU:
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

                if (const CarlaPluginPtr plugin = pData->plugins[pluginId].plugin)
                {
                    if (pData->aboutToClose)
                        return true;

                    if (pData->actionCanceled)
                    {
                        setLastError("Project load canceled");
                        return false;
                    }

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
                    callback(true, true, ENGINE_CALLBACK_PLUGIN_ADDED, pluginId, plugin->getType(),
                             0, 0, 0.0f,
                             plugin->getName());

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                    if (isPatchbay)
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
                carla_stderr2("Failed to load a plugin '%s', error was:\n%s", stateSave.name, getLastError());
            }

            if (! isPreset)
                callback(true, true, ENGINE_CALLBACK_IDLE, 0, 0, 0, 0, 0.0f, nullptr);
        }

        if (isPreset)
        {
            callback(true, true, ENGINE_CALLBACK_PROJECT_LOAD_FINISHED, 0, 0, 0, 0, 0.0f, nullptr);
            callback(true, true, ENGINE_CALLBACK_CANCELABLE_ACTION, 0, 0, 0, 0, 0.0f, "Loading project");
            return true;
        }
    }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // tell bridges we're done loading
    for (uint i=0; i < pData->curPluginCount; ++i)
    {
        if (const CarlaPluginPtr plugin = pData->plugins[i].plugin)
            if (plugin->isEnabled() && (plugin->getHints() & PLUGIN_IS_BRIDGE) != 0)
                plugin->setCustomData(CUSTOM_DATA_TYPE_STRING, "__CarlaPingOnOff__", "true", false);
    }

    if (pData->aboutToClose)
        return true;

    if (pData->actionCanceled)
    {
        setLastError("Project load canceled");
        return false;
    }

    // now we handle positions
    bool loadingAsExternal;
    std::map<water::String, water::String> mapGroupNamesInternal, mapGroupNamesExternal;

    bool hasInternalPositions = false;

    if (XmlElement* const elemPatchbay = xmlElement->getChildByName("Patchbay"))
    {
        hasInternalPositions = true;

        if (XmlElement* const elemPositions = elemPatchbay->getChildByName("Positions"))
        {
            water::String name;
            PatchbayPosition ppos = { nullptr, -1, 0, 0, 0, 0, false };

            for (XmlElement* patchElem = elemPositions->getFirstChildElement(); patchElem != nullptr; patchElem = patchElem->getNextElement())
            {
                const water::String& patchTag(patchElem->getTagName());

                if (patchTag != "Position")
                    continue;

                XmlElement* const patchName = patchElem->getChildByName("Name");
                CARLA_SAFE_ASSERT_CONTINUE(patchName != nullptr);

                const water::String nameText(patchName->getAllSubText().trim());
                name = xmlSafeString(nameText, false);

                ppos.name = name.toRawUTF8();
                ppos.x1 = patchElem->getIntAttribute("x1");
                ppos.y1 = patchElem->getIntAttribute("y1");
                ppos.x2 = patchElem->getIntAttribute("x2");
                ppos.y2 = patchElem->getIntAttribute("y2");
                ppos.pluginId = patchElem->getIntAttribute("pluginId", -1);
                ppos.dealloc = false;

                loadingAsExternal = ppos.pluginId >= 0 && isMultiClient;

                if (name.isNotEmpty() && restorePatchbayGroupPosition(loadingAsExternal, ppos))
                {
                    if (name != ppos.name)
                    {
                        carla_stdout("Converted client name '%s' to '%s' for this session",
                                     name.toRawUTF8(), ppos.name);
                        if (loadingAsExternal)
                            mapGroupNamesExternal[name] = ppos.name;
                        else
                            mapGroupNamesInternal[name] = ppos.name;
                    }

                    if (ppos.dealloc)
                        std::free(const_cast<char*>(ppos.name));
                }
            }

            if (pData->aboutToClose)
                return true;

            if (pData->actionCanceled)
            {
                setLastError("Project load canceled");
                return false;
            }
        }
    }

    if (XmlElement* const elemPatchbay = xmlElement->getChildByName("ExternalPatchbay"))
    {
        if (XmlElement* const elemPositions = elemPatchbay->getChildByName("Positions"))
        {
            water::String name;
            PatchbayPosition ppos = { nullptr, -1, 0, 0, 0, 0, false };

            for (XmlElement* patchElem = elemPositions->getFirstChildElement(); patchElem != nullptr; patchElem = patchElem->getNextElement())
            {
                const water::String& patchTag(patchElem->getTagName());

                if (patchTag != "Position")
                    continue;

                XmlElement* const patchName = patchElem->getChildByName("Name");
                CARLA_SAFE_ASSERT_CONTINUE(patchName != nullptr);

                const water::String nameText(patchName->getAllSubText().trim());
                name = xmlSafeString(nameText, false);

                ppos.name = name.toRawUTF8();
                ppos.x1 = patchElem->getIntAttribute("x1");
                ppos.y1 = patchElem->getIntAttribute("y1");
                ppos.x2 = patchElem->getIntAttribute("x2");
                ppos.y2 = patchElem->getIntAttribute("y2");
                ppos.pluginId = patchElem->getIntAttribute("pluginId", -1);
                ppos.dealloc = false;

                loadingAsExternal = ppos.pluginId < 0 || hasInternalPositions || !isPatchbay;

                carla_debug("loadingAsExternal: %i because %i %i %i",
                            loadingAsExternal, ppos.pluginId < 0, hasInternalPositions, !isPatchbay);

                if (name.isNotEmpty() && restorePatchbayGroupPosition(loadingAsExternal, ppos))
                {
                    if (name != ppos.name)
                    {
                        carla_stdout("Converted client name '%s' to '%s' for this session",
                                     name.toRawUTF8(), ppos.name);

                        if (loadingAsExternal)
                            mapGroupNamesExternal[name] = ppos.name;
                        else
                            mapGroupNamesInternal[name] = ppos.name;
                    }

                    if (ppos.dealloc)
                        std::free(const_cast<char*>(ppos.name));
                }
            }

            if (pData->aboutToClose)
                return true;

            if (pData->actionCanceled)
            {
                setLastError("Project load canceled");
                return false;
            }
        }
    }

    bool hasInternalConnections = false;

    // and now we handle connections (internal)
    if (XmlElement* const elem = xmlElement->getChildByName("Patchbay"))
    {
        hasInternalConnections = true;

        if (isPatchbay)
        {
            water::String sourcePort, targetPort;

            for (XmlElement* patchElem = elem->getFirstChildElement(); patchElem != nullptr; patchElem = patchElem->getNextElement())
            {
                const water::String& patchTag(patchElem->getTagName());

                if (patchTag != "Connection")
                    continue;

                sourcePort.clear();
                targetPort.clear();

                for (XmlElement* connElem = patchElem->getFirstChildElement(); connElem != nullptr; connElem = connElem->getNextElement())
                {
                    const water::String& tag(connElem->getTagName());
                    const water::String  text(connElem->getAllSubText().trim());

                    /**/ if (tag == "Source")
                        sourcePort = xmlSafeString(text, false);
                    else if (tag == "Target")
                        targetPort = xmlSafeString(text, false);
                }

                if (sourcePort.isNotEmpty() && targetPort.isNotEmpty())
                {
                    std::map<water::String, water::String>& map(mapGroupNamesInternal);
                    std::map<water::String, water::String>::iterator it;

                    if ((it = map.find(sourcePort.upToFirstOccurrenceOf(":", false, false))) != map.end())
                        sourcePort = it->second + sourcePort.fromFirstOccurrenceOf(":", true, false);
                    if ((it = map.find(targetPort.upToFirstOccurrenceOf(":", false, false))) != map.end())
                        targetPort = it->second + targetPort.fromFirstOccurrenceOf(":", true, false);

                    restorePatchbayConnection(false, sourcePort.toRawUTF8(), targetPort.toRawUTF8());
                }
            }

            if (pData->aboutToClose)
                return true;

            if (pData->actionCanceled)
            {
                setLastError("Project load canceled");
                return false;
            }
        }
    }

    // if we're running inside some session-manager (and using JACK), let them handle the external connections
    bool loadExternalConnections;

    if (alwaysLoadConnections)
    {
        loadExternalConnections = true;
    }
    else
    {
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
    }

    // plus external connections too
    if (loadExternalConnections)
    {
        bool isExternal;
        loadingAsExternal = hasInternalConnections &&
            (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK || isPatchbay);

        for (XmlElement* elem = xmlElement->getFirstChildElement(); elem != nullptr; elem = elem->getNextElement())
        {
            const water::String& tagName(elem->getTagName());

            // check if we want to load patchbay-mode connections into an external (multi-client) graph
            if (tagName == "Patchbay")
            {
                if (isPatchbay)
                    continue;
                isExternal = false;
                loadingAsExternal = true;
            }
            // or load external patchbay connections
            else if (tagName == "ExternalPatchbay")
            {
                if (! isPatchbay)
                    loadingAsExternal = true;
                isExternal = true;
            }
            else
            {
                continue;
            }

            water::String sourcePort, targetPort;

            for (XmlElement* patchElem = elem->getFirstChildElement(); patchElem != nullptr; patchElem = patchElem->getNextElement())
            {
                const water::String& patchTag(patchElem->getTagName());

                if (patchTag != "Connection")
                    continue;

                sourcePort.clear();
                targetPort.clear();

                for (XmlElement* connElem = patchElem->getFirstChildElement(); connElem != nullptr; connElem = connElem->getNextElement())
                {
                    const water::String& tag(connElem->getTagName());
                    const water::String  text(connElem->getAllSubText().trim());

                    /**/ if (tag == "Source")
                        sourcePort = xmlSafeString(text, false);
                    else if (tag == "Target")
                        targetPort = xmlSafeString(text, false);
                }

                if (sourcePort.isNotEmpty() && targetPort.isNotEmpty())
                {
                    std::map<water::String, water::String>& map(loadingAsExternal ? mapGroupNamesExternal
                                                                                  : mapGroupNamesInternal);
                    std::map<water::String, water::String>::iterator it;

                    if (isExternal && isPatchbay && !loadingAsExternal && sourcePort.startsWith("system:capture_"))
                    {
                        water::String internalPort = sourcePort.trimCharactersAtStart("system:capture_");

                        if (pData->graph.getNumAudioOuts() < 3)
                        {
                            /**/ if (internalPort == "1")
                                internalPort = "Audio Input:Left";
                            else if (internalPort == "2")
                                internalPort = "Audio Input:Right";
                            else if (internalPort == "3")
                                internalPort = "Audio Input:Sidechain";
                            else
                                continue;
                        }
                        else
                        {
                            internalPort = "Audio Input:Capture " + internalPort;
                        }

                        carla_stdout("Converted port name '%s' to '%s' for this session",
                                     sourcePort.toRawUTF8(), internalPort.toRawUTF8());
                        sourcePort = internalPort;
                    }
                    else if (!isExternal && isMultiClient && sourcePort.startsWith("Audio Input:"))
                    {
                        water::String externalPort = sourcePort.trimCharactersAtStart("Audio Input:");

                        /**/ if (externalPort == "Left")
                            externalPort = "system:capture_1";
                        else if (externalPort == "Right")
                            externalPort = "system:capture_2";
                        else if (externalPort == "Sidechain")
                            externalPort = "system:capture_3";
                        else
                            externalPort = "system:capture_ " + externalPort.trimCharactersAtStart("Capture ");

                        carla_stdout("Converted port name '%s' to '%s' for this session",
                                     sourcePort.toRawUTF8(), externalPort.toRawUTF8());
                        sourcePort = externalPort;
                    }
                    else if ((it = map.find(sourcePort.upToFirstOccurrenceOf(":", false, false))) != map.end())
                    {
                        sourcePort = it->second + sourcePort.fromFirstOccurrenceOf(":", true, false);
                    }

                    if (isExternal && isPatchbay && !loadingAsExternal && targetPort.startsWith("system:playback_"))
                    {
                        water::String internalPort = targetPort.trimCharactersAtStart("system:playback_");

                        if (pData->graph.getNumAudioOuts() < 3)
                        {
                            /**/ if (internalPort == "1")
                                internalPort = "Audio Output:Left";
                            else if (internalPort == "2")
                                internalPort = "Audio Output:Right";
                            else
                                continue;
                        }
                        else
                        {
                            internalPort = "Audio Input:Playback " + internalPort;
                        }

                        carla_stdout("Converted port name '%s' to '%s' for this session",
                                     targetPort.toRawUTF8(), internalPort.toRawUTF8());
                        targetPort = internalPort;
                    }
                    else if (!isExternal && isMultiClient && targetPort.startsWith("Audio Output:"))
                    {
                        water::String externalPort = targetPort.trimCharactersAtStart("Audio Output:");

                        /**/ if (externalPort == "Left")
                            externalPort = "system:playback_1";
                        else if (externalPort == "Right")
                            externalPort = "system:playback_2";
                        else
                            externalPort = "system:playback_ " + externalPort.trimCharactersAtStart("Playback ");

                        carla_stdout("Converted port name '%s' to '%s' for this session",
                                     targetPort.toRawUTF8(), externalPort.toRawUTF8());
                        targetPort = externalPort;
                    }
                    else if ((it = map.find(targetPort.upToFirstOccurrenceOf(":", false, false))) != map.end())
                    {
                        targetPort = it->second + targetPort.fromFirstOccurrenceOf(":", true, false);
                    }

                    restorePatchbayConnection(loadingAsExternal, sourcePort.toRawUTF8(), targetPort.toRawUTF8());
                }
            }
            break;
        }
    }
#endif

    if (pData->options.resetXruns)
        clearXruns();

    callback(true, true, ENGINE_CALLBACK_PROJECT_LOAD_FINISHED, 0, 0, 0, 0, 0.0f, nullptr);
    callback(true, true, ENGINE_CALLBACK_CANCELABLE_ACTION, 0, 0, 0, 0, 0.0f, "Loading project");

    carla_debug("CarlaEngine::loadProjectInternal(%p, %s) - END", &xmlDoc, bool2str(alwaysLoadConnections));
    return true;

#ifdef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // unused
    (void)alwaysLoadConnections;
#endif
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
