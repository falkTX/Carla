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

#include "carla_settings.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#include <QtCore/QStringList>

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "ui_carla_settings.hpp"
#include "ui_carla_settings_driver.hpp"

#include "carla_host.hpp"
#include "patchcanvas/theme.hpp"

#include "CarlaHost.h"
#include "CarlaUtils.hpp"

// --------------------------------------------------------------------------------------------------------------------

static const char* const AUTOMATIC_OPTION = "(Auto)";

static const QList<uint> BUFFER_SIZE_LIST = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };
static const QList<double> SAMPLE_RATE_LIST = { 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000 };

CARLA_BACKEND_USE_NAMESPACE;

// --------------------------------------------------------------------------------------------------------------------
// Driver Settings

struct DriverSettingsW::PrivateData {
    Ui::DriverSettingsW ui;

    uint fDriverIndex;
    QString fDriverName;
    QStringList fDeviceNames;

    QList<uint> fBufferSizes;
    QList<double> fSampleRates;

    PrivateData(DriverSettingsW* const self, const uint driverIndex, const QString driverName)
        : ui(),
          fDriverIndex(driverIndex),
          fDriverName(driverName),
          fDeviceNames(),
          fBufferSizes(BUFFER_SIZE_LIST),
          fSampleRates(SAMPLE_RATE_LIST)
    {
        ui.setupUi(self);

        const char* const* const deviceNames = carla_get_engine_driver_device_names(driverIndex);
        CARLA_SAFE_ASSERT_RETURN(deviceNames != nullptr,);

        fillQStringListFromStringArray(fDeviceNames, deviceNames);
    }

    void loadSettings(DriverSettingsW* const self)
    {
        const QSafeSettings settings("falkTX", "Carla2");

        const QString prefix(QString("%1%2").arg(CARLA_KEY_ENGINE_DRIVER_PREFIX).arg(fDriverName));

        const QCarlaString audioDevice       = settings.valueString(QString("%1/Device").arg(prefix), "");
        const uint         audioBufferSize   = settings.valueUInt(QString("%1/BufferSize").arg(prefix), CARLA_DEFAULT_AUDIO_BUFFER_SIZE);
        const uint         audioSampleRate   = settings.valueUInt(QString("%1/SampleRate").arg(prefix), CARLA_DEFAULT_AUDIO_SAMPLE_RATE);
        const bool         audioTripleBuffer = settings.valueBool(QString("%1/TripleBuffer").arg(prefix), CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER);

        if (audioDevice.isNotEmpty() && fDeviceNames.contains(audioDevice))
            ui.cb_device->setCurrentIndex(fDeviceNames.indexOf(audioDevice));
        else
            ui.cb_device->setCurrentIndex(-1);

        // fill combo-boxes first
        self->slot_updateDeviceInfo();

        if (audioBufferSize != 0 && fBufferSizes.contains(audioBufferSize))
            ui.cb_buffersize->setCurrentIndex(fBufferSizes.indexOf(audioBufferSize));
        else if (fBufferSizes == BUFFER_SIZE_LIST)
            ui.cb_buffersize->setCurrentIndex(BUFFER_SIZE_LIST.indexOf(CARLA_DEFAULT_AUDIO_BUFFER_SIZE));
        else
            ui.cb_buffersize->setCurrentIndex(fBufferSizes.size()/2);

        if (audioSampleRate != 0 && fSampleRates.contains(audioSampleRate))
            ui.cb_samplerate->setCurrentIndex(fSampleRates.indexOf(audioSampleRate));
        else if (fSampleRates == SAMPLE_RATE_LIST)
            ui.cb_samplerate->setCurrentIndex(SAMPLE_RATE_LIST.indexOf(CARLA_DEFAULT_AUDIO_SAMPLE_RATE));
        else
            ui.cb_samplerate->setCurrentIndex(fSampleRates.size()/2);

        ui.cb_triple_buffer->setChecked(audioTripleBuffer && ui.cb_triple_buffer->isEnabled());
    }
};

