// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaBridgeFormat.hpp"
#include "CarlaBridgeToolkit.hpp"

#include "CarlaMainLoop.hpp"
#include "CarlaPluginUI.hpp"
#include "CarlaUtils.h"

#if defined(CARLA_OS_MAC) && defined(BRIDGE_COCOA)
# include "CarlaMacUtils.hpp"
#endif

#include "extra/Sleep.hpp"

#if defined(HAVE_X11) && defined(BRIDGE_X11)
# include <X11/Xlib.h>
#endif

CARLA_BRIDGE_UI_START_NAMESPACE

using CARLA_BACKEND_NAMESPACE::runMainLoopOnce;

// -------------------------------------------------------------------------

class CarlaBridgeToolkitNative : public CarlaBridgeToolkit,
                                 private CarlaPluginUI::Callback
{
public:
    CarlaBridgeToolkitNative(CarlaBridgeFormat* const format)
        : CarlaBridgeToolkit(format),
          fHostUI(nullptr),
          fIdling(false)
    {
        carla_debug("CarlaBridgeToolkitNative::CarlaBridgeToolkitNative(%p)", format);
    }

    ~CarlaBridgeToolkitNative() override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI == nullptr,);
        carla_debug("CarlaBridgeToolkitNative::~CarlaBridgeToolkitNative()");
    }

    bool init(const int /*argc*/, const char** /*argv[]*/) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI == nullptr, false);
        carla_debug("CarlaBridgeToolkitNative::init()");

        const CarlaBridgeFormat::Options& options(fPlugin->getOptions());

#if defined(CARLA_OS_MAC) && defined(BRIDGE_COCOA)
        CARLA_BACKEND_NAMESPACE::initStandaloneApplication();
        fHostUI = CarlaPluginUI::newCocoa(this, 0, options.isStandalone, options.isResizable);
#elif defined(CARLA_OS_WIN) && defined(BRIDGE_HWND)
        fHostUI = CarlaPluginUI::newWindows(this, 0, options.isStandalone, options.isResizable);
#elif defined(HAVE_X11) && defined(BRIDGE_X11)
        XInitThreads();
        fHostUI = CarlaPluginUI::newX11(this, 0, options.isStandalone, options.isResizable, true);
#endif
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr, false);

#ifndef CARLA_OS_MAC
        if (options.isStandalone)
            fPlugin->setScaleFactor(carla_get_desktop_scale_factor());
#endif

        fHostUI->setTitle(options.windowTitle.buffer());

#ifndef CARLA_OS_MAC
        if (options.transientWindowId != 0)
        {
            fHostUI->setTransientWinId(options.transientWindowId);
        }
        else if (const char* const winIdStr = std::getenv("ENGINE_OPTION_FRONTEND_WIN_ID"))
        {
            if (const long long winId = std::strtoll(winIdStr, nullptr, 16))
                fHostUI->setTransientWinId(static_cast<uintptr_t>(winId));
        }
#endif

        return true;
    }

    void exec(const bool showUI) override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr,);
        carla_debug("CarlaBridgeToolkitNative::exec(%s)", bool2str(showUI));

        if (showUI)
        {
            fHostUI->show();
            fHostUI->focus();
        }

        fIdling = true;

        for (; runMainLoopOnce() && fIdling;)
        {
            if (fPlugin->isPipeRunning())
                fPlugin->idlePipe();

            fPlugin->idleUI();
            fHostUI->idle();
#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
            // MacOS and Win32 have event-loops to run, so minimize sleep time
            d_msleep(1);
#else
            d_msleep(33);
#endif
        }
    }

    void quit() override
    {
        carla_debug("CarlaBridgeToolkitNative::quit()");

        fIdling = false;

        if (fHostUI != nullptr)
        {
            fHostUI->hide();
            delete fHostUI;
            fHostUI = nullptr;
        }
    }

    void show() override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr,);
        carla_debug("CarlaBridgeToolkitNative::show()");

        fHostUI->show();
    }

    void focus() override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr,);
        carla_debug("CarlaBridgeToolkitNative::focus()");

        fHostUI->focus();
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr,);
        carla_debug("CarlaBridgeToolkitNative::hide()");

        fHostUI->hide();
    }

    void setChildWindow(void* const ptr) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);
        carla_debug("CarlaBridgeToolkitNative::setChildWindow(%p)", ptr);

        fHostUI->setChildWindow(ptr);
    }

    void setSize(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(width > 0,);
        CARLA_SAFE_ASSERT_RETURN(height > 0,);
        carla_debug("CarlaBridgeToolkitNative::resize(%i, %i)", width, height);

        fHostUI->setSize(width, height, false, false);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr,);
        carla_debug("CarlaBridgeToolkitNative::setTitle(\"%s\")", title);

        fHostUI->setTitle(title);
    }

    void* getContainerId() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr, nullptr);

        return fHostUI->getPtr();
    }

#ifdef HAVE_X11
    void* getContainerId2() const override
    {
        CARLA_SAFE_ASSERT_RETURN(fHostUI != nullptr, nullptr);

        return fHostUI->getDisplay();
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
        fPlugin->uiResized(width, height);
    }

    // ---------------------------------------------------------------------

private:
    CarlaPluginUI* fHostUI;
    bool fIdling;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeToolkitNative)
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeFormat* const format)
{
    return new CarlaBridgeToolkitNative(format);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_UI_END_NAMESPACE

#define CARLA_PLUGIN_UI_CLASS_PREFIX ToolkitNative
#include "CarlaPluginUI.cpp"
#include "utils/Windows.cpp"

// -------------------------------------------------------------------------
