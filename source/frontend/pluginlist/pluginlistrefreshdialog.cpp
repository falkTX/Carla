/*
 * Carla plugin host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#include "pluginlistrefreshdialog.hpp"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include "pluginlistrefreshdialog_ui.hpp"
#include <QtCore/QFileInfo>
#include <QtWidgets/QPushButton>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qsafesettings.hpp"

#include "CarlaUtils.h"

#include <cstring>

static bool hasFeature(const char* const* const features, const char* const feature)
{
    if (features == nullptr)
        return false;

    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i], feature) == 0)
            return true;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

struct PluginListRefreshDialog::Self {
    Ui_PluginRefreshW ui;
    QPixmap fIconYes;
    QPixmap fIconNo;

    Self() {}

    static Self& create()
    {
        Self* const self = new Self();
        return *self;
    }
};

PluginListRefreshDialog::PluginListRefreshDialog(QWidget* const parent, const bool useSystemIcons)
    : QDialog(parent),
      self(Self::create())
{
    self.ui.setupUi(this);

    // ----------------------------------------------------------------------------------------------------------------
    // Internal stuff

   #ifdef CARLA_OS_WIN
    #define APP_EXT ".exe"
   #else
    #define APP_EXT ".exe"
   #endif

    QString hostPathBinaries;
    const bool hasNative  = QFileInfo::exists(QCarlaString(hostPathBinaries) + CARLA_OS_SEP_STR "carla-discovery-native" APP_EXT);
    const bool hasPosix32 = QFileInfo::exists(QCarlaString(hostPathBinaries) + CARLA_OS_SEP_STR "carla-discovery-posix32");
    const bool hasPosix64 = QFileInfo::exists(QCarlaString(hostPathBinaries) + CARLA_OS_SEP_STR "carla-discovery-posix64");
    const bool hasWin32   = QFileInfo::exists(QCarlaString(hostPathBinaries) + CARLA_OS_SEP_STR "carla-discovery-win32.exe");
    const bool hasWin64   = QFileInfo::exists(QCarlaString(hostPathBinaries) + CARLA_OS_SEP_STR "carla-discovery-win64.exe");

    // self.fThread = SearchPluginsThread(self, host.pathBinaries)

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up Icons

    if (useSystemIcons)
    {
//         self.ui.b_start->setIcon(getIcon("arrow-right", 16, "svgz"));
//         self.ui.b_close->setIcon(getIcon("window-close", 16, "svgz"));
//         if QT_VERSION >= 0x50600:
//             size = int(16 * self.devicePixelRatioF())
//         else:
//             size = 16
//         self.fIconYes = QPixmap(getIcon('dialog-ok-apply', 16, 'svgz').pixmap(size))
//         self.fIconNo  = QPixmap(getIcon('dialog-error', 16, 'svgz').pixmap(size))
    }
    else
    {
        self.fIconYes = QPixmap(":/16x16/dialog-ok-apply.svgz");
        self.fIconNo  = QPixmap(":/16x16/dialog-error.svgz");
    }

    // ----------------------------------------------------------------------------------------------------------------
    // UI setup

    // FIXME remove LRDF
    self.ui.ico_rdflib->setPixmap(self.fIconNo);

    self.ui.b_skip->hide();

   #if defined(CARLA_OS_HAIKU)
    self.ui.ch_posix32->setText("Haiku 32bit");
    self.ui.ch_posix64->setText("Haiku 64bit");
   #elif defined(CARLA_OS_LINUX)
    self.ui.ch_posix32->setText("Linux 32bit");
    self.ui.ch_posix64->setText("Linux 64bit");
   #elif defined(CARLA_OS_MAC)
    self.ui.ch_posix32->setText("MacOS 32bit");
    self.ui.ch_posix64->setText("MacOS 64bit");
   #endif

   #ifndef CARLA_OS_WIN
    if (hasPosix32)
    {
        self.ui.ico_posix32->setPixmap(self.fIconYes);
    }
    else
   #endif
    {
        self.ui.ico_posix32->setPixmap(self.fIconNo);
        self.ui.ch_posix32->setEnabled(false);
    }

   #ifndef CARLA_OS_WIN
    if (hasPosix64)
    {
        self.ui.ico_posix64->setPixmap(self.fIconYes);
    }
    else
   #endif
    {
        self.ui.ico_posix64->setPixmap(self.fIconNo);
        self.ui.ch_posix64->setEnabled(false);
    }

    if (hasWin32)
    {
        self.ui.ico_win32->setPixmap(self.fIconYes);
    }
    else
    {
        self.ui.ico_win32->setPixmap(self.fIconNo);
        self.ui.ch_win32->setEnabled(false);
    }

    if (hasWin64)
    {
        self.ui.ico_win64->setPixmap(self.fIconYes);
    }
    else
    {
        self.ui.ico_win64->setPixmap(self.fIconNo);
        self.ui.ch_win64->setEnabled(false);
    }

    bool hasNonNative;

  #if defined(CARLA_OS_WIN)
   #ifdef CARLA_OS_64BIT
    hasNonNative = hasWin32;
    self.ui.ch_win64->setEnabled(false);
    self.ui.ch_win64->setVisible(false);
    self.ui.ico_win64->setVisible(false);
    self.ui.label_win64->setVisible(false);
   #else
    hasNonNative = hasWin64;
    self.ui.ch_win32->setEnabled(false);
    self.ui.ch_win32->setVisible(false);
    self.ui.ico_win32->setVisible(false);
    self.ui.label_win32->setVisible(false);
   #endif

    self.ui.ch_posix32->setEnabled(false);
    self.ui.ch_posix32->setVisible(false);
    self.ui.ch_posix64->setEnabled(false);
    self.ui.ch_posix64->setVisible(false);
    self.ui.ico_posix32->hide();
    self.ui.ico_posix64->hide();
    self.ui.label_posix32->hide();
    self.ui.label_posix64->hide();
    self.ui.ico_rdflib->hide();
    self.ui.label_rdflib->hide();
  #elif defined(CARLA_OS_64BIT)
    hasNonNative = hasPosix32 || hasWin32 || hasWin64;
    self.ui.ch_posix64->setEnabled(false);
    self.ui.ch_posix64->setVisible(false);
    self.ui.ico_posix64->setVisible(false);
    self.ui.label_posix64->setVisible(false);
  #else
    hasNonNative = hasPosix64 || hasWin32 || hasWin64;
    self.ui.ch_posix32->setEnabled(false);
    self.ui.ch_posix32->setVisible(false);
    self.ui.ico_posix32->setVisible(false);
    self.ui.label_posix32->setVisible(false);
  #endif

   #ifdef CARLA_OS_MAC
    setWindowModality(Qt::WindowModal);
   #else
    self.ui.ch_au->setEnabled(false);
    self.ui.ch_au->setVisible(false);
   #endif

    if (hasNative)
    {
        self.ui.ico_native->setPixmap(self.fIconYes);
    }
    else
    {
        self.ui.ico_native->setPixmap(self.fIconNo);
        self.ui.ch_native->setEnabled(false);
        self.ui.ch_sf2->setEnabled(false);
        if (! hasNonNative)
        {
            self.ui.ch_ladspa->setEnabled(false);
            self.ui.ch_dssi->setEnabled(false);
            self.ui.ch_vst->setEnabled(false);
            self.ui.ch_vst3->setEnabled(false);
            self.ui.ch_clap->setEnabled(false);
        }
    }

    // TODO
    // if (! hasLoadedLv2Plugins)
    self.ui.lv2_restart_notice->hide();

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Load settings

    loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // Hide bridges if disabled

    // TODO
    bool showPluginBridges = true;
    bool showWineBridges = true;

  #ifdef CARLA_OS_WIN
   #ifndef CARLA_OS_64BIT
    // NOTE: We Assume win32 carla build will not run win64 plugins
    showPluginBridges = false;
   #endif
    showWineBridges = false;
  #endif

    if (! showPluginBridges)
    {
        self.ui.ch_native->setChecked(true);
        self.ui.ch_native->setEnabled(false);
        self.ui.ch_native->setVisible(false);
        self.ui.ch_posix32->setChecked(false);
        self.ui.ch_posix32->setEnabled(false);
        self.ui.ch_posix32->setVisible(false);
        self.ui.ch_posix64->setChecked(false);
        self.ui.ch_posix64->setEnabled(false);
        self.ui.ch_posix64->setVisible(false);
        self.ui.ch_win32->setChecked(false);
        self.ui.ch_win32->setEnabled(false);
        self.ui.ch_win32->setVisible(false);
        self.ui.ch_win64->setChecked(false);
        self.ui.ch_win64->setEnabled(false);
        self.ui.ch_win64->setVisible(false);
        self.ui.ico_posix32->hide();
        self.ui.ico_posix64->hide();
        self.ui.ico_win32->hide();
        self.ui.ico_win64->hide();
        self.ui.label_posix32->hide();
        self.ui.label_posix64->hide();
        self.ui.label_win32->hide();
        self.ui.label_win64->hide();
        self.ui.sep_format->hide();
    }
    else if (! showWineBridges)
    {
        self.ui.ch_win32->setChecked(false);
        self.ui.ch_win32->setEnabled(false);
        self.ui.ch_win32->setVisible(false);
        self.ui.ch_win64->setChecked(false);
        self.ui.ch_win64->setEnabled(false);
        self.ui.ch_win64->setVisible(false);
        self.ui.ico_win32->hide();
        self.ui.ico_win64->hide();
        self.ui.label_win32->hide();
        self.ui.label_win64->hide();
    }

    // Disable non-supported features
    const char* const* const features = carla_get_supported_features();

    if (! hasFeature(features, "sf2"))
    {
        self.ui.ch_sf2->setChecked(false);
        self.ui.ch_sf2->setEnabled(false);
    }

   #ifdef CARLA_OS_MAC
    if (! hasFeature(features, "juce"))
    {
        self.ui.ch_au->setChecked(false);
        self.ui.ch_au->setEnabled(false);
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // Resize to minimum size, as it's very likely UI stuff was hidden

    resize(minimumSize());

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, &QDialog::finished,
            this, &PluginListRefreshDialog::slot_saveSettings);
//     self.ui.b_start.clicked.connect(self.slot_start)
//     self.ui.b_skip.clicked.connect(self.slot_skip)
//     self.ui.ch_native.clicked.connect(self.slot_checkTools)
//     self.ui.ch_posix32.clicked.connect(self.slot_checkTools)
//     self.ui.ch_posix64.clicked.connect(self.slot_checkTools)
//     self.ui.ch_win32.clicked.connect(self.slot_checkTools)
//     self.ui.ch_win64.clicked.connect(self.slot_checkTools)
//     self.ui.ch_ladspa.clicked.connect(self.slot_checkTools)
//     self.ui.ch_dssi.clicked.connect(self.slot_checkTools)
//     self.ui.ch_lv2.clicked.connect(self.slot_checkTools)
//     self.ui.ch_vst.clicked.connect(self.slot_checkTools)
//     self.ui.ch_vst3.clicked.connect(self.slot_checkTools)
//     self.ui.ch_clap.clicked.connect(self.slot_checkTools)
//     self.ui.ch_au.clicked.connect(self.slot_checkTools)
//     self.ui.ch_sf2.clicked.connect(self.slot_checkTools)
//     self.ui.ch_sfz.clicked.connect(self.slot_checkTools)
//     self.ui.ch_jsfx.clicked.connect(self.slot_checkTools)
//     self.fThread.pluginLook.connect(self.slot_handlePluginLook)
//     self.fThread.finished.connect(self.slot_handlePluginThreadFinished)

//     connect(this, &QDialog::finished,
//             this, &JackAppDialog::slot_saveSettings);
//     connect(self.ui.cb_session_mgr, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//             this, &JackAppDialog::slot_sessionManagerChanged);
//     connect(self.ui.le_command, &QLineEdit::textChanged,
//             this, &JackAppDialog::slot_commandChanged);

    // ----------------------------------------------------------------------------------------------------------------
    // Post-connect setup

    slot_checkTools();
}

PluginListRefreshDialog::~PluginListRefreshDialog()
{
    delete &self;
}

// -----------------------------------------------------------------------------------------------------------------
// public methods

// -----------------------------------------------------------------------------------------------------------------
// protected methods

void PluginListRefreshDialog::closeEvent(QCloseEvent* const event)
{
    /*
    if (self.fThread.isRunning())
    {
        self.fThread.stop();
        killDiscovery();
        #self.fThread.terminate();
        self.fThread.wait();
    }

    if (self.fThread.hasSomethingChanged())
        accept();
    else
    */
    reject();

    QDialog::closeEvent(event);
}

