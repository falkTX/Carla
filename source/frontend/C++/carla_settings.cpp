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

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

// --------------------------------------------------------------------------------------------------------------------

#include <QtCore/QStringList>

#include <QtWidgets/QFileDialog>

// --------------------------------------------------------------------------------------------------------------------

#include "ui_carla_settings.hpp"
#include "ui_carla_settings_driver.hpp"

// --------------------------------------------------------------------------------------------------------------------

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_host.hpp"
#include "patchcanvas/theme.hpp"

#include "CarlaHost.h"
#include "CarlaMathUtils.hpp"

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
            ui.cb_samplerate->setCurrentIndex(getIndexOfQDoubleListValue(fSampleRates, audioSampleRate));
        else if (isQDoubleListEqual(fSampleRates, SAMPLE_RATE_LIST))
            ui.cb_samplerate->setCurrentIndex(getIndexOfQDoubleListValue(SAMPLE_RATE_LIST, CARLA_DEFAULT_AUDIO_SAMPLE_RATE));
        else
            ui.cb_samplerate->setCurrentIndex(fSampleRates.size()/2);

        ui.cb_triple_buffer->setChecked(audioTripleBuffer && ui.cb_triple_buffer->isEnabled());
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
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
    const CarlaHostHandle hostHandle;

    PrivateData(RuntimeDriverSettingsW* const self, const CarlaHostHandle h)
        : ui(),
          hostHandle(h)
    {
        ui.setupUi(self);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

RuntimeDriverSettingsW::RuntimeDriverSettingsW(const CarlaHostHandle hostHandle, QWidget* const parent)
    : QDialog(parent),
      self(new PrivateData(this, hostHandle))
{
    const CarlaRuntimeEngineDriverDeviceInfo* const driverDeviceInfo =
        carla_get_runtime_engine_driver_device_info(hostHandle);

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

    if (carla_is_engine_running(hostHandle))
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

            if (carla_isEqual(driverDeviceInfo->sampleRate, srate))
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
    carla_show_engine_device_control_panel(self->hostHandle);
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
    PLUGINPATH_INDEX_SFZ,
    PLUGINPATH_INDEX_JSFX
};

/*
  Single and Multiple client mode is only for JACK,
  but we still want to match QComboBox index to backend defines,
  so add +2 pos padding if driverName != "JACK".
*/
#define PROCESS_MODE_NON_JACK_PADDING 2

struct CarlaSettingsW::PrivateData {
    Ui::CarlaSettingsW ui;
    CarlaHost& host;

    PrivateData(CarlaSettingsW* const self, CarlaHost& h)
        : ui(),
          host(h)
    {
        ui.setupUi(self);
    }

    void loadSettings()
    {
        {
            const QSafeSettings settings;

            // --------------------------------------------------------------------------------------------------------
            // Main

            ui.ch_main_show_logs->setChecked(host.showLogs);
            ui.ch_engine_uis_always_on_top->setChecked(host.uisAlwaysOnTop);

            ui.le_main_proj_folder->setText(settings.valueString(CARLA_KEY_MAIN_PROJECT_FOLDER,
                                                                 CARLA_DEFAULT_MAIN_PROJECT_FOLDER));

            ui.ch_main_theme_pro->setChecked(settings.valueBool(CARLA_KEY_MAIN_USE_PRO_THEME,
                                                                CARLA_DEFAULT_MAIN_USE_PRO_THEME) && ui.group_main_theme->isEnabled());

            ui.cb_main_theme_color->setCurrentIndex(ui.cb_main_theme_color->findText(settings.valueString(CARLA_KEY_MAIN_PRO_THEME_COLOR,
                                                                                                          CARLA_DEFAULT_MAIN_PRO_THEME_COLOR)));

            ui.sb_main_refresh_interval->setValue(settings.valueIntPositive(CARLA_KEY_MAIN_REFRESH_INTERVAL,
                                                                            CARLA_DEFAULT_MAIN_REFRESH_INTERVAL));

            ui.ch_main_confirm_exit->setChecked(settings.valueBool(CARLA_KEY_MAIN_CONFIRM_EXIT,
                                                                   CARLA_DEFAULT_MAIN_CONFIRM_EXIT));

            // --------------------------------------------------------------------------------------------------------
            // Canvas

            ui.cb_canvas_theme->setCurrentIndex(ui.cb_canvas_theme->findText(settings.valueString(CARLA_KEY_CANVAS_THEME,
                                                                                                  CARLA_DEFAULT_CANVAS_THEME)));

            ui.cb_canvas_size->setCurrentIndex(ui.cb_canvas_size->findText(settings.valueString(CARLA_KEY_CANVAS_SIZE,
                                                                                                CARLA_DEFAULT_CANVAS_SIZE)));

            ui.cb_canvas_bezier_lines->setChecked(settings.valueBool(CARLA_KEY_CANVAS_USE_BEZIER_LINES,
                                                                    CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES));

            ui.cb_canvas_hide_groups->setChecked(settings.valueBool(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS,
                                                                    CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS));

            ui.cb_canvas_auto_select->setChecked(settings.valueBool(CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS,
                                                                    CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS));

            ui.cb_canvas_eyecandy->setChecked(settings.valueBool(CARLA_KEY_CANVAS_EYE_CANDY,
                                                                 CARLA_DEFAULT_CANVAS_EYE_CANDY));

            ui.cb_canvas_fancy_eyecandy->setChecked(settings.valueBool(CARLA_KEY_CANVAS_FANCY_EYE_CANDY,
                                                                      CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY));

            ui.cb_canvas_use_opengl->setChecked(settings.valueBool(CARLA_KEY_CANVAS_USE_OPENGL,
                                                                   CARLA_DEFAULT_CANVAS_USE_OPENGL) && ui.cb_canvas_use_opengl->isEnabled());

            ui.cb_canvas_render_aa->setCheckState(settings.valueCheckState(CARLA_KEY_CANVAS_ANTIALIASING,
                                                                           CARLA_DEFAULT_CANVAS_ANTIALIASING));

            ui.cb_canvas_render_hq_aa->setChecked(settings.valueBool(CARLA_KEY_CANVAS_HQ_ANTIALIASING,
                                                                     CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING) && ui.cb_canvas_render_hq_aa->isEnabled());

            ui.cb_canvas_full_repaints->setChecked(settings.valueBool(CARLA_KEY_CANVAS_FULL_REPAINTS,
                                                                      CARLA_DEFAULT_CANVAS_FULL_REPAINTS));

            ui.cb_canvas_inline_displays->setChecked(settings.valueBool(CARLA_KEY_CANVAS_INLINE_DISPLAYS,
                                                                        CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS));
        }

        // ------------------------------------------------------------------------------------------------------------

        const QSafeSettings settings("falkTX", "Carla2");

        // ------------------------------------------------------------------------------------------------------------
        // Main

        ui.ch_main_experimental->setChecked(host.experimental);

        if (! host.experimental)
        {
            ui.lw_page->hideRow(TAB_INDEX_EXPERIMENTAL);
            ui.lw_page->hideRow(TAB_INDEX_WINE);
        }
        else if (! host.showWineBridges)
        {
            ui.lw_page->hideRow(TAB_INDEX_WINE);
        }

        // ------------------------------------------------------------------------------------------------------------
        // Engine

        QString audioDriver;

        if (host.isPlugin)
        {
            audioDriver = "Plugin";
            ui.cb_engine_audio_driver->setCurrentIndex(0);
        }
        else if (host.audioDriverForced.isNotEmpty())
        {
            audioDriver = host.audioDriverForced;
            ui.cb_engine_audio_driver->setCurrentIndex(0);
        }
        else
        {
            audioDriver = settings.valueString(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER);

            bool found = false;
            for (int i=0; i < ui.cb_engine_audio_driver->count(); ++i)
            {
                if (ui.cb_engine_audio_driver->itemText(i) == audioDriver)
                {
                    found = true;
                    ui.cb_engine_audio_driver->setCurrentIndex(i);
                    break;
                }
            }

            if (! found)
                ui.cb_engine_audio_driver->setCurrentIndex(-1);
        }

        if (audioDriver == "JACK")
            ui.sw_engine_process_mode->setCurrentIndex(0);
        else
            ui.sw_engine_process_mode->setCurrentIndex(1);

        ui.tb_engine_driver_config->setEnabled(host.audioDriverForced.isNotEmpty() && ! host.isPlugin);

        ui.cb_engine_process_mode_jack->setCurrentIndex(host.nextProcessMode);

        if (host.nextProcessMode >= PROCESS_MODE_NON_JACK_PADDING)
            ui.cb_engine_process_mode_other->setCurrentIndex(host.nextProcessMode-PROCESS_MODE_NON_JACK_PADDING);
        else
            ui.cb_engine_process_mode_other->setCurrentIndex(0);

        ui.sb_engine_max_params->setValue(static_cast<int>(host.maxParameters));
        ui.cb_engine_reset_xruns->setChecked(host.resetXruns);
        ui.ch_engine_manage_uis->setChecked(host.manageUIs);
        ui.ch_engine_prefer_ui_bridges->setChecked(host.preferUIBridges);
        ui.sb_engine_ui_bridges_timeout->setValue(host.uiBridgesTimeout);
        ui.ch_engine_force_stereo->setChecked(host.forceStereo || ! ui.ch_engine_force_stereo->isEnabled());
        ui.ch_engine_prefer_plugin_bridges->setChecked(host.preferPluginBridges);
        ui.ch_exp_export_lv2->setChecked(host.exportLV2);
        ui.cb_exp_plugin_bridges->setChecked(host.showPluginBridges);
        ui.ch_exp_wine_bridges->setChecked(host.showWineBridges);

        // ------------------------------------------------------------------------------------------------------------
        // OSC

        ui.ch_osc_enable->setChecked(settings.valueBool(CARLA_KEY_OSC_ENABLED,
                                                        CARLA_DEFAULT_OSC_ENABLED));

        ui.group_osc_tcp_port->setChecked(settings.valueBool(CARLA_KEY_OSC_TCP_PORT_ENABLED,
                                                             CARLA_DEFAULT_OSC_TCP_PORT_ENABLED));

        ui.group_osc_udp_port->setChecked(settings.valueBool(CARLA_KEY_OSC_UDP_PORT_ENABLED,
                                                             CARLA_DEFAULT_OSC_UDP_PORT_ENABLED));

        ui.sb_osc_tcp_port_number->setValue(settings.valueIntPositive(CARLA_KEY_OSC_TCP_PORT_NUMBER,
                                                                      CARLA_DEFAULT_OSC_TCP_PORT_NUMBER));

        ui.sb_osc_udp_port_number->setValue(settings.valueIntPositive(CARLA_KEY_OSC_UDP_PORT_NUMBER,
                                                                      CARLA_DEFAULT_OSC_UDP_PORT_NUMBER));

        if (settings.valueBool(CARLA_KEY_OSC_TCP_PORT_RANDOM, CARLA_DEFAULT_OSC_TCP_PORT_RANDOM))
        {
            ui.rb_osc_tcp_port_specific->setChecked(false);
            ui.rb_osc_tcp_port_random->setChecked(true);
        }
        else
        {
            ui.rb_osc_tcp_port_random->setChecked(false);
            ui.rb_osc_tcp_port_specific->setChecked(true);
        }

        if (settings.valueBool(CARLA_KEY_OSC_UDP_PORT_RANDOM, CARLA_DEFAULT_OSC_UDP_PORT_RANDOM))
        {
            ui.rb_osc_udp_port_specific->setChecked(false);
            ui.rb_osc_udp_port_random->setChecked(true);
        }
        else
        {
            ui.rb_osc_udp_port_random->setChecked(false);
            ui.rb_osc_udp_port_specific->setChecked(true);
        }

        // ------------------------------------------------------------------------------------------------------------
        // File Paths

        QStringList audioPaths = settings.valueStringList(CARLA_KEY_PATHS_AUDIO, CARLA_DEFAULT_FILE_PATH_AUDIO);
        QStringList midiPaths  = settings.valueStringList(CARLA_KEY_PATHS_MIDI,  CARLA_DEFAULT_FILE_PATH_MIDI);

        audioPaths.sort();
        midiPaths.sort();

        for (const QString& audioPath : audioPaths)
        {
            if (audioPath.isEmpty()) continue;
            ui.lw_files_audio->addItem(audioPath);
        }

        for (const QString& midiPath : midiPaths)
        {
            if (midiPath.isEmpty()) continue;
            ui.lw_files_midi->addItem(midiPath);
        }

        // ------------------------------------------------------------------------------------------------------------
        // Plugin Paths

        QStringList ladspas = settings.valueStringList(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH);
        QStringList dssis   = settings.valueStringList(CARLA_KEY_PATHS_DSSI,   CARLA_DEFAULT_DSSI_PATH);
        QStringList lv2s    = settings.valueStringList(CARLA_KEY_PATHS_LV2,    CARLA_DEFAULT_LV2_PATH);
        QStringList vst2s   = settings.valueStringList(CARLA_KEY_PATHS_VST2,   CARLA_DEFAULT_VST2_PATH);
        QStringList vst3s   = settings.valueStringList(CARLA_KEY_PATHS_VST3,   CARLA_DEFAULT_VST3_PATH);
        QStringList sf2s    = settings.valueStringList(CARLA_KEY_PATHS_SF2,    CARLA_DEFAULT_SF2_PATH);
        QStringList sfzs    = settings.valueStringList(CARLA_KEY_PATHS_SFZ,    CARLA_DEFAULT_SFZ_PATH);
        QStringList jsfxs   = settings.valueStringList(CARLA_KEY_PATHS_JSFX,   CARLA_DEFAULT_JSFX_PATH);

        ladspas.sort();
        dssis.sort();
        lv2s.sort();
        vst2s.sort();
        vst3s.sort();
        sf2s.sort();
        sfzs.sort();
        jsfxs.sort();

        for (const QString& ladspa : ladspas)
        {
            if (ladspa.isEmpty()) continue;
            ui.lw_ladspa->addItem(ladspa);
        }

        for (const QString& dssi : dssis)
        {
            if (dssi.isEmpty()) continue;
            ui.lw_dssi->addItem(dssi);
        }

        for (const QString& lv2 : lv2s)
        {
            if (lv2.isEmpty()) continue;
            ui.lw_lv2->addItem(lv2);
        }

        for (const QString& vst2 : vst2s)
        {
            if (vst2.isEmpty()) continue;
            ui.lw_vst->addItem(vst2);
        }

        for (const QString& vst3 : vst3s)
        {
            if (vst3.isEmpty()) continue;
            ui.lw_vst3->addItem(vst3);
        }

        for (const QString& sf2 : sf2s)
        {
            if (sf2.isEmpty()) continue;
            ui.lw_sf2->addItem(sf2);
        }

        for (const QString& sfz : sfzs)
        {
            if (sfz.isEmpty()) continue;
            ui.lw_sfz->addItem(sfz);
        }

        for (const QString& jsfx : jsfxs)
        {
            if (jsfx.isEmpty()) continue;
            ui.lw_jsfx->addItem(jsfx);
        }

        // ------------------------------------------------------------------------------------------------------------
        // Wine

        ui.le_wine_exec->setText(settings.valueString(CARLA_KEY_WINE_EXECUTABLE,
                                                      CARLA_DEFAULT_WINE_EXECUTABLE));

        ui.cb_wine_prefix_detect->setChecked(settings.valueBool(CARLA_KEY_WINE_AUTO_PREFIX,
                                                                CARLA_DEFAULT_WINE_AUTO_PREFIX));

        ui.le_wine_prefix_fallback->setText(settings.valueString(CARLA_KEY_WINE_FALLBACK_PREFIX,
                                                                 CARLA_DEFAULT_WINE_FALLBACK_PREFIX));

        ui.group_wine_realtime->setChecked(settings.valueBool(CARLA_KEY_WINE_RT_PRIO_ENABLED,
                                                              CARLA_DEFAULT_WINE_RT_PRIO_ENABLED));

        ui.sb_wine_base_prio->setValue(settings.valueIntPositive(CARLA_KEY_WINE_BASE_RT_PRIO,
                                                                 CARLA_DEFAULT_WINE_BASE_RT_PRIO));

        ui.sb_wine_server_prio->setValue(settings.valueIntPositive(CARLA_KEY_WINE_SERVER_RT_PRIO,
                                                                   CARLA_DEFAULT_WINE_SERVER_RT_PRIO));

        // ------------------------------------------------------------------------------------------------------------
        // Experimental

        ui.ch_exp_jack_apps->setChecked(settings.valueBool(CARLA_KEY_EXPERIMENTAL_JACK_APPS,
                                                           CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS));

        ui.ch_exp_export_lv2->setChecked(settings.valueBool(CARLA_KEY_EXPERIMENTAL_EXPORT_LV2,
                                                            CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT));

        ui.ch_exp_load_lib_global->setChecked(settings.valueBool(CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL,
                                                                 CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL));

        ui.ch_exp_prevent_bad_behaviour->setChecked(settings.valueBool(CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR,
                                                                       CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR));
    }

    void saveSettings()
    {
        {
            QSafeSettings settings;

            host.experimental = ui.ch_main_experimental->isChecked();

            if (! host.experimental)
                resetExperimentalSettings();

            // --------------------------------------------------------------------------------------------------------
            // Main

            settings.setValue(CARLA_KEY_MAIN_PROJECT_FOLDER,   ui.le_main_proj_folder->text());
            settings.setValue(CARLA_KEY_MAIN_CONFIRM_EXIT,     ui.ch_main_confirm_exit->isChecked());
            settings.setValue(CARLA_KEY_MAIN_USE_PRO_THEME,    ui.ch_main_theme_pro->isChecked());
            settings.setValue(CARLA_KEY_MAIN_PRO_THEME_COLOR,  ui.cb_main_theme_color->currentText());
            settings.setValue(CARLA_KEY_MAIN_REFRESH_INTERVAL, ui.sb_main_refresh_interval->value());

            // --------------------------------------------------------------------------------------------------------
            // Canvas

            settings.setValue(CARLA_KEY_CANVAS_THEME,             ui.cb_canvas_theme->currentText());
            settings.setValue(CARLA_KEY_CANVAS_SIZE,              ui.cb_canvas_size->currentText());
            settings.setValue(CARLA_KEY_CANVAS_USE_BEZIER_LINES,  ui.cb_canvas_bezier_lines->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS,  ui.cb_canvas_hide_groups->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS, ui.cb_canvas_auto_select->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_EYE_CANDY,         ui.cb_canvas_eyecandy->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_FANCY_EYE_CANDY,   ui.cb_canvas_fancy_eyecandy->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_USE_OPENGL,        ui.cb_canvas_use_opengl->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_HQ_ANTIALIASING,   ui.cb_canvas_render_hq_aa->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_ANTIALIASING,      ui.cb_canvas_render_aa->checkState()); // 0, 1, 2 match their enum variants
            settings.setValue(CARLA_KEY_CANVAS_FULL_REPAINTS,     ui.cb_canvas_full_repaints->isChecked());
            settings.setValue(CARLA_KEY_CANVAS_INLINE_DISPLAYS,   ui.cb_canvas_inline_displays->isChecked());
        }

        // ------------------------------------------------------------------------------------------------------------

        QSafeSettings settings("falkTX", "Carla2");

        // ------------------------------------------------------------------------------------------------------------
        // Main

        settings.setValue(CARLA_KEY_MAIN_EXPERIMENTAL, host.experimental);

        // ------------------------------------------------------------------------------------------------------------
        // Engine

        const QCarlaString audioDriver = ui.cb_engine_audio_driver->currentText();

        if (audioDriver.isNotEmpty() && host.audioDriverForced.isEmpty() && ! host.isPlugin)
            settings.setValue(CARLA_KEY_ENGINE_AUDIO_DRIVER, audioDriver);

        if (! host.processModeForced)
        {
            // engine sends callback if processMode really changes
            if (audioDriver == "JACK")
                host.nextProcessMode = static_cast<EngineProcessMode>(ui.cb_engine_process_mode_jack->currentIndex());
            else
                host.nextProcessMode = static_cast<EngineProcessMode>(ui.cb_engine_process_mode_other->currentIndex() + PROCESS_MODE_NON_JACK_PADDING);

            settings.setValue(CARLA_KEY_ENGINE_PROCESS_MODE, host.nextProcessMode);
        }

        host.exportLV2           = ui.ch_exp_export_lv2->isChecked();
        host.forceStereo         = ui.ch_engine_force_stereo->isChecked();
        host.resetXruns          = ui.cb_engine_reset_xruns->isChecked();
        host.maxParameters       = static_cast<uint>(std::max(0, ui.sb_engine_max_params->value()));
        host.manageUIs           = ui.ch_engine_manage_uis->isChecked();
        host.preferPluginBridges = ui.ch_engine_prefer_plugin_bridges->isChecked();
        host.preferUIBridges     = ui.ch_engine_prefer_ui_bridges->isChecked();
        host.showLogs            = ui.ch_main_show_logs->isChecked();
        host.showPluginBridges   = ui.cb_exp_plugin_bridges->isChecked();
        host.showWineBridges     = ui.ch_exp_wine_bridges->isChecked();
        host.uiBridgesTimeout    = ui.sb_engine_ui_bridges_timeout->value();
        host.uisAlwaysOnTop      = ui.ch_engine_uis_always_on_top->isChecked();

        if (ui.ch_engine_force_stereo->isEnabled())
            settings.setValue(CARLA_KEY_ENGINE_FORCE_STEREO, host.forceStereo);

        settings.setValue(CARLA_KEY_MAIN_SHOW_LOGS,               host.showLogs);
        settings.setValue(CARLA_KEY_ENGINE_MAX_PARAMETERS,        host.maxParameters);
        settings.setValue(CARLA_KEY_ENGINE_RESET_XRUNS,           host.resetXruns);
        settings.setValue(CARLA_KEY_ENGINE_MANAGE_UIS,            host.manageUIs);
        settings.setValue(CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES, host.preferPluginBridges);
        settings.setValue(CARLA_KEY_ENGINE_PREFER_UI_BRIDGES,     host.preferUIBridges);
        settings.setValue(CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT,    host.uiBridgesTimeout);
        settings.setValue(CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP,     host.uisAlwaysOnTop);
        settings.setValue(CARLA_KEY_EXPERIMENTAL_EXPORT_LV2,      host.exportLV2);
        settings.setValue(CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES,  host.showPluginBridges);
        settings.setValue(CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES,    host.showWineBridges);

        // ------------------------------------------------------------------------------------------------------------
        // OSC

        settings.setValue(CARLA_KEY_OSC_ENABLED,          ui.ch_osc_enable->isChecked());
        settings.setValue(CARLA_KEY_OSC_TCP_PORT_ENABLED, ui.group_osc_tcp_port->isChecked());
        settings.setValue(CARLA_KEY_OSC_UDP_PORT_ENABLED, ui.group_osc_udp_port->isChecked());
        settings.setValue(CARLA_KEY_OSC_TCP_PORT_RANDOM,  ui.rb_osc_tcp_port_random->isChecked());
        settings.setValue(CARLA_KEY_OSC_UDP_PORT_RANDOM,  ui.rb_osc_udp_port_random->isChecked());
        settings.setValue(CARLA_KEY_OSC_TCP_PORT_NUMBER,  ui.sb_osc_tcp_port_number->value());
        settings.setValue(CARLA_KEY_OSC_UDP_PORT_NUMBER,  ui.sb_osc_udp_port_number->value());

        // ------------------------------------------------------------------------------------------------------------
        // File Paths

        QStringList audioPaths;
        QStringList midiPaths;

        for (int i=0; i < ui.lw_files_audio->count(); ++i)
            audioPaths.append(ui.lw_files_audio->item(i)->text());

        for (int i=0; i < ui.lw_files_midi->count(); ++i)
            midiPaths.append(ui.lw_files_midi->item(i)->text());

        /* TODO
        host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_AUDIO, splitter.join(audioPaths));
        host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_MIDI,  splitter.join(midiPaths));
        */

        settings.setValue(CARLA_KEY_PATHS_AUDIO, audioPaths);
        settings.setValue(CARLA_KEY_PATHS_MIDI, midiPaths);

        // ------------------------------------------------------------------------------------------------------------
        // Plugin Paths

        QStringList ladspas;
        QStringList dssis;
        QStringList lv2s;
        QStringList vst2s;
        QStringList vst3s;
        QStringList sf2s;
        QStringList sfzs;
        QStringList jsfxs;

        for (int i=0; i < ui.lw_ladspa->count(); ++i)
            ladspas.append(ui.lw_ladspa->item(i)->text());

        for (int i=0; i < ui.lw_dssi->count(); ++i)
            dssis.append(ui.lw_dssi->item(i)->text());

        for (int i=0; i < ui.lw_lv2->count(); ++i)
            lv2s.append(ui.lw_lv2->item(i)->text());

        for (int i=0; i < ui.lw_vst->count(); ++i)
            vst2s.append(ui.lw_vst->item(i)->text());

        for (int i=0; i < ui.lw_vst3->count(); ++i)
            vst3s.append(ui.lw_vst3->item(i)->text());

        for (int i=0; i < ui.lw_sf2->count(); ++i)
            sf2s.append(ui.lw_sf2->item(i)->text());

        for (int i=0; i < ui.lw_sfz->count(); ++i)
            sfzs.append(ui.lw_sfz->item(i)->text());

        for (int i=0; i < ui.lw_jsfx->count(); ++i)
            jsfxs.append(ui.lw_jsfx->item(i)->text());

        /* TODO
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LADSPA, splitter.join(ladspas));
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_DSSI,   splitter.join(dssis));
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2,    splitter.join(lv2s));
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST2,   splitter.join(vst2s));
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST3,   splitter.join(vst3s));
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SF2,    splitter.join(sf2s));
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SFZ,    splitter.join(sfzs));
        host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_JSFX,   splitter.join(jsfxs));
        */

        settings.setValue(CARLA_KEY_PATHS_LADSPA, ladspas);
        settings.setValue(CARLA_KEY_PATHS_DSSI, dssis);
        settings.setValue(CARLA_KEY_PATHS_LV2, lv2s);
        settings.setValue(CARLA_KEY_PATHS_VST2, vst2s);
        settings.setValue(CARLA_KEY_PATHS_VST3, vst3s);
        settings.setValue(CARLA_KEY_PATHS_SF2, sf2s);
        settings.setValue(CARLA_KEY_PATHS_SFZ, sfzs);
        settings.setValue(CARLA_KEY_PATHS_JSFX, jsfxs);

        // ------------------------------------------------------------------------------------------------------------
        // Wine

        settings.setValue(CARLA_KEY_WINE_EXECUTABLE, ui.le_wine_exec->text());
        settings.setValue(CARLA_KEY_WINE_AUTO_PREFIX, ui.cb_wine_prefix_detect->isChecked());
        settings.setValue(CARLA_KEY_WINE_FALLBACK_PREFIX, ui.le_wine_prefix_fallback->text());
        settings.setValue(CARLA_KEY_WINE_RT_PRIO_ENABLED, ui.group_wine_realtime->isChecked());
        settings.setValue(CARLA_KEY_WINE_BASE_RT_PRIO, ui.sb_wine_base_prio->value());
        settings.setValue(CARLA_KEY_WINE_SERVER_RT_PRIO, ui.sb_wine_server_prio->value());

        // ------------------------------------------------------------------------------------------------------------
        // Experimental

        settings.setValue(CARLA_KEY_EXPERIMENTAL_JACK_APPS, ui.ch_exp_jack_apps->isChecked());
        settings.setValue(CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL, ui.ch_exp_load_lib_global->isChecked());
        settings.setValue(CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR, ui.ch_exp_prevent_bad_behaviour->isChecked());
    }

    void resetSettings()
    {
        switch (ui.lw_page->currentRow())
        {
        // ------------------------------------------------------------------------------------------------------------
        // Main

        case TAB_INDEX_MAIN:
            ui.le_main_proj_folder->setText(CARLA_DEFAULT_MAIN_PROJECT_FOLDER);
            ui.ch_main_theme_pro->setChecked(CARLA_DEFAULT_MAIN_USE_PRO_THEME && ui.group_main_theme->isEnabled());
            ui.cb_main_theme_color->setCurrentIndex(ui.cb_main_theme_color->findText(CARLA_DEFAULT_MAIN_PRO_THEME_COLOR));
            ui.sb_main_refresh_interval->setValue(CARLA_DEFAULT_MAIN_REFRESH_INTERVAL);
            ui.ch_main_confirm_exit->setChecked(CARLA_DEFAULT_MAIN_CONFIRM_EXIT);
            ui.ch_main_show_logs->setChecked(CARLA_DEFAULT_MAIN_SHOW_LOGS);
            break;

        // ------------------------------------------------------------------------------------------------------------
        // Canvas

        case TAB_INDEX_CANVAS:
            ui.cb_canvas_theme->setCurrentIndex(ui.cb_canvas_theme->findText(CARLA_DEFAULT_CANVAS_THEME));
            ui.cb_canvas_size->setCurrentIndex(ui.cb_canvas_size->findText(CARLA_DEFAULT_CANVAS_SIZE));
            ui.cb_canvas_bezier_lines->setChecked(CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES);
            ui.cb_canvas_hide_groups->setChecked(CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS);
            ui.cb_canvas_auto_select->setChecked(CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS);
            ui.cb_canvas_eyecandy->setChecked(CARLA_DEFAULT_CANVAS_EYE_CANDY);
            ui.cb_canvas_render_aa->setCheckState(Qt::PartiallyChecked); // CARLA_DEFAULT_CANVAS_ANTIALIASING
            ui.cb_canvas_full_repaints->setChecked(CARLA_DEFAULT_CANVAS_FULL_REPAINTS);
            break;

        // ------------------------------------------------------------------------------------------------------------
        // Engine

        case TAB_INDEX_ENGINE:
            if (! host.isPlugin)
                ui.cb_engine_audio_driver->setCurrentIndex(0);

            if (! host.processModeForced)
            {
                if (ui.cb_engine_audio_driver->currentText() == "JACK")
                {
                    ui.cb_engine_process_mode_jack->setCurrentIndex(CARLA_DEFAULT_PROCESS_MODE);
                    ui.sw_engine_process_mode->setCurrentIndex(0); // show all modes
                }
                else
                {
                    ui.cb_engine_process_mode_other->setCurrentIndex(CARLA_DEFAULT_PROCESS_MODE-PROCESS_MODE_NON_JACK_PADDING);
                    ui.sw_engine_process_mode->setCurrentIndex(1); // hide single+multi client modes
                }
            }

            ui.sb_engine_max_params->setValue(CARLA_DEFAULT_MAX_PARAMETERS);
            ui.cb_engine_reset_xruns->setChecked(CARLA_DEFAULT_RESET_XRUNS);
            ui.ch_engine_uis_always_on_top->setChecked(CARLA_DEFAULT_UIS_ALWAYS_ON_TOP);
            ui.ch_engine_prefer_ui_bridges->setChecked(CARLA_DEFAULT_PREFER_UI_BRIDGES);
            ui.sb_engine_ui_bridges_timeout->setValue(CARLA_DEFAULT_UI_BRIDGES_TIMEOUT);
            ui.ch_engine_manage_uis->setChecked(CARLA_DEFAULT_MANAGE_UIS);
            break;

        // ------------------------------------------------------------------------------------------------------------
        // OSC

        case TAB_INDEX_OSC:
            ui.ch_osc_enable->setChecked(CARLA_DEFAULT_OSC_ENABLED);
            ui.group_osc_tcp_port->setChecked(CARLA_DEFAULT_OSC_TCP_PORT_ENABLED);
            ui.group_osc_udp_port->setChecked(CARLA_DEFAULT_OSC_UDP_PORT_ENABLED);
            ui.sb_osc_tcp_port_number->setValue(CARLA_DEFAULT_OSC_TCP_PORT_NUMBER);
            ui.sb_osc_udp_port_number->setValue(CARLA_DEFAULT_OSC_UDP_PORT_NUMBER);

            if (CARLA_DEFAULT_OSC_TCP_PORT_RANDOM)
            {
                ui.rb_osc_tcp_port_specific->setChecked(false);
                ui.rb_osc_tcp_port_random->setChecked(true);
            }
            else
            {
                ui.rb_osc_tcp_port_random->setChecked(false);
                ui.rb_osc_tcp_port_specific->setChecked(true);
            }

            if (CARLA_DEFAULT_OSC_UDP_PORT_RANDOM)
            {
                ui.rb_osc_udp_port_specific->setChecked(false);
                ui.rb_osc_udp_port_random->setChecked(true);
            }
            else
            {
                ui.rb_osc_udp_port_random->setChecked(false);
                ui.rb_osc_udp_port_specific->setChecked(true);
            }
            break;

        // ------------------------------------------------------------------------------------------------------------
        // Plugin Paths

        case TAB_INDEX_FILEPATHS:
            switch (ui.tw_filepaths->currentIndex())
            {
            case FILEPATH_INDEX_AUDIO:
                ui.lw_files_audio->clear();
                break;
            case FILEPATH_INDEX_MIDI:
                ui.lw_files_midi->clear();
                break;
            }
            break;

        // ------------------------------------------------------------------------------------------------------------
        // Plugin Paths

        case TAB_INDEX_PLUGINPATHS:
        {
            QStringList paths;

            switch (ui.tw_paths->currentIndex())
            {
            case PLUGINPATH_INDEX_LADSPA:
                paths = CARLA_DEFAULT_LADSPA_PATH;
                paths.sort();
                ui.lw_ladspa->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_ladspa->addItem(path);
                }
                break;

            case PLUGINPATH_INDEX_DSSI:
                paths = CARLA_DEFAULT_DSSI_PATH;
                paths.sort();
                ui.lw_dssi->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_dssi->addItem(path);
                }
                break;

            case PLUGINPATH_INDEX_LV2:
                paths = CARLA_DEFAULT_LV2_PATH;
                paths.sort();
                ui.lw_lv2->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_lv2->addItem(path);
                }
                break;

            case PLUGINPATH_INDEX_VST2:
                paths = CARLA_DEFAULT_VST2_PATH;
                paths.sort();
                ui.lw_vst->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_vst->addItem(path);
                }
                break;

            case PLUGINPATH_INDEX_VST3:
                paths = CARLA_DEFAULT_VST3_PATH;
                paths.sort();
                ui.lw_vst3->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_vst3->addItem(path);
                }
                break;

            case PLUGINPATH_INDEX_SF2:
                paths = CARLA_DEFAULT_SF2_PATH;
                paths.sort();
                ui.lw_sf2->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_sf2->addItem(path);
                }
                break;

            case PLUGINPATH_INDEX_SFZ:
                paths = CARLA_DEFAULT_SFZ_PATH;
                paths.sort();
                ui.lw_sfz->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_sfz->addItem(path);
                }
                break;

            case PLUGINPATH_INDEX_JSFX:
                paths = CARLA_DEFAULT_JSFX_PATH;
                paths.sort();
                ui.lw_jsfx->clear();

                for (const auto& path : paths)
                {
                    if (path.isEmpty())
                        continue;
                    ui.lw_jsfx->addItem(path);
                }
                break;
            }
            break;
        }

        // ------------------------------------------------------------------------------------------------------------
        // Wine

        case TAB_INDEX_WINE:
            ui.le_wine_exec->setText(CARLA_DEFAULT_WINE_EXECUTABLE);
            ui.cb_wine_prefix_detect->setChecked(CARLA_DEFAULT_WINE_AUTO_PREFIX);
            ui.le_wine_prefix_fallback->setText(CARLA_DEFAULT_WINE_FALLBACK_PREFIX);
            ui.group_wine_realtime->setChecked(CARLA_DEFAULT_WINE_RT_PRIO_ENABLED);
            ui.sb_wine_base_prio->setValue(CARLA_DEFAULT_WINE_BASE_RT_PRIO);
            ui.sb_wine_server_prio->setValue(CARLA_DEFAULT_WINE_SERVER_RT_PRIO);
            break;

        // ------------------------------------------------------------------------------------------------------------
        // Experimental

        case TAB_INDEX_EXPERIMENTAL:
            resetExperimentalSettings();
            break;
        }

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

