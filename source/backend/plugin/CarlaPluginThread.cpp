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

#include "CarlaPluginInternal.hpp"

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

    qWarning("CarlaPluginThread::PluginThreadMode2str(%i) - invalid mode", mode);
    return nullptr;
}

CarlaPluginThread::CarlaPluginThread(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin, const PluginThreadMode mode, QObject* const parent)
    : QThread(parent),
      kEngine(engine),
      kPlugin(plugin),
      kMode(mode)
{
    qDebug("CarlaPluginThread::CarlaPluginThread(plugin:\"%s\", engine:\"%s\", %s)", plugin->name(), engine->getName(), PluginThreadMode2str(mode));

    fProcess = nullptr;
}

CarlaPluginThread::~CarlaPluginThread()
{
    if (fProcess != nullptr)
        delete fProcess;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const data1)
{
    fBinary = binary;
    fLabel  = label;
    fData1  = data1;
}

void CarlaPluginThread::run()
{
    qDebug("CarlaPluginThread::run()");

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
        /* ui-URI   */ arguments << (const char*)fData1;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* filename */ arguments << kPlugin->filename();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(kPlugin->name());
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* osc_url  */ arguments << QString("%1/%2").arg(kEngine->getOscServerPathTCP()).arg(kPlugin->id());
        /* stype    */ arguments << (const char*)fData1;
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
                kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), 0, 0, 0.0, nullptr);
                qWarning("CarlaPluginThread::run() - GUI closed");
            }
            else
            {
                // Kill
                kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), -1, 0, 0.0, nullptr);
                qWarning("CarlaPluginThread::run() - GUI crashed");
                break;
            }
        }
        else
        {
            qDebug("CarlaPluginThread::run() - GUI timeout");
            kEngine->callback(CarlaBackend::CALLBACK_SHOW_GUI, kPlugin->id(), 0, 0, 0.0, nullptr);
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        fProcess->waitForFinished(-1);

        if (fProcess->exitCode() != 0)
        {
            qWarning("CarlaPluginThread::run() - bridge crashed");

            QString errorString = QString("Plugin '%1' has crashed!\n"
                                          "Saving now will lose its current settings.\n"
                                          "Please remove this plugin, and not rely on it from this point.").arg(kPlugin->name());
            kEngine->setLastError(errorString.toUtf8().constData());
            kEngine->callback(CarlaBackend::CALLBACK_ERROR, kPlugin->id(), 0, 0, 0.0, nullptr);
        }

        break;
    }
}

CARLA_BACKEND_END_NAMESPACE
