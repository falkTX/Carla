/*
 * Carla Standalone
 * Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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

#ifdef HAVE_LIBLO

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 2

#define NSM_CLIENT_FEATURES ":switch:"
//#define NSM_CLIENT_FEATURES ":switch:optional-gui:"

#include "CarlaHost.h"
#include "CarlaOscUtils.hpp"
#include "CarlaString.hpp"
#include "juce_core.h"

namespace CB = CarlaBackend;

// -------------------------------------------------------------------------------------------------------------------

struct CarlaBackendStandalone {
    CarlaEngine*       engine;
    EngineCallbackFunc engineCallback;
    void*              engineCallbackPtr;
    // ...
};

extern CarlaBackendStandalone gStandalone;

// -------------------------------------------------------------------------------------------------------------------

class CarlaNSM
{
public:
    CarlaNSM() noexcept
        : fReplyAddress(nullptr),
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

    bool announce(const int pid, const char* const executableName)
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

        lo_send_from(nsmAddress, fServer, LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                     "Carla", NSM_CLIENT_FEATURES, executableName, NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid);

        lo_address_free(nsmAddress);

        return true;
    }

    void ready(const int action)
    {
        CARLA_SAFE_ASSERT_RETURN(fServerThread != nullptr,);

        switch (action)
        {
        case -1: // init
            CARLA_SAFE_ASSERT_BREAK(! fStarted);
            fStarted = true;
            lo_server_thread_start(fServerThread);
            break;

        case 0: // error
            break;

        case 1: // reply
            break;

        case 2: // open
            fReadyActionOpen = true;
            break;

        case 3: // save
            fReadyActionSave = true;
            break;

        case 4: // session loaded
            break;

        case 5: // show gui
            CARLA_SAFE_ASSERT_BREAK(fReplyAddress != nullptr);
            CARLA_SAFE_ASSERT_BREAK(fServer != nullptr);
            lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/client/gui_is_shown", "");
            break;

        case 6: // hide gui
            CARLA_SAFE_ASSERT_BREAK(fReplyAddress != nullptr);
            CARLA_SAFE_ASSERT_BREAK(fServer != nullptr);
            lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/client/gui_is_hidden", "");
            break;
        }
    }

    static CarlaNSM& getInstance()
    {
        static CarlaNSM sInstance;
        return sInstance;
    }

protected:
    int handleError(const char* const method, const int code, const char* const message)
    {
        carla_stdout("CarlaNSM::handleError(\"%s\", %i, \"%s\")", method, code, message);

        if (gStandalone.engineCallback != nullptr)
            gStandalone.engineCallback(gStandalone.engineCallbackPtr, CB::ENGINE_CALLBACK_NSM, 0, 0, code, 0.0f, message);

        return 1;

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
            fHasServerControl = std::strstr(features, ":server_control:") != nullptr;

#if 0
            // UI starts visible
            if (fHasOptionalGui)
                lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/client/gui_is_shown", "");
#endif

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

                gStandalone.engineCallback(gStandalone.engineCallbackPtr, CB::ENGINE_CALLBACK_NSM, 0, 1, flags, 0.0f, smName);
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

        if (gStandalone.engineCallback != nullptr)
        {
            fReadyActionOpen = false;
            gStandalone.engineCallback(gStandalone.engineCallbackPtr, CB::ENGINE_CALLBACK_NSM, 0, 2, 0, 0.0f, projectPath);

            for (; ! fReadyActionOpen;)
                carla_msleep(10);
        }
        else
        {
            using namespace juce;

            if (carla_is_engine_running())
                carla_engine_close();

            carla_engine_init("JACK", clientNameId);

            fProjectPath  = projectPath;
            fProjectPath += ".carxp";

            const String jfilename = String(CharPointer_UTF8(fProjectPath));

            if (File(jfilename).existsAsFile())
                carla_load_project(fProjectPath);
        }

        fClientNameId = clientNameId;

        lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/open", "OK");

        // Broadcast ourselves
        if (fHasBroadcast)
        {
            lo_send_from(fReplyAddress, fServer, LO_TT_IMMEDIATE, "/nsm/server/broadcast", "sssss",
                         "/non/hello", fServerURL, "Carla", CARLA_VERSION_STRING, fClientNameId.buffer());
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
            gStandalone.engineCallback(gStandalone.engineCallbackPtr, CB::ENGINE_CALLBACK_NSM, 0, 3, 0, 0.0f, nullptr);

            for (; ! fReadyActionSave;)
                carla_msleep(10);
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(fProjectPath.isNotEmpty(), 0);

            carla_save_project(fProjectPath);
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
            gStandalone.engineCallback(gStandalone.engineCallbackPtr, CB::ENGINE_CALLBACK_NSM, 0, 4, 0, 0.0f, nullptr);

        return 0;
    }

    int handleShowOptionalGui()
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        carla_stdout("CarlaNSM::handleShowOptionalGui()");

        if (gStandalone.engineCallback != nullptr)
            gStandalone.engineCallback(gStandalone.engineCallbackPtr, CB::ENGINE_CALLBACK_NSM, 0, 5, 0, 0.0f, nullptr);

        return 0;
    }

    int handleHideOptionalGui()
    {
        CARLA_SAFE_ASSERT_RETURN(fReplyAddress != nullptr, 1);
        CARLA_SAFE_ASSERT_RETURN(fServer != nullptr, 1);
        carla_stdout("CarlaNSM::handleHideOptionalGui()");

        if (gStandalone.engineCallback != nullptr)
            gStandalone.engineCallback(gStandalone.engineCallbackPtr, CB::ENGINE_CALLBACK_NSM, 0, 6, 0, 0.0f, nullptr);

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

                /*const*/ CarlaString pluginNameId(fClientNameId + "/" + CarlaString(pluginInfo->name).replace('/','_') + "/");

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
                    if ((paramData->hints & CB::PARAMETER_IS_AUTOMABLE) == 0)
                        continue;
                    if (paramData->hints & CB::PARAMETER_IS_READ_ONLY)
                        continue;

                    const char* const dir         = paramData->type == CB::PARAMETER_INPUT ? "in" : "out";
                    const CarlaString paramNameId = pluginNameId + CarlaString(paramInfo->name).replace('/','_');

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
    lo_address       fReplyAddress;
    lo_server        fServer;
    lo_server_thread fServerThread;
    char*            fServerURL;

    CarlaString fClientNameId;
    CarlaString fProjectPath;

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
    CARLA_DECLARE_NON_COPY_CLASS(CarlaNSM)
};

#endif // HAVE_LIBLO

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
bool carla_nsm_init(int pid, const char* executableName);

bool carla_nsm_init(int pid, const char* executableName)
{
#ifdef HAVE_LIBLO
    return CarlaNSM::getInstance().announce(pid, executableName);
#else
    return false;

    // unused
    (void)pid; (void)executableName;
#endif
}

CARLA_EXPORT
void carla_nsm_ready(int action);

void carla_nsm_ready(int action)
{
#ifdef HAVE_LIBLO
    CarlaNSM::getInstance().ready(action);
#endif
}

// -------------------------------------------------------------------------------------------------------------------
