// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// TODO:
// Check carla_stderr2("Engine is not running"); <= prepend func name and args

#include "CarlaHostImpl.hpp"
#include "CarlaMIDI.h"

#include "CarlaEngineInit.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "ThreadSafeFFTW.hpp"

#include "extra/Base64.hpp"
#include "extra/ScopedPointer.hpp"

#include "water/files/File.h"

#define CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(cond, msg, ret) \
    if (! (cond)) {                                              \
        carla_stderr2("%s: " msg, __FUNCTION__);                 \
        if (handle->isStandalone)                                \
            ((CarlaHostStandalone*)handle)->lastError = msg;     \
        return ret;                                              \
    }

// -------------------------------------------------------------------------------------------------------------------
// Always return a valid string ptr for standalone functions

static const char* const gNullCharPtr = "";

static void checkStringPtr(const char*& charPtr) noexcept
{
    if (charPtr == nullptr)
        charPtr = gNullCharPtr;
}

// -------------------------------------------------------------------------------------------------------------------
// Constructors

_CarlaPluginInfo::_CarlaPluginInfo() noexcept
    : type(CB::PLUGIN_NONE),
      category(CB::PLUGIN_CATEGORY_NONE),
      hints(0x0),
      optionsAvailable(0x0),
      optionsEnabled(0x0),
      filename(gNullCharPtr),
      name(gNullCharPtr),
      label(gNullCharPtr),
      maker(gNullCharPtr),
      copyright(gNullCharPtr),
      iconName(gNullCharPtr),
      uniqueId(0) {}

_CarlaPluginInfo::~_CarlaPluginInfo() noexcept
{
    if (label != gNullCharPtr)
        delete[] label;
    if (maker != gNullCharPtr)
        delete[] maker;
    if (copyright != gNullCharPtr)
        delete[] copyright;
}

_CarlaParameterInfo::_CarlaParameterInfo() noexcept
    : name(gNullCharPtr),
      symbol(gNullCharPtr),
      unit(gNullCharPtr),
      comment(gNullCharPtr),
      groupName(gNullCharPtr),
      scalePointCount(0) {}

_CarlaParameterInfo::~_CarlaParameterInfo() noexcept
{
    if (name != gNullCharPtr)
        delete[] name;
    if (symbol != gNullCharPtr)
        delete[] symbol;
    if (unit != gNullCharPtr)
        delete[] unit;
    if (comment != gNullCharPtr)
        delete[] comment;
    if (groupName != gNullCharPtr)
        delete[] groupName;
}

_CarlaScalePointInfo::_CarlaScalePointInfo() noexcept
    : value(0.0f),
      label(gNullCharPtr) {}

_CarlaScalePointInfo::~_CarlaScalePointInfo() noexcept
{
    if (label != gNullCharPtr)
        delete[] label;
}

_CarlaTransportInfo::_CarlaTransportInfo() noexcept
    : playing(false),
      frame(0),
      bar(0),
      beat(0),
      tick(0),
      bpm(0.0) {}

void _CarlaTransportInfo::clear() noexcept
{
    playing = false;
    frame = 0;
    bar = 0;
    beat = 0;
    tick = 0;
    bpm = 0.0;
}

// --------------------------------------------------------------------------------------------------------------------

using CARLA_BACKEND_NAMESPACE::CarlaPluginPtr;

// --------------------------------------------------------------------------------------------------------------------

uint carla_get_engine_driver_count()
{
    carla_debug("carla_get_engine_driver_count()");

    return CarlaEngine::getDriverCount();
}

const char* carla_get_engine_driver_name(uint index)
{
    carla_debug("carla_get_engine_driver_name(%i)", index);

    return CarlaEngine::getDriverName(index);
}

const char* const* carla_get_engine_driver_device_names(uint index)
{
    carla_debug("carla_get_engine_driver_device_names(%i)", index);

    return CarlaEngine::getDriverDeviceNames(index);
}

const EngineDriverDeviceInfo* carla_get_engine_driver_device_info(uint index, const char* name)
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr, nullptr);

    static EngineDriverDeviceInfo retDevInfo;
    static const uint32_t nullBufferSizes[] = { 0   };
    static const double   nullSampleRates[] = { 0.0 };

    carla_debug("carla_get_engine_driver_device_info(%i, \"%s\")", index, name);

    if (const EngineDriverDeviceInfo* const devInfo = CarlaEngine::getDriverDeviceInfo(index, name))
    {
        retDevInfo.hints       = devInfo->hints;
        retDevInfo.bufferSizes = devInfo->bufferSizes != nullptr ? devInfo->bufferSizes : nullBufferSizes;
        retDevInfo.sampleRates = devInfo->sampleRates != nullptr ? devInfo->sampleRates : nullSampleRates;
    }
    else
    {
        retDevInfo.hints       = 0x0;
        retDevInfo.bufferSizes = nullBufferSizes;
        retDevInfo.sampleRates = nullSampleRates;
    }

    return &retDevInfo;
}

bool carla_show_engine_driver_device_control_panel(uint index, const char* name)
{
    return CarlaEngine::showDriverDeviceControlPanel(index, name);
}

// --------------------------------------------------------------------------------------------------------------------

CarlaHostHandle carla_standalone_host_init(void)
{
#ifdef CARLA_OS_UNIX
    static const ThreadSafeFFTW sThreadSafeFFTW;
#endif

    static CarlaHostStandalone gStandalone;

    return &gStandalone;
}

CarlaEngine* carla_get_engine_from_handle(CarlaHostHandle handle)
{
    carla_debug("carla_get_engine(%p)", handle);

    return handle->engine;
}

// --------------------------------------------------------------------------------------------------------------------

