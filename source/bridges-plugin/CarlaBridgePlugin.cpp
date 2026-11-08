// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BUILD_BRIDGE
# error This file should not be compiled if not building bridge
#endif

#include "CarlaEngine.hpp"
#include "CarlaHost.h"
#include "CarlaUtils.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaMainLoop.hpp"

#include "CarlaMIDI.h"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
#endif

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

#include "extra/Sleep.hpp"

#include "water/files/File.h"
#include "water/misc/Time.h"

// must be last
#include "jackbridge/JackBridge.hpp"

using CARLA_BACKEND_NAMESPACE::CarlaEngine;
using CARLA_BACKEND_NAMESPACE::EngineCallbackOpcode;
using CARLA_BACKEND_NAMESPACE::EngineCallbackOpcode2Str;
using CARLA_BACKEND_NAMESPACE::runMainLoopOnce;

using water::CharPointer_UTF8;
using water::File;

// -------------------------------------------------------------------------

static bool gIsInitiated = false;
static volatile bool gCloseBridge = false;
static volatile bool gCloseSignal = false;
static volatile bool gSaveNow  = false;

#if defined(CARLA_OS_UNIX)
static void closeSignalHandler(int) noexcept
{
    gCloseSignal = true;
}
static void saveSignalHandler(int) noexcept
{
    gSaveNow = true;
}
#elif defined(CARLA_OS_WIN)
static LONG WINAPI winExceptionFilter(_EXCEPTION_POINTERS*)
{
    return EXCEPTION_EXECUTE_HANDLER;
}

static BOOL WINAPI winSignalHandler(DWORD dwCtrlType) noexcept
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        gCloseSignal = true;
        return TRUE;
    }
    return FALSE;
}
#endif

static void initSignalHandler()
{
#if defined(CARLA_OS_UNIX)
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
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(winExceptionFilter);
#endif
}

// -------------------------------------------------------------------------

static CarlaHostHandle gHostHandle;
static String gProjectFilename;

static void gIdle()
{
    carla_engine_idle(gHostHandle);

    if (gSaveNow)
    {
        gSaveNow = false;

        if (gProjectFilename.isNotEmpty())
        {
            if (! carla_save_plugin_state(gHostHandle, 0, gProjectFilename))
                carla_stderr("Plugin preset save failed, error was:\n%s", carla_get_last_error(gHostHandle));
        }
    }
}

// -------------------------------------------------------------------------

class CarlaBridgePlugin
{
public:
    CarlaBridgePlugin(const bool useBridge, const char* const clientName, const char* const audioPoolBaseName,
                      const char* const rtClientBaseName, const char* const nonRtClientBaseName, const char* const nonRtServerBaseName)
        : fEngine(nullptr),
          fUsingBridge(false),
          fUsingExec(false)
    {
        CARLA_ASSERT(clientName != nullptr && clientName[0] != '\0');
        carla_debug("CarlaBridgePlugin::CarlaBridgePlugin(%s, \"%s\", %s, %s, %s, %s)",
                    bool2str(useBridge), clientName, audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);

        carla_set_engine_callback(gHostHandle, callback, this);

        if (useBridge)
        {
            carla_engine_init_bridge(gHostHandle,
                                     audioPoolBaseName,
                                     rtClientBaseName,
                                     nonRtClientBaseName,
                                     nonRtServerBaseName,
                                     clientName);
        }
        else if (std::getenv("CARLA_BRIDGE_DUMMY") != nullptr)
        {
            carla_engine_init(gHostHandle, "Dummy", clientName);
        }
        else
        {
            carla_engine_init(gHostHandle, "JACK", clientName);
        }

        fEngine = carla_get_engine_from_handle(gHostHandle);
    }

