/*
 * Carla Plugin bridge code
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifdef BRIDGE_PLUGIN

#include "carla_bridge_client.hpp"
#include "carla_bridge_toolkit.hpp"
#include "carla_plugin.hpp"
#include "../carla/Shared.hpp"

#include <set>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtXml/QDomDocument>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QApplication>
#else
# include <QtGui/QApplication>
#endif

#ifdef Q_OS_UNIX
# include <signal.h>
#endif

// -------------------------------------------------------------------------

static int    qargc     = 0;
static char** qargv     = nullptr;
static bool   qCloseNow = false;
static bool   qSaveNow  = false;

#if defined(Q_OS_HAIKU) || defined(Q_OS_UNIX)
void closeSignalHandler(int)
{
    qCloseNow = true;
}
void saveSignalHandler(int)
{
    qSaveNow = true;
}
#elif defined(Q_OS_WIN)
BOOL WINAPI closeSignalHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        qCloseNow = true;
        return TRUE;
    }

    return FALSE;
}
#endif

void initSignalHandler()
{
#if defined(Q_OS_HAIKU) || defined(Q_OS_UNIX)
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
#elif defined(Q_OS_WIN)
    SetConsoleCtrlHandler(closeSignalHandler, TRUE);
#endif
}

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

class CarlaBridgeToolkitPlugin : public CarlaBridgeToolkit,
                                 public CarlaBackend::CarlaPluginGUI::Callback
{
public:
    CarlaBridgeToolkitPlugin(CarlaBridgeClient* const client, const char* const uiTitle)
        : CarlaBridgeToolkit(client, uiTitle)
    {
        qDebug("CarlaBridgeToolkitPlugin::CarlaBridgeToolkitPlugin(%p, \"%s\")", client, uiTitle);

        app = nullptr;
        gui = nullptr;

        m_hasUI  = false;
        m_uiQuit = false;
        m_uiShow = false;

        init();
    }

    ~CarlaBridgeToolkitPlugin()
    {
        qDebug("CarlaBridgeToolkitPlugin::~CarlaBridgeToolkitPlugin()");

        if (gui)
        {
            gui->close();

            delete gui;
            gui = nullptr;
        }

        if (app)
        {
            if (! app->closingDown())
                app->quit();

            delete app;
            app = nullptr;
        }
    }

    void init()
    {
        qDebug("CarlaBridgeToolkitPlugin::init()");
        CARLA_ASSERT(! app);
        CARLA_ASSERT(! gui);

        app = new QApplication(qargc, qargv);

        gui = new CarlaBackend::CarlaPluginGUI(nullptr, this);
    }

    void exec(const bool showGui)
    {
        qDebug("CarlaBridgeToolkitPlugin::exec(%s)", bool2str(showGui));
        CARLA_ASSERT(app);
        CARLA_ASSERT(gui);
        CARLA_ASSERT(client);

        if (showGui)
        {
            if (m_hasUI)
                show();
        }
        else
        {
            app->setQuitOnLastWindowClosed(false);
            client->sendOscUpdate();
            client->sendOscBridgeUpdate();
        }

        m_uiQuit = showGui;

        // Main loop
        app->exec();
    }

    void quit()
    {
        qDebug("CarlaBridgeToolkitPlugin::quit()");
        CARLA_ASSERT(app);

        if (app && ! app->closingDown())
            app->quit();
    }

    void show();
    void hide();

    void resize(const int width, const int height)
    {
        qDebug("CarlaBridgeToolkitPlugin::resize(%i, %i)", width, height);
        CARLA_ASSERT(gui);

        if (gui)
            gui->setNewSize(width, height);
    }

    GuiContainer* getContainer() const
    {
        CARLA_ASSERT(gui);

        if (gui)
            return gui->getContainer();

        return nullptr;
    }

    void* getContainerId()
    {
        CARLA_ASSERT(gui);

        if (gui)
            return (void*)gui->getWinId();

        return nullptr;
    }

    void setHasUI(const bool hasUI, const bool showUI)
    {
        m_hasUI  = hasUI;
        m_uiShow = showUI;
    }

    void guiClosedCallback();

protected:
    QApplication* app;
    CarlaBackend::CarlaPluginGUI* gui;

private:
    bool m_hasUI;
    bool m_uiQuit;
    bool m_uiShow;
};

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const uiTitle)
{
    return new CarlaBridgeToolkitPlugin(client, uiTitle);
}

// -------------------------------------------------------------------------

class CarlaPluginClient : public CarlaBridgeClient,
                          public QObject
{
public:
    CarlaPluginClient()
        : CarlaBridgeClient(""),
          QObject(nullptr)
    {
        qDebug("CarlaPluginClient::CarlaPluginClient()");

        msgTimerGUI = 0;
        msgTimerOSC = 0;

        needsResize = 0;
        nextWidth   = 0;
        nextHeight  = 0;

        engine = nullptr;
        plugin = nullptr;
    }

    ~CarlaPluginClient()
    {
        qDebug("CarlaPluginClient::~CarlaPluginClient()");
        CARLA_ASSERT(msgTimerGUI == 0);
        CARLA_ASSERT(msgTimerOSC == 0);
    }

    // ---------------------------------------------------------------------

    void init()
    {
        CARLA_ASSERT(plugin);

        msgTimerGUI = startTimer(50);
        msgTimerOSC = startTimer(25);

        if (! plugin)
            return;

        bool guiResizable;
        CarlaBackend::GuiType guiType;
        plugin->getGuiInfo(&guiType, &guiResizable);

        CarlaBridgeToolkitPlugin* const plugToolkit = (CarlaBridgeToolkitPlugin*)m_toolkit;

        if (guiType == CarlaBackend::GUI_INTERNAL_QT4 || guiType == CarlaBackend::GUI_INTERNAL_COCOA || guiType == CarlaBackend::GUI_INTERNAL_HWND || guiType == CarlaBackend::GUI_INTERNAL_X11)
        {
            plugin->setGuiContainer(plugToolkit->getContainer());
            plugToolkit->setHasUI(true, true);
        }
        else
        {
            plugToolkit->setHasUI(guiType != CarlaBackend::GUI_NONE, false);
        }
    }

    void quit()
    {
        engine = nullptr;
        plugin = nullptr;

        if (msgTimerGUI != 0)
        {
            killTimer(msgTimerGUI);
            msgTimerGUI = 0;
        }

        if (msgTimerOSC != 0)
        {
            killTimer(msgTimerOSC);
            msgTimerOSC = 0;
        }
    }

    // ---------------------------------------------------------------------

    void setEngine(CarlaBackend::CarlaEngine* const engine)
    {
        qDebug("CarlaPluginClient::setEngine(%p)", engine);
        CARLA_ASSERT(engine);

        this->engine = engine;

        engine->setOscBridgeData(m_oscData);
    }

    void setPlugin(CarlaBackend::CarlaPlugin* const plugin)
    {
        qDebug("CarlaPluginClient::setPlugin(%p)", plugin);
        CARLA_ASSERT(plugin);

        this->plugin = plugin;

        // load carla plugin preset if possible
        if (const char* const pName = plugin->name())
        {
            QFile pFile(QDir::currentPath() + QDir::separator() + pName + ".carxs");
            qDebug("Trying to load plugin preset file '%s'", pFile.fileName().toUtf8().constData());

            if (! /*(*/pFile.exists() /*&& pFile.isReadable())*/)
            {
                qDebug("Plugin preset file doesn't exist or is not readable");
                return;
            }

            if (! pFile.open(QFile::ReadOnly))
            {
                qWarning("Plugin preset file read failed");
                return;
            }

            QDomDocument xml;
            xml.setContent(pFile.readAll());

            QDomElement xmlNode(xml.documentElement());

            if (xmlNode.tagName() == "CARLA-PRESET")
            {
                loadStateDict(getSaveStateDictFromXML(xmlNode));
            }
            else
                qWarning("Plugin preset file is not valid or corrupted");

            pFile.close();
        }
    }

    // ---------------------------------------------------------------------

    void loadStateDict(const CarlaSaveState* const content)
    {
        CARLA_ASSERT(content);

        if (! content)
            return;

        qDebug("Loading plugin state now...");

        // ---------------------------------------------------------------------
        // Part 1 - set custom data (except chunks)

        foreach (const CarlaStateCustomData& customData, content->customData)
        {
            if (customData.type != CUSTOM_DATA_CHUNK)
            {
                const char* const type  = customData.type.toUtf8().constData();
                const char* const key   = customData.key.toUtf8().constData();
                const char* const value = customData.value.toUtf8().constData();

                plugin->setCustomData(type, key, value, true);
            }
        }

        // ---------------------------------------------------------------------
        // Part 2 - set program

        int32_t programId = -1;

        if (! content->currentProgramName.isEmpty())
        {
            const uint32_t programCount = plugin->programCount();

            char strBuf[STR_MAX] = { 0 };
            plugin->getProgramName(content->currentProgramIndex, strBuf);
            QString testProgramName(strBuf);

            // Program name matches
            if (content->currentProgramName == testProgramName)
            {
                programId = content->currentProgramIndex;
            }

            // index < count
            else if (content->currentProgramIndex < (int32_t)programCount)
            {
                programId = content->currentProgramIndex;
            }

            // index not valid, try to find by name
            else
            {
                for (uint32_t i=0; i < programCount;  i++)
                {
                    plugin->getProgramName(i, strBuf);
                    testProgramName = QString(strBuf);

                    if (content->currentProgramName == testProgramName)
                    {
                        programId = i;
                        break;
                    }
                }
            }
        }

        // set program now, if valid
        if (programId >= 0)
        {
            plugin->setProgram(programId, true, true, false, true);
        }

        // ---------------------------------------------------------------------
        // Part 3 - set midi program

        if (content->currentMidiBank >= 0 and content->currentMidiProgram >= 0)
        {
            const uint32_t midiProgramCount = plugin->midiProgramCount();

            for (uint32_t i=0; i < midiProgramCount;  i++)
            {
                const MidiProgramData* const midiProgramData = plugin->midiProgramData(i);

                if ((int32_t)midiProgramData->bank == content->currentMidiBank && (int32_t)midiProgramData->program == content->currentMidiProgram)
                {
                    plugin->setMidiProgram(i, true, true, false, true);
                    break;
                }
            }
        }

        // ---------------------------------------------------------------------
        // Part 4a - get plugin parameter symbols

        struct ParamSymbol {
            uint32_t index;
            QString  symbol;

            ParamSymbol() {}

            ParamSymbol(uint32_t index_, const char* symbol_)
                : index(index_),
                  symbol(symbol_) {}
        };

        QVector<ParamSymbol> paramSymbols;

        foreach (const CarlaStateParameter& parameter, content->parameters)
        {
            if (! parameter.symbol.isEmpty())
            {
                char strBuf[STR_MAX] = { 0 };
                plugin->getParameterSymbol(parameter.index, strBuf);

                if (strBuf[0] != 0)
                {
                    ParamSymbol ps(parameter.index, strBuf);
                    paramSymbols.append(ps);
                }
            }
        }

        // ---------------------------------------------------------------------
        // Part 4b - set parameter values (carefully)

        const double sampleRate = engine->getSampleRate();

        foreach (const CarlaStateParameter& parameter, content->parameters)
        {
            int32_t index = -1;

            if (content->type == "LADSPA")
            {
                // Try to set by symbol, otherwise use index
                if (! parameter.symbol.isEmpty())
                {
                    bool breaked = false;

                    foreach (const ParamSymbol& ps, paramSymbols)
                    {
                        if (parameter.symbol == ps.symbol)
                        {
                            index = ps.index;
                            breaked = true;
                            break;
                        }
                    }

                    if (! breaked)
                        index = parameter.index;
                }
                else
                    index = parameter.index;
            }
            else if (content->type == "LV2")
            {
                // Symbol only
                if (! parameter.symbol.isEmpty())
                {
                    bool breaked = false;

                    foreach (const ParamSymbol& ps, paramSymbols)
                    {
                        if (parameter.symbol == ps.symbol)
                        {
                            index = ps.index;
                            breaked = true;
                            break;
                        }
                    }

                    if (! breaked)
                        qWarning("Failed to find LV2 parameter symbol for '%s')", parameter.symbol.toUtf8().constData());
                }
                else
                    qWarning("LV2 Plugin parameter '%s' has no symbol", parameter.name.toUtf8().constData());
            }
            else
            {
                // Index only
                index = parameter.index;
            }

            // Now set parameter
            if (index >= 0)
            {
                const ParameterData* const paramData = plugin->parameterData(index);
                double value = parameter.value;

                if (paramData->hints & PARAMETER_USES_SAMPLERATE)
                    value *= sampleRate;

                plugin->setParameterValue(index, value, true, true, false);
                plugin->setParameterMidiCC(index, parameter.midiCC, true, false);
                plugin->setParameterMidiChannel(index, parameter.midiChannel-1, true, false);
            }
            else
                qWarning("Could not set parameter data for '%s')", parameter.name.toUtf8().constData());
        }

        // ---------------------------------------------------------------------
        // Part 5 - set chunk data

        foreach (const CarlaStateCustomData& customData, content->customData)
        {
            if (customData.type == CUSTOM_DATA_CHUNK)
            {
                const char* const type  = customData.type.toUtf8().constData();
                const char* const key   = customData.key.toUtf8().constData();
                const char* const value = customData.value.toUtf8().constData();

                plugin->setCustomData(type, key, value, true);
            }
        }

        if (! content->chunk.isEmpty())
            plugin->setChunkData(content->chunk.toUtf8().constData());

        qDebug("Loading plugin state now finished");
    }

    void guiClosed()
    {
        CARLA_ASSERT(engine);

        if (engine)
            engine->osc_send_bridge_configure(CarlaBackend::CARLA_BRIDGE_MSG_HIDE_GUI, "");
    }

    void showPluginGui(const bool yesNo)
    {
        CARLA_ASSERT(plugin);

        if (plugin)
            plugin->showGui(yesNo);
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(const int32_t rindex, const double value)
    {
        qDebug("CarlaPluginClient::setParameter(%i, %g)", rindex, value);
        CARLA_ASSERT(plugin);

        if (plugin)
            plugin->setParameterValueByRIndex(rindex, value, true, true, false);
    }

    void setProgram(const uint32_t index)
    {
        qDebug("CarlaPluginClient::setProgram(%i)", index);
        CARLA_ASSERT(engine);
        CARLA_ASSERT(plugin);
        CARLA_ASSERT(index < plugin->programCount());

        if (! (plugin && engine))
            return;
        if (index >= plugin->programCount())
            return;

        plugin->setProgram(index, true, true, false, true);

        double value;
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
        {
            value = plugin->getParameterValue(i);
            engine->osc_send_bridge_set_parameter_value(i, value);
            engine->osc_send_bridge_set_default_value(i, value);
        }
    }

    void setMidiProgram(const uint32_t index)
    {
        qDebug("CarlaPluginClient::setMidiProgram(%i)", index);
        CARLA_ASSERT(engine);
        CARLA_ASSERT(plugin);

        if (! (plugin && engine))
            return;

        plugin->setMidiProgram(index, true, true, false, true);

        double value;
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
        {
            value = plugin->getParameterValue(i);
            engine->osc_send_bridge_set_parameter_value(i, value);
            engine->osc_send_bridge_set_default_value(i, value);
        }
    }

    void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        qDebug("CarlaPluginClient::noteOn(%i, %i, %i)", channel, note, velo);
        CARLA_ASSERT(plugin);
        CARLA_ASSERT(velo > 0);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, velo, true, true, false);
    }

    void noteOff(const uint8_t channel, const uint8_t note)
    {
        qDebug("CarlaPluginClient::noteOff(%i, %i)", channel, note);
        CARLA_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, 0, true, true, false);
    }

    // ---------------------------------------------------------------------
    // plugin

    void saveNow()
    {
        qDebug("CarlaPluginClient::saveNow()");
        CARLA_ASSERT(plugin);
        CARLA_ASSERT(engine);

        if (! (plugin && engine))
            return;

        plugin->prepareForSave();

        for (uint32_t i=0; i < plugin->customDataCount(); i++)
        {
            const CarlaBackend::CustomData* const cdata = plugin->customData(i);
            engine->osc_send_bridge_set_custom_data(cdata->type, cdata->key, cdata->value);
        }

        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
        {
            void* data = nullptr;
            int32_t dataSize = plugin->chunkData(&data);

            if (data && dataSize >= 4)
            {
                QString filePath;
                filePath = QDir::tempPath();
#ifdef Q_OS_WIN
                filePath += "\\.CarlaChunk_";
#else
                filePath += "/.CarlaChunk_";
#endif
                filePath += plugin->name();

                QFile file(filePath);

                if (file.open(QIODevice::WriteOnly))
                {
                    QByteArray chunk((const char*)data, dataSize);
                    file.write(chunk);
                    file.close();
                    engine->osc_send_bridge_set_chunk_data(filePath.toUtf8().constData());
                }
            }
        }

        engine->osc_send_bridge_configure(CarlaBackend::CARLA_BRIDGE_MSG_SAVED, "");
    }

    void setCustomData(const char* const type, const char* const key, const char* const value)
    {
        qDebug("CarlaPluginClient::setCustomData(\"%s\", \"%s\", \"%s\")", type, key, value);
        CARLA_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setCustomData(type, key, value, true);
    }

    void setChunkData(const char* const filePath)
    {
        qDebug("CarlaPluginClient::setChunkData(\"%s\")", filePath);
        CARLA_ASSERT(plugin);

        if (! plugin)
            return;

        QString chunkFilePath(filePath);

#ifdef Q_OS_WIN
        if (chunkFilePath.startsWith("/"))
        {
            // running under Wine, posix host
            chunkFilePath = chunkFilePath.replace(0, 1, "Z:/");
            chunkFilePath = QDir::toNativeSeparators(chunkFilePath);
        }
#endif
        QFile chunkFile(chunkFilePath);

        if (plugin && chunkFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&chunkFile);
            QString stringData(in.readAll());
            chunkFile.close();
            chunkFile.remove();

            plugin->setChunkData(stringData.toUtf8().constData());
        }
    }

    // ---------------------------------------------------------------------
    // callback

    static void callback(void* const ptr, CarlaBackend::CallbackType const action, const unsigned short, const int value1, const int value2, const double value3, const char* const valueStr)
    {
        CARLA_ASSERT(ptr);

        if (CarlaPluginClient* const _this_ = (CarlaPluginClient*)ptr)
            _this_->handleCallback(action, value1, value2, value3, valueStr);
    }

    // ---------------------------------------------------------------------

