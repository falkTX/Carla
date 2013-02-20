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

#include "CarlaPluginThread.hpp"

#include "CarlaPlugin.hpp"
#include "CarlaEngine.hpp"

#include <QtCore/QProcess>

CARLA_BACKEND_START_NAMESPACE

const char* PluginThreadMode2str(const CarlaPluginThread::PluginThreadMode mode)
{
    switch (mode)
    {
    case CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI:
        return "PLUGIN_THREAD_DSSI_GUI";
    case CarlaPluginThread::PLUGIN_THREAD_LV2_GUI:
        return "PLUGIN_THREAD_LV2_GUI";
    case CarlaPluginThread::PLUGIN_THREAD_VST_GUI:
        return "PLUGIN_THREAD_VST_GUI";
    case CarlaPluginThread::PLUGIN_THREAD_BRIDGE:
        return "PLUGIN_THREAD_BRIDGE";
    }

    carla_stderr("CarlaPluginThread::PluginThreadMode2str(%i) - invalid mode", mode);
    return nullptr;
}

CarlaPluginThread::CarlaPluginThread(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin, const PluginThreadMode mode)
    : kEngine(engine),
      kPlugin(plugin),
      kMode(mode),
      fProcess(nullptr)
{
    carla_debug("CarlaPluginThread::CarlaPluginThread(plugin:\"%s\", engine:\"%s\", %s)", plugin->name(), engine->getName(), PluginThreadMode2str(mode));
}

CarlaPluginThread::~CarlaPluginThread()
{
    if (fProcess != nullptr)
       delete fProcess;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const extra)
{
    fBinary = binary;
    fLabel  = label;
    fExtra  = extra;
}

void CarlaPluginThread::run()
{
    carla_debug("CarlaPluginThread::run()");

    if (fProcess == nullptr)
    {
       fProcess = new QProcess(nullptr);
       fProcess->setProcessChannelMode(QProcess::ForwardedChannels);
#ifndef BUILD_BRIDGE
       fProcess->setProcessEnvironment(kEngine->getOptionsAsProcessEnvironment());
#endif
    }

    QString name(kPlugin->name() ? kPlugin->name() : "(none)");
    QStringList arguments;

    switch (kMode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathUDP()).arg(kPlugin->id());
        /* filename */ arguments << kPlugin->filename();
        /* label    */ arguments << (const char*)fLabel;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* URI      */ arguments << (const char*)fLabel;
        /* ui-URI   */ arguments << (const char*)fExtra;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* filename */ arguments << kPlugin->filename();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* osc_url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* stype    */ arguments << (const char*)fExtra;
        /* filename */ arguments << kPlugin->filename();
        /* name     */ arguments << name;
        /* label    */ arguments << (const char*)fLabel;
        break;
    }

    fProcess->start((const char*)fBinary, arguments);
    fProcess->waitForStarted();

    switch (kMode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
    case PLUGIN_THREAD_LV2_GUI:
    case PLUGIN_THREAD_VST_GUI:
        if (kPlugin->waitForOscGuiShow())
        {
            fProcess->waitForFinished(-1);

            if (fProcess->exitCode() == 0)
            {
                // Hide
                kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), 0, 0, 0.0f, nullptr);
                carla_stdout("CarlaPluginThread::run() - GUI closed");
            }
            else
            {
                // Kill
                kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), -1, 0, 0.0f, nullptr);
                carla_stderr("CarlaPluginThread::run() - GUI crashed");
            }
        }
        else
        {
            carla_debug("CarlaPluginThread::run() - GUI timeout");
            kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), 0, 0, 0.0f, nullptr);
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        fProcess->waitForFinished(-1);

        if (fProcess->exitCode() != 0)
        {
            carla_stderr("CarlaPluginThread::run() - bridge crashed");

            CarlaString errorString("Plugin '" + CarlaString(kPlugin->name()) + "' has crashed!\n"
                                    "Saving now will lose its current settings.\n"
                                    "Please remove this plugin, and not rely on it from this point.");
            kEngine->callback(CarlaBackend::CALLBACK_ERROR, kPlugin->id(), 0, 0, 0.0f, (const char*)errorString);
        }
        break;
    }
}

CARLA_BACKEND_END_NAMESPACE
