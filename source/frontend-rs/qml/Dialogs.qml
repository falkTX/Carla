// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: root
    required property var controller
    signal quitConfirmed()

    function openRename(pluginId, currentName) {
        renameDialog.pluginId = pluginId;
        renameField.text = currentName;
        renameDialog.open();
        renameField.forceActiveFocus();
        renameField.selectAll();
    }

    function openJackApp() {
        jackAppDialog.open();
    }

    function openAbout() {
        try {
            aboutDialog.aboutData = JSON.parse(controller.get_about_json());
        } catch (e) {
            aboutDialog.aboutData = { version: "2.6.0-alpha1", features: [], license: "" };
        }
        aboutTabs.currentIndex = 0;
        aboutDialog.open();
    }

    function openAlert(title, message) {
        alertDialog.title = title;
        alertLabel.text = message;
        alertDialog.open();
    }

    function openConfirmRemoveAll() {
        confirmRemoveAllDialog.open();
    }

    function openConfirmQuit() {
        confirmQuitDialog.open();
    }

    // 1. Rename Plugin Dialog
    Controls.Dialog {
        id: renameDialog
        property int pluginId: -1
        title: qsTr("Rename Plugin")
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
        width: 360

        ColumnLayout {
            anchors.fill: parent
            Controls.Label { text: qsTr("New plugin name:") }
            Controls.TextField {
                id: renameField
                Layout.fillWidth: true
                onAccepted: renameDialog.accept()
            }
        }
        onAccepted: {
            var nm = renameField.text.trim();
            if (nm.length > 0 && pluginId >= 0) {
                root.controller.rename_plugin(pluginId, nm);
            }
        }
    }

    // 2. Add JACK Application Dialog
    Controls.Dialog {
        id: jackAppDialog
        title: qsTr("Add JACK Application")
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
        width: 420

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 10
            rowSpacing: 6

            Controls.Label { text: qsTr("Command:") }
            Controls.TextField { id: jackCmd; Layout.fillWidth: true; placeholderText: qsTr("e.g. zynaddsubfx") }

            Controls.Label { text: qsTr("Name:") }
            Controls.TextField { id: jackName; Layout.fillWidth: true; placeholderText: qsTr("Optional display name") }

            Controls.Label { text: qsTr("Session Manager:") }
            Controls.ComboBox { id: jackSm; Layout.fillWidth: true; model: ["None", "LADISH (SIGUSR1)", "NSM"] }

            Controls.Label { text: qsTr("Audio Ins / Outs:") }
            RowLayout {
                Controls.SpinBox { id: jackAIn; from: 0; to: 32; value: 2 }
                Controls.Label { text: "/" }
                Controls.SpinBox { id: jackAOut; from: 0; to: 32; value: 2 }
            }

            Controls.Label { text: qsTr("MIDI Ins / Outs:") }
            RowLayout {
                Controls.SpinBox { id: jackMIn; from: 0; to: 16; value: 1 }
                Controls.Label { text: "/" }
                Controls.SpinBox { id: jackMOut; from: 0; to: 16; value: 1 }
            }

            Controls.CheckBox {
                id: jackManageWin
                Layout.columnSpan: 2
                text: qsTr("Manage application window")
            }
        }

        onAccepted: {
            if (jackCmd.text.trim().length > 0) {
                root.controller.add_jack_app(
                    jackName.text,
                    jackCmd.text,
                    jackAIn.value,
                    jackAOut.value,
                    jackMIn.value,
                    jackMOut.value,
                    jackSm.currentIndex,
                    jackManageWin.checked
                );
            }
        }
    }

    // 3. About Franca Dialog
    Controls.Dialog {
        id: aboutDialog
        property var aboutData: ({ version: "2.6.0-alpha1", features: [], license: "" })
        title: qsTr("About Franca")
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Close
        width: 540
        height: 420

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            RowLayout {
                spacing: 12
                Image {
                    source: "qrc:/franca.png"
                    sourceSize.width: 48
                    sourceSize.height: 48
                }
                ColumnLayout {
                    spacing: 2
                    Kirigami.Heading { text: "Franca"; level: 2 }
                    Controls.Label {
                        text: qsTr("Audio Plugin Host (Carla %1 backend)").arg(aboutDialog.aboutData.version || "")
                    }
                    Controls.Label {
                        text: qsTr("Native Rust + KDE Kirigami / Qt 6 UI")
                        opacity: 0.75
                    }
                }
            }

            Controls.TabBar {
                id: aboutTabs
                Layout.fillWidth: true
                Controls.TabButton { text: qsTr("Info & Features") }
                Controls.TabButton { text: qsTr("Credits & License") }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: aboutTabs.currentIndex

                Rectangle {
                    color: Kirigami.Theme.alternateBackgroundColor
                    border.width: 1
                    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.14)
                    radius: 4
                    clip: true

                    Controls.ScrollView {
                        anchors.fill: parent
                        anchors.margins: 10
                        clip: true
                        ColumnLayout {
                            width: parent.width - 12
                            spacing: 8
                            Controls.Label {
                                text: qsTr("Supported Backend Features")
                                font.bold: true
                            }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 6
                                Repeater {
                                    model: aboutDialog.aboutData.features || []
                                    delegate: Rectangle {
                                        required property string modelData
                                        height: 22
                                        width: featLabel.implicitWidth + 14
                                        radius: 3
                                        color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                                        border.width: 1
                                        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
                                        Controls.Label {
                                            id: featLabel
                                            anchors.centerIn: parent
                                            text: modelData
                                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    color: Kirigami.Theme.alternateBackgroundColor
                    border.width: 1
                    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.14)
                    radius: 4
                    clip: true

                    Controls.ScrollView {
                        anchors.fill: parent
                        clip: true
                        Controls.TextArea {
                            readOnly: true
                            selectByMouse: true
                            activeFocusOnTab: false
                            textFormat: TextEdit.RichText
                            wrapMode: TextEdit.WordWrap
                            padding: 10
                            color: Kirigami.Theme.textColor
                            background: null
                            text: aboutDialog.aboutData.license || ""
                            onLinkActivated: function(link) { Qt.openUrlExternally(link); }
                        }
                    }
                }
            }
        }
    }

    // 4. Alert Dialog
    Controls.Dialog {
        id: alertDialog
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Ok
        width: 420
        Controls.Label {
            id: alertLabel
            width: parent.width
            wrapMode: Text.WordWrap
        }
    }

    // 5. Confirm Remove All Dialog
    Controls.Dialog {
        id: confirmRemoveAllDialog
        title: qsTr("Remove All Plugins")
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.Cancel
        width: 380
        Controls.Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Are you sure you want to remove all plugins from the rack?")
        }
        onAccepted: root.controller.remove_all_plugins()
    }

    // 6. Confirm Quit Dialog
    Controls.Dialog {
        id: confirmQuitDialog
        title: qsTr("Quit Franca")
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.Cancel
        width: 380
        Controls.Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Plugins are currently loaded. Are you sure you want to exit?")
        }
        onAccepted: root.quitConfirmed()
    }
}