// -----------------------------------------------------------------------------------------------------------------
// private methods

void PluginListRefreshDialog::loadSettings()
{
    const QSafeSettings settings("falkTX", "CarlaRefresh2");

    bool check;

    check = settings.valueBool("PluginDatabase/SearchLADSPA", true) and self.ui.ch_ladspa->isEnabled();
    self.ui.ch_ladspa->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchDSSI", true) and self.ui.ch_dssi->isEnabled();
    self.ui.ch_dssi->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchLV2", true) and self.ui.ch_lv2->isEnabled();
    self.ui.ch_lv2->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchVST2", true) and self.ui.ch_vst->isEnabled();
    self.ui.ch_vst->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchVST3", true) and self.ui.ch_vst3->isEnabled();
    self.ui.ch_vst3->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchCLAP", true) and self.ui.ch_clap->isEnabled();
    self.ui.ch_clap->setChecked(check);

   #ifdef CARLA_OS_MAC
    check = settings.valueBool("PluginDatabase/SearchAU", true) and self.ui.ch_au->isEnabled();
   #else
    check = false;
   #endif
    self.ui.ch_au->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchSF2", false) and self.ui.ch_sf2->isEnabled();
    self.ui.ch_sf2->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchSFZ", false) and self.ui.ch_sfz->isEnabled();
    self.ui.ch_sfz->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchJSFX", true) and self.ui.ch_jsfx->isEnabled();
    self.ui.ch_jsfx->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchNative", true) and self.ui.ch_native->isEnabled();
    self.ui.ch_native->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchPOSIX32", false) and self.ui.ch_posix32->isEnabled();
    self.ui.ch_posix32->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchPOSIX64", false) and self.ui.ch_posix64->isEnabled();
    self.ui.ch_posix64->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchWin32", false) and self.ui.ch_win32->isEnabled();
    self.ui.ch_win32->setChecked(check);

    check = settings.valueBool("PluginDatabase/SearchWin64", false) and self.ui.ch_win64->isEnabled();
    self.ui.ch_win64->setChecked(check);

    self.ui.ch_do_checks->setChecked(settings.valueBool("PluginDatabase/DoChecks", false));
}

