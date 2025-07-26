// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaHostImpl.hpp"

#if defined(HAVE_LIBLO) && !defined(CARLA_OS_WASM)

#define CARLA_ENABLE_STANDALONE_NSM

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 2

// #define NSM_CLIENT_FEATURES ":switch:"
#define NSM_CLIENT_FEATURES ":switch:optional-gui:"

#include "CarlaOscUtils.hpp"

#include "water/files/File.h"

// -------------------------------------------------------------------------------------------------------------------

class CarlaNSM
{
public:
    CarlaNSM(CarlaHostStandalone& shandle) noexcept
        : gStandalone(shandle),
          fReplyAddress(nullptr),
          fServer(nullptr),
          fServerThread(nullptr),
          fServerURL(nullptr),
          fClientNameId(),
          fProjectPath(),
          fHasBroadcast(false),
          fHasOptionalGui(false),
          fHasServerControl(false),
          fStarted(),
          fReadyActionOpen(true),
          fReadyActionSave(true) {}

    ~CarlaNSM()
    {
        CARLA_SAFE_ASSERT(fReadyActionOpen);
        CARLA_SAFE_ASSERT(fReadyActionSave);

        if (fServerThread != nullptr)
        {
            lo_server_thread_stop(fServerThread);
            lo_server_thread_free(fServerThread);
            fServerThread = nullptr;
            fServer = nullptr;
        }

        if (fServerURL != nullptr)
        {
            std::free(fServerURL);
            fServerURL = nullptr;
        }

        if (fReplyAddress != nullptr)
        {
            lo_address_free(fReplyAddress);
            fReplyAddress = nullptr;
        }
    }

    bool announce(const uint64_t pid, const char* const executableName)
    {
        CARLA_SAFE_ASSERT_RETURN(pid != 0, false);
        CARLA_SAFE_ASSERT_RETURN(executableName != nullptr && executableName[0] != '\0', false);

        const char* const NSM_URL(std::getenv("NSM_URL"));

        if (NSM_URL == nullptr)
            return false;

        const lo_address nsmAddress(lo_address_new_from_url(NSM_URL));
        CARLA_SAFE_ASSERT_RETURN(nsmAddress != nullptr, false);

        const int proto = lo_address_get_protocol(nsmAddress);

        if (fServerThread == nullptr)
        {
            // create new OSC server
            fServerThread = lo_server_thread_new_with_proto(nullptr, proto, _osc_error_handler);
            CARLA_SAFE_ASSERT_RETURN(fServerThread != nullptr, false);

            // register message handlers
            lo_server_thread_add_method(fServerThread, "/error",                       "sis",    _error_handler,     this);
            lo_server_thread_add_method(fServerThread, "/reply",                       "ssss",   _reply_handler,     this);
            lo_server_thread_add_method(fServerThread, "/nsm/client/open",             "sss",    _open_handler,      this);
            lo_server_thread_add_method(fServerThread, "/nsm/client/save",              "",      _save_handler,      this);
            lo_server_thread_add_method(fServerThread, "/nsm/client/session_is_loaded", "",      _loaded_handler,    this);
            lo_server_thread_add_method(fServerThread, "/nsm/client/show_optional_gui", "",      _show_gui_handler,  this);
            lo_server_thread_add_method(fServerThread, "/nsm/client/hide_optional_gui", "",      _hide_gui_handler,  this);
            lo_server_thread_add_method(fServerThread, nullptr,                         nullptr, _broadcast_handler, this);

            fServer    = lo_server_thread_get_server(fServerThread);
            fServerURL = lo_server_thread_get_url(fServerThread);
        }

        const char* appName = std::getenv("CARLA_NSM_NAME");

        if (appName == nullptr)
            appName = "Carla";

        lo_send_from(nsmAddress, fServer, LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                     appName, NSM_CLIENT_FEATURES, executableName, NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid);

        lo_address_free(nsmAddress);

        if (gStandalone.engineCallback != nullptr)
        {
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_INIT,
                                       0, 0, 0.0f, nullptr);
        }

