// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Item {
    id: root
    property int channels: 2
    property real levelL: 0.0
    property real levelR: 0.0
    property bool clipL: false
    property bool clipR: false
    property bool vertical: false
    property color baseColor: "#4ADE80"
    property color hotColor: "#FACC15"
    property color clipColor: "#DC2626"
    property color clipCapColor: "#EF4444"
    property string tooltipText: ""

    onLevelLChanged: canvas.requestPaint()
    onLevelRChanged: canvas.requestPaint()
    onClipLChanged: canvas.requestPaint()
    onClipRChanged: canvas.requestPaint()
    onChannelsChanged: canvas.requestPaint()

    Controls.ToolTip.visible: hoverArea.containsMouse && tooltipText.length > 0
    Controls.ToolTip.text: tooltipText

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;
            ctx.clearRect(0, 0, w, h);

            // Background well
            ctx.fillStyle = Qt.darker(Kirigami.Theme.backgroundColor, 1.35);
            ctx.fillRect(0, 0, w, h);
            ctx.strokeStyle = Qt.alpha(Kirigami.Theme.textColor, 0.2);
            ctx.lineWidth = 1;
            ctx.strokeRect(0.5, 0.5, w - 1, h - 1);

            var chCount = Math.max(0, Math.min(2, root.channels));
            if (chCount === 0) return;

            var levels = [root.levelL, root.levelR];
            var clips = [root.clipL, root.clipR];

            if (!root.vertical) {
                var innerW = Math.max(1, w - 2);
                var innerH = Math.max(1, h - 2);
                var gap = chCount > 1 ? 1 : 0;
                var chH = (innerH - gap * (chCount - 1)) / chCount;
                for (var i = 0; i < chCount; i++) {
                    var y0 = 1 + i * (chH + gap);
                    var lv = Math.max(0, Math.min(1, levels[i]));
                    if (clips[i]) {
                        var fw = Math.max(3, innerW * Math.max(lv, 0.95));
                        ctx.fillStyle = root.clipColor;
                        ctx.fillRect(1, y0, fw, chH);
                        var capW = Math.min(3, innerW);
                        ctx.fillStyle = root.clipCapColor;
                        ctx.fillRect(1 + innerW - capW, y0, capW, chH);
                    } else if (lv > 0.001) {
                        var vis = Math.sqrt(lv);
                        ctx.fillStyle = lv >= 0.85 ? root.hotColor : root.baseColor;
                        ctx.fillRect(1, y0, innerW * vis, chH);
                    }
                }
            } else {
                var iw = Math.max(1, w - 2);
                var ih = Math.max(1, h - 2);
                var vgap = chCount > 1 ? 2 : 0;
                var chW = (iw - vgap * (chCount - 1)) / chCount;
                for (var j = 0; j < chCount; j++) {
                    var x0 = 1 + j * (chW + vgap);
                    var vlv = Math.max(0, Math.min(1, levels[j]));
                    if (clips[j]) {
                        var fh = Math.max(4, ih * Math.max(vlv, 0.95));
                        ctx.fillStyle = root.clipColor;
                        ctx.fillRect(x0, 1 + ih - fh, chW, fh);
                        var capH = Math.min(3, ih);
                        ctx.fillStyle = root.clipCapColor;
                        ctx.fillRect(x0, 1, chW, capH);
                    } else if (vlv > 0.001) {
                        var vvis = Math.sqrt(vlv);
                        var barH = ih * vvis;
                        ctx.fillStyle = vlv >= 0.85 ? root.hotColor : root.baseColor;
                        ctx.fillRect(x0, 1 + ih - barH, chW, barH);
                    }
                }
            }
        }
    }
}
