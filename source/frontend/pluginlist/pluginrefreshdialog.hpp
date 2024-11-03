/*
 * Carla plugin host
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "ui_pluginrefreshdialog.h"

#include "qsafesettings.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Plugin Refresh Dialog

struct PluginRefreshDialog : QDialog, Ui_PluginRefreshDialog {
    const bool _firstInit;

    explicit PluginRefreshDialog(QWidget* const parent, bool firstInit = false)
        : QDialog(parent),
          _firstInit(firstInit)
    {
        setupUi(this);

        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
       #ifdef CARLA_OS_MAC
        setWindowModality(Qt::WindowModal);
       #endif

        b_skip->setEnabled(false);
        ch_invalid->setEnabled(false);

        if (_firstInit)
            return;

        // ------------------------------------------------------------------------------------------------------------
        // Load settings

        {
            const QSafeSettings settings;

            restoreGeometry(settings.valueByteArray("PluginRefreshDialog/Geometry"));

            if (settings.valueBool("PluginRefreshDialog/RefreshAll", false))
                ch_all->setChecked(true);
            else
                ch_updated->setChecked(true);

            ch_invalid->setChecked(settings.valueBool("PluginRefreshDialog/CheckInvalid", false));

            group_formats->setChecked(settings.valueBool("PluginRefreshDialog/RestrictFormats", false));
            ch_ladspa->setChecked(settings.valueBool("PluginRefreshDialog/SearchLADSPA", true));
            ch_dssi->setChecked(settings.valueBool("PluginRefreshDialog/SearchDSSI", true));
            ch_lv2->setChecked(settings.valueBool("PluginRefreshDialog/SearchLV2", true));
            ch_vst2->setChecked(settings.valueBool("PluginRefreshDialog/SearchVST2", true));
            ch_vst3->setChecked(settings.valueBool("PluginRefreshDialog/SearchVST3", true));
            ch_clap->setChecked(settings.valueBool("PluginRefreshDialog/SearchCLAP", true));
            ch_au->setChecked(settings.valueBool("PluginRefreshDialog/SearchAU", true));
            ch_jsfx->setChecked(settings.valueBool("PluginRefreshDialog/SearchJSFX", true));
            ch_sf2->setChecked(settings.valueBool("PluginRefreshDialog/SearchSF2", true));
            ch_sfz->setChecked(settings.valueBool("PluginRefreshDialog/SearchSFZ", true));
        }

        // ------------------------------------------------------------------------------------------------------------
        // Set-up connections

        QObject::connect(this, &QDialog::finished, this, &PluginRefreshDialog::saveSettings);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // private slots

private Q_SLOTS:
    void saveSettings()
    {
        if (_firstInit)
            return;

        QSafeSettings settings;
        settings.setValue("PluginRefreshDialog/Geometry", saveGeometry());
        settings.setValue("PluginRefreshDialog/RefreshAll", ch_all->isChecked());
        settings.setValue("PluginRefreshDialog/CheckInvalid", ch_invalid->isChecked());
        settings.setValue("PluginRefreshDialog/RestrictFormats", group_formats->isChecked());
        settings.setValue("PluginRefreshDialog/SearchLADSPA", ch_ladspa->isChecked());
        settings.setValue("PluginRefreshDialog/SearchDSSI", ch_dssi->isChecked());
        settings.setValue("PluginRefreshDialog/SearchLV2", ch_lv2->isChecked());
        settings.setValue("PluginRefreshDialog/SearchVST2", ch_vst2->isChecked());
        settings.setValue("PluginRefreshDialog/SearchVST3", ch_vst3->isChecked());
        settings.setValue("PluginRefreshDialog/SearchCLAP", ch_clap->isChecked());
        settings.setValue("PluginRefreshDialog/SearchAU", ch_au->isChecked());
        settings.setValue("PluginRefreshDialog/SearchJSFX", ch_jsfx->isChecked());
        settings.setValue("PluginRefreshDialog/SearchSF2", ch_sf2->isChecked());
        settings.setValue("PluginRefreshDialog/SearchSFZ", ch_sfz->isChecked());
    }
};

// --------------------------------------------------------------------------------------------------------------------
