// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: root
    property var sceneData: ({ canvas_width: 3100, canvas_height: 2400, boxes: [], connections: [] })
    property real panX: 1550
    property real panY: 1200
    property real zoom: 1.0
    property real viewportW: 800
    property real viewportH: 600

    signal panRequested(real sceneX, real sceneY)

    implicitHeight: 130

    onSceneDataChanged: miniCanvas.requestPaint()
    onPanXChanged: miniCanvas.requestPaint()
    onPanYChanged: miniCanvas.requestPaint()
    onZoomChanged: miniCanvas.requestPaint()
    onViewportWChanged: miniCanvas.requestPaint()
    onViewportHChanged: miniCanvas.requestPaint()

    Canvas {
        id: miniCanvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;
            ctx.clearRect(0, 0, w, h);

            ctx.fillStyle = Qt.darker(Kirigami.Theme.backgroundColor, 1.25);
            ctx.fillRect(0, 0, w, h);
            ctx.strokeStyle = Qt.alpha(Kirigami.Theme.textColor, 0.25);
            ctx.lineWidth = 1;
            ctx.strokeRect(0.5, 0.5, w - 1, h - 1);

            var data = root.sceneData;
            if (!data) return;
            var cw = Math.max(1, data.canvas_width || 3100);
            var ch = Math.max(1, data.canvas_height || 2400);
            var sx = w / cw;
            var sy = h / ch;

            // Connections
            var conns = data.connections || [];
            ctx.lineWidth = 1;
            for (var i = 0; i < conns.length; i++) {
                var c = conns[i];
                ctx.strokeStyle = c.color || "#60A5FA";
                ctx.beginPath();
                ctx.moveTo(c.x1 * sx, c.y1 * sy);
                ctx.lineTo(c.x2 * sx, c.y2 * sy);
                ctx.stroke();
            }

            // Boxes
            var boxes = data.boxes || [];
            for (var j = 0; j < boxes.length; j++) {
                var b = boxes[j];
                ctx.fillStyle = b.selected
                    ? Kirigami.Theme.highlightColor
                    : Qt.alpha(Kirigami.Theme.textColor, 0.55);
                ctx.fillRect(b.rect.x * sx, b.rect.y * sy, Math.max(2, b.rect.w * sx), Math.max(2, b.rect.h * sy));
            }

            // Viewport rect
            var z = Math.max(0.1, root.zoom);
            var vw = root.viewportW / z;
            var vh = root.viewportH / z;
            var vx = (root.panX - vw * 0.5) * sx;
            var vy = (root.panY - vh * 0.5) * sy;
            ctx.strokeStyle = "#FACC15";
            ctx.lineWidth = 1.4;
            ctx.strokeRect(vx, vy, vw * sx, vh * sy);
        }

        MouseArea {
            anchors.fill: parent
            function moveTo(mx, my) {
                var data = root.sceneData;
                var cw = (data && data.canvas_width) ? data.canvas_width : 3100;
                var ch = (data && data.canvas_height) ? data.canvas_height : 2400;
                var sx = Math.max(0, Math.min(cw, (mx / Math.max(1, width)) * cw));
                var sy = Math.max(0, Math.min(ch, (my / Math.max(1, height)) * ch));
                root.panRequested(sx, sy);
            }
            onPressed: function(mouse) { moveTo(mouse.x, mouse.y); }
            onPositionChanged: function(mouse) { if (pressed) moveTo(mouse.x, mouse.y); }
        }
    }
}