        return true;
    }

    void ready(const NsmCallbackOpcode action)
    {
        CARLA_SAFE_ASSERT_RETURN(fServerThread != nullptr,);

        switch (action)
        {
        case CB::NSM_CALLBACK_INIT:
            CARLA_SAFE_ASSERT_BREAK(! fStarted);
            fStarted = true;
            lo_server_thread_start(fServerThread);
            break;

        case CB::NSM_CALLBACK_ERROR:
            break;

        case CB::NSM_CALLBACK_ANNOUNCE:
            break;

        case CB::NSM_CALLBACK_OPEN:
            fReadyActionOpen = true;
            break;

        case CB::NSM_CALLBACK_SAVE:
            fReadyActionSave = true;
            break;

        case CB::NSM_CALLBACK_SESSION_IS_LOADED:
            break;

        case CB::NSM_CALLBACK_SHOW_OPTIONAL_GUI:
            CARLA_SAFE_ASSERT_BREAK(fReplyAddress != nullptr);
            CARLA_SAFE_ASSERT_BREAK(fServer != nullptr);
            {
                // NOTE: lo_send_from is a macro that creates local variables
                lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/client/gui_is_shown", "");
            }
            break;

        case CB::NSM_CALLBACK_HIDE_OPTIONAL_GUI:
            CARLA_SAFE_ASSERT_BREAK(fReplyAddress != nullptr);
            CARLA_SAFE_ASSERT_BREAK(fServer != nullptr);
            {
                // NOTE: lo_send_from is a macro that creates local variables
                lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/client/gui_is_hidden", "");
            }
            break;

        case CB::NSM_CALLBACK_SET_CLIENT_NAME_ID:
            break;
        }
    }

    static CarlaNSM& getInstance(CarlaHostStandalone& shandle)
    {
        static CarlaNSM sInstance(shandle);
        return sInstance;
    }

