/*
 * Carla Standalone
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

// TODO:
// Check carla_stderr2("Engine is not running"); <= prepend func name and args

#include "CarlaHost.h"
#include "CarlaMIDI.h"

#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBase64Utils.hpp"

#ifdef CARLA_OS_UNIX
# include "CarlaLibUtils.hpp"
#endif

#ifdef BUILD_BRIDGE
# include "water/files/File.h"
#else
# include "CarlaLogThread.hpp"
#endif

#define CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(cond, msg, ret) \
    if (! (cond)) {                                              \
        carla_stderr2("%s: " msg, __FUNCTION__);                 \
        gStandalone.lastError = msg;                             \
        return ret;                                              \
    }

namespace CB = CarlaBackend;
using CB::EngineOptions;

// --------------------------------------------------------------------------------------------------------------------
// Single, standalone engine

struct CarlaBackendStandalone {
    CarlaEngine*       engine;
    EngineCallbackFunc engineCallback;
    void*              engineCallbackPtr;
#ifndef BUILD_BRIDGE
    EngineOptions      engineOptions;
    CarlaLogThread     logThread;
    bool               logThreadEnabled;
#endif

    FileCallbackFunc fileCallback;
    void*            fileCallbackPtr;

    CarlaString lastError;

    CarlaBackendStandalone() noexcept
        : engine(nullptr),
          engineCallback(nullptr),
          engineCallbackPtr(nullptr),
#ifndef BUILD_BRIDGE
          engineOptions(),
          logThread(),
          logThreadEnabled(false),
#endif
          fileCallback(nullptr),
          fileCallbackPtr(nullptr),
          lastError() {}

    ~CarlaBackendStandalone() noexcept
    {
        CARLA_SAFE_ASSERT(engine == nullptr);
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaBackendStandalone)
};

CarlaBackendStandalone gStandalone;

#ifdef CARLA_OS_UNIX
// --------------------------------------------------------------------------------------------------------------------
// Thread-safe fftw

typedef void (*void_func)(void);

class ThreadSafeFFTW
{
public:
    struct Deinitializer {
        Deinitializer(ThreadSafeFFTW& s)
            : tsfftw(s) {}

        ~Deinitializer()
        {
            tsfftw.deinit();
        }

        ThreadSafeFFTW& tsfftw;
    };

    ThreadSafeFFTW()
        : libfftw3(nullptr),
          libfftw3f(nullptr),
          libfftw3l(nullptr),
          libfftw3q(nullptr) {}

    ~ThreadSafeFFTW()
    {
        CARLA_SAFE_ASSERT(libfftw3 == nullptr);
    }

    void init()
    {
        if ((libfftw3 = lib_open("libfftw3_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3, "fftw_make_planner_thread_safe"))
                func();

        if ((libfftw3f = lib_open("libfftw3f_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3f, "fftwf_make_planner_thread_safe"))
                func();

        if ((libfftw3l = lib_open("libfftw3l_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3l, "fftwl_make_planner_thread_safe"))
                func();

        if ((libfftw3q = lib_open("libfftw3q_threads.so.3")) != nullptr)
            if (const void_func func = lib_symbol<void_func>(libfftw3q, "fftwq_make_planner_thread_safe"))
                func();
    }

    void deinit()
    {
        if (libfftw3 != nullptr)
        {
            lib_close(libfftw3);
            libfftw3 = nullptr;
        }

        if (libfftw3f != nullptr)
        {
            lib_close(libfftw3f);
            libfftw3f = nullptr;
        }

        if (libfftw3l != nullptr)
        {
            lib_close(libfftw3l);
            libfftw3l = nullptr;
        }

        if (libfftw3q != nullptr)
        {
            lib_close(libfftw3q);
            libfftw3q = nullptr;
        }
    }

private:
    lib_t libfftw3;
    lib_t libfftw3f;
    lib_t libfftw3l;
    lib_t libfftw3q;
};

static ThreadSafeFFTW sThreadSafeFFTW;
#endif

// --------------------------------------------------------------------------------------------------------------------
// API

#define CARLA_COMMON_NEED_CHECKSTRINGPTR
#include "CarlaHostCommon.cpp"
#undef CARLA_COMMON_NEED_CHECKSTRINGPTR

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
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0', nullptr);

    static EngineDriverDeviceInfo retDevInfo;
    static const uint32_t nullBufferSizes[] = { 0   };
    static const double   nullSampleRates[] = { 0.0 };

    carla_debug("carla_get_engine_driver_device_info(%i, \"%s\")", index, name);

    if (const EngineDriverDeviceInfo* const devInfo = CarlaEngine::getDriverDeviceInfo(index, name))
    {
        retDevInfo.hints       =  devInfo->hints;
        retDevInfo.bufferSizes = (devInfo->bufferSizes != nullptr) ? devInfo->bufferSizes : nullBufferSizes;
        retDevInfo.sampleRates = (devInfo->sampleRates != nullptr) ? devInfo->sampleRates : nullSampleRates;
    }
    else
    {
        retDevInfo.hints       = 0x0;
        retDevInfo.bufferSizes = nullBufferSizes;
        retDevInfo.sampleRates = nullSampleRates;
    }

    return &retDevInfo;
}

// --------------------------------------------------------------------------------------------------------------------

CarlaEngine* carla_get_engine()
{
    carla_debug("carla_get_engine()");

    return gStandalone.engine;
}

// --------------------------------------------------------------------------------------------------------------------

static void carla_engine_init_common(CarlaEngine* const engine)
{
    engine->setCallback(gStandalone.engineCallback, gStandalone.engineCallbackPtr);
    engine->setFileCallback(gStandalone.fileCallback, gStandalone.fileCallbackPtr);

#ifdef BUILD_BRIDGE
    using water::File;
    File waterBinaryDir(File::getSpecialLocation(File::currentExecutableFile).getParentDirectory());

    /*
    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_FORCE_STEREO"))
        engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_PREFER_PLUGIN_BRIDGES"))
        engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_PREFER_UI_BRIDGES"))
        engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);
    */

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_UIS_ALWAYS_ON_TOP"))
        engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const maxParameters = std::getenv("ENGINE_OPTION_MAX_PARAMETERS"))
        engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS,     std::atoi(maxParameters), nullptr);

    if (const char* const uiBridgesTimeout = std::getenv("ENGINE_OPTION_UI_BRIDGES_TIMEOUT"))
        engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT, std::atoi(uiBridgesTimeout), nullptr);

    if (const char* const pathLADSPA = std::getenv("ENGINE_OPTION_PLUGIN_PATH_LADSPA"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_LADSPA, pathLADSPA);

    if (const char* const pathDSSI = std::getenv("ENGINE_OPTION_PLUGIN_PATH_DSSI"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_DSSI, pathDSSI);

    if (const char* const pathLV2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_LV2"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_LV2, pathLV2);

    if (const char* const pathVST2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_VST2"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_VST2, pathVST2);

    if (const char* const pathSF2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_SF2"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_SF2, pathSF2);

    if (const char* const pathSFZ = std::getenv("ENGINE_OPTION_PLUGIN_PATH_SFZ"))
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_SFZ, pathSFZ);

    if (const char* const binaryDir = std::getenv("ENGINE_OPTION_PATH_BINARIES"))
        engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,   0, binaryDir);
    else
        engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,   0, waterBinaryDir.getFullPathName().toRawUTF8());

    if (const char* const resourceDir = std::getenv("ENGINE_OPTION_PATH_RESOURCES"))
        engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,  0, resourceDir);
    else
        engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,  0, waterBinaryDir.getChildFile("resources").getFullPathName().toRawUTF8());

    if (const char* const preventBadBehaviour = std::getenv("ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR"))
        engine->setOption(CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR, (std::strcmp(preventBadBehaviour, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const frontendWinId = std::getenv("ENGINE_OPTION_FRONTEND_WIN_ID"))
        engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, frontendWinId);
#else
    engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          gStandalone.engineOptions.forceStereo         ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, gStandalone.engineOptions.preferPluginBridges ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     gStandalone.engineOptions.preferUiBridges     ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     gStandalone.engineOptions.uisAlwaysOnTop      ? 1 : 0,        nullptr);
    engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS,        static_cast<int>(gStandalone.engineOptions.maxParameters),    nullptr);
    engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    static_cast<int>(gStandalone.engineOptions.uiBridgesTimeout), nullptr);
    engine->setOption(CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE,     static_cast<int>(gStandalone.engineOptions.audioBufferSize),  nullptr);
    engine->setOption(CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE,     static_cast<int>(gStandalone.engineOptions.audioSampleRate),  nullptr);
    engine->setOption(CB::ENGINE_OPTION_AUDIO_TRIPLE_BUFFER,   gStandalone.engineOptions.audioTripleBuffer   ? 1 : 0,        nullptr);

    if (gStandalone.engineOptions.audioDevice != nullptr)
        engine->setOption(CB::ENGINE_OPTION_AUDIO_DEVICE,      0, gStandalone.engineOptions.audioDevice);

    if (gStandalone.engineOptions.pathLADSPA != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_LADSPA, gStandalone.engineOptions.pathLADSPA);

    if (gStandalone.engineOptions.pathDSSI != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_DSSI, gStandalone.engineOptions.pathDSSI);

    if (gStandalone.engineOptions.pathLV2 != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_LV2, gStandalone.engineOptions.pathLV2);

    if (gStandalone.engineOptions.pathVST2 != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_VST2, gStandalone.engineOptions.pathVST2);

    if (gStandalone.engineOptions.pathSF2 != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_SF2, gStandalone.engineOptions.pathSF2);

    if (gStandalone.engineOptions.pathSFZ != nullptr)
        engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_SFZ, gStandalone.engineOptions.pathSFZ);

    if (gStandalone.engineOptions.binaryDir != nullptr && gStandalone.engineOptions.binaryDir[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,     0, gStandalone.engineOptions.binaryDir);

    if (gStandalone.engineOptions.resourceDir != nullptr && gStandalone.engineOptions.resourceDir[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,    0, gStandalone.engineOptions.resourceDir);

    engine->setOption(CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR,    gStandalone.engineOptions.preventBadBehaviour ? 1 : 0,  nullptr);

    if (gStandalone.engineOptions.frontendWinId != 0)
    {
        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';
        std::snprintf(strBuf, STR_MAX, P_UINTPTR, gStandalone.engineOptions.frontendWinId);
        engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, strBuf);
    }
    else
    {
        engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, "0");
    }

