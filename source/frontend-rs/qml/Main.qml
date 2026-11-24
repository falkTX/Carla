// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.franca.app 1.0

Controls.ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1180
    height: 760
    minimumWidth: 740
    minimumHeight: 500
    title: ctrl.window_title

    property bool forceClose: false

    FrancaController {
        id: ctrl
        onAlert_requested: function(title, message) {
            dialogs.openAlert(title, message);
        }
        onFocus_patchbay_requested: {
            mainTabs.currentIndex = 1;
        }
        onQuit_signal_received: {
            mainWindow.forceClose = true;
            mainWindow.close();
        }
    }

    // Periodic engine idle, meter, transport & discovery poll
    Timer {
        id: pollTimer
        interval: Math.max(10, Math.min(100, ctrl.refresh_interval_ms))
        running: !francaUiBusy
        repeat: true
        onTriggered: ctrl.poll_tick()
    }

    onClosing: function(close) {
        if (!forceClose && ctrl.confirm_exit && ctrl.plugin_count > 0) {
            close.accepted = false;
            dialogs.openConfirmQuit();
            return;
        }
        pollTimer.stop();
        ctrl.shutdown_clean();
    }

    // Keyboard Shortcuts
    Shortcut { sequences: [StandardKey.New]; enabled: ctrl.engine_running; onActivated: ctrl.file_new() }
    Shortcut { sequences: [StandardKey.Open]; enabled: ctrl.engine_running; onActivated: ctrl.file_open() }
    Shortcut { sequences: [StandardKey.Save]; enabled: ctrl.engine_running; onActivated: ctrl.file_save() }
    Shortcut { sequences: [StandardKey.SaveAs]; enabled: ctrl.engine_running; onActivated: ctrl.file_save_as() }
    Shortcut { sequences: [StandardKey.Undo]; enabled: ctrl.can_undo; onActivated: ctrl.undo() }
    Shortcut { sequences: [StandardKey.Redo, "Ctrl+Y"]; enabled: ctrl.can_redo; onActivated: ctrl.redo() }
    Shortcut { sequence: "Ctrl+P"; enabled: ctrl.engine_running; onActivated: addPluginDlg.openForAdd() }
    Shortcut { sequence: "Ctrl+G"; enabled: ctrl.engine_running && mainTabs.currentIndex === 1; onActivated: { ctrl.arrange_patchbay(); patchbayView.zoomFit(); } }
    Shortcut { sequences: ["Ctrl+R", "F5"]; enabled: ctrl.engine_running && mainTabs.currentIndex === 1; onActivated: ctrl.refresh_patchbay() }
    Shortcut { sequences: ["Ctrl++", "Ctrl+="]; enabled: mainTabs.currentIndex === 1; onActivated: patchbayView.zoomIn() }
    Shortcut { sequence: "Ctrl+-"; enabled: mainTabs.currentIndex === 1; onActivated: patchbayView.zoomOut() }
    Shortcut { sequence: "Ctrl+1"; enabled: mainTabs.currentIndex === 1; onActivated: patchbayView.zoomReset() }
    Shortcut { sequence: "Home"; enabled: mainTabs.currentIndex === 1; onActivated: patchbayView.zoomFit() }
    Shortcut { sequence: "Ctrl+A"; enabled: mainTabs.currentIndex === 1; onActivated: patchbayView.selectAllBoxes() }
    Shortcut {
        sequences: [StandardKey.Delete]
        enabled: mainTabs.currentIndex === 0 && rackView.selectedPluginId >= 0
        onActivated: ctrl.remove_plugin(rackView.selectedPluginId)
    }
    Shortcut { sequences: [StandardKey.Quit]; onActivated: mainWindow.close() }

    // Native KDE Menu Bar
    menuBar: Controls.MenuBar {
        Controls.Menu {
            title: qsTr("&File")
            Controls.MenuItem { text: qsTr("New"); icon.name: "document-new"; enabled: ctrl.engine_running; onTriggered: ctrl.file_new() }
            Controls.MenuItem { text: qsTr("Open…"); icon.name: "document-open"; enabled: ctrl.engine_running; onTriggered: ctrl.file_open() }
            Controls.MenuItem { text: qsTr("Save"); icon.name: "document-save"; enabled: ctrl.engine_running; onTriggered: ctrl.file_save() }
            Controls.MenuItem { text: qsTr("Save As…"); icon.name: "document-save-as"; enabled: ctrl.engine_running; onTriggered: ctrl.file_save_as() }
            Controls.MenuSeparator {}
            Controls.MenuItem { text: qsTr("Quit"); icon.name: "application-exit"; onTriggered: mainWindow.close() }
        }

        Controls.Menu {
            title: qsTr("&Edit")
            Controls.MenuItem { text: ctrl.undo_label; icon.name: "edit-undo"; enabled: ctrl.can_undo; onTriggered: ctrl.undo() }
            Controls.MenuItem { text: ctrl.redo_label; icon.name: "edit-redo"; enabled: ctrl.can_redo; onTriggered: ctrl.redo() }
        }

        Controls.Menu {
            title: qsTr("E&ngine")
            Controls.MenuItem { text: qsTr("Start"); icon.name: "media-playback-start"; enabled: !ctrl.engine_running; onTriggered: ctrl.start_engine() }
            Controls.MenuItem { text: qsTr("Stop"); icon.name: "media-playback-stop"; enabled: ctrl.engine_running; onTriggered: ctrl.stop_engine() }
            Controls.MenuSeparator {}
            Controls.MenuItem { text: qsTr("Panic (All Notes Off)"); enabled: ctrl.engine_running; onTriggered: ctrl.engine_panic() }
            Controls.MenuItem { text: qsTr("Clear Xruns"); enabled: ctrl.engine_running; onTriggered: ctrl.clear_xruns() }
            Controls.MenuItem { text: qsTr("Driver Settings…"); icon.name: "configure"; enabled: ctrl.engine_running; onTriggered: runtimeDriverDlg.openRuntime() }
        }

        Controls.Menu {
            title: qsTr("&Plugin")
            Controls.MenuItem { text: qsTr("Add Plugin…"); icon.name: "list-add"; enabled: ctrl.engine_running; onTriggered: addPluginDlg.openForAdd() }
            Controls.MenuItem {
                text: qsTr("Add JACK Application…")
                visible: ctrl.jack_apps_enabled
                enabled: ctrl.engine_running
                onTriggered: dialogs.openJackApp()
            }
            Controls.MenuItem { text: qsTr("Remove All…"); icon.name: "edit-delete"; enabled: ctrl.plugin_count > 0; onTriggered: dialogs.openConfirmRemoveAll() }
            Controls.MenuSeparator {}
            Controls.Menu {
                title: qsTr("Macros")
                enabled: ctrl.plugin_count > 0
                Controls.MenuItem { text: qsTr("Enable All"); onTriggered: ctrl.macro_enable_all() }
                Controls.MenuItem { text: qsTr("Disable All"); onTriggered: ctrl.macro_disable_all() }
                Controls.MenuSeparator {}
                Controls.MenuItem { text: qsTr("Volume 100%"); onTriggered: ctrl.macro_volume_100() }
                Controls.MenuItem { text: qsTr("Mute All"); onTriggered: ctrl.macro_mute_all() }
                Controls.MenuSeparator {}
                Controls.MenuItem { text: qsTr("Dry/Wet 100%"); onTriggered: ctrl.macro_wet_100() }
                Controls.MenuItem { text: qsTr("Bypass All (Dry/Wet 0%)"); onTriggered: ctrl.macro_bypass_all() }
                Controls.MenuItem { text: qsTr("Center Balance / Pan"); onTriggered: ctrl.macro_center_balance() }
            }
        }

        Controls.Menu {
            title: qsTr("&Canvas")
            enabled: ctrl.engine_running
            Controls.MenuItem {
                text: qsTr("Plugin Routing (Internal)")
                checkable: true
                checked: !ctrl.external_patchbay
                onTriggered: { patchbayView.needsInitialFit = true; ctrl.switch_external_patchbay(false); }
            }
            Controls.MenuItem {
                text: qsTr("System Routing (External)")
                checkable: true
                checked: ctrl.external_patchbay
                onTriggered: { patchbayView.needsInitialFit = true; ctrl.switch_external_patchbay(true); }
            }
            Controls.MenuSeparator {}
            Controls.MenuItem { text: qsTr("Arrange"); icon.name: "view-sort-ascending"; onTriggered: { ctrl.arrange_patchbay(); patchbayView.zoomFit(); } }
            Controls.MenuItem { text: qsTr("Refresh"); icon.name: "view-refresh"; onTriggered: ctrl.refresh_patchbay() }
            Controls.MenuSeparator {}
            Controls.Menu {
                title: qsTr("Zoom")
                Controls.MenuItem { text: qsTr("Auto-Fit"); icon.name: "zoom-fit-best"; onTriggered: patchbayView.zoomFit() }
                Controls.MenuItem { text: qsTr("Zoom In"); icon.name: "zoom-in"; onTriggered: patchbayView.zoomIn() }
                Controls.MenuItem { text: qsTr("Zoom Out"); icon.name: "zoom-out"; onTriggered: patchbayView.zoomOut() }
                Controls.MenuItem { text: qsTr("Zoom 100%"); icon.name: "zoom-original"; onTriggered: patchbayView.zoomReset() }
            }
            Controls.Menu {
                title: qsTr("Save Image")
                Controls.MenuItem { text: qsTr("Save PNG (1x)…"); onTriggered: ctrl.save_patchbay_image(1.0) }
                Controls.MenuItem { text: qsTr("Save PNG (2x)…"); onTriggered: ctrl.save_patchbay_image(2.0) }
                Controls.MenuItem { text: qsTr("Save PNG (4x)…"); onTriggered: ctrl.save_patchbay_image(4.0) }
                Controls.MenuSeparator {}
                Controls.MenuItem { text: qsTr("Copy to Clipboard"); icon.name: "edit-copy"; onTriggered: ctrl.copy_patchbay_image() }
            }
        }

        Controls.Menu {
            title: qsTr("&Settings")
            Controls.MenuItem {
                text: qsTr("Show Main Toolbar")
                checkable: true
                checked: ctrl.show_toolbar
                onToggled: ctrl.set_layout_toggle("show_toolbar", checked)
            }
            Controls.MenuItem {
                text: qsTr("Show Files, Transport and Minimap")
                checkable: true
                checked: ctrl.show_side_panel
                onToggled: ctrl.set_layout_toggle("show_side_panel", checked)
            }
            Controls.MenuItem {
                text: qsTr("Show Patchbay Peak Meters")
                checkable: true
                checked: ctrl.show_meters
                onToggled: ctrl.set_layout_toggle("show_meters", checked)
            }
            Controls.MenuItem {
                text: qsTr("Show Patchbay Keyboard")
                checkable: true
                checked: ctrl.show_keyboard
                onToggled: ctrl.set_layout_toggle("show_keyboard", checked)
            }
            Controls.MenuSeparator {}
            Controls.MenuItem {
                text: qsTr("Configure Franca…")
                icon.name: "configure"
                onTriggered: settingsDlg.openSettings()
            }
        }

        Controls.Menu {
            title: qsTr("&Help")
            Controls.MenuItem {
                text: qsTr("About Franca")
                icon.name: "help-about"
                onTriggered: dialogs.openAbout()
            }
        }
    }

    // Native KDE Main Toolbar
    header: Controls.ToolBar {
        visible: ctrl.show_toolbar
        RowLayout {
            anchors.fill: parent
            spacing: 2
            Controls.ToolButton { text: qsTr("New"); icon.name: "document-new"; enabled: ctrl.engine_running; onClicked: ctrl.file_new() }
            Controls.ToolButton { text: qsTr("Open"); icon.name: "document-open"; enabled: ctrl.engine_running; onClicked: ctrl.file_open() }
            Controls.ToolButton { text: qsTr("Save"); icon.name: "document-save"; enabled: ctrl.engine_running; onClicked: ctrl.file_save() }
            Controls.ToolSeparator {}
            Controls.ToolButton {
                text: qsTr("Undo")
                icon.name: "edit-undo"
                enabled: ctrl.can_undo
                onClicked: ctrl.undo()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: ctrl.undo_label
            }
            Controls.ToolButton {
                text: qsTr("Redo")
                icon.name: "edit-redo"
                enabled: ctrl.can_redo
                onClicked: ctrl.redo()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: ctrl.redo_label
            }
            Controls.ToolSeparator {}
            Controls.ToolButton {
                text: qsTr("Add Plugin")
                icon.name: "list-add"
                enabled: ctrl.engine_running
                onClicked: addPluginDlg.openForAdd()
            }
            Item { Layout.fillWidth: true }
            Controls.Label {
                visible: ctrl.project_name.length > 0
                text: ctrl.project_name
                opacity: 0.7
                Layout.rightMargin: 8
            }
        }
    }

    // Main Content Area with Drag-and-Drop Support
    Item {
        anchors.fill: parent

        Controls.SplitView {
            anchors.fill: parent
            orientation: Qt.Horizontal

            // Collapsible Left Tools Side Panel
            SidePanel {
                visible: ctrl.show_side_panel
                Controls.SplitView.preferredWidth: 240
                Controls.SplitView.minimumWidth: 190
                Controls.SplitView.maximumWidth: 420
                controller: ctrl
                patchbayView: patchbayView
            }

            // Center Tabs: Rack / Patchbay / Logs
            ColumnLayout {
                Controls.SplitView.fillWidth: true
                spacing: 0

                Controls.TabBar {
                    id: mainTabs
                    Layout.fillWidth: true
                    Controls.TabButton { text: qsTr("Rack") }
                    Controls.TabButton { text: qsTr("Patchbay") }
                    Controls.TabButton {
                        text: qsTr("Logs")
                        visible: ctrl.show_logs
                        width: visible ? implicitWidth : 0
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: mainTabs.currentIndex

                    RackView {
                        id: rackView
                        controller: ctrl
                        onOpenAddPlugin: addPluginDlg.openForAdd()
                        onOpenPluginEdit: function(pid) { pluginEditWin.openForPlugin(pid); }
                        onOpenRenamePlugin: function(pid, name) { dialogs.openRename(pid, name); }
                        onOpenReplacePlugin: function(pid) { addPluginDlg.openForReplace(pid); }
                    }

                    PatchbayView {
                        id: patchbayView
                        controller: ctrl
                        onOpenPluginEdit: function(pid) { pluginEditWin.openForPlugin(pid); }
                        onOpenRenamePlugin: function(pid, name) { dialogs.openRename(pid, name); }
                        onOpenReplacePlugin: function(pid) { addPluginDlg.openForReplace(pid); }
                    }

                    LogsTab {
                        controller: ctrl
                    }
                }
            }
        }

        DropArea {
            id: fileDropArea
            anchors.fill: parent
            z: 100
            onEntered: function(drag) {
                if (drag.hasUrls) {
                    drag.acceptProposedAction();
                } else {
                    drag.accepted = false;
                }
            }
            onDropped: function(drop) {
                if (drop.hasUrls) {
                    drop.acceptProposedAction();
                    for (var i = 0; i < drop.urls.length; i++) {
                        ctrl.load_file_url(drop.urls[i].toString());
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            z: 101
            visible: fileDropArea.containsDrag
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.10)
            border.width: 2
            border.color: Kirigami.Theme.highlightColor
        }
    }

    // Native KDE Status Bar
    footer: Controls.ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 8

            Rectangle {
                width: 9; height: 9; radius: 4.5
                Layout.leftMargin: 6
                color: ctrl.engine_running ? "#22C55E" : "#9CA3AF"
            }
            Controls.Label {
                text: ctrl.engine_running ? qsTr("%1 running").arg(ctrl.driver_name) : qsTr("Audio stopped")
            }

            Controls.ToolSeparator {}

            Controls.Label {
                text: ctrl.plugin_count === 1 ? qsTr("1 plugin") : qsTr("%1 plugins").arg(ctrl.plugin_count)
            }

            Item { Layout.fillWidth: true }

            // Buffer size
            Controls.Label { text: qsTr("Buffer"); opacity: 0.75 }
            Controls.ComboBox {
                id: bufCombo
                property var sizesList: {
                    try { return JSON.parse(ctrl.buffer_sizes_json); } catch (e) { return []; }
                }
                visible: ctrl.engine_running && ctrl.variable_buffer_size && sizesList.length > 0
                implicitWidth: 82
                model: sizesList
                currentIndex: Math.max(0, sizesList.indexOf(ctrl.buffer_size))
                onActivated: function(i) { ctrl.apply_buffer_size(sizesList[i]); }
            }
            Controls.Label {
                visible: !bufCombo.visible
                text: ctrl.engine_running ? String(ctrl.buffer_size) : "—"
            }

            Controls.ToolSeparator {}

            // Sample rate
            Controls.Label { text: qsTr("Sample rate"); opacity: 0.75 }
            Controls.ComboBox {
                id: srCombo
                property var ratesList: {
                    try { return JSON.parse(ctrl.sample_rates_json); } catch (e) { return []; }
                }
                visible: ctrl.engine_running && ctrl.variable_sample_rate && ratesList.length > 0
                implicitWidth: 100
                model: ratesList.map(function(r) { return r + " Hz"; })
                currentIndex: Math.max(0, ratesList.indexOf(ctrl.sample_rate))
                onActivated: function(i) { ctrl.apply_sample_rate(ratesList[i]); }
            }
            Controls.Label {
                visible: !srCombo.visible
                text: ctrl.engine_running ? (ctrl.sample_rate + " Hz") : "— Hz"
            }

            Controls.ToolSeparator {}

            // Clickable Xruns counter
            Controls.ToolButton {
                enabled: ctrl.engine_running
                text: ctrl.xruns === 1 ? qsTr("1 xrun") : qsTr("%1 xruns").arg(ctrl.xruns)
                onClicked: ctrl.clear_xruns()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Click to clear xruns")
            }

            Controls.ToolSeparator {}

            // DSP % Load Bar
            Controls.Label {
                text: qsTr("DSP")
                opacity: 0.75
            }
            Controls.ProgressBar {
                Layout.preferredWidth: 84
                Layout.alignment: Qt.AlignVCenter
                from: 0; to: 100
                value: ctrl.dsp_load
            }
            Controls.Label {
                Layout.preferredWidth: 34
                Layout.rightMargin: 6
                horizontalAlignment: Text.AlignRight
                text: Math.round(ctrl.dsp_load) + "%"
            }
        }
    }

    // Secondary Windows & Dialogs
    PluginEditWindow { id: pluginEditWin; controller: ctrl }
    AddPluginDialog { id: addPluginDlg; controller: ctrl }
    SettingsDialog { id: settingsDlg; controller: ctrl }
    RuntimeDriverDialog { id: runtimeDriverDlg; controller: ctrl }
    Dialogs {
        id: dialogs
        controller: ctrl
        onQuitConfirmed: {
            mainWindow.forceClose = true;
            mainWindow.close();
        }
    }
}
