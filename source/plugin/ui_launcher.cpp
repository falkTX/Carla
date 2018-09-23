/*
 * Carla Native Plugin UI launcher
 * Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>
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

#include "dgl/Application.hpp"
#include "dgl/ImageWidgets.hpp"
#include "CarlaNative.h"
#include "ui_launcher_res.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DGL

class CarlaButtonWidget : public Widget,
                          private ImageButton::Callback
{
public:
    CarlaButtonWidget(Window& parent, const NativePluginDescriptor* const d, const NativePluginHandle h)
      : Widget(parent),
        startButtonImage(ui_launcher_res::carla_uiData,
                         ui_launcher_res::carla_uiWidth,
                         ui_launcher_res::carla_uiHeight,
                         GL_BGR),
        startButton(this, startButtonImage),
        descriptor(d),
        handle(h)
    {
        startButton.setCallback(this);
        setSize(startButtonImage.getSize());
        parent.setSize(startButtonImage.getSize());

    }

protected:
    void onDisplay() override
    {
    }

    void imageButtonClicked(ImageButton* imageButton, int) override
    {
        if (imageButton != &startButton)
            return;

        if (descriptor->ui_show != nullptr)
            descriptor->ui_show(handle, true);
    }

private:
    Image startButtonImage;
    ImageButton startButton;
    const NativePluginDescriptor* const descriptor;
    const NativePluginHandle handle;
};

END_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

struct CarlaUILauncher {
    DGL_NAMESPACE::Application app;
    DGL_NAMESPACE::Window window;
    CarlaButtonWidget widget;

    CarlaUILauncher(const intptr_t winId, const NativePluginDescriptor* const d, const NativePluginHandle h)
      : app(),
        window(app, winId),
        widget(window, d, h) {}
};

CarlaUILauncher* createUILauncher(const intptr_t winId,
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