static void carla_engine_init_common(const CarlaHostStandalone& standalone, CarlaEngine* const engine)
{
    engine->setCallback(standalone.engineCallback, standalone.engineCallbackPtr);
    engine->setFileCallback(standalone.fileCallback, standalone.fileCallbackPtr);

    using water::File;
    const File waterBinaryDir(File::getSpecialLocation(File::currentExecutableFile).getParentDirectory());

#ifdef BUILD_BRIDGE
    /*
    if (const char* const forceStereo = std::getenv("ENGINE_OPTION_FORCE_STEREO"))
        engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO, (std::strcmp(forceStereo, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const preferPluginBridges = std::getenv("ENGINE_OPTION_PREFER_PLUGIN_BRIDGES"))
        engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, (std::strcmp(preferPluginBridges, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const preferUiBridges = std::getenv("ENGINE_OPTION_PREFER_UI_BRIDGES"))
        engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES, (std::strcmp(preferUiBridges, "true") == 0) ? 1 : 0, nullptr);
    */

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_UIS_ALWAYS_ON_TOP"))
        engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const maxParameters = std::getenv("ENGINE_OPTION_MAX_PARAMETERS"))
        engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS, std::atoi(maxParameters), nullptr);

    if (const char* const resetXruns = std::getenv("ENGINE_OPTION_RESET_XRUNS"))
        engine->setOption(CB::ENGINE_OPTION_RESET_XRUNS, (std::strcmp(resetXruns, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const uiBridgesTimeout = std::getenv("ENGINE_OPTION_UI_BRIDGES_TIMEOUT"))
        engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT, std::atoi(uiBridgesTimeout), nullptr);

    if (const char* const pathAudio = std::getenv("ENGINE_OPTION_FILE_PATH_AUDIO"))
        engine->setOption(CB::ENGINE_OPTION_FILE_PATH, CB::FILE_AUDIO, pathAudio);

    if (const char* const pathMIDI = std::getenv("ENGINE_OPTION_FILE_PATH_MIDI"))
        engine->setOption(CB::ENGINE_OPTION_FILE_PATH, CB::FILE_MIDI, pathMIDI);

    if (const char* const pathLADSPA = std::getenv("ENGINE_OPTION_PLUGIN_PATH_LADSPA"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_LADSPA, pathLADSPA);

    if (const char* const pathDSSI = std::getenv("ENGINE_OPTION_PLUGIN_PATH_DSSI"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_DSSI, pathDSSI);

    if (const char* const pathLV2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_LV2"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_LV2, pathLV2);

    if (const char* const pathVST2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_VST2"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_VST2, pathVST2);

    if (const char* const pathVST3 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_VST3"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_VST3, pathVST3);

    if (const char* const pathSF2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_SF2"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_SF2, pathSF2);

    if (const char* const pathSFZ = std::getenv("ENGINE_OPTION_PLUGIN_PATH_SFZ"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_SFZ, pathSFZ);

    if (const char* const pathJSFX = std::getenv("ENGINE_OPTION_PLUGIN_PATH_JSFX"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_JSFX, pathJSFX);

    if (const char* const pathCLAP = std::getenv("ENGINE_OPTION_PLUGIN_PATH_CLAP"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_CLAP, pathCLAP);

    if (const char* const binaryDir = std::getenv("ENGINE_OPTION_PATH_BINARIES"))
        engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES, 0, binaryDir);
    else
        engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES, 0, waterBinaryDir.getFullPathName().toRawUTF8());

    if (const char* const resourceDir = std::getenv("ENGINE_OPTION_PATH_RESOURCES"))
        engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES, 0, resourceDir);
    else
        engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES, 0, waterBinaryDir.getChildFile("resources").getFullPathName().toRawUTF8());

    if (const char* const preventBadBehaviour = std::getenv("ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR"))
        engine->setOption(CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR, (std::strcmp(preventBadBehaviour, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const frontendWinId = std::getenv("ENGINE_OPTION_FRONTEND_WIN_ID"))
        engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, frontendWinId);
#else
    engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          standalone.engineOptions.forceStereo         ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, standalone.engineOptions.preferPluginBridges ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     standalone.engineOptions.preferUiBridges     ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     standalone.engineOptions.uisAlwaysOnTop      ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS,        static_cast<int>(standalone.engineOptions.maxParameters),    nullptr);
    engine->setOption(CB::ENGINE_OPTION_RESET_XRUNS,           standalone.engineOptions.resetXruns          ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    static_cast<int>(standalone.engineOptions.uiBridgesTimeout), nullptr);
    engine->setOption(CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE,     static_cast<int>(standalone.engineOptions.audioBufferSize),  nullptr);
    engine->setOption(CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE,     static_cast<int>(standalone.engineOptions.audioSampleRate),  nullptr);
    engine->setOption(CB::ENGINE_OPTION_AUDIO_TRIPLE_BUFFER,   standalone.engineOptions.audioTripleBuffer   ? 1 : 0,        nullptr);

    if (standalone.engineOptions.audioDriver != nullptr)
        engine->setOption(CB::ENGINE_OPTION_AUDIO_DRIVER,      0, standalone.engineOptions.audioDriver);

    if (standalone.engineOptions.audioDevice != nullptr)
        engine->setOption(CB::ENGINE_OPTION_AUDIO_DEVICE,      0, standalone.engineOptions.audioDevice);

    engine->setOption(CB::ENGINE_OPTION_OSC_ENABLED,  standalone.engineOptions.oscEnabled, nullptr);
    engine->setOption(CB::ENGINE_OPTION_OSC_PORT_TCP, standalone.engineOptions.oscPortTCP, nullptr);
    engine->setOption(CB::ENGINE_OPTION_OSC_PORT_UDP, standalone.engineOptions.oscPortUDP, nullptr);

    if (standalone.engineOptions.pathAudio != nullptr)
        engine->setOption(CB::ENGINE_OPTION_FILE_PATH, CB::FILE_AUDIO, standalone.engineOptions.pathAudio);

    if (standalone.engineOptions.pathMIDI != nullptr)
        engine->setOption(CB::ENGINE_OPTION_FILE_PATH, CB::FILE_MIDI, standalone.engineOptions.pathMIDI);

    if (standalone.engineOptions.pathLADSPA != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_LADSPA, standalone.engineOptions.pathLADSPA);

    if (standalone.engineOptions.pathDSSI != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_DSSI, standalone.engineOptions.pathDSSI);

    if (standalone.engineOptions.pathLV2 != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_LV2, standalone.engineOptions.pathLV2);

    if (standalone.engineOptions.pathVST2 != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_VST2, standalone.engineOptions.pathVST2);

    if (standalone.engineOptions.pathVST3 != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_VST3, standalone.engineOptions.pathVST3);

    if (standalone.engineOptions.pathSF2 != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_SF2, standalone.engineOptions.pathSF2);

    if (standalone.engineOptions.pathSFZ != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_SFZ, standalone.engineOptions.pathSFZ);

    if (standalone.engineOptions.pathJSFX != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_JSFX, standalone.engineOptions.pathJSFX);

    if (standalone.engineOptions.pathCLAP != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_CLAP, standalone.engineOptions.pathCLAP);

    if (standalone.engineOptions.binaryDir != nullptr && standalone.engineOptions.binaryDir[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES, 0, standalone.engineOptions.binaryDir);
    else
        engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES, 0, waterBinaryDir.getFullPathName().toRawUTF8());

    if (standalone.engineOptions.resourceDir != nullptr && standalone.engineOptions.resourceDir[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,    0, standalone.engineOptions.resourceDir);

    engine->setOption(CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR,    standalone.engineOptions.preventBadBehaviour ? 1 : 0,  nullptr);

    engine->setOption(CB::ENGINE_OPTION_FRONTEND_BACKGROUND_COLOR, static_cast<int>(standalone.engineOptions.bgColor), nullptr);
    engine->setOption(CB::ENGINE_OPTION_FRONTEND_FOREGROUND_COLOR, static_cast<int>(standalone.engineOptions.fgColor), nullptr);
    engine->setOption(CB::ENGINE_OPTION_FRONTEND_UI_SCALE, static_cast<int>(standalone.engineOptions.uiScale * 1000.0f), nullptr);

    if (standalone.engineOptions.frontendWinId != 0)
    {
        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';
        std::snprintf(strBuf, STR_MAX, P_UINTPTR, standalone.engineOptions.frontendWinId);
        engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, strBuf);
    }
    else
    {
        engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, "0");
    }

# ifndef CARLA_OS_WIN
    if (standalone.engineOptions.wine.executable != nullptr && standalone.engineOptions.wine.executable[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_WINE_EXECUTABLE, 0, standalone.engineOptions.wine.executable);

    engine->setOption(CB::ENGINE_OPTION_WINE_AUTO_PREFIX, standalone.engineOptions.wine.autoPrefix ? 1 : 0, nullptr);

    if (standalone.engineOptions.wine.fallbackPrefix != nullptr && standalone.engineOptions.wine.fallbackPrefix[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_WINE_FALLBACK_PREFIX, 0, standalone.engineOptions.wine.fallbackPrefix);

    engine->setOption(CB::ENGINE_OPTION_WINE_RT_PRIO_ENABLED, standalone.engineOptions.wine.rtPrio ? 1 : 0, nullptr);
    engine->setOption(CB::ENGINE_OPTION_WINE_BASE_RT_PRIO, standalone.engineOptions.wine.baseRtPrio, nullptr);
    engine->setOption(CB::ENGINE_OPTION_WINE_SERVER_RT_PRIO, standalone.engineOptions.wine.serverRtPrio, nullptr);
# endif

    engine->setOption(CB::ENGINE_OPTION_CLIENT_NAME_PREFIX, 0, standalone.engineOptions.clientNamePrefix);

    engine->setOption(CB::ENGINE_OPTION_PLUGINS_ARE_STANDALONE, standalone.engineOptions.pluginsAreStandalone, nullptr);
#endif // BUILD_BRIDGE
}

bool carla_engine_init(CarlaHostHandle handle, const char* driverName, const char* clientName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    carla_debug("carla_engine_init(%p, \"%s\", \"%s\")", handle, driverName, clientName);

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->isStandalone, "Must be a standalone host handle", false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine == nullptr, "Engine is already initialized", false);

#ifdef CARLA_OS_WIN
    carla_setenv("WINEASIO_CLIENT_NAME", clientName);
#endif

    CarlaHostStandalone& shandle((CarlaHostStandalone&)*handle);

    CarlaEngine* const engine = CarlaEngine::newDriverByName(driverName);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(engine != nullptr, "The selected audio driver is not available", false);

    shandle.engine = engine;

#ifdef BUILD_BRIDGE
    if (std::getenv("CARLA_BRIDGE_DUMMY") != nullptr)
    {
        // engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,      CB::ENGINE_PROCESS_MODE_PATCHBAY,         nullptr);
        engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,      CB::ENGINE_PROCESS_MODE_CONTINUOUS_RACK,  nullptr);
        engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,    CB::ENGINE_TRANSPORT_MODE_INTERNAL,       nullptr);

        engine->setOption(CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE, 4096, nullptr);
        engine->setOption(CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE, 48000, nullptr);
    }
    else
    {
        engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,      CB::ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, nullptr);
        engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,    CB::ENGINE_TRANSPORT_MODE_JACK,           nullptr);
    }
    engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          false,                                    nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, false,                                    nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     false,                                    nullptr);
#else
    engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          static_cast<int>(shandle.engineOptions.processMode),   nullptr);
    engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        static_cast<int>(shandle.engineOptions.transportMode), shandle.engineOptions.transportExtra);