protected:
    int  msgTimerGUI;
    int  msgTimerOSC;

    bool needsResize;
    int  nextWidth;
    int  nextHeight;

    CarlaBackend::CarlaEngine* engine;
    CarlaBackend::CarlaPlugin* plugin;

    std::set<int32_t> parametersToUpdate;

    void handleCallback(const CarlaBackend::CallbackType action, const int value1, const int value2, const double value3, const char* const valueStr)
    {
        CARLA_BACKEND_USE_NAMESPACE
        qDebug("CarlaPluginClient::handleCallback(%s, %i, %i, %g \"%s\")", CallbackType2Str(action), value1, value2, value3, valueStr);

        if (! engine)
            return;

        switch (action)
        {
        case CALLBACK_DEBUG:
            break;

        case CALLBACK_PARAMETER_VALUE_CHANGED:
            parametersToUpdate.insert(value1);
            break;

        case CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
            // todo, unused?
            break;

        case CALLBACK_PARAMETER_MIDI_CC_CHANGED:
            // todo, unused?
            break;

        case CALLBACK_PROGRAM_CHANGED:
            engine->osc_send_bridge_set_program(value1);
            break;

        case CALLBACK_MIDI_PROGRAM_CHANGED:
            engine->osc_send_bridge_set_midi_program(value1);
            break;

        case CALLBACK_NOTE_ON:
            // todo
            break;

        case CALLBACK_NOTE_OFF:
            // todo
            break;

        case CALLBACK_SHOW_GUI:
            if (value1 == 0)
            {
                CarlaBridgeToolkitPlugin* const plugToolkit = (CarlaBridgeToolkitPlugin*)m_toolkit;
                plugToolkit->guiClosedCallback();
            }
            break;

        case CALLBACK_RESIZE_GUI:
            nextWidth   = value1;
            nextHeight  = value2;
            needsResize = true;
            break;

        case CALLBACK_UPDATE:
            // todo
            break;

        case CALLBACK_RELOAD_INFO:
            // todo
            break;

        case CALLBACK_RELOAD_PARAMETERS:
            // todo
            break;

        case CALLBACK_RELOAD_PROGRAMS:
            // todo
            break;

        case CALLBACK_RELOAD_ALL:
            // todo
            break;

        case CALLBACK_NSM_ANNOUNCE:
        case CALLBACK_NSM_OPEN1:
        case CALLBACK_NSM_OPEN2:
        case CALLBACK_NSM_SAVE:
            break;

        case CALLBACK_ERROR:
            break;

        case CALLBACK_QUIT:
            m_toolkit->quit();
            break;
        }
    }

    void timerEvent(QTimerEvent* const event)
    {
        if (qCloseNow)
            return toolkitQuit();

        if (qSaveNow)
        {
            // TODO
            qSaveNow = false;
        }

        if (event->timerId() == msgTimerGUI)
        {
            if (plugin)
                plugin->idleGui();
        }
        else if (event->timerId() == msgTimerOSC)
        {
            if (isOscControlRegistered())
                oscIdle();
        }

        QObject::timerEvent(event);
    }
};

