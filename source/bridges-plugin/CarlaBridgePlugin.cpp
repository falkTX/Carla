/*
 * Carla Bridge Plugin
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

#include "CarlaEngine.hpp"
#include "CarlaHost.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaOscUtils.hpp"
#include "CarlaMIDI.h"

#ifdef CARLA_OS_UNIX
# include <signal.h>
#endif

#include "juce_core.h"

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
# include "juce_gui_basics.h"
using juce::JUCEApplication;
using juce::JUCEApplicationBase;
using juce::Timer;
#endif

using CarlaBackend::CarlaEngine;
using CarlaBackend::EngineCallbackOpcode;
using CarlaBackend::EngineCallbackOpcode2Str;

using juce::File;
using juce::String;

// -------------------------------------------------------------------------

static bool gIsInitiated = false;
static volatile bool gCloseNow = false;
static volatile bool gSaveNow  = false;

#ifdef CARLA_OS_WIN
static BOOL WINAPI winSignalHandler(DWORD dwCtrlType) noexcept
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        gCloseNow = true;
        return TRUE;
    }
    return FALSE;
}
#elif defined(CARLA_OS_LINUX)
static void closeSignalHandler(int) noexcept
{
    gCloseNow = true;
}
static void saveSignalHandler(int) noexcept
{
    gSaveNow = true;
}
#endif

static void initSignalHandler()
{
#ifdef CARLA_OS_WIN
    SetConsoleCtrlHandler(winSignalHandler, TRUE);
#elif defined(CARLA_OS_LINUX)
    struct sigaction sint;
    struct sigaction sterm;
    struct sigaction susr1;

    sint.sa_handler  = closeSignalHandler;
    sint.sa_flags    = SA_RESTART;
    sint.sa_restorer = nullptr;
    sigemptyset(&sint.sa_mask);
    sigaction(SIGINT, &sint, nullptr);

    sterm.sa_handler  = closeSignalHandler;
    sterm.sa_flags    = SA_RESTART;
    sterm.sa_restorer = nullptr;
    sigemptyset(&sterm.sa_mask);
    sigaction(SIGTERM, &sterm, nullptr);

    susr1.sa_handler  = saveSignalHandler;
    susr1.sa_flags    = SA_RESTART;
    susr1.sa_restorer = nullptr;
    sigemptyset(&susr1.sa_mask);
    sigaction(SIGUSR1, &susr1, nullptr);
#endif
}

// -------------------------------------------------------------------------

static CarlaString gProjectFilename;

static void gIdle()
{
    carla_engine_idle();

    if (gSaveNow)
    {
        gSaveNow = false;

        if (gProjectFilename.isNotEmpty())
        {
            if (! carla_save_plugin_state(0, gProjectFilename))
                carla_stderr("Plugin preset save failed, error was:\n%s", carla_get_last_error());
        }
    }
}

// -------------------------------------------------------------------------

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
class CarlaJuceApp : public JUCEApplication,
                     private Timer
{
public:
    CarlaJuceApp()  {}
    ~CarlaJuceApp() {}

    void initialise(const String&) override
    {
        startTimer(15);
    }

    void shutdown() override
    {
        gCloseNow = true;
        stopTimer();
    }

    const String getApplicationName() override
    {
        return "CarlaPlugin";
    }

    const String getApplicationVersion() override
    {
        return CARLA_VERSION_STRING;
    }

    void timerCallback() override
    {
        gIdle();

        if (gCloseNow)
        {
            quit();
            gCloseNow = false;
        }
    }
};

static JUCEApplicationBase* juce_CreateApplication() { return new CarlaJuceApp(); }
#endif

// -------------------------------------------------------------------------

class CarlaBridgePlugin
{
public:
    CarlaBridgePlugin(const bool useBridge, const char* const clientName, const char* const audioPoolBaseName, const char* const rtBaseName, const char* const nonRtBaseName)
        : fEngine(nullptr),
          fProjFilename(),
          fOscControlData(),
          fOscServerPath(),
          fOscServerThread(nullptr),
          leakDetector_CarlaBridgePlugin()
    {
        CARLA_ASSERT(clientName != nullptr && clientName[0] != '\0');
        carla_debug("CarlaBridgePlugin::CarlaBridgePlugin(%s, \"%s\", %s, %s, %s)", bool2str(useBridge), clientName, audioPoolBaseName, rtBaseName, nonRtBaseName);

        carla_set_engine_callback(callback, this);

        if (useBridge)
            carla_engine_init_bridge(audioPoolBaseName, rtBaseName, nonRtBaseName, clientName);
        else
            carla_engine_init("JACK", clientName);

        fEngine = carla_get_engine();
    }

    ~CarlaBridgePlugin()
    {
        carla_debug("CarlaBridgePlugin::~CarlaBridgePlugin()");

        carla_engine_close();
    }

    bool isOk() const noexcept
    {
        return (fEngine != nullptr);
    }

    // ---------------------------------------------------------------------

    void oscInit(const char* const url)
    {
        fOscServerThread = lo_server_thread_new_with_proto(nullptr, LO_UDP, osc_error_handler);

        CARLA_SAFE_ASSERT_RETURN(fOscServerThread != nullptr,)

        {
            char* const host = lo_url_get_hostname(url);
            char* const port = lo_url_get_port(url);
            fOscControlData.path   = carla_strdup_free(lo_url_get_path(url));
            fOscControlData.target = lo_address_new_with_proto(LO_UDP, host, port);

            std::free(host);
            std::free(port);
        }

        if (char* const tmpServerPath = lo_server_thread_get_url(fOscServerThread))
        {
            std::srand((uint)(uintptr_t)this);
            std::srand((uint)(uintptr_t)&url);

            CarlaString oscName("plug-" + CarlaString(std::rand() % 99999));

            fOscServerPath  = tmpServerPath;
            fOscServerPath += oscName;
            std::free(tmpServerPath);
        }

        lo_server_thread_start(fOscServerThread);

        fEngine->setOscBridgeData(&fOscControlData);
    }

    void oscClose()
    {
        lo_server_thread_stop(fOscServerThread);

        fEngine->setOscBridgeData(nullptr);

        if (fOscServerThread != nullptr)
        {
            lo_server_thread_free(fOscServerThread);
            fOscServerThread = nullptr;
        }

        fOscControlData.clear();
        fOscServerPath.clear();
    }

    // ---------------------------------------------------------------------

    void sendOscUpdate() const noexcept
    {
        if (fOscControlData.target != nullptr)
            osc_send_update(fOscControlData, fOscServerPath);
    }

    void sendOscBridgeUpdate() const noexcept
    {
        if (fOscControlData.target != nullptr)
            osc_send_bridge_ready(fOscControlData, fOscControlData.path);
    }

    void sendOscBridgeError(const char* const error) const noexcept
    {
        if (fOscControlData.target != nullptr)
            osc_send_bridge_error(fOscControlData, error);
    }

    // ---------------------------------------------------------------------

    void exec(const bool useOsc, int argc, char* argv[])
    {
        if (! useOsc)
        {
            const CarlaPluginInfo* const pInfo(carla_get_plugin_info(0));
            CARLA_SAFE_ASSERT_RETURN(pInfo != nullptr,);

            fProjFilename  = pInfo->name;
            fProjFilename += ".carxs";

            if (! File::isAbsolutePath(fProjFilename))
                fProjFilename = File::getCurrentWorkingDirectory().getChildFile(fProjFilename).getFullPathName();

            if (File(fProjFilename).existsAsFile() && ! carla_load_plugin_state(0, fProjFilename.toRawUTF8()))
                carla_stderr("Plugin preset load failed, error was:\n%s", carla_get_last_error());
        }

        gIsInitiated = true;

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
        JUCEApplicationBase::createInstance = &juce_CreateApplication;
        JUCEApplicationBase::main(JUCE_MAIN_FUNCTION_ARGS);
#else
        for (; ! gCloseNow;)
        {
            gIdle();
            carla_msleep(15);
        }
#endif

        carla_set_engine_about_to_close();
        carla_remove_plugin(0);

        // may be unused
        return; (void)argc; (void)argv;
    }

    // ---------------------------------------------------------------------

protected:
    void handleCallback(const EngineCallbackOpcode action, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        CARLA_BACKEND_USE_NAMESPACE;

        // TODO

        switch (action)
        {
        case ENGINE_CALLBACK_ENGINE_STOPPED:
        case ENGINE_CALLBACK_PLUGIN_REMOVED:
        case ENGINE_CALLBACK_QUIT:
            gCloseNow = true;
            break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            if (gIsInitiated && value1 != 1 && fOscControlData.target == nullptr)
                gCloseNow = true;
            break;

        default:
            break;
        }

        return;
        (void)value2;
        (void)value3;
        (void)valueStr;
    }

private:
    const CarlaEngine* fEngine;
    String             fProjFilename;

    CarlaOscData     fOscControlData;
    CarlaString      fOscServerPath;
    lo_server_thread fOscServerThread;

    static void callback(void* ptr, EngineCallbackOpcode action, unsigned int pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        carla_debug("CarlaBridgePlugin::callback(%p, %i:%s, %i, %i, %i, %f, \"%s\")", ptr, action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
        CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(pluginId == 0,);

        return ((CarlaBridgePlugin*)ptr)->handleCallback(action, value1, value2, value3, valueStr);
    }

    static void osc_error_handler(int num, const char* msg, const char* path)
    {
        carla_stderr("CarlaBridgePlugin::osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgePlugin)
};

// -------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // ---------------------------------------------------------------------
    // Check argument count

    if (argc != 5)
    {
        carla_stdout("usage: %s <type> <filename> <label> <uniqueId>", argv[0]);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Get args

    const char* const stype    = argv[1];
    const char* const filename = argv[2];
    const char*       label    = argv[3];
    const int64_t     uniqueId = static_cast<int64_t>(std::atoll(argv[4]));

    // ---------------------------------------------------------------------
    // Check plugin type

    CarlaBackend::PluginType itype(CarlaBackend::getPluginTypeFromString(stype));

    if (itype == CarlaBackend::PLUGIN_NONE)
    {
        carla_stderr("Invalid plugin type '%s'", stype);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Set name

    const char* name(std::getenv("CARLA_CLIENT_NAME"));

    if (name != nullptr && (name[0] == '\0' || std::strcmp(name, "(none)") == 0))
        name = nullptr;

    // ---------------------------------------------------------------------
    // Setup options

    const char* const oscUrl(std::getenv("ENGINE_BRIDGE_OSC_URL"));
    const char* const shmIds(std::getenv("ENGINE_BRIDGE_SHM_IDS"));

    const bool useBridge = (shmIds != nullptr && oscUrl != nullptr);

    // ---------------------------------------------------------------------
    // Setup bridge ids

    char bridgeBaseAudioName[6+1];
    char bridgeBaseControlName[6+1];
    char bridgeBaseTimeName[6+1];

    if (useBridge)
    {
        CARLA_SAFE_ASSERT_RETURN(std::strlen(shmIds) == 6*3, 1);
        std::strncpy(bridgeBaseAudioName,   shmIds,    6);
        std::strncpy(bridgeBaseControlName, shmIds+6,  6);
        std::strncpy(bridgeBaseTimeName,    shmIds+12, 6);
        bridgeBaseAudioName[6]   = '\0';
        bridgeBaseControlName[6] = '\0';
        bridgeBaseTimeName[6]    = '\0';
    }
    else
    {
        bridgeBaseAudioName[0]   = '\0';
        bridgeBaseControlName[0] = '\0';
        bridgeBaseTimeName[0]    = '\0';
    }

    // ---------------------------------------------------------------------
    // Set client name

    CarlaString clientName(name != nullptr ? name : label);

    if (clientName.isEmpty())
        clientName = juce::File(filename).getFileNameWithoutExtension().toRawUTF8();

    // ---------------------------------------------------------------------
    // Set extraStuff

    const void* extraStuff = nullptr;

    if (itype == CarlaBackend::PLUGIN_GIG || itype == CarlaBackend::PLUGIN_SF2)
    {
        if (label == nullptr)
            label = clientName;

        if (std::strstr(label, " (16 outs)") != nullptr)
            extraStuff = "true";
    }

    // ---------------------------------------------------------------------
    // Init plugin bridge

    CarlaBridgePlugin bridge(useBridge, clientName, bridgeBaseAudioName, bridgeBaseControlName, bridgeBaseTimeName);

    if (! bridge.isOk())
    {
        carla_stderr("Failed to init engine, error was:\n%s", carla_get_last_error());
        return 1;
    }

    // ---------------------------------------------------------------------
    // Init OSC

    if (useBridge)
        bridge.oscInit(oscUrl);

    // ---------------------------------------------------------------------
    // Listen for ctrl+c or sigint/sigterm events

    initSignalHandler();

    // ---------------------------------------------------------------------
    // Init plugin

    int ret;

    if (carla_add_plugin(CarlaBackend::BINARY_NATIVE, itype, filename, name, label, uniqueId, extraStuff))
    {
        ret = 0;

        if (useBridge)
        {
            bridge.sendOscUpdate();
            bridge.sendOscBridgeUpdate();
        }
        else
        {
            carla_set_active(0, true);

            if (const CarlaPluginInfo* const pluginInfo = carla_get_plugin_info(0))
            {
                if (pluginInfo->hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI)
                    carla_show_custom_ui(0, true);
            }
        }

        bridge.exec(useBridge, argc, argv);
    }
    else
    {
        ret = 1;

        const char* const lastError(carla_get_last_error());
        carla_stderr("Plugin failed to load, error was:\n%s", lastError);

        if (useBridge)
            bridge.sendOscBridgeError(lastError);
    }

    // ---------------------------------------------------------------------
    // Close OSC

    if (useBridge)
        bridge.oscClose();

    return ret;
}
