// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ApplicationWindow {
    id: win
    required property var controller
    property var draft: ({})
    property var drivers: []
    property string oscTcpUrl: ""
    property string oscUdpUrl: ""
    property var drvInfo: ({ devices: [], buffer_sizes: [64,128,256,512,1024,2048], sample_rates: [44100,48000,88200,96000], can_triple_buffer: false, has_control_panel: false })
    property int sectionIdx: 0
    property int pathCatIdx: 4 // LV2 default
    property int selectedPathIdx: -1

    readonly property var pathCategories: [
        { key: "paths_audio", defKey: "audio", label: "Audio Files" },
        { key: "paths_midi", defKey: "midi", label: "MIDI Files" },
        { key: "paths_ladspa", defKey: "ladspa", label: "LADSPA" },
        { key: "paths_dssi", defKey: "dssi", label: "DSSI" },
        { key: "paths_lv2", defKey: "lv2", label: "LV2" },
        { key: "paths_vst2", defKey: "vst2", label: "VST2" },
        { key: "paths_vst3", defKey: "vst3", label: "VST3" },
        { key: "paths_clap", defKey: "clap", label: "CLAP" },
        { key: "paths_sf2", defKey: "sf2", label: "SF2" },
        { key: "paths_sfz", defKey: "sfz", label: "SFZ" },
        { key: "paths_jsfx", defKey: "jsfx", label: "JSFX" }
    ]

    title: qsTr("Configure Franca")
    width: 780
    height: 540
    minimumWidth: 640
    minimumHeight: 440

    function openSettings() {
        try {
            var p = JSON.parse(controller.load_settings_draft_json());
            draft = p.settings || {};
            drivers = p.drivers || [];
            oscTcpUrl = p.osc_tcp_url || "";
            oscUdpUrl = p.osc_udp_url || "";
            refreshDriverDevices();
        } catch (e) {}
        sectionIdx = 0;
        show();
        raise();
        requestActivate();
    }

    function currentDriverCfg() {
        if (!draft.driver_settings) draft.driver_settings = {};
        var d = draft.audio_driver || "JACK";
        if (!draft.driver_settings[d]) {
            draft.driver_settings[d] = { device: "", buffer_size: 512, sample_rate: 44100, triple_buffer: false };
        }
        return draft.driver_settings[d];
    }

    function refreshDriverDevices() {
        var d = draft.audio_driver || "JACK";
        var cfg = currentDriverCfg();
        try {
            drvInfo = JSON.parse(controller.query_driver_devices_json(d, cfg.device || ""));
        } catch (e) {}
    }

    function mutateDraft(fn) {
        var copy = JSON.parse(JSON.stringify(draft));
        fn(copy);
        draft = copy;
    }

    function currentPathsList() {
        var k = pathCategories[pathCatIdx].key;
        return (draft && draft[k]) ? draft[k] : [];
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // Left sidebar navigation
            Controls.Frame {
                Layout.fillHeight: true
                Layout.preferredWidth: 150
                padding: 4
                ListView {
                    anchors.fill: parent
                    model: {
                        var base = [qsTr("Main"), qsTr("Engine"), qsTr("Canvas"), qsTr("Paths"), qsTr("OSC")];
                        if (win.draft.experimental) base.push(qsTr("Wine"), qsTr("Experimental"));
                        return base;
                    }
                    delegate: Controls.ItemDelegate {
                        required property string modelData
                        required property int index
                        width: ListView.view.width
                        text: modelData
                        highlighted: win.sectionIdx === index
                        onClicked: win.sectionIdx = index
                    }
                }
            }

            // Right section pages
            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                StackLayout {
                    width: parent.width - 12
                    currentIndex: win.sectionIdx

                    // 0: Main
                    ColumnLayout {
                        spacing: 8
                        Kirigami.Heading { text: qsTr("Main Settings"); level: 3 }
                        RowLayout {
                            Layout.fillWidth: true
                            Controls.Label { text: qsTr("Project Folder:") }
                            Controls.TextField {
                                Layout.fillWidth: true
                                text: win.draft.project_folder || ""
                                onTextEdited: win.mutateDraft(function(d) { d.project_folder = text; })
                            }
                            Controls.Button {
                                text: qsTr("Browse…")
                                onClicked: {
                                    var dir = win.controller.browse_folder(qsTr("Select Project Folder"));
                                    if (dir.length > 0) win.mutateDraft(function(d) { d.project_folder = dir; });
                                }
                            }
                        }
                        RowLayout {
                            Controls.Label { text: qsTr("UI Refresh Interval (ms):") }
                            Controls.SpinBox {
                                from: 10; to: 200
                                value: win.draft.refresh_interval_ms || 20
                                onValueModified: win.mutateDraft(function(d) { d.refresh_interval_ms = value; })
                            }
                        }
                        Controls.CheckBox {
                            text: qsTr("Confirm before closing when plugins are loaded")
                            checked: !!win.draft.confirm_exit
                            onToggled: win.mutateDraft(function(d) { d.confirm_exit = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Show Logs tab")
                            checked: !!win.draft.show_logs
                            onToggled: win.mutateDraft(function(d) { d.show_logs = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Enable experimental features (Wine, JACK Apps, Bridges, LV2 Export)")
                            checked: !!win.draft.experimental
                            onToggled: win.mutateDraft(function(d) { d.experimental = checked; })
                        }
                    }

                    // 1: Engine
                    ColumnLayout {
                        spacing: 8
                        Kirigami.Heading { text: qsTr("Audio Engine"); level: 3 }

                        RowLayout {
                            Controls.Label { text: qsTr("Audio Driver:") }
                            Controls.ComboBox {
                                model: win.drivers
                                currentIndex: Math.max(0, win.drivers.indexOf(win.draft.audio_driver || ""))
                                onActivated: function(idx) {
                                    var drv = win.drivers[idx];
                                    win.mutateDraft(function(d) { d.audio_driver = drv; });
                                    win.refreshDriverDevices();
                                }
                            }
                        }

                        Controls.GroupBox {
                            Layout.fillWidth: true
                            title: (win.draft.audio_driver || "Audio") + qsTr(" Device Settings")
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 6
                                RowLayout {
                                    visible: (win.drvInfo.devices || []).length > 0
                                    Controls.Label { text: qsTr("Device:") }
                                    Controls.ComboBox {
                                        Layout.fillWidth: true
                                        model: [qsTr("(Default)")].concat(win.drvInfo.devices || [])
                                        currentIndex: {
                                            var dev = win.currentDriverCfg().device || "";
                                            var idx = (win.drvInfo.devices || []).indexOf(dev);
                                            return idx >= 0 ? idx + 1 : 0;
                                        }
                                        onActivated: function(i) {
                                            var dev = i === 0 ? "" : win.drvInfo.devices[i - 1];
                                            win.mutateDraft(function(d) {
                                                if (!d.driver_settings) d.driver_settings = {};
                                                var drv = d.audio_driver || "JACK";
                                                if (!d.driver_settings[drv]) d.driver_settings[drv] = { device: "", buffer_size: 512, sample_rate: 44100, triple_buffer: false };
                                                d.driver_settings[drv].device = dev;
                                            });
                                            win.refreshDriverDevices();
                                        }
                                    }
                                }
                                RowLayout {
                                    spacing: 10
                                    Controls.Label { text: qsTr("Buffer Size:") }
                                    Controls.ComboBox {
                                        model: win.drvInfo.buffer_sizes || [512]
                                        currentIndex: Math.max(0, (win.drvInfo.buffer_sizes || []).indexOf(win.currentDriverCfg().buffer_size))
                                        onActivated: function(i) {
                                            var v = win.drvInfo.buffer_sizes[i];
                                            win.mutateDraft(function(d) {
                                                var drv = d.audio_driver || "JACK";
                                                if (!d.driver_settings) d.driver_settings = {};
                                                if (!d.driver_settings[drv]) d.driver_settings[drv] = { device: "", buffer_size: 512, sample_rate: 44100, triple_buffer: false };
                                                d.driver_settings[drv].buffer_size = v;
                                            });
                                        }
                                    }
                                    Controls.Label { text: qsTr("Sample Rate:") }
                                    Controls.ComboBox {
                                        model: (win.drvInfo.sample_rates || [44100]).map(function(r) { return r + " Hz"; })
                                        currentIndex: Math.max(0, (win.drvInfo.sample_rates || []).indexOf(win.currentDriverCfg().sample_rate))
                                        onActivated: function(i) {
                                            var v = win.drvInfo.sample_rates[i];
                                            win.mutateDraft(function(d) {
                                                var drv = d.audio_driver || "JACK";
                                                if (!d.driver_settings) d.driver_settings = {};
                                                if (!d.driver_settings[drv]) d.driver_settings[drv] = { device: "", buffer_size: 512, sample_rate: 44100, triple_buffer: false };
                                                d.driver_settings[drv].sample_rate = v;
                                            });
                                        }
                                    }
                                }
                                Controls.CheckBox {
                                    visible: !!win.drvInfo.can_triple_buffer
                                    text: qsTr("Triple buffering")
                                    checked: !!win.currentDriverCfg().triple_buffer
                                    onToggled: win.mutateDraft(function(d) {
                                        var drv = d.audio_driver || "JACK";
                                        if (!d.driver_settings) d.driver_settings = {};
                                        if (!d.driver_settings[drv]) d.driver_settings[drv] = { device: "", buffer_size: 512, sample_rate: 44100, triple_buffer: false };
                                        d.driver_settings[drv].triple_buffer = checked;
                                    })
                                }
                                Controls.Button {
                                    visible: !!win.drvInfo.has_control_panel
                                    text: qsTr("Open Device Control Panel")
                                    onClicked: win.controller.open_driver_control_panel(win.draft.audio_driver || "", win.currentDriverCfg().device || "")
                                }
                            }
                        }

                        RowLayout {
                            Controls.Label { text: qsTr("Process Mode:") }
                            Controls.ComboBox {
                                model: ["Single Client", "Multiple Clients", "Continuous Rack", "Patchbay"]
                                currentIndex: Math.max(0, Math.min(3, win.draft.process_mode !== undefined ? win.draft.process_mode : 2))
                                onActivated: function(i) { win.mutateDraft(function(d) { d.process_mode = i; }); }
                            }
                        }

                        RowLayout {
                            Controls.Label { text: qsTr("Transport Mode:") }
                            Controls.ComboBox {
                                model: ["Disabled", "Internal", "JACK"]
                                currentIndex: Math.max(0, Math.min(2, win.draft.transport_mode !== undefined ? win.draft.transport_mode : 1))
                                onActivated: function(i) { win.mutateDraft(function(d) { d.transport_mode = i; }); }
                            }
                        }

                        Controls.CheckBox {
                            text: qsTr("Force stereo plugins")
                            checked: !!win.draft.force_stereo
                            onToggled: win.mutateDraft(function(d) { d.force_stereo = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Prefer UI bridges")
                            checked: !!win.draft.prefer_ui_bridges
                            onToggled: win.mutateDraft(function(d) { d.prefer_ui_bridges = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Manage plugin UIs")
                            checked: !!win.draft.manage_uis
                            onToggled: win.mutateDraft(function(d) { d.manage_uis = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Plugin UIs always on top")
                            checked: !!win.draft.uis_always_on_top
                            onToggled: win.mutateDraft(function(d) { d.uis_always_on_top = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Reset xruns on transport start")
                            checked: !!win.draft.reset_xruns
                            onToggled: win.mutateDraft(function(d) { d.reset_xruns = checked; })
                        }
                        RowLayout {
                            Controls.Label { text: qsTr("Max parameters per plugin:") }
                            Controls.SpinBox {
                                from: 16; to: 4096
                                value: win.draft.max_parameters || 200
                                onValueModified: win.mutateDraft(function(d) { d.max_parameters = value; })
                            }
                        }
                        RowLayout {
                            Controls.Label { text: qsTr("UI bridges timeout (ms):") }
                            Controls.SpinBox {
                                from: 500; to: 30000; stepSize: 500
                                value: win.draft.ui_bridges_timeout || 4000
                                onValueModified: win.mutateDraft(function(d) { d.ui_bridges_timeout = value; })
                            }
                        }
                    }

                    // 2: Canvas
                    ColumnLayout {
                        spacing: 8
                        Kirigami.Heading { text: qsTr("Patchbay Canvas"); level: 3 }
                        RowLayout {
                            Controls.Label { text: qsTr("Canvas Size:") }
                            Controls.SpinBox {
                                from: 1000; to: 10000; stepSize: 100
                                value: win.draft.canvas_width || 3100
                                onValueModified: win.mutateDraft(function(d) { d.canvas_width = value; })
                            }
                            Controls.Label { text: "×" }
                            Controls.SpinBox {
                                from: 800; to: 10000; stepSize: 100
                                value: win.draft.canvas_height || 2400
                                onValueModified: win.mutateDraft(function(d) { d.canvas_height = value; })
                            }
                        }
                        Controls.CheckBox {
                            text: qsTr("Use curved Bezier cables")
                            checked: !!win.draft.canvas_use_bezier
                            onToggled: win.mutateDraft(function(d) { d.canvas_use_bezier = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Auto-hide empty groups")
                            checked: !!win.draft.canvas_auto_hide_groups
                            onToggled: win.mutateDraft(function(d) { d.canvas_auto_hide_groups = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Auto-select items")
                            checked: !!win.draft.canvas_auto_select_items
                            onToggled: win.mutateDraft(function(d) { d.canvas_auto_select_items = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Eye-candy rendering")
                            checked: !!win.draft.canvas_eye_candy
                            onToggled: win.mutateDraft(function(d) { d.canvas_eye_candy = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Show inline displays")
                            checked: !!win.draft.canvas_inline_displays
                            onToggled: win.mutateDraft(function(d) { d.canvas_inline_displays = checked; })
                        }
                    }

                    // 3: Paths
                    ColumnLayout {
                        spacing: 8
                        Kirigami.Heading { text: qsTr("Plugin & File Paths"); level: 3 }
                        RowLayout {
                            Controls.Label { text: qsTr("Category:") }
                            Controls.ComboBox {
                                model: win.pathCategories
                                textRole: "label"
                                currentIndex: win.pathCatIdx
                                onActivated: function(i) { win.pathCatIdx = i; win.selectedPathIdx = -1; }
                            }
                        }
                        Controls.Frame {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 230
                            padding: 0
                            Controls.ScrollView {
                                anchors.fill: parent
                                clip: true
                                ListView {
                                    model: win.currentPathsList()
                                    delegate: Controls.ItemDelegate {
                                        required property string modelData
                                        required property int index
                                        width: ListView.view.width
                                        text: modelData
                                        highlighted: win.selectedPathIdx === index
                                        onClicked: win.selectedPathIdx = index
                                    }
                                }
                            }
                        }
                        RowLayout {
                            Controls.Button {
                                text: qsTr("Add Folder…")
                                icon.name: "list-add"
                                onClicked: {
                                    var dir = win.controller.browse_folder(qsTr("Add Search Folder"));
                                    if (dir.length > 0) {
                                        var k = win.pathCategories[win.pathCatIdx].key;
                                        win.mutateDraft(function(d) {
                                            if (!d[k]) d[k] = [];
                                            if (d[k].indexOf(dir) < 0) d[k].push(dir);
                                        });
                                    }
                                }
                            }
                            Controls.Button {
                                text: qsTr("Remove")
                                icon.name: "list-remove"
                                enabled: win.selectedPathIdx >= 0
                                onClicked: {
                                    var k = win.pathCategories[win.pathCatIdx].key;
                                    var idx = win.selectedPathIdx;
                                    win.mutateDraft(function(d) {
                                        if (d[k] && idx >= 0 && idx < d[k].length) d[k].splice(idx, 1);
                                    });
                                    win.selectedPathIdx = -1;
                                }
                            }
                            Controls.Button {
                                text: qsTr("Reset Defaults")
                                onClicked: {
                                    var cat = win.pathCategories[win.pathCatIdx];
                                    var defs = JSON.parse(win.controller.default_paths_json(cat.defKey));
                                    win.mutateDraft(function(d) { d[cat.key] = defs; });
                                    win.selectedPathIdx = -1;
                                }
                            }
                        }
                    }

                    // 4: OSC
                    ColumnLayout {
                        spacing: 8
                        Kirigami.Heading { text: qsTr("OSC (Open Sound Control)"); level: 3 }
                        Controls.CheckBox {
                            text: qsTr("Enable OSC server")
                            checked: !!win.draft.osc_enabled
                            onToggled: win.mutateDraft(function(d) { d.osc_enabled = checked; })
                        }
                        ColumnLayout {
                            enabled: !!win.draft.osc_enabled
                            RowLayout {
                                Controls.CheckBox {
                                    text: qsTr("TCP Port:")
                                    checked: !!win.draft.osc_tcp_enabled
                                    onToggled: win.mutateDraft(function(d) { d.osc_tcp_enabled = checked; })
                                }
                                Controls.CheckBox {
                                    text: qsTr("Random")
                                    checked: !!win.draft.osc_tcp_random
                                    onToggled: win.mutateDraft(function(d) { d.osc_tcp_random = checked; })
                                }
                                Controls.SpinBox {
                                    enabled: !!win.draft.osc_tcp_enabled && !win.draft.osc_tcp_random
                                    from: 1024; to: 65535
                                    value: win.draft.osc_tcp_port || 22752
                                    onValueModified: win.mutateDraft(function(d) { d.osc_tcp_port = value; })
                                }
                            }
                            RowLayout {
                                Controls.CheckBox {
                                    text: qsTr("UDP Port:")
                                    checked: !!win.draft.osc_udp_enabled
                                    onToggled: win.mutateDraft(function(d) { d.osc_udp_enabled = checked; })
                                }
                                Controls.CheckBox {
                                    text: qsTr("Random")
                                    checked: !!win.draft.osc_udp_random
                                    onToggled: win.mutateDraft(function(d) { d.osc_udp_random = checked; })
                                }
                                Controls.SpinBox {
                                    enabled: !!win.draft.osc_udp_enabled && !win.draft.osc_udp_random
                                    from: 1024; to: 65535
                                    value: win.draft.osc_udp_port || 22752
                                    onValueModified: win.mutateDraft(function(d) { d.osc_udp_port = value; })
                                }
                            }
                        }
                        Controls.Label {
                            visible: win.oscTcpUrl.length > 0
                            text: "TCP: " + win.oscTcpUrl
                            font.family: "monospace"
                        }
                        Controls.Label {
                            visible: win.oscUdpUrl.length > 0
                            text: "UDP: " + win.oscUdpUrl
                            font.family: "monospace"
                        }
                    }

                    // 5: Wine
                    ColumnLayout {
                        spacing: 8
                        Kirigami.Heading { text: qsTr("Wine Bridge Settings"); level: 3 }
                        RowLayout {
                            Layout.fillWidth: true
                            Controls.Label { text: qsTr("Wine Executable:") }
                            Controls.TextField {
                                Layout.fillWidth: true
                                text: win.draft.wine_executable || "wine"
                                onTextEdited: win.mutateDraft(function(d) { d.wine_executable = text; })
                            }
                        }
                        Controls.CheckBox {
                            text: qsTr("Auto-detect Wine prefix")
                            checked: !!win.draft.wine_auto_prefix
                            onToggled: win.mutateDraft(function(d) { d.wine_auto_prefix = checked; })
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Controls.Label { text: qsTr("Fallback Prefix:") }
                            Controls.TextField {
                                Layout.fillWidth: true
                                text: win.draft.wine_fallback_prefix || ""
                                onTextEdited: win.mutateDraft(function(d) { d.wine_fallback_prefix = text; })
                            }
                        }
                        Controls.CheckBox {
                            text: qsTr("Enable Wine RT priority")
                            checked: !!win.draft.wine_rt_prio_enabled
                            onToggled: win.mutateDraft(function(d) { d.wine_rt_prio_enabled = checked; })
                        }
                        RowLayout {
                            enabled: !!win.draft.wine_rt_prio_enabled
                            Controls.Label { text: qsTr("Base RT Priority:") }
                            Controls.SpinBox {
                                from: 1; to: 99
                                value: win.draft.wine_base_rt_prio || 15
                                onValueModified: win.mutateDraft(function(d) { d.wine_base_rt_prio = value; })
                            }
                            Controls.Label { text: qsTr("Server RT Priority:") }
                            Controls.SpinBox {
                                from: 1; to: 99
                                value: win.draft.wine_server_rt_prio || 10
                                onValueModified: win.mutateDraft(function(d) { d.wine_server_rt_prio = value; })
                            }
                        }
                    }

                    // 6: Experimental
                    ColumnLayout {
                        spacing: 8
                        Kirigami.Heading { text: qsTr("Experimental Features"); level: 3 }
                        Controls.CheckBox {
                            text: qsTr("Enable plugin bridges")
                            checked: !!win.draft.exp_plugin_bridges
                            onToggled: win.mutateDraft(function(d) { d.exp_plugin_bridges = checked; })
                        }
                        Controls.CheckBox {
                            enabled: !!win.draft.exp_plugin_bridges
                            text: qsTr("Prefer plugin bridges")
                            checked: !!win.draft.prefer_plugin_bridges
                            onToggled: win.mutateDraft(function(d) { d.prefer_plugin_bridges = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Enable Wine plugin bridges")
                            checked: !!win.draft.exp_wine_bridges
                            onToggled: win.mutateDraft(function(d) { d.exp_wine_bridges = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Enable JACK applications")
                            checked: !!win.draft.exp_jack_apps
                            onToggled: win.mutateDraft(function(d) { d.exp_jack_apps = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Enable LV2 plugin export")
                            checked: !!win.draft.exp_export_lv2
                            onToggled: win.mutateDraft(function(d) { d.exp_export_lv2 = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Prevent bad plugin behaviour")
                            checked: !!win.draft.exp_prevent_bad_behaviour
                            onToggled: win.mutateDraft(function(d) { d.exp_prevent_bad_behaviour = checked; })
                        }
                        Controls.CheckBox {
                            text: qsTr("Load plugin libraries globally")
                            checked: !!win.draft.exp_load_lib_global
                            onToggled: win.mutateDraft(function(d) { d.exp_load_lib_global = checked; })
                        }
                    }
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        RowLayout {
            Layout.fillWidth: true
            Controls.Button {
                text: qsTr("Restore Defaults")
                onClicked: {
                    win.draft = JSON.parse(win.controller.restore_default_settings_json());
                    win.refreshDriverDevices();
                }
            }
            Item { Layout.fillWidth: true }
            Controls.Button { text: qsTr("Cancel"); onClicked: win.close() }
            Controls.Button {
                text: qsTr("OK")
                highlighted: true
                onClicked: {
                    win.controller.save_settings_json(JSON.stringify(win.draft));
                    win.close();
                }
            }
        }
    }
}