protected:
    int handleError(const char* const method, const int code, const char* const message)
    {
        carla_stdout("CarlaNSM::handleError(\"%s\", %i, \"%s\")", method, code, message);

        if (gStandalone.engineCallback != nullptr)
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_ERROR,
                                       code,
                                       0, 0.0f,
                                       message);

        return 0;

        // may be unused
        (void)method;
    }

    int handleReply(const char* const method, const char* const message, const char* const smName, const char* const features,
                    const lo_message msg)
    {
        CARLA_SAFE_ASSERT_RETURN(fServerThread != nullptr, 1);
        carla_stdout("CarlaNSM::handleReply(\"%s\", \"%s\", \"%s\", \"%s\")", method, message, smName, features);

        if (std::strcmp(method, "/nsm/server/announce") == 0)
        {
            const lo_address msgAddress(lo_message_get_source(msg));
            CARLA_SAFE_ASSERT_RETURN(msgAddress != nullptr, 0);

            char* const msgURL(lo_address_get_url(msgAddress));
            CARLA_SAFE_ASSERT_RETURN(msgURL != nullptr, 0);

            if (fReplyAddress != nullptr)
                lo_address_free(fReplyAddress);

            fReplyAddress = lo_address_new_from_url(msgURL);
            CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 0);

            fHasBroadcast     = std::strstr(features, ":broadcast:")      != nullptr;
            fHasOptionalGui   = std::strstr(features, ":optional-gui:")   != nullptr;
            fHasServerControl = std::strstr(features, ":server-control:") != nullptr;

            // UI starts hidden
            if (fHasOptionalGui)
            {
                // NOTE: lo_send_from is a macro that creates local variables
                lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/client/gui_is_hidden", "");
            }

            carla_stdout("Carla started via '%s', message: %s", smName, message);

            if (gStandalone.engineCallback != nullptr)
            {
                int flags = 0;

                if (fHasBroadcast)
                    flags |= 1 << 0;
                if (fHasOptionalGui)
                    flags |= 1 << 1;
                if (fHasServerControl)
                    flags |= 1 << 2;

                gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                           CB::ENGINE_CALLBACK_NSM,
                                           0,
                                           CB::NSM_CALLBACK_ANNOUNCE,
                                           flags,
                                           0, 0.0f,
                                           smName);
            }

            std::free(msgURL);
        }
        else
        {
            carla_stdout("Got unknown NSM reply method '%s'", method);
        }

        return 0;
    }

    int handleOpen(const char* const projectPath, const char* const displayName, const char* const clientNameId)
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        carla_stdout("CarlaNSM::handleOpen(\"%s\", \"%s\", \"%s\")", projectPath, displayName, clientNameId);

        const CarlaHostHandle handle = (CarlaHostHandle)&gStandalone;

        carla_set_engine_option(handle, CB::ENGINE_OPTION_CLIENT_NAME_PREFIX, 0, clientNameId);

        if (gStandalone.engineCallback != nullptr)
        {
            fReadyActionOpen = false;
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_SET_CLIENT_NAME_ID,
                                       0, 0, 0.0f,
                                       clientNameId);
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_OPEN,
                                       0, 0, 0.0f,
                                       projectPath);

            for (; ! fReadyActionOpen;)
                d_msleep(10);
        }
        else
        {
            using namespace water;

            if (carla_is_engine_running(handle))
                carla_engine_close(handle);

            // TODO send error if engine failed to initialize
            carla_engine_init(handle, "JACK", clientNameId);

            fProjectPath  = projectPath;
            fProjectPath += ".carxp";

            if (File(fProjectPath).existsAsFile())
                carla_load_project(handle, fProjectPath);
        }

        fClientNameId = clientNameId;

        lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/open", "OK");

        // Broadcast ourselves
        if (fHasBroadcast)
        {
            const char* appName = std::getenv("CARLA_NSM_NAME");

            if (appName == nullptr)
                appName = "Carla";

            lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/server/broadcast", "sssss",
                         "/non/hello", fServerURL, appName, CARLA_VERSION_STRING, fClientNameId.buffer());
        }

        return 0;
    }

    int handleSave()
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        carla_stdout("CarlaNSM::handleSave()");

        if (gStandalone.engineCallback != nullptr)
        {
            fReadyActionSave = false;
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_SAVE,
                                       0, 0, 0.0f, nullptr);

            for (; ! fReadyActionSave;)
                d_msleep(10);
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(fProjectPath.isNotEmpty(), 0);

            const CarlaHostHandle handle = (CarlaHostHandle)&gStandalone;

            carla_save_project(handle, fProjectPath);
        }

        lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/save", "OK");

        return 0;
    }

    int handleSessionIsLoaded()
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        carla_stdout("CarlaNSM::handleSessionIsLoaded()");

        if (gStandalone.engineCallback != nullptr)
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_SESSION_IS_LOADED,
                                       0, 0, 0.0f, nullptr);

        return 0;
    }

    int handleShowOptionalGui()
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        carla_stdout("CarlaNSM::handleShowOptionalGui()");

        if (gStandalone.engineCallback != nullptr)
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_SHOW_OPTIONAL_GUI,
                                       0, 0, 0.0f, nullptr);

        return 0;
    }

    int handleHideOptionalGui()
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        carla_stdout("CarlaNSM::handleHideOptionalGui()");

        if (gStandalone.engineCallback != nullptr)
            gStandalone.engineCallback(gStandalone.engineCallbackPtr,
                                       CB::ENGINE_CALLBACK_NSM,
                                       0,
                                       CB::NSM_CALLBACK_HIDE_OPTIONAL_GUI,
                                       0, 0, 0.0f, nullptr);

        return 0;
    }

    int handleBroadcast(const char* const path, const char* const types, lo_arg** const argv, const int argc,
                        const lo_message msg)
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(argc >= 0, 0);
        carla_stdout("CarlaNSM::handleBroadcast(%s, %s, %p, %i)", path, types, argv, argc);

