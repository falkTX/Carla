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
    property var categoryColorMap: ({})

    // Shared table column widths (exact match between header and rows)
    readonly property int colFavW: 32
    readonly property int colFormatW: 78
    readonly property int colCatW: 118
    readonly property int colMakerW: 175
    readonly property int colIoW: 112
    readonly property int colScrollPad: 18

    title: replacePluginId >= 0 ? qsTr("Replace Plugin") : qsTr("Add Plugin")
    width: 920
    height: 620
    minimumWidth: 700
    minimumHeight: 460

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

    function toggleSort(col) {
        if (sortCol === col) {
            sortAsc = !sortAsc;
        } else {
            sortCol = col;
            sortAsc = true;
        }
        runQuery();
    }

    function selectByIndex(idx) {
        var items = queryResult.items || [];
        if (idx < 0 || idx >= items.length) return;
        selectedKey = items[idx].key;
        selectedPlugin = items[idx];
        pluginList.currentIndex = idx;
        pluginList.positionViewAtIndex(idx, ListView.Contain);
    }

    function runQuery() {
        try {
            var raw = controller.query_plugins_json(
                searchText, favoritesOnly, categoryFilter, formatsMask, typesMask, sortCol, sortAsc
            );
            queryResult = JSON.parse(raw);
            selectedPlugin = null;
            var items = queryResult.items || [];
            var foundIdx = -1;
            for (var i = 0; i < items.length; i++) {
                if (items[i].key === selectedKey) {
                    selectedPlugin = items[i];
                    foundIdx = i;
                    break;
                }
            }
            pluginList.currentIndex = foundIdx;
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
        try {
            categories = JSON.parse(controller.categories_json);
            var cmap = {};
            for (var i = 0; i < categories.length; i++) {
                if (categories[i].color) {
                    cmap[categories[i].label] = categories[i].color;
                }
            }
            categoryColorMap = cmap;
        } catch (e) {}
    }

    Connections {
        target: win.controller
        function onDiscovery_revisionChanged() {
            if (win.visible) win.runQuery();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
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
                Keys.onDownPressed: {
                    var items = win.queryResult.items || [];
                    if (items.length > 0) {
                        var next = pluginList.currentIndex >= 0 ? Math.min(items.length - 1, pluginList.currentIndex + 1) : 0;
                        win.selectByIndex(next);
                        pluginList.forceActiveFocus();
                    }
                }
                Keys.onReturnPressed: {
                    if (win.selectedPlugin) {
                        win.commitSelection(win.selectedPlugin);
                    } else if ((win.queryResult.items || []).length > 0) {
                        win.commitSelection(win.queryResult.items[0]);
                    }
                }
            }
            Controls.CheckBox {
                text: qsTr("★ Favorites")
                checked: win.favoritesOnly
                onToggled: { win.favoritesOnly = checked; win.runQuery(); }
            }
            Controls.Label { text: qsTr("Category:"); opacity: 0.8 }
            Controls.ComboBox {
                id: catCombo
                Layout.preferredWidth: 130
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

        // Aligned Format + Type filter grid
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 10
            rowSpacing: 4

            Controls.Label {
                text: qsTr("Format:")
                opacity: 0.72
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }
            Flow {
                Layout.fillWidth: true
                spacing: 8
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

            Controls.Label {
                text: qsTr("Type:")
                opacity: 0.72
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
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

        // Unified Table Frame (Header + Scrollable Plugin List)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.18)
            radius: 4
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0
                enabled: !win.queryResult.scanning
                opacity: win.queryResult.scanning ? 0.38 : 1.0

                // Integrated Sortable Table Header Bar
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    color: Kirigami.Theme.alternateBackgroundColor

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: win.colScrollPad
                        spacing: 8

                        // Favorite header spacer
                        Controls.Label {
                            Layout.preferredWidth: win.colFavW
                            Layout.minimumWidth: win.colFavW
                            Layout.maximumWidth: win.colFavW
                            horizontalAlignment: Text.AlignHCenter
                            text: "★"
                            opacity: 0.45
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }

                        // Name header
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Rectangle {
                                anchors.fill: parent
                                color: hdrNameMouse.containsMouse ? Qt.alpha(Kirigami.Theme.textColor, 0.06) : "transparent"
                            }
                            Controls.Label {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: win.sortCol === 0 ? 1.0 : 0.78
                                text: qsTr("Name") + (win.sortCol === 0 ? (win.sortAsc ? "  ▲" : "  ▼") : "")
                            }
                            MouseArea {
                                id: hdrNameMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: win.toggleSort(0)
                            }
                        }

                        // Format header
                        Item {
                            Layout.preferredWidth: win.colFormatW
                            Layout.minimumWidth: win.colFormatW
                            Layout.maximumWidth: win.colFormatW
                            Layout.fillHeight: true
                            Rectangle {
                                anchors.fill: parent
                                color: hdrFmtMouse.containsMouse ? Qt.alpha(Kirigami.Theme.textColor, 0.06) : "transparent"
                            }
                            Controls.Label {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: win.sortCol === 1 ? 1.0 : 0.78
                                text: qsTr("Format") + (win.sortCol === 1 ? (win.sortAsc ? " ▲" : " ▼") : "")
                            }
                            MouseArea {
                                id: hdrFmtMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: win.toggleSort(1)
                            }
                        }

                        // Category header
                        Item {
                            Layout.preferredWidth: win.colCatW
                            Layout.minimumWidth: win.colCatW
                            Layout.maximumWidth: win.colCatW
                            Layout.fillHeight: true
                            Rectangle {
                                anchors.fill: parent
                                color: hdrCatMouse.containsMouse ? Qt.alpha(Kirigami.Theme.textColor, 0.06) : "transparent"
                            }
                            Controls.Label {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: win.sortCol === 2 ? 1.0 : 0.78
                                text: qsTr("Category") + (win.sortCol === 2 ? (win.sortAsc ? " ▲" : " ▼") : "")
                            }
                            MouseArea {
                                id: hdrCatMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: win.toggleSort(2)
                            }
                        }

                        // Maker header
                        Item {
                            Layout.preferredWidth: win.colMakerW
                            Layout.minimumWidth: win.colMakerW
                            Layout.maximumWidth: win.colMakerW
                            Layout.fillHeight: true
                            Rectangle {
                                anchors.fill: parent
                                color: hdrMakerMouse.containsMouse ? Qt.alpha(Kirigami.Theme.textColor, 0.06) : "transparent"
                            }
                            Controls.Label {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: win.sortCol === 3 ? 1.0 : 0.78
                                text: qsTr("Maker") + (win.sortCol === 3 ? (win.sortAsc ? " ▲" : " ▼") : "")
                            }
                            MouseArea {
                                id: hdrMakerMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: win.toggleSort(3)
                            }
                        }

                        // I/O header
                        Controls.Label {
                            Layout.preferredWidth: win.colIoW
                            Layout.minimumWidth: win.colIoW
                            Layout.maximumWidth: win.colIoW
                            Layout.fillHeight: true
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.72
                            text: qsTr("Audio / MIDI")
                        }
                    }

                    // Bottom divider line
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: Qt.alpha(Kirigami.Theme.textColor, 0.16)
                    }
                }

                // Scrollable Plugin Rows
                ListView {
                    id: pluginList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: win.queryResult.items || []
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    focus: true

                    Controls.ScrollBar.vertical: Controls.ScrollBar {
                        policy: Controls.ScrollBar.AsNeeded
                    }

                    Keys.onUpPressed: {
                        if (currentIndex > 0) win.selectByIndex(currentIndex - 1);
                    }
                    Keys.onDownPressed: {
                        var items = win.queryResult.items || [];
                        if (currentIndex + 1 < items.length) win.selectByIndex(currentIndex + 1);
                    }
                    Keys.onReturnPressed: {
                        if (win.selectedPlugin) win.commitSelection(win.selectedPlugin);
                    }
                    Keys.onEnterPressed: {
                        if (win.selectedPlugin) win.commitSelection(win.selectedPlugin);
                    }

                    delegate: Item {
                        id: pluginRow
                        required property var modelData
                        required property int index

                        readonly property bool isSelected: win.selectedKey === modelData.key
                        readonly property string catHex: win.categoryColorMap[modelData.category] || ""

                        width: pluginList.width
                        height: 30

                        // Zebra stripe + hover + selection backdrop
                        Rectangle {
                            anchors.fill: parent
                            color: pluginRow.isSelected
                                ? Qt.alpha(Kirigami.Theme.highlightColor, 0.34)
                                : (rowMouse.containsMouse
                                    ? Qt.alpha(Kirigami.Theme.highlightColor, 0.13)
                                    : (pluginRow.index % 2 === 1
                                        ? Qt.alpha(Kirigami.Theme.alternateBackgroundColor, 0.75)
                                        : "transparent"))
                        }
                        // Crisp left accent bar + subtle border on selected row
                        Rectangle {
                            visible: pluginRow.isSelected
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 3
                            color: Kirigami.Theme.highlightColor
                        }
                        // Subtle bottom row hairline
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Qt.alpha(Kirigami.Theme.textColor, 0.05)
                        }

                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                win.selectedKey = pluginRow.modelData.key;
                                win.selectedPlugin = pluginRow.modelData;
                                pluginList.currentIndex = pluginRow.index;
                                pluginList.forceActiveFocus();
                            }
                            onDoubleClicked: win.commitSelection(pluginRow.modelData)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: win.colScrollPad
                            spacing: 8

                            // 1. Favorite star
                            Item {
                                Layout.preferredWidth: win.colFavW
                                Layout.minimumWidth: win.colFavW
                                Layout.maximumWidth: win.colFavW
                                Layout.fillHeight: true
                                Controls.Label {
                                    anchors.centerIn: parent
                                    text: pluginRow.modelData.favorite ? "★" : "☆"
                                    font.pixelSize: 14
                                    color: pluginRow.modelData.favorite
                                        ? "#FBBF24"
                                        : (favMouse.containsMouse ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor)
                                    opacity: pluginRow.modelData.favorite ? 1.0 : (favMouse.containsMouse ? 0.9 : 0.45)
                                }
                                MouseArea {
                                    id: favMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        win.controller.toggle_favorite(pluginRow.modelData.key);
                                        win.runQuery();
                                    }
                                }
                            }

                            // 2. Plugin Name
                            Controls.Label {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                verticalAlignment: Text.AlignVCenter
                                text: pluginRow.modelData.name
                                elide: Text.ElideRight
                                font.bold: pluginRow.isSelected
                                color: Kirigami.Theme.textColor
                            }

                            // 3. Format badge
                            Item {
                                Layout.preferredWidth: win.colFormatW
                                Layout.minimumWidth: win.colFormatW
                                Layout.maximumWidth: win.colFormatW
                                Layout.fillHeight: true
                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    height: 20
                                    width: Math.min(parent.width, fmtLabel.implicitWidth + 12)
                                    radius: 3
                                    color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                                    border.width: 1
                                    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
                                    Controls.Label {
                                        id: fmtLabel
                                        anchors.centerIn: parent
                                        text: pluginRow.modelData.format
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                                        opacity: 0.9
                                    }
                                }
                            }

                            // 4. Category with color dot
                            RowLayout {
                                Layout.preferredWidth: win.colCatW
                                Layout.minimumWidth: win.colCatW
                                Layout.maximumWidth: win.colCatW
                                Layout.fillHeight: true
                                spacing: 6
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    Layout.alignment: Qt.AlignVCenter
                                    visible: pluginRow.catHex.length > 0
                                    color: pluginRow.catHex.length > 0 ? pluginRow.catHex : "transparent"
                                }
                                Controls.Label {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    verticalAlignment: Text.AlignVCenter
                                    text: pluginRow.modelData.category
                                    elide: Text.ElideRight
                                    opacity: 0.85
                                }
                            }

                            // 5. Maker
                            Controls.Label {
                                Layout.preferredWidth: win.colMakerW
                                Layout.minimumWidth: win.colMakerW
                                Layout.maximumWidth: win.colMakerW
                                Layout.fillHeight: true
                                verticalAlignment: Text.AlignVCenter
                                text: pluginRow.modelData.maker || "—"
                                elide: Text.ElideRight
                                opacity: 0.78
                            }

                            // 6. Audio / MIDI I/O
                            Controls.Label {
                                Layout.preferredWidth: win.colIoW
                                Layout.minimumWidth: win.colIoW
                                Layout.maximumWidth: win.colIoW
                                Layout.fillHeight: true
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                                font.family: "monospace"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.78
                                text: "A " + pluginRow.modelData.audio_ins + "/" + pluginRow.modelData.audio_outs
                                    + "  M " + pluginRow.modelData.midi_ins + "/" + pluginRow.modelData.midi_outs
                            }
                        }
                    }
                }
            }

            // Greyed-out overlay while scanning plugins
            Rectangle {
                anchors.fill: parent
                z: 50
                visible: !!win.queryResult.scanning
                color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.45)

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.AllButtons
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Controls.BusyIndicator {
                        Layout.alignment: Qt.AlignHCenter
                        running: !!win.queryResult.scanning
                    }
                    Controls.Label {
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        text: qsTr("Scanning plugins…")
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
                enabled: !!win.selectedPlugin && !win.queryResult.scanning
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
        width: 460

        contentItem: ColumnLayout {
            spacing: 10
            Controls.Label { text: qsTr("Select plugin formats to scan:") }
            GridLayout {
                Layout.fillWidth: true
                columns: 5
                columnSpacing: 10
                rowSpacing: 6
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
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Qt.alpha(Kirigami.Theme.textColor, 0.14)
            }
            Controls.CheckBox {
                id: rescanAllCb
                Layout.fillWidth: true
                text: qsTr("Ignore cache and rescan all files")
            }
        }

        onAccepted: {
            win.controller.start_discovery(refreshModal.scanMask, rescanAllCb.checked);
        }
    }
}