# ifndef CARLA_OS_WIN
    if (gStandalone.engineOptions.wine.executable != nullptr && gStandalone.engineOptions.wine.executable[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_WINE_EXECUTABLE, 0, gStandalone.engineOptions.wine.executable);

    engine->setOption(CB::ENGINE_OPTION_WINE_AUTO_PREFIX, gStandalone.engineOptions.wine.autoPrefix ? 1 : 0, nullptr);

    if (gStandalone.engineOptions.wine.fallbackPrefix != nullptr && gStandalone.engineOptions.wine.fallbackPrefix[0] != '\0')
        engine->setOption(CB::ENGINE_OPTION_WINE_FALLBACK_PREFIX, 0, gStandalone.engineOptions.wine.fallbackPrefix);

    engine->setOption(CB::ENGINE_OPTION_WINE_RT_PRIO_ENABLED, gStandalone.engineOptions.wine.rtPrio ? 1 : 0, nullptr);
    engine->setOption(CB::ENGINE_OPTION_WINE_BASE_RT_PRIO, gStandalone.engineOptions.wine.baseRtPrio, nullptr);
    engine->setOption(CB::ENGINE_OPTION_WINE_SERVER_RT_PRIO, gStandalone.engineOptions.wine.serverRtPrio, nullptr);
# endif
#endif
}

bool carla_engine_init(const char* driverName, const char* clientName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    carla_debug("carla_engine_init(\"%s\", \"%s\")", driverName, clientName);

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine == nullptr, "Engine is already initialized", false);

#ifdef CARLA_OS_WIN
    carla_setenv("WINEASIO_CLIENT_NAME", clientName);
#endif

    ScopedPointer<CarlaEngine> engine(CarlaEngine::newDriverByName(driverName));

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(engine != nullptr, "The seleted audio driver is not available", false);

#ifdef BUILD_BRIDGE
    engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          CB::ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS, nullptr);
    engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        CB::ENGINE_TRANSPORT_MODE_JACK,           nullptr);
    engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          false,                                    nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, false,                                    nullptr);
    engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     false,                                    nullptr);