#endif

    carla_engine_init_common(shandle, engine);

    if (engine->init(clientName))
    {
#ifdef CARLA_CAN_USE_LOG_THREAD
        if (shandle.logThreadEnabled && std::getenv("CARLA_LOGS_DISABLED") == nullptr)
            shandle.logThread.init();
#endif
        shandle.lastError = "No error";
        return true;
    }
    else
    {
        shandle.lastError = engine->getLastError();
        shandle.engine = nullptr;
        delete engine;
        return false;
    }
}

#ifdef BUILD_BRIDGE
bool carla_engine_init_bridge(CarlaHostHandle handle,
                              const char audioBaseName[6+1],
                              const char rtClientBaseName[6+1],
                              const char nonRtClientBaseName[6+1],
                              const char nonRtServerBaseName[6+1],
                              const char* const clientName)
{
    CARLA_SAFE_ASSERT_RETURN(audioBaseName != nullptr && audioBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(rtClientBaseName != nullptr && rtClientBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(nonRtClientBaseName != nullptr && nonRtClientBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(nonRtServerBaseName != nullptr && nonRtServerBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    carla_debug("carla_engine_init_bridge(%p, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")",
                handle, audioBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName, clientName);

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->isStandalone, "Must be a standalone host handle", false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine == nullptr, "Engine is already initialized", false);

    ScopedPointer<CarlaEngine> engine(CB::EngineInit::newBridge(audioBaseName,
                                                                rtClientBaseName,
                                                                nonRtClientBaseName,
                                                                nonRtServerBaseName));

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(engine != nullptr, "The selected audio driver is not available", false);

    engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,   CB::ENGINE_PROCESS_MODE_BRIDGE,   nullptr);
    engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE, CB::ENGINE_TRANSPORT_MODE_BRIDGE, nullptr);

    CarlaHostStandalone& shandle((CarlaHostStandalone&)*handle);

    carla_engine_init_common(shandle, engine);

    if (engine->init(clientName))
    {
        shandle.lastError = "No error";
        shandle.engine = engine.release();
        return true;
    }
    else
    {
        shandle.lastError = engine->getLastError();
        return false;
    }
}
#endif

bool carla_engine_close(CarlaHostHandle handle)
{
    carla_debug("carla_engine_close(%p)", handle);

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->isStandalone, "Must be a standalone host handle", false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    CarlaHostStandalone& shandle((CarlaHostStandalone&)*handle);

    CarlaEngine* const engine = shandle.engine;

    engine->setAboutToClose();
    engine->removeAllPlugins();

    const bool closed = engine->close();

    if (! closed)
        shandle.lastError = engine->getLastError();

#ifdef CARLA_CAN_USE_LOG_THREAD
    shandle.logThread.stop();
#endif

    shandle.engine = nullptr;
    delete engine;

    return closed;
}

void carla_engine_idle(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->isStandalone,);

    handle->engine->idle();
}

bool carla_is_engine_running(CarlaHostHandle handle)
{
    return (handle->engine != nullptr && handle->engine->isRunning());
}

const CarlaRuntimeEngineInfo* carla_get_runtime_engine_info(CarlaHostHandle handle)
{
    static CarlaRuntimeEngineInfo retInfo;

    // reset
    retInfo.load = 0.0f;
    retInfo.xruns = 0;

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retInfo);

    retInfo.load = handle->engine->getDSPLoad();
    retInfo.xruns = handle->engine->getTotalXruns();

    return &retInfo;
}

#ifndef BUILD_BRIDGE
const CarlaRuntimeEngineDriverDeviceInfo* carla_get_runtime_engine_driver_device_info(CarlaHostHandle handle)
{
    static CarlaRuntimeEngineDriverDeviceInfo retInfo;

    // reset
    retInfo.name = gNullCharPtr;
    retInfo.hints = 0x0;
    retInfo.bufferSize = 0;
    retInfo.bufferSizes = nullptr;
    retInfo.sampleRate = 0.0;
    retInfo.sampleRates = nullptr;

    const char* audioDriver;
    const char* audioDevice;

    if (CarlaEngine* const engine = handle->engine)
    {
        audioDriver = engine->getCurrentDriverName();
        audioDevice = engine->getOptions().audioDevice;

        retInfo.bufferSize = engine->getBufferSize();
        retInfo.sampleRate = engine->getSampleRate();
    }
    else if (handle->isStandalone)
    {
        CarlaHostStandalone& shandle((CarlaHostStandalone&)*handle);

        audioDriver = shandle.engineOptions.audioDriver;
        audioDevice = shandle.engineOptions.audioDevice;

        retInfo.bufferSize = shandle.engineOptions.audioBufferSize;
        retInfo.sampleRate = shandle.engineOptions.audioSampleRate;
    }
    else
    {
        return &retInfo;
    }
    CARLA_SAFE_ASSERT_RETURN(audioDriver != nullptr, &retInfo);
    CARLA_SAFE_ASSERT_RETURN(audioDevice != nullptr, &retInfo);

    uint index = 0;
    uint count = CarlaEngine::getDriverCount();
    for (; index<count; ++index)
    {
        const char* const testDriverName = CarlaEngine::getDriverName(index);
        CARLA_SAFE_ASSERT_CONTINUE(testDriverName != nullptr);

        if (std::strcmp(testDriverName, audioDriver) == 0)
            break;
    }
    CARLA_SAFE_ASSERT_RETURN(index != count, &retInfo);

    const EngineDriverDeviceInfo* const devInfo = CarlaEngine::getDriverDeviceInfo(index, audioDevice);
    CARLA_SAFE_ASSERT_RETURN(devInfo != nullptr, &retInfo);

    retInfo.name        = audioDevice;
    retInfo.hints       = devInfo->hints;
    retInfo.bufferSizes = devInfo->bufferSizes;
    retInfo.sampleRates = devInfo->sampleRates;

    return &retInfo;
}

bool carla_set_engine_buffer_size_and_sample_rate(CarlaHostHandle handle, uint bufferSize, double sampleRate)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, false);
    carla_debug("carla_set_engine_buffer_size_and_sample_rate(%p, %u, %f)", handle, bufferSize, sampleRate);

    return handle->engine->setBufferSizeAndSampleRate(bufferSize, sampleRate);
}

bool carla_show_engine_device_control_panel(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, false);
    carla_debug("carla_show_engine_device_control_panel(%p)", handle);

    return handle->engine->showDeviceControlPanel();
}
#endif // BUILD_BRIDGE

void carla_clear_engine_xruns(CarlaHostHandle handle)
{
    if (handle->engine != nullptr)
        handle->engine->clearXruns();
}

void carla_cancel_engine_action(CarlaHostHandle handle)
{
    if (handle->engine != nullptr)
        handle->engine->setActionCanceled(true);
}

bool carla_set_engine_about_to_close(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, true);
    carla_debug("carla_set_engine_about_to_close(%p)", handle);

    return handle->engine->setAboutToClose();
}

void carla_set_engine_callback(CarlaHostHandle handle, EngineCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_engine_callback(%p, %p, %p)", handle, func, ptr);

    if (handle->isStandalone)
    {
        CarlaHostStandalone& shandle((CarlaHostStandalone&)*handle);

        shandle.engineCallback    = func;
        shandle.engineCallbackPtr = ptr;

#ifdef CARLA_CAN_USE_LOG_THREAD
        shandle.logThread.setCallback(func, ptr);
#endif
    }

    if (handle->engine != nullptr)
        handle->engine->setCallback(func, ptr);
}

