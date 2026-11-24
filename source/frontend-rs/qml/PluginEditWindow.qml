// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ApplicationWindow {
    id: win
    required property var controller
    property int pluginId: -1
    property var editData: ({})
    property string paramFilter: ""

    readonly property var midiCcItems: [
        { cc: -1, label: "None" },
        { cc: 132, label: "MIDI Learn…" },
        { cc: 131, label: "Pitchbend" },
        { cc: 130, label: "CV" },
        { cc: 1, label: "01 [0x01] Modulation" },
        { cc: 2, label: "02 [0x02] Breath" },
        { cc: 4, label: "04 [0x04] Foot" },
        { cc: 5, label: "05 [0x05] Portamento" },
        { cc: 7, label: "07 [0x07] Volume" },
        { cc: 8, label: "08 [0x08] Balance" },
        { cc: 10, label: "10 [0x0A] Pan" },
        { cc: 11, label: "11 [0x0B] Expression" },
        { cc: 70, label: "70 [0x46] Variation" },
        { cc: 71, label: "71 [0x47] Timbre" },
        { cc: 72, label: "72 [0x48] Release" },
        { cc: 73, label: "73 [0x49] Attack" },
        { cc: 74, label: "74 [0x4A] Brightness" },
        { cc: 91, label: "91 [0x5B] Reverb" },
        { cc: 93, label: "93 [0x5D] Chorus" }
    ]

    title: (editData.name ? editData.name + " — " : "") + qsTr("Parameters")
    width: 620
    height: 540
    minimumWidth: 480
    minimumHeight: 380

    function openForPlugin(pid) {
        pluginId = pid;
        refreshData();
        show();
        raise();
        requestActivate();
    }

    function refreshData() {
        if (pluginId < 0) return;
        try {
            var parsed = JSON.parse(controller.get_plugin_edit_json(pluginId));
            if (!parsed.name) {
                close();
                return;
            }
            editData = parsed;
        } catch (e) {}
    }

    function filteredParams() {
        var list = editData.params || [];
        var q = paramFilter.trim().toLowerCase();
        if (!q) return list;
        return list.filter(function(p) {
            return p.name.toLowerCase().indexOf(q) >= 0 || p.group_name.toLowerCase().indexOf(q) >= 0;
        });
    }

    Connections {
        target: win.controller
        function onPlugin_edit_revisionChanged() {
            if (win.visible) win.refreshData();
        }
        function onRack_jsonChanged() {
            if (win.visible) win.refreshData();
        }
    }

    // Periodic refresh for live output parameters while visible
    Timer {
        interval: 250
        running: win.visible && win.controller.engine_running
        repeat: true
        onTriggered: win.refreshData()
    }

    header: Controls.ToolBar {
        RowLayout {
            anchors.fill: parent
            Controls.TabBar {
                id: editTabs
                Controls.TabButton { text: qsTr("Parameters") }
                Controls.TabButton { text: qsTr("Info & Options") }
                Controls.TabButton {
                    text: qsTr("Programs")
                    enabled: !!((win.editData.programs && win.editData.programs.length > 0)
                             || (win.editData.midi_programs && win.editData.midi_programs.length > 0))
                }
                Controls.TabButton { text: qsTr("Keyboard") }
            }
            Item { Layout.fillWidth: true }
            Controls.ToolButton {
                text: qsTr("Reset")
                icon.name: "edit-undo"
                onClicked: win.controller.reset_parameters(win.pluginId)
            }
            Controls.ToolButton {
                text: qsTr("Randomize")
                onClicked: win.controller.randomize_parameters(win.pluginId)
            }
            Controls.ToolButton {
                text: qsTr("Load…")
                icon.name: "document-open"
                onClicked: win.controller.load_plugin_state(win.pluginId)
            }
            Controls.ToolButton {
                text: qsTr("Save…")
                icon.name: "document-save"
                onClicked: win.controller.save_plugin_state(win.pluginId)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Internal Controls Bar (Active, Dry/Wet, Volume, Balance/Pan)
        Flow {
            Layout.fillWidth: true
            spacing: 12

            Controls.CheckBox {
                text: qsTr("Active")
                checked: !!win.editData.active
                onToggled: win.controller.set_plugin_active(win.pluginId, checked)
            }

            RowLayout {
                visible: !!win.editData.can_drywet
                spacing: 4
                Controls.Label { text: qsTr("Dry/Wet:") }
                Controls.Slider {
                    implicitWidth: 100
                    from: 0.0; to: 1.0
                    value: win.editData.drywet !== undefined ? win.editData.drywet : 1.0
                    onMoved: win.controller.set_internal_param(win.pluginId, -3, value)
                }
                Controls.Label { text: Math.round((win.editData.drywet || 0) * 100) + "%" }
            }

            RowLayout {
                visible: !!win.editData.can_volume
                spacing: 4
                Controls.Label { text: qsTr("Volume:") }
                Controls.Slider {
                    implicitWidth: 100
                    from: 0.0; to: 1.27
                    value: win.editData.volume !== undefined ? win.editData.volume : 1.0
                    onMoved: win.controller.set_internal_param(win.pluginId, -4, value)
                }
                Controls.Label { text: Math.round((win.editData.volume || 0) * 100) + "%" }
            }

            RowLayout {
                visible: !!win.editData.can_balance
                spacing: 4
                Controls.Label { text: qsTr("Bal L:") }
                Controls.Slider {
                    implicitWidth: 80
                    from: -1.0; to: 1.0
                    value: win.editData.balance_left !== undefined ? win.editData.balance_left : -1.0
                    onMoved: win.controller.set_internal_param(win.pluginId, -5, value)
                }
                Controls.Label { text: qsTr("R:") }
                Controls.Slider {
                    implicitWidth: 80
                    from: -1.0; to: 1.0
                    value: win.editData.balance_right !== undefined ? win.editData.balance_right : 1.0
                    onMoved: win.controller.set_internal_param(win.pluginId, -6, value)
                }
            }

            RowLayout {
                visible: !win.editData.can_balance && !!win.editData.can_panning
                spacing: 4
                Controls.Label { text: qsTr("Pan:") }
                Controls.Slider {
                    implicitWidth: 100
                    from: -1.0; to: 1.0
                    value: win.editData.panning !== undefined ? win.editData.panning : 0.0
                    onMoved: win.controller.set_internal_param(win.pluginId, -7, value)
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: editTabs.currentIndex

            // Tab 0: Parameters
            ColumnLayout {
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Controls.TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Filter parameters…")
                        text: win.paramFilter
                        onTextChanged: win.paramFilter = text
                    }
                    Controls.ToolButton {
                        icon.name: "edit-clear"
                        visible: win.paramFilter.length > 0
                        onClicked: win.paramFilter = ""
                    }
                }

                Controls.ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ListView {
                        model: win.filteredParams()
                        spacing: 4
                        section.property: "group_name"
                        section.delegate: Controls.Label {
                            required property string section
                            text: section
                            font.bold: true
                            topPadding: 6
                            bottomPadding: 2
                            opacity: 0.8
                        }

                        delegate: RowLayout {
                            id: pRow
                            required property var modelData
                            width: ListView.view.width - 12
                            spacing: 6

                            Controls.Label {
                                Layout.preferredWidth: 180
                                elide: Text.ElideRight
                                text: modelData.unit ? (modelData.name + " (" + modelData.unit + ")") : modelData.name
                                opacity: modelData.is_output ? 0.75 : 1.0
                                Controls.ToolTip.visible: modelData.comment && pLblHover.containsMouse
                                Controls.ToolTip.text: modelData.comment || ""
                                MouseArea { id: pLblHover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                            }

                            // Boolean control
                            Controls.CheckBox {
                                visible: !!modelData.is_boolean
                                enabled: !modelData.is_read_only
                                checked: modelData.value >= 0.5 * (modelData.min + modelData.max)
                                onToggled: win.controller.set_param_value(win.pluginId, modelData.index, checked ? modelData.max : modelData.min)
                            }

                            // Scale points ComboBox
                            Controls.ComboBox {
                                visible: !modelData.is_boolean && !modelData.is_read_only && modelData.scale_points && modelData.scale_points.length > 0
                                Layout.fillWidth: true
                                model: modelData.scale_points || []
                                textRole: "label"
                                currentIndex: {
                                    var sps = modelData.scale_points || [];
                                    var best = 0, bestD = 1e9;
                                    for (var i = 0; i < sps.length; i++) {
                                        var d = Math.abs(sps[i].value - modelData.value);
                                        if (d < bestD) { bestD = d; best = i; }
                                    }
                                    return best;
                                }
                                onActivated: function(idx) {
                                    var sp = modelData.scale_points[idx];
                                    if (sp) win.controller.set_param_value(win.pluginId, modelData.index, sp.value);
                                }
                            }

                            // Continuous / Integer slider
                            Controls.Slider {
                                visible: !modelData.is_boolean && (modelData.is_read_only || !modelData.scale_points || modelData.scale_points.length === 0)
                                Layout.fillWidth: true
                                enabled: !modelData.is_read_only
                                from: modelData.min
                                to: modelData.max
                                stepSize: modelData.is_integer ? 1.0 : 0.0
                                value: modelData.value
                                onMoved: win.controller.set_param_value(win.pluginId, modelData.index, value)
                            }

                            // Value readout
                            Controls.Label {
                                Layout.preferredWidth: 74
                                horizontalAlignment: Text.AlignRight
                                font.family: "monospace"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: modelData.custom_text && modelData.custom_text.length > 0
                                    ? modelData.custom_text
                                    : (modelData.is_integer ? Math.round(modelData.value).toString() : modelData.value.toFixed(3))
                            }

                            // Reset default
                            Controls.ToolButton {
                                visible: !modelData.is_read_only
                                icon.name: "edit-undo"
                                Controls.ToolTip.visible: hovered
                                Controls.ToolTip.text: qsTr("Reset to default")
                                onClicked: win.controller.set_param_value(win.pluginId, modelData.index, modelData.def)
                            }

                            // MIDI CC mapping button
                            Controls.ToolButton {
                                visible: !modelData.is_read_only && !!modelData.is_automatable
                                text: {
                                    var cc = modelData.mapped_control_index;
                                    if (cc === -1) return "CC";
                                    if (cc === 130) return "CV";
                                    if (cc === 131) return "PB";
                                    if (cc === 132) return "Learn";
                                    return "CC" + cc;
                                }
                                onClicked: ccDialog.openForParam(modelData)
                            }
                        }
                    }
                }
            }

            // Tab 1: Info & Options
            Controls.ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width - 12
                    spacing: 8

                    GridLayout {
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 4
                        Controls.Label { text: qsTr("Format:"); opacity: 0.75 }
                        Controls.Label { text: win.editData.format || "" }
                        Controls.Label { text: qsTr("Name:"); opacity: 0.75 }
                        Controls.Label { text: win.editData.name || "" }
                        Controls.Label { text: qsTr("Label:"); opacity: 0.75 }
                        Controls.Label { text: win.editData.label || "" }
                        Controls.Label { text: qsTr("Maker:"); opacity: 0.75 }
                        Controls.Label { text: win.editData.maker || "" }
                        Controls.Label { text: qsTr("Copyright:"); opacity: 0.75 }
                        Controls.Label { text: win.editData.copyright || "" }
                        Controls.Label { text: qsTr("Unique ID:"); opacity: 0.75 }
                        Controls.Label { text: String(win.editData.unique_id || 0) }
                        Controls.Label { text: qsTr("Filename:"); opacity: 0.75 }
                        Controls.Label { text: win.editData.filename || ""; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
                        Controls.Label { text: qsTr("Audio I/O:"); opacity: 0.75 }
                        Controls.Label { text: (win.editData.audio_ins || 0) + " in / " + (win.editData.audio_outs || 0) + " out" }
                        Controls.Label { text: qsTr("MIDI I/O:"); opacity: 0.75 }
                        Controls.Label { text: (win.editData.midi_ins || 0) + " in / " + (win.editData.midi_outs || 0) + " out" }
                        Controls.Label { text: qsTr("Latency:"); opacity: 0.75 }
                        Controls.Label { text: (win.editData.latency || 0) + " frames" }
                        Controls.Label { text: qsTr("Control Channel:"); opacity: 0.75 }
                        Controls.ComboBox {
                            model: ["None","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"]
                            currentIndex: (win.editData.ctrl_channel !== undefined ? win.editData.ctrl_channel + 1 : 0)
                            onActivated: function(idx) { win.controller.set_ctrl_channel(win.pluginId, idx - 1); }
                        }
                    }

                    Kirigami.Separator { Layout.fillWidth: true }
                    Kirigami.Heading { text: qsTr("Plugin Options"); level: 4 }

                    Repeater {
                        model: win.editData.options || []
                        delegate: Controls.CheckBox {
                            required property var modelData
                            text: modelData.label
                            checked: !!modelData.enabled
                            onToggled: win.controller.set_plugin_option(win.pluginId, modelData.bit, checked)
                        }
                    }

                    Controls.Button {
                        text: qsTr("Reload Info & Parameters")
                        icon.name: "view-refresh"
                        onClicked: win.controller.reload_plugin_all(win.pluginId)
                    }
                }
            }

            // Tab 2: Programs
            ColumnLayout {
                spacing: 10
                Controls.Label {
                    visible: !!(win.editData.programs && win.editData.programs.length > 0)
                    text: qsTr("Programs:")
                    font.bold: true
                }
                Controls.ComboBox {
                    visible: !!(win.editData.programs && win.editData.programs.length > 0)
                    Layout.fillWidth: true
                    model: win.editData.programs || []
                    currentIndex: Math.max(0, win.editData.current_program || 0)
                    onActivated: function(idx) { win.controller.set_program(win.pluginId, idx); }
                }

                Controls.Label {
                    visible: !!(win.editData.midi_programs && win.editData.midi_programs.length > 0)
                    text: qsTr("MIDI Programs:")
                    font.bold: true
                }
                Controls.ComboBox {
                    visible: !!(win.editData.midi_programs && win.editData.midi_programs.length > 0)
                    Layout.fillWidth: true
                    model: win.editData.midi_programs || []
                    currentIndex: Math.max(0, win.editData.current_midi_program || 0)
                    onActivated: function(idx) { win.controller.set_midi_program(win.pluginId, idx); }
                }
                Item { Layout.fillHeight: true }
            }

            // Tab 3: Keyboard
            Item {
                PianoKeyboard {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    activeEnabled: !!win.editData.active
                    externalNotes: win.editData.active_notes || []
                    onNoteOn: function(ch, n, v) { win.controller.send_midi_note(win.pluginId, ch, n, v); }
                    onNoteOff: function(ch, n) { win.controller.send_midi_note(win.pluginId, ch, n, 0); }
                }
            }
        }
    }

    // MIDI CC / CV Mapping Dialog
    Controls.Dialog {
        id: ccDialog
        property var targetParam: null
        title: qsTr("MIDI / CV Mapping")
        modal: true
        anchors.centerIn: Controls.Overlay.overlay
        standardButtons: Controls.Dialog.Close
        width: 360

        function openForParam(p) {
            targetParam = p;
            var curCc = p.mapped_control_index;
            var idx = 0;
            for (var i = 0; i < win.midiCcItems.length; i++) {
                if (win.midiCcItems[i].cc === curCc) { idx = i; break; }
            }
            ccCombo.currentIndex = idx;
            ccChSpin.value = (p.midi_channel || 0) + 1;
            ccMinField.text = String(p.mapped_minimum !== undefined ? p.mapped_minimum : p.min);
            ccMaxField.text = String(p.mapped_maximum !== undefined ? p.mapped_maximum : p.max);
            open();
        }

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 10
            rowSpacing: 6

            Controls.Label { text: qsTr("Controller:") }
            Controls.ComboBox {
                id: ccCombo
                Layout.fillWidth: true
                model: win.midiCcItems
                textRole: "label"
                onActivated: function(i) {
                    if (ccDialog.targetParam) {
                        win.controller.set_param_mapped_control(win.pluginId, ccDialog.targetParam.index, win.midiCcItems[i].cc);
                    }
                }
            }

            Controls.Label { text: qsTr("MIDI Channel:") }
            Controls.SpinBox {
                id: ccChSpin
                from: 1; to: 16
                onValueModified: {
                    if (ccDialog.targetParam) {
                        win.controller.set_param_midi_channel(win.pluginId, ccDialog.targetParam.index, value - 1);
                    }
                }
            }

            Controls.Label { text: qsTr("Mapped Min / Max:") }
            RowLayout {
                Controls.TextField {
                    id: ccMinField
                    Layout.fillWidth: true
                    onEditingFinished: applyRange()
                }
                Controls.TextField {
                    id: ccMaxField
                    Layout.fillWidth: true
                    onEditingFinished: applyRange()
                }
                function applyRange() {
                    if (!ccDialog.targetParam) return;
                    var mn = parseFloat(ccMinField.text);
                    var mx = parseFloat(ccMaxField.text);
                    if (!isNaN(mn) && !isNaN(mx)) {
                        win.controller.set_param_mapped_range(win.pluginId, ccDialog.targetParam.index, mn, mx);
                    }
                }
            }
        }
    }
}