// -----------------------------------------------------------------------------------------------------------------
// private slots

void PluginListRefreshDialog::slot_saveSettings()
{
    QSafeSettings settings("falkTX", "CarlaRefresh2");
    settings.setValue("PluginDatabase/SearchLADSPA", self.ui.ch_ladspa->isChecked());
    settings.setValue("PluginDatabase/SearchDSSI", self.ui.ch_dssi->isChecked());
    settings.setValue("PluginDatabase/SearchLV2", self.ui.ch_lv2->isChecked());
    settings.setValue("PluginDatabase/SearchVST2", self.ui.ch_vst->isChecked());
    settings.setValue("PluginDatabase/SearchVST3", self.ui.ch_vst3->isChecked());
    settings.setValue("PluginDatabase/SearchCLAP", self.ui.ch_clap->isChecked());
    settings.setValue("PluginDatabase/SearchAU", self.ui.ch_au->isChecked());
    settings.setValue("PluginDatabase/SearchSF2", self.ui.ch_sf2->isChecked());
    settings.setValue("PluginDatabase/SearchSFZ", self.ui.ch_sfz->isChecked());
    settings.setValue("PluginDatabase/SearchJSFX", self.ui.ch_jsfx->isChecked());
    settings.setValue("PluginDatabase/SearchNative", self.ui.ch_native->isChecked());
    settings.setValue("PluginDatabase/SearchPOSIX32", self.ui.ch_posix32->isChecked());
    settings.setValue("PluginDatabase/SearchPOSIX64", self.ui.ch_posix64->isChecked());
    settings.setValue("PluginDatabase/SearchWin32", self.ui.ch_win32->isChecked());
    settings.setValue("PluginDatabase/SearchWin64", self.ui.ch_win64->isChecked());
    settings.setValue("PluginDatabase/DoChecks", self.ui.ch_do_checks->isChecked());
}

