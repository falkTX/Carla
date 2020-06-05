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

#include "carla_widgets.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

//---------------------------------------------------------------------------------------------------------------------

#include <QtWidgets/QDialog>

//---------------------------------------------------------------------------------------------------------------------

#include "ui_carla_about.hpp"
#include "ui_carla_about_juce.hpp"
#include "ui_carla_parameter.hpp"

//---------------------------------------------------------------------------------------------------------------------

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_host.hpp"

#include "CarlaHost.h"
#include "CarlaUtils.h"

// --------------------------------------------------------------------------------------------------------------------
// Carla About dialog

struct CarlaAboutW::PrivateData {
    Ui::CarlaAboutW ui;

    PrivateData(CarlaAboutW* const aboutWindow, const CarlaHost& host)
        : ui()
    {
        ui.setupUi(aboutWindow);

        QString extraInfo;
        if (host.isControl)
            extraInfo = QString(" - <b>%1</b>").arg(tr("OSC Bridge Version"));
        else if (host.isPlugin)
            extraInfo = QString(" - <b>%1</b>").arg(tr("Plugin Version"));

        ui.l_about->setText(tr("<br>Version %1"
                              "<br>Carla is a fully-featured audio plugin host%2.<br>"
                              "<br>Copyright (C) 2011-2019 falkTX<br>"
                              "").arg(CARLA_VERSION_STRING).arg(extraInfo));

        if (ui.about->palette().color(QPalette::Background).blackF() < 0.5)
        {
            ui.l_icons->setPixmap(QPixmap(":/bitmaps/carla_about_black.png"));
            ui.ico_example_edit->setPixmap(QPixmap(":/bitmaps/button_file-black.png"));
            ui.ico_example_file->setPixmap(QPixmap(":/bitmaps/button_edit-black.png"));
            ui.ico_example_gui->setPixmap(QPixmap(":/bitmaps/button_gui-black.png"));
        }

        if (host.isControl)
        {
            ui.l_extended->hide();
            ui.tabWidget->removeTab(3);
            ui.tabWidget->removeTab(2);
        }

        ui.l_extended->setText(carla_get_complete_license_text());

        if (carla_is_engine_running(host.handle) && ! host.isControl)
        {
            ui.le_osc_url_tcp->setText(carla_get_host_osc_url_tcp(host.handle));
            ui.le_osc_url_udp->setText(carla_get_host_osc_url_udp(host.handle));
        }
        else
        {
            ui.le_osc_url_tcp->setText(tr("(Engine not running)"));
            ui.le_osc_url_udp->setText(tr("(Engine not running)"));
        }

        ui.l_osc_cmds->setText("<table>"
                               "<tr><td>" "/set_active"                 "&nbsp;</td><td>&lt;i-value&gt;</td></tr>"
                               "<tr><td>" "/set_drywet"                 "&nbsp;</td><td>&lt;f-value&gt;</td></tr>"
                               "<tr><td>" "/set_volume"                 "&nbsp;</td><td>&lt;f-value&gt;</td></tr>"
                               "<tr><td>" "/set_balance_left"           "&nbsp;</td><td>&lt;f-value&gt;</td></tr>"
                               "<tr><td>" "/set_balance_right"          "&nbsp;</td><td>&lt;f-value&gt;</td></tr>"
                               "<tr><td>" "/set_panning"                "&nbsp;</td><td>&lt;f-value&gt;</td></tr>"
                               "<tr><td>" "/set_parameter_value"        "&nbsp;</td><td>&lt;i-index&gt; &lt;f-value&gt;</td></tr>"
                               "<tr><td>" "/set_parameter_midi_cc"      "&nbsp;</td><td>&lt;i-index&gt; &lt;i-cc&gt;</td></tr>"
                               "<tr><td>" "/set_parameter_midi_channel" "&nbsp;</td><td>&lt;i-index&gt; &lt;i-channel&gt;</td></tr>"
                               "<tr><td>" "/set_program"                "&nbsp;</td><td>&lt;i-index&gt;</td></tr>"
                               "<tr><td>" "/set_midi_program"           "&nbsp;</td><td>&lt;i-index&gt;</td></tr>"
                               "<tr><td>" "/note_on"                    "&nbsp;</td><td>&lt;i-channel&gt; &lt;i-note&gt; &lt;i-velo&gt;</td></tr>"
                               "<tr><td>" "/note_off"                   "&nbsp;</td><td>&lt;i-channel&gt; &lt;i-note</td></tr>"
                               "</table>");

        ui.l_example->setText("/Carla/2/set_parameter_value 5 1.0");
        ui.l_example_help->setText("<i>(as in this example, \"2\" is the plugin number and \"5\" the parameter)</i>");

        ui.l_ladspa->setText(tr("Everything! (Including LRDF)"));
        ui.l_dssi->setText(tr("Everything! (Including CustomData/Chunks)"));
        ui.l_lv2->setText(tr("About 110&#37; complete (using custom extensions)<br/>"
                                      "Implemented Feature/Extensions:"
                                      "<ul>"
                                      "<li>http://lv2plug.in/ns/ext/atom</li>"
                                      "<li>http://lv2plug.in/ns/ext/buf-size</li>"
                                      "<li>http://lv2plug.in/ns/ext/data-access</li>"
                                      // "<li>http://lv2plug.in/ns/ext/dynmanifest</li>"
                                      "<li>http://lv2plug.in/ns/ext/event</li>"
                                      "<li>http://lv2plug.in/ns/ext/instance-access</li>"
                                      "<li>http://lv2plug.in/ns/ext/log</li>"
                                      "<li>http://lv2plug.in/ns/ext/midi</li>"
                                      // "<li>http://lv2plug.in/ns/ext/morph</li>"
                                      "<li>http://lv2plug.in/ns/ext/options</li>"
                                      "<li>http://lv2plug.in/ns/ext/parameters</li>"
                                      // "<li>http://lv2plug.in/ns/ext/patch</li>"
                                      // "<li>http://lv2plug.in/ns/ext/port-groups</li>"
                                      "<li>http://lv2plug.in/ns/ext/port-props</li>"
                                      "<li>http://lv2plug.in/ns/ext/presets</li>"
                                      "<li>http://lv2plug.in/ns/ext/resize-port</li>"
                                      "<li>http://lv2plug.in/ns/ext/state</li>"
                                      "<li>http://lv2plug.in/ns/ext/time</li>"
                                      "<li>http://lv2plug.in/ns/ext/uri-map</li>"
                                      "<li>http://lv2plug.in/ns/ext/urid</li>"
                                      "<li>http://lv2plug.in/ns/ext/worker</li>"
                                      "<li>http://lv2plug.in/ns/extensions/ui</li>"
                                      "<li>http://lv2plug.in/ns/extensions/units</li>"
                                      "<li>http://home.gna.org/lv2dynparam/rtmempool/v1</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/external-ui</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/programs</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/props</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/rtmempool</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/midimap</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/miditype</li>"
                                      "</ul>"));

#ifdef USING_JUCE
        ui.l_vst2->setText(tr("Using Juce host"));
        ui.l_vst3->setText(tr("Using Juce host"));
#else
        ui.l_vst2->setText(tr("About 85% complete (missing vst bank/presets and some minor stuff)"));
        ui.line_vst2->hide();
        ui.l_vst3->hide();
        ui.lid_vst3->hide();
#endif

#ifdef CARLA_OS_MAC
        ui.l_au->setText(tr("Using Juce host"));
#else
        ui.line_vst3->hide();
        ui.l_au->hide();
        ui.lid_au->hide();
#endif

        // 3rd tab is usually longer than the 1st, adjust appropriately
        ui.tabWidget->setCurrentIndex(2);
        aboutWindow->adjustSize();
        ui.tabWidget->setCurrentIndex(0);
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

CarlaAboutW::CarlaAboutW(QWidget* parent, const CarlaHost& host)
    : QDialog(parent),
      self(new PrivateData(this, host))
{
    setFixedSize(size());

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
#ifdef CARLA_OS_WIN
    flags |= Qt::MSWindowsFixedSizeDialogHint;
#endif
    setWindowFlags(flags);
}

CarlaAboutW::~CarlaAboutW()
{
    delete self;
}

// --------------------------------------------------------------------------------------------------------------------
// JUCE About dialog

struct JuceAboutW::PrivateData {
    Ui::JuceAboutW ui;

    PrivateData(JuceAboutW* const self)
        : ui()
    {
        ui.setupUi(self);

        ui.l_text2->setText(tr("This program uses JUCE version %1.").arg(carla_get_juce_version()));
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

JuceAboutW::JuceAboutW(QWidget* const parent)
    : QDialog(parent),
      self(new PrivateData(this))
{
    adjustSize();
    setFixedSize(size());

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
#ifdef CARLA_OS_WIN
    flags |= Qt::MSWindowsFixedSizeDialogHint;
#endif
    setWindowFlags(flags);
}

JuceAboutW::~JuceAboutW()
{
    delete self;
}

// --------------------------------------------------------------------------------------------------------------------
// Settings Dialog

struct PluginParameter::PrivateData {
    Ui::PluginParameter ui;

    PrivateData(PluginParameter* const self, const CarlaHost& /*host*/)
        : ui()
    {
        ui.setupUi(self);
    }

    /*
    const char* _textCallBack()
    {
        return carla_get_parameter_text(fPluginId, fParameterId);
    }

    void _valueCallBack(const float value)
    {
        parameterWidget->valueChanged.emit(fParameterId, value);
    }
    */

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

PluginParameter::PluginParameter(QWidget* const parent, const CarlaHost& host)
    : QWidget(parent),
      self(new PrivateData(this, host))
{
}

PluginParameter::~PluginParameter()
{
    delete self;
}

// --------------------------------------------------------------------------------------------------------------------

void PluginParameter::slot_optionsCustomMenu()
{
}

void PluginParameter::slot_parameterDragStateChanged(const bool /*touch*/)
{
}

//---------------------------------------------------------------------------------------------------------------------