#if 0
        if (std::strcmp(path, "/non/hello") == 0)
        {
            CARLA_SAFE_ASSERT_RETURN(argc == 4, 0);
            CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "ssss") == 0, 0);

            const char* const url  = &argv[0]->s;
            //const char* const name     = &argv[1]->s;
            //const char* const version  = &argv[2]->s;
            //const char* const clientId = &argv[3]->s;

            const lo_address targetAddress(lo_address_new_from_url(url));
            CARLA_SAFE_ASSERT_RETURN(targetAddress != nullptr, 0);

            lo_send_from(targetAddress, fServer, LO_TT_IMMEDIATE, "/signal/hello", "ss",
                         fClientNameId.buffer(), fServerURL);

            lo_address_free(targetAddress);

            return 0;
        }

        if (std::strcmp(path, "/signal/hello") == 0)
        {
            //const char* const name = &argv[0]->s;
            const char* const url  = &argv[1]->s;

            const lo_address targetAddress(lo_address_new_from_url(url));
            CARLA_SAFE_ASSERT_RETURN(targetAddress != nullptr, 0);

            lo_send_from(targetAddress, fServer, LO_TT_IMMEDIATE, "/signal/hello", "ss",
                         fClientNameId.buffer(), fServerURL);

            lo_address_free(targetAddress);

            return 0;
        }

        if (std::strcmp(path, "/signal/list") == 0)
        {
            carla_stdout("CarlaNSM::handleBroadcast - got list");
            CARLA_SAFE_ASSERT_RETURN(carla_is_engine_running(), 0);

            const char* prefix = nullptr;

            if (argc > 0)
                prefix = &argv[0]->s;

            const lo_address msgAddress(lo_message_get_source(msg));
            CARLA_SAFE_ASSERT_RETURN(msgAddress != nullptr, 0);

            for (uint32_t i = 0, pluginCount = carla_get_current_plugin_count(); i < pluginCount; ++i)
            {
                const CarlaPluginInfo* const pluginInfo(carla_get_plugin_info(i));
                CARLA_SAFE_ASSERT_CONTINUE(pluginInfo != nullptr);

                /*const*/ String pluginNameId(fClientNameId + "/" + String(pluginInfo->name).replace('/','_') + "/");

                for (uint32_t j=0, paramCount = carla_get_parameter_count(i); j < paramCount; ++j)
                {
                    const CarlaParameterInfo* const paramInfo(carla_get_parameter_info(i, j));
                    CARLA_SAFE_ASSERT_CONTINUE(paramInfo != nullptr);

                    const ParameterData* const paramData(carla_get_parameter_data(i, j));
                    CARLA_SAFE_ASSERT_CONTINUE(paramData != nullptr);

                    const ParameterRanges* const paramRanges(carla_get_parameter_ranges(i, j));
                    CARLA_SAFE_ASSERT_CONTINUE(paramRanges != nullptr);

                    if (paramData->type != CB::PARAMETER_INPUT /*&& paramData->type != CB::PARAMETER_OUTPUT*/)
                        continue;
                    if ((paramData->hints & CB::PARAMETER_IS_ENABLED) == 0)
                        continue;
                    if ((paramData->hints & CB::PARAMETER_IS_AUTOMATABLE) == 0)
                        continue;
                    if (paramData->hints & CB::PARAMETER_IS_READ_ONLY)
                        continue;

                    const char* const dir = paramData->type == CB::PARAMETER_INPUT ? "in" : "out";
                    const String paramNameId = pluginNameId + String(paramInfo->name).replace('/','_');

                    const float defNorm = paramRanges->getNormalizedValue(paramRanges->def);

                    if (prefix == nullptr || std::strncmp(paramNameId, prefix, std::strlen(prefix)) == 0)
                    {
                        lo_send_from(msgAddress, fServer, LO_TT_IMMEDIATE, "/reply", "sssfff",
                                     path, paramNameId.buffer(), dir, 0.0f, 1.0f, defNorm);
                    }
                }
            }

            lo_send_from(msgAddress, fServer, LO_TT_IMMEDIATE, "/reply", "s", path);

            //return 0;
        }

        for (int i=0; i<argc; ++i)
            if (types[i] == 's')
                carla_stdout("%i: %s", i+1, &argv[i]->s);
