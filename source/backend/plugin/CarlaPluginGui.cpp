/*
 * Carla Plugin
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

#include "CarlaPluginInternal.hpp"

#include <QtGui/QMainWindow>
#include <QtGui/QCloseEvent>

#ifdef Q_WS_X11
# include <QtGui/QX11EmbedContainer>
#endif

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Engine Helpers, defined in CarlaEngine.cpp

extern QMainWindow* getEngineHostWindow(CarlaEngine* const engine);

class CarlaPluginGUI : public QMainWindow
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void guiClosedCallback() = 0;
    };

    CarlaPluginGUI(CarlaEngine* const engine, Callback* const callback);
    ~CarlaPluginGUI();

    void idle();
    void resizeLater(int width, int height);

    // Parent UIs
    void* getContainerWinId();
    void  closeContainer();

    // Qt4 UIs, TODO

protected:
    void closeEvent(QCloseEvent* const event);

private:
    Callback* const kCallback;
    QWidget* fContainer;

    int fNextWidth;
    int fNextHeight;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginGUI)
};

// -------------------------------------------------------------------
// CarlaPluginGUI

CarlaPluginGUI::CarlaPluginGUI(CarlaEngine* const engine, Callback* const callback)
    : QMainWindow(getEngineHostWindow(engine)),
      kCallback(callback),
      fContainer(nullptr),
      fNextWidth(0),
      fNextHeight(0)
{
    CARLA_ASSERT(callback != nullptr);
    carla_debug("CarlaPluginGUI::CarlaPluginGUI(%p, %p)", engine, callback);
}

CarlaPluginGUI::~CarlaPluginGUI()
{
    carla_debug("CarlaPluginGUI::~CarlaPluginGUI()");

    closeContainer();
}

void CarlaPluginGUI::idle()
{
    if (fNextWidth > 0 && fNextHeight > 0)
    {
        setFixedSize(fNextWidth, fNextHeight);
        fNextWidth  = 0;
        fNextHeight = 0;
    }
}

void CarlaPluginGUI::resizeLater(int width, int height)
{
    CARLA_ASSERT_INT(width > 0, width);
    CARLA_ASSERT_INT(height > 0, height);

    if (width <= 0)
        return;
    if (height <= 0)
        return;

    fNextWidth  = width;
    fNextHeight = height;
}

void* CarlaPluginGUI::getContainerWinId()
{
    carla_debug("CarlaPluginGUI::getContainerWinId()");

    if (fContainer == nullptr)
    {
#ifdef Q_WS_X11
        QX11EmbedContainer* container(new QX11EmbedContainer(this));
#else
        QWidget* container(new QWidget(this));
#endif
        setCentralWidget(container);
        fContainer = container;
    }

    return (void*)fContainer->winId();
}

void  CarlaPluginGUI::closeContainer()
{
    carla_debug("CarlaPluginGUI::closeContainer()");

    if (fContainer != nullptr)
    {
#ifdef Q_WS_X11
        delete (QX11EmbedContainer*)fContainer;
#else
        delete (QWidget*)fContainer;
#endif
        fContainer = nullptr;
    }
}

void CarlaPluginGUI::closeEvent(QCloseEvent* const event)
{
    carla_debug("CarlaPluginGUI::closeEvent(%p)", event);
    CARLA_ASSERT(event != nullptr);

    if (event == nullptr)
        return;

    if (! event->spontaneous())
    {
        event->ignore();
        return;
    }

    if (kCallback != nullptr)
        kCallback->guiClosedCallback();

    QMainWindow::closeEvent(event);
}

// -------------------------------------------------------------------
// CarlaPluginGUI

#if 0
void createUiIfNeeded(CarlaPluginGUI::Callback* const callback)
{
    if (gui != nullptr)
        return;

    gui = new CarlaPluginGUI(engine, callback);
}

void destroyUiIfNeeded()
{
    if (gui == nullptr)
        return;

    gui->close();
    delete gui;
    gui = nullptr;
}

void resizeUiLater(int width, int height)
{
    if (gui == nullptr)
        return;

    gui->resizeLater(width, height);
}
#endif

CARLA_BACKEND_END_NAMESPACE
