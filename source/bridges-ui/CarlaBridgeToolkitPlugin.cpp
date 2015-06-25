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

#include "CarlaBridgeUI.hpp"
#include "CarlaBridgeToolkit.hpp"

#include "CarlaPluginUI.hpp"

#if defined(CARLA_OS_WIN) || defined(CARLA_OS_MAC)
# include "juce_events.h"
using juce::MessageManager;
using juce::ScopedJuceInitialiser_GUI;
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

class CarlaBridgeToolkitPlugin : public CarlaBridgeToolkit,
                                 private CarlaPluginUI::CloseCallback
{
public:
    CarlaBridgeToolkitPlugin(CarlaBridgeUI* const u)
        : CarlaBridgeToolkit(u),
          fUI(nullptr),
          fIdling(false)
#if defined(CARLA_OS_WIN) || defined(CARLA_OS_MAC)
        , kJuceInit()
#endif
    {
        carla_debug("CarlaBridgeToolkitPlugin::CarlaBridgeToolkitPlugin(%p)", u);
    }

    ~CarlaBridgeToolkitPlugin() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI == nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::~CarlaBridgeToolkitPlugin()");
    }

    bool init(const int /*argc*/, const char** /*argv[]*/) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI == nullptr, false);
        carla_debug("CarlaBridgeToolkitPlugin::init()");

        const CarlaBridgeUI::Options& options(ui->getOptions());

#if defined(CARLA_OS_MAC) && defined(BRIDGE_COCOA)
        fUI = CarlaPluginUI::newCocoa(this, 0, options.isResizable);
#elif defined(CARLA_OS_WIN) && defined(BRIDGE_HWND)
        fUI = CarlaPluginUI::newWindows(this, 0, options.isResizable);
#elif defined(HAVE_X11) && defined(BRIDGE_X11)
        fUI = CarlaPluginUI::newX11(this, 0, options.isResizable);
#endif
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr, false);

        fUI->setTitle(options.windowTitle.buffer());

        if (options.transientWindowId != 0)
            fUI->setTransientWinId(options.transientWindowId);

        return true;
    }

    void exec(const bool showUI) override
    {
        CARLA_SAFE_ASSERT_RETURN(ui != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::exec(%s)", bool2str(showUI));

        if (const char* const winIdStr = std::getenv("ENGINE_OPTION_FRONTEND_WIN_ID"))
        {
            if (const long long winId = std::strtoll(winIdStr, nullptr, 16))
                fUI->setTransientWinId(static_cast<uintptr_t>(winId));
        }

        if (showUI)
            fUI->show();

        fIdling = true;

        for (; fIdling;)
        {
            if (ui->isPipeRunning())
                ui->idlePipe();

            ui->idleUI();

            fUI->idle();

#if defined(CARLA_OS_WIN) || defined(CARLA_OS_MAC)
            if (MessageManager* const msgMgr = MessageManager::getInstance())
                msgMgr->runDispatchLoopUntil(20);
#else
            carla_msleep(20);
#endif
        }
    }

    void quit() override
    {
        carla_debug("CarlaBridgeToolkitPlugin::quit()");

        fIdling = false;

        if (fUI != nullptr)
        {
            fUI->hide();
            delete fUI;
            fUI = nullptr;
        }
    }

    void show() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::show()");

        fUI->show();
    }

    void focus() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::focus()");

        fUI->focus();
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::hide()");

        fUI->hide();
    }

    void setSize(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(width > 0,);
        CARLA_SAFE_ASSERT_RETURN(height > 0,);
        carla_debug("CarlaBridgeToolkitPlugin::resize(%i, %i)", width, height);

        fUI->setSize(width, height, false);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI != nullptr,);
        carla_debug("CarlaBridgeToolkitPlugin::setTitle(\"%s\")", title);

        fUI->setTitle(title);
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

    void handlePluginUIResized(const uint width, const uint height) override
    {
        ui->uiResized(width, height);
    }

    // ---------------------------------------------------------------------

private:
    CarlaPluginUI* fUI;
    bool fIdling;

#if defined(CARLA_OS_WIN) || defined(CARLA_OS_MAC)
    const ScopedJuceInitialiser_GUI kJuceInit;
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeToolkitPlugin)
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeUI* const ui)
{
    return new CarlaBridgeToolkitPlugin(ui);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#define CARLA_PLUGIN_UI_WITHOUT_JUCE_PROCESSORS
#include "CarlaPluginUI.cpp"

// -------------------------------------------------------------------------
