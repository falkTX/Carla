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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaBridgeClient.hpp"
#include "CarlaBridgeToolkit.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaStandalone.hpp"
#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QApplication>
#else
# include <QtGui/QApplication>
#endif

#ifdef CARLA_OS_UNIX
# include <signal.h>
#endif

// -------------------------------------------------------------------------

static bool gCloseNow = false;
static bool gSaveNow  = false;

#ifdef CARLA_OS_WIN
BOOL WINAPI closeSignalHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        gCloseNow = true;
        return TRUE;
    }

    return FALSE;
}
#else
void closeSignalHandler(int)
{
    gCloseNow = true;
}
void saveSignalHandler(int)
{
    gSaveNow = true;
}
#endif

void initSignalHandler()
{
#ifdef CARLA_OS_WIN
    SetConsoleCtrlHandler(closeSignalHandler, TRUE);
#elif defined(CARLA_OS_LINUX) || defined(CARLA_OS_HAIKU)
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
// Helpers

extern CarlaBackend::CarlaEngine* carla_get_standalone_engine();

CARLA_BACKEND_START_NAMESPACE
extern const char* findDSSIGUI(const char* const filename, const char* const label);
CARLA_BACKEND_END_NAMESPACE

CARLA_BRIDGE_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------

class CarlaPluginClient : public CarlaBridgeClient,
                          public QObject
{
public:
    CarlaPluginClient(const bool useBridge, const char* const driverName, const char* audioBaseName, const char* controlBaseName)
        : CarlaBridgeClient(nullptr),
          QObject(nullptr),
          fEngine(nullptr),
          fPlugin(nullptr),
          fTimerId(0)
    {
        CARLA_ASSERT(driverName != nullptr);
        carla_debug("CarlaPluginClient::CarlaPluginClient(%s, \"%s\", %s, %s)", bool2str(useBridge), driverName, audioBaseName, controlBaseName);

        if (useBridge)
            carla_engine_init_bridge(audioBaseName, controlBaseName, driverName);
        else
            carla_engine_init("JACK", driverName);

        carla_set_engine_callback(callback, this);
    }

    ~CarlaPluginClient()
    {
        CARLA_ASSERT(fTimerId == 0);
        carla_debug("CarlaPluginClient::~CarlaPluginClient()");

        carla_set_engine_about_to_close();
        carla_engine_close();
    }

    void oscInit(const char* const url)
    {
        CarlaBridgeClient::oscInit(url);

        fEngine = carla_get_standalone_engine();
        fEngine->setOscBridgeData(fOscData);
    }

    void ready()
    {
        CARLA_ASSERT(fTimerId == 0);

        fEngine = carla_get_standalone_engine();
        fPlugin = fEngine->getPlugin(0);

        fTimerId = startTimer(50);
    }

    void idle()
    {
        if (fEngine != nullptr)
            fEngine->idle();

        CarlaBridgeClient::oscIdle();

        if (gSaveNow)
        {
            // TODO
            gSaveNow = false;
        }

        if (gCloseNow)
        {
            if (fTimerId != 0)
            {
                killTimer(fTimerId);
                fTimerId = 0;
            }

            if (QApplication* const app = qApp)
            {
                if (! app->closingDown())
                    app->quit();
            }
        }
    }

    void exec()
    {
        while (! gCloseNow)
        {
            idle();
            carla_msleep(50);
        }
    }

    // ---------------------------------------------------------------------
    // plugin management

    void saveNow()
    {
        qDebug("CarlaPluginClient::saveNow()");
        CARLA_ASSERT(fEngine != nullptr);
        CARLA_ASSERT(fPlugin != nullptr);

        if (fPlugin == nullptr || fEngine == nullptr)
            return;

        fPlugin->prepareForSave();

        for (uint32_t i=0; i < fPlugin->customDataCount(); i++)
        {
            const CarlaBackend::CustomData& cdata(fPlugin->customData(i));
            fEngine->osc_send_bridge_set_custom_data(cdata.type, cdata.key, cdata.value);
        }

        if (fPlugin->options() & CarlaBackend::PLUGIN_OPTION_USE_CHUNKS)
        {
            void* data = nullptr;
            int32_t dataSize = fPlugin->chunkData(&data);

            if (data && dataSize >= 4)
            {
                QString filePath;
                filePath = QDir::tempPath();
#ifdef Q_OS_WIN
                filePath += "\\.CarlaChunk_";
#else
                filePath += "/.CarlaChunk_";
#endif
                filePath += fPlugin->name();

                QFile file(filePath);

                if (file.open(QIODevice::WriteOnly))
                {
                    QByteArray chunk((const char*)data, dataSize);
                    file.write(chunk);
                    file.close();
                    fEngine->osc_send_bridge_set_chunk_data(filePath.toUtf8().constData());
                }
            }
        }

        fEngine->osc_send_bridge_configure(CARLA_BRIDGE_MSG_SAVED, "");
    }

    void setCustomData(const char* const type, const char* const key, const char* const value)
    {
        carla_debug("CarlaPluginClient::setCustomData(\"%s\", \"%s\", \"%s\")", type, key, value);
        CARLA_ASSERT(fPlugin != nullptr);

        if (fPlugin != nullptr)
            fPlugin->setCustomData(type, key, value, true);
    }

    void setChunkData(const char* const filePath)
    {
        carla_debug("CarlaPluginClient::setChunkData(\"%s\")", filePath);
        CARLA_ASSERT(fPlugin != nullptr);

        if (fPlugin == nullptr)
            return;

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
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(const int32_t rindex, const float value)
    {
        carla_debug("CarlaPluginClient::setParameter(%i, %f)", rindex, value);
        CARLA_ASSERT(fPlugin != nullptr);

        if (fPlugin != nullptr)
            fPlugin->setParameterValueByRealIndex(rindex, value, true, true, false);
    }

protected:
    void handleCallback(const CarlaBackend::CallbackType action, const int value1, const int value2, const float value3, const char* const valueStr)
    {
        CARLA_BACKEND_USE_NAMESPACE;

        // TODO

        switch (action)
        {
        case CALLBACK_PARAMETER_VALUE_CHANGED:
            sendOscControl(value1, value3);
            break;

        case CALLBACK_SHOW_GUI:
            if (! isOscControlRegistered())
            {
                if (value1 != 1)
                    gCloseNow = true;
            }
            else
            {
                // show-gui button
                fEngine->osc_send_bridge_configure(CARLA_BRIDGE_MSG_HIDE_GUI, "");
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
    CarlaBackend::CarlaEngine* fEngine;
    CarlaBackend::CarlaPlugin* fPlugin;

    int fTimerId;

    void timerEvent(QTimerEvent* const event)
    {
        if (event->timerId() == fTimerId)
            idle();

        QObject::timerEvent(event);
    }

    static void callback(void* ptr, CarlaBackend::CallbackType action, unsigned int pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        return ((CarlaPluginClient*)ptr)->handleCallback(action, value1, value2, value3, valueStr);

        // unused
        (void)pluginId;
    }
};

// -------------------------------------------------------------------------

int CarlaBridgeOsc::handleMsgShow()
{
    carla_debug("CarlaBridgeOsc::handleMsgShow()");
    CARLA_ASSERT(kClient != nullptr);

    if (kClient == nullptr)
        return 1;

    carla_show_gui(0, true);

    return 0;
}

int CarlaBridgeOsc::handleMsgHide()
{
    carla_debug("CarlaBridgeOsc::handleMsgHide()");
    CARLA_ASSERT(kClient != nullptr);

    if (kClient == nullptr)
        return 1;

    carla_show_gui(0, false);

    return 0;
}

int CarlaBridgeOsc::handleMsgQuit()
{
    carla_debug("CarlaBridgeOsc::handleMsgQuit()");
    CARLA_ASSERT(kClient != nullptr);

    gCloseNow = true;

    return 0;
}

// -------------------------------------------------------------------------

int CarlaBridgeOsc::handleMsgPluginSaveNow()
{
    carla_debug("CarlaBridgeOsc::handleMsgPluginSaveNow()");
    CARLA_ASSERT(kClient != nullptr);

    if (kClient == nullptr)
        return 1;

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)kClient;
    plugClient->saveNow();

    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetChunk(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaBridgeOsc::handleMsgPluginSaveNow()");
    CARLA_ASSERT(kClient != nullptr);
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "s");

    if (kClient == nullptr)
        return 1;

    const char* const chunkFile = (const char*)&argv[0]->s;

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)kClient;
    plugClient->setChunkData(chunkFile);

    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetCustomData(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    carla_debug("CarlaBridgeOsc::handleMsgPluginSaveNow()");
    CARLA_ASSERT(kClient != nullptr);
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(3, "sss");

    if (kClient == nullptr)
        return 1;

    const char* const type  = (const char*)&argv[0]->s;
    const char* const key   = (const char*)&argv[1]->s;
    const char* const value = (const char*)&argv[2]->s;

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)kClient;
    plugClient->setCustomData(type, key, value);

    return 0;
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    CARLA_BRIDGE_USE_NAMESPACE

    if (argc != 6 && argc != 7)
    {
        carla_stdout("usage: %s <osc-url|\"null\"> <type> <filename> <name|\"(none)\"> <label>", argv[0]);
        return 1;
    }

    const char* const oscUrl   = argv[1];
    const char* const stype    = argv[2];
    const char* const filename = argv[3];
    const char*       name     = argv[4];
    const char* const label    = argv[5];

    const bool useBridge = (argc == 7);
    const bool useOsc    = std::strcmp(oscUrl, "null");

    if (std::strcmp(name, "(none)") == 0)
        name = nullptr;

    char bridgeBaseAudioName[6+1]   = { 0 };
    char bridgeBaseControlName[6+1] = { 0 };

    if (useBridge)
    {
        std::strncpy(bridgeBaseAudioName,   argv[6], 6);
        std::strncpy(bridgeBaseControlName, argv[6]+6, 6);
    }

    CarlaBackend::PluginType itype(CarlaBackend::getPluginTypeFromString(stype));

    if (itype == CarlaBackend::PLUGIN_NONE)
    {
        carla_stderr("Invalid plugin type '%s'", stype);
        return 1;
    }

    QApplication app(argc, argv, true);
    app.setQuitOnLastWindowClosed(false);

    // Init Plugin client
    CarlaPluginClient client(useBridge, (name != nullptr) ? name : label, bridgeBaseAudioName, bridgeBaseControlName);

    // Init OSC
    if (useOsc)
        client.oscInit(oscUrl);

    // Listen for ctrl+c or sigint/sigterm events
    initSignalHandler();

    const void* extraStuff = nullptr;

    if (itype == CarlaBackend::PLUGIN_DSSI)
        extraStuff = CarlaBackend::findDSSIGUI(filename, label);

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
                if (pluginInfo->hints & CarlaBackend::PLUGIN_HAS_GUI)
                    carla_show_gui(0, true);
            }
        }

        client.ready();

        ret = app.exec();

        carla_remove_plugin(0);
    }
    else
    {
        const char* const lastError = carla_get_last_error();
        carla_stderr("Plugin failed to load, error was:\n%s", lastError);

        if (useOsc)
            client.sendOscBridgeError(lastError);

        ret = 1;
    }

    if (extraStuff != nullptr && itype == CarlaBackend::PLUGIN_DSSI)
        delete[] (const char*)extraStuff;

    // Close OSC
    if (useOsc)
        client.oscClose();

    return ret;
}
