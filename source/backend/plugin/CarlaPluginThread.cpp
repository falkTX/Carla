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

using juce::ChildProcess;
using juce::String;
using juce::StringArray;

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
    : juce::Thread("CarlaPluginThread"),
      fEngine(engine),
      fPlugin(plugin),
      fMode(mode)
{
    carla_debug("CarlaPluginThread::CarlaPluginThread(plugin:\"%s\", engine:\"%s\", %s)", plugin->getName(), engine->getName(), PluginThreadMode2str(mode));

    setPriority(5);
}

void CarlaPluginThread::setMode(const CarlaPluginThread::Mode mode)
{
    CARLA_ASSERT(! isThreadRunning());
    carla_debug("CarlaPluginThread::setMode(%s)", PluginThreadMode2str(mode));

    fMode = mode;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const extra1, const char* const extra2)
{
    CARLA_ASSERT(! isThreadRunning());
    carla_debug("CarlaPluginThread::setOscData(\"%s\", \"%s\", \"%s\", \"%s\")", binary, label, extra1, extra2);

    fBinary = binary;
    fLabel  = label;
    fExtra1 = extra1;
    fExtra2 = extra2;
}

void CarlaPluginThread::run()
{
    carla_debug("CarlaPluginThread::run()");

    ChildProcess process;

    StringArray arguments;
    arguments.add((const char*)fBinary);

    String name(fPlugin->getName());
    if (name.isEmpty())
        name = "(none)";

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

    case PLUGIN_THREAD_DSSI_GUI:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathUDP()) + "/" + String(fPlugin->getId()));
        /* filename */ arguments.add(fPlugin->getFilename());
        /* label    */ arguments.add((const char*)fLabel);
        /* ui-title */ arguments.add(name + " (GUI)");
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathTCP()) + "/" + String(fPlugin->getId()));
        /* URI      */ arguments.add((const char*)fLabel);
        /* ui-URI   */ arguments.add((const char*)fExtra1);
        /* ui-title */ arguments.add(name + " (GUI)");
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathTCP()) + "/" + String(fPlugin->getId()));
        /* filename */ arguments.add(fPlugin->getFilename());
        /* ui-title */ arguments.add(name + " (GUI)");
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* osc-url  */ arguments.add(String(fEngine->getOscServerPathTCP()) + "/" + String(fPlugin->getId()));
        /* stype    */ arguments.add((const char*)fExtra1);
        /* filename */ arguments.add(fPlugin->getFilename());
        /* name     */ arguments.add(name);
        /* label    */ arguments.add((const char*)fLabel);
        /* SHM ids  */ arguments.add((const char*)fExtra2);
        break;
    }

    if (! process.start(arguments))
        return;

    switch (fMode)
    {
    case PLUGIN_THREAD_NULL:
        break;

    case PLUGIN_THREAD_DSSI_GUI:
    case PLUGIN_THREAD_LV2_GUI:
    case PLUGIN_THREAD_VST_GUI:
        if (fPlugin->waitForOscGuiShow())
        {
            while (process.isRunning() && ! threadShouldExit())
                sleep(1000);

            // we only get here is UI was closed or thread asked to exit

            if (threadShouldExit())
            {
                if (process.isRunning())
                    process.kill();
            }
            else
            {
                fEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, fPlugin->getId(), 0, 0, 0.0f, nullptr);
            }
        }
        else
        {
            if (process.isRunning() && ! process.waitForProcessToFinish(500))
            {
                process.kill();

                fEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, fPlugin->getId(), -1, 0, 0.0f, nullptr);
                carla_stderr("CarlaPluginThread::run() - GUI crashed while opening");
            }
            else
            {
                fEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, fPlugin->getId(), 0, 0, 0.0f, nullptr);
                carla_stderr("CarlaPluginThread::run() - GUI timeout");
            }
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        while (process.isRunning() && ! threadShouldExit())
            sleep(1000);

        if (threadShouldExit())
        {
            if (process.isRunning())
                process.kill();
        }
        break;
    }
}

CARLA_BACKEND_END_NAMESPACE
