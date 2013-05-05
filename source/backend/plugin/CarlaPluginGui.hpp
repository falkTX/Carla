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

#ifndef __CARLA_PLUGIN_GUI_HPP__
#define __CARLA_PLUGIN_GUI_HPP__

#include "CarlaPluginInternal.hpp"

#include <QtGui/QCloseEvent>

# if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QMainWindow>
#else
# include <QtGui/QMainWindow>
#endif

CARLA_BACKEND_START_NAMESPACE

class CarlaPluginGui : public QMainWindow
{
    Q_OBJECT

public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void guiClosedCallback() = 0;
    };

    struct Options {
        bool parented;
        bool resizable;
    };

    CarlaPluginGui(CarlaEngine* const engine, Callback* const callback, const Options& options, const QByteArray& lastGeometry);
    ~CarlaPluginGui();

    void setSize(const int width, const int height);

    // Parent UIs
    void* getContainerWinId();

    // Qt UIs
    void setWidget(QWidget* widget);

protected:
    void closeEvent(QCloseEvent* const event);

private:
    Callback* const kCallback;
    QWidget* fContainer;
    const Options fOptions;

signals:
    void setSizeSafeSignal(int, int);

private slots:
    void setSizeSafeSlot(int width, int height);

#ifndef MOC_PARSING
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginGui)
#endif
};

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_PLUGIN_GUI_HPP__