#else
    engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          static_cast<int>(gStandalone.engineOptions.processMode),   nullptr);
    engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        static_cast<int>(gStandalone.engineOptions.transportMode), gStandalone.engineOptions.transportExtra);
#endif

    carla_engine_init_common(engine);

    if (engine->init(clientName))
    {
#ifndef BUILD_BRIDGE
        if (gStandalone.logThreadEnabled && std::getenv("CARLA_LOGS_DISABLED") == nullptr)
            gStandalone.logThread.init();
#endif
#ifdef CARLA_OS_UNIX
        sThreadSafeFFTW.init();
#endif
        gStandalone.lastError = "No error";
        gStandalone.engine = engine.release();
        return true;
    }
    else
    {
        gStandalone.lastError = engine->getLastError();
        return false;
    }
}

#ifdef BUILD_BRIDGE
bool carla_engine_init_bridge(const char audioBaseName[6+1], const char rtClientBaseName[6+1], const char nonRtClientBaseName[6+1],
                              const char nonRtServerBaseName[6+1], const char* clientName)
{
    CARLA_SAFE_ASSERT_RETURN(audioBaseName != nullptr && audioBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(rtClientBaseName != nullptr && rtClientBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(nonRtClientBaseName != nullptr && nonRtClientBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(nonRtServerBaseName != nullptr && nonRtServerBaseName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    carla_debug("carla_engine_init_bridge(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", audioBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName, clientName);

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine == nullptr, "Engine is already initialized", false);

    ScopedPointer<CarlaEngine> engine(CarlaEngine::newBridge(audioBaseName,
                                                             rtClientBaseName,
                                                             nonRtClientBaseName,
                                                             nonRtServerBaseName));

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(engine != nullptr, "The seleted audio driver is not available", false);

    carla_engine_init_common(engine);

    engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,   CB::ENGINE_PROCESS_MODE_BRIDGE,   nullptr);
    engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE, CB::ENGINE_TRANSPORT_MODE_BRIDGE, nullptr);

    if (engine->init(clientName))
    {
        gStandalone.lastError = "No error";
        gStandalone.engine = engine.release();
        return true;
    }
    else
    {
        gStandalone.lastError = engine->getLastError();
        return false;
    }
}
#endif

bool carla_engine_close()
{
    carla_debug("carla_engine_close()");

    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

#ifdef CARLA_OS_UNIX
    const ThreadSafeFFTW::Deinitializer tsfftwde(sThreadSafeFFTW);
#endif

    ScopedPointer<CarlaEngine> engine(gStandalone.engine);
    gStandalone.engine = nullptr;

    engine->setAboutToClose();
    engine->removeAllPlugins();

    const bool closed = engine->close();

    if (! closed)
        gStandalone.lastError = engine->getLastError();

#ifndef BUILD_BRIDGE
    gStandalone.logThread.stop();
#endif

    return closed;
}

void carla_engine_idle()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    gStandalone.engine->idle();
}

bool carla_is_engine_running()
{
    return (gStandalone.engine != nullptr && gStandalone.engine->isRunning());
}

bool carla_set_engine_about_to_close()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, true);
    carla_debug("carla_set_engine_about_to_close()");

    return gStandalone.engine->setAboutToClose();
}

void carla_set_engine_callback(EngineCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_engine_callback(%p, %p)", func, ptr);

    gStandalone.engineCallback    = func;
    gStandalone.engineCallbackPtr = ptr;

#ifndef BUILD_BRIDGE
    gStandalone.logThread.setCallback(func, ptr);
#endif

    if (gStandalone.engine != nullptr)
        gStandalone.engine->setCallback(func, ptr);
}

