// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aboutdialog.hpp"

#include "CarlaHost.h"

// --------------------------------------------------------------------------------------------------------------------
// About Dialog

AboutDialog::AboutDialog(QWidget* const parent,
                         const CarlaHostHandle hostHandle,
                         const bool isControl,
                         const bool isPlugin)
    : QDialog(parent)
{
    ui.setupUi(this);

    QString extraInfo;
    if (isControl)
        extraInfo = QString(" - <b>%1</b>").arg(tr("OSC Bridge Version"));
    else if (isPlugin)
        extraInfo = QString(" - <b>%1</b>").arg(tr("Plugin Version"));

    ui.l_about->setText(tr(""
                           "<br>Version %1"
                           "<br>Carla is a fully-featured audio plugin host%2.<br>"
                           "<br>Copyright (C) 2011-2025 falkTX<br>"
                           "").arg(CARLA_VERSION_STRING).arg(extraInfo));

    if (ui.about->palette().color(QPalette::Window).blackF() < 0.5)
    {
        ui.l_icons->setPixmap(QPixmap(":/bitmaps/carla_about_black.png"));
        ui.ico_example_edit->setPixmap(QPixmap(":/bitmaps/button_file-black.png"));
        ui.ico_example_file->setPixmap(QPixmap(":/scalable/button_edit-black.svg"));
        ui.ico_example_gui->setPixmap(QPixmap(":/bitmaps/button_gui-black.png"));
    }

    if (isControl || isPlugin)
    {
        ui.l_extended->hide();
        ui.tabWidget->removeTab(3);
        ui.tabWidget->removeTab(2);
    }
   #ifndef STATIC_PLUGIN_TARGET
    else if (carla_is_engine_running(hostHandle))
    {
        ui.le_osc_url_tcp->setText(carla_get_host_osc_url_tcp(hostHandle));
        ui.le_osc_url_udp->setText(carla_get_host_osc_url_udp(hostHandle));
    }
   #endif
    else
    {
        ui.le_osc_url_tcp->setText(tr("(Engine not running)"));
        ui.le_osc_url_udp->setText(tr("(Engine not running)"));
    }

    ui.l_extended->setText(carla_get_complete_license_text());

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

    ui.l_vst2->setText(tr("About 85&#37; complete (missing vst bank/presets and some minor stuff)"));
    ui.l_vst3->setText(tr("About 66&#37; complete"));

#ifdef CARLA_OS_MAC
    ui.l_au->setText(tr("About 20&#37; complete"));
#else
    ui.line_vst3->hide();
    ui.l_au->hide();
    ui.lid_au->hide();
#endif

    // 3rd tab is usually longer than the 1st, adjust appropriately
    ui.tabWidget->setCurrentIndex(2);
    adjustSize();
    ui.tabWidget->setCurrentIndex(0);

    setFixedSize(size());

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
#ifdef CARLA_OS_WIN
    flags |= Qt::MSWindowsFixedSizeDialogHint;
#endif
    setWindowFlags(flags);

#ifdef CARLA_OS_MAC
    if (parent != nullptr)
        setWindowModality(Qt::WindowModal);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