CarlaSettingsW::CarlaSettingsW(QWidget* const parent, CarlaHost& host, const bool hasCanvas, const bool hasCanvasGL)
    : QDialog(parent),
      self(new PrivateData(this, host))
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
    connect(self->ui.lw_jsfx, SIGNAL(currentRowChanged(int)), SLOT(slot_pluginPathRowChanged(int)));

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
    self->ui.lw_jsfx->setCurrentRow(0);

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
    self->saveSettings();
}

void CarlaSettingsW::slot_resetSettings()
{
    self->resetSettings();
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_enableExperimental(const bool toggled)
{
    if (toggled)
    {
        self->ui.lw_page->showRow(TAB_INDEX_EXPERIMENTAL);
        if (self->ui.ch_exp_wine_bridges->isChecked() and not self->host.isControl)
            self->ui.lw_page->showRow(TAB_INDEX_WINE);
    }
    else
    {
        self->ui.lw_page->hideRow(TAB_INDEX_EXPERIMENTAL);
        self->ui.lw_page->hideRow(TAB_INDEX_WINE);
    }
}

void CarlaSettingsW::slot_enableWineBridges(const bool toggled)
{
    if (toggled && ! self->host.isControl)
        self->ui.lw_page->showRow(TAB_INDEX_WINE);
    else
        self->ui.lw_page->hideRow(TAB_INDEX_WINE);
}

void CarlaSettingsW::slot_pluginBridgesToggled(const bool toggled)
{
    if (! toggled)
    {
        self->ui.ch_exp_wine_bridges->setChecked(false);
        self->ui.ch_engine_prefer_plugin_bridges->setChecked(false);
        self->ui.lw_page->hideRow(TAB_INDEX_WINE);
    }
}

void CarlaSettingsW::slot_canvasEyeCandyToggled(const bool toggled)
{
    if (! toggled)
    {
        // disable fancy eyecandy too
        self->ui.cb_canvas_fancy_eyecandy->setChecked(false);
    }
}

void CarlaSettingsW::slot_canvasFancyEyeCandyToggled(const bool toggled)
{
    if (! toggled)
    {
        // make sure normal eyecandy is enabled too
        self->ui.cb_canvas_eyecandy->setChecked(true);
    }
}

void CarlaSettingsW::slot_canvasOpenGLToggled(const bool toggled)
{
    if (! toggled)
    {
        // uncheck GL specific option
        self->ui.cb_canvas_render_hq_aa->setChecked(false);
    }
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
    const QString newPath = QFileDialog::getExistingDirectory(this, tr("Add Path"), "", QFileDialog::ShowDirsOnly);

    if (newPath.isEmpty())
        return;

    switch (self->ui.tw_paths->currentIndex())
    {
    case PLUGINPATH_INDEX_LADSPA:
        self->ui.lw_ladspa->addItem(newPath);
        break;
    case PLUGINPATH_INDEX_DSSI:
        self->ui.lw_dssi->addItem(newPath);
        break;
    case PLUGINPATH_INDEX_LV2:
        self->ui.lw_lv2->addItem(newPath);
        break;
    case PLUGINPATH_INDEX_VST2:
        self->ui.lw_vst->addItem(newPath);
        break;
    case PLUGINPATH_INDEX_VST3:
        self->ui.lw_vst3->addItem(newPath);
        break;
    case PLUGINPATH_INDEX_SF2:
        self->ui.lw_sf2->addItem(newPath);
        break;
    case PLUGINPATH_INDEX_SFZ:
        self->ui.lw_sfz->addItem(newPath);
        break;
    case PLUGINPATH_INDEX_JSFX:
        self->ui.lw_jsfx->addItem(newPath);
        break;
    }
}

void CarlaSettingsW::slot_removePluginPath()
{
    switch (self->ui.tw_paths->currentIndex())
    {
    case PLUGINPATH_INDEX_LADSPA:
        self->ui.lw_ladspa->takeItem(self->ui.lw_ladspa->currentRow());
        break;
    case PLUGINPATH_INDEX_DSSI:
        self->ui.lw_dssi->takeItem(self->ui.lw_dssi->currentRow());
        break;
    case PLUGINPATH_INDEX_LV2:
        self->ui.lw_lv2->takeItem(self->ui.lw_lv2->currentRow());
        break;
    case PLUGINPATH_INDEX_VST2:
        self->ui.lw_vst->takeItem(self->ui.lw_vst->currentRow());
        break;
    case PLUGINPATH_INDEX_VST3:
        self->ui.lw_vst3->takeItem(self->ui.lw_vst3->currentRow());
        break;
    case PLUGINPATH_INDEX_SF2:
        self->ui.lw_sf2->takeItem(self->ui.lw_sf2->currentRow());
        break;
    case PLUGINPATH_INDEX_SFZ:
        self->ui.lw_sfz->takeItem(self->ui.lw_sfz->currentRow());
        break;
    case PLUGINPATH_INDEX_JSFX:
        self->ui.lw_jsfx->takeItem(self->ui.lw_jsfx->currentRow());
        break;
    }
}

void CarlaSettingsW::slot_changePluginPath()
{
    const int curIndex = self->ui.tw_paths->currentIndex();

    QString currentPath;

    switch (curIndex)
    {
    case PLUGINPATH_INDEX_LADSPA:
        currentPath = self->ui.lw_ladspa->currentItem()->text();
        break;
    case PLUGINPATH_INDEX_DSSI:
        currentPath = self->ui.lw_dssi->currentItem()->text();
        break;
    case PLUGINPATH_INDEX_LV2:
        currentPath = self->ui.lw_lv2->currentItem()->text();
        break;
    case PLUGINPATH_INDEX_VST2:
        currentPath = self->ui.lw_vst->currentItem()->text();
        break;
    case PLUGINPATH_INDEX_VST3:
        currentPath = self->ui.lw_vst3->currentItem()->text();
        break;
    case PLUGINPATH_INDEX_SF2:
        currentPath = self->ui.lw_sf2->currentItem()->text();
        break;
    case PLUGINPATH_INDEX_SFZ:
        currentPath = self->ui.lw_sfz->currentItem()->text();
        break;
    case PLUGINPATH_INDEX_JSFX:
        currentPath = self->ui.lw_jsfx->currentItem()->text();
        break;
    }

    const QString newPath = QFileDialog::getExistingDirectory(this, tr("Add Path"), currentPath, QFileDialog::ShowDirsOnly);

    if (newPath.isEmpty())
        return;

    switch (curIndex)
    {
    case PLUGINPATH_INDEX_LADSPA:
        self->ui.lw_ladspa->currentItem()->setText(newPath);
        break;
    case PLUGINPATH_INDEX_DSSI:
        self->ui.lw_dssi->currentItem()->setText(newPath);
        break;
    case PLUGINPATH_INDEX_LV2:
        self->ui.lw_lv2->currentItem()->setText(newPath);
        break;
    case PLUGINPATH_INDEX_VST2:
        self->ui.lw_vst->currentItem()->setText(newPath);
        break;
    case PLUGINPATH_INDEX_VST3:
        self->ui.lw_vst3->currentItem()->setText(newPath);
        break;
    case PLUGINPATH_INDEX_SF2:
        self->ui.lw_sf2->currentItem()->setText(newPath);
        break;
    case PLUGINPATH_INDEX_SFZ:
        self->ui.lw_sfz->currentItem()->setText(newPath);
        break;
    case PLUGINPATH_INDEX_JSFX:
        self->ui.lw_jsfx->currentItem()->setText(newPath);
        break;
    }
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_pluginPathTabChanged(const int index)
{
    int row;

    switch (index)
    {
    case PLUGINPATH_INDEX_LADSPA:
        row = self->ui.lw_ladspa->currentRow();
        break;
    case PLUGINPATH_INDEX_DSSI:
        row = self->ui.lw_dssi->currentRow();
        break;
    case PLUGINPATH_INDEX_LV2:
        row = self->ui.lw_lv2->currentRow();
        break;
    case PLUGINPATH_INDEX_VST2:
        row = self->ui.lw_vst->currentRow();
        break;
    case PLUGINPATH_INDEX_VST3:
        row = self->ui.lw_vst3->currentRow();
        break;
    case PLUGINPATH_INDEX_SF2:
        row = self->ui.lw_sf2->currentRow();
        break;
    case PLUGINPATH_INDEX_SFZ:
        row = self->ui.lw_sfz->currentRow();
        break;
    case PLUGINPATH_INDEX_JSFX:
        row = self->ui.lw_jsfx->currentRow();
        break;
    default:
        row = -1;
        break;
    }

    slot_pluginPathRowChanged(row);
}

void CarlaSettingsW::slot_pluginPathRowChanged(const int row)
{
    const bool check = bool(row >= 0);
    self->ui.b_paths_remove->setEnabled(check);
    self->ui.b_paths_change->setEnabled(check);
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_addFilePath()
{
    const QString newPath = QFileDialog::getExistingDirectory(this, tr("Add Path"), "", QFileDialog::ShowDirsOnly);

    if (newPath.isEmpty())
        return;

    switch (self->ui.tw_filepaths->currentIndex())
    {
    case FILEPATH_INDEX_AUDIO:
        self->ui.lw_files_audio->addItem(newPath);
        break;
    case FILEPATH_INDEX_MIDI:
        self->ui.lw_files_midi->addItem(newPath);
        break;
    }
}

void CarlaSettingsW::slot_removeFilePath()
{
    switch (self->ui.tw_filepaths->currentIndex())
    {
    case FILEPATH_INDEX_AUDIO:
        self->ui.lw_files_audio->takeItem(self->ui.lw_files_audio->currentRow());
        break;
    case FILEPATH_INDEX_MIDI:
        self->ui.lw_files_midi->takeItem(self->ui.lw_files_midi->currentRow());
        break;
    }
}

void CarlaSettingsW::slot_changeFilePath()
{
    const int curIndex = self->ui.tw_filepaths->currentIndex();

    QString currentPath;

    switch (curIndex)
    {
    case FILEPATH_INDEX_AUDIO:
        currentPath = self->ui.lw_files_audio->currentItem()->text();
        break;
    case FILEPATH_INDEX_MIDI:
        currentPath = self->ui.lw_files_midi->currentItem()->text();
        break;
    }

    const QString newPath = QFileDialog::getExistingDirectory(this, tr("Add Path"), currentPath, QFileDialog::ShowDirsOnly);

    if (newPath.isEmpty())
        return;

    switch (curIndex)
    {
    case FILEPATH_INDEX_AUDIO:
        self->ui.lw_files_audio->currentItem()->setText(newPath);
        break;
    case FILEPATH_INDEX_MIDI:
        self->ui.lw_files_midi->currentItem()->setText(newPath);
        break;
    }
}

// --------------------------------------------------------------------------------------------------------------------

void CarlaSettingsW::slot_filePathTabChanged(const int index)
{
    int row;

    switch (index)
    {
    case FILEPATH_INDEX_AUDIO:
        row = self->ui.lw_files_audio->currentRow();
        break;
    case FILEPATH_INDEX_MIDI:
        row = self->ui.lw_files_midi->currentRow();
        break;
    default:
        row = -1;
        break;
    }

    slot_filePathRowChanged(row);
}

void CarlaSettingsW::slot_filePathRowChanged(const int row)
{
    const bool check = bool(row >= 0);
    self->ui.b_filepaths_remove->setEnabled(check);
    self->ui.b_filepaths_change->setEnabled(check);
}

// --------------------------------------------------------------------------------------------------------------------