#ifndef BUILD_BRIDGE
void carla_set_engine_option(EngineOption option, int value, const char* valueStr)
{
    carla_debug("carla_set_engine_option(%i:%s, %i, \"%s\")", option, CB::EngineOption2Str(option), value, valueStr);

    switch (option)
    {
    case CB::ENGINE_OPTION_DEBUG:
        break;

    case CB::ENGINE_OPTION_PROCESS_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= CB::ENGINE_PROCESS_MODE_SINGLE_CLIENT && value < CB::ENGINE_PROCESS_MODE_BRIDGE,);
        gStandalone.engineOptions.processMode = static_cast<CB::EngineProcessMode>(value);
        break;

    case CB::ENGINE_OPTION_TRANSPORT_MODE:
        CARLA_SAFE_ASSERT_RETURN(value >= CB::ENGINE_TRANSPORT_MODE_DISABLED && value <= CB::ENGINE_TRANSPORT_MODE_BRIDGE,);
        gStandalone.engineOptions.transportMode = static_cast<CB::EngineTransportMode>(value);
        delete[] gStandalone.engineOptions.transportExtra;
        if (value != CB::ENGINE_TRANSPORT_MODE_DISABLED && valueStr != nullptr)
            gStandalone.engineOptions.transportExtra = carla_strdup_safe(valueStr);
        else
            gStandalone.engineOptions.transportExtra = nullptr;
        break;

    case CB::ENGINE_OPTION_FORCE_STEREO:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.forceStereo = (value != 0);
        break;

    case CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.preferPluginBridges = (value != 0);
        break;

    case CB::ENGINE_OPTION_PREFER_UI_BRIDGES:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.preferUiBridges = (value != 0);
        break;

    case CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.uisAlwaysOnTop = (value != 0);
        break;

    case CB::ENGINE_OPTION_MAX_PARAMETERS:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        gStandalone.engineOptions.maxParameters = static_cast<uint>(value);
        break;

    case CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT:
        CARLA_SAFE_ASSERT_RETURN(value >= 0,);
        gStandalone.engineOptions.uiBridgesTimeout = static_cast<uint>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE:
        CARLA_SAFE_ASSERT_RETURN(value >= 8,);
        gStandalone.engineOptions.audioBufferSize = static_cast<uint>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE:
        CARLA_SAFE_ASSERT_RETURN(value >= 22050,);
        gStandalone.engineOptions.audioSampleRate = static_cast<uint>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_TRIPLE_BUFFER:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.audioTripleBuffer = (value != 0);
        break;

    case CB::ENGINE_OPTION_AUDIO_DEVICE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

        if (gStandalone.engineOptions.audioDevice != nullptr)
            delete[] gStandalone.engineOptions.audioDevice;

        gStandalone.engineOptions.audioDevice = carla_strdup_safe(valueStr);
        break;

    case CB::ENGINE_OPTION_PLUGIN_PATH:
        CARLA_SAFE_ASSERT_RETURN(value > CB::PLUGIN_NONE,);
        CARLA_SAFE_ASSERT_RETURN(value <= CB::PLUGIN_SFZ,);
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr,);

        switch (value)
        {
        case CB::PLUGIN_LADSPA:
            if (gStandalone.engineOptions.pathLADSPA != nullptr)
                delete[] gStandalone.engineOptions.pathLADSPA;
            gStandalone.engineOptions.pathLADSPA = carla_strdup_safe(valueStr);
            break;
        case CB::PLUGIN_DSSI:
            if (gStandalone.engineOptions.pathDSSI != nullptr)
                delete[] gStandalone.engineOptions.pathDSSI;
            gStandalone.engineOptions.pathDSSI = carla_strdup_safe(valueStr);
            break;
        case CB::PLUGIN_LV2:
            if (gStandalone.engineOptions.pathLV2 != nullptr)
                delete[] gStandalone.engineOptions.pathLV2;
            gStandalone.engineOptions.pathLV2 = carla_strdup_safe(valueStr);
            break;
        case CB::PLUGIN_VST2:
            if (gStandalone.engineOptions.pathVST2 != nullptr)
                delete[] gStandalone.engineOptions.pathVST2;
            gStandalone.engineOptions.pathVST2 = carla_strdup_safe(valueStr);
            break;
        case CB::PLUGIN_SF2:
            if (gStandalone.engineOptions.pathSF2 != nullptr)
                delete[] gStandalone.engineOptions.pathSF2;
            gStandalone.engineOptions.pathSF2 = carla_strdup_safe(valueStr);
            break;
        case CB::PLUGIN_SFZ:
            if (gStandalone.engineOptions.pathSFZ != nullptr)
                delete[] gStandalone.engineOptions.pathSFZ;
            gStandalone.engineOptions.pathSFZ = carla_strdup_safe(valueStr);
            break;
        }
        break;

    case CB::ENGINE_OPTION_PATH_BINARIES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (gStandalone.engineOptions.binaryDir != nullptr)
            delete[] gStandalone.engineOptions.binaryDir;

        gStandalone.engineOptions.binaryDir = carla_strdup_safe(valueStr);
        break;

    case CB::ENGINE_OPTION_PATH_RESOURCES:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (gStandalone.engineOptions.resourceDir != nullptr)
            delete[] gStandalone.engineOptions.resourceDir;

        gStandalone.engineOptions.resourceDir = carla_strdup_safe(valueStr);
        break;

    case CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.preventBadBehaviour = (value != 0);
        break;

    case CB::ENGINE_OPTION_FRONTEND_WIN_ID: {
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);
        const long long winId(std::strtoll(valueStr, nullptr, 16));
        CARLA_SAFE_ASSERT_RETURN(winId >= 0,);
        gStandalone.engineOptions.frontendWinId = static_cast<uintptr_t>(winId);
    }   break;

#ifndef CARLA_OS_WIN
    case CB::ENGINE_OPTION_WINE_EXECUTABLE:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (gStandalone.engineOptions.wine.executable != nullptr)
            delete[] gStandalone.engineOptions.wine.executable;

        gStandalone.engineOptions.wine.executable = carla_strdup_safe(valueStr);
        break;

    case CB::ENGINE_OPTION_WINE_AUTO_PREFIX:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.wine.autoPrefix = (value != 0);
        break;

    case CB::ENGINE_OPTION_WINE_FALLBACK_PREFIX:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);

        if (gStandalone.engineOptions.wine.fallbackPrefix != nullptr)
            delete[] gStandalone.engineOptions.wine.fallbackPrefix;

        gStandalone.engineOptions.wine.fallbackPrefix = carla_strdup_safe(valueStr);
        break;

    case CB::ENGINE_OPTION_WINE_RT_PRIO_ENABLED:
        CARLA_SAFE_ASSERT_RETURN(value == 0 || value == 1,);
        gStandalone.engineOptions.wine.rtPrio = (value != 0);
        break;

    case CB::ENGINE_OPTION_WINE_BASE_RT_PRIO:
        CARLA_SAFE_ASSERT_RETURN(value >= 1 && value <= 89,);
        gStandalone.engineOptions.wine.baseRtPrio = value;
        break;

    case CB::ENGINE_OPTION_WINE_SERVER_RT_PRIO:
        CARLA_SAFE_ASSERT_RETURN(value >= 1 && value <= 99,);
        gStandalone.engineOptions.wine.serverRtPrio = value;
        break;
#endif

    case CB::ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT:
        gStandalone.logThreadEnabled = (value != 0);
        break;
    }

    if (gStandalone.engine != nullptr)
        gStandalone.engine->setOption(option, value, valueStr);
}
#endif

void carla_set_file_callback(FileCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_file_callback(%p, %p)", func, ptr);

    gStandalone.fileCallback    = func;
    gStandalone.fileCallbackPtr = ptr;

    if (gStandalone.engine != nullptr)
        gStandalone.engine->setFileCallback(func, ptr);
}

// --------------------------------------------------------------------------------------------------------------------

bool carla_load_file(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_load_file(\"%s\")", filename);

    return gStandalone.engine->loadFile(filename);
}

bool carla_load_project(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_load_project(\"%s\")", filename);

    return gStandalone.engine->loadProject(filename);
}

bool carla_save_project(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_save_project(\"%s\")", filename);

    return gStandalone.engine->saveProject(filename);
}

#ifndef BUILD_BRIDGE
// --------------------------------------------------------------------------------------------------------------------

bool carla_patchbay_connect(uint groupIdA, uint portIdA, uint groupIdB, uint portIdB)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_patchbay_connect(%u, %u, %u, %u)", groupIdA, portIdA, groupIdB, portIdB);

    return gStandalone.engine->patchbayConnect(groupIdA, portIdA, groupIdB, portIdB);
}

bool carla_patchbay_disconnect(uint connectionId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_patchbay_disconnect(%i)", connectionId);

    return gStandalone.engine->patchbayDisconnect(connectionId);
}

