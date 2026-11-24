// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root
    required property var controller
    property bool autoScroll: true

    spacing: 4

    Controls.ToolBar {
        Layout.fillWidth: true
        RowLayout {
            anchors.fill: parent
            spacing: 4
            Controls.ToolButton {
                text: qsTr("Open Folder")
                icon.name: "folder-open"
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: root.controller.log_dir
                onClicked: root.controller.open_log_folder()
            }
            Controls.ToolButton {
                text: qsTr("Save…")
                icon.name: "document-save"
                onClicked: root.controller.save_logs_dialog()
            }
            Controls.ToolButton {
                text: qsTr("Clear")
                icon.name: "edit-clear"
                onClicked: root.controller.clear_logs()
            }
            Controls.ToolSeparator {}
            Controls.CheckBox {
                text: qsTr("Auto-Scroll")
                checked: root.autoScroll
                onToggled: root.autoScroll = checked
            }
            Item { Layout.fillWidth: true }
        }
    }

    Controls.ScrollView {
        id: scrollView
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        Controls.TextArea {
            id: logArea
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.NoWrap
            font.family: "monospace"
            text: root.controller.logs_text
            onTextChanged: {
                if (root.autoScroll) {
                    cursorPosition = text.length;
                }
            }
        }
    }
}