void carla_set_engine_option(CarlaHostHandle handle, EngineOption option, int value, const char* valueStr)
{
    carla_debug("carla_set_engine_option(%p, %i:%s, %i, \"%s\")",
                handle, option, CB::EngineOption2Str(option), value, valueStr);

    if (handle->isStandalone)
    {
        CarlaHostStandalone& shandle((CarlaHostStandalone&)*handle);

        switch (option)
        {
        case CB::ENGINE_OPTION_DEBUG:
            break;

        case CB::ENGINE_OPTION_PROCESS_MODE:
            CARLA_SAFE_ASSERT_RETURN(value >= CB::ENGINE_PROCESS_MODE_SINGLE_CLIENT && value < CB::ENGINE_PROCESS_MODE_BRIDGE,);
            shandle.engineOptions.processMode = static_cast<CB::EngineProcessMode>(value);
            break;

        case CB::ENGINE_OPTION_TRANSPORT_MODE:
            CARLA_SAFE_ASSERT_RETURN(value >= CB::ENGINE_TRANSPORT_MODE_DISABLED && value <= CB::ENGINE_TRANSPORT_MODE_BRIDGE,);

            // jack transport cannot be disabled in multi-client
            if (shandle.engineOptions.processMode == CB::ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS
                && value != CB::ENGINE_TRANSPORT_MODE_JACK)
            {
                shandle.engineOptions.transportMode = CB::ENGINE_TRANSPORT_MODE_JACK;

                if (shandle.engineCallback != nullptr)
                    shandle.engineCallback(shandle.engineCallbackPtr,
                                           CB::ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED,
                                           0,
                                           CB::ENGINE_TRANSPORT_MODE_JACK,
                                           0, 0, 0.0f,
                                           shandle.engineOptions.transportExtra);
            }
            else
            {
                shandle.engineOptions.transportMode = static_cast<CB::EngineTransportMode>(value);
            }

            delete[] shandle.engineOptions.transportExtra;
            if (value != CB::ENGINE_TRANSPORT_MODE_DISABLED && valueStr != nullptr)
                shandle.engineOptions.transportExtra = carla_strdup_safe(valueStr);
            else
                shandle.engineOptions.transportExtra = nullptr;
            break;

        case CB::ENGINE_OPTION_FORCE_STEREO:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.forceStereo = (value != 0);
            break;

        case CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.preferPluginBridges = (value != 0);
            break;

        case CB::ENGINE_OPTION_PREFER_UI_BRIDGES:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.preferUiBridges = (value != 0);
            break;

        case CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.uisAlwaysOnTop = (value != 0);
            break;

        case CB::ENGINE_OPTION_MAX_PARAMETERS:
            CARLA_SAFE_ASSERT_RETURN(value >= 0,);
            shandle.engineOptions.maxParameters = static_cast<uint>(value);
            break;

        case CB::ENGINE_OPTION_RESET_XRUNS:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.resetXruns = (value != 0);
            break;

        case CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT:
            CARLA_SAFE_ASSERT_RETURN(value >= 0,);
            shandle.engineOptions.uiBridgesTimeout = static_cast<uint>(value);
            break;

        case CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE:
            CARLA_SAFE_ASSERT_RETURN(value >= 8,);
            shandle.engineOptions.audioBufferSize = static_cast<uint>(value);
            break;

        case CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE:
            CARLA_SAFE_ASSERT_RETURN(value >= 22050,);
            shandle.engineOptions.audioSampleRate = static_cast<uint>(value);
            break;

        case CB::ENGINE_OPTION_AUDIO_TRIPLE_BUFFER:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.audioTripleBuffer = (value != 0);
            break;

        case CB::ENGINE_OPTION_AUDIO_DRIVER:
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

            if (shandle.engineOptions.audioDriver != nullptr)
                delete[] shandle.engineOptions.audioDriver;

            shandle.engineOptions.audioDriver = carla_strdup_safe(valueStr);
            break;

        case CB::ENGINE_OPTION_AUDIO_DEVICE:
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

            if (shandle.engineOptions.audioDevice != nullptr)
                delete[] shandle.engineOptions.audioDevice;

            shandle.engineOptions.audioDevice = carla_strdup_safe(valueStr);
            break;

#ifndef BUILD_BRIDGE
        case CB::ENGINE_OPTION_OSC_ENABLED:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.oscEnabled = (value != 0);
            break;

        case CB::ENGINE_OPTION_OSC_PORT_TCP:
            CARLA_SAFE_ASSERT_RETURN(value <= 0 || value >= 1024,);
            shandle.engineOptions.oscPortTCP = value;
            break;

        case CB::ENGINE_OPTION_OSC_PORT_UDP:
            CARLA_SAFE_ASSERT_RETURN(value <= 0 || value >= 1024,);
            shandle.engineOptions.oscPortUDP = value;
            break;
#endif

        case CB::ENGINE_OPTION_FILE_PATH:
            CARLA_SAFE_ASSERT_RETURN(value > CB::FILE_NONE,);
            CARLA_SAFE_ASSERT_RETURN(value <= CB::FILE_MIDI,);
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

            switch (value)
            {
            case CB::FILE_AUDIO:
                if (shandle.engineOptions.pathAudio != nullptr)
                    delete[] shandle.engineOptions.pathAudio;
                shandle.engineOptions.pathAudio = carla_strdup_safe(valueStr);
                break;
            case CB::FILE_MIDI:
                if (shandle.engineOptions.pathMIDI != nullptr)
                    delete[] shandle.engineOptions.pathMIDI;
                shandle.engineOptions.pathMIDI = carla_strdup_safe(valueStr);
                break;
            }
            break;

        case CB::ENGINE_OPTION_PLUGIN_PATH:
            CARLA_SAFE_ASSERT_RETURN(value > CB::PLUGIN_NONE,);
            CARLA_SAFE_ASSERT_RETURN(value <= CB::PLUGIN_TYPE_COUNT,);
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

            switch (value)
            {
            case CB::PLUGIN_LADSPA:
                if (shandle.engineOptions.pathLADSPA != nullptr)
                    delete[] shandle.engineOptions.pathLADSPA;
                shandle.engineOptions.pathLADSPA = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_DSSI:
                if (shandle.engineOptions.pathDSSI != nullptr)
                    delete[] shandle.engineOptions.pathDSSI;
                shandle.engineOptions.pathDSSI = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_LV2:
                if (shandle.engineOptions.pathLV2 != nullptr)
                    delete[] shandle.engineOptions.pathLV2;
                shandle.engineOptions.pathLV2 = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_VST2:
                if (shandle.engineOptions.pathVST2 != nullptr)
                    delete[] shandle.engineOptions.pathVST2;
                shandle.engineOptions.pathVST2 = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_VST3:
                if (shandle.engineOptions.pathVST3 != nullptr)
                    delete[] shandle.engineOptions.pathVST3;
                shandle.engineOptions.pathVST3 = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_SF2:
                if (shandle.engineOptions.pathSF2 != nullptr)
                    delete[] shandle.engineOptions.pathSF2;
                shandle.engineOptions.pathSF2 = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_SFZ:
                if (shandle.engineOptions.pathSFZ != nullptr)
                    delete[] shandle.engineOptions.pathSFZ;
                shandle.engineOptions.pathSFZ = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_JSFX:
                if (shandle.engineOptions.pathJSFX != nullptr)
                    delete[] shandle.engineOptions.pathJSFX;
                shandle.engineOptions.pathJSFX = carla_strdup_safe(valueStr);
                break;
            case CB::PLUGIN_CLAP:
                if (shandle.engineOptions.pathCLAP != nullptr)
                    delete[] shandle.engineOptions.pathCLAP;
                shandle.engineOptions.pathCLAP = carla_strdup_safe(valueStr);
                break;
            }
            break;

        case CB::ENGINE_OPTION_PATH_BINARIES:
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

            if (shandle.engineOptions.binaryDir != nullptr)
                delete[] shandle.engineOptions.binaryDir;

            shandle.engineOptions.binaryDir = carla_strdup_safe(valueStr);
            break;

        case CB::ENGINE_OPTION_PATH_RESOURCES:
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

            if (shandle.engineOptions.resourceDir != nullptr)
                delete[] shandle.engineOptions.resourceDir;

            shandle.engineOptions.resourceDir = carla_strdup_safe(valueStr);
            break;

        case CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.preventBadBehaviour = (value != 0);
            break;

        case CB::ENGINE_OPTION_FRONTEND_BACKGROUND_COLOR:
            shandle.engineOptions.bgColor = static_cast<uint>(value);
            break;

        case CB::ENGINE_OPTION_FRONTEND_FOREGROUND_COLOR:
            shandle.engineOptions.fgColor = static_cast<uint>(value);
            break;

        case CB::ENGINE_OPTION_FRONTEND_UI_SCALE:
            CARLA_SAFE_ASSERT_RETURN(value > 0,);
            shandle.engineOptions.uiScale = static_cast<float>(value) / 1000;
            break;

        case CB::ENGINE_OPTION_FRONTEND_WIN_ID: {
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);
            const long long winId(std::strtoll(valueStr, nullptr, 16));
            CARLA_SAFE_ASSERT_RETURN(winId >= 0,);
            shandle.engineOptions.frontendWinId = static_cast<uintptr_t>(winId);
        }   break;

#if !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) && !defined(CARLA_OS_WIN)
        case CB::ENGINE_OPTION_WINE_EXECUTABLE:
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

            if (shandle.engineOptions.wine.executable != nullptr)
                delete[] shandle.engineOptions.wine.executable;

            shandle.engineOptions.wine.executable = carla_strdup_safe(valueStr);
            break;

        case CB::ENGINE_OPTION_WINE_AUTO_PREFIX:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.wine.autoPrefix = (value != 0);
            break;

        case CB::ENGINE_OPTION_WINE_FALLBACK_PREFIX:
            CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

            if (shandle.engineOptions.wine.fallbackPrefix != nullptr)
                delete[] shandle.engineOptions.wine.fallbackPrefix;

            shandle.engineOptions.wine.fallbackPrefix = carla_strdup_safe(valueStr);
            break;

        case CB::ENGINE_OPTION_WINE_RT_PRIO_ENABLED:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.wine.rtPrio = (value != 0);
            break;

        case CB::ENGINE_OPTION_WINE_BASE_RT_PRIO:
            CARLA_SAFE_ASSERT_RETURN(value >= 1 && value <= 89,);
            shandle.engineOptions.wine.baseRtPrio = value;
            break;

        case CB::ENGINE_OPTION_WINE_SERVER_RT_PRIO:
            CARLA_SAFE_ASSERT_RETURN(value >= 1 && value <= 99,);
            shandle.engineOptions.wine.serverRtPrio = value;
            break;
#endif

#ifndef BUILD_BRIDGE
        case CB::ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT:
#ifdef CARLA_CAN_USE_LOG_THREAD
            shandle.logThreadEnabled = (value != 0);
#endif
            break;
#endif

        case CB::ENGINE_OPTION_CLIENT_NAME_PREFIX:
            if (shandle.engineOptions.clientNamePrefix != nullptr)
                delete[] shandle.engineOptions.clientNamePrefix;

            shandle.engineOptions.clientNamePrefix = valueStr != nullptr && valueStr[0] != '\0'
                                                   ? carla_strdup_safe(valueStr)
                                                   : nullptr;
            break;

        case CB::ENGINE_OPTION_PLUGINS_ARE_STANDALONE:
            CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
            shandle.engineOptions.pluginsAreStandalone = (value != 0);
            break;
        }
    }

    if (handle->engine != nullptr)
        handle->engine->setOption(option, value, valueStr);
}