bool carla_patchbay_refresh(bool external)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_patchbay_refresh(%s)", bool2str(external));

    return gStandalone.engine->patchbayRefresh(external);
}

// --------------------------------------------------------------------------------------------------------------------

void carla_transport_play()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);

    carla_debug("carla_transport_play()");

    gStandalone.engine->transportPlay();
}

void carla_transport_pause()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);

    carla_debug("carla_transport_pause()");

    gStandalone.engine->transportPause();
}

void carla_transport_bpm(double bpm)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);

    carla_debug("carla_transport_bpm(%f)", bpm);

    gStandalone.engine->transportBPM(bpm);
}

void carla_transport_relocate(uint64_t frame)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);

    carla_debug("carla_transport_relocate(%i)", frame);

    gStandalone.engine->transportRelocate(frame);
}

uint64_t carla_get_current_transport_frame()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(), 0);

    return gStandalone.engine->getTimeInfo().frame;
}

const CarlaTransportInfo* carla_get_transport_info()
{
    static CarlaTransportInfo retTransInfo;
    carla_zeroStruct(retTransInfo);

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(), &retTransInfo);

    const CB::EngineTimeInfo& timeInfo(gStandalone.engine->getTimeInfo());

    retTransInfo.playing = timeInfo.playing;
    retTransInfo.frame   = timeInfo.frame;

    if (timeInfo.bbt.valid)
    {
        retTransInfo.bar  = timeInfo.bbt.bar;
        retTransInfo.beat = timeInfo.bbt.beat;
        retTransInfo.tick = timeInfo.bbt.tick;
        retTransInfo.bpm  = timeInfo.bbt.beatsPerMinute;
    }

    return &retTransInfo;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_current_plugin_count()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

    carla_debug("carla_get_current_plugin_count()");

    return gStandalone.engine->getCurrentPluginCount();
}

uint32_t carla_get_max_plugin_number()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

    carla_debug("carla_get_max_plugin_number()");

    return gStandalone.engine->getMaxPluginNumber();
}

// --------------------------------------------------------------------------------------------------------------------

bool carla_add_plugin(BinaryType btype, PluginType ptype,
                      const char* filename, const char* name, const char* label, int64_t uniqueId,
                      const void* extraPtr, uint options)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_add_plugin(%i:%s, %i:%s, \"%s\", \"%s\", \"%s\", " P_INT64 ", %p, %u)",
                btype, CB::BinaryType2Str(btype),
                ptype, CB::PluginType2Str(ptype),
                filename, name, label, uniqueId, extraPtr, options);

    return gStandalone.engine->addPlugin(btype, ptype, filename, name, label, uniqueId, extraPtr, options);
}

bool carla_remove_plugin(uint pluginId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_remove_plugin(%i)", pluginId);

    return gStandalone.engine->removePlugin(pluginId);
}

bool carla_remove_all_plugins()
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_remove_all_plugins()");

    return gStandalone.engine->removeAllPlugins();
}

#ifndef BUILD_BRIDGE
const char* carla_rename_plugin(uint pluginId, const char* newName)
{
    CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", nullptr);

    carla_debug("carla_rename_plugin(%i, \"%s\")", pluginId, newName);

    return gStandalone.engine->renamePlugin(pluginId, newName);
}

bool carla_clone_plugin(uint pluginId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_clone_plugin(%i)", pluginId);

    return gStandalone.engine->clonePlugin(pluginId);
}

bool carla_replace_plugin(uint pluginId)
{
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_replace_plugin(%i)", pluginId);

    return gStandalone.engine->replacePlugin(pluginId);
}

bool carla_switch_plugins(uint pluginIdA, uint pluginIdB)
{
    CARLA_SAFE_ASSERT_RETURN(pluginIdA != pluginIdB, false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    carla_debug("carla_switch_plugins(%i, %i)", pluginIdA, pluginIdB);

    return gStandalone.engine->switchPlugins(pluginIdA, pluginIdB);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

bool carla_load_plugin_state(uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr
                                          && gStandalone.engine->isRunning(), "Engine is not running", false);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(plugin != nullptr, "could not find requested plugin", false);

    carla_debug("carla_load_plugin_state(%i, \"%s\")", pluginId, filename);

    return plugin->loadStateFromFile(filename);
}

bool carla_save_plugin_state(uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(plugin != nullptr, "could not find requested plugin", false);

    carla_debug("carla_save_plugin_state(%i, \"%s\")", pluginId, filename);

    return plugin->saveStateToFile(filename);
}

bool carla_export_plugin_lv2(uint pluginId, const char* lv2path)
{
    CARLA_SAFE_ASSERT_RETURN(lv2path != nullptr && lv2path[0] != '\0', false);
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(gStandalone.engine != nullptr, "Engine is not initialized", false);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_WITH_LAST_ERROR_RETURN(plugin != nullptr, "could not find requested plugin", false);

    carla_debug("carla_export_plugin_lv2(%i, \"%s\")", pluginId, lv2path);

    return plugin->exportAsLV2(lv2path);
}

// --------------------------------------------------------------------------------------------------------------------

const CarlaPluginInfo* carla_get_plugin_info(uint pluginId)
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

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retInfo);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retInfo);

    carla_debug("carla_get_plugin_info(%i)", pluginId);

    char strBuf[STR_MAX+1];

    retInfo.type     = plugin->getType();
    retInfo.category = plugin->getCategory();
    retInfo.hints    = plugin->getHints();
    retInfo.filename = plugin->getFilename();
    retInfo.name     = plugin->getName();
    retInfo.iconName = plugin->getIconName();
    retInfo.uniqueId = plugin->getUniqueId();

    retInfo.optionsAvailable = plugin->getOptionsAvailable();
    retInfo.optionsEnabled   = plugin->getOptionsEnabled();

    carla_zeroChars(strBuf, STR_MAX+1);
    plugin->getLabel(strBuf);
    retInfo.label = carla_strdup_safe(strBuf);

    carla_zeroChars(strBuf, STR_MAX+1);
    plugin->getMaker(strBuf);
    retInfo.maker = carla_strdup_safe(strBuf);

    carla_zeroChars(strBuf, STR_MAX+1);
    plugin->getCopyright(strBuf);
    retInfo.copyright = carla_strdup_safe(strBuf);

    checkStringPtr(retInfo.filename);
    checkStringPtr(retInfo.name);
    checkStringPtr(retInfo.iconName);
    checkStringPtr(retInfo.label);
    checkStringPtr(retInfo.maker);
    checkStringPtr(retInfo.copyright);

    return &retInfo;
}

