// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ApplicationWindow {
    id: win
    required property var controller
    property int replacePluginId: -1

    property string searchText: ""
    property bool favoritesOnly: false
    property int categoryFilter: -1
    property int formatsMask: 0x3FF
    property int typesMask: 0x0F
    property int sortCol: 0
    property bool sortAsc: true

    property var queryResult: ({ total_count: 0, scanning: false, scan_progress: 0, scan_status: "", scan_found: 0, items: [] })
    property string selectedKey: ""
    property var selectedPlugin: null
    property var categories: []

    title: replacePluginId >= 0 ? qsTr("Replace Plugin") : qsTr("Add Plugin")
    width: 900
    height: 600
    minimumWidth: 680
    minimumHeight: 440

    function openForAdd() {
        replacePluginId = -1;
        runQuery();
        show();
        raise();
        requestActivate();
        searchField.forceActiveFocus();
    }

    function openForReplace(pid) {
        replacePluginId = pid;
        runQuery();
        show();
        raise();
        requestActivate();
        searchField.forceActiveFocus();
    }

    function runQuery() {
        try {
            var raw = controller.query_plugins_json(
                searchText, favoritesOnly, categoryFilter, formatsMask, typesMask, sortCol, sortAsc
            );
            queryResult = JSON.parse(raw);
            selectedPlugin = null;
            var items = queryResult.items || [];
            for (var i = 0; i < items.length; i++) {
                if (items[i].key === selectedKey) {
                    selectedPlugin = items[i];
                    break;
                }
            }
        } catch (e) {}
    }

    function toggleFormatBit(bit, on) {
        if (on) formatsMask |= (1 << bit); else formatsMask &= ~(1 << bit);
        runQuery();
    }
    function toggleTypeBit(bit, on) {
        if (on) typesMask |= (1 << bit); else typesMask &= ~(1 << bit);
        runQuery();
    }

    function commitSelection(plugin) {
        if (!plugin) return;
        controller.add_plugin_by_key(plugin.key, replacePluginId);
        close();
    }

    Component.onCompleted: {
        try { categories = JSON.parse(controller.categories_json); } catch (e) {}
    }

    Connections {
        target: win.controller
        function onDiscovery_revisionChanged() {
            if (win.visible) win.runQuery();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // Search & top filters
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Controls.TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search by name, maker, format…")
                text: win.searchText
                onTextChanged: { win.searchText = text; win.runQuery(); }
            }
            Controls.CheckBox {
                text: qsTr("★ Favorites")
                checked: win.favoritesOnly
                onToggled: { win.favoritesOnly = checked; win.runQuery(); }
            }
            Controls.Label { text: qsTr("Category:") }
            Controls.ComboBox {
                id: catCombo
                model: [{ id: -1, label: qsTr("All") }].concat(win.categories)
                textRole: "label"
                onActivated: function(idx) {
                    win.categoryFilter = idx === 0 ? -1 : win.categories[idx - 1].id;
                    win.runQuery();
                }
            }
            Controls.Button {
                text: qsTr("Refresh…")
                icon.name: "view-refresh"
                enabled: !win.queryResult.scanning
                onClicked: refreshModal.open()
            }
        }

        // Format filter checkboxes
        Flow {
            Layout.fillWidth: true
            spacing: 8
            Controls.Label { text: qsTr("Format:"); opacity: 0.7; topPadding: 5 }
            Repeater {
                model: ["Internal", "LADSPA", "DSSI", "LV2", "VST2", "VST3", "CLAP", "SF2", "SFZ", "JSFX"]
                delegate: Controls.CheckBox {
                    required property string modelData
                    required property int index
                    text: modelData
                    checked: (win.formatsMask & (1 << index)) !== 0
                    onToggled: win.toggleFormatBit(index, checked)
                }
            }
        }

        // Type filter checkboxes
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Controls.Label { text: qsTr("Type:"); opacity: 0.7 }
            Repeater {
                model: [qsTr("Effect"), qsTr("Instrument"), qsTr("MIDI"), qsTr("Other")]
                delegate: Controls.CheckBox {
                    required property string modelData
                    required property int index
                    text: modelData
                    checked: (win.typesMask & (1 << index)) !== 0
                    onToggled: win.toggleTypeBit(index, checked)
                }
            }
            Item { Layout.fillWidth: true }
        }

        // Live discovery progress banner
        Controls.Frame {
            Layout.fillWidth: true
            visible: !!win.queryResult.scanning
            RowLayout {
                anchors.fill: parent
                spacing: 8
                Controls.ProgressBar {
                    Layout.preferredWidth: 180
                    value: win.queryResult.scan_progress || 0
                }
                Controls.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: (win.queryResult.scan_status || "") + " (" + (win.queryResult.scan_found || 0) + " found)"
                }
                Controls.Button { text: qsTr("Skip"); onClicked: win.controller.skip_discovery() }
                Controls.Button { text: qsTr("Cancel"); onClicked: win.controller.cancel_discovery() }
            }
        }

        // Sortable Header Row
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Item { Layout.preferredWidth: 32 }
            Controls.ToolButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 240
                text: qsTr("Name") + (win.sortCol === 0 ? (win.sortAsc ? " ▲" : " ▼") : "")
                onClicked: { if (win.sortCol === 0) win.sortAsc = !win.sortAsc; else { win.sortCol = 0; win.sortAsc = true; } win.runQuery(); }
            }
            Controls.ToolButton {
                Layout.preferredWidth: 76
                text: qsTr("Format") + (win.sortCol === 1 ? (win.sortAsc ? " ▲" : " ▼") : "")
                onClicked: { if (win.sortCol === 1) win.sortAsc = !win.sortAsc; else { win.sortCol = 1; win.sortAsc = true; } win.runQuery(); }
            }
            Controls.ToolButton {
                Layout.preferredWidth: 96
                text: qsTr("Category") + (win.sortCol === 2 ? (win.sortAsc ? " ▲" : " ▼") : "")
                onClicked: { if (win.sortCol === 2) win.sortAsc = !win.sortAsc; else { win.sortCol = 2; win.sortAsc = true; } win.runQuery(); }
            }
            Controls.ToolButton {
                Layout.preferredWidth: 160
                text: qsTr("Maker") + (win.sortCol === 3 ? (win.sortAsc ? " ▲" : " ▼") : "")
                onClicked: { if (win.sortCol === 3) win.sortAsc = !win.sortAsc; else { win.sortCol = 3; win.sortAsc = true; } win.runQuery(); }
            }
            Controls.Label { Layout.preferredWidth: 92; text: qsTr("I/O"); opacity: 0.7 }
        }

        // Plugin List
        Controls.Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0

            Controls.ScrollView {
                anchors.fill: parent
                clip: true

                ListView {
                    model: win.queryResult.items || []
                    clip: true
                    delegate: Controls.ItemDelegate {
                        required property var modelData
                        required property int index
                        width: ListView.view.width
                        height: 28
                        highlighted: win.selectedKey === modelData.key
                        onClicked: {
                            win.selectedKey = modelData.key;
                            win.selectedPlugin = modelData;
                        }
                        onDoubleClicked: win.commitSelection(modelData)

                        contentItem: RowLayout {
                            spacing: 4
                            Controls.ToolButton {
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 24
                                text: modelData.favorite ? "★" : "☆"
                                onClicked: win.controller.toggle_favorite(modelData.key)
                            }
                            Controls.Label {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 240
                                text: modelData.name
                                elide: Text.ElideRight
                            }
                            Controls.Label {
                                Layout.preferredWidth: 76
                                text: modelData.format
                                opacity: 0.85
                            }
                            Controls.Label {
                                Layout.preferredWidth: 96
                                text: modelData.category
                                opacity: 0.85
                            }
                            Controls.Label {
                                Layout.preferredWidth: 160
                                text: modelData.maker
                                elide: Text.ElideRight
                                opacity: 0.85
                            }
                            Controls.Label {
                                Layout.preferredWidth: 92
                                font.family: "monospace"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: "A:" + modelData.audio_ins + "/" + modelData.audio_outs + " M:" + modelData.midi_ins + "/" + modelData.midi_outs
                            }
                        }
                    }
                }
            }
        }

        // Selected Plugin Details
        Controls.Frame {
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 2
                Controls.Label {
                    font.bold: true
                    text: win.selectedPlugin
                        ? (win.selectedPlugin.name + "  [" + win.selectedPlugin.format + " · " + win.selectedPlugin.build + "]")
                        : qsTr("Showing %1 of %2 plugins. Select a plugin or click Refresh to scan.")
                              .arg((win.queryResult.items || []).length)
                              .arg(win.queryResult.total_count || 0)
                }
                Controls.Label {
                    visible: !!win.selectedPlugin
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    text: win.selectedPlugin
                        ? qsTr("Maker: %1  |  Category: %2  |  Audio: %3/%4  MIDI: %5/%6  Params: %7/%8")
                              .arg(win.selectedPlugin.maker || qsTr("Unknown"))
                              .arg(win.selectedPlugin.category)
                              .arg(win.selectedPlugin.audio_ins).arg(win.selectedPlugin.audio_outs)
                              .arg(win.selectedPlugin.midi_ins).arg(win.selectedPlugin.midi_outs)
                              .arg(win.selectedPlugin.param_ins).arg(win.selectedPlugin.param_outs)
                        : ""
                }
                Controls.Label {
                    visible: !!(win.selectedPlugin && win.selectedPlugin.filename)
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.7
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                    text: win.selectedPlugin ? (win.selectedPlugin.filename || "") : ""
                }
            }
        }

        // Footer buttons
        RowLayout {
            Layout.fillWidth: true
            Controls.Label {
                opacity: 0.75
                text: qsTr("%1 matching plugins").arg((win.queryResult.items || []).length)
            }
            Item { Layout.fillWidth: true }
            Controls.Button { text: qsTr("Cancel"); onClicked: win.close() }
            Controls.Button {
                text: win.replacePluginId >= 0 ? qsTr("Replace Plugin") : qsTr("Add Plugin")
                icon.name: "list-add"
                enabled: !!win.selectedPlugin
                highlighted: true
                onClicked: win.commitSelection(win.selectedPlugin)
            }
        }
    }

    // Refresh Plugins Options Modal
    Controls.Dialog {
        id: refreshModal
        property int scanMask: 0x3FF
        title: qsTr("Refresh Plugins")
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
        width: 440

        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Controls.Label { text: qsTr("Select plugin formats to scan:") }
            Flow {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: ["Internal", "LADSPA", "DSSI", "LV2", "VST2", "VST3", "CLAP", "SF2", "SFZ", "JSFX"]
                    delegate: Controls.CheckBox {
                        required property string modelData
                        required property int index
                        text: modelData
                        checked: (refreshModal.scanMask & (1 << index)) !== 0
                        onToggled: {
                            if (checked) refreshModal.scanMask |= (1 << index);
                            else refreshModal.scanMask &= ~(1 << index);
                        }
                    }
                }
            }
            Controls.CheckBox {
                id: rescanAllCb
                text: qsTr("Ignore cache and rescan all files")
            }
        }

        onAccepted: {
            win.controller.start_discovery(refreshModal.scanMask, rescanAllCb.checked);
        }
    }
}
