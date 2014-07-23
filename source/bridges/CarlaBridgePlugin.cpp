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

#include "CarlaBridgeClient.hpp"

#include "CarlaEngine.hpp"
#include "CarlaHost.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
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

#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
static CarlaBridge::CarlaBridgeClient* gBridgeClient = nullptr;

class CarlaJuceApp : public JUCEApplication,
                     private Timer
{
public:
    CarlaJuceApp()  {}
    ~CarlaJuceApp() {}

    void initialise(const String&) override
    {
        startTimer(30);
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
        carla_engine_idle();

        if (gBridgeClient != nullptr)
            gBridgeClient->oscIdle();

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

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

class CarlaPluginClient : public CarlaBridgeClient
{
public:
    CarlaPluginClient(const bool useBridge, const char* const clientName, const char* const audioBaseName, const char* const controlBaseName, const char* const timeBaseName)
        : CarlaBridgeClient(nullptr),
          fEngine(nullptr)
    {
        CARLA_ASSERT(clientName != nullptr && clientName[0] != '\0');
        carla_debug("CarlaPluginClient::CarlaPluginClient(%s, \"%s\", %s, %s, %s)", bool2str(useBridge), clientName, audioBaseName, controlBaseName, timeBaseName);

        carla_set_engine_callback(callback, this);

        if (useBridge)
            carla_engine_init_bridge(audioBaseName, controlBaseName, timeBaseName, clientName);
        else
            carla_engine_init("JACK", clientName);

        fEngine = carla_get_engine();
    }

    ~CarlaPluginClient() override
    {
        carla_debug("CarlaPluginClient::~CarlaPluginClient()");

        carla_engine_close();
    }

    bool isOk() const noexcept
    {
        return (fEngine != nullptr);
    }

    void oscInit(const char* const url)
    {
        CarlaBridgeClient::oscInit(url);
        fEngine->setOscBridgeData(&fOscData);
    }

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
        gBridgeClient = this;
        JUCEApplicationBase::createInstance = &juce_CreateApplication;
        JUCEApplicationBase::main(JUCE_MAIN_FUNCTION_ARGS);
        gBridgeClient = nullptr;
#else
        for (; ! gCloseNow;)
        {
            idle();
            carla_msleep(25);
        }
#endif

        carla_set_engine_about_to_close();
        carla_remove_plugin(0);

        // may be unused
        return; (void)argc; (void)argv;
    }

    void idle()
    {
        CARLA_SAFE_ASSERT_RETURN(fEngine != nullptr,);

        carla_engine_idle();
        CarlaBridgeClient::oscIdle();

        if (gSaveNow)
        {
            gSaveNow = false;

            if (fProjFilename.isNotEmpty())
            {
                if (! carla_save_plugin_state(0, fProjFilename.toRawUTF8()))
                    carla_stderr("Plugin preset save failed, error was:\n%s", carla_get_last_error());
            }
        }
    }

    // ---------------------------------------------------------------------
    // plugin management

    void saveNow()
    {
        CARLA_SAFE_ASSERT_RETURN(fEngine != nullptr,);
        carla_debug("CarlaPluginClient::saveNow()");

        carla_prepare_for_save(0);

        for (uint32_t i=0, count=carla_get_custom_data_count(0); i<count; ++i)
        {
            const CarlaBackend::CustomData* const cdata(carla_get_custom_data(0, i));
            CARLA_SAFE_ASSERT_CONTINUE(cdata != nullptr);

            fEngine->oscSend_bridge_set_custom_data(cdata->type, cdata->key, cdata->value);
        }

        //if (fPlugin->getOptionsEnabled() & CarlaBackend::PLUGIN_OPTION_USE_CHUNKS)
        {
            //if (const char* const chunkData = carla_get_chunk_data(0))
            {
#if 0
                QString filePath;
                filePath = QDir::tempPath();
#ifdef Q_OS_WIN
                filePath += "\\.CarlaChunk_";
#else
                filePath += "/.CarlaChunk_";
#endif
                filePath += fPlugin->getName();

                QFile file(filePath);

                if (file.open(QIODevice::WriteOnly))
                {
                    QByteArray chunk((const char*)data, dataSize);
                    file.write(chunk);
                    file.close();
                    fEngine->oscSend_bridge_set_chunk_data(filePath.toUtf8().constData());
                }
#endif
            }
        }

        fEngine->oscSend_bridge_configure(CARLA_BRIDGE_MSG_SAVED, "");
    }

    // ---------------------------------------------------------------------

protected:
    void handleCallback(const EngineCallbackOpcode action, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        CARLA_BACKEND_USE_NAMESPACE;

        // TODO

        switch (action)
        {
        case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
            if (isOscControlRegistered())
            {
                CARLA_SAFE_ASSERT_RETURN(value1 >= 0,);
                fEngine->oscSend_bridge_parameter_value(static_cast<uint32_t>(value1), value3);
            }
            break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            if (! isOscControlRegistered())
            {
                if (value1 != 1 && gIsInitiated)
                    gCloseNow = true;
            }
            else
            {
                // show-gui button
                fEngine->oscSend_bridge_configure(CARLA_BRIDGE_MSG_HIDE_GUI, "");
            }
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

    static void callback(void* ptr, EngineCallbackOpcode action, unsigned int pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        carla_debug("CarlaPluginClient::callback(%p, %i:%s, %i, %i, %i, %f, \"%s\")", ptr, action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
        CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(pluginId == 0,);

        return ((CarlaPluginClient*)ptr)->handleCallback(action, value1, value2, value3, valueStr);
    }
};

// -------------------------------------------------------------------------

int CarlaBridgeOsc::handleMsgShow()
{
    carla_debug("CarlaBridgeOsc::handleMsgShow()");

    if (carla_get_plugin_info(0)->hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI)
        carla_show_custom_ui(0, true);

    return 0;
}

int CarlaBridgeOsc::handleMsgHide()
{
    carla_debug("CarlaBridgeOsc::handleMsgHide()");

    if (carla_get_plugin_info(0)->hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI)
        carla_show_custom_ui(0, false);

    return 0;
}

int CarlaBridgeOsc::handleMsgQuit()
{
    carla_debug("CarlaBridgeOsc::handleMsgQuit()");

    gCloseNow = true;
    return 0;
}

// -------------------------------------------------------------------------

int CarlaBridgeOsc::handleMsgPluginSaveNow()
{
    CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, 1);
    carla_debug("CarlaBridgeOsc::handleMsgPluginSaveNow()");

    ((CarlaPluginClient*)fClient)->saveNow();
    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetParameterMidiChannel(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ii");
    CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, 1);
    carla_debug("CarlaBridgeOsc::handleMsgPluginSetParameterMidiChannel()");

    const int32_t index   = argv[0]->i;
    const int32_t channel = argv[1]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);
    CARLA_SAFE_ASSERT_RETURN(channel >= 0 && channel < MAX_MIDI_CHANNELS, 0);

    carla_set_parameter_midi_channel(0, static_cast<uint32_t>(index), static_cast<uint8_t>(channel));
    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetParameterMidiCC(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ii");
    CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, 1);
    carla_debug("CarlaBridgeOsc::handleMsgPluginSetParameterMidiCC()");

    const int32_t index  = argv[0]->i;
    const int32_t cc     = argv[1]->i;

    CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);
    CARLA_SAFE_ASSERT_RETURN(cc >= 1 && cc < 0x5F, 0);

    carla_set_parameter_midi_cc(0, static_cast<uint32_t>(index), static_cast<int16_t>(cc));
    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetCustomData(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(3, "sss");
    CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, 1);
    carla_debug("CarlaBridgeOsc::handleMsgPluginSetCustomData()");

    const char* const type  = (const char*)&argv[0]->s;
    const char* const key   = (const char*)&argv[1]->s;
    const char* const value = (const char*)&argv[2]->s;

    carla_set_custom_data(0, type, key, value);
    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetChunk(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "s");
    CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, 1);
    carla_debug("CarlaBridgeOsc::handleMsgPluginSetChunk()");

    const char* const chunkFilePathTry = (const char*)&argv[0]->s;

    CARLA_SAFE_ASSERT_RETURN(chunkFilePathTry != nullptr && chunkFilePathTry[0] != '\0', 0);

    String chunkFilePath(chunkFilePathTry);

#ifdef CARLA_OS_WIN
    if (chunkFilePath.startsWith("/"))
    {
        // running under Wine, posix host
        chunkFilePath = chunkFilePath.replaceSection(0, 1, "Z:\\");
        chunkFilePath = chunkFilePath.replace("/", "\\");
    }
#endif

    File chunkFile(chunkFilePath);
    CARLA_SAFE_ASSERT_RETURN(chunkFile.existsAsFile(), 0);

    String chunkData(chunkFile.loadFileAsString());
    chunkFile.deleteFile();
    CARLA_SAFE_ASSERT_RETURN(chunkData.isNotEmpty(), 0);

    carla_set_chunk_data(0, chunkData.toRawUTF8());
    return 0;
}

