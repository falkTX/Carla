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
      fBinary(),
      fLabel(),
      fExtra1(),
      fExtra2(),
      fProcess(nullptr),
      leakDetector_CarlaPluginThread()
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

    return (uintptr_t)fProcess->getPID();
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
        case PLUGIN_THREAD_BRIDGE:
            break;

        case PLUGIN_THREAD_DSSI_GUI:
        case PLUGIN_THREAD_LV2_GUI:
        case PLUGIN_THREAD_VST_GUI:
            fEngine->callback(CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED, fPlugin->getId(), 0, 0, 0.0f, nullptr);
            fProcess->kill();
            fProcess = nullptr;
            return;
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

    // use a global mutex to ensure bridge environment is correct
    static CarlaMutex sEnvMutex;

    char strBuf[STR_MAX+1];
    strBuf[STR_MAX] = '\0';

    const EngineOptions& options(fEngine->getOptions());

    sEnvMutex.lock();

    carla_setenv("CARLA_CLIENT_NAME", name.toRawUTF8());

    std::snprintf(strBuf, STR_MAX, "%f", fEngine->getSampleRate());
    carla_setenv("CARLA_SAMPLE_RATE", strBuf);

    carla_setenv("ENGINE_OPTION_FORCE_STEREO",          bool2str(options.forceStereo));
    carla_setenv("ENGINE_OPTION_PREFER_PLUGIN_BRIDGES", bool2str(options.preferPluginBridges));
    carla_setenv("ENGINE_OPTION_PREFER_UI_BRIDGES",     bool2str(options.preferUiBridges));
    carla_setenv("ENGINE_OPTION_UIS_ALWAYS_ON_TOP",     bool2str(options.uisAlwaysOnTop));

    std::snprintf(strBuf, STR_MAX, "%u", options.maxParameters);
    carla_setenv("ENGINE_OPTION_MAX_PARAMETERS", strBuf);

    std::snprintf(strBuf, STR_MAX, "%u", options.uiBridgesTimeout);
    carla_setenv("ENGINE_OPTION_UI_BRIDGES_TIMEOUT",strBuf);

    if (options.pathLADSPA != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LADSPA", options.pathLADSPA);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LADSPA", "");

    if (options.pathDSSI != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_DSSI", options.pathDSSI);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_DSSI", "");

    if (options.pathLV2 != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LV2", options.pathLV2);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_LV2", "");

    if (options.pathVST != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_VST", options.pathVST);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_VST", "");

    if (options.pathVST3 != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_VST3", options.pathVST3);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_VST3", "");

    if (options.pathAU != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_AU", options.pathAU);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_AU", "");

    if (options.pathGIG != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_GIG", options.pathGIG);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_GIG", "");

    if (options.pathSF2 != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SF2", options.pathSF2);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SF2", "");

    if (options.pathSFZ != nullptr)
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SFZ", options.pathSFZ);
    else
        carla_setenv("ENGINE_OPTION_PLUGIN_PATH_SFZ", "");

    if (options.binaryDir != nullptr)
        carla_setenv("ENGINE_OPTION_PATH_BINARIES", options.binaryDir);
    else
        carla_setenv("ENGINE_OPTION_PATH_BINARIES", "");

    if (options.resourceDir != nullptr)
        carla_setenv("ENGINE_OPTION_PATH_RESOURCES", options.resourceDir);
    else
        carla_setenv("ENGINE_OPTION_PATH_RESOURCES", "");

    carla_setenv("ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR", bool2str(options.preventBadBehaviour));

    std::snprintf(strBuf, STR_MAX, P_UINTPTR, options.frontendWinId);
    carla_setenv("ENGINE_OPTION_FRONTEND_WIN_ID", strBuf);

#ifdef CARLA_OS_LINUX
    const char* const oldPreload(std::getenv("LD_PRELOAD"));
    ::unsetenv("LD_PRELOAD");
#endif

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
        /* osc-url   */ arguments.add(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId()));
        /* URI       */ arguments.add(fLabel.buffer());
        /* UI URI    */ arguments.add(fExtra1.buffer());
        /* UI Bundle */ arguments.add(fExtra2.buffer());
        /* UI Title  */ arguments.add(name + String(" (GUI)"));
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId()));
        /* filename */ arguments.add(fPlugin->getFilename());
        /* ui-title */ arguments.add(name + String(" (GUI)"));
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* stype    */ arguments.add(fExtra1.buffer());
        /* filename */ arguments.add(fPlugin->getFilename());
        /* label    */ arguments.add(fLabel.buffer());
        /* uniqueId */ arguments.add(String(static_cast<juce::int64>(fPlugin->getUniqueId())));

        carla_setenv("ENGINE_BRIDGE_OSC_URL", String(String(fEngine->getOscServerPathUDP()) + String("/") + String(fPlugin->getId())).toRawUTF8());
        carla_setenv("ENGINE_BRIDGE_SHM_IDS", fExtra2.buffer());
        carla_setenv("WINEDEBUG", "-all");
        break;
    }

    carla_stdout("starting app..");

    fProcess->start(arguments);

#ifdef CARLA_OS_LINUX
    if (oldPreload != nullptr)
        ::setenv("LD_PRELOAD", oldPreload, 1);
#endif

    sEnvMutex.unlock();

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
                fProcess->waitForProcessToFinish(static_cast<int>(fEngine->getOptions().uiBridgesTimeout));

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
        if (fProcess->isRunning() && shouldThreadExit())
        {
            fProcess->waitForProcessToFinish(2000);

            if (fProcess->isRunning())
            {
                carla_stdout("CarlaPluginThread::run() - bridge refused to close, force kill now");
                fProcess->kill();
            }
            else
            {
                carla_stdout("CarlaPluginThread::run() - bridge auto-closed successfully");
            }
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

CARLA_BACKEND_END_NAMESPACE