DriverSettingsW::DriverSettingsW(QWidget* const parent, const uint driverIndex, const QString driverName)
    : QDialog(parent),
      self(new PrivateData(this, driverIndex, driverName))
{
    // ----------------------------------------------------------------------------------------------------------------
    // Set-up GUI

    for (const auto& name : self->fDeviceNames)
        self->ui.cb_device->addItem(name);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Load settings

    self->loadSettings(this);

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, SIGNAL(accepted()), SLOT(slot_saveSettings()));
    connect(self->ui.b_panel, SIGNAL(clicked()), SLOT(slot_showDevicePanel()));
    connect(self->ui.cb_device, SIGNAL(currentIndexChanged(int)), SLOT(slot_updateDeviceInfo()));
}

DriverSettingsW::~DriverSettingsW()
{
    delete self;
}

// --------------------------------------------------------------------------------------------------------------------

void DriverSettingsW::slot_saveSettings()
{
    QSafeSettings settings("falkTX", "Carla2");

    QString bufferSize = self->ui.cb_buffersize->currentText();
    QString sampleRate = self->ui.cb_samplerate->currentText();

    if (bufferSize == AUTOMATIC_OPTION)
        bufferSize = "0";
    if (sampleRate == AUTOMATIC_OPTION)
        sampleRate = "0";

    const QString prefix(QString("%1%2").arg(CARLA_KEY_ENGINE_DRIVER_PREFIX).arg(self->fDriverName));

    settings.setValue(QString("%1/Device").arg(prefix), self->ui.cb_device->currentText());
    settings.setValue(QString("%1/BufferSize").arg(prefix), bufferSize);
    settings.setValue(QString("%1/SampleRate").arg(prefix), sampleRate);
    settings.setValue(QString("%1/TripleBuffer").arg(prefix), self->ui.cb_triple_buffer->isChecked());
}

void DriverSettingsW::slot_showDevicePanel()
{
    carla_show_engine_driver_device_control_panel(self->fDriverIndex, self->ui.cb_device->currentText().toUtf8());
}