CARLA_BRIDGE_END_NAMESPACE

// -------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    CARLA_BRIDGE_USE_NAMESPACE;

    // ---------------------------------------------------------------------
    // Check argument count

    if (argc != 7)
    {
        carla_stdout("usage: %s <osc-url|\"null\"> <type> <filename> <name|\"(none)\"> <label> <uniqueId>", argv[0]);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Get args

    const char* const oscUrl   = argv[1];
    const char* const stype    = argv[2];
    const char* const filename = argv[3];
    const char*       name     = argv[4];
    const char*       label    = argv[5];
    const int64_t     uniqueId = static_cast<int64_t>(std::atoll(argv[6]));

    // ---------------------------------------------------------------------
    // Check plugin type

    CarlaBackend::PluginType itype(CarlaBackend::getPluginTypeFromString(stype));

    if (itype == CarlaBackend::PLUGIN_NONE)
    {
        carla_stderr("Invalid plugin type '%s'", stype);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Set name as null if invalid

    if (std::strlen(name) == 0 || std::strcmp(name, "(none)") == 0)
        name = nullptr;

    // ---------------------------------------------------------------------
    // Setup options

    const char* const shmIds(std::getenv("ENGINE_BRIDGE_SHM_IDS"));

    const bool useBridge = (shmIds != nullptr);
    const bool useOsc    = (std::strcmp(oscUrl, "null") != 0 && std::strcmp(oscUrl, "(null)") != 0 && std::strcmp(oscUrl, "NULL") != 0);

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

    CarlaString clientName((name != nullptr) ? name : label);

    if (clientName.isEmpty())
        clientName = juce::File(filename).getFileNameWithoutExtension().toRawUTF8();

    // ---------------------------------------------------------------------
    // Set extraStuff

    const void* extraStuff = nullptr;

    if (itype == CarlaBackend::PLUGIN_GIG || itype == CarlaBackend::PLUGIN_SF2)
    {
        if (label == nullptr)
            label = clientName;

        if (std::strstr(label, " (16 outs)") == 0)
            extraStuff = "true";
    }

    // ---------------------------------------------------------------------
    // Init plugin client

    CarlaPluginClient client(useBridge, clientName, bridgeBaseAudioName, bridgeBaseControlName, bridgeBaseTimeName);

    if (! client.isOk())
    {
        carla_stderr("Failed to init engine, error was:\n%s", carla_get_last_error());
        return 1;
    }

    // ---------------------------------------------------------------------
    // Init OSC

    if (useOsc)
        client.oscInit(oscUrl);

    // ---------------------------------------------------------------------
    // Listen for ctrl+c or sigint/sigterm events

    initSignalHandler();

    // ---------------------------------------------------------------------
    // Init plugin

    int ret;

    if (carla_add_plugin(CarlaBackend::BINARY_NATIVE, itype, filename, name, label, uniqueId, extraStuff))
    {
        ret = 0;

        if (useOsc)
        {
            client.sendOscUpdate();
            client.sendOscBridgeUpdate();
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

        client.exec(useOsc, argc, argv);
    }
    else
    {
        ret = 1;

        const char* const lastError(carla_get_last_error());
        carla_stderr("Plugin failed to load, error was:\n%s", lastError);

        if (useOsc)
            client.sendOscBridgeError(lastError);
    }

    // ---------------------------------------------------------------------
    // Close OSC

    if (useOsc)
        client.oscClose();

    return ret;
}