    ~CarlaBridgePlugin()
    {
        carla_debug("CarlaBridgePlugin::~CarlaBridgePlugin()");

        if (fEngine != nullptr && ! fUsingExec)
            carla_engine_close(gHostHandle);
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

        const bool testing = std::getenv("CARLA_BRIDGE_TESTING") != nullptr;

        if (! useBridge && ! testing)
        {
            const CarlaPluginInfo* const pInfo = carla_get_plugin_info(gHostHandle, 0);
            CARLA_SAFE_ASSERT_RETURN(pInfo != nullptr,);

            gProjectFilename  = pInfo->name;
            gProjectFilename += ".carxs";

            if (! File::isAbsolutePath(gProjectFilename))
                gProjectFilename = File::getCurrentWorkingDirectory().getChildFile(gProjectFilename).getFullPathName().toRawUTF8();

            if (File(gProjectFilename).existsAsFile())
            {
                if (carla_load_plugin_state(gHostHandle, 0, gProjectFilename))
                    carla_stdout("Plugin state loaded successfully");
                else
                    carla_stderr("Plugin state load failed, error was:\n%s", carla_get_last_error(gHostHandle));
            }
            else
            {
                carla_stdout("Previous plugin state in '%s' is non-existent, will start from default state",
                             gProjectFilename.buffer());
            }
        }

        gIsInitiated = true;

        int64_t timeToEnd = 0;

        if (testing)
        {
            timeToEnd = water::Time::currentTimeMillis() + 5 * 1000;
            fEngine->transportPlay();
        }

        for (; runMainLoopOnce() && ! gCloseBridge;)
        {
            gIdle();
           #if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
            // MacOS and Win32 have event-loops to run, so minimize sleep time
            d_msleep(1);
           #else
            d_msleep(5);
           #endif
            if (testing && timeToEnd - water::Time::currentTimeMillis() < 0)
                break;
            if (gCloseSignal && ! fUsingBridge)
                break;
        }

        carla_engine_close(gHostHandle);
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
            gCloseBridge = gCloseSignal = true;
            break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            if (gIsInitiated && value1 != 1 && ! fUsingBridge)
                gCloseBridge = gCloseSignal = true;
            break;

        default:
            break;
        }
    }

private:
    CarlaEngine* fEngine;

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
        if (action < CARLA_BACKEND_NAMESPACE::ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED ||
            action > CARLA_BACKEND_NAMESPACE::ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED)
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

#if defined(CARLA_OS_WIN) && defined(BUILDING_CARLA_FOR_WINE)
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

    CARLA_BACKEND_NAMESPACE::BinaryType btype = CARLA_BACKEND_NAMESPACE::BINARY_NATIVE;

    if (const char* const binaryTypeStr = std::getenv("CARLA_BRIDGE_PLUGIN_BINARY_TYPE"))
        btype = CARLA_BACKEND_NAMESPACE::getBinaryTypeFromString(binaryTypeStr);

    if (btype == CARLA_BACKEND_NAMESPACE::BINARY_NONE)
    {
        carla_stderr("Invalid binary type '%i'", btype);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Check plugin type

    CARLA_BACKEND_NAMESPACE::PluginType itype = CARLA_BACKEND_NAMESPACE::getPluginTypeFromString(stype);

    if (itype == CARLA_BACKEND_NAMESPACE::PLUGIN_NONE)
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

    String clientName;

    if (name != nullptr)
    {
        clientName = name;
    }
    else if (itype == CARLA_BACKEND_NAMESPACE::PLUGIN_LV2)
    {
        // LV2 requires URI
        CARLA_SAFE_ASSERT_RETURN(label != nullptr && label[0] != '\0', 1);

        // LV2 URI is not usable as client name, create a usable name from URI
        String label2(label);

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

    if (itype == CARLA_BACKEND_NAMESPACE::PLUGIN_SF2)
    {
        if (label == nullptr)
            label = clientName;

        if (std::strstr(label, " (16 outs)") != nullptr)
            extraStuff = "true";
    }

    // ---------------------------------------------------------------------
    // Initialize OS features

    const bool dummy = std::getenv("CARLA_BRIDGE_DUMMY") != nullptr;
    const bool testing = std::getenv("CARLA_BRIDGE_TESTING") != nullptr;

#ifdef CARLA_OS_MAC
    CARLA_BACKEND_NAMESPACE::initStandaloneApplication();
#endif

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

    if (!dummy && !testing)
    {
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
                    String error(std::strerror(errno));
                    carla_stderr("Failed to set high priority, error %i: %s", errno, error.buffer());
                }
            }
        }
