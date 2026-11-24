// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Controls.Dialog {
    id: dlg
    required property var controller
    property var rtInfo: ({ name: "", buffer_size: 512, buffer_sizes: [], sample_rate: 44100, sample_rates: [], variable_buffer: false, variable_rate: false, has_control_panel: false })

    title: qsTr("Runtime Audio Driver Settings")
    modal: true
    anchors.centerIn: Controls.Overlay.overlay
    standardButtons: Controls.Dialog.Apply | Controls.Dialog.Close
    width: 380

    function openRuntime() {
        try {
            rtInfo = JSON.parse(controller.get_runtime_driver_json());
        } catch (e) {}
        open();
    }

    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 10
        rowSpacing: 8

        Controls.Label { text: qsTr("Device:") }
        Controls.Label { text: dlg.rtInfo.name || qsTr("Default"); font.bold: true }

        Controls.Label { text: qsTr("Buffer Size:") }
        Controls.ComboBox {
            id: rtBufCombo
            Layout.fillWidth: true
            enabled: !!dlg.rtInfo.variable_buffer && (dlg.rtInfo.buffer_sizes || []).length > 0
            model: dlg.rtInfo.buffer_sizes || []
            currentIndex: Math.max(0, (dlg.rtInfo.buffer_sizes || []).indexOf(dlg.rtInfo.buffer_size))
        }

        Controls.Label { text: qsTr("Sample Rate:") }
        Controls.ComboBox {
            id: rtSrCombo
            Layout.fillWidth: true
            enabled: !!dlg.rtInfo.variable_rate && (dlg.rtInfo.sample_rates || []).length > 0
            model: (dlg.rtInfo.sample_rates || []).map(function(r) { return r + " Hz"; })
            currentIndex: Math.max(0, (dlg.rtInfo.sample_rates || []).indexOf(dlg.rtInfo.sample_rate))
        }

        Controls.Button {
            Layout.columnSpan: 2
            visible: !!dlg.rtInfo.has_control_panel
            text: qsTr("Open Device Control Panel")
            onClicked: dlg.controller.open_device_control_panel()
        }
    }

    onApplied: {
        if (rtBufCombo.enabled && rtBufCombo.currentIndex >= 0) {
            controller.apply_buffer_size(dlg.rtInfo.buffer_sizes[rtBufCombo.currentIndex]);
        }
        if (rtSrCombo.enabled && rtSrCombo.currentIndex >= 0) {
            controller.apply_sample_rate(dlg.rtInfo.sample_rates[rtSrCombo.currentIndex]);
        }
        close();
    }
}