const CarlaPortCountInfo* carla_get_audio_port_count_info(uint pluginId)
{
    static CarlaPortCountInfo retInfo;
    carla_zeroStruct(retInfo);

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retInfo);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retInfo);

    carla_debug("carla_get_audio_port_count_info(%i)", pluginId);

    retInfo.ins   = plugin->getAudioInCount();
    retInfo.outs  = plugin->getAudioOutCount();
    return &retInfo;
}

const CarlaPortCountInfo* carla_get_midi_port_count_info(uint pluginId)
{
    static CarlaPortCountInfo retInfo;
    carla_zeroStruct(retInfo);

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retInfo);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retInfo);

    carla_debug("carla_get_midi_port_count_info(%i)", pluginId);

    retInfo.ins   = plugin->getMidiInCount();
    retInfo.outs  = plugin->getMidiOutCount();
    return &retInfo;
}

const CarlaPortCountInfo* carla_get_parameter_count_info(uint pluginId)
{
    static CarlaPortCountInfo retInfo;
    carla_zeroStruct(retInfo);

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retInfo);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retInfo);

    carla_debug("carla_get_parameter_count_info(%i)", pluginId);

    plugin->getParameterCountInfo(retInfo.ins, retInfo.outs);
    return &retInfo;
}

const CarlaParameterInfo* carla_get_parameter_info(uint pluginId, uint32_t parameterId)
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

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retInfo);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retInfo);

    carla_debug("carla_get_parameter_info(%i, %i)", pluginId, parameterId);

    char strBuf[STR_MAX+1];

    retInfo.scalePointCount = plugin->getParameterScalePointCount(parameterId);

    carla_zeroChars(strBuf, STR_MAX+1);
    plugin->getParameterName(parameterId, strBuf);
    retInfo.name = carla_strdup_safe(strBuf);

    carla_zeroChars(strBuf, STR_MAX+1);
    plugin->getParameterSymbol(parameterId, strBuf);
    retInfo.symbol = carla_strdup_safe(strBuf);

    carla_zeroChars(strBuf, STR_MAX+1);
    plugin->getParameterUnit(parameterId, strBuf);
    retInfo.unit = carla_strdup_safe(strBuf);

    checkStringPtr(retInfo.name);
    checkStringPtr(retInfo.symbol);
    checkStringPtr(retInfo.unit);

    return &retInfo;
}

const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(uint pluginId, uint32_t parameterId, uint32_t scalePointId)
{
    CARLA_ASSERT(gStandalone.engine != nullptr);

    static CarlaScalePointInfo retInfo;

    // reset
    retInfo.value = 0.0f;

    // cleanup
    if (retInfo.label != gNullCharPtr)
    {
        delete[] retInfo.label;
        retInfo.label = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retInfo);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retInfo);

    carla_debug("carla_get_parameter_scalepoint_info(%i, %i, %i)", pluginId, parameterId, scalePointId);

    char strBuf[STR_MAX+1];

    retInfo.value = plugin->getParameterScalePointValue(parameterId, scalePointId);

    carla_zeroChars(strBuf, STR_MAX+1);
    plugin->getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    retInfo.label = carla_strdup_safe(strBuf);

    checkStringPtr(retInfo.label);

    return &retInfo;
}

// --------------------------------------------------------------------------------------------------------------------

const ParameterData* carla_get_parameter_data(uint pluginId, uint32_t parameterId)
{
    static ParameterData retParamData;

    // reset
    retParamData.type        = CB::PARAMETER_UNKNOWN;
    retParamData.hints       = 0x0;
    retParamData.index       = CB::PARAMETER_NULL;
    retParamData.rindex      = -1;
    retParamData.midiCC      = -1;
    retParamData.midiChannel = 0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retParamData);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retParamData);

    carla_debug("carla_get_parameter_data(%i, %i)", pluginId, parameterId);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), &retParamData);

    const ParameterData& pluginParamData(plugin->getParameterData(parameterId));
    retParamData.type        = pluginParamData.type;
    retParamData.hints       = pluginParamData.hints;
    retParamData.index       = pluginParamData.index;
    retParamData.rindex      = pluginParamData.rindex;
    retParamData.midiCC      = pluginParamData.midiCC;
    retParamData.midiChannel = pluginParamData.midiChannel;
    return &plugin->getParameterData(parameterId);
}

const ParameterRanges* carla_get_parameter_ranges(uint pluginId, uint32_t parameterId)
{
    static ParameterRanges retParamRanges;

    // reset
    retParamRanges.def       = 0.0f;
    retParamRanges.min       = 0.0f;
    retParamRanges.max       = 1.0f;
    retParamRanges.step      = 0.01f;
    retParamRanges.stepSmall = 0.0001f;
    retParamRanges.stepLarge = 0.1f;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retParamRanges);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retParamRanges);

    carla_debug("carla_get_parameter_ranges(%i, %i)", pluginId, parameterId);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), &retParamRanges);

    const ParameterRanges& pluginParamRanges(plugin->getParameterRanges(parameterId));
    retParamRanges.def       = pluginParamRanges.def;
    retParamRanges.min       = pluginParamRanges.min;
    retParamRanges.max       = pluginParamRanges.max;
    retParamRanges.step      = pluginParamRanges.step;
    retParamRanges.stepSmall = pluginParamRanges.stepSmall;
    retParamRanges.stepLarge = pluginParamRanges.stepLarge;
    return &pluginParamRanges;
}

const MidiProgramData* carla_get_midi_program_data(uint pluginId, uint32_t midiProgramId)
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

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retMidiProgData);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retMidiProgData);

    carla_debug("carla_get_midi_program_data(%i, %i)", pluginId, midiProgramId);
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

    return &retMidiProgData;
}