void DriverSettingsW::slot_updateDeviceInfo()
{
    const QString deviceName = self->ui.cb_device->currentText();

    const QString oldBufferSize = self->ui.cb_buffersize->currentText();
    const QString oldSampleRate = self->ui.cb_samplerate->currentText();

    self->ui.cb_buffersize->clear();
    self->ui.cb_samplerate->clear();

    self->fBufferSizes.clear();
    self->fSampleRates.clear();

    const EngineDriverDeviceInfo* const driverDeviceInfo = carla_get_engine_driver_device_info(self->fDriverIndex, deviceName.toUtf8());
    CARLA_SAFE_ASSERT_RETURN(driverDeviceInfo != nullptr,);

    const uint driverDeviceHints = driverDeviceInfo->hints;

    fillQUIntListFromUIntArray(self->fBufferSizes, driverDeviceInfo->bufferSizes);
    fillQDoubleListFromDoubleArray(self->fSampleRates, driverDeviceInfo->sampleRates);

    if (driverDeviceHints & ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER)
        self->ui.cb_triple_buffer->setEnabled(true);
    else
        self->ui.cb_triple_buffer->setEnabled(false);

    if (driverDeviceHints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL)
        self->ui.b_panel->setEnabled(true);
    else
        self->ui.b_panel->setEnabled(false);

    if (self->fBufferSizes.size() > 0)
    {
        for (const uint bsize : self->fBufferSizes)
        {
            const QString sbsize(QString("%1").arg(bsize));
            self->ui.cb_buffersize->addItem(sbsize);

            if (oldBufferSize == sbsize)
                self->ui.cb_buffersize->setCurrentIndex(self->ui.cb_buffersize->count()-1);
        }
    }
    else
    {
        self->ui.cb_buffersize->addItem(AUTOMATIC_OPTION);
        self->ui.cb_buffersize->setCurrentIndex(0);
    }

    if (self->fSampleRates.size() > 0)
    {
        for (const double srate : self->fSampleRates)
        {
            const QString ssrate(QString("%1").arg(srate));
            self->ui.cb_samplerate->addItem(ssrate);

            if (oldSampleRate == ssrate)
                self->ui.cb_samplerate->setCurrentIndex(self->ui.cb_samplerate->count()-1);
        }
    }
    else
    {
        self->ui.cb_samplerate->addItem(AUTOMATIC_OPTION);
        self->ui.cb_samplerate->setCurrentIndex(0);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Runtime Driver Settings

struct RuntimeDriverSettingsW::PrivateData {
    Ui::DriverSettingsW ui;

    PrivateData(RuntimeDriverSettingsW* const self)
        : ui()
    {
        ui.setupUi(self);
    }
};

RuntimeDriverSettingsW::RuntimeDriverSettingsW(QWidget* const parent)
    : QDialog(parent),
      self(new PrivateData(this))
{
    const CarlaRuntimeEngineDriverDeviceInfo* const driverDeviceInfo = carla_get_runtime_engine_driver_device_info();

    QList<uint> bufferSizes;
    fillQUIntListFromUIntArray(bufferSizes, driverDeviceInfo->bufferSizes);

    QList<double> sampleRates;
    fillQDoubleListFromDoubleArray(sampleRates, driverDeviceInfo->sampleRates);

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up GUI

    self->ui.cb_device->clear();
    self->ui.cb_buffersize->clear();
    self->ui.cb_samplerate->clear();
    self->ui.cb_triple_buffer->hide();
    self->ui.ico_restart->hide();
    self->ui.label_restart->hide();

    self->ui.layout_triple_buffer->takeAt(2);
    self->ui.layout_triple_buffer->takeAt(1);
    self->ui.layout_triple_buffer->takeAt(0);
    self->ui.verticalLayout->removeItem(self->ui.layout_triple_buffer);

    self->ui.layout_restart->takeAt(3);
    self->ui.layout_restart->takeAt(2);
    self->ui.layout_restart->takeAt(1);
    self->ui.layout_restart->takeAt(0);
    self->ui.verticalLayout->removeItem(self->ui.layout_restart);

    adjustSize();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Load runtime settings

    if (carla_is_engine_running())
    {
        self->ui.cb_device->addItem(driverDeviceInfo->name);
        self->ui.cb_device->setCurrentIndex(0);
        self->ui.cb_device->setEnabled(false);
    }
    else
    {
        self->ui.cb_device->addItem(driverDeviceInfo->name);
        self->ui.cb_device->setCurrentIndex(0);
    }

    if (bufferSizes.size() > 0)
    {
        for (const uint bsize : bufferSizes)
        {
            const QString sbsize(QString("%1").arg(bsize));
            self->ui.cb_buffersize->addItem(sbsize);

            if (driverDeviceInfo->bufferSize == bsize)
                self->ui.cb_buffersize->setCurrentIndex(self->ui.cb_buffersize->count()-1);
        }
    }
    else
    {
        self->ui.cb_buffersize->addItem(AUTOMATIC_OPTION);
        self->ui.cb_buffersize->setCurrentIndex(0);
    }

    if ((driverDeviceInfo->hints & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE) == 0x0)
        self->ui.cb_buffersize->setEnabled(false);

    if (sampleRates.size() > 0)
    {
        for (const double srate : sampleRates)
        {
            const QString ssrate(QString("%1").arg(srate));
            self->ui.cb_samplerate->addItem(ssrate);

            if (driverDeviceInfo->sampleRate == srate)
                self->ui.cb_samplerate->setCurrentIndex(self->ui.cb_samplerate->count()-1);
        }
    }
    else
    {
        self->ui.cb_samplerate->addItem(AUTOMATIC_OPTION);
        self->ui.cb_samplerate->setCurrentIndex(0);
    }

    if ((driverDeviceInfo->hints & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE) == 0x0)
        self->ui.cb_samplerate->setEnabled(false);

    if ((driverDeviceInfo->hints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL) == 0x0)
        self->ui.b_panel->setEnabled(false);

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(self->ui.b_panel, SIGNAL(clicked()), SLOT(slot_showDevicePanel()));
}

RuntimeDriverSettingsW::~RuntimeDriverSettingsW()
{
    delete self;
}

void RuntimeDriverSettingsW::getValues(QString& retAudioDevice, uint& retBufferSize, double& retSampleRate)
{
    const QString bufferSize = self->ui.cb_buffersize->currentText();
    const QString sampleRate = self->ui.cb_samplerate->currentText();

    if (bufferSize == AUTOMATIC_OPTION)
        retBufferSize = 0;
    else
        retBufferSize = bufferSize.toUInt();

    if (sampleRate == AUTOMATIC_OPTION)
        retSampleRate = 0.0;
    else
        retSampleRate = sampleRate.toDouble();

    retAudioDevice = self->ui.cb_buffersize->currentText();
}

void RuntimeDriverSettingsW::slot_showDevicePanel()
{
    carla_show_engine_device_control_panel();
}

// --------------------------------------------------------------------------------------------------------------------
// Settings Dialog

enum TabIndexes {
    TAB_INDEX_MAIN,
    TAB_INDEX_CANVAS,
    TAB_INDEX_ENGINE,
    TAB_INDEX_OSC,
    TAB_INDEX_FILEPATHS,
    TAB_INDEX_PLUGINPATHS,
    TAB_INDEX_WINE,
    TAB_INDEX_EXPERIMENTAL,
    TAB_INDEX_NONE,
};

enum FilePathIndexes {
    FILEPATH_INDEX_AUDIO,
    FILEPATH_INDEX_MIDI
};

enum PluginPathIndexes {
    PLUGINPATH_INDEX_LADSPA,
    PLUGINPATH_INDEX_DSSI,
    PLUGINPATH_INDEX_LV2,
    PLUGINPATH_INDEX_VST2,
    PLUGINPATH_INDEX_VST3,
    PLUGINPATH_INDEX_SF2,
    PLUGINPATH_INDEX_SFZ
};

/*
  Single and Multiple client mode is only for JACK,
  but we still want to match QComboBox index to backend defines,
  so add +2 pos padding if driverName != "JACK".
*/
#define PROCESS_MODE_NON_JACK_PADDING 2

struct CarlaSettingsW::PrivateData {
    Ui::CarlaSettingsW ui;

    PrivateData(CarlaSettingsW* const self)
        : ui()
    {
        ui.setupUi(self);
    }

    void loadSettings()
    {
    }

    void resetExperimentalSettings()
    {
        // Forever experimental
        ui.cb_exp_plugin_bridges->setChecked(CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES);
        ui.ch_exp_wine_bridges->setChecked(CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES);
        ui.ch_exp_jack_apps->setChecked(CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS);
        ui.ch_exp_export_lv2->setChecked(CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT);
        ui.ch_exp_load_lib_global->setChecked(CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL);
        ui.ch_exp_prevent_bad_behaviour->setChecked(CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR);

        // Temporary, until stable
        ui.cb_canvas_fancy_eyecandy->setChecked(CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY);
        ui.cb_canvas_use_opengl->setChecked(CARLA_DEFAULT_CANVAS_USE_OPENGL and ui.cb_canvas_use_opengl->isEnabled());
        ui.cb_canvas_render_hq_aa->setChecked(CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING and ui.cb_canvas_render_hq_aa->isEnabled());
        ui.cb_canvas_inline_displays->setChecked(CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS);
        ui.ch_engine_force_stereo->setChecked(CARLA_DEFAULT_FORCE_STEREO);
        ui.ch_engine_prefer_plugin_bridges->setChecked(CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES);
    }
};

CarlaSettingsW::CarlaSettingsW(QWidget* const parent, const CarlaHost& host, const bool hasCanvas, const bool hasCanvasGL)
    : QDialog(parent),
      self(new PrivateData(this))
{
    // ----------------------------------------------------------------------------------------------------------------
    // Set-up GUI

    self->ui.lw_page->setFixedWidth(48 + 6*3 + fontMetricsHorizontalAdvance(self->ui.lw_page->fontMetrics(), "   Experimental   "));

    for (uint i=0; i < carla_get_engine_driver_count(); ++i)
        self->ui.cb_engine_audio_driver->addItem(carla_get_engine_driver_name(i));

    for (uint i=0; i < Theme::THEME_MAX; ++i)
        self->ui.cb_canvas_theme->addItem(getThemeName((Theme::List)i));

#ifdef CARLA_OS_MAC
    self->ui.group_main_theme->setEnabled(false);
    self->ui.group_main_theme->setVisible(false);
#endif

#ifdef CARLA_OS_Win
    if (true)
#else
    if (host.isControl)
#endif
    {
        self->ui.ch_main_show_logs->setEnabled(false);
        self->ui.ch_main_show_logs->setVisible(false);
    }

    if (host.isControl)
    {
        self->ui.lw_page->hideRow(TAB_INDEX_ENGINE);
        self->ui.lw_page->hideRow(TAB_INDEX_FILEPATHS);
        self->ui.lw_page->hideRow(TAB_INDEX_PLUGINPATHS);
        self->ui.ch_exp_export_lv2->hide();
        self->ui.group_experimental_engine->hide();
    }
    else if (! hasCanvas)
    {
        self->ui.lw_page->hideRow(TAB_INDEX_CANVAS);
    }
    else if (! hasCanvasGL)
    {
        self->ui.cb_canvas_use_opengl->setEnabled(false);
        self->ui.cb_canvas_render_hq_aa->setEnabled(false);
    }

    if (host.isPlugin)
        self->ui.cb_engine_audio_driver->setEnabled(false);

    if (host.audioDriverForced.isNotEmpty())
    {
        self->ui.cb_engine_audio_driver->setEnabled(false);
        self->ui.tb_engine_driver_config->setEnabled(false);
    }

    if (host.processModeForced)
    {
        self->ui.cb_engine_process_mode_jack->setEnabled(false);
        self->ui.cb_engine_process_mode_other->setEnabled(false);

        if (host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            self->ui.ch_engine_force_stereo->setEnabled(false);
    }

    if (host.isControl || host.isPlugin)
    {
        self->ui.ch_main_confirm_exit->hide();
        self->ui.ch_exp_load_lib_global->hide();
        self->ui.lw_page->hideRow(TAB_INDEX_OSC);
        self->ui.lw_page->hideRow(TAB_INDEX_WINE);
    }

#ifndef CARLA_OS_LINUX
    self->ui.ch_exp_wine_bridges->setVisible(false);
    self->ui.ch_exp_jack_apps->setVisible(false);
    self->ui.ch_exp_prevent_bad_behaviour->setVisible(false);
    self->ui.lw_page->hideRow(TAB_INDEX_WINE);
#endif

#ifndef CARLA_OS_MAC
    self->ui.label_engine_ui_bridges_mac_note->setVisible(false);
#endif

    // FIXME, not implemented yet
    self->ui.ch_engine_uis_always_on_top->hide();

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Load settings

    self->loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, SIGNAL(accepted()), SLOT(slot_saveSettings()));
    connect(self->ui.buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), SLOT(slot_resetSettings()));

    connect(self->ui.b_main_proj_folder_open, SIGNAL(clicked()), SLOT(slot_getAndSetProjectPath()));

    connect(self->ui.cb_engine_audio_driver, SIGNAL(currentIndexChanged(int)), SLOT(slot_engineAudioDriverChanged()));
    connect(self->ui.tb_engine_driver_config, SIGNAL(clicked()), SLOT(slot_showAudioDriverSettings()));

    connect(self->ui.b_paths_add, SIGNAL(clicked()), SLOT(slot_addPluginPath()));
    connect(self->ui.b_paths_remove, SIGNAL(clicked()), SLOT(slot_removePluginPath()));
    connect(self->ui.b_paths_change, SIGNAL(clicked()), SLOT(slot_changePluginPath()));
    connect(self->ui.cb_paths, SIGNAL(currentIndexChanged(int)), SLOT(slot_pluginPathTabChanged(int)));
    connect(self->ui.lw_ladspa, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));
    connect(self->ui.lw_dssi, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));
    connect(self->ui.lw_lv2, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));
    connect(self->ui.lw_vst, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));
    connect(self->ui.lw_vst3, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));
    connect(self->ui.lw_sf2, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));
    connect(self->ui.lw_sfz, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));

    connect(self->ui.b_filepaths_add, SIGNAL(clicked()), SLOT(slot_addFilePath()));
    connect(self->ui.b_filepaths_remove, SIGNAL(clicked()), SLOT(slot_removeFilePath()));
    connect(self->ui.b_filepaths_change, SIGNAL(clicked()), SLOT(slot_changeFilePath()));
    connect(self->ui.cb_filepaths, SIGNAL(currentIndexChanged(int)), SLOT(slot_filePathTabChanged(int)));
    connect(self->ui.lw_files_audio, SIGNAL(currentRowChanged(int)), SLOT(slot_filePathRowChanged(int)));
    connect(self->ui.lw_files_midi, SIGNAL(currentRowChanged(int)), SLOT(slot_filePathRowChanged(int)));

    connect(self->ui.ch_main_experimental, SIGNAL(toggled(bool)), SLOT(slot_enableExperimental(bool)));
    connect(self->ui.ch_exp_wine_bridges, SIGNAL(toggled(bool)), SLOT(slot_enableWineBridges(bool)));
    connect(self->ui.cb_exp_plugin_bridges, SIGNAL(toggled(bool)), SLOT(slot_pluginBridgesToggled(bool)));
    connect(self->ui.cb_canvas_eyecandy, SIGNAL(toggled(bool)), SLOT(slot_canvasEyeCandyToggled(bool)));
    connect(self->ui.cb_canvas_fancy_eyecandy, SIGNAL(toggled(bool)), SLOT(slot_canvasFancyEyeCandyToggled(bool)));
    connect(self->ui.cb_canvas_use_opengl, SIGNAL(toggled(bool)), SLOT(slot_canvasOpenGLToggled(bool)));

    // ----------------------------------------------------------------------------------------------------------------
    // Post-connect setup

    self->ui.lw_ladspa->setCurrentRow(0);
    self->ui.lw_dssi->setCurrentRow(0);
    self->ui.lw_lv2->setCurrentRow(0);
    self->ui.lw_vst->setCurrentRow(0);
    self->ui.lw_vst3->setCurrentRow(0);
    self->ui.lw_sf2->setCurrentRow(0);
    self->ui.lw_sfz->setCurrentRow(0);

    self->ui.lw_files_audio->setCurrentRow(0);
    self->ui.lw_files_midi->setCurrentRow(0);

    self->ui.lw_page->setCurrentCell(0, 0);

    slot_filePathTabChanged(0);
    slot_pluginPathTabChanged(0);

    adjustSize();
}