void carla_set_file_callback(CarlaHostHandle handle, FileCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_file_callback(%p, %p, %p)", handle, func, ptr);

    if (handle->isStandalone)
    {
        CarlaHostStandalone& shandle((CarlaHostStandalone&)*handle);

        shandle.fileCallback    = func;
        shandle.fileCallbackPtr = ptr;
    }

    if (handle->engine != nullptr)
        handle->engine->setFileCallback(func, ptr);
}

// --------------------------------------------------------------------------------------------------------------------

bool carla_load_file(CarlaHostHandle handle, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_load_file(%p, \"%s\")", handle, filename);

    return handle->engine->loadFile(filename);
}

bool carla_load_project(CarlaHostHandle handle, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_load_project(%p, \"%s\")", handle, filename);

    return handle->engine->loadProject(filename, true);
}

bool carla_save_project(CarlaHostHandle handle, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_save_project(%p, \"%s\")", handle, filename);

    return handle->engine->saveProject(filename, true);
}

#ifndef BUILD_BRIDGE
const char* carla_get_current_project_folder(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, gNullCharPtr);

    carla_debug("carla_get_current_project_folder(%p)", handle);

    if (const char* const ret = handle->engine->getCurrentProjectFolder())
        return ret;

    return gNullCharPtr;
}

const char* carla_get_current_project_filename(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->isStandalone, gNullCharPtr);

    carla_debug("carla_get_current_project_filename(%p)", handle);

    if (const char* const ret = handle->engine->getCurrentProjectFilename())
        return ret;

    return gNullCharPtr;
}

void carla_clear_project_filename(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    carla_debug("carla_clear_project_filename(%p)", handle);

    handle->engine->clearCurrentProjectFilename();
}

// --------------------------------------------------------------------------------------------------------------------

bool carla_patchbay_connect(CarlaHostHandle handle, bool external, uint groupIdA, uint portIdA, uint groupIdB, uint portIdB)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_patchbay_connect(%p, %s, %u, %u, %u, %u)",
                handle, bool2str(external), groupIdA, portIdA, groupIdB, portIdB);

    return handle->engine->patchbayConnect(external, groupIdA, portIdA, groupIdB, portIdB);
}

bool carla_patchbay_disconnect(CarlaHostHandle handle, bool external, uint connectionId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_patchbay_disconnect(%p, %s, %i)", handle, bool2str(external), connectionId);

    return handle->engine->patchbayDisconnect(external, connectionId);
}

bool carla_patchbay_set_group_pos(CarlaHostHandle handle, bool external, uint groupId, int x1, int y1, int x2, int y2)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr && handle->engine->isRunning(),
                                             "Engine is not running", false);

    carla_debug("carla_patchbay_set_group_pos(%p, %s, %u, %i, %i, %i, %i)",
                handle, bool2str(external), groupId, x1, y1, x2, y2);

    if (handle->engine->isAboutToClose())
        return true;

    return handle->engine->patchbaySetGroupPos(false, true, external, groupId, x1, y1, x2, y2);
}

bool carla_patchbay_refresh(CarlaHostHandle handle, bool external)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_patchbay_refresh(%p, %s)", handle, bool2str(external));

    return handle->engine->patchbayRefresh(true, false, external);
}

// --------------------------------------------------------------------------------------------------------------------

void carla_transport_play(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(),);

    carla_debug("carla_transport_play(%p)", handle);

    handle->engine->transportPlay();
}

void carla_transport_pause(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(),);

    carla_debug("carla_transport_pause(%p)", handle);

    handle->engine->transportPause();
}

void carla_transport_bpm(CarlaHostHandle handle, double bpm)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(),);

    carla_debug("carla_transport_bpm(%p, %f)", handle, bpm);

    handle->engine->transportBPM(bpm);
}

void carla_transport_relocate(CarlaHostHandle handle, uint64_t frame)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(),);

    carla_debug("carla_transport_relocate(%p, %i)", handle, frame);

    handle->engine->transportRelocate(frame);
}

uint64_t carla_get_current_transport_frame(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(), 0);

    return handle->engine->getTimeInfo().frame;
}

const CarlaTransportInfo* carla_get_transport_info(CarlaHostHandle handle)
{
    static CarlaTransportInfo retTransInfo;
    retTransInfo.clear();

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(), &retTransInfo);

    const CB::EngineTimeInfo& timeInfo(handle->engine->getTimeInfo());

    retTransInfo.playing = timeInfo.playing;
    retTransInfo.frame   = timeInfo.frame;

    if (timeInfo.bbt.valid)
    {
        retTransInfo.bar  = timeInfo.bbt.bar;
        retTransInfo.beat = timeInfo.bbt.beat;
        retTransInfo.tick = static_cast<int32_t>(timeInfo.bbt.tick + 0.5);
        retTransInfo.bpm  = timeInfo.bbt.beatsPerMinute;
    }

    return &retTransInfo;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_current_plugin_count(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    // too noisy!
    // carla_debug("carla_get_current_plugin_count(%p)", handle);

    return handle->engine->getCurrentPluginCount();
}

uint32_t carla_get_max_plugin_number(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    carla_debug("carla_get_max_plugin_number(%p)", handle);

    return handle->engine->getMaxPluginNumber();
}

// --------------------------------------------------------------------------------------------------------------------

bool carla_add_plugin(CarlaHostHandle handle,
                      BinaryType btype, PluginType ptype,
                      const char* filename, const char* name, const char* label, int64_t uniqueId,
                      const void* extraPtr, uint options)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_add_plugin(%p, %i:%s, %i:%s, \"%s\", \"%s\", \"%s\", " P_INT64 ", %p, %u)",
                handle,
                btype, CB::BinaryType2Str(btype),
                ptype, CB::PluginType2Str(ptype),
                filename, name, label, uniqueId, extraPtr, options);

    return handle->engine->addPlugin(btype, ptype, filename, name, label, uniqueId, extraPtr, options);
}

bool carla_remove_plugin(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_remove_plugin(%p, %i)", handle, pluginId);

    return handle->engine->removePlugin(pluginId);
}

bool carla_remove_all_plugins(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_remove_all_plugins(%p)", handle);

    return handle->engine->removeAllPlugins();
}

#ifndef BUILD_BRIDGE
bool carla_rename_plugin(CarlaHostHandle handle, uint pluginId, const char* newName)
{
    CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_rename_plugin(%p, %i, \"%s\")", handle, pluginId, newName);

    return handle->engine->renamePlugin(pluginId, newName);
}

bool carla_clone_plugin(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_clone_plugin(%p, %i)", handle, pluginId);

    return handle->engine->clonePlugin(pluginId);
}

bool carla_replace_plugin(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_replace_plugin(%p, %i)", handle, pluginId);

    return handle->engine->replacePlugin(pluginId);
}

