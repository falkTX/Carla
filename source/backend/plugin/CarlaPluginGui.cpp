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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPluginGui.hpp"

# include <QtCore/QSettings>

#ifdef Q_WS_X11
# include <QtGui/QX11EmbedContainer>
#endif

CARLA_BACKEND_START_NAMESPACE

#include "moc_CarlaPluginGui.cpp"

// -------------------------------------------------------------------
// CarlaPluginGUI

CarlaPluginGui::CarlaPluginGui(CarlaEngine* const engine, Callback* const callback, const Options& options, const QByteArray& lastGeometry)
    : QMainWindow(nullptr),
      kCallback(callback),
      fContainer(nullptr),
      fOptions(options)
{
    CARLA_ASSERT(callback != nullptr);
    carla_debug("CarlaPluginGui::CarlaPluginGui(%p, %p)", engine, callback);

    setWindowIcon(QIcon::fromTheme("carla", QIcon(":/scalable/carla.svg")));

    if (options.parented)
    {
#ifdef Q_WS_X11
        fContainer = new QX11EmbedContainer(this);
#else
        fContainer = new QWidget(this);
#endif
        setCentralWidget(fContainer);
    }

#ifdef Q_OS_WIN
    if (! options.resizable)
        setWindowFlags(windowFlags()|Qt::MSWindowsFixedSizeDialogHint);
#endif

    connect(this, SIGNAL(setSizeSafeSignal(int,int)), SLOT(setSizeSafeSlot(int,int)));

    {
        QSettings settings;

        if (settings.value("Engine/UIsAlwaysOnTop", true).toBool())
            setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint);

        if (! lastGeometry.isNull())
            restoreGeometry(lastGeometry);
    }
}

CarlaPluginGui::~CarlaPluginGui()
{
    carla_debug("CarlaPluginGui::~CarlaPluginGui()");

    if (fOptions.parented)
    {
        CARLA_ASSERT(fContainer != nullptr);

        if (fContainer != nullptr)
        {
#ifdef Q_WS_X11
            delete (QX11EmbedContainer*)fContainer;
#else
            delete fContainer;
#endif
            fContainer = nullptr;
        }
    }
}

void CarlaPluginGui::setSize(const int width, const int height)
{
    CARLA_ASSERT_INT(width > 0, width);
    CARLA_ASSERT_INT(height > 0, height);
    carla_debug("CarlaPluginGui::setSize(%i, %i)", width, height);

    if (width <= 0)
        return;
    if (height <= 0)
        return;

    emit setSizeSafeSignal(width, height);
}

void* CarlaPluginGui::getContainerWinId()
{
    CARLA_ASSERT(fContainer != nullptr);
    carla_debug("CarlaPluginGui::getContainerWinId()");

    return (fContainer != nullptr) ? (void*)fContainer->winId() : nullptr;
}

void CarlaPluginGui::setWidget(QWidget* const widget)
{
    CARLA_ASSERT(fContainer == nullptr);
    carla_debug("CarlaPluginGui::setWidget(%p)", widget);

    setCentralWidget(widget);
    widget->setParent(this);

    fContainer = widget;
}

void CarlaPluginGui::closeEvent(QCloseEvent* const event)
{
    CARLA_ASSERT(event != nullptr);
    carla_debug("CarlaPluginGui::closeEvent(%p)", event);

    if (event == nullptr)
        return;

    if (event->spontaneous() && kCallback != nullptr)
        kCallback->guiClosedCallback();

    QMainWindow::closeEvent(event);
}

void CarlaPluginGui::setSizeSafeSlot(int width, int height)
{
    carla_debug("CarlaPluginGui::setSizeSafeSlot(%i, %i)", width, height);

    if (fOptions.resizable)
        resize(width, height);
    else
        setFixedSize(width, height);
}

// -------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