CarlaSettingsW::~CarlaSettingsW()
{
    delete self;
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_saveSettings()
{
}

void CarlaSettingsW::slot_resetSettings()
{
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_enableExperimental(bool toggled)
{
}

void CarlaSettingsW::slot_enableWineBridges(bool toggled)
{
}

void CarlaSettingsW::slot_pluginBridgesToggled(bool toggled)
{
}

void CarlaSettingsW::slot_canvasEyeCandyToggled(bool toggled)
{
}

void CarlaSettingsW::slot_canvasFancyEyeCandyToggled(bool toggled)
{
}

void CarlaSettingsW::slot_canvasOpenGLToggled(bool toggled)
{
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_getAndSetProjectPath()
{
    // FIXME?
    getAndSetPath(this, self->ui.le_main_proj_folder);
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_engineAudioDriverChanged()
{
    if (self->ui.cb_engine_audio_driver->currentText() == "JACK")
        self->ui.sw_engine_process_mode->setCurrentIndex(0);
    else
        self->ui.sw_engine_process_mode->setCurrentIndex(1);
}

void CarlaSettingsW::slot_showAudioDriverSettings()
{
    const int     driverIndex = self->ui.cb_engine_audio_driver->currentIndex();
    const QString driverName  = self->ui.cb_engine_audio_driver->currentText();
    CARLA_SAFE_ASSERT_RETURN(driverIndex >= 0,);

    DriverSettingsW(this, static_cast<uint>(driverIndex), driverName).exec();
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_addPluginPath()
{
}

void CarlaSettingsW::slot_removePluginPath()
{
}

void CarlaSettingsW::slot_changePluginPath()
{
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_pluginPathTabChanged(int index)
{
}

void CarlaSettingsW::slot_pluginPathRowChanged(int row)
{
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_addFilePath()
{
}

void CarlaSettingsW::slot_removeFilePath()
{
}

void CarlaSettingsW::slot_changeFilePath()
{
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_filePathTabChanged(int index)
{
}

void CarlaSettingsW::slot_filePathRowChanged(int row)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Main

#include "carla_app.hpp"
#include "carla_shared.hpp"

int main(int argc, char* argv[])
{
    // ----------------------------------------------------------------------------------------------------------------
    // Read CLI args

    const QString initName(handleInitialCommandLineArguments(argc, argv));

    // ----------------------------------------------------------------------------------------------------------------
    // App initialization

    const CarlaApplication app("Carla2-Settings", argc, argv);

    // ----------------------------------------------------------------------------------------------------------------
    // Init host backend

    CarlaHost& host = initHost(initName, false, false, true);
    loadHostSettings(host);

    // ----------------------------------------------------------------------------------------------------------------
    // Create GUI

    CarlaSettingsW gui(nullptr, host, true, true);

    // ----------------------------------------------------------------------------------------------------------------
    // Show GUI

    gui.show();

    // ----------------------------------------------------------------------------------------------------------------
    // App-Loop

    return app.exec();
}