bool carla_switch_plugins(CarlaHostHandle handle, uint pluginIdA, uint pluginIdB)
{
    CARLA_SAFE_ASSERT_RETURN(pluginIdA != pluginIdB, false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_switch_plugins(%p, %i, %i)", handle, pluginIdA, pluginIdB);

    return handle->engine->switchPlugins(pluginIdA, pluginIdB);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

bool carla_load_plugin_state(CarlaHostHandle handle, uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr
                                          && handle->engine->isRunning(), "Engine is not running", false);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->loadStateFromFile(filename);

    return false;
}

bool carla_save_plugin_state(CarlaHostHandle handle, uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->saveStateToFile(filename);

    return false;
}

#ifndef CARLA_PLUGIN_ONLY_BRIDGE
bool carla_export_plugin_lv2(CarlaHostHandle handle, uint pluginId, const char* lv2path)
{
    CARLA_SAFE_ASSERT_RETURN(lv2path != nullptr && lv2path[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(handle->engine != nullptr, "Engine is not initialized", false);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->exportAsLV2(lv2path);

    return false;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

const CarlaPluginInfo* carla_get_plugin_info(CarlaHostHandle handle, uint pluginId)
{
    static CarlaPluginInfo retInfo;

    // reset
    retInfo.type             = CB::PLUGIN_NONE;
    retInfo.category         = CB::PLUGIN_CATEGORY_NONE;
    retInfo.hints            = 0x0;
    retInfo.optionsAvailable = 0x0;
    retInfo.optionsEnabled   = 0x0;
    retInfo.filename         = gNullCharPtr;
    retInfo.name             = gNullCharPtr;
    retInfo.iconName         = gNullCharPtr;
    retInfo.uniqueId         = 0;

    // cleanup
    if (retInfo.label != gNullCharPtr)
    {
        delete[] retInfo.label;
        retInfo.label = gNullCharPtr;
    }

    if (retInfo.maker != gNullCharPtr)
    {
        delete[] retInfo.maker;
        retInfo.maker = gNullCharPtr;
    }

    if (retInfo.copyright != gNullCharPtr)
    {
        delete[] retInfo.copyright;
        retInfo.copyright = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retInfo);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        char strBuf[STR_MAX+1];
        carla_zeroChars(strBuf, STR_MAX+1);

        retInfo.type     = plugin->getType();
        retInfo.category = plugin->getCategory();
        retInfo.hints    = plugin->getHints();
        retInfo.filename = plugin->getFilename();
        retInfo.name     = plugin->getName();
        retInfo.iconName = plugin->getIconName();
        retInfo.uniqueId = plugin->getUniqueId();

        retInfo.optionsAvailable = plugin->getOptionsAvailable();
        retInfo.optionsEnabled   = plugin->getOptionsEnabled();

        if (plugin->getLabel(strBuf))
            retInfo.label = carla_strdup_safe(strBuf);
        if (plugin->getMaker(strBuf))
            retInfo.maker = carla_strdup_safe(strBuf);
        if (plugin->getCopyright(strBuf))
            retInfo.copyright = carla_strdup_safe(strBuf);

        checkStringPtr(retInfo.filename);
        checkStringPtr(retInfo.name);
        checkStringPtr(retInfo.iconName);
        checkStringPtr(retInfo.label);
        checkStringPtr(retInfo.maker);
        checkStringPtr(retInfo.copyright);
    }

    return &retInfo;
}

const CarlaPortCountInfo* carla_get_audio_port_count_info(CarlaHostHandle handle, uint pluginId)
{
    static CarlaPortCountInfo retInfo;
    carla_zeroStruct(retInfo);

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retInfo);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        retInfo.ins   = plugin->getAudioInCount();
        retInfo.outs  = plugin->getAudioOutCount();
    }

    return &retInfo;
}

const CarlaPortCountInfo* carla_get_midi_port_count_info(CarlaHostHandle handle, uint pluginId)
{
    static CarlaPortCountInfo retInfo;
    carla_zeroStruct(retInfo);

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retInfo);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        retInfo.ins   = plugin->getMidiInCount();
        retInfo.outs  = plugin->getMidiOutCount();
    }

    return &retInfo;
}

const CarlaPortCountInfo* carla_get_parameter_count_info(CarlaHostHandle handle, uint pluginId)
{
    static CarlaPortCountInfo retInfo;
    carla_zeroStruct(retInfo);

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retInfo);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->getParameterCountInfo(retInfo.ins, retInfo.outs);

    return &retInfo;
}

const CarlaParameterInfo* carla_get_parameter_info(CarlaHostHandle handle, uint pluginId, uint32_t parameterId)
{
    static CarlaParameterInfo retInfo;

    // reset
    retInfo.scalePointCount = 0;

    // cleanup
    if (retInfo.name != gNullCharPtr)
    {
        delete[] retInfo.name;
        retInfo.name = gNullCharPtr;
    }

    if (retInfo.symbol != gNullCharPtr)
    {
        delete[] retInfo.symbol;
        retInfo.symbol = gNullCharPtr;
    }

    if (retInfo.unit != gNullCharPtr)
    {
        delete[] retInfo.unit;
        retInfo.unit = gNullCharPtr;
    }

    if (retInfo.comment != gNullCharPtr)
    {
        delete[] retInfo.comment;
        retInfo.comment = gNullCharPtr;
    }

    if (retInfo.groupName != gNullCharPtr)
    {
        delete[] retInfo.groupName;
        retInfo.groupName = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retInfo);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        char strBuf[STR_MAX+1];
        carla_zeroChars(strBuf, STR_MAX+1);

        retInfo.scalePointCount = plugin->getParameterScalePointCount(parameterId);

        if (plugin->getParameterName(parameterId, strBuf))
        {
            retInfo.name = carla_strdup_safe(strBuf);
            carla_zeroChars(strBuf, STR_MAX+1);
        }

        if (plugin->getParameterSymbol(parameterId, strBuf))
        {
            retInfo.symbol = carla_strdup_safe(strBuf);
            carla_zeroChars(strBuf, STR_MAX+1);
        }

        if (plugin->getParameterUnit(parameterId, strBuf))
        {
            retInfo.unit = carla_strdup_safe(strBuf);
            carla_zeroChars(strBuf, STR_MAX+1);
        }

        if (plugin->getParameterComment(parameterId, strBuf))
        {
            retInfo.comment = carla_strdup_safe(strBuf);
            carla_zeroChars(strBuf, STR_MAX+1);
        }

        if (plugin->getParameterGroupName(parameterId, strBuf))
        {
            retInfo.groupName = carla_strdup_safe(strBuf);
            carla_zeroChars(strBuf, STR_MAX+1);
        }

        checkStringPtr(retInfo.name);
        checkStringPtr(retInfo.symbol);
        checkStringPtr(retInfo.unit);
        checkStringPtr(retInfo.comment);
        checkStringPtr(retInfo.groupName);
    }

    return &retInfo;
}

const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(CarlaHostHandle handle,
                                                               uint pluginId,
                                                               uint32_t parameterId,
                                                               uint32_t scalePointId)
{
    CARLA_ASSERT(handle->engine != nullptr);

    static CarlaScalePointInfo retInfo;

    // reset
    retInfo.value = 0.0f;

    // cleanup
    if (retInfo.label != gNullCharPtr)
    {
        delete[] retInfo.label;
        retInfo.label = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retInfo);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        char strBuf[STR_MAX+1];

        retInfo.value = plugin->getParameterScalePointValue(parameterId, scalePointId);

        carla_zeroChars(strBuf, STR_MAX+1);
        if (plugin->getParameterScalePointLabel(parameterId, scalePointId, strBuf))
            retInfo.label = carla_strdup_safe(strBuf);

        checkStringPtr(retInfo.label);
    }

    return &retInfo;
}

// --------------------------------------------------------------------------------------------------------------------

uint carla_get_audio_port_hints(CarlaHostHandle handle, uint pluginId, bool isOutput, uint32_t portIndex)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0x0);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(portIndex < (isOutput ? plugin->getAudioOutCount() : plugin->getAudioInCount()), 0x0);

        return plugin->getAudioPortHints(isOutput, portIndex);
    }

    return 0x0;
}

// --------------------------------------------------------------------------------------------------------------------

const ParameterData* carla_get_parameter_data(CarlaHostHandle handle, uint pluginId, uint32_t parameterId)
{
    static ParameterData retParamData;

    // reset
    retParamData.type        = CB::PARAMETER_UNKNOWN;
    retParamData.hints       = 0x0;
    retParamData.index       = CB::PARAMETER_NULL;
    retParamData.rindex      = -1;
    retParamData.midiChannel = 0;
    retParamData.mappedControlIndex = CB::CONTROL_INDEX_NONE;
    retParamData.mappedMinimum = 0.0f;
    retParamData.mappedMaximum = 0.0f;

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retParamData);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), &retParamData);

        const ParameterData& pluginParamData(plugin->getParameterData(parameterId));
        retParamData.type        = pluginParamData.type;
        retParamData.hints       = pluginParamData.hints;
        retParamData.index       = pluginParamData.index;
        retParamData.rindex      = pluginParamData.rindex;
        retParamData.midiChannel = pluginParamData.midiChannel;
        retParamData.mappedControlIndex = pluginParamData.mappedControlIndex;
        retParamData.mappedMinimum = pluginParamData.mappedMinimum;
        retParamData.mappedMaximum = pluginParamData.mappedMaximum;
    }

    return &retParamData;
}

