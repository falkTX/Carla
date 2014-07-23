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

#include "juce_core.h"

using juce::String;
using juce::StringArray;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

#ifdef DEBUG
static inline
const char* PluginThreadMode2str(const CarlaPluginThread::Mode mode) noexcept
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
#endif

// -----------------------------------------------------------------------

CarlaPluginThread::CarlaPluginThread(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin, const Mode mode) noexcept
    : CarlaThread("CarlaPluginThread"),
      fEngine(engine),
      fPlugin(plugin),
      fMode(mode),
      fProcess(nullptr)
{
    carla_debug("CarlaPluginThread::CarlaPluginThread(%p, %p, %s)", engine, plugin, PluginThreadMode2str(mode));
}

CarlaPluginThread::~CarlaPluginThread() noexcept
{
    carla_debug("CarlaPluginThread::~CarlaPluginThread()");

    if (fProcess != nullptr)
    {
        //fProcess.release();
        //try {
            //delete fProcess;
        //} CARLA_SAFE_EXCEPTION("~CarlaPluginThread(): delete ChildProcess");
        fProcess = nullptr;
    }
}

void CarlaPluginThread::setMode(const CarlaPluginThread::Mode mode) noexcept
{
    CARLA_SAFE_ASSERT(! isThreadRunning());
    carla_debug("CarlaPluginThread::setMode(%s)", PluginThreadMode2str(mode));

    fMode = mode;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const extra1, const char* const extra2) noexcept
{
    CARLA_SAFE_ASSERT(! isThreadRunning());
    carla_debug("CarlaPluginThread::setOscData(\"%s\", \"%s\", \"%s\", \"%s\")", binary, label, extra1, extra2);

    fBinary = binary;
    fLabel  = label;
    fExtra1 = extra1;
    fExtra2 = extra2;
}

uintptr_t CarlaPluginThread::getPid() const
{
    CARLA_SAFE_ASSERT_RETURN(fProcess != nullptr, 0);

    return 0;
    //return (uintptr_t)fProcess->pid();
}

void CarlaPluginThread::run()
{
    carla_debug("CarlaPluginThread::run()");

    if (fProcess == nullptr)
    {
       fProcess = new ChildProcess;
       //fProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    }
    else if (fProcess->isRunning())
    {
        carla_stderr("CarlaPluginThread::run() - already running, giving up...");

        switch (fMode)
        {
        case PLUGIN_THREAD_NULL:
            break;

        case PLUGIN_THREAD_DSSI_GUI:
        case PLUGIN_THREAD_LV2_GUI:
        case PLUGIN_THREAD_VST_GUI:
            fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
            fProcess->kill();
            fProcess = nullptr;
            return;

        case PLUGIN_THREAD_BRIDGE:
            break;
        }
    }

    String name(fPlugin->getName());

    if (name.isEmpty())
        name = "(none)";

    if (fLabel.isEmpty())
        fLabel = "\"\"";

    StringArray arguments;

#ifndef CARLA_OS_WIN
    if (fBinary.endsWith(".exe"))
        arguments.add("wine");
#endif

    arguments.add(fBinary.buffer());

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

    case PLUGIN_THREAD_DSSI_GUI:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId()));
        /* filename */ arguments.add(fPlugin->getFilename());
        /* label    */ arguments.add(fLabel.buffer());
        /* ui-title */ arguments.add(name + String(" (GUI)"));
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId()));
        /* URI      */ arguments.add(fLabel.buffer());
        /* ui-URI   */ arguments.add(fExtra1.buffer());
        /* ui-title */ arguments.add(name + String(" (GUI)"));
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId()));
        /* filename */ arguments.add(fPlugin->getFilename());
        /* ui-title */ arguments.add(name + String(" (GUI)"));
        break;

    case PLUGIN_THREAD_BRIDGE:
        // FIXME
        carla_setenv("ENGINE_BRIDGE_SHM_IDS", fExtra2.buffer());
        carla_setenv("ENGINE_BRIDGE_CLIENT_NAME", name.toRawUTF8());
        carla_setenv("ENGINE_BRIDGE_OSC_URL", String(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId())).toRawUTF8());
        carla_setenv("WINEDEBUG", "-all");

        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId()));
        /* stype    */ arguments.add(fExtra1.buffer());
        /* filename */ arguments.add(fPlugin->getFilename());
        /* name     */ arguments.add(name);
        /* label    */ arguments.add(fLabel.buffer());
        /* uniqueId */ arguments.add(String(static_cast<juce::int64>(fPlugin->getUniqueId())));
        break;
    }

    carla_stdout("starting app..");
    //qWarning() << arguments;

    fProcess->start(arguments);
    //fProcess->waitForStarted();

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

    case PLUGIN_THREAD_DSSI_GUI:
    case PLUGIN_THREAD_LV2_GUI:
    case PLUGIN_THREAD_VST_GUI:
        if (fPlugin->waitForOscGuiShow())
        {
            while (fProcess->isRunning() && ! shouldThreadExit())
                carla_sleep(1);

            // we only get here if UI was closed or thread asked to exit
            if (fProcess->isRunning() && shouldThreadExit())
            {
                //fProcess->waitForFinished(static_cast<int>(fEngine->getOptions().uiBridgesTimeout));

                if (fProcess->isRunning())
                {
                    carla_stdout("CarlaPluginThread::run() - UI refused to close, force kill now");
                    fProcess->kill();
                }
                else
                {
                    carla_stdout("CarlaPluginThread::run() - UI auto-closed successfully");
                }
            }
            else if (fProcess->getExitCode() != 0 /*|| fProcess->exitStatus() == QProcess::CrashExit*/)
                carla_stderr("CarlaPluginThread::run() - UI crashed while running");
            else
                carla_stdout("CarlaPluginThread::run() - UI closed cleanly");

            fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
        }
        else
        {
            fProcess->kill();

            carla_stdout("CarlaPluginThread::run() - GUI timeout");
            fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        //fProcess->waitForFinished(-1);

        while (fProcess->isRunning() && ! shouldThreadExit())
            carla_sleep(1);

        // we only get here if bridge crashed or thread asked to exit
        if (shouldThreadExit())
        {
            fProcess->getExitCode(); // TEST

            if (fProcess->isRunning())
                fProcess->kill();
        }
        else
        {
            // forced quit, may have crashed
            if (fProcess->getExitCode() != 0 /*|| fProcess->exitStatus() == QProcess::CrashExit*/)
            {
                carla_stderr("CarlaPluginThread::run() - bridge crashed");

                CarlaString errorString("Plugin '" + CarlaString(fPlugin->getName()) + "' has crashed!\n"
                                        "Saving now will lose its current settings.\n"
                                        "Please remove this plugin, and not rely on it from this point.");
                fEngine->callback(CarlaBackend::ENGINE_CALLBACK_ERROR, fPlugin->getId(), 0, 0, 0.0f, errorString);
            }
        }
        break;
    }

    carla_stdout("app finished");
    fProcess = nullptr;
}

// -----------------------------------------------------------------------

#if 0
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

    fProcess->setProcessEnvironment(env);
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
