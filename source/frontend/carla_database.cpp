/*
 * Carla plugin database code
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

//---------------------------------------------------------------------------------------------------------------------

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtWidgets/QPushButton>

//---------------------------------------------------------------------------------------------------------------------

#include "ui_carla_add_jack.hpp"
#include "ui_carla_database.hpp"
#include "ui_carla_refresh.hpp"

//---------------------------------------------------------------------------------------------------------------------

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_host.hpp"

#include "CarlaHost.h"
#include "CarlaLibJackHints.h"

// --------------------------------------------------------------------------------------------------------------------
// Separate Thread for Plugin Search

struct WineSettings {
    QString executable;
    bool autoPrefix;
    QString fallbackPrefix;

    WineSettings()
        : executable(),
          autoPrefix(false),
          fallbackPrefix()
    {
        const QSafeSettings settings("falkTX", "Carla2");

        executable = settings.valueString(CARLA_KEY_WINE_EXECUTABLE, CARLA_DEFAULT_WINE_EXECUTABLE);
        autoPrefix = settings.valueBool(CARLA_KEY_WINE_AUTO_PREFIX, CARLA_DEFAULT_WINE_AUTO_PREFIX);
        fallbackPrefix = settings.valueString(CARLA_KEY_WINE_FALLBACK_PREFIX, CARLA_DEFAULT_WINE_FALLBACK_PREFIX);
    }
};

struct SearchPluginsThread::PrivateData {
    bool fContinueChecking;
    QString fPathBinaries;

    bool fCheckNative;
    bool fCheckPosix32;
    bool fCheckPosix64;
    bool fCheckWin32;
    bool fCheckWin64;

    bool fCheckLADSPA;
    bool fCheckDSSI;
    bool fCheckLV2;
    bool fCheckVST2;
    bool fCheckVST3;
    bool fCheckAU;
    bool fCheckSF2;
    bool fCheckSFZ;

    WineSettings fWineSettings;

    QString fToolNative;

    uint fCurCount;
    uint fCurPercentValue;
    uint fLastCheckValue;
    bool fSomethingChanged;

    PrivateData(void*, const QString pathBinaries)
        : fContinueChecking(false),
          fPathBinaries(pathBinaries),
          fCheckNative(false),
          fCheckPosix32(false),
          fCheckPosix64(false),
          fCheckWin32(false),
          fCheckWin64(false),
          fCheckLADSPA(false),
          fCheckDSSI(false),
          fCheckLV2(false),
          fCheckVST2(false),
          fCheckVST3(false),
          fCheckAU(false),
          fCheckSF2(false),
          fCheckSFZ(false),
          fWineSettings(),
          fToolNative(),
          fCurCount(0),
          fCurPercentValue(0),
          fLastCheckValue(0),
          fSomethingChanged(false)
    {
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

SearchPluginsThread::SearchPluginsThread(QObject* parent, const QString pathBinaries)
    : QThread(parent),
      self(new PrivateData(this, pathBinaries))
{
}

SearchPluginsThread::~SearchPluginsThread()
{
    delete self;
}

void SearchPluginsThread::run()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Plugin Refresh Dialog

struct PluginRefreshW::PrivateData {
    Ui::PluginRefreshW ui;

    SearchPluginsThread fThread;
    QPixmap fIconYes;
    QPixmap fIconNo;

    PrivateData(PluginRefreshW* const refreshDialog, const CarlaHost& host)
        : ui(),
          fThread(refreshDialog, host.pathBinaries),
          fIconYes(":/16x16/dialog-ok-apply.svgz"),
          fIconNo(":/16x16/dialog-error.svgz")
    {
        ui.setupUi(refreshDialog);

        // ------------------------------------------------------------------------------------------------------------
        // Internal stuff

        const bool hasNative  = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-native");
        const bool hasPosix32 = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-posix32");
        const bool hasPosix64 = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-posix64");
        const bool hasWin32   = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-win32.exe");
        const bool hasWin64   = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-win64.exe");
    }

    void loadSettings()
    {
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

PluginRefreshW::PluginRefreshW(QWidget* const parent, const CarlaHost& host)
    : QDialog(parent),
      self(new PrivateData(this, host))
{
    // ----------------------------------------------------------------------------------------------------------------
    // Resize to minimum size, as it's very likely UI stuff was hidden

    resize(minimumSize());

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, SIGNAL(finished(int)), SLOT(slot_saveSettings()));
    connect(self->ui.b_start, SIGNAL(clicked()), SLOT(slot_start()));
    connect(self->ui.b_skip, SIGNAL(clicked()), SLOT(slot_skip()));
    connect(self->ui.ch_native, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_posix32, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_posix64, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_win32, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_win64, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_ladspa, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_dssi, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_lv2, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_vst, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_vst3, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_au, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_sf2, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_sfz, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(&self->fThread, SIGNAL(pluginLook(float, QString)), SLOT(slot_handlePluginLook(float, QString)));
    connect(&self->fThread, SIGNAL(finished(int)), SLOT(slot_handlePluginThreadFinished()));

    // ----------------------------------------------------------------------------------------------------------------
    // Post-connect setup

    slot_checkTools();
}

PluginRefreshW::~PluginRefreshW()
{
    delete self;
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

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
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

// NOTE: index matches the one in the UI
enum UiSessionManager {
    UI_SESSION_NONE,
    UI_SESSION_LADISH,
    UI_SESSION_NSM
};

struct JackApplicationW::PrivateData {
    Ui::Dialog ui;

    const QString fProjectFilename;

    PrivateData(JackApplicationW* const dialog, const QString& projectFilename)
        : ui(),
          fProjectFilename(projectFilename)
    {
        ui.setupUi(dialog);

        ui.group_error->setVisible(false);

        // ------------------------------------------------------------------------------------------------------------
        // Load settings

        loadSettings();
    }

    void checkIfButtonBoxShouldBeEnabled(int index, const QString text)
    {
        static QList<QChar> badFirstChars = { '.', '/' };

        bool enabled = text.length() > 0;
        QCarlaString showErr;

        // NSM applications must not be abstract or absolute paths, and must not contain arguments
        if (enabled && index == UI_SESSION_NSM)
        {
            if (badFirstChars.contains(text[0]))
                showErr = tr("NSM applications cannot use abstract or absolute paths");
            else if (text.contains(' ') || text.contains(';') || text.contains('&'))
                showErr = tr("NSM applications cannot use CLI arguments");
            else if (fProjectFilename.isEmpty())
                showErr = tr("You need to save the current Carla project before NSM can be used");
        }

        if (showErr.isNotEmpty())
        {
            enabled = false;
            ui.l_error->setText(showErr);
            ui.group_error->setVisible(true);
        }
        else
        {
            ui.group_error->setVisible(false);
        }

        if (QPushButton* const button = ui.buttonBox->button(QDialogButtonBox::Ok))
            button->setEnabled(enabled);
    }

    void loadSettings()
    {
        const QSafeSettings settings("falkTX", "CarlaAddJackApp");

        const QString smName = settings.valueString("SessionManager", "");

        if (smName == "LADISH (SIGUSR1)")
            ui.cb_session_mgr->setCurrentIndex(UI_SESSION_LADISH);
        else if (smName == "NSM")
            ui.cb_session_mgr->setCurrentIndex(UI_SESSION_NSM);
        else
            ui.cb_session_mgr->setCurrentIndex(UI_SESSION_NONE);

        ui.le_command->setText(settings.valueString("Command", ""));
        ui.le_name->setText(settings.valueString("Name", ""));
        ui.sb_audio_ins->setValue(settings.valueIntPositive("NumAudioIns", 2));
        ui.sb_audio_ins->setValue(settings.valueIntPositive("NumAudioIns", 2));
        ui.sb_audio_outs->setValue(settings.valueIntPositive("NumAudioOuts", 2));
        ui.sb_midi_ins->setValue(settings.valueIntPositive("NumMidiIns", 0));
        ui.sb_midi_outs->setValue(settings.valueIntPositive("NumMidiOuts", 0));
        ui.cb_manage_window->setChecked(settings.valueBool("ManageWindow", true));
        ui.cb_capture_first_window->setChecked(settings.valueBool("CaptureFirstWindow", false));
        ui.cb_out_midi_mixdown->setChecked(settings.valueBool("MidiOutMixdown", false));

        checkIfButtonBoxShouldBeEnabled(ui.cb_session_mgr->currentIndex(), ui.le_command->text());
    }

    void saveSettings()
    {
        QSafeSettings settings("falkTX", "CarlaAddJackApp");
        settings.setValue("Command", ui.le_command->text());
        settings.setValue("Name", ui.le_name->text());
        settings.setValue("SessionManager", ui.cb_session_mgr->currentText());
        settings.setValue("NumAudioIns", ui.sb_audio_ins->value());
        settings.setValue("NumAudioOuts", ui.sb_audio_outs->value());
        settings.setValue("NumMidiIns", ui.sb_midi_ins->value());
        settings.setValue("NumMidiOuts", ui.sb_midi_outs->value());
        settings.setValue("ManageWindow", ui.cb_manage_window->isChecked());
        settings.setValue("CaptureFirstWindow", ui.cb_capture_first_window->isChecked());
        settings.setValue("MidiOutMixdown", ui.cb_out_midi_mixdown->isChecked());
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

JackApplicationW::JackApplicationW(QWidget* parent, const QString& projectFilename)
    : QDialog(parent),
      self(new PrivateData(this, projectFilename))
{
    adjustSize();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, SIGNAL(finished(int)), SLOT(slot_saveSettings()));
    connect(self->ui.cb_session_mgr, SIGNAL(currentIndexChanged(int)), SLOT(slot_sessionManagerChanged(int)));
    connect(self->ui.le_command, SIGNAL(textChanged(QString)), SLOT(slot_commandChanged(QString)));
}

JackApplicationW::~JackApplicationW()
{
    delete self;
}

void JackApplicationW::getCommandAndFlags(QString& command, QString& name, QString& labelSetup)
{
    name    = self->ui.le_name->text();
    command = self->ui.le_command->text();

    if (name.isEmpty())
    {
        name = QFileInfo(command.split(' ').first()).baseName();
        // FIXME
        name[0] = name[0].toTitleCase();
    }

    SessionManager smgr;
    switch (self->ui.cb_session_mgr->currentIndex())
    {
    case UI_SESSION_LADISH:
        smgr = LIBJACK_SESSION_MANAGER_LADISH;
        break;
    case UI_SESSION_NSM:
        smgr = LIBJACK_SESSION_MANAGER_NSM;
        break;
    default:
        smgr = LIBJACK_SESSION_MANAGER_NONE;
        break;
    }

    uint flags = 0x0;
    if (self->ui.cb_manage_window->isChecked())
        flags |= LIBJACK_FLAG_CONTROL_WINDOW;
    if (self->ui.cb_capture_first_window->isChecked())
        flags |= LIBJACK_FLAG_CAPTURE_FIRST_WINDOW;
    if (self->ui.cb_buffers_addition_mode->isChecked())
        flags |= LIBJACK_FLAG_AUDIO_BUFFERS_ADDITION;
    if (self->ui.cb_out_midi_mixdown->isChecked())
        flags |= LIBJACK_FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN;
    if (self->ui.cb_external_start->isChecked())
        flags |= LIBJACK_FLAG_EXTERNAL_START;

    labelSetup = QString("%1%2%3%4%5%6").arg(QChar('0' + self->ui.sb_audio_ins->value()))
                                        .arg(QChar('0' + self->ui.sb_audio_outs->value()))
                                        .arg(QChar('0' + self->ui.sb_midi_ins->value()))
                                        .arg(QChar('0' + self->ui.sb_midi_outs->value()))
                                        .arg(QChar('0' + smgr))
                                        .arg(QChar('0' + flags));
}

void JackApplicationW::slot_commandChanged(const QString text)
{
    self->checkIfButtonBoxShouldBeEnabled(self->ui.cb_session_mgr->currentIndex(), text);
}

void JackApplicationW::slot_sessionManagerChanged(const int index)
{
    self->checkIfButtonBoxShouldBeEnabled(index, self->ui.le_command->text());
}

void JackApplicationW::slot_saveSettings()
{
    self->saveSettings();
}

// --------------------------------------------------------------------------------------------------------------------
