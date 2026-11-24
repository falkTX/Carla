// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: root
    required property var controller
    property var rackItems: []
    property var meterItems: []
    property var categories: []
    property int selectedPluginId: -1

    // Drag-to-reorder state
    property int dragSourceIdx: -1
    property int dragTargetIdx: -1

    signal openAddPlugin()
    signal openPluginEdit(int pluginId)
    signal openRenamePlugin(int pluginId, string currentName)
    signal openReplacePlugin(int pluginId)

    function parseRack() {
        try { rackItems = JSON.parse(controller.rack_json); } catch (e) { rackItems = []; }
    }
    function parseMeters() {
        try {
            var m = JSON.parse(controller.meters_json);
            meterItems = m.slots || [];
        } catch (e) {}
    }
    function parseCategories() {
        try { categories = JSON.parse(controller.categories_json); } catch (e) { categories = []; }
    }

    Component.onCompleted: {
        parseCategories();
        parseRack();
        parseMeters();
    }

    Connections {
        target: root.controller
        function onRack_jsonChanged() { root.parseRack(); }
        function onMeters_jsonChanged() { root.parseMeters(); }
    }

    // Empty state placeholder
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - 48
        visible: root.rackItems.length === 0
        text: qsTr("No plugins loaded in the rack")
        explanation: qsTr("Add audio or MIDI plugins to start processing.")
        icon.name: "audio-card"
        helpfulAction: Kirigami.Action {
            text: qsTr("Add Plugin…")
            icon.name: "list-add"
            enabled: root.controller.engine_running
            onTriggered: root.openAddPlugin()
        }
    }

    Controls.ScrollView {
        anchors.fill: parent
        visible: root.rackItems.length > 0
        clip: true

        ListView {
            id: rackList
            model: root.rackItems
            spacing: 1
            clip: true

            delegate: Item {
                id: rowItem
                required property var modelData
                required property int index

                readonly property int pid: modelData.plugin_id
                readonly property bool isActive: !!modelData.active
                readonly property bool isSelected: root.selectedPluginId === pid
                readonly property bool isUnavailable: !!modelData.unavailable_error
                readonly property var meter: (index < root.meterItems.length) ? root.meterItems[index] : null
                readonly property string catColor: modelData.category_color || ""

                width: ListView.view.width
                height: 40

                // Alternating base + selection + bypassed dim + category tint + 5px left category accent bar
                Rectangle {
                    anchors.fill: parent
                    color: rowItem.isSelected
                        ? Kirigami.Theme.highlightColor
                        : (rowItem.index % 2 === 1 ? Kirigami.Theme.alternateBackgroundColor : Kirigami.Theme.backgroundColor)
                    opacity: rowItem.isSelected ? (rowItem.isActive ? 0.30 : 0.16) : 1.0
                }
                Rectangle {
                    anchors.fill: parent
                    visible: !rowItem.isActive && !rowItem.isSelected
                    color: Kirigami.Theme.disabledTextColor
                    opacity: 0.10
                }
                Rectangle {
                    anchors.fill: parent
                    visible: rowItem.catColor.length > 0 && !rowItem.isSelected && rowItem.isActive
                    color: rowItem.catColor.length > 0 ? rowItem.catColor : "transparent"
                    opacity: 0.10
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 5
                    visible: rowItem.catColor.length > 0
                    color: rowItem.isActive
                        ? (rowItem.catColor.length > 0 ? rowItem.catColor : "transparent")
                        : Kirigami.Theme.disabledTextColor
                    opacity: rowItem.isActive ? 1.0 : 0.45
                }
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.width: rowItem.isSelected ? 1 : 0
                    border.color: Kirigami.Theme.highlightColor
                }

                // Drop indicator line during drag-to-reorder
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 3
                    color: Kirigami.Theme.highlightColor
                    z: 20
                    visible: root.dragSourceIdx >= 0 && root.dragTargetIdx === rowItem.index && root.dragSourceIdx !== rowItem.index
                    y: root.dragTargetIdx > root.dragSourceIdx ? parent.height - 2 : 0
                }

                // Click / Right-click handler for row
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        root.selectedPluginId = rowItem.pid;
                        if (mouse.button === Qt.RightButton) {
                            ctxMenu.openFor(rowItem.modelData, rowItem.index);
                        }
                    }
                    onDoubleClicked: function(mouse) {
                        if (mouse.button === Qt.LeftButton && !rowItem.isUnavailable) {
                            root.openPluginEdit(rowItem.pid);
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 11
                    anchors.rightMargin: 8
                    spacing: 6

                    // 1. Drag handle
                    Controls.Label {
                        id: gripLabel
                        text: "⠿"
                        color: rowItem.isActive ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                        opacity: rowItem.isActive ? 0.6 : 0.35
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignVCenter

                        Controls.ToolTip.visible: gripMouse.containsMouse
                        Controls.ToolTip.text: qsTr("Drag to reorder")

                        MouseArea {
                            id: gripMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            onPressed: {
                                root.dragSourceIdx = rowItem.index;
                                root.dragTargetIdx = rowItem.index;
                            }
                            onPositionChanged: function(mouse) {
                                if (!pressed || root.dragSourceIdx < 0) return;
                                var pt = gripMouse.mapToItem(rackList, mouse.x, mouse.y);
                                var target = rackList.indexAt(10, pt.y + rackList.contentY);
                                if (target >= 0) {
                                    root.dragTargetIdx = target;
                                } else if (pt.y < 0) {
                                    root.dragTargetIdx = 0;
                                } else {
                                    root.dragTargetIdx = root.rackItems.length - 1;
                                }
                            }
                            onReleased: {
                                if (root.dragSourceIdx >= 0 && root.dragTargetIdx >= 0 && root.dragSourceIdx !== root.dragTargetIdx) {
                                    root.controller.move_plugin(root.dragSourceIdx, root.dragTargetIdx);
                                }
                                root.dragSourceIdx = -1;
                                root.dragTargetIdx = -1;
                            }
                            onCanceled: {
                                root.dragSourceIdx = -1;
                                root.dragTargetIdx = -1;
                            }
                        }
                    }

                    // 2. Active / Bypass CheckBox
                    Controls.CheckBox {
                        enabled: !rowItem.isUnavailable
                        checked: !!rowItem.modelData.active
                        onToggled: root.controller.set_plugin_active(rowItem.pid, checked)
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.text: rowItem.modelData.active ? qsTr("Bypass effect") : qsTr("Enable effect")
                    }

                    // 3. Fixed 220px elided Plugin Name column
                    Controls.Label {
                        Layout.preferredWidth: 220
                        Layout.minimumWidth: 220
                        Layout.maximumWidth: 220
                        text: rowItem.modelData.name
                        elide: Text.ElideRight
                        font.bold: rowItem.isSelected
                        color: rowItem.isActive ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                        opacity: rowItem.isActive ? 1.0 : 0.72
                        Controls.ToolTip.visible: nameHover.containsMouse
                        Controls.ToolTip.text: rowItem.modelData.name
                        MouseArea {
                            id: nameHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    // 4. Aligned button column right next to plugin name: [Editor] [Parameters] [⋯]
                    Controls.ToolButton {
                        icon.name: "window-new"
                        enabled: !!rowItem.modelData.has_custom_ui && !rowItem.isUnavailable
                        checkable: true
                        checked: !!rowItem.modelData.custom_ui_visible
                        opacity: rowItem.isActive ? 1.0 : 0.45
                        onClicked: root.controller.toggle_custom_ui(rowItem.pid)
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.text: rowItem.modelData.custom_ui_failed
                            ? qsTr("Editor failed. See Logs for details.")
                            : qsTr("Plugin Editor")
                    }

                    Controls.ToolButton {
                        icon.name: "view-media-equalizer"
                        enabled: !rowItem.isUnavailable
                        opacity: rowItem.isActive ? 1.0 : 0.45
                        onClicked: root.openPluginEdit(rowItem.pid)
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.text: qsTr("Parameters")
                    }

                    Controls.ToolButton {
                        icon.name: "application-menu"
                        opacity: rowItem.isActive ? 1.0 : 0.45
                        onClicked: ctxMenu.openFor(rowItem.modelData, rowItem.index)
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.text: qsTr("Plugin actions")
                    }

                    // Flexible spacer
                    Item { Layout.fillWidth: true }

                    // 5. Format / Unavailable label
                    Controls.Label {
                        Layout.preferredWidth: 82
                        horizontalAlignment: Text.AlignRight
                        text: rowItem.isUnavailable ? qsTr("Unavailable") : rowItem.modelData.format
                        color: rowItem.isUnavailable ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
                        opacity: (rowItem.isActive || rowItem.isUnavailable) ? 1.0 : 0.55
                        font.bold: rowItem.isUnavailable
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        Controls.ToolTip.visible: rowItem.isUnavailable && fmtHover.containsMouse
                        Controls.ToolTip.text: rowItem.modelData.unavailable_error || ""
                        MouseArea {
                            id: fmtHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    // 6. Yellow repeated-clip warning triangle
                    ClipWarning {
                        active: rowItem.meter ? !!rowItem.meter.clip_warning : false
                        onDismissed: root.controller.dismiss_clip_warning(rowItem.pid)
                    }

                    // 7. Stacked Input (green) & Output (blue) peak meters
                    ColumnLayout {
                        spacing: 3
                        opacity: rowItem.isActive ? 1.0 : 0.35
                        PeakMeter {
                            Layout.preferredWidth: 62
                            Layout.preferredHeight: 12
                            channels: rowItem.modelData.in_channels || 0
                            baseColor: "#4ADE80"
                            levelL: rowItem.meter ? rowItem.meter.in_l : 0.0
                            levelR: rowItem.meter ? rowItem.meter.in_r : 0.0
                            clipL: rowItem.meter ? !!rowItem.meter.in_clip_l : false
                            clipR: rowItem.meter ? !!rowItem.meter.in_clip_r : false
                            tooltipText: qsTr("Input level")
                        }
                        PeakMeter {
                            Layout.preferredWidth: 62
                            Layout.preferredHeight: 12
                            channels: rowItem.modelData.out_channels || 0
                            baseColor: "#38BDF8"
                            levelL: rowItem.meter ? rowItem.meter.out_l : 0.0
                            levelR: rowItem.meter ? rowItem.meter.out_r : 0.0
                            clipL: rowItem.meter ? !!rowItem.meter.out_clip_l : false
                            clipR: rowItem.meter ? !!rowItem.meter.out_clip_r : false
                            tooltipText: qsTr("Output level")
                        }
                    }
                }
            }
        }
    }

    // Native KDE Plugin Context Menu
    Controls.Menu {
        id: ctxMenu
        property var targetSlot: null
        property int targetIdx: -1

        function openFor(slot, idx) {
            targetSlot = slot;
            targetIdx = idx;
            popup();
        }

        Controls.MenuItem {
            text: (ctxMenu.targetSlot && ctxMenu.targetSlot.active) ? qsTr("Bypass") : qsTr("Enable")
            enabled: ctxMenu.targetSlot && !ctxMenu.targetSlot.unavailable_error
            onTriggered: root.controller.set_plugin_active(ctxMenu.targetSlot.plugin_id, !ctxMenu.targetSlot.active)
        }
        Controls.MenuSeparator {}
        Controls.MenuItem {
            text: qsTr("Move Up")
            icon.name: "go-up"
            enabled: ctxMenu.targetIdx > 0
            onTriggered: root.controller.move_plugin(ctxMenu.targetSlot.plugin_id, ctxMenu.targetIdx - 1)
        }
        Controls.MenuItem {
            text: qsTr("Move Down")
            icon.name: "go-down"
            enabled: ctxMenu.targetIdx >= 0 && ctxMenu.targetIdx + 1 < root.rackItems.length
            onTriggered: root.controller.move_plugin(ctxMenu.targetSlot.plugin_id, ctxMenu.targetIdx + 1)
        }
        Controls.MenuSeparator {}

        // Set Category submenu with color swatches
        Controls.Menu {
            title: qsTr("Set Category")
            icon.name: "color-management"
            Repeater {
                model: root.categories
                delegate: Controls.MenuItem {
                    required property var modelData
                    checkable: true
                    checked: ctxMenu.targetSlot && ctxMenu.targetSlot.category === modelData.id
                    text: modelData.label
                    contentItem: RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 12
                            height: 12
                            radius: 2
                            color: modelData.color.length > 0 ? modelData.color : "transparent"
                            border.width: 1
                            border.color: modelData.color.length > 0 ? Qt.rgba(0, 0, 0, 0.35) : Kirigami.Theme.disabledTextColor
                        }
                        Controls.Label {
                            text: (ctxMenu.targetSlot && ctxMenu.targetSlot.category === modelData.id ? "✓ " : "") + modelData.label
                            Layout.fillWidth: true
                        }
                    }
                    onTriggered: root.controller.set_plugin_category(ctxMenu.targetSlot.plugin_id, modelData.id)
                }
            }
        }

        Controls.MenuSeparator {}
        Controls.MenuItem {
            text: qsTr("Reset Parameters")
            icon.name: "edit-undo"
            enabled: ctxMenu.targetSlot && !ctxMenu.targetSlot.unavailable_error
            onTriggered: root.controller.reset_parameters(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuItem {
            text: qsTr("Randomize Parameters")
            enabled: ctxMenu.targetSlot && !ctxMenu.targetSlot.unavailable_error
            onTriggered: root.controller.randomize_parameters(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuSeparator {}
        Controls.MenuItem {
            text: qsTr("Edit Parameters")
            icon.name: "view-media-equalizer"
            enabled: ctxMenu.targetSlot && !ctxMenu.targetSlot.unavailable_error
            onTriggered: root.openPluginEdit(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuItem {
            text: (ctxMenu.targetSlot && ctxMenu.targetSlot.custom_ui_visible) ? qsTr("Hide Custom UI") : qsTr("Show Custom UI")
            icon.name: "window-new"
            enabled: ctxMenu.targetSlot && ctxMenu.targetSlot.has_custom_ui && !ctxMenu.targetSlot.unavailable_error
            onTriggered: root.controller.toggle_custom_ui(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuSeparator {}
        Controls.MenuItem {
            text: qsTr("Clone")
            icon.name: "edit-copy"
            onTriggered: root.controller.clone_plugin(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuItem {
            text: qsTr("Rename…")
            icon.name: "edit-rename"
            onTriggered: root.openRenamePlugin(ctxMenu.targetSlot.plugin_id, ctxMenu.targetSlot.name)
        }
        Controls.MenuItem {
            text: qsTr("Replace…")
            icon.name: "document-swap"
            onTriggered: root.openReplacePlugin(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuItem {
            text: qsTr("Remove")
            icon.name: "edit-delete"
            onTriggered: root.controller.remove_plugin(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuSeparator { visible: root.controller.export_lv2_enabled }
        Controls.MenuItem {
            visible: root.controller.export_lv2_enabled
            text: qsTr("Export as LV2…")
            onTriggered: root.controller.export_plugin_lv2(ctxMenu.targetSlot.plugin_id)
        }
        Controls.MenuSeparator {}
        Controls.MenuItem {
            text: qsTr("Find in Patchbay")
            icon.name: "edit-find"
            onTriggered: root.controller.find_plugin_in_patchbay(ctxMenu.targetSlot.plugin_id)
        }
    }
}
