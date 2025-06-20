// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dgl/OpenGL.hpp"
#include "dgl/src/pugl.hpp"
#include "dgl/src/WindowPrivateData.hpp"

#include "CarlaNative.h"
#include "ui_launcher_res.hpp"
#include "CarlaDefines.h"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

class PluginApplication : public DGL_NAMESPACE::Application
{
public:
    explicit PluginApplication()
        : Application(false)
    {
        setClassName("CarlaPluginWrapper");
    }
};

class PluginWindow : public DGL_NAMESPACE::Window
{
public:
    explicit PluginWindow(PluginApplication& app, const uintptr_t winId)
        : Window(app,
                 winId,
                 ui_launcher_res::carla_uiWidth,
                 ui_launcher_res::carla_uiHeight,
                 0.0,
                 false,
                 false,
                 false,
                 false)
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

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DGL

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
        const uint width = ui_launcher_res::carla_uiWidth;
        const uint height = ui_launcher_res::carla_uiHeight;

        Widget::setSize(width, height);
        setGeometryConstraints(width, height, true, true, true);

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

void getUILauncherSize(CarlaUILauncher* const ui, VstRect* const rect)
{
    rect->right = ui->window.getWidth();
    rect->bottom = ui->window.getHeight();
   #ifdef DISTRHO_OS_MAC
    const double scaleFactor = ui->window.getScaleFactor();
    rect->right /= scaleFactor;
    rect->bottom /= scaleFactor;
   #endif
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
