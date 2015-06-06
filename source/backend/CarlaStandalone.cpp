/*
 * Carla Standalone
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

// TODO:
// Check carla_stderr2("Engine is not running"); <= prepend func name and args

#include "CarlaHost.h"
#include "CarlaMIDI.h"

#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBase64Utils.hpp"

#include "juce_audio_formats.h"

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
# include "juce_gui_basics.h"
#else
namespace juce {
# include "juce_events/messages/juce_Initialisation.h"
} // namespace juce
#endif

namespace CB = CarlaBackend;
using CB::EngineOptions;

// -------------------------------------------------------------------------------------------------------------------
// Single, standalone engine

struct CarlaBackendStandalone {
    CarlaEngine*       engine;
    EngineCallbackFunc engineCallback;
    void*              engineCallbackPtr;
#ifndef BUILD_BRIDGE
    EngineOptions      engineOptions;
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

// -------------------------------------------------------------------------------------------------------------------
// API

#define CARLA_COMMON_NEED_CHECKSTRINGPTR
#include "CarlaHostCommon.cpp"

// -------------------------------------------------------------------------------------------------------------------

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
    carla_debug("carla_get_engine_driver_device_info(%i, \"%s\")", index, name);

    if (const EngineDriverDeviceInfo* const ret = CarlaEngine::getDriverDeviceInfo(index, name))
    {
        static EngineDriverDeviceInfo devInfo;
        static const uint32_t nullBufferSizes[] = { 0   };
        static const double   nullSampleRates[] = { 0.0 };

        devInfo.hints       = ret->hints;
        devInfo.bufferSizes = (ret->bufferSizes != nullptr) ? ret->bufferSizes : nullBufferSizes;
        devInfo.sampleRates = (ret->sampleRates != nullptr) ? ret->sampleRates : nullSampleRates;
        return &devInfo;
    }

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

CarlaEngine* carla_get_engine()
{
    carla_debug("carla_get_engine()");

    return gStandalone.engine;
}

// -------------------------------------------------------------------------------------------------------------------

static void carla_engine_init_common()
{
    gStandalone.engine->setCallback(gStandalone.engineCallback, gStandalone.engineCallbackPtr);
    gStandalone.engine->setFileCallback(gStandalone.fileCallback, gStandalone.fileCallbackPtr);

#ifdef BUILD_BRIDGE
    using juce::File;
    File juceBinaryDir(File::getSpecialLocation(File::currentExecutableFile).getParentDirectory());

    /*
    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_FORCE_STEREO"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_PREFER_PLUGIN_BRIDGES"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_PREFER_UI_BRIDGES"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);
    */

    if (const char* const uisAlwaysOnTop = std::getenv("ENGINE_OPTION_UIS_ALWAYS_ON_TOP"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP, (std::strcmp(uisAlwaysOnTop, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const maxParameters = std::getenv("ENGINE_OPTION_MAX_PARAMETERS"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS,     std::atoi(maxParameters), nullptr);

    if (const char* const uiBridgesTimeout = std::getenv("ENGINE_OPTION_UI_BRIDGES_TIMEOUT"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT, std::atoi(uiBridgesTimeout), nullptr);

    if (const char* const pathLADSPA = std::getenv("ENGINE_OPTION_PLUGIN_PATH_LADSPA"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_LADSPA, pathLADSPA);

    if (const char* const pathDSSI = std::getenv("ENGINE_OPTION_PLUGIN_PATH_DSSI"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_DSSI, pathDSSI);

    if (const char* const pathLV2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_LV2"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_LV2, pathLV2);

    if (const char* const pathVST2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_VST2"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_VST2, pathVST2);

    if (const char* const pathVST3 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_VST3"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_VST3, pathVST3);

    if (const char* const pathGIG = std::getenv("ENGINE_OPTION_PLUGIN_PATH_GIG"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_GIG, pathGIG);

    if (const char* const pathSF2 = std::getenv("ENGINE_OPTION_PLUGIN_PATH_SF2"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_SF2, pathSF2);

    if (const char* const pathSFZ = std::getenv("ENGINE_OPTION_PLUGIN_PATH_SFZ"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH, CB::PLUGIN_SFZ, pathSFZ);

    if (const char* const binaryDir = std::getenv("ENGINE_OPTION_PATH_BINARIES"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,   0, binaryDir);
    else
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,   0, juceBinaryDir.getFullPathName().toRawUTF8());

    if (const char* const resourceDir = std::getenv("ENGINE_OPTION_PATH_RESOURCES"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,  0, resourceDir);
    else
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,  0, juceBinaryDir.getChildFile("resources").getFullPathName().toRawUTF8());

    if (const char* const preventBadBehaviour = std::getenv("ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR, (std::strcmp(preventBadBehaviour, "true") == 0) ? 1 : 0, nullptr);

    if (const char* const frontendWinId = std::getenv("ENGINE_OPTION_FRONTEND_WIN_ID"))
        gStandalone.engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, frontendWinId);
#else
    gStandalone.engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          gStandalone.engineOptions.forceStereo         ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, gStandalone.engineOptions.preferPluginBridges ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     gStandalone.engineOptions.preferUiBridges     ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     gStandalone.engineOptions.uisAlwaysOnTop      ? 1 : 0,        nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_MAX_PARAMETERS,        static_cast<int>(gStandalone.engineOptions.maxParameters),    nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    static_cast<int>(gStandalone.engineOptions.uiBridgesTimeout), nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_NUM_PERIODS,     static_cast<int>(gStandalone.engineOptions.audioNumPeriods),  nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE,     static_cast<int>(gStandalone.engineOptions.audioBufferSize),  nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE,     static_cast<int>(gStandalone.engineOptions.audioSampleRate),  nullptr);

    if (gStandalone.engineOptions.audioDevice != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_AUDIO_DEVICE,      0, gStandalone.engineOptions.audioDevice);

    if (gStandalone.engineOptions.pathLADSPA != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_LADSPA, gStandalone.engineOptions.pathLADSPA);

    if (gStandalone.engineOptions.pathDSSI != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_DSSI, gStandalone.engineOptions.pathDSSI);

    if (gStandalone.engineOptions.pathLV2 != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_LV2, gStandalone.engineOptions.pathLV2);

    if (gStandalone.engineOptions.pathVST2 != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_VST2, gStandalone.engineOptions.pathVST2);

    if (gStandalone.engineOptions.pathVST3 != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_VST3, gStandalone.engineOptions.pathVST3);

    if (gStandalone.engineOptions.pathGIG != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_GIG, gStandalone.engineOptions.pathGIG);

    if (gStandalone.engineOptions.pathSF2 != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_SF2, gStandalone.engineOptions.pathSF2);

    if (gStandalone.engineOptions.pathSFZ != nullptr)
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PLUGIN_PATH,       CB::PLUGIN_SFZ, gStandalone.engineOptions.pathSFZ);

    if (gStandalone.engineOptions.binaryDir != nullptr && gStandalone.engineOptions.binaryDir[0] != '\0')
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_BINARIES,     0, gStandalone.engineOptions.binaryDir);

    if (gStandalone.engineOptions.resourceDir != nullptr && gStandalone.engineOptions.resourceDir[0] != '\0')
        gStandalone.engine->setOption(CB::ENGINE_OPTION_PATH_RESOURCES,    0, gStandalone.engineOptions.resourceDir);

    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR,    gStandalone.engineOptions.preventBadBehaviour ? 1 : 0,  nullptr);

    if (gStandalone.engineOptions.frontendWinId != 0)
    {
        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';
        std::snprintf(strBuf, STR_MAX, P_UINTPTR, gStandalone.engineOptions.frontendWinId);
        gStandalone.engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, strBuf);
    }
    else
        gStandalone.engine->setOption(CB::ENGINE_OPTION_FRONTEND_WIN_ID, 0, "0");
#endif
}

bool carla_engine_init(const char* driverName, const char* clientName)
{
    CARLA_SAFE_ASSERT_RETURN(driverName != nullptr && driverName[0] != '\0', false);
    CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
    carla_debug("carla_engine_init(\"%s\", \"%s\")", driverName, clientName);

    if (gStandalone.engine != nullptr)
    {
        carla_stderr2("Engine is already running");
        gStandalone.lastError = "Engine is already running";
        return false;
    }

#ifdef CARLA_OS_WIN
    carla_setenv("WINEASIO_CLIENT_NAME", clientName);
#endif

    // TODO: make this an option, put somewhere else
    if (std::getenv("WINE_RT") == nullptr)
    {
        carla_setenv("WINE_RT", "15");
        carla_setenv("WINE_SVR_RT", "10");
    }

    gStandalone.engine = CarlaEngine::newDriverByName(driverName);

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("The seleted audio driver is not available");
        gStandalone.lastError = "The seleted audio driver is not available";
        return false;
    }

#ifdef BUILD_BRIDGE
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          CB::ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS,                  nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        CB::ENGINE_TRANSPORT_MODE_JACK,                            nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_FORCE_STEREO,          false,                                                     nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, false,                                                     nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PREFER_UI_BRIDGES,     false,                                                     nullptr);
#else
    gStandalone.engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,          static_cast<int>(gStandalone.engineOptions.processMode),   nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE,        static_cast<int>(gStandalone.engineOptions.transportMode), nullptr);
#endif

    carla_engine_init_common();

    if (gStandalone.engine->init(clientName))
    {
#ifndef BUILD_BRIDGE
        juce::initialiseJuce_GUI();
#endif
        gStandalone.lastError = "No error";
        return true;
    }
    else
    {
        gStandalone.lastError = gStandalone.engine->getLastError();
        delete gStandalone.engine;
        gStandalone.engine = nullptr;
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

    if (gStandalone.engine != nullptr)
    {
        carla_stderr2("Engine is already running");
        gStandalone.lastError = "Engine is already running";
        return false;
    }

    gStandalone.engine = CarlaEngine::newBridge(audioBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("The seleted audio driver is not available!");
        gStandalone.lastError = "The seleted audio driver is not available!";
        return false;
    }

    carla_engine_init_common();

    gStandalone.engine->setOption(CB::ENGINE_OPTION_PROCESS_MODE,   CB::ENGINE_PROCESS_MODE_BRIDGE,   nullptr);
    gStandalone.engine->setOption(CB::ENGINE_OPTION_TRANSPORT_MODE, CB::ENGINE_TRANSPORT_MODE_BRIDGE, nullptr);

    if (gStandalone.engine->init(clientName))
    {
        gStandalone.lastError = "No error";
        return true;
    }
    else
    {
        gStandalone.lastError = gStandalone.engine->getLastError();
        delete gStandalone.engine;
        gStandalone.engine = nullptr;
        return false;
    }
}
#endif

bool carla_engine_close()
{
    carla_debug("carla_engine_close()");

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return false;
    }

    gStandalone.engine->setAboutToClose();
    gStandalone.engine->removeAllPlugins();

    const bool closed(gStandalone.engine->close());

    if (! closed)
        gStandalone.lastError = gStandalone.engine->getLastError();

#ifndef BUILD_BRIDGE
    juce::shutdownJuce_GUI();
#endif
    delete gStandalone.engine;
    gStandalone.engine = nullptr;

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

void carla_set_engine_about_to_close()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_engine_about_to_close()");

    gStandalone.engine->setAboutToClose();
}

void carla_set_engine_callback(EngineCallbackFunc func, void* ptr)
{
    carla_debug("carla_set_engine_callback(%p, %p)", func, ptr);

    gStandalone.engineCallback    = func;
    gStandalone.engineCallbackPtr = ptr;

    if (gStandalone.engine != nullptr)
        gStandalone.engine->setCallback(func, ptr);

//#ifdef WANT_LOGS
//    gLogThread.setCallback(func, ptr);
//#endif
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
        CARLA_SAFE_ASSERT_RETURN(value >= CB::ENGINE_TRANSPORT_MODE_INTERNAL && value < CB::ENGINE_TRANSPORT_MODE_BRIDGE,);
        gStandalone.engineOptions.transportMode = static_cast<CB::EngineTransportMode>(value);
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

    case CB::ENGINE_OPTION_AUDIO_NUM_PERIODS:
        CARLA_SAFE_ASSERT_RETURN(value >= 2 && value <= 3,);
        gStandalone.engineOptions.audioNumPeriods = static_cast<uint>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_BUFFER_SIZE:
        CARLA_SAFE_ASSERT_RETURN(value >= 8,);
        gStandalone.engineOptions.audioBufferSize = static_cast<uint>(value);
        break;

    case CB::ENGINE_OPTION_AUDIO_SAMPLE_RATE:
        CARLA_SAFE_ASSERT_RETURN(value >= 22050,);
        gStandalone.engineOptions.audioSampleRate = static_cast<uint>(value);
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
        case CB::PLUGIN_VST3:
            if (gStandalone.engineOptions.pathVST3 != nullptr)
                delete[] gStandalone.engineOptions.pathVST3;
            gStandalone.engineOptions.pathVST3 = carla_strdup_safe(valueStr);
            break;
        case CB::PLUGIN_GIG:
            if (gStandalone.engineOptions.pathGIG != nullptr)
                delete[] gStandalone.engineOptions.pathGIG;
            gStandalone.engineOptions.pathGIG = carla_strdup_safe(valueStr);
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

    case CB::ENGINE_OPTION_FRONTEND_WIN_ID:
        CARLA_SAFE_ASSERT_RETURN(valueStr != nullptr && valueStr[0] != '\0',);
        const long long winId(std::strtoll(valueStr, nullptr, 16));
        CARLA_SAFE_ASSERT_RETURN(winId >= 0,);
        gStandalone.engineOptions.frontendWinId = static_cast<uintptr_t>(winId);
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

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_file(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_load_file(\"%s\")", filename);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->loadFile(filename);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_load_project(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_load_project(\"%s\")", filename);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->loadProject(filename);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_save_project(const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_save_project(\"%s\")", filename);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->saveProject(filename);

    carla_stderr2("Engine was never initiated");
    gStandalone.lastError = "Engine was never initiated";
    return false;
}

#ifndef BUILD_BRIDGE
// -------------------------------------------------------------------------------------------------------------------

bool carla_patchbay_connect(uint groupIdA, uint portIdA, uint groupIdB, uint portIdB)
{
    carla_debug("carla_patchbay_connect(%u, %u, %u, %u)", groupIdA, portIdA, groupIdB, portIdB);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->patchbayConnect(groupIdA, portIdA, groupIdB, portIdB);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_patchbay_disconnect(uint connectionId)
{
    carla_debug("carla_patchbay_disconnect(%i)", connectionId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->patchbayDisconnect(connectionId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_patchbay_refresh(bool external)
{
    carla_debug("carla_patchbay_refresh(%s)", bool2str(external));

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->patchbayRefresh(external);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

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

void carla_transport_relocate(uint64_t frame)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);
    carla_debug("carla_transport_relocate(%i)", frame);

    gStandalone.engine->transportRelocate(frame);
}

uint64_t carla_get_current_transport_frame()
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(), 0);

    const CB::EngineTimeInfo& timeInfo(gStandalone.engine->getTimeInfo());
    return timeInfo.frame;
}

const CarlaTransportInfo* carla_get_transport_info()
{
    static CarlaTransportInfo retInfo;

    // reset
    retInfo.playing = false;
    retInfo.frame   = 0;
    retInfo.bar     = 0;
    retInfo.beat    = 0;
    retInfo.tick    = 0;
    retInfo.bpm     = 0.0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(), &retInfo);

    const CB::EngineTimeInfo& timeInfo(gStandalone.engine->getTimeInfo());

    retInfo.playing = timeInfo.playing;
    retInfo.frame   = timeInfo.frame;

    if (timeInfo.valid & CB::EngineTimeInfo::kValidBBT)
    {
        retInfo.bar  = timeInfo.bbt.bar;
        retInfo.beat = timeInfo.bbt.beat;
        retInfo.tick = timeInfo.bbt.tick;
        retInfo.bpm  = timeInfo.bbt.beatsPerMinute;
    }

    return &retInfo;
}
#endif

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_current_plugin_count()
{
    if (gStandalone.engine != nullptr)
        return gStandalone.engine->getCurrentPluginCount();

    return 0;
}

uint32_t carla_get_max_plugin_number()
{
    if (gStandalone.engine != nullptr)
        return gStandalone.engine->getMaxPluginNumber();

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_add_plugin(BinaryType btype, PluginType ptype, const char* filename, const char* name, const char* label, int64_t uniqueId, const void* extraPtr, uint options)
{
    carla_debug("carla_add_plugin(%i:%s, %i:%s, \"%s\", \"%s\", \"%s\", " P_INT64 ", %p, %u)", btype, CB::BinaryType2Str(btype), ptype, CB::PluginType2Str(ptype), filename, name, label, uniqueId, extraPtr, options);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->addPlugin(btype, ptype, filename, name, label, uniqueId, extraPtr, options);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_remove_plugin(uint pluginId)
{
    carla_debug("carla_remove_plugin(%i)", pluginId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->removePlugin(pluginId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_remove_all_plugins()
{
    carla_debug("carla_remove_all_plugins()");

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->removeAllPlugins();

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

#ifndef BUILD_BRIDGE
const char* carla_rename_plugin(uint pluginId, const char* newName)
{
    CARLA_SAFE_ASSERT_RETURN(newName != nullptr && newName[0] != '\0', nullptr);
    carla_debug("carla_rename_plugin(%i, \"%s\")", pluginId, newName);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->renamePlugin(pluginId, newName);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return nullptr;
}

bool carla_clone_plugin(uint pluginId)
{
    carla_debug("carla_clone_plugin(%i)", pluginId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->clonePlugin(pluginId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_replace_plugin(uint pluginId)
{
    carla_debug("carla_replace_plugin(%i)", pluginId);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->replacePlugin(pluginId);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}

bool carla_switch_plugins(uint pluginIdA, uint pluginIdB)
{
    CARLA_SAFE_ASSERT_RETURN(pluginIdA != pluginIdB, false);
    carla_debug("carla_switch_plugins(%i, %i)", pluginIdA, pluginIdB);

    if (gStandalone.engine != nullptr)
        return gStandalone.engine->switchPlugins(pluginIdA, pluginIdB);

    carla_stderr2("Engine is not running");
    gStandalone.lastError = "Engine is not running";
    return false;
}
#endif

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_plugin_state(uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_load_plugin_state(%i, \"%s\")", pluginId, filename);

    if (gStandalone.engine == nullptr || ! gStandalone.engine->isRunning())
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return false;
    }

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->loadStateFromFile(filename);

    carla_stderr2("carla_load_plugin_state(%i, \"%s\") - could not find plugin", pluginId, filename);
    return false;
}

bool carla_save_plugin_state(uint pluginId, const char* filename)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', false);
    carla_debug("carla_save_plugin_state(%i, \"%s\")", pluginId, filename);

    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return false;
    }

    // allow to save even if engine isn't running

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->saveStateToFile(filename);

    carla_stderr2("carla_save_plugin_state(%i, \"%s\") - could not find plugin", pluginId, filename);
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaPluginInfo* carla_get_plugin_info(uint pluginId)
{
    carla_debug("carla_get_plugin_info(%i)", pluginId);

    static CarlaPluginInfo info;

    // reset
    info.type             = CB::PLUGIN_NONE;
    info.category         = CB::PLUGIN_CATEGORY_NONE;
    info.hints            = 0x0;
    info.optionsAvailable = 0x0;
    info.optionsEnabled   = 0x0;
    info.filename         = gNullCharPtr;
    info.name             = gNullCharPtr;
    info.iconName         = gNullCharPtr;
    info.uniqueId         = 0;

    // cleanup
    if (info.label != gNullCharPtr)
    {
        delete[] info.label;
        info.label = gNullCharPtr;
    }

    if (info.maker != gNullCharPtr)
    {
        delete[] info.maker;
        info.maker = gNullCharPtr;
    }

    if (info.copyright != gNullCharPtr)
    {
        delete[] info.copyright;
        info.copyright = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        char strBufLabel[STR_MAX+1];
        char strBufMaker[STR_MAX+1];
        char strBufCopyright[STR_MAX+1];

        carla_zeroChars(strBufLabel, STR_MAX+1);
        carla_zeroChars(strBufMaker, STR_MAX+1);
        carla_zeroChars(strBufCopyright, STR_MAX+1);

        info.type     = plugin->getType();
        info.category = plugin->getCategory();
        info.hints    = plugin->getHints();
        info.filename = plugin->getFilename();
        info.name     = plugin->getName();
        info.iconName = plugin->getIconName();
        info.uniqueId = plugin->getUniqueId();

        info.optionsAvailable = plugin->getOptionsAvailable();
        info.optionsEnabled   = plugin->getOptionsEnabled();

        plugin->getLabel(strBufLabel);
        info.label = carla_strdup_safe(strBufLabel);

        plugin->getMaker(strBufMaker);
        info.maker = carla_strdup_safe(strBufMaker);

        plugin->getCopyright(strBufCopyright);
        info.copyright = carla_strdup_safe(strBufCopyright);

        checkStringPtr(info.filename);
        checkStringPtr(info.name);
        checkStringPtr(info.iconName);
        checkStringPtr(info.label);
        checkStringPtr(info.maker);
        checkStringPtr(info.copyright);

        return &info;
    }

    carla_stderr2("carla_get_plugin_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_audio_port_count_info(uint pluginId)
{
    carla_debug("carla_get_audio_port_count_info(%i)", pluginId);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        info.ins   = plugin->getAudioInCount();
        info.outs  = plugin->getAudioOutCount();
        return &info;
    }

    carla_stderr2("carla_get_audio_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_midi_port_count_info(uint pluginId)
{
    carla_debug("carla_get_midi_port_count_info(%i)", pluginId);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        info.ins   = plugin->getMidiInCount();
        info.outs  = plugin->getMidiOutCount();
        return &info;
    }

    carla_stderr2("carla_get_midi_port_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaPortCountInfo* carla_get_parameter_count_info(uint pluginId)
{
    carla_debug("carla_get_parameter_count_info(%i)", pluginId);

    static CarlaPortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        plugin->getParameterCountInfo(info.ins, info.outs);
        return &info;
    }

    carla_stderr2("carla_get_parameter_count_info(%i) - could not find plugin", pluginId);
    return &info;
}

const CarlaParameterInfo* carla_get_parameter_info(uint pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_info(%i, %i)", pluginId, parameterId);

    static CarlaParameterInfo info;

    // reset
    info.scalePointCount = 0;

    // cleanup
    if (info.name != gNullCharPtr)
    {
        delete[] info.name;
        info.name = gNullCharPtr;
    }

    if (info.symbol != gNullCharPtr)
    {
        delete[] info.symbol;
        info.symbol = gNullCharPtr;
    }

    if (info.unit != gNullCharPtr)
    {
        delete[] info.unit;
        info.unit = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
        {
            char strBufName[STR_MAX+1];
            char strBufSymbol[STR_MAX+1];
            char strBufUnit[STR_MAX+1];

            carla_zeroChars(strBufName, STR_MAX+1);
            carla_zeroChars(strBufSymbol, STR_MAX+1);
            carla_zeroChars(strBufUnit, STR_MAX+1);

            info.scalePointCount = plugin->getParameterScalePointCount(parameterId);

            plugin->getParameterName(parameterId, strBufName);
            info.name = carla_strdup_safe(strBufName);

            plugin->getParameterSymbol(parameterId, strBufSymbol);
            info.symbol = carla_strdup_safe(strBufSymbol);

            plugin->getParameterUnit(parameterId, strBufUnit);
            info.unit = carla_strdup_safe(strBufUnit);

            checkStringPtr(info.name);
            checkStringPtr(info.symbol);
            checkStringPtr(info.unit);
        }
        else
            carla_stderr2("carla_get_parameter_info(%i, %i) - parameterId out of bounds", pluginId, parameterId);

        return &info;
    }

    carla_stderr2("carla_get_parameter_info(%i, %i) - could not find plugin", pluginId, parameterId);
    return &info;
}

const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(uint pluginId, uint32_t parameterId, uint32_t scalePointId)
{
    carla_debug("carla_get_parameter_scalepoint_info(%i, %i, %i)", pluginId, parameterId, scalePointId);
    CARLA_ASSERT(gStandalone.engine != nullptr);

    static CarlaScalePointInfo info;

    // reset
    info.value = 0.0f;

    // cleanup
    if (info.label != gNullCharPtr)
    {
        delete[] info.label;
        info.label = gNullCharPtr;
    }

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &info);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
        {
            if (scalePointId < plugin->getParameterScalePointCount(parameterId))
            {
                char strBufLabel[STR_MAX+1];
                carla_zeroChars(strBufLabel, STR_MAX+1);

                info.value = plugin->getParameterScalePointValue(parameterId, scalePointId);

                plugin->getParameterScalePointLabel(parameterId, scalePointId, strBufLabel);
                info.label = carla_strdup_safe(strBufLabel);
                checkStringPtr(info.label);
            }
            else
                carla_stderr2("carla_get_parameter_scalepoint_info(%i, %i, %i) - scalePointId out of bounds", pluginId, parameterId, scalePointId);
        }
        else
            carla_stderr2("carla_get_parameter_scalepoint_info(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, scalePointId);

        return &info;
    }

    carla_stderr2("carla_get_parameter_scalepoint_info(%i, %i, %i) - could not find plugin", pluginId, parameterId, scalePointId);
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

const ParameterData* carla_get_parameter_data(uint pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_data(%i, %i)", pluginId, parameterId);

    static const ParameterData fallbackParameterData = { CB::PARAMETER_UNKNOWN, 0x0, CB::PARAMETER_NULL, -1, -1, 0 };

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &fallbackParameterData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return &plugin->getParameterData(parameterId);

        carla_stderr2("carla_get_parameter_data(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &fallbackParameterData;
    }

    carla_stderr2("carla_get_parameter_data(%i, %i) - could not find plugin", pluginId, parameterId);
    return &fallbackParameterData;
}

const ParameterRanges* carla_get_parameter_ranges(uint pluginId, uint32_t parameterId)
{
    carla_debug("carla_get_parameter_ranges(%i, %i)", pluginId, parameterId);

    static const ParameterRanges fallbackParamRanges = { 0.0f, 0.0f, 1.0f, 0.01f, 0.0001f, 0.1f };

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &fallbackParamRanges);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return &plugin->getParameterRanges(parameterId);

        carla_stderr2("carla_get_parameter_ranges(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return &fallbackParamRanges;
    }

    carla_stderr2("carla_get_parameter_ranges(%i, %i) - could not find plugin", pluginId, parameterId);
    return &fallbackParamRanges;
}

const MidiProgramData* carla_get_midi_program_data(uint pluginId, uint32_t midiProgramId)
{
    carla_debug("carla_get_midi_program_data(%i, %i)", pluginId, midiProgramId);

    static MidiProgramData midiProgData;

    // reset
    midiProgData.bank    = 0;
    midiProgData.program = 0;
    midiProgData.name    = gNullCharPtr;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &midiProgData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->getMidiProgramCount())
        {
            const MidiProgramData& ret(plugin->getMidiProgramData(midiProgramId));
            carla_copyStruct(midiProgData, ret);
            checkStringPtr(midiProgData.name);
            return &midiProgData;
        }

        carla_stderr2("carla_get_midi_program_data(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return &midiProgData;
    }

    carla_stderr2("carla_get_midi_program_data(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return &midiProgData;
}

const CustomData* carla_get_custom_data(uint pluginId, uint32_t customDataId)
{
    carla_debug("carla_get_custom_data(%i, %i)", pluginId, customDataId);

    static CustomData customData;

    // reset
    customData.type  = gNullCharPtr;
    customData.key   = gNullCharPtr;
    customData.value = gNullCharPtr;

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, &customData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (customDataId < plugin->getCustomDataCount())
        {
            const CustomData& ret(plugin->getCustomData(customDataId));
            carla_copyStruct(customData, ret);
            checkStringPtr(customData.type);
            checkStringPtr(customData.key);
            checkStringPtr(customData.value);
            return &customData;
        }

        carla_stderr2("carla_get_custom_data(%i, %i) - customDataId out of bounds", pluginId, customDataId);
        return &customData;
    }

    carla_stderr2("carla_get_custom_data(%i, %i) - could not find plugin", pluginId, customDataId);
    return &customData;
}

const char* carla_get_chunk_data(uint pluginId)
{
    carla_debug("carla_get_chunk_data(%i)", pluginId);

    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);

    static CarlaString chunkData;

    // cleanup
    chunkData.clear();

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS)
        {
            void* data = nullptr;
            const std::size_t dataSize(plugin->getChunkData(&data));

            if (data != nullptr && dataSize > 0)
            {
                chunkData = CarlaString::asBase64(data, static_cast<std::size_t>(dataSize));
                return chunkData;
            }
            else
                carla_stderr2("carla_get_chunk_data(%i) - got invalid chunk data", pluginId);
        }
        else
            carla_stderr2("carla_get_chunk_data(%i) - plugin does not use chunks", pluginId);

        return nullptr;
    }

    carla_stderr2("carla_get_chunk_data(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_parameter_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_parameter_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getParameterCount();

    carla_stderr2("carla_get_parameter_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_program_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_program_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getProgramCount();

    carla_stderr2("carla_get_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_midi_program_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_midi_program_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getMidiProgramCount();

    carla_stderr2("carla_get_midi_program_count(%i) - could not find plugin", pluginId);
    return 0;
}

uint32_t carla_get_custom_data_count(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0);
    carla_debug("carla_get_custom_data_count(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getCustomDataCount();

    carla_stderr2("carla_get_custom_data_count(%i) - could not find plugin", pluginId);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_parameter_text(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_parameter_text(%i, %i)", pluginId, parameterId);

    static char textBuf[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
        {
            carla_zeroChars(textBuf, STR_MAX+1);
            plugin->getParameterText(parameterId, textBuf);
            return textBuf;
        }

        carla_stderr2("carla_get_parameter_text(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return nullptr;
    }

    carla_stderr2("carla_get_parameter_text(%i, %i) - could not find plugin", pluginId, parameterId);
    return nullptr;
}

const char* carla_get_program_name(uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_program_name(%i, %i)", pluginId, programId);

    static char programName[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (programId < plugin->getProgramCount())
        {
            carla_zeroChars(programName, STR_MAX+1);
            plugin->getProgramName(programId, programName);
            return programName;
        }

        carla_stderr2("carla_get_program_name(%i, %i) - programId out of bounds", pluginId, programId);
        return nullptr;
    }

    carla_stderr2("carla_get_program_name(%i, %i) - could not find plugin", pluginId, programId);
    return nullptr;
}

const char* carla_get_midi_program_name(uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_midi_program_name(%i, %i)", pluginId, midiProgramId);

    static char midiProgramName[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->getMidiProgramCount())
        {
            carla_zeroChars(midiProgramName, STR_MAX+1);
            plugin->getMidiProgramName(midiProgramId, midiProgramName);
            return midiProgramName;
        }

        carla_stderr2("carla_get_midi_program_name(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return nullptr;
    }

    carla_stderr2("carla_get_midi_program_name(%i, %i) - could not find plugin", pluginId, midiProgramId);
    return nullptr;
}

const char* carla_get_real_plugin_name(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, nullptr);
    carla_debug("carla_get_real_plugin_name(%i)", pluginId);

    static char realPluginName[STR_MAX+1];

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        carla_zeroChars(realPluginName, STR_MAX+1);
        plugin->getRealName(realPluginName);
        return realPluginName;
    }

    carla_stderr2("carla_get_real_plugin_name(%i) - could not find plugin", pluginId);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int32_t carla_get_current_program_index(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, -1);
    carla_debug("carla_get_current_program_index(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getCurrentProgram();

    carla_stderr2("carla_get_current_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

int32_t carla_get_current_midi_program_index(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, -1);
    carla_debug("carla_get_current_midi_program_index(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getCurrentMidiProgram();

    carla_stderr2("carla_get_current_midi_program_index(%i) - could not find plugin", pluginId);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

float carla_get_default_parameter_value(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);
    carla_debug("carla_get_default_parameter_value(%i, %i)", pluginId, parameterId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->getParameterRanges(parameterId).def;

        carla_stderr2("carla_get_default_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    carla_stderr2("carla_get_default_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

float carla_get_current_parameter_value(uint pluginId, uint32_t parameterId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->getParameterValue(parameterId);

        carla_stderr2("carla_get_current_parameter_value(%i, %i) - parameterId out of bounds", pluginId, parameterId);
        return 0.0f;
    }

    carla_stderr2("carla_get_current_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

float carla_get_internal_parameter_value(uint pluginId, int32_t parameterId)
{
#ifdef BUILD_BRIDGE
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, 0.0f);
#else
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr, (parameterId == CB::PARAMETER_CTRL_CHANNEL) ? -1.0f : 0.0f);
#endif
    CARLA_SAFE_ASSERT_RETURN(parameterId != CB::PARAMETER_NULL && parameterId > CB::PARAMETER_MAX, 0.0f);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->getInternalParameterValue(parameterId);

    carla_stderr2("carla_get_internal_parameter_value(%i, %i) - could not find plugin", pluginId, parameterId);
    return 0.0f;
}

// -------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------

void carla_set_active(uint pluginId, bool onOff)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_active(%i, %s)", pluginId, bool2str(onOff));

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setActive(onOff, true, false);

    carla_stderr2("carla_set_active(%i, %s) - could not find plugin", pluginId, bool2str(onOff));
}

#ifndef BUILD_BRIDGE
void carla_set_drywet(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_drywet(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setDryWet(value, true, false);

    carla_stderr2("carla_set_drywet(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_volume(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_volume(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setVolume(value, true, false);

    carla_stderr2("carla_set_volume(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_left(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_balance_left(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setBalanceLeft(value, true, false);

    carla_stderr2("carla_set_balance_left(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_balance_right(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_balance_right(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setBalanceRight(value, true, false);

    carla_stderr2("carla_set_balance_right(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_panning(uint pluginId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_panning(%i, %f)", pluginId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setPanning(value, true, false);

    carla_stderr2("carla_set_panning(%i, %f) - could not find plugin", pluginId, value);
}

void carla_set_ctrl_channel(uint pluginId, int8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(channel >= -1 && channel < MAX_MIDI_CHANNELS,);
    carla_debug("carla_set_ctrl_channel(%i, %i)", pluginId, channel);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setCtrlChannel(channel, true, false);

    carla_stderr2("carla_set_ctrl_channel(%i, %i) - could not find plugin", pluginId, channel);
}

void carla_set_option(uint pluginId, uint option, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_option(%i, %i, %s)", pluginId, option, bool2str(yesNo));

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setOption(option, yesNo, false);

    carla_stderr2("carla_set_option(%i, %i, %s) - could not find plugin", pluginId, option, bool2str(yesNo));
}
#endif

// -------------------------------------------------------------------------------------------------------------------

void carla_set_parameter_value(uint pluginId, uint32_t parameterId, float value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_parameter_value(%i, %i, %f)", pluginId, parameterId, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->setParameterValue(parameterId, value, true, true, false);

        carla_stderr2("carla_set_parameter_value(%i, %i, %f) - parameterId out of bounds", pluginId, parameterId, value);
        return;
    }

    carla_stderr2("carla_set_parameter_value(%i, %i, %f) - could not find plugin", pluginId, parameterId, value);
}

#ifndef BUILD_BRIDGE
void carla_set_parameter_midi_channel(uint pluginId, uint32_t parameterId, uint8_t channel)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);
    carla_debug("carla_set_parameter_midi_channel(%i, %i, %i)", pluginId, parameterId, channel);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->setParameterMidiChannel(parameterId, channel, true, false);

        carla_stderr2("carla_set_parameter_midi_channel(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, channel);
        return;
    }

    carla_stderr2("carla_set_parameter_midi_channel(%i, %i, %i) - could not find plugin", pluginId, parameterId, channel);
}

void carla_set_parameter_midi_cc(uint pluginId, uint32_t parameterId, int16_t cc)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(cc >= -1 && cc < MAX_MIDI_CONTROL,);
    carla_debug("carla_set_parameter_midi_cc(%i, %i, %i)", pluginId, parameterId, cc);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (parameterId < plugin->getParameterCount())
            return plugin->setParameterMidiCC(parameterId, cc, true, false);

        carla_stderr2("carla_set_parameter_midi_cc(%i, %i, %i) - parameterId out of bounds", pluginId, parameterId, cc);
        return;
    }

    carla_stderr2("carla_set_parameter_midi_cc(%i, %i, %i) - could not find plugin", pluginId, parameterId, cc);
}
#endif

// -------------------------------------------------------------------------------------------------------------------

void carla_set_program(uint pluginId, uint32_t programId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_program(%i, %i)", pluginId, programId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (programId < plugin->getProgramCount())
            return plugin->setProgram(static_cast<int32_t>(programId), true, true, false);

        carla_stderr2("carla_set_program(%i, %i) - programId out of bounds", pluginId, programId);
        return;
    }

    carla_stderr2("carla_set_program(%i, %i) - could not find plugin", pluginId, programId);
}

void carla_set_midi_program(uint pluginId, uint32_t midiProgramId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_set_midi_program(%i, %i)", pluginId, midiProgramId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (midiProgramId < plugin->getMidiProgramCount())
            return plugin->setMidiProgram(static_cast<int32_t>(midiProgramId), true, true, false);

        carla_stderr2("carla_set_midi_program(%i, %i) - midiProgramId out of bounds", pluginId, midiProgramId);
        return;
    }

    carla_stderr2("carla_set_midi_program(%i, %i) - could not find plugin", pluginId, midiProgramId);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_set_custom_data(uint pluginId, const char* type, const char* key, const char* value)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
    CARLA_SAFE_ASSERT_RETURN(value != nullptr,);
    carla_debug("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\")", pluginId, type, key, value);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->setCustomData(type, key, value, true);

    carla_stderr2("carla_set_custom_data(%i, \"%s\", \"%s\", \"%s\") - could not find plugin", pluginId, type, key, value);
}

void carla_set_chunk_data(uint pluginId, const char* chunkData)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(chunkData != nullptr && chunkData[0] != '\0',);
    carla_debug("carla_set_chunk_data(%i, \"%s\")", pluginId, chunkData);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
    {
        if (plugin->getOptionsEnabled() & CB::PLUGIN_OPTION_USE_CHUNKS)
        {
            std::vector<uint8_t> chunk(carla_getChunkFromBase64String(chunkData));
            return plugin->setChunkData(chunk.data(), chunk.size());
        }

        carla_stderr2("carla_set_chunk_data(%i, \"%s\") - plugin does not use chunks", pluginId, chunkData);
        return;
    }

    carla_stderr2("carla_set_chunk_data(%i, \"%s\") - could not find plugin", pluginId, chunkData);
}

// -------------------------------------------------------------------------------------------------------------------

void carla_prepare_for_save(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_prepare_for_save(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->prepareForSave();

    carla_stderr2("carla_prepare_for_save(%i) - could not find plugin", pluginId);
}

void carla_reset_parameters(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_reset_parameters(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->resetParameters();

    carla_stderr2("carla_reset_parameters(%i) - could not find plugin", pluginId);
}

void carla_randomize_parameters(uint pluginId)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_randomize_parameters(%i)", pluginId);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->randomizeParameters();

    carla_stderr2("carla_randomize_parameters(%i) - could not find plugin", pluginId);
}

#ifndef BUILD_BRIDGE
void carla_send_midi_note(uint pluginId, uint8_t channel, uint8_t note, uint8_t velocity)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr && gStandalone.engine->isRunning(),);
    carla_debug("carla_send_midi_note(%i, %i, %i, %i)", pluginId, channel, note, velocity);

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);

    carla_stderr2("carla_send_midi_note(%i, %i, %i, %i) - could not find plugin", pluginId, channel, note, velocity);
}
#endif

void carla_show_custom_ui(uint pluginId, bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(gStandalone.engine != nullptr,);
    carla_debug("carla_show_custom_ui(%i, %s)", pluginId, bool2str(yesNo));

    if (CarlaPlugin* const plugin = gStandalone.engine->getPlugin(pluginId))
        return plugin->showCustomUI(yesNo);

    carla_stderr2("carla_show_custom_ui(%i, %s) - could not find plugin", pluginId, bool2str(yesNo));
}

// -------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------

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

#ifdef HAVE_LIBLO
    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return nullptr;
    }

    return gStandalone.engine->getOscServerPathTCP();
#else
    return nullptr;
#endif
}

const char* carla_get_host_osc_url_udp()
{
    carla_debug("carla_get_host_osc_url_udp()");

#ifdef HAVE_LIBLO
    if (gStandalone.engine == nullptr)
    {
        carla_stderr2("Engine is not running");
        gStandalone.lastError = "Engine is not running";
        return nullptr;
    }

    return gStandalone.engine->getOscServerPathUDP();
#else
    return nullptr;
#endif
}

// -------------------------------------------------------------------------------------------------------------------

#include "CarlaPluginUI.cpp"
#include "CarlaDssiUtils.cpp"
#include "CarlaPatchbayUtils.cpp"
#include "CarlaPipeUtils.cpp"
#include "CarlaStateUtils.cpp"
#include "CarlaJuceEvents.cpp"

// -------------------------------------------------------------------------------------------------------------------
