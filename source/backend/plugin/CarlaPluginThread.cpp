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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
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
    : CarlaThread("CarlaPluginThread"),
      fEngine(engine),
      fPlugin(plugin),
      fMode(mode),
      fProcess(nullptr)
{
    carla_debug("CarlaPluginThread::CarlaPluginThread(%p, %p, %s)", engine, plugin, PluginThreadMode2str(mode));
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

void CarlaPluginThread::setMode(const CarlaPluginThread::Mode mode)
{
    CARLA_ASSERT(! isRunning());
    carla_debug("CarlaPluginThread::setMode(%s)", PluginThreadMode2str(mode));

    fMode = mode;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const extra1, const char* const extra2)
{
    CARLA_ASSERT(! isRunning());
    carla_debug("CarlaPluginThread::setOscData(\"%s\", \"%s\", \"%s\", \"%s\")", binary, label, extra1, extra2);

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
            fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), -1, 0, 0.0f, nullptr);
            fProcess->terminate();
            return;

        case PLUGIN_THREAD_BRIDGE:
            break;
        }
    }

    QString name(fPlugin->getName());
    QStringList arguments;

    if (name.isEmpty())
        name = "(none)";

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

    case PLUGIN_THREAD_DSSI_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathUDP()).arg(fPlugin->getId());
        /* filename */ arguments << fPlugin->getFilename();
        /* label    */ arguments << (const char*)fLabel;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(fPlugin->getName());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathTCP()).arg(fPlugin->getId());
        /* URI      */ arguments << (const char*)fLabel;
        /* ui-URI   */ arguments << (const char*)fExtra1;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(fPlugin->getName());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathTCP()).arg(fPlugin->getId());
        /* filename */ arguments << fPlugin->getFilename();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(fPlugin->getName());
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathTCP()).arg(fPlugin->getId());
        /* stype    */ arguments << (const char*)fExtra1;
        /* filename */ arguments << fPlugin->getFilename();
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
        if (fPlugin->waitForOscGuiShow())
        {
            //fProcess->waitForFinished(-1);

            while (fProcess->state() != QProcess::NotRunning && ! shouldExit())
                carla_sleep(1);

            // we only get here if UI was closed or thread asked to exit

            if (fProcess->state() != QProcess::NotRunning && shouldExit())
            {
                fProcess->waitForFinished(fEngine->getOptions().uiBridgesTimeout);

                if (fProcess->state() == QProcess::Running)
                {
                    carla_stdout("CarlaPluginThread::run() - UI refused to close, force kill now");
                    fProcess->kill();
                }
                else
                {
                    carla_stdout("CarlaPluginThread::run() - UI auto-closed successfully");
                }

                fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
            }
            else if (fProcess->exitCode() != 0 || fProcess->exitStatus() == QProcess::CrashExit)
            {
                carla_stderr("CarlaPluginThread::run() - UI crashed while running");
                fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), -1, 0, 0.0f, nullptr);
            }
            else
            {
                carla_stdout("CarlaPluginThread::run() - UI closed cleanly");
                fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
            }
        }
        else
        {
            fProcess->close();
            CARLA_ASSERT(fProcess->state() == QProcess::NotRunning);

            if (fProcess->exitCode() != 0 || fProcess->exitStatus() == QProcess::CrashExit)
            {
                carla_stderr("CarlaPluginThread::run() - GUI crashed while opening");
                fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), -1, 0, 0.0f, nullptr);
            }
            else
            {
                carla_debug("CarlaPluginThread::run() - GUI timeout");
                fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
            }
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        //fProcess->waitForFinished(-1);

        while (fProcess->state() != QProcess::NotRunning && ! shouldExit())
            carla_sleep(1);

        // we only get here if bridge crashed or thread asked to exit

        if (shouldExit())
        {
            fProcess->waitForFinished(500);

            if (fProcess->state() == QProcess::Running)
                fProcess->close();
        }
        else
        {
            // forced quit, may have crashed
            if (fProcess->exitCode() != 0 || fProcess->exitStatus() == QProcess::CrashExit)
            {
                carla_stderr("CarlaPluginThread::run() - bridge crashed");

                CarlaString errorString("Plugin '" + CarlaString(fPlugin->getName()) + "' has crashed!\n"
                                        "Saving now will lose its current settings.\n"
                                        "Please remove this plugin, and not rely on it from this point.");
                fEngine->callback(CarlaBackend::ENGINE_CALLBACK_ERROR, fPlugin->getId(), 0, 0, 0.0f, (const char*)errorString);
            }
        }
        break;
    }
}

CARLA_BACKEND_END_NAMESPACE