#endif

#ifdef CARLA_OS_WIN
        if (! SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS))
            carla_stderr("Failed to set high priority.");
#endif
    }

    // ---------------------------------------------------------------------
    // Listen for ctrl+c or sigint/sigterm events

    initSignalHandler();

    // ---------------------------------------------------------------------
    // Init plugin bridge

    int ret;

    {
        gHostHandle = carla_standalone_host_init();

        CarlaBridgePlugin bridge(useBridge, clientName,
                                 audioPoolBaseName, rtClientBaseName, nonRtClientBaseName, nonRtServerBaseName);

        if (! bridge.isOk())
        {
            carla_stderr("Failed to init engine, error was:\n%s", carla_get_last_error(gHostHandle));
            return 1;
        }

        if (! useBridge && ! testing)
        {
#ifdef HAVE_X11
            if (std::getenv("DISPLAY") != nullptr)
#endif
                carla_set_engine_option(gHostHandle,
                                        CARLA_BACKEND_NAMESPACE::ENGINE_OPTION_FRONTEND_UI_SCALE,
                                        static_cast<int>(carla_get_desktop_scale_factor()*1000+0.5),
                                        nullptr);
        }

        // -----------------------------------------------------------------
        // Init plugin

        if (carla_add_plugin(gHostHandle,
                             btype, itype,
                             file.getFullPathName().toRawUTF8(), name, label, uniqueId, extraStuff,
                             CARLA_BACKEND_NAMESPACE::PLUGIN_OPTIONS_NULL))
        {
            ret = 0;

            if (! useBridge)
            {
                carla_set_active(gHostHandle, 0, true);
                carla_set_engine_option(gHostHandle, CARLA_BACKEND_NAMESPACE::ENGINE_OPTION_PLUGINS_ARE_STANDALONE, 1, nullptr);

                if (const CarlaPluginInfo* const pluginInfo = carla_get_plugin_info(gHostHandle, 0))
                {
                    if (itype == CARLA_BACKEND_NAMESPACE::PLUGIN_INTERNAL && (std::strcmp(label, "audiofile") == 0 || std::strcmp(label, "midifile") == 0))
                    {
                        if (file.exists())
                            carla_set_custom_data(gHostHandle, 0,
                                                  CARLA_BACKEND_NAMESPACE::CUSTOM_DATA_TYPE_STRING,
                                                  "file", file.getFullPathName().toRawUTF8());
                    }
                    else if (pluginInfo->hints & CARLA_BACKEND_NAMESPACE::PLUGIN_HAS_CUSTOM_UI)
                    {
#ifdef HAVE_X11
                        if (std::getenv("DISPLAY") != nullptr)
#endif
                            if (! testing)
                                carla_show_custom_ui(gHostHandle, 0, true);
                    }

                    // on standalone usage, enable everything that makes sense
                    const uint optsAvailable = pluginInfo->optionsAvailable;
                    if (optsAvailable & CARLA_BACKEND_NAMESPACE::PLUGIN_OPTION_FIXED_BUFFERS)
                        carla_set_option(gHostHandle, 0, CARLA_BACKEND_NAMESPACE::PLUGIN_OPTION_FIXED_BUFFERS, true);
                }
            }

            bridge.exec(useBridge);
        }
        else
        {
            ret = 1;

            const char* const lastError(carla_get_last_error(gHostHandle));
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
