/*
 * Carla Plugin
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_plugin.hpp"

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

CarlaPluginThread::CarlaPluginThread(CarlaBackend::CarlaEngine* const engine_, CarlaBackend::CarlaPlugin* const plugin_, const PluginThreadMode mode_, QObject* const parent)
    : QThread(parent),
      engine(engine_),
      plugin(plugin_),
      mode(mode_)
{
    qDebug("CarlaPluginThread::CarlaPluginThread(plugin:\"%s\", engine:\"%s\", %s)", plugin->name(), engine->getName(), PluginThreadMode2str(mode));

    m_process = nullptr;
}

CarlaPluginThread::~CarlaPluginThread()
{
    if (m_process)
        delete m_process;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const data1)
{
    m_binary = QString(binary);
    m_label  = QString(label);
    m_data1  = QString(data1);
}

void CarlaPluginThread::run()
{
    qDebug("CarlaPluginThread::run()");

    if (! m_process)
    {
        m_process = new QProcess(nullptr);
        m_process->setProcessChannelMode(QProcess::ForwardedChannels);
#ifndef BUILD_BRIDGE
        m_process->setProcessEnvironment(engine->getOptionsAsProcessEnvironment());
#endif
    }

    QString name(plugin->name() ? plugin->name() : "(none)");
    QStringList arguments;

    switch (mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPathUDP()).arg(plugin->id());
        /* filename */ arguments << plugin->filename();
        /* label    */ arguments << m_label;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPathTCP()).arg(plugin->id());
        /* URI      */ arguments << m_label;
        /* ui-URI   */ arguments << m_data1;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPathTCP()).arg(plugin->id());
        /* filename */ arguments << plugin->filename();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPathTCP()).arg(plugin->id());
        /* stype    */ arguments << m_data1;
        /* filename */ arguments << plugin->filename();
        /* name     */ arguments << name;
        /* label    */ arguments << m_label;
        break;
    }

    m_process->start(m_binary, arguments);
    m_process->waitForStarted();

    switch (mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
    case PLUGIN_THREAD_LV2_GUI:
    case PLUGIN_THREAD_VST_GUI:
        if (plugin->waitForOscGuiShow())
        {
            m_process->waitForFinished(-1);

            if (m_process->exitCode() == 0)
            {
                // Hide
                engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0, nullptr);
                qWarning("CarlaPluginThread::run() - GUI closed");
            }
            else
            {
                // Kill
                engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), -1, 0, 0.0, nullptr);
                qWarning("CarlaPluginThread::run() - GUI crashed");
                break;
            }
        }
        else
        {
            qDebug("CarlaPluginThread::run() - GUI timeout");
            engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0, nullptr);
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        m_process->waitForFinished(-1);

        if (m_process->exitCode() != 0)
        {
            qWarning("CarlaPluginThread::run() - bridge crashed");

            QString errorString = QString("Plugin '%1' has crashed!\n"
                                          "Saving now will lose its current settings.\n"
                                          "Please remove this plugin, and not rely on it from this point.").arg(plugin->name());
            engine->setLastError(errorString.toUtf8().constData());
            engine->callback(CarlaBackend::CALLBACK_ERROR, plugin->id(), 0, 0, 0.0, nullptr);
        }

        break;
    }
}

CARLA_BACKEND_END_NAMESPACE
