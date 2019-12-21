/*
 * Carla plugin database code
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

#include "carla_database.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Separate Thread for Plugin Search

struct SearchPluginsThread::PrivateData {
    PrivateData(void*)
    {
    }
};

SearchPluginsThread::SearchPluginsThread(QObject* parent, QString pathBinaries)
    : QThread(parent),
      self(new PrivateData(this))
{
}

SearchPluginsThread::~SearchPluginsThread()
{
    delete self;
}

// --------------------------------------------------------------------------------------------------------------------
// Plugin Refresh Dialog

struct PluginRefreshW::PrivateData {
    PrivateData(void*)
    {
    }
};

PluginRefreshW::PluginRefreshW(QWidget* parent)
    : QDialog(parent),
      self(new PrivateData(this))
{
    delete self;
}

PluginRefreshW::~PluginRefreshW()
{
}

void PluginRefreshW::getValues(QString& audioDevice, uint& bufferSize, double& sampleRate)
{
}

void PluginRefreshW::closeEvent(QCloseEvent* event)
{
}

void PluginRefreshW::slot_saveSettings()
{
}

void PluginRefreshW::slot_start()
{
}

void PluginRefreshW::slot_skip()
{
}

void PluginRefreshW::slot_checkTools()
{
}

void PluginRefreshW::slot_handlePluginLook(float percent, QString plugin)
{
}

void PluginRefreshW::slot_handlePluginThreadFinished()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Plugin Database Dialog

struct PluginDatabaseW::PrivateData {
    PrivateData(void*)
    {
    }
};

PluginDatabaseW::PluginDatabaseW(QWidget* parent, const CarlaHost& host, bool hasCanvas, bool hasCanvasGL)
    : QDialog(parent),
      self(new PrivateData(this))
{
}

PluginDatabaseW::~PluginDatabaseW()
{
    delete self;
}

void PluginDatabaseW::showEvent(QShowEvent* event)
{
}

void PluginDatabaseW::slot_cellClicked(int row, int column)
{
}

void PluginDatabaseW::slot_cellDoubleClicked(int row, int column)
{
}

void PluginDatabaseW::slot_addPlugin()
{
}

void PluginDatabaseW::slot_checkPlugin(int row)
{
}

void PluginDatabaseW::slot_checkFilters()
{
}

void PluginDatabaseW::slot_refreshPlugins()
{
}

void PluginDatabaseW::slot_clearFilters()
{
}

void PluginDatabaseW::slot_saveSettings()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

struct JackApplicationW::PrivateData {
    PrivateData(void*)
    {
    }
};

JackApplicationW::JackApplicationW(QWidget* parent)
    : QDialog(parent),
      self(new PrivateData(this))
{
}

JackApplicationW::~JackApplicationW()
{
    delete self;
}

void JackApplicationW::getCommandAndFlags(QString& command, QString& name, QString& labelSetup)
{
}

void JackApplicationW::slot_commandChanged(QString text)
{
}

void JackApplicationW::slot_sessionManagerChanged(int index)
{
}

void JackApplicationW::slot_saveSettings()
{
}

// --------------------------------------------------------------------------------------------------------------------
