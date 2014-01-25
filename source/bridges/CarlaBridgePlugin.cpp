/*
 * Carla Bridge Plugin
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaPlugin.hpp"
#include "CarlaHost.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaMIDI.h"

//#include "juce_core.h"

#ifdef CARLA_OS_UNIX
# include <signal.h>
#endif

//using juce::File;

// -------------------------------------------------------------------------

static volatile bool gCloseNow = false;
static volatile bool gSaveNow  = false;

#ifdef CARLA_OS_WIN
BOOL WINAPI winSignalHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        gCloseNow = true;
        return TRUE;
    }

    return FALSE;
}
#elif defined(CARLA_OS_LINUX)
static void closeSignalHandler(int)
{
    gCloseNow = true;
}
static void saveSignalHandler(int)
{
    gSaveNow = true;
}
#endif

void initSignalHandler()
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

CARLA_BRIDGE_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------

class CarlaPluginClient : public CarlaBridgeClient
{
public:
    CarlaPluginClient(const bool useBridge, const char* const driverName, const char* audioBaseName, const char* controlBaseName)
        : CarlaBridgeClient(nullptr),
          fPlugin(nullptr),
          fEngine(nullptr)
    {
        CARLA_ASSERT(driverName != nullptr && driverName[0] != '\0');
        carla_debug("CarlaPluginClient::CarlaPluginClient(%s, \"%s\", %s, %s)", bool2str(useBridge), driverName, audioBaseName, controlBaseName);

        carla_set_engine_callback(callback, this);

#if 0
        File curDir(File::getSpecialLocation(File::currentApplicationFile).getParentDirectory());

        if (curDir.getChildFile("resources").exists())
            carla_set_engine_option(CarlaBackend::OPTION_PATH_RESOURCES, 0, curDir.getChildFile("resources").getFullPathName().toRawUTF8());
        else if (curDir.getChildFile("../../modules/daz-plugins/resources").exists())
            carla_set_engine_option(CarlaBackend::OPTION_PATH_RESOURCES, 0, curDir.getChildFile("../../modules/daz-plugins/resources").getFullPathName().toRawUTF8());
        else
            carla_set_engine_option(CarlaBackend::OPTION_PATH_RESOURCES, 0, curDir.getChildFile("../modules/daz-plugins/resources").getFullPathName().toRawUTF8());
#endif

        if (useBridge)
            carla_engine_init_bridge(audioBaseName, controlBaseName, driverName);
        else
            carla_engine_init("JACK", driverName);

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

    void ready(const bool doSaveLoad)
    {
        fPlugin = fEngine->getPlugin(0);

        if (doSaveLoad)
        {
            fProjFileName  = fPlugin->getName();
            fProjFileName += ".carxs";

            //if (! File::isAbsolutePath((const char*)fProjFileName))
            //    fProjFileName = File::getCurrentWorkingDirectory().getChildFile((const char*)fProjFileName).getFullPathName().toRawUTF8();

            if (! fPlugin->loadStateFromFile(fProjFileName))
                carla_stderr("Plugin preset load failed, error was:\n%s", fEngine->getLastError());
        }
    }

    void idle()
    {
        CARLA_SAFE_ASSERT_RETURN(fEngine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);

        carla_engine_idle();
        CarlaBridgeClient::oscIdle();

        if (gSaveNow)
        {
            gSaveNow = false;

            if (fProjFileName.isNotEmpty())
            {
                if (! fPlugin->saveStateToFile(fProjFileName))
                    carla_stderr("Plugin preset save failed, error was:\n%s", fEngine->getLastError());
            }
        }

        if (gCloseNow)
        {
            //gCloseNow = false;
            // close something?
        }
    }

    void exec()
    {
        while (! gCloseNow)
        {
            idle();
            carla_msleep(30);
        }
    }

    // ---------------------------------------------------------------------
    // plugin management

    void saveNow()
    {
        CARLA_SAFE_ASSERT_RETURN(fEngine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        carla_debug("CarlaPluginClient::saveNow()");

        fPlugin->prepareForSave();

        for (uint32_t i=0; i < fPlugin->getCustomDataCount(); ++i)
        {
            const CarlaBackend::CustomData& cdata(fPlugin->getCustomData(i));
            fEngine->oscSend_bridge_set_custom_data(cdata.type, cdata.key, cdata.value);
        }

        if (fPlugin->getOptionsEnabled() & CarlaBackend::PLUGIN_OPTION_USE_CHUNKS)
        {
            void* data = nullptr;
            int32_t dataSize = fPlugin->getChunkData(&data);

            if (data && dataSize >= 4)
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

    void setParameterMidiChannel(const int32_t index, const uint8_t channel)
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        carla_debug("CarlaPluginClient::setParameterMidiChannel(%i, %i)", index, channel);

        fPlugin->setParameterMidiChannel(index, channel, false, false);
    }

    void setParameterMidiCC(const int32_t index, const int16_t cc)
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        carla_debug("CarlaPluginClient::setParameterMidiCC(%i, %i)", index, cc);

        fPlugin->setParameterMidiCC(index, cc, false, false);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value)
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        carla_debug("CarlaPluginClient::setCustomData(\"%s\", \"%s\", \"%s\")", type, key, value);

        fPlugin->setCustomData(type, key, value, true);
    }

    void setChunkData(const char* const filePath)
    {
        CARLA_SAFE_ASSERT_RETURN(filePath != nullptr && filePath[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        carla_debug("CarlaPluginClient::setChunkData(\"%s\")", filePath);

#if 0
        QString chunkFilePath(filePath);

#ifdef CARLA_OS_WIN
        if (chunkFilePath.startsWith("/"))
        {
            // running under Wine, posix host
            chunkFilePath = chunkFilePath.replace(0, 1, "Z:/");
            chunkFilePath = QDir::toNativeSeparators(chunkFilePath);
        }
#endif
        QFile chunkFile(chunkFilePath);

        if (fPlugin != nullptr && chunkFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&chunkFile);
            QString stringData(in.readAll());
            chunkFile.close();
            chunkFile.remove();

            fPlugin->setChunkData(stringData.toUtf8().constData());
        }
#endif
    }

    // ---------------------------------------------------------------------

protected:
    void handleCallback(const CarlaBackend::EngineCallbackOpcode action, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        CARLA_BACKEND_USE_NAMESPACE;

        // TODO

        switch (action)
        {
        case ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
            if (isOscControlRegistered())
                sendOscControl(value1, value3);
            break;

        case ENGINE_CALLBACK_UI_STATE_CHANGED:
            if (! isOscControlRegistered())
            {
                if (value1 != 1)
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
    CarlaBackend::CarlaPlugin* fPlugin;
    const CarlaBackend::CarlaEngine* fEngine;

    CarlaString fProjFileName;

    static void callback(void* ptr, CarlaBackend::EngineCallbackOpcode action, unsigned int pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        carla_debug("CarlaPluginClient::callback(%p, %i:%s, %i, %i, %i, %f, \"%s\")", ptr, action, CarlaBackend::EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
        CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(pluginId == 0,);

        return ((CarlaPluginClient*)ptr)->handleCallback(action, value1, value2, value3, valueStr);
    }
};

// -------------------------------------------------------------------------

int CarlaBridgeOsc::handleMsgShow()
{
    carla_debug("CarlaBridgeOsc::handleMsgShow()");

    carla_show_custom_ui(0, true);
    return 0;
}

int CarlaBridgeOsc::handleMsgHide()
{
    carla_debug("CarlaBridgeOsc::handleMsgHide()");

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

    CarlaPluginClient* const plugClient((CarlaPluginClient*)fClient);
    plugClient->saveNow();

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

    CarlaPluginClient* const plugClient((CarlaPluginClient*)fClient);
    plugClient->setParameterMidiChannel(static_cast<uint32_t>(index), static_cast<uint8_t>(channel));

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

    CarlaPluginClient* const plugClient((CarlaPluginClient*)fClient);
    plugClient->setParameterMidiCC(static_cast<uint32_t>(index), static_cast<int16_t>(cc));

    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetChunk(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "s");
    CARLA_SAFE_ASSERT_RETURN(fClient != nullptr, 1);
    carla_debug("CarlaBridgeOsc::handleMsgPluginSetChunk()");

    const char* const chunkFile = (const char*)&argv[0]->s;

    CarlaPluginClient* const plugClient((CarlaPluginClient*)fClient);
    plugClient->setChunkData(chunkFile);

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

    CarlaPluginClient* const plugClient((CarlaPluginClient*)fClient);
    plugClient->setCustomData(type, key, value);

    return 0;
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

// -------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    CARLA_BRIDGE_USE_NAMESPACE;

    // ---------------------------------------------------------------------
    // Check argument count

    if (argc != 6 && argc != 7)
    {
        carla_stdout("usage: %s <osc-url|\"null\"> <type> <filename> <name|\"(none)\"> <label>", argv[0]);
        return 1;
    }

    // ---------------------------------------------------------------------
    // Get args

    const char* const oscUrl   = argv[1];
    const char* const stype    = argv[2];
    const char* const filename = argv[3];
    const char*       name     = argv[4];
    const char*       label    = argv[5];

    // ---------------------------------------------------------------------
    // Setup args

    const bool useBridge = (argc == 7);
    const bool useOsc    = (std::strcmp(oscUrl, "null") != 0 && std::strcmp(oscUrl, "(null)") != 0 && std::strcmp(oscUrl, "NULL") != 0);

    if (std::strcmp(name, "(none)") == 0)
        name = nullptr;

    if (std::strlen(label) == 0)
        label = nullptr;

    char bridgeBaseAudioName[6+1];
    char bridgeBaseControlName[6+1];

    if (useBridge)
    {
        CARLA_SAFE_ASSERT_RETURN(std::strlen(argv[6]) == 6*2, 1);
        std::strncpy(bridgeBaseAudioName,   argv[6],   6);
        std::strncpy(bridgeBaseControlName, argv[6]+6, 6);
        bridgeBaseAudioName[6]   = '\0';
        bridgeBaseControlName[6] = '\0';
    }
    else
    {
        bridgeBaseAudioName[0]   = '\0';
        bridgeBaseControlName[0] = '\0';
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
    // Set client name

    CarlaString clientName((name != nullptr) ? name : label);

    //if (clientName.isEmpty())
    //    clientName = File(filename).getFileNameWithoutExtension().toRawUTF8();

    // ---------------------------------------------------------------------
    // Set extraStuff

    const void* extraStuff = nullptr;

    if (itype == CarlaBackend::PLUGIN_FILE_GIG || itype == CarlaBackend::PLUGIN_FILE_SF2)
    {
        if (label == nullptr)
            label = clientName;

        if (std::strstr(label, " (16 outs)") == 0)
            extraStuff = "true";
    }

    // ---------------------------------------------------------------------
    // Init plugin client

    CarlaPluginClient client(useBridge, (const char*)clientName, bridgeBaseAudioName, bridgeBaseControlName);

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

    if (carla_add_plugin(CarlaBackend::BINARY_NATIVE, itype, filename, name, label, extraStuff))
    {
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

        client.ready(!useOsc);
        client.exec();

        carla_set_engine_about_to_close();
        carla_remove_plugin(0);

        ret = 0;
    }
    else
    {
        const char* const lastError(carla_get_last_error());
        carla_stderr("Plugin failed to load, error was:\n%s", lastError);

        if (useOsc)
            client.sendOscBridgeError(lastError);

        ret = 1;
    }

    // ---------------------------------------------------------------------
    // Close OSC

    if (useOsc)
        client.oscClose();

    return ret;
}
