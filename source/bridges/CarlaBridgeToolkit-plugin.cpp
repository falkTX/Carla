/*
 * Carla Bridge Toolkit, Plugin version
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#include "carla_bridge_client.hpp"
#include "carla_bridge_toolkit.hpp"
#include "carla_plugin.hpp"

CARLA_BRIDGE_START_NAMESPACE

static int   qargc = 0;
static char* qargv[0] = {};

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

        m_uiQuit = false;

        init();
    }

    ~CarlaBridgeToolkitPlugin()
    {
        qDebug("CarlaBridgeToolkitPlugin::~CarlaBridgeToolkitPlugin()");
        CARLA_ASSERT(! app);
        CARLA_ASSERT(! gui);
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

    void show()
    {
        qDebug("CarlaBridgeToolkitPlugin::show()");
        CARLA_ASSERT(gui);

        if (gui && m_uiShow)
            gui->setVisible(true);
    }

    void hide()
    {
        qDebug("CarlaBridgeToolkitPlugin::hide()");
        CARLA_ASSERT(gui);

        if (gui && m_uiShow)
            gui->setVisible(false);
    }

    void resize(const int width, const int height)
    {
        qDebug("CarlaBridgeToolkitPlugin::resize(%i, %i)", width, height);
        CARLA_ASSERT(gui);

        if (gui)
            gui->setNewSize(width, height);
    }

protected:
    QApplication* app;
    CarlaBackend::CarlaPluginGUI* gui;

    void guiClosedCallback();

private:
    bool m_uiQuit;
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const uiTitle)
{
    return new CarlaBridgeToolkitPlugin(client, uiTitle);
}

CARLA_BRIDGE_END_NAMESPACE
