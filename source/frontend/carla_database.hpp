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

#ifndef CARLA_DATABASE_HPP_INCLUDED
#define CARLA_DATABASE_HPP_INCLUDED

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtCore/QThread>

#include <QtWidgets/QDialog>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "CarlaJuceUtils.hpp"

class CarlaHost;

// --------------------------------------------------------------------------------------------------------------------
// Separate Thread for Plugin Search

class SearchPluginsThread : public QThread
{
    Q_OBJECT

signals:
    void pluginLook(float percent, QString plugin);

public:
    SearchPluginsThread(QObject* parent, QString pathBinaries);
    ~SearchPluginsThread() override;

private:
    struct PrivateData;
    PrivateData* const self;

protected:
    void run() override;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchPluginsThread)
};

// --------------------------------------------------------------------------------------------------------------------
// Plugin Refresh Dialog

class PluginRefreshW : public QDialog
{
    Q_OBJECT

public:
    PluginRefreshW(QWidget* parent, const CarlaHost& host);
    ~PluginRefreshW() override;

    void getValues(QString& audioDevice, uint& bufferSize, double& sampleRate);

private:
    struct PrivateData;
    PrivateData* const self;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void slot_saveSettings();
    void slot_start();
    void slot_skip();
    void slot_checkTools();
    void slot_handlePluginLook(float percent, QString plugin);
    void slot_handlePluginThreadFinished();

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginRefreshW)
};

// --------------------------------------------------------------------------------------------------------------------
// Plugin Database Dialog

class PluginDatabaseW : public QDialog
{
    Q_OBJECT

public:
    PluginDatabaseW(QWidget* parent, const CarlaHost& host, bool hasCanvas, bool hasCanvasGL);
    ~PluginDatabaseW() override;

private:
    struct PrivateData;
    PrivateData* const self;

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void slot_cellClicked(int row, int column);
    void slot_cellDoubleClicked(int row, int column);
    void slot_addPlugin();
    void slot_checkPlugin(int row);
    void slot_checkFilters();
    void slot_refreshPlugins();
    void slot_clearFilters();
    void slot_saveSettings();

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDatabaseW)
};

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

class JackApplicationW : public QDialog
{
    Q_OBJECT

public:
    JackApplicationW(QWidget* parent, const QString& projectFilename);
    ~JackApplicationW() override;

    void getCommandAndFlags(QString& command, QString& name, QString& labelSetup);

private:
    struct PrivateData;
    PrivateData* const self;

private slots:
    void slot_commandChanged(QString text);
    void slot_sessionManagerChanged(int index);
    void slot_saveSettings();

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JackApplicationW)
};

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_DATABASE_HPP_INCLUDED
