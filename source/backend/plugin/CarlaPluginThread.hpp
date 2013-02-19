/*
 * Carla Plugin Thread
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef __CARLA_PLUGIN_THREAD_HPP__
#define __CARLA_PLUGIN_THREAD_HPP__

#include "CarlaBackendUtils.hpp"

#include <QtCore/QThread>

class QProcess;

CARLA_BACKEND_START_NAMESPACE

class CarlaPluginThread : public QThread
{
public:
    enum PluginThreadMode {
        PLUGIN_THREAD_DSSI_GUI,
        PLUGIN_THREAD_LV2_GUI,
        PLUGIN_THREAD_VST_GUI,
        PLUGIN_THREAD_BRIDGE
    };

    CarlaPluginThread(CarlaEngine* const engine, CarlaPlugin* const plugin, const PluginThreadMode mode, QObject* const parent = nullptr);
    ~CarlaPluginThread();

    void setOscData(const char* const binary, const char* const label, const char* const data1="");

protected:
    void run();

private:
    CarlaEngine* const kEngine;
    CarlaPlugin* const kPlugin;
    const PluginThreadMode kMode;

    CarlaString fBinary;
    CarlaString fLabel;
    CarlaString fData1;
    QProcess*   fProcess;
};

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_PLUGIN_THREAD_HPP__