void PluginListRefreshDialog::slot_start()
{
    self.ui.progressBar->setMinimum(0);
    self.ui.progressBar->setMaximum(100);
    self.ui.progressBar->setValue(0);
    self.ui.b_start->setEnabled(false);
    self.ui.b_skip->setVisible(true);
    self.ui.b_close->setVisible(false);
    self.ui.group_types->setEnabled(false);
    self.ui.group_options->setEnabled(false);

//     if gCarla.utils:
//         if self.ui.ch_do_checks.isChecked():
//             gCarla.utils.unsetenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS")
//         else:
//             gCarla.utils.setenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS", "true")
//
//     native, posix32, posix64, win32, win64 = (self.ui.ch_native.isChecked(),
//                                                 self.ui.ch_posix32.isChecked(), self.ui.ch_posix64.isChecked(),
//                                                 self.ui.ch_win32.isChecked(), self.ui.ch_win64.isChecked())
//
//     ladspa, dssi, lv2, vst, vst3, clap, au, sf2, sfz, jsfx = (self.ui.ch_ladspa.isChecked(), self.ui.ch_dssi.isChecked(),
//                                                                 self.ui.ch_lv2.isChecked(), self.ui.ch_vst.isChecked(),
//                                                                 self.ui.ch_vst3.isChecked(), self.ui.ch_clap.isChecked(),
//                                                                 self.ui.ch_au.isChecked(), self.ui.ch_sf2.isChecked(),
//                                                                 self.ui.ch_sfz.isChecked(), self.ui.ch_jsfx.isChecked())
//
//     self.fThread.setSearchBinaryTypes(native, posix32, posix64, win32, win64)
//     self.fThread.setSearchPluginTypes(ladspa, dssi, lv2, vst, vst3, clap, au, sf2, sfz, jsfx)
//     self.fThread.start()
}

