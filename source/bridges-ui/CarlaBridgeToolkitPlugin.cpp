/*
 * Carla Bridge Toolkit, Plugin version
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaBridgeToolkit.hpp"

#include "CarlaPluginUI.hpp"

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

class CarlaBridgeToolkitPlugin : public CarlaBridgeToolkit,
                                 private CarlaPluginUI::CloseCallback
{
public:
    CarlaBridgeToolkitPlugin(CarlaBridgeClient* const client, const char* const windowTitle)
        : CarlaBridgeToolkit(client, windowTitle),
          fUI(nullptr),
          fIdling(false)
    {
        carla_debug("CarlaBridgeToolkitPlugin::CarlaBridgeToolkitPlugin(%p, \"%s\")", client, windowTitle);
    }

    ~CarlaBridgeToolkitPlugin() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI == nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::~CarlaBridgeToolkitPlugin()");
    }

    void init() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI == nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::init()");

#if defined(CARLA_OS_MAC) && defined(BRIDGE_COCOA)
        fUI = CarlaPluginUI::newCocoa(this, 0);
#elif defined(CARLA_OS_WIN) && defined(BRIDGE_HWND)
        fUI = CarlaPluginUI::newWindows(this, 0);
#elif defined(HAVE_X11) && defined(BRIDGE_X11)
        fUI = CarlaPluginUI::newX11(this, 0);
#endif
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->setTitle(kWindowTitle);
    }

    void exec(const bool showUI) override
    {
        CARLA_SAFE_ASSERT_RETURN(kClient != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::exec(%s)", bool2str(showUI));

        if (showUI)
            fUI->show();

        fIdling = true;

        for (; fIdling;)
        {
            fUI->idle();
            kClient->uiIdle();

            if (! kClient->oscIdle())
                break;

            carla_msleep(20);
        }
    }

    void quit() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::quit()");

        fIdling = false;

        delete fUI;
        fUI = nullptr;
    }

    void show() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::show()");

        fUI->show();
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::hide()");

        fUI->hide();
    }

    void resize(int width, int height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::resize(%i, %i)", width, height);

        fUI->setSize(width, height, false);
    }

    void* getContainerId() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr, nullptr);

        return fUI->getPtr();
    }

#ifdef HAVE_X11
    void* getContainerId2() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr, nullptr);

        return fUI->getDisplay();
    }
#endif

    // ---------------------------------------------------------------------

protected:
    void handlePluginUIClosed() override
    {
        fIdling = false;
    }

    // ---------------------------------------------------------------------

private:
    CarlaPluginUI* fUI;
    bool fIdling;
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const windowTitle)
{
    return new CarlaBridgeToolkitPlugin(client, windowTitle);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#include "CarlaPluginUI.cpp"

// -------------------------------------------------------------------------