const CustomData* carla_get_custom_data(uint pluginId, uint32_t customDataId)
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

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &retCustomData);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, &retCustomData);

    carla_debug("carla_get_custom_data(%i, %i)", pluginId, customDataId);
    CARLA_SAFE_ASSERT_RETURN(customDataId < plugin->getCustomDataCount(), &retCustomData)

    const CustomData& pluginCustomData(plugin->getCustomData(customDataId));
    retCustomData.type  = carla_strdup_safe(pluginCustomData.type);
    retCustomData.key   = carla_strdup_safe(pluginCustomData.key);
    retCustomData.value = carla_strdup_safe(pluginCustomData.value);
    checkStringPtr(retCustomData.type);
    checkStringPtr(retCustomData.key);
    checkStringPtr(retCustomData.value);
    return &retCustomData;
}

const char* carla_get_chunk_data(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, gNullCharPtr);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, gNullCharPtr);

    carla_debug("carla_get_chunk_data(%i)", pluginId);
    CARLA_SAFE_ASSERT_RETURN(plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS, gNullCharPtr);

    void* data = nullptr;
    const std::size_t dataSize(plugin->getChunkData(&data));
    CARLA_SAFE_ASSERT_RETURN(data != nullptr && dataSize > 0, gNullCharPtr);

    static CarlaString chunkData;

    chunkData = CarlaString::asBase64(data, static_cast<std::size_t>(dataSize));
    return chunkData.buffer();
}

// --------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_parameter_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, 0);

    carla_debug("carla_get_parameter_count(%i)", pluginId);
    return plugin->getParameterCount();
}

uint32_t carla_get_program_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, 0);

    carla_debug("carla_get_program_count(%i)", pluginId);
    return plugin->getProgramCount();
}

uint32_t carla_get_midi_program_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, 0);

    carla_debug("carla_get_midi_program_count(%i)", pluginId);
    return plugin->getMidiProgramCount();
}

uint32_t carla_get_custom_data_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, 0);

    carla_debug("carla_get_custom_data_count(%i)", pluginId);
    return plugin->getCustomDataCount();
}

// --------------------------------------------------------------------------------------------------------------------

const char* carla_get_parameter_text(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, gNullCharPtr);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, gNullCharPtr);

    carla_debug("carla_get_parameter_text(%i, %i)", pluginId, parameterId);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), gNullCharPtr);

    static char textBuf[STR_MAX+1];
    carla_zeroChars(textBuf, STR_MAX+1);

    plugin->getParameterText(parameterId, textBuf);
    return textBuf;
}

const char* carla_get_program_name(uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, gNullCharPtr);

    carla_debug("carla_get_program_name(%i, %i)", pluginId, programId);
    CARLA_SAFE_ASSERT_RETURN(programId < plugin->getProgramCount(), gNullCharPtr);

    static char programName[STR_MAX+1];
    carla_zeroChars(programName, STR_MAX+1);

    plugin->getProgramName(programId, programName);
    return programName;
}

const char* carla_get_midi_program_name(uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, gNullCharPtr);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, gNullCharPtr);

    carla_debug("carla_get_midi_program_name(%i, %i)", pluginId, midiProgramId);
    CARLA_SAFE_ASSERT_RETURN(midiProgramId < plugin->getMidiProgramCount(), gNullCharPtr);

    static char midiProgramName[STR_MAX+1];
    carla_zeroChars(midiProgramName, STR_MAX+1);

    plugin->getMidiProgramName(midiProgramId, midiProgramName);
    return midiProgramName;
}

const char* carla_get_real_plugin_name(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, gNullCharPtr);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, gNullCharPtr);

    carla_debug("carla_get_real_plugin_name(%i)", pluginId);
    static char realPluginName[STR_MAX+1];
    carla_zeroChars(realPluginName, STR_MAX+1);

    plugin->getRealName(realPluginName);
    return realPluginName;
}

// --------------------------------------------------------------------------------------------------------------------

int32_t carla_get_current_program_index(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, -1);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, -1);

    carla_debug("carla_get_current_program_index(%i)", pluginId);
    return plugin->getCurrentProgram();
}

int32_t carla_get_current_midi_program_index(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, -1);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, -1);

    carla_debug("carla_get_current_midi_program_index(%i)", pluginId);
    return plugin->getCurrentMidiProgram();
}

// --------------------------------------------------------------------------------------------------------------------

float carla_get_default_parameter_value(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, 0.0f);

    carla_debug("carla_get_default_parameter_value(%i, %i)", pluginId, parameterId);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), 0.0f);

    return plugin->getParameterRanges(parameterId).def;
}

float carla_get_current_parameter_value(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, 0.0f);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(), 0.0f);

    return plugin->getParameterValue(parameterId);
}

float carla_get_internal_parameter_value(uint pluginId, int32_t parameterId)
{
#ifdef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);
#else
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, (parameterId == CB::PARAMETER_CTRL_CHANNEL) ? -1.0f : 0.0f);
#endif
    CARLA_SAFE_ASSERT_RETURN(parameterId != CB::PARAMETER_NULL && parameterId > CB::PARAMETER_MAX, 0.0f);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, 0.0f);

    carla_debug("carla_get_internal_parameter_value(%i, %i)", pluginId, parameterId);
    return plugin->getInternalParameterValue(parameterId);
}

// --------------------------------------------------------------------------------------------------------------------

float carla_get_input_peak_value(uint pluginId, bool isLeft)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    return gStandalone.engine->getInputPeak(pluginId, isLeft);
}

float carla_get_output_peak_value(uint pluginId, bool isLeft)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    return gStandalone.engine->getOutputPeak(pluginId, isLeft);
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// defined in CarlaPluginLV2.cpp
void* carla_render_inline_display_lv2(CarlaPlugin* plugin, int width, int height);

CARLA_BACKEND_END_NAMESPACE

CarlaInlineDisplayImageSurface* carla_render_inline_display(uint pluginId, int width, int height)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr, nullptr);

    carla_debug("carla_render_inline_display(%i, %i, %i)", pluginId, width, height);
    CARLA_SAFE_ASSERT_RETURN(plugin->getType() == CB::PLUGIN_LV2, nullptr);

    return (CarlaInlineDisplayImageSurface*)CB::carla_render_inline_display_lv2(plugin, width, height);
}

// --------------------------------------------------------------------------------------------------------------------

void carla_set_active(uint pluginId, bool onOff)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_active(%i, %s)", pluginId, bool2str(onOff));
    return plugin->setActive(onOff, true, false);
}

