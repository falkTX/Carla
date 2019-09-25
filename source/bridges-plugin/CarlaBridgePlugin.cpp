﻿/*
 * Carla Bridge Plugin
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef BUILD_BRIDGE
# error This file should not be compiled if not building bridge
#endif

#include "CarlaEngine.hpp"
#include "CarlaHost.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaMainLoop.hpp"
#include "CarlaMIDI.h"

#ifdef CARLA_OS_UNIX
# include <signal.h>
#endif

#ifdef CARLA_OS_LINUX
# include <sched.h>
# define SCHED_RESET_ON_FORK 0x40000000
#endif

#ifdef CARLA_OS_WIN
# include <pthread.h>
# include <objbase.h>
#endif

#ifdef HAVE_X11
# include <X11/Xlib.h>
#endif

#ifdef USING_JUCE
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Weffc++"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
# endif
# include "AppConfig.h"
# if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
#  include "juce_gui_basics/juce_gui_basics.h"
# else
#  include "juce_events/juce_events.h"
# endif
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
#endif

#include "jackbridge/JackBridge.hpp"

#include "water/files/File.h"

using CarlaBackend::CarlaEngine;
using CarlaBackend::EngineCallbackOpcode;
using CarlaBackend::EngineCallbackOpcode2Str;
using CarlaBackend::runMainLoopOnce;

using water::CharPointer_UTF8;
using water::File;
using water::String;

// -------------------------------------------------------------------------

static bool gIsInitiated = false;
static volatile bool gCloseNow = false;
static volatile bool gSaveNow  = false;

#if defined(CARLA_OS_LINUX)
static void closeSignalHandler(int) noexcept
{
    gCloseNow = true;
}
static void saveSignalHandler(int) noexcept
{
    gSaveNow = true;
}
#elif defined(CARLA_OS_WIN)
static BOOL WINAPI winSignalHandler(DWORD dwCtrlType) noexcept
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        gCloseNow = true;
        return TRUE;
    }
    return FALSE;
}
#endif

static void initSignalHandler()
{
#if defined(CARLA_OS_LINUX)
    struct sigaction sig;
    carla_zeroStruct(sig);

    sig.sa_handler = closeSignalHandler;
    sig.sa_flags   = SA_RESTART;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGINT, &sig, nullptr);

    sig.sa_handler = saveSignalHandler;
    sig.sa_flags   = SA_RESTART;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGUSR1, &sig, nullptr);
#elif defined(CARLA_OS_WIN)
    SetConsoleCtrlHandler(winSignalHandler, TRUE);
#endif
}

// -------------------------------------------------------------------------

static String gProjectFilename;

static void gIdle()
{
    carla_engine_idle();

    if (gSaveNow)
    {
        gSaveNow = false;

        if (gProjectFilename.isNotEmpty())
        {
            if (! carla_save_plugin_state(0, gProjectFilename.toRawUTF8()))
                carla_stderr("Plugin preset save failed, error was:\n%s", carla_get_last_error());
        }
    }
}

 // -------------------------------------------------------------------------

#if defined(USING_JUCE) && (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
class CarlaJuceApp : public juce::JUCEApplication,
                     private juce::Timer
{
public:
    CarlaJuceApp()  {}
    ~CarlaJuceApp() {}

    void initialise(const juce::String&) override
    {
        startTimer(8);
    }

    void shutdown() override
    {
        gCloseNow = true;
        stopTimer();
    }

    const juce::String getApplicationName() override
    {
        return "CarlaPlugin";
    }

    const juce::String getApplicationVersion() override
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

static juce::JUCEApplicationBase* juce_CreateApplication() { return new CarlaJuceApp(); }
#endif

// -------------------------------------------------------------------------

class CarlaBridgePlugin
{
public:
    CarlaBridgePlugin(const bool useBridge, const char* const clientName, const char* const audioPoolBaseName,
                      const char* const rtClientBaseName, const char* const nonRtClientBaseName, const char* const nonRtServerBaseName)
        : fEngine(nullptr),
#ifdef USING_JUCE
          fJuceInitialiser(),
#endif
          fUsingBridge(false),
          fUsingExec(false)
    {
        CARLA_ASSERT(clientName != nullptr && clientName[0] != '\0');
        carla_debug("CarlaBridgePlugin::CarlaBridgePlugin(%s, \"%s\", %s, %s, %s, %s)",
                    bool2str(useBridge), clientName, audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);

        carla_set_engine_callback(callback, this);

        if (useBridge)
        {
            carla_engine_init_bridge(audioPoolBaseName,
                                     rtClientBaseName,
                                     nonRtClientBaseName,
                                     nonRtServerBaseName,
                                     clientName);
        }
        else if (std::getenv("CARLA_BRIDGE_DUMMY") != nullptr)
        {
            carla_engine_init("Dummy", clientName);
        }
        else
        {
            carla_engine_init("JACK", clientName);
        }

        fEngine = carla_get_engine();
    }

    ~CarlaBridgePlugin()
    {
        carla_debug("CarlaBridgePlugin::~CarlaBridgePlugin()");

        if (! fUsingExec)
            carla_engine_close();
    }

    bool isOk() const noexcept
    {
        return (fEngine != nullptr);
    }

    // ---------------------------------------------------------------------

    void exec(const bool useBridge)
    {
        fUsingBridge = useBridge;
        fUsingExec   = true;

        if (! useBridge)
        {
            const CarlaPluginInfo* const pInfo(carla_get_plugin_info(0));
            CARLA_SAFE_ASSERT_RETURN(pInfo != nullptr,);

            gProjectFilename  = CharPointer_UTF8(pInfo->name);
            gProjectFilename += ".carxs";

            if (! File::isAbsolutePath(gProjectFilename))
                gProjectFilename = File::getCurrentWorkingDirectory().getChildFile(gProjectFilename).getFullPathName();

            if (File(gProjectFilename).existsAsFile())
            {
                if (carla_load_plugin_state(0, gProjectFilename.toRawUTF8()))
                    carla_stdout("Plugin state loaded successfully");
                else
                    carla_stderr("Plugin state load failed, error was:\n%s", carla_get_last_error());
            }
            else
            {
                carla_stdout("Previous plugin state in '%s' is non-existent, will start from default state",
                             gProjectFilename.toRawUTF8());
            }
        }

        gIsInitiated = true;

#if defined(USING_JUCE) && (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
# ifndef CARLA_OS_WIN
        static const int argc = 0;
        static const char* argv[] = {};
# endif
        juce::JUCEApplicationBase::createInstance = &juce_CreateApplication;
        juce::JUCEApplicationBase::main(JUCE_MAIN_FUNCTION_ARGS);
#else
        for (; runMainLoopOnce() && ! gCloseNow;)
        {
            gIdle();
# if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
            // MacOS and Win32 have event-loops to run, so minimize sleep time
            carla_msleep(1);
# else
            carla_msleep(5);
# endif
        }
#endif

        carla_engine_close();
    }

    // ---------------------------------------------------------------------

protected:
    void handleCallback(const EngineCallbackOpcode action,
                        const int value1,
                        const int, const int, const float, const char* const)
    {
        CARLA_BACKEND_USE_NAMESPACE;

        switch (action)
        {
        case ENGINE_CALLBACK_ENGINE_STOPPED:
        case ENGINE_CALLBACK_PLUGIN_REMOVED:
        case ENGINE_CALLBACK_QUIT:
            gCloseNow = true;
            break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            if (gIsInitiated && value1 != 1 && ! fUsingBridge)
                gCloseNow = true;
            break;

        default:
            break;
        }
    }

private:
    const CarlaEngine* fEngine;

#ifdef USING_JUCE
    const juce::ScopedJuceInitialiser_GUI fJuceInitialiser;
#endif

    bool fUsingBridge;
    bool fUsingExec;

    static void callback(void* ptr, EngineCallbackOpcode action, unsigned int pluginId,
                         int value1, int value2, int value3,
                         float valuef, const char* valueStr)
    {
        carla_debug("CarlaBridgePlugin::callback(%p, %i:%s, %i, %i, %i, %i, %f, \"%s\")",
                    ptr, action, EngineCallbackOpcode2Str(action),
                    pluginId, value1, value2, value3, static_cast<double>(valuef), valueStr);

        // ptr must not be null
        CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // pluginId must be 0 (first), except for patchbay things
        if (action < CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED ||
            action > CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED)
#endif
        {
            CARLA_SAFE_ASSERT_UINT_RETURN(pluginId == 0, pluginId,);
        }

        return ((CarlaBridgePlugin*)ptr)->handleCallback(action, value1, value2, value3, valuef, valueStr);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgePlugin)
};

// -------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // ---------------------------------------------------------------------
    // Check argument count

    if (argc != 4 && argc != 5)
    {
        carla_stdout("usage: %s <type> <filename> <label> [uniqueId]", argv[0]);
        return 1;
    }

#if defined(CARLA_OS_WIN) && ! defined(BUILDING_CARLA_FOR_WINDOWS)
    // ---------------------------------------------------------------------
    // Test if bridge is working

    if (! jackbridge_is_ok())
    {
        carla_stderr("A JACK or Wine library is missing, cannot continue");
        return 1;
    }
#endif

    // ---------------------------------------------------------------------
    // Get args

    const char* const stype    = argv[1];
    const char*       filename = argv[2];
    const char*       label    = argv[3];
    const int64_t     uniqueId = (argc == 5) ? static_cast<int64_t>(std::atoll(argv[4])) : 0;

    if (filename[0] == '\0' || std::strcmp(filename, "(none)") == 0)
        filename = nullptr;

    if (label[0] == '\0' || std::strcmp(label, "(none)") == 0)
        label = nullptr;

    // ---------------------------------------------------------------------
    // Check binary type

    CarlaBackend::BinaryType btype = CarlaBackend::BINARY_NATIVE;

    if (const char* const binaryTypeStr = std::getenv("CARLA_BRIDGE_PLUGIN_BINARY_TYPE"))
        btype = CarlaBackend::getBinaryTypeFromString(binaryTypeStr);

    if (btype == CarlaBackend::BINARY_NONE)
    {
        carla_stderr("Invalid binary type '%i'", btype);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Check plugin type

    CarlaBackend::PluginType itype(CarlaBackend::getPluginTypeFromString(stype));

    if (itype == CarlaBackend::PLUGIN_NONE)
    {
        carla_stderr("Invalid plugin type '%s'", stype);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Set file

    const File file(filename != nullptr ? filename : "");

    // ---------------------------------------------------------------------
    // Set name

    const char* name(std::getenv("CARLA_CLIENT_NAME"));

    if (name != nullptr && (name[0] == '\0' || std::strcmp(name, "(none)") == 0))
        name = nullptr;

    // ---------------------------------------------------------------------
    // Setup options

    const char* const shmIds(std::getenv("ENGINE_BRIDGE_SHM_IDS"));

    const bool useBridge = (shmIds != nullptr);

    // ---------------------------------------------------------------------
    // Setup bridge ids

    char audioPoolBaseName[6+1];
    char rtClientBaseName[6+1];
    char nonRtClientBaseName[6+1];
    char nonRtServerBaseName[6+1];

    if (useBridge)
    {
        CARLA_SAFE_ASSERT_RETURN(std::strlen(shmIds) == 6*4, 1);
        std::strncpy(audioPoolBaseName,   shmIds+6*0, 6);
        std::strncpy(rtClientBaseName,    shmIds+6*1, 6);
        std::strncpy(nonRtClientBaseName, shmIds+6*2, 6);
        std::strncpy(nonRtServerBaseName, shmIds+6*3, 6);
        audioPoolBaseName[6]   = '\0';
        rtClientBaseName[6]    = '\0';
        nonRtClientBaseName[6] = '\0';
        nonRtServerBaseName[6] = '\0';
        jackbridge_parent_deathsig(false);
    }
    else
    {
        audioPoolBaseName[0]   = '\0';
        rtClientBaseName[0]    = '\0';
        nonRtClientBaseName[0] = '\0';
        nonRtServerBaseName[0] = '\0';
        jackbridge_init();
    }

    // ---------------------------------------------------------------------
    // Set client name

    CarlaString clientName;

    if (name != nullptr)
    {
        clientName = name;
    }
    else if (itype == CarlaBackend::PLUGIN_LV2)
    {
        // LV2 requires URI
        CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0', 1);

        // LV2 URI is not usable as client name, create a usable name from URI
        CarlaString label2(label);

        // truncate until last valid char
        for (std::size_t i=label2.length()-1; i != 0; --i)
        {
            if (! std::isalnum(label2[i]))
                continue;

            label2.truncate(i+1);
            break;
        }

        // get last used separator
        bool found;
        std::size_t septmp, sep = 0;

        septmp = label2.rfind('#', &found)+1;
        if (found && septmp > sep)
            sep = septmp;

        septmp = label2.rfind('/', &found)+1;
        if (found && septmp > sep)
            sep = septmp;

        septmp = label2.rfind('=', &found)+1;
        if (found && septmp > sep)
            sep = septmp;

        septmp = label2.rfind(':', &found)+1;
        if (found && septmp > sep)
            sep = septmp;

        // make name starting from the separator and first valid char
        const char* name2 = label2.buffer() + sep;
        for (; *name2 != '\0' && ! std::isalnum(*name2); ++name2) {}

        if (*name2 != '\0')
            clientName = name2;
    }
    else if (label != nullptr)
    {
        clientName = label;
    }
    else
    {
        clientName = file.getFileNameWithoutExtension().toRawUTF8();
    }

    // if we still have no client name by now, use a dummy one
    if (clientName.isEmpty())
        clientName = "carla-plugin";

    // just to be safe
    clientName.toBasic();

    // ---------------------------------------------------------------------
    // Set extraStuff

    const void* extraStuff = nullptr;

    if (itype == CarlaBackend::PLUGIN_SF2)
    {
        if (label == nullptr)
            label = clientName;

        if (std::strstr(label, " (16 outs)") != nullptr)
            extraStuff = "true";
    }

    // ---------------------------------------------------------------------
    // Initialize OS features

#ifdef CARLA_OS_WIN
    OleInitialize(nullptr);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
# ifndef __WINPTHREADS_VERSION
    // (non-portable) initialization of statically linked pthread library
    pthread_win32_process_attach_np();
    pthread_win32_thread_attach_np();
# endif
#endif

#ifdef HAVE_X11
    if (std::getenv("DISPLAY") != nullptr)
        XInitThreads();
#endif

    // ---------------------------------------------------------------------
    // Set ourselves with high priority

#ifdef CARLA_OS_LINUX
    // reset scheduler to normal mode
    struct sched_param sparam;
    carla_zeroStruct(sparam);
    sched_setscheduler(0, SCHED_OTHER|SCHED_RESET_ON_FORK, &sparam);

    // try niceness first, if it fails, try SCHED_RR
    if (nice(-5) < 0)
    {
        sparam.sched_priority = (sched_get_priority_max(SCHED_RR) + sched_get_priority_min(SCHED_RR*7)) / 8;

        if (sparam.sched_priority > 0)
        {
            if (sched_setscheduler(0, SCHED_RR|SCHED_RESET_ON_FORK, &sparam) < 0)
            {
                CarlaString error(std::strerror(errno));
                carla_stderr("Failed to set high priority, error %i: %s", errno, error.buffer());
            }
        }
    }
#endif

#ifdef CARLA_OS_WIN
    if (! SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS))
        carla_stderr("Failed to set high priority.");
#endif

    // ---------------------------------------------------------------------
    // Listen for ctrl+c or sigint/sigterm events

    initSignalHandler();

    // ---------------------------------------------------------------------
    // Init plugin bridge

    int ret;

    {
        CarlaBridgePlugin bridge(useBridge, clientName,
                                 audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);

        if (! bridge.isOk())
        {
            carla_stderr("Failed to init engine, error was:\n%s", carla_get_last_error());
            return 1;
        }

        // -----------------------------------------------------------------
        // Init plugin

        if (carla_add_plugin(btype, itype, file.getFullPathName().toRawUTF8(), name, label, uniqueId, extraStuff, 0x0))
        {
            ret = 0;

            if (! useBridge)
            {
                carla_set_active(0, true);
                carla_set_option(0, CarlaBackend::PLUGIN_OPTION_SEND_CONTROL_CHANGES, true);

                if (const CarlaPluginInfo* const pluginInfo = carla_get_plugin_info(0))
                {
                    if (pluginInfo->hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI)
                    {
#ifdef HAVE_X11
                        if (std::getenv("DISPLAY") != nullptr)
#endif
                            carla_show_custom_ui(0, true);
                    }
                }
            }

            bridge.exec(useBridge);
        }
        else
        {
            ret = 1;

            const char* const lastError(carla_get_last_error());
            carla_stderr("Plugin failed to load, error was:\n%s", lastError);

            if (useBridge)
            {
                // do a single idle so that we can send error message to server
                gIdle();

#ifdef CARLA_OS_UNIX
                // kill ourselves now if we can't load plugin in bridge mode
                ::kill(::getpid(), SIGKILL);
#endif
            }
        }
    }

#ifdef CARLA_OS_WIN
#ifndef __WINPTHREADS_VERSION
    pthread_win32_thread_detach_np();
    pthread_win32_process_detach_np();
#endif
    CoUninitialize();
    OleUninitialize();
#endif

    return ret;
}
