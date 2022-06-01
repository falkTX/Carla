/*
 * Carla Native Plugin UI launcher
 * Copyright (C) 2018-2022 Filipe Coelho <falktx@falktx.com>
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

#include "dgl/OpenGL.hpp"
#include "dgl/src/pugl.hpp"
#include "dgl/src/WindowPrivateData.hpp"

#include "CarlaNative.h"
#include "ui_launcher_res.hpp"
#include "CarlaDefines.h"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DGL

class PluginApplication : public Application
{
public:
    explicit PluginApplication()
        : Application(false)
    {
        setClassName("CarlaPluginWrapper");
    }
};

class PluginWindow : public Window
{
public:
    explicit PluginWindow(PluginApplication& app, const uintptr_t winId)
        : Window(app, winId, ui_launcher_res::carla_uiWidth, ui_launcher_res::carla_uiHeight, 0.0, false, false, false)
    {
        // this is called just before creating UI, ensuring proper context to it
        if (pData->view != nullptr && pData->initPost())
            puglBackendEnter(pData->view);
    }

    ~PluginWindow()
    {
        if (pData->view != nullptr)
            puglBackendLeave(pData->view);
    }

    // called right before deleting UI, ensuring correct context
    void enterContextForDeletion()
    {
        if (pData->view != nullptr)
            puglBackendEnter(pData->view);
    }

    // called after creating UI, restoring proper context
    void leaveContextAfterCreation()
    {
        if (pData->view != nullptr)
            puglBackendLeave(pData->view);
    }
};

class CarlaButtonWidget : public TopLevelWidget,
                          private OpenGLImageButton::Callback
{
public:
    explicit CarlaButtonWidget(PluginWindow& parent, const NativePluginDescriptor* const d, const NativePluginHandle h)
      : TopLevelWidget(parent),
        startButtonImage(ui_launcher_res::carla_uiData,
                         ui_launcher_res::carla_uiWidth,
                         ui_launcher_res::carla_uiHeight,
                         kImageFormatBGR),
        startButton(this, startButtonImage),
        descriptor(d),
        handle(h),
        pluginWindow(parent)
    {
        startButton.setCallback(this);

        pluginWindow.leaveContextAfterCreation();
    }

    ~CarlaButtonWidget() override
    {
        pluginWindow.enterContextForDeletion();
    }

protected:
    void onDisplay() override
    {
    }

    void imageButtonClicked(OpenGLImageButton* imageButton, int) override
    {
        if (imageButton != &startButton)
            return;

        if (descriptor->ui_show != nullptr)
            descriptor->ui_show(handle, true);
    }

private:
    OpenGLImage startButtonImage;
    OpenGLImageButton startButton;
    const NativePluginDescriptor* const descriptor;
    const NativePluginHandle handle;
    PluginWindow& pluginWindow;

    CARLA_DECLARE_NON_COPYABLE(CarlaButtonWidget);
};

END_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

USE_NAMESPACE_DGL

struct CarlaUILauncher {
    PluginApplication app;
    PluginWindow window;
    CarlaButtonWidget widget;

    CarlaUILauncher(const uintptr_t winId, const NativePluginDescriptor* const d, const NativePluginHandle h)
      : app(),
        window(app, winId),
        widget(window, d, h) {}
};

CarlaUILauncher* createUILauncher(const uintptr_t winId,
                                  const NativePluginDescriptor* const d,
                                  const NativePluginHandle h)
{
    return new CarlaUILauncher(winId, d, h);
}

void idleUILauncher(CarlaUILauncher* const ui)
{
    ui->app.idle();
}

void destoryUILauncher(CarlaUILauncher* const ui)
{
    delete ui;
}

// --------------------------------------------------------------------------------------------------------------------
