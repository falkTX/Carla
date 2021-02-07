/*
 * Carla settings code
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

#ifndef CARLA_SETTINGS_HPP_INCLUDED
#define CARLA_SETTINGS_HPP_INCLUDED

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtWidgets/QDialog>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "CarlaHost.h"
#include "CarlaJuceUtils.hpp"

class CarlaHost;

// --------------------------------------------------------------------------------------------------------------------
// Driver Settings

class DriverSettingsW : public QDialog
{
    Q_OBJECT

public:
    DriverSettingsW(QWidget* parent, uint driverIndex, QString driverName);
    ~DriverSettingsW() override;

private:
    struct PrivateData;
    PrivateData* const self;

    Q_SLOT void slot_saveSettings();
    Q_SLOT void slot_showDevicePanel();
    Q_SLOT void slot_updateDeviceInfo();

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriverSettingsW)
};

// --------------------------------------------------------------------------------------------------------------------
// Runtime Driver Settings

class RuntimeDriverSettingsW : public QDialog
{
    Q_OBJECT

public:
    RuntimeDriverSettingsW(CarlaHostHandle hostHandle, QWidget* parent = nullptr);
    ~RuntimeDriverSettingsW() override;

    void getValues(QString& audioDevice, uint& bufferSize, double& sampleRate);

private:
    struct PrivateData;
    PrivateData* const self;

    Q_SLOT void slot_showDevicePanel();

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RuntimeDriverSettingsW)
};

// --------------------------------------------------------------------------------------------------------------------
// Settings Dialog

class CarlaSettingsW : public QDialog
{
    Q_OBJECT

public:
    CarlaSettingsW(QWidget* parent, CarlaHost& host, bool hasCanvas, bool hasCanvasGL);
    ~CarlaSettingsW() override;

private:
    struct PrivateData;
    PrivateData* const self;

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_saveSettings();
    Q_SLOT void slot_resetSettings();

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_enableExperimental(bool toggled);
    Q_SLOT void slot_enableWineBridges(bool toggled);
    Q_SLOT void slot_pluginBridgesToggled(bool toggled);
    Q_SLOT void slot_canvasEyeCandyToggled(bool toggled);
    Q_SLOT void slot_canvasFancyEyeCandyToggled(bool toggled);
    Q_SLOT void slot_canvasOpenGLToggled(bool toggled);

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_getAndSetProjectPath();

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_engineAudioDriverChanged();
    Q_SLOT void slot_showAudioDriverSettings();

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_addPluginPath();
    Q_SLOT void slot_removePluginPath();
    Q_SLOT void slot_changePluginPath();

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_pluginPathTabChanged(int index);
    Q_SLOT void slot_pluginPathRowChanged(int row);

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_addFilePath();
    Q_SLOT void slot_removeFilePath();
    Q_SLOT void slot_changeFilePath();

    // ----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_filePathTabChanged(int index);
    Q_SLOT void slot_filePathRowChanged(int row);

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaSettingsW)
};

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_SETTINGS_HPP_INCLUDED