// -------------------------------------------------------------------------

void CarlaBridgeToolkitPlugin::show()
{
    qDebug("CarlaBridgeToolkitPlugin::show()");
    CARLA_ASSERT(gui);

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)client;

    plugClient->showPluginGui(true);

    if (gui && m_uiShow)
        gui->setVisible(true);
}

void CarlaBridgeToolkitPlugin::hide()
{
    qDebug("CarlaBridgeToolkitPlugin::hide()");
    CARLA_ASSERT(gui);

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)client;

    if (gui && m_uiShow)
        gui->setVisible(false);

    plugClient->showPluginGui(false);
}

void CarlaBridgeToolkitPlugin::guiClosedCallback()
{
    qDebug("CarlaBridgeToolkitPlugin::guiClosedCallback()");

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)client;

    if (m_uiQuit)
    {
        plugClient->quit();
        quit();
    }
    else
    {
        plugClient->guiClosed();
    }
}

// -------------------------------------------------------------------------

int CarlaBridgeOsc::handleMsgPluginSaveNow()
{
    qDebug("CarlaBridgeOsc::handleMsgPluginSaveNow()");

    if (! client)
        return 1;

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)client;
    plugClient->saveNow();

    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetChunk(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgPluginSaveNow()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "s");

    if (! client)
        return 1;

    const char* const chunkFile = (const char*)&argv[0]->s;

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)client;
    plugClient->setChunkData(chunkFile);

    return 0;
}

