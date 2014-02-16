/*
 * Carla Plugin
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPlugin.hpp"
#include "CarlaPluginThread.hpp"
#include "CarlaEngine.hpp"

#include <QtCore/QDebug>
#include <QtCore/QProcess>

CARLA_BACKEND_START_NAMESPACE

static inline
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

uintptr_t CarlaPluginThread::getPid() const
{
    CARLA_SAFE_ASSERT_RETURN(fProcess != nullptr, 0);

    return (uintptr_t)fProcess->pid();
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

    if (name.isEmpty())
        name = "(none)";

    QStringList arguments;
    QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
    const EngineOptions& options(fEngine->getOptions());

    char strBuf[STR_MAX+1];
    env.insert("ENGINE_OPTION_UIS_ALWAYS_ON_TOP", options.uisAlwaysOnTop ? "true" : "false");

    if (options.maxParameters != 0)
    {
        std::sprintf(strBuf, "%u", options.maxParameters);
        env.insert("ENGINE_OPTION_MAX_PARAMETERS", strBuf);
    }

    if (options.uiBridgesTimeout != 0)
    {
        std::sprintf(strBuf, "%u", options.uiBridgesTimeout);
        env.insert("ENGINE_OPTION_UI_BRIDGES_TIMEOUT", strBuf);
    }

    if (options.frontendWinId != 0)
    {
        std::sprintf(strBuf, P_UINTPTR, options.frontendWinId);
        env.insert("ENGINE_OPTION_FRONTEND_WIN_ID", strBuf);
    }

    if (options.binaryDir != nullptr)
        env.insert("ENGINE_OPTION_PATH_BINARIES", options.binaryDir);

    if (options.resourceDir != nullptr)
        env.insert("ENGINE_OPTION_PATH_RESOURCES", options.resourceDir);

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

    case PLUGIN_THREAD_DSSI_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathUDP()).arg(fPlugin->getId());
        /* filename */ arguments << fPlugin->getFilename();
        /* label    */ arguments << fLabel.getBuffer();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(fPlugin->getName());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathUDP()).arg(fPlugin->getId());
        /* URI      */ arguments << fLabel.getBuffer();
        /* ui-URI   */ arguments << fExtra1.getBuffer();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(fPlugin->getName());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathUDP()).arg(fPlugin->getId());
        /* filename */ arguments << fPlugin->getFilename();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(fPlugin->getName());
        break;

    case PLUGIN_THREAD_BRIDGE:
        env.insert("ENGINE_BRIDGE_SHM_IDS", fExtra2.getBuffer());
        env.insert("ENGINE_BRIDGE_CLIENT_NAME", name);
        env.insert("ENGINE_BRIDGE_OSC_URL", QString("%1/%2").arg(fEngine->getOscServerPathUDP()).arg(fPlugin->getId()));

        if (fPlugin->getType() != PLUGIN_JACK)
        {
            /* osc-url  */ arguments << QString("%1/%2").arg(fEngine->getOscServerPathUDP()).arg(fPlugin->getId());
            /* stype    */ arguments << fExtra1.getBuffer();
            /* filename */ arguments << fPlugin->getFilename();
            /* name     */ arguments << name;
            /* label    */ arguments << fLabel.getBuffer();
            /* uniqueId */ arguments << QString("%1").arg(fPlugin->getUniqueId());
        }
        else
        {
            env.insert("LD_LIBRARY_PATH", "/home/falktx/FOSS/GIT-mine/Carla/source/bridges/jackplugin/");
            carla_stdout("JACK app bridge here, filename: %s", fPlugin->getFilename());
            fBinary = fPlugin->getFilename();
        }
        break;
    }

    fProcess->setProcessEnvironment(env);

    carla_stdout("starting app..");
    qWarning() << arguments;

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
                fProcess->waitForFinished(static_cast<int>(fEngine->getOptions().uiBridgesTimeout));

                if (fProcess->state() == QProcess::Running)
                {
                    carla_stdout("CarlaPluginThread::run() - UI refused to close, force kill now");
                    fProcess->kill();
                }
                else
                {
                    carla_stdout("CarlaPluginThread::run() - UI auto-closed successfully");
                }
            }
            else if (fProcess->exitCode() != 0 || fProcess->exitStatus() == QProcess::CrashExit)
                carla_stderr("CarlaPluginThread::run() - UI crashed while running");
            else
                carla_stdout("CarlaPluginThread::run() - UI closed cleanly");

            fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
        }
        else
        {
            fProcess->close();
            CARLA_ASSERT(fProcess->state() == QProcess::NotRunning);

            if (fProcess->exitCode() != 0 || fProcess->exitStatus() == QProcess::CrashExit)
                carla_stderr("CarlaPluginThread::run() - GUI crashed while opening");
            else
                carla_stdout("CarlaPluginThread::run() - GUI timeout");

            fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
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

    carla_stdout("app finished");
}

CARLA_BACKEND_END_NAMESPACE