void PluginListRefreshDialog::slot_skip()
{
    // killDiscovery();
}

void PluginListRefreshDialog::slot_checkTools()
{
    const bool enabled1 = self.ui.ch_native->isChecked()
                        || self.ui.ch_posix32->isChecked()
                        || self.ui.ch_posix64->isChecked()
                        || self.ui.ch_win32->isChecked()
                        || self.ui.ch_win64->isChecked();

    const bool enabled2 = self.ui.ch_ladspa->isChecked()
                        || self.ui.ch_dssi->isChecked()
                        || self.ui.ch_lv2->isChecked()
                        || self.ui.ch_vst->isChecked()
                        || self.ui.ch_vst3->isChecked()
                        || self.ui.ch_clap->isChecked()
                        || self.ui.ch_au->isChecked()
                        || self.ui.ch_sf2->isChecked()
                        || self.ui.ch_sfz->isChecked()
                        || self.ui.ch_jsfx->isChecked();

    self.ui.b_start->setEnabled(enabled1 and enabled2);
}

void PluginListRefreshDialog::slot_handlePluginLook(const int percent, const QString plugin)
{
    self.ui.progressBar->setFormat(plugin);
    self.ui.progressBar->setValue(percent);
}

void PluginListRefreshDialog::slot_handlePluginThreadFinished()
{
    self.ui.progressBar->setMinimum(0);
    self.ui.progressBar->setMaximum(1);
    self.ui.progressBar->setValue(1);
    self.ui.progressBar->setFormat(tr("Done"));
    self.ui.b_start->setEnabled(true);
    self.ui.b_skip->setVisible(false);
    self.ui.b_close->setVisible(true);
    self.ui.group_types->setEnabled(true);
    self.ui.group_options->setEnabled(true);
}

// --------------------------------------------------------------------------------------------------------------------

PluginListRefreshDialogResults* carla_frontend_createAndExecPluginListRefreshDialog(void* const parent,
                                                                                    const bool useSystemIcons)
{
    PluginListRefreshDialog gui(reinterpret_cast<QWidget*>(parent), useSystemIcons);

    if (gui.exec())
    {
        static PluginListRefreshDialogResults ret = {};

        return &ret;
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