#ifndef BUILD_BRIDGE
void carla_set_drywet(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_drywet(%i, %f)", pluginId, value);
    return plugin->setDryWet(value, true, false);
}

void carla_set_volume(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_volume(%i, %f)", pluginId, value);
    return plugin->setVolume(value, true, false);
}

void carla_set_balance_left(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_balance_left(%i, %f)", pluginId, value);
    return plugin->setBalanceLeft(value, true, false);
}

void carla_set_balance_right(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_balance_right(%i, %f)", pluginId, value);
    return plugin->setBalanceRight(value, true, false);
}

void carla_set_panning(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_panning(%i, %f)", pluginId, value);
    return plugin->setPanning(value, true, false);
}

void carla_set_ctrl_channel(uint pluginId, int8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(channel >= -1 && channel < MAX_MIDI_CHANNELS,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_ctrl_channel(%i, %i)", pluginId, channel);
    return plugin->setCtrlChannel(channel, true, false);
}

void carla_set_option(uint pluginId, uint option, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_option(%i, %i, %s)", pluginId, option, bool2str(yesNo));
    return plugin->setOption(option, yesNo, false);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

void carla_set_parameter_value(uint pluginId, uint32_t parameterId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_parameter_value(%i, %i, %f)", pluginId, parameterId, value);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(),);

    return plugin->setParameterValue(parameterId, value, true, true, false);
}

#ifndef BUILD_BRIDGE
void carla_set_parameter_midi_channel(uint pluginId, uint32_t parameterId, uint8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_parameter_midi_channel(%i, %i, %i)", pluginId, parameterId, channel);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(),);

    return plugin->setParameterMidiChannel(parameterId, channel, true, false);
}

void carla_set_parameter_midi_cc(uint pluginId, uint32_t parameterId, int16_t cc)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < MAX_MIDI_CONTROL,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_parameter_midi_cc(%i, %i, %i)", pluginId, parameterId, cc);
    CARLA_SAFE_ASSERT_RETURN(parameterId < plugin->getParameterCount(),);

    return plugin->setParameterMidiCC(parameterId, cc, true, false);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

void carla_set_program(uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_program(%i, %i)", pluginId, programId);
    CARLA_SAFE_ASSERT_RETURN(programId < plugin->getProgramCount(),);

    return plugin->setProgram(static_cast<int32_t>(programId), true, true, false);
}

void carla_set_midi_program(uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_midi_program(%i, %i)", pluginId, midiProgramId);
    CARLA_SAFE_ASSERT_RETURN(midiProgramId < plugin->getMidiProgramCount(),);

    return plugin->setMidiProgram(static_cast<int32_t>(midiProgramId), true, true, false);
}

// --------------------------------------------------------------------------------------------------------------------

void carla_set_custom_data(uint pluginId, const char* type, const char* key, const char* value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\")", pluginId, type, key, value);
    return plugin->setCustomData(type, key, value, true);
}

void carla_set_chunk_data(uint pluginId, const char* chunkData)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(chunkData != nullptr && chunkData[0] != '\0',);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_set_chunk_data(%i, \"%s\")", pluginId, chunkData);
    CARLA_SAFE_ASSERT_RETURN(plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS,);

    std::vector<uint8_t> chunk(carla_getChunkFromBase64String(chunkData));
#ifdef CARLA_PROPER_CPP11_SUPPORT
    return plugin->setChunkData(chunk.data(), chunk.size());
#else
    return plugin->setChunkData(&chunk.front(), chunk.size());
#endif
}

// --------------------------------------------------------------------------------------------------------------------

void carla_prepare_for_save(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_prepare_for_save(%i)", pluginId);
    return plugin->prepareForSave();
}

void carla_reset_parameters(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_reset_parameters(%i)", pluginId);
    return plugin->resetParameters();
}

void carla_randomize_parameters(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_randomize_parameters(%i)", pluginId);
    return plugin->randomizeParameters();
}

#ifndef BUILD_BRIDGE
void carla_send_midi_note(uint pluginId, uint8_t channel, uint8_t note, uint8_t velocity)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_send_midi_note(%i, %i, %i, %i)", pluginId, channel, note, velocity);
    return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);
}
#endif

void carla_show_custom_ui(uint pluginId, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);

    CarlaPlugin* const plugin(gStandalone.engine->getPlugin(pluginId));
    CARLA_SAFE_ASSERT_RETURN(plugin != nullptr,);

    carla_debug("carla_show_custom_ui(%i, %s)", pluginId, bool2str(yesNo));
    return plugin->showCustomUI(yesNo);
}

// --------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_buffer_size()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);

    carla_debug("carla_get_buffer_size()");
    return gStandalone.engine->getBufferSize();
}

double carla_get_sample_rate()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0);

    carla_debug("carla_get_sample_rate()");
    return gStandalone.engine->getSampleRate();
}

// --------------------------------------------------------------------------------------------------------------------

const char* carla_get_last_error()
{
    carla_debug("carla_get_last_error()");

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->getLastError();

    return gStandalone.lastError;
}

const char* carla_get_host_osc_url_tcp()
{
    carla_debug("carla_get_host_osc_url_tcp()");

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("carla_get_host_osc_url_tcp() failed, engine is not running");
        gStandalone.lastError = "Engine is not running";
        return gNullCharPtr;
    }

    return gStandalone.engine->getOscServerPathTCP();
#else
    return gNullCharPtr;
#endif
}

const char* carla_get_host_osc_url_udp()
{
    carla_debug("carla_get_host_osc_url_udp()");

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("carla_get_host_osc_url_udp() failed, engine is not running");
        gStandalone.lastError = "Engine is not running";
        return gNullCharPtr;
    }

    return gStandalone.engine->getOscServerPathUDP();
#else
    return gNullCharPtr;
#endif
}

// --------------------------------------------------------------------------------------------------------------------

#define CARLA_PLUGIN_UI_CLASS_PREFIX Standalone
#include "CarlaPluginUI.cpp"
#undef CARLA_PLUGIN_UI_CLASS_PREFIX

#include "CarlaDssiUtils.cpp"
#include "CarlaPatchbayUtils.cpp"
#include "CarlaPipeUtils.cpp"
#include "CarlaStateUtils.cpp"

// --------------------------------------------------------------------------------------------------------------------
