// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.Pane {
    id: root
    required property var controller
    required property var patchbayView
    property var filesData: ({ folders: ["Home"], selected_folder_idx: 0, current_dir: "", can_go_up: false, entries: [] })

    function refreshFiles() {
        try {
            filesData = JSON.parse(controller.files_json);
        } catch (e) {}
    }

    Component.onCompleted: refreshFiles()
    Connections {
        target: root.controller
        function onFiles_jsonChanged() { root.refreshFiles(); }
    }

    padding: 6

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        // 1. Files Browser
        Kirigami.Heading {
            text: qsTr("Files")
            level: 4
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Controls.ComboBox {
                id: folderCombo
                Layout.fillWidth: true
                model: root.filesData.folders || ["Home"]
                currentIndex: root.filesData.selected_folder_idx || 0
                onActivated: function(idx) {
                    root.controller.files_select_folder_index(idx);
                }
            }
            Controls.ToolButton {
                icon.name: "list-add"
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Add folder")
                onClicked: root.controller.files_add_folder()
            }
            Controls.ToolButton {
                icon.name: "list-remove"
                enabled: folderCombo.currentIndex > 0
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Remove folder")
                onClicked: root.controller.files_remove_folder(folderCombo.currentIndex)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Controls.ToolButton {
                icon.name: "go-up"
                enabled: !!root.filesData.can_go_up
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Parent folder")
                onClicked: root.controller.files_go_up()
            }
            Controls.Label {
                Layout.fillWidth: true
                text: root.filesData.current_dir || ""
                elide: Text.ElideLeft
                opacity: 0.75
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }
        }

        Controls.Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 120
            padding: 0

            Controls.ScrollView {
                anchors.fill: parent
                clip: true

                ListView {
                    id: filesList
                    model: root.filesData.entries || []
                    delegate: Controls.ItemDelegate {
                        width: ListView.view.width
                        height: 26
                        text: modelData.name
                        icon.name: modelData.is_dir ? "folder" : "audio-x-generic"
                        onClicked: {
                            if (modelData.is_dir) {
                                root.controller.files_navigate(modelData.path);
                            }
                        }
                        onDoubleClicked: {
                            if (!modelData.is_dir && root.controller.engine_running) {
                                root.controller.load_file_url(modelData.path);
                            }
                        }
                    }
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        // 2. Transport
        Kirigami.Heading {
            text: qsTr("Transport")
            level: 4
        }

        ColumnLayout {
            Layout.fillWidth: true
            enabled: root.controller.engine_running
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Controls.ToolButton {
                    icon.name: root.controller.transport_playing ? "media-playback-pause" : "media-playback-start"
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.text: qsTr("Play / Pause")
                    onClicked: root.controller.transport_play_pause()
                }
                Controls.ToolButton {
                    icon.name: "media-playback-stop"
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.text: qsTr("Stop")
                    onClicked: root.controller.transport_stop()
                }
                Controls.ToolButton {
                    icon.name: "media-seek-backward"
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.text: qsTr("Rewind")
                    onClicked: root.controller.transport_rewind()
                }
                Controls.ToolButton {
                    icon.name: "media-seek-forward"
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.text: qsTr("Fast Forward")
                    onClicked: root.controller.transport_forward()
                }
                Item { Layout.fillWidth: true }
            }

            RowLayout {
                Layout.fillWidth: true
                Controls.Label { text: qsTr("BPM:") }
                Controls.SpinBox {
                    id: bpmSpin
                    Layout.fillWidth: true
                    from: 2000
                    to: 30000
                    stepSize: 100
                    editable: true
                    value: Math.round(root.controller.transport_bpm * 100)
                    textFromValue: function(v) { return (v / 100.0).toFixed(2); }
                    valueFromText: function(t) { return Math.round(parseFloat(t) * 100) || 12000; }
                    onValueModified: root.controller.transport_set_bpm(value / 100.0)
                }
            }

            GridLayout {
                columns: 2
                columnSpacing: 8
                rowSpacing: 2
                Controls.Label { text: qsTr("Frame:"); opacity: 0.7; font.pointSize: Kirigami.Theme.smallFont.pointSize }
                Controls.Label { text: root.controller.transport_frame_str; font.family: "monospace"; font.pointSize: Kirigami.Theme.smallFont.pointSize }
                Controls.Label { text: qsTr("Time:"); opacity: 0.7; font.pointSize: Kirigami.Theme.smallFont.pointSize }
                Controls.Label { text: root.controller.transport_time_str; font.family: "monospace"; font.pointSize: Kirigami.Theme.smallFont.pointSize }
                Controls.Label { text: qsTr("BBT:"); opacity: 0.7; font.pointSize: Kirigami.Theme.smallFont.pointSize }
                Controls.Label { text: root.controller.transport_bbt_str; font.family: "monospace"; font.pointSize: Kirigami.Theme.smallFont.pointSize }
            }

            Controls.CheckBox {
                text: qsTr("JACK Transport")
                checked: root.controller.transport_jack
                onToggled: root.controller.transport_set_jack(checked)
            }
            Controls.CheckBox {
                text: qsTr("Ableton Link")
                checked: root.controller.transport_link
                onToggled: root.controller.transport_set_link(checked)
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        // 3. Patchbay Minimap
        Kirigami.Heading {
            text: qsTr("Minimap")
            level: 4
        }

        Minimap {
            Layout.fillWidth: true
            Layout.preferredHeight: 130
            sceneData: root.patchbayView ? root.patchbayView.sceneData : null
            panX: root.patchbayView ? root.patchbayView.panX : 1550
            panY: root.patchbayView ? root.patchbayView.panY : 1200
            zoom: root.patchbayView ? root.patchbayView.zoom : 1.0
            viewportW: root.patchbayView ? root.patchbayView.canvasViewportW : 800
            viewportH: root.patchbayView ? root.patchbayView.canvasViewportH : 600
            onPanRequested: function(sx, sy) {
                if (root.patchbayView) {
                    root.patchbayView.panX = sx;
                    root.patchbayView.panY = sy;
                }
            }
        }
    }
}
