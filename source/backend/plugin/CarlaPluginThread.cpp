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

const char* PluginThreadMode2str(const CarlaPluginThread::Mode mode)
{
    switch (mode)
    {
    case CarlaPluginThread::PLUGIN_THREAD_NULL:
        return "PLUGIN_THREAD_NULL";
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

CarlaPluginThread::CarlaPluginThread(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin, const Mode mode)
    : kEngine(engine),
      kPlugin(plugin),
      fMode(mode),
      fProcess(nullptr)
{
    carla_debug("CarlaPluginThread::CarlaPluginThread(plugin:\"%s\", engine:\"%s\", %s)", plugin->name(), engine->getName(), PluginThreadMode2str(mode));
}

CarlaPluginThread::~CarlaPluginThread()
{
    carla_debug("CarlaPluginThread::~CarlaPluginThread()");

    if (fProcess != nullptr)
    {
       delete fProcess;
       fProcess = nullptr;
    }
}

void CarlaPluginThread::setMode(const CarlaPluginThread::Mode mode) noexcept
{
    fMode = mode;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const extra1, const char* const extra2)
{
    fBinary = binary;
    fLabel  = label;
    fExtra1 = extra1;
    fExtra2 = extra2;
}

void CarlaPluginThread::run()
{
    carla_debug("CarlaPluginThread::run()");

    if (fProcess == nullptr)
    {
       fProcess = new QProcess(nullptr);
       fProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    }
    else if (fProcess->state() == QProcess::Running)
    {
        carla_stderr("CarlaPluginThread::run() - already running, giving up...");

        switch (fMode)
        {
        case PLUGIN_THREAD_NULL:
            break;

        case PLUGIN_THREAD_DSSI_GUI:
        case PLUGIN_THREAD_LV2_GUI:
        case PLUGIN_THREAD_VST_GUI:
            fProcess->terminate();
            kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), -1, 0, 0.0f, nullptr);
            return;

        case PLUGIN_THREAD_BRIDGE:
            break;
        }
    }

    QString name(kPlugin->name());
    QStringList arguments;

    if (name.isEmpty())
        name = "(none)";

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

    case PLUGIN_THREAD_DSSI_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathUDP()).arg(kPlugin->id());
        /* filename */ arguments << kPlugin->filename();
        /* label    */ arguments << (const char*)fLabel;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* URI      */ arguments << (const char*)fLabel;
        /* ui-URI   */ arguments << (const char*)fExtra1;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* filename */ arguments << kPlugin->filename();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* osc-url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* stype    */ arguments << (const char*)fExtra1;
        /* filename */ arguments << kPlugin->filename();
        /* name     */ arguments << name;
        /* label    */ arguments << (const char*)fLabel;
        /* SHM ids  */ arguments << (const char*)fExtra2;
        break;
    }

    fProcess->start((const char*)fBinary, arguments);
    fProcess->waitForStarted();

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

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
                carla_stderr("CarlaPluginThread::run() - GUI crashed while running");
            }
        }
        else
        {
            fProcess->close();
            CARLA_ASSERT(fProcess->state() == QProcess::NotRunning);

            if (fProcess->exitCode() != 0 || fProcess->exitStatus() == QProcess::CrashExit)
            {
                kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), -1, 0, 0.0f, nullptr);
                carla_stderr("CarlaPluginThread::run() - GUI crashed while opening");
            }
            else
            {
                kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), 0, 0, 0.0f, nullptr);
                carla_debug("CarlaPluginThread::run() - GUI timeout");
            }
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        fProcess->waitForFinished(-1);

        if (fProcess->exitCode() != 0 || fProcess->exitStatus() == QProcess::CrashExit)
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