#endif

        return 0;

        // unused
        (void)msg;
    }

private:
    CarlaHostStandalone& gStandalone;

    lo_address       fReplyAddress;
    lo_server        fServer;
    lo_server_thread fServerThread;
    char*            fServerURL;

    String fClientNameId;
    String fProjectPath;

    bool fHasBroadcast;
    bool fHasOptionalGui;
    bool fHasServerControl;
    bool fStarted;

    volatile bool fReadyActionOpen;
    volatile bool fReadyActionSave;

    #define handlePtr ((CarlaNSM*)data)

    static void _osc_error_handler(int num, const char* msg, const char* path)
    {
        carla_stderr2("CarlaNSM::_osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
    }

    static int _error_handler(const char*, const char* types, lo_arg** argv, int argc, lo_message, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(argc == 3, 1);
        CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "sis") == 0, 1);

        const char* const method  = &argv[0]->s;
        const int         code    =  argv[1]->i;
        const char* const message = &argv[2]->s;

        return handlePtr->handleError(method, code, message);
    }

    static int _reply_handler(const char*, const char* types, lo_arg** argv, int argc, lo_message msg, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(argc == 4, 1);
        CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "ssss") == 0, 1);

        const char* const method   = &argv[0]->s;
        const char* const message  = &argv[1]->s;
        const char* const smName   = &argv[2]->s;
        const char* const features = &argv[3]->s;

        return handlePtr->handleReply(method, message, smName, features, msg);
    }

    static int _open_handler(const char*, const char* types, lo_arg** argv, int argc, lo_message, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(argc == 3, 1);
        CARLA_SAFE_ASSERT_RETURN(std::strcmp(types, "sss") == 0, 1);

        const char* const projectPath  = &argv[0]->s;
        const char* const displayName  = &argv[1]->s;
        const char* const clientNameId = &argv[2]->s;

        return handlePtr->handleOpen(projectPath, displayName, clientNameId);
    }

    static int _save_handler(const char*, const char*, lo_arg**, int argc, lo_message, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(argc == 0, 1);

        return handlePtr->handleSave();
    }

    static int _loaded_handler(const char*, const char*, lo_arg**, int argc, lo_message, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(argc == 0, 1);

        return handlePtr->handleSessionIsLoaded();
    }

    static int _show_gui_handler(const char*, const char*, lo_arg**, int argc, lo_message, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(argc == 0, 1);

        return handlePtr->handleShowOptionalGui();
    }

    static int _hide_gui_handler(const char*, const char*, lo_arg**, int argc, lo_message, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(argc == 0, 1);

        return handlePtr->handleHideOptionalGui();
    }

    static int _broadcast_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* data)
    {
        return handlePtr->handleBroadcast(path, types, argv, argc, msg);
    }

    #undef handlePtr

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaNSM)
};

#endif // CARLA_ENABLE_STANDALONE_NSM

// -------------------------------------------------------------------------------------------------------------------

bool carla_nsm_init(CarlaHostHandle handle, uint64_t pid, const char* executableName)
{
    CARLA_SAFE_ASSERT_RETURN(handle->isStandalone, false);

   #ifdef CARLA_ENABLE_STANDALONE_NSM
    return CarlaNSM::getInstance(*(CarlaHostStandalone*)handle).announce(pid, executableName);
   #else
    return false;

    // unused
    (void)pid; (void)executableName;
   #endif
}

void carla_nsm_ready(CarlaHostHandle handle, NsmCallbackOpcode action)
{
    CARLA_SAFE_ASSERT_RETURN(handle->isStandalone,);

   #ifdef CARLA_ENABLE_STANDALONE_NSM
    CarlaNSM::getInstance(*(CarlaHostStandalone*)handle).ready(action);
   #else
    // unused
    return; (void)action;
   #endif
}

// -------------------------------------------------------------------------------------------------------------------