const ParameterRanges* carla_get_parameter_ranges(CarlaHostHandle handle, uint pluginId, uint32_t parameterId)
{
    static ParameterRanges retParamRanges;

    // reset
    retParamRanges.def       = 0.0f;
    retParamRanges.min       = 0.0f;
    retParamRanges.max       = 1.0f;
    retParamRanges.step      = 0.01f;
    retParamRanges.stepSmall = 0.0001f;
    retParamRanges.stepLarge = 0.1f;

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retParamRanges);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), &retParamRanges);

        const ParameterRanges& pluginParamRanges(plugin->getParameterRanges(parameterId));
        retParamRanges.def       = pluginParamRanges.def;
        retParamRanges.min       = pluginParamRanges.min;
        retParamRanges.max       = pluginParamRanges.max;
        retParamRanges.step      = pluginParamRanges.step;
        retParamRanges.stepSmall = pluginParamRanges.stepSmall;
        retParamRanges.stepLarge = pluginParamRanges.stepLarge;
    }

    return &retParamRanges;
}

const MidiProgramData* carla_get_midi_program_data(CarlaHostHandle handle, uint pluginId, uint32_t midiProgramId)
{
    static MidiProgramData retMidiProgData = { 0, 0, gNullCharPtr };

    // reset
    retMidiProgData.bank    = 0;
    retMidiProgData.program = 0;

    if (retMidiProgData.name != gNullCharPtr)
    {
        delete[] retMidiProgData.name;
        retMidiProgData.name = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retMidiProgData);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(midiProgramId < plugin->getMidiProgramCount(), &retMidiProgData);

        const MidiProgramData& pluginMidiProgData(plugin->getMidiProgramData(midiProgramId));
        retMidiProgData.bank    = pluginMidiProgData.bank;
        retMidiProgData.program = pluginMidiProgData.program;

        if (pluginMidiProgData.name != nullptr)
        {
            retMidiProgData.name = carla_strdup_safe(pluginMidiProgData.name);
            checkStringPtr(retMidiProgData.name);
        }
        else
        {
            retMidiProgData.name = gNullCharPtr;
        }
    }

    return &retMidiProgData;
}

const CustomData* carla_get_custom_data(CarlaHostHandle handle, uint pluginId, uint32_t customDataId)
{
    static CustomData retCustomData = { gNullCharPtr, gNullCharPtr, gNullCharPtr };

    // reset
    if (retCustomData.type != gNullCharPtr)
    {
        delete[] retCustomData.type;
        retCustomData.type = gNullCharPtr;
    }

    if (retCustomData.key != gNullCharPtr)
    {
        delete[] retCustomData.key;
        retCustomData.key = gNullCharPtr;
    }

    if (retCustomData.value != gNullCharPtr)
    {
        delete[] retCustomData.value;
        retCustomData.value = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, &retCustomData);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(customDataId < plugin->getCustomDataCount(), &retCustomData)

        const CustomData& pluginCustomData(plugin->getCustomData(customDataId));
        retCustomData.type  = carla_strdup_safe(pluginCustomData.type);
        retCustomData.key   = carla_strdup_safe(pluginCustomData.key);
        retCustomData.value = carla_strdup_safe(pluginCustomData.value);
        checkStringPtr(retCustomData.type);
        checkStringPtr(retCustomData.key);
        checkStringPtr(retCustomData.value);
    }

    return &retCustomData;
}

const char* carla_get_custom_data_value(CarlaHostHandle handle, uint pluginId, const char* type, const char* key)
{
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0', gNullCharPtr);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', gNullCharPtr);
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, gNullCharPtr);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        const uint32_t count = plugin->getCustomDataCount();

        if (count == 0)
            return gNullCharPtr;

        static String customDataValue;

        for (uint32_t i=0; i<count; ++i)
        {
            const CustomData& pluginCustomData(plugin->getCustomData(i));

            if (std::strcmp(pluginCustomData.type, type) != 0)
                continue;
            if (std::strcmp(pluginCustomData.key, key) != 0)
                continue;

            customDataValue = pluginCustomData.value;
            return customDataValue.buffer();
        }
    }

    return gNullCharPtr;
}

const char* carla_get_chunk_data(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, gNullCharPtr);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS, gNullCharPtr);

        void* data = nullptr;
        const std::size_t dataSize(plugin->getChunkData(&data));
        CARLA_SAFE_ASSERT_RETURN(data != nullptr && dataSize > 0, gNullCharPtr);

        static String chunkData;

        chunkData = String::asBase64(data, static_cast<std::size_t>(dataSize));
        return chunkData.buffer();
    }

    return gNullCharPtr;
}

// --------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_parameter_count(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->getParameterCount();

    return 0;
}

uint32_t carla_get_program_count(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->getProgramCount();

    return 0;
}

uint32_t carla_get_midi_program_count(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->getMidiProgramCount();

    return 0;
}

uint32_t carla_get_custom_data_count(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->getCustomDataCount();

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

const char* carla_get_parameter_text(CarlaHostHandle handle, uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, gNullCharPtr);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), gNullCharPtr);

        static char textBuf[STR_MAX+1];
        carla_zeroChars(textBuf, STR_MAX+1);

        if (! plugin->getParameterText(parameterId, textBuf))
            textBuf[0] = '\0';

        return textBuf;
    }

    return gNullCharPtr;
}

const char* carla_get_program_name(CarlaHostHandle handle, uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, nullptr);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(programId < plugin->getProgramCount(), gNullCharPtr);

        static char programName[STR_MAX+1];
        carla_zeroChars(programName, STR_MAX+1);

        if (! plugin->getProgramName(programId, programName))
            programName[0] = '\0';

        return programName;
    }

    return gNullCharPtr;
}

const char* carla_get_midi_program_name(CarlaHostHandle handle, uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, gNullCharPtr);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(midiProgramId < plugin->getMidiProgramCount(), gNullCharPtr);

        static char midiProgramName[STR_MAX+1];
        carla_zeroChars(midiProgramName, STR_MAX+1);

        if (! plugin->getMidiProgramName(midiProgramId, midiProgramName))
            midiProgramName[0] = '\0';

        return midiProgramName;
    }

    return gNullCharPtr;
}

const char* carla_get_real_plugin_name(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, gNullCharPtr);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        static char realPluginName[STR_MAX+1];
        carla_zeroChars(realPluginName, STR_MAX+1);

        if (! plugin->getRealName(realPluginName))
            realPluginName[0] = '\0';

        return realPluginName;
    }

    return gNullCharPtr;
}

// --------------------------------------------------------------------------------------------------------------------

int32_t carla_get_current_program_index(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, -1);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->getCurrentProgram();

    return -1;
}

int32_t carla_get_current_midi_program_index(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, -1);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->getCurrentMidiProgram();

    return -1;
}

// --------------------------------------------------------------------------------------------------------------------

float carla_get_default_parameter_value(CarlaHostHandle handle, uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0.0f);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), 0.0f);

        return plugin->getParameterRanges(parameterId).def;
    }

    return 0.0f;
}

float carla_get_current_parameter_value(CarlaHostHandle handle, uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0.0f);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), 0.0f);

        return plugin->getParameterValue(parameterId);
    }

    return 0.0f;
}

float carla_get_internal_parameter_value(CarlaHostHandle handle, uint pluginId, int32_t parameterId)
{
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, (parameterId == CB::PARAMETER_CTRL_CHANNEL) ? -1.0f : 0.0f);
#else
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0.0f);
#endif
    CARLA_SAFE_ASSERT_RETURN(parameterId != CB::PARAMETER_NULL && parameterId > CB::PARAMETER_MAX, 0.0f);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->getInternalParameterValue(parameterId);

    return 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_plugin_latency(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
         return plugin->getLatencyInFrames();

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

const float* carla_get_peak_values(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, nullptr);

    return handle->engine->getPeaks(pluginId);
}

float carla_get_input_peak_value(CarlaHostHandle handle, uint pluginId, bool isLeft)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0.0f);

    return handle->engine->getInputPeak(pluginId, isLeft);
}

float carla_get_output_peak_value(CarlaHostHandle handle, uint pluginId, bool isLeft)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0.0f);

    return handle->engine->getOutputPeak(pluginId, isLeft);
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if !(defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) || defined(CARLA_PLUGIN_ONLY_BRIDGE))
// defined in CarlaPluginInternal.cpp
const void* carla_render_inline_display_internal(const CarlaPluginPtr& plugin, uint32_t width, uint32_t height);
#endif

