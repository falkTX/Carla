/*
 * Carla widgets code
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

#ifndef CARLA_WIDGETS_HPP_INCLUDED
#define CARLA_WIDGETS_HPP_INCLUDED

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

#include "CarlaJuceUtils.hpp"

class CarlaHost;

// --------------------------------------------------------------------------------------------------------------------
// Carla About dialog

class CarlaAboutW : public QDialog
{
    Q_OBJECT

public:
    CarlaAboutW(QWidget* parent, const CarlaHost& host);
    ~CarlaAboutW() override;

private:
    struct PrivateData;
    PrivateData* const self;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaAboutW)
};

// --------------------------------------------------------------------------------------------------------------------
// JUCE About dialog

class JuceAboutW : public QDialog
{
    Q_OBJECT

public:
    JuceAboutW(QWidget* parent);
    ~JuceAboutW() override;

private:
    struct PrivateData;
    PrivateData* const self;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuceAboutW)
};

// --------------------------------------------------------------------------------------------------------------------
// Settings Dialog

class PluginParameter : public QWidget
{
    Q_OBJECT

signals:
    void mappedControlChanged(int, int);
    void midiChannelChanged(int, int);
    void valueChanged(int, float);

public:
    PluginParameter(QWidget* parent, const CarlaHost& host);
    ~PluginParameter() override;

private:
    struct PrivateData;
    PrivateData* const self;

    // ----------------------------------------------------------------------------------------------------------------

private slots:
    void slot_optionsCustomMenu();
    void slot_parameterDragStateChanged(bool touch);

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginParameter)
};

//---------------------------------------------------------------------------------------------------------------------
// Plugin Editor Parent (Meta class)

class PluginEditParentMeta
{
protected:
    virtual ~PluginEditParentMeta() {};
    virtual void editDialogVisibilityChanged(int pluginId, bool visible) = 0;
    virtual void editDialogPluginHintsChanged(int pluginId, int hints) = 0;
    virtual void editDialogParameterValueChanged(int pluginId, int parameterId, float value) = 0;
    virtual void editDialogProgramChanged(int pluginId, int index) = 0;
    virtual void editDialogMidiProgramChanged(int pluginId, int index) = 0;
    virtual void editDialogNotePressed(int pluginId, int note) = 0;
    virtual void editDialogNoteReleased(int pluginId, int note) = 0;
    virtual void editDialogMidiActivityChanged(int pluginId, bool onOff) = 0;
};

//---------------------------------------------------------------------------------------------------------------------
// Plugin Editor (Built-in)

class PluginEdit : public QDialog
{
    Q_OBJECT

public:
    PluginEdit(QWidget* parent, const CarlaHost& host);
    ~PluginEdit() override;

private:
    struct PrivateData;
    PrivateData* const self;

protected:
    void closeEvent(QCloseEvent* event) override;
    void timerEvent(QTimerEvent* event) override;

private slots:
    void slot_handleNoteOnCallback(int pluginId, int channel, int note, int velocity);
    void slot_handleNoteOffCallback(int pluginId, int channel, int note);
    void slot_handleUpdateCallback(int pluginId);
    void slot_handleReloadInfoCallback(int pluginId);
    void slot_handleReloadParametersCallback(int pluginId);
    void slot_handleReloadProgramsCallback(int pluginId);
    void slot_handleReloadAllCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_stateSave();
    void slot_stateLoad();

    //-----------------------------------------------------------------------------------------------------------------

    void slot_optionChanged(bool clicked);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_dryWetChanged(float value);
    void slot_volumeChanged(float value);
    void slot_balanceLeftChanged(float value);
    void slot_balanceRightChanged(float value);
    void slot_panChanged(float value);
    void slot_ctrlChannelChanged(int value);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_parameterValueChanged(int parameterId, float value);
    void slot_parameterMappedControlChanged(int parameterId, int control);
    void slot_parameterMidiChannelChanged(int parameterId, int channel);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_programIndexChanged(int index);
    void slot_midiProgramIndexChanged(int index);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_noteOn(int note);
    void slot_noteOff(int note);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_finished();

    //-----------------------------------------------------------------------------------------------------------------

    void slot_knobCustomMenu();

    //-----------------------------------------------------------------------------------------------------------------

    void slot_channelCustomMenu();

    //-----------------------------------------------------------------------------------------------------------------

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEdit)
};

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_WIDGETS_HPP_INCLUDED
