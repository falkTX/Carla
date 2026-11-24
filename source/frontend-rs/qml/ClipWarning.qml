// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls

Item {
    id: root
    property bool active: false
    signal dismissed()

    implicitWidth: 18
    implicitHeight: 18

    onActiveChanged: triCanvas.requestPaint()

    Controls.ToolTip.visible: active && area.containsMouse
    Controls.ToolTip.text: qsTr("Audio is repeatedly hitting 0 dB (clipping). Click to dismiss.")

    Canvas {
        id: triCanvas
        anchors.fill: parent
        visible: root.active
        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;
            ctx.clearRect(0, 0, w, h);
            if (!root.active) return;

            ctx.beginPath();
            ctx.moveTo(w * 0.5, 1.5);
            ctx.lineTo(w - 1.5, h - 1.5);
            ctx.lineTo(1.5, h - 1.5);
            ctx.closePath();
            ctx.fillStyle = "#FACC15";
            ctx.fill();
            ctx.lineWidth = 1.3;
            ctx.strokeStyle = "#92400E";
            ctx.lineJoin = "round";
            ctx.stroke();

            // Exclamation mark
            ctx.beginPath();
            ctx.moveTo(w * 0.5, 6.0);
            ctx.lineTo(w * 0.5, h - 6.2);
            ctx.lineWidth = 1.8;
            ctx.lineCap = "round";
            ctx.strokeStyle = "#1F2937";
            ctx.stroke();

            ctx.beginPath();
            ctx.arc(w * 0.5, h - 3.9, 1.15, 0, Math.PI * 2);
            ctx.fillStyle = "#1F2937";
            ctx.fill();
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.active ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (root.active) root.dismissed();
        }
    }
}