int CarlaBridgeOsc::handleMsgPluginSetCustomData(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgPluginSaveNow()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(3, "sss");

    if (! client)
        return 1;

    const char* const type  = (const char*)&argv[0]->s;
    const char* const key   = (const char*)&argv[1]->s;
    const char* const value = (const char*)&argv[2]->s;

    CarlaPluginClient* const plugClient = (CarlaPluginClient*)client;
    plugClient->setCustomData(type, key, value);

    return 0;
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    CARLA_BRIDGE_USE_NAMESPACE

    if (argc != 6)
    {
        qWarning("usage: %s <osc-url|\"null\"> <type> <filename> <name|\"(none)\"> <label>", argv[0]);
        return 1;
    }

    const char* const oscUrl   = argv[1];
    const char* const stype    = argv[2];
    const char* const filename = argv[3];
    const char*       name     = argv[4];
    const char* const label    = argv[5];

    const bool useOsc = strcmp(oscUrl, "null");

    if (strcmp(name, "(none)") == 0)
        name = nullptr;

    CarlaBackend::PluginType itype;

    if (strcmp(stype, "LADSPA") == 0)
        itype = CarlaBackend::PLUGIN_LADSPA;
    else if (strcmp(stype, "DSSI") == 0)
        itype = CarlaBackend::PLUGIN_DSSI;
    else if (strcmp(stype, "LV2") == 0)
        itype = CarlaBackend::PLUGIN_LV2;
    else if (strcmp(stype, "VST") == 0)
        itype = CarlaBackend::PLUGIN_VST;
    else
    {
        qWarning("Invalid plugin type '%s'", stype);
        return 1;
    }

    // Init Plugin client
    CarlaPluginClient client;

    // Init OSC
    if (useOsc && ! client.oscInit(oscUrl))
    {
        return 1;
    }

    // Listen for ctrl+c or sigint/sigterm events
    initSignalHandler();

    // Init backend engine
    CarlaBackend::CarlaEngine* engine = CarlaBackend::CarlaEngine::newDriverByName("JACK");
    engine->setCallback(client.callback, &client);
    client.setEngine(engine);

    // Init engine
    CarlaString engName(name ? name : label);
    engName += " (master)";
    engName.toBasic();
    engName.truncate(engine->maxClientNameSize());

    if (! engine->init(engName))
    {
        if (const char* const lastError = engine->getLastError())
        {
            qWarning("Bridge engine failed to start, error was:\n%s", lastError);
            client.sendOscBridgeError(lastError);
        }

        engine->close();
        delete engine;

        return 2;
    }

    void* extraStuff = nullptr;

    if (itype == CarlaBackend::PLUGIN_DSSI)
        extraStuff = findDSSIGUI(filename, name, label);

    // Init plugin
    short id = engine->addPlugin(itype, filename, name, label, extraStuff);
    int ret;

    if (id >= 0 && id < CarlaBackend::MAX_PLUGINS)
    {
        CarlaBackend::CarlaPlugin* const plugin = engine->getPlugin(id);
        client.setPlugin(plugin);

        if (! useOsc)
            plugin->setActive(true, false, false);

        client.init();
        client.toolkitExec(!useOsc);
        client.quit();

        ret = 0;
    }
    else
    {
        const char* const lastError = engine->getLastError();
        qWarning("Plugin failed to load, error was:\n%s", lastError);

        if (useOsc)
            client.sendOscBridgeError(lastError);

        ret = 1;
    }

    if (extraStuff && itype == CarlaBackend::PLUGIN_DSSI)
        free((char*)extraStuff);

    engine->aboutToClose();
    engine->removeAllPlugins();
    engine->close();
    delete engine;

    // Close OSC
    if (useOsc)
    {
        client.oscClose();
    }

    return ret;
}

#endif // BRIDGE_PLUGIN