#ifndef CARLA_PLUGIN_ONLY_BRIDGE
// defined in CarlaPluginLV2.cpp
const void* carla_render_inline_display_lv2(const CarlaPluginPtr& plugin, uint32_t width, uint32_t height);
#endif

CARLA_BACKEND_END_NAMESPACE

const CarlaInlineDisplayImageSurface* carla_render_inline_display(CarlaHostHandle handle,
                                                                  uint pluginId,
                                                                  uint32_t width, uint32_t height)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(), nullptr);

    if (handle->engine->isAboutToClose())
        return nullptr;

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        switch (plugin->getType())
        {
#if !(defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) || defined(CARLA_PLUGIN_ONLY_BRIDGE))
        case CB::PLUGIN_INTERNAL:
            return (const CarlaInlineDisplayImageSurface*)CB::carla_render_inline_display_internal(plugin, width, height);
#endif
#ifndef CARLA_PLUGIN_ONLY_BRIDGE
        case CB::PLUGIN_LV2:
            return (const CarlaInlineDisplayImageSurface*)CB::carla_render_inline_display_lv2(plugin, width, height);
#endif
        default:
            return nullptr;
        }
    }

    return nullptr;

    // maybe unused
    (void)width;
    (void)height;
}

// --------------------------------------------------------------------------------------------------------------------

void carla_set_active(CarlaHostHandle handle, uint pluginId, bool onOff)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setActive(onOff, true, false);
}

#ifndef BUILD_BRIDGE
void carla_set_drywet(CarlaHostHandle handle, uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setDryWet(value, true, false);
}

void carla_set_volume(CarlaHostHandle handle, uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setVolume(value, true, false);
}

void carla_set_balance_left(CarlaHostHandle handle, uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setBalanceLeft(value, true, false);
}

void carla_set_balance_right(CarlaHostHandle handle, uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setBalanceRight(value, true, false);
}

void carla_set_panning(CarlaHostHandle handle, uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setPanning(value, true, false);
}

void carla_set_ctrl_channel(CarlaHostHandle handle, uint pluginId, int8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(channel >= -1 && channel < MAX_MIDI_CHANNELS,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setCtrlChannel(channel, true, false);
}
#endif

void carla_set_option(CarlaHostHandle handle, uint pluginId, uint option, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setOption(option, yesNo, false);
}

// --------------------------------------------------------------------------------------------------------------------

void carla_set_parameter_value(CarlaHostHandle handle, uint pluginId, uint32_t parameterId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(),);

        plugin->setParameterValue(parameterId, value, true, true, false);
    }
}

#ifndef BUILD_BRIDGE

void carla_set_parameter_midi_channel(CarlaHostHandle handle, uint pluginId, uint32_t parameterId, uint8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(),);

        plugin->setParameterMidiChannel(parameterId, channel, true, false);
    }
}

void carla_set_parameter_mapped_control_index(CarlaHostHandle handle, uint pluginId, uint32_t parameterId, int16_t index)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(index >= CB::CONTROL_INDEX_NONE && index <= CB::CONTROL_INDEX_MAX_ALLOWED,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(),);

        plugin->setParameterMappedControlIndex(parameterId, index, true, false, true);
    }
}

void carla_set_parameter_mapped_range(CarlaHostHandle handle, uint pluginId, uint32_t parameterId, float minimum, float maximum)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(),);

        plugin->setParameterMappedRange(parameterId, minimum, maximum, true, false);
    }
}

void carla_set_parameter_touch(CarlaHostHandle handle, uint pluginId, uint32_t parameterId, bool touch)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    carla_debug("carla_set_parameter_touch(%p, %i, %i, %s)", handle, pluginId, parameterId, bool2str(touch));
    return handle->engine->touchPluginParameter(pluginId, parameterId, touch);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

void carla_set_program(CarlaHostHandle handle, uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(programId < plugin->getProgramCount(),);

        plugin->setProgram(static_cast<int32_t>(programId), true, true, false);
    }
}

void carla_set_midi_program(CarlaHostHandle handle, uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(midiProgramId < plugin->getMidiProgramCount(),);

        plugin->setMidiProgram(static_cast<int32_t>(midiProgramId), true, true, false);
    }
}

// --------------------------------------------------------------------------------------------------------------------

void carla_set_custom_data(CarlaHostHandle handle, uint pluginId, const char* type, const char* key, const char* value)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setCustomData(type, key, value, true);
}

void carla_set_chunk_data(CarlaHostHandle handle, uint pluginId, const char* chunkData)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(chunkData != nullptr && chunkData[0] != '\0',);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
    {
        CARLA_SAFE_ASSERT_RETURN(plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS,);

        std::vector<uint8_t> chunk;
        d_getChunkFromBase64String_impl(chunk, chunkData);
#ifdef CARLA_PROPER_CPP11_SUPPORT
        plugin->setChunkData(chunk.data(), chunk.size());
#else
        plugin->setChunkData(&chunk.front(), chunk.size());
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------

void carla_prepare_for_save(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->prepareForSave(false);
}

void carla_reset_parameters(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->resetParameters();
}

void carla_randomize_parameters(CarlaHostHandle handle, uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->randomizeParameters();
}

#ifndef BUILD_BRIDGE
void carla_send_midi_note(CarlaHostHandle handle, uint pluginId, uint8_t channel, uint8_t note, uint8_t velocity)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr && handle->engine->isRunning(),);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);
}
#endif

void carla_set_custom_ui_title(CarlaHostHandle handle, uint pluginId, const char* title)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(title != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->setCustomUITitle(title);
}

void carla_show_custom_ui(CarlaHostHandle handle, uint pluginId, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr,);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        plugin->showCustomUI(yesNo);
}

void* carla_embed_custom_ui(CarlaHostHandle handle, uint pluginId, void* ptr)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, nullptr);

    if (const CarlaPluginPtr plugin = handle->engine->getPlugin(pluginId))
        return plugin->embedCustomUI(ptr);

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_buffer_size(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0);

    carla_debug("carla_get_buffer_size(%p)", handle);
    return handle->engine->getBufferSize();
}

double carla_get_sample_rate(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle->engine != nullptr, 0.0);

    carla_debug("carla_get_sample_rate(%p)", handle);
    return handle->engine->getSampleRate();
}

// --------------------------------------------------------------------------------------------------------------------

const char* carla_get_last_error(CarlaHostHandle handle)
{
    carla_debug("carla_get_last_error(%p)", handle);

    if (handle->engine != nullptr)
        return handle->engine->getLastError();

    return handle->isStandalone
           ? ((CarlaHostStandalone*)handle)->lastError.buffer()
           : gNullCharPtr;
}

const char* carla_get_host_osc_url_tcp(CarlaHostHandle handle)
{
    carla_debug("carla_get_host_osc_url_tcp(%p)", handle);

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    if (handle->engine == nullptr)
    {
        carla_stderr2("carla_get_host_osc_url_tcp() failed, engine is not running");
        if (handle->isStandalone)
            ((CarlaHostStandalone*)handle)->lastError = "Engine is not running";
        return gNullCharPtr;
    }

    const char* const path = handle->engine->getOscServerPathTCP();

    if (path != nullptr && path[0] != '\0')
        return path;

    static const char* const notAvailable = "(OSC TCP port not available)";
    return notAvailable;
#else
    return "(OSC support not available in this build)";

    // unused
    (void)handle;
#endif
}

const char* carla_get_host_osc_url_udp(CarlaHostHandle handle)
{
    carla_debug("carla_get_host_osc_url_udp(%p)", handle);

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    if (handle->engine == nullptr)
    {
        carla_stderr2("carla_get_host_osc_url_udp() failed, engine is not running");
        if (handle->isStandalone)
            ((CarlaHostStandalone*)handle)->lastError = "Engine is not running";
        return gNullCharPtr;
    }

    const char* const path = handle->engine->getOscServerPathUDP();

    if (path != nullptr && path[0] != '\0')
        return path;

    static const char* const notAvailable = "(OSC UDP port not available)";
    return notAvailable;
#else
    return "(OSC support not available in this build)";

    // unused
    (void)handle;
#endif
}

// --------------------------------------------------------------------------------------------------------------------

#ifndef CARLA_PLUGIN_BUILD
# define CARLA_PLUGIN_UI_CLASS_PREFIX Standalone
# include "CarlaPluginUI.cpp"
# undef CARLA_PLUGIN_UI_CLASS_PREFIX
# include "CarlaDssiUtils.cpp"
# include "CarlaMacUtils.cpp"
# include "CarlaPatchbayUtils.cpp"
# include "CarlaPipeUtils.cpp"
# include "CarlaProcessUtils.cpp"
# include "CarlaStateUtils.cpp"
# include "utils/Information.cpp"
# include "utils/Windows.cpp"
#endif /* CARLA_PLUGIN_BUILD */

// --------------------------------------------------------------------------------------------------------------------
