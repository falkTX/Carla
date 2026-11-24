// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root
    required property var controller
    property var sceneData: ({ canvas_width: 3100, canvas_height: 2400, use_bezier: true, boxes: [], connections: [] })
    property var canvasMeters: ({ in_l: 0, in_r: 0, in_clip_l: false, in_clip_r: false, out_l: 0, out_r: 0, out_clip_l: false, out_clip_r: false })
    property var canvasNotes: []

    property real panX: 1550
    property real panY: 1200
    property real zoom: 1.0
    property bool needsInitialFit: true
    readonly property real canvasViewportW: pbCanvas.width
    readonly property real canvasViewportH: pbCanvas.height

    // Interactive drag states
    property string dragMode: "" // "pan" | "boxes" | "port" | "rubberband" | "zoomrect" | "knife"
    property real dragStartSceneX: 0
    property real dragStartSceneY: 0
    property real dragCurSceneX: 0
    property real dragCurSceneY: 0
    property real dragStartScreenX: 0
    property real dragStartScreenY: 0
    property real panOriginX: 0
    property real panOriginY: 0
    property var boxOrigins: ({})
    property var dragSourcePort: null

    signal openPluginEdit(int pluginId)
    signal openRenamePlugin(int pluginId, string currentName)
    signal openReplacePlugin(int pluginId)

    spacing: 0

    function parseScene() {
        try {
            sceneData = JSON.parse(controller.patchbay_json);
            if (needsInitialFit && sceneData.boxes && sceneData.boxes.length > 0 && pbCanvas.width > 50) {
                zoomFit();
                needsInitialFit = false;
            }
            pbCanvas.requestPaint();
        } catch (e) {}
    }

    function parseMeters() {
        try {
            var m = JSON.parse(controller.meters_json);
            if (m.canvas) canvasMeters = m.canvas;
            canvasNotes = m.canvas_notes || [];
        } catch (e) {}
    }

    function screenToScene(sx, sy) {
        return {
            x: panX + (sx - pbCanvas.width * 0.5) / zoom,
            y: panY + (sy - pbCanvas.height * 0.5) / zoom
        };
    }

    function sceneToScreen(px, py) {
        return {
            x: pbCanvas.width * 0.5 + (px - panX) * zoom,
            y: pbCanvas.height * 0.5 + (py - panY) * zoom
        };
    }

    function zoomFit() {
        var boxes = sceneData ? (sceneData.boxes || []) : [];
        if (boxes.length === 0) {
            panX = (sceneData ? sceneData.canvas_width : 3100) * 0.5;
            panY = (sceneData ? sceneData.canvas_height : 2400) * 0.5;
            zoom = 1.0;
            pbCanvas.requestPaint();
            return;
        }
        var minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
        for (var i = 0; i < boxes.length; i++) {
            var r = boxes[i].rect;
            minX = Math.min(minX, r.x);
            minY = Math.min(minY, r.y);
            maxX = Math.max(maxX, r.x + r.w);
            maxY = Math.max(maxY, r.y + r.h);
        }
        var pad = 48;
        var bw = Math.max(1, (maxX - minX) + pad * 2);
        var bh = Math.max(1, (maxY - minY) + pad * 2);
        var vw = Math.max(200, pbCanvas.width);
        var vh = Math.max(200, pbCanvas.height);
        zoom = Math.max(0.2, Math.min(2.0, Math.min(vw / bw, vh / bh)));
        panX = (minX + maxX) * 0.5;
        panY = (minY + maxY) * 0.5;
        pbCanvas.requestPaint();
    }

    function zoomIn() { zoom = Math.min(3.0, zoom * 1.18); pbCanvas.requestPaint(); }
    function zoomOut() { zoom = Math.max(0.2, zoom / 1.18); pbCanvas.requestPaint(); }
    function zoomReset() { zoom = 1.0; pbCanvas.requestPaint(); }

    function selectAllBoxes() {
        var boxes = sceneData ? (sceneData.boxes || []) : [];
        var keys = [];
        for (var i = 0; i < boxes.length; i++) {
            keys.push(boxes[i].group_id + ":" + boxes[i].split_part);
        }
        controller.patchbay_select_boxes(keys.join(","));
    }

    function focusSelected() {
        var boxes = sceneData ? (sceneData.boxes || []) : [];
        var minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
        var found = false;
        for (var i = 0; i < boxes.length; i++) {
            if (boxes[i].selected) {
                var r = boxes[i].rect;
                minX = Math.min(minX, r.x);
                minY = Math.min(minY, r.y);
                maxX = Math.max(maxX, r.x + r.w);
                maxY = Math.max(maxY, r.y + r.h);
                found = true;
            }
        }
        if (found) {
            panX = (minX + maxX) * 0.5;
            panY = (minY + maxY) * 0.5;
            pbCanvas.requestPaint();
        }
    }

    function hitPort(sx, sy) {
        var boxes = sceneData ? (sceneData.boxes || []) : [];
        for (var i = boxes.length - 1; i >= 0; i--) {
            var b = boxes[i];
            var ins = b.in_ports || [];
            for (var j = 0; j < ins.length; j++) {
                var r = ins[j].rect;
                if (sx >= r.x - 3 && sx <= r.x + r.w + 3 && sy >= r.y - 3 && sy <= r.y + r.h + 3) {
                    return ins[j];
                }
            }
            var outs = b.out_ports || [];
            for (var k = 0; k < outs.length; k++) {
                var ro = outs[k].rect;
                if (sx >= ro.x - 3 && sx <= ro.x + ro.w + 3 && sy >= ro.y - 3 && sy <= ro.y + ro.h + 3) {
                    return outs[k];
                }
            }
        }
        return null;
    }

    function hitBox(sx, sy) {
        var boxes = sceneData ? (sceneData.boxes || []) : [];
        for (var i = boxes.length - 1; i >= 0; i--) {
            var r = boxes[i].rect;
            if (sx >= r.x && sx <= r.x + r.w && sy >= r.y && sy <= r.y + r.h) {
                return boxes[i];
            }
        }
        return null;
    }

    function portsCompatible(p1, p2) {
        if (!p1 || !p2 || p1.is_input === p2.is_input) return false;
        var mask = 0x02 | 0x04 | 0x08 | 0x10;
        var t1 = p1.hints & mask;
        var t2 = p2.hints & mask;
        if (t1 === t2) return true;
        var audioCv = ((p1.hints & 0x02) && (p2.hints & 0x04)) || ((p1.hints & 0x04) && (p2.hints & 0x02));
        return !!audioCv;
    }

    function findConnection(gOut, pOut, gIn, pIn) {
        var conns = sceneData ? (sceneData.connections || []) : [];
        for (var i = 0; i < conns.length; i++) {
            var c = conns[i];
            if (c.group_out === gOut && c.port_out === pOut && c.group_in === gIn && c.port_in === pIn) {
                return c.connection_id;
            }
        }
        return -1;
    }

    function getSelectedKeysArray() {
        var boxes = sceneData ? (sceneData.boxes || []) : [];
        var out = [];
        for (var i = 0; i < boxes.length; i++) {
            if (boxes[i].selected) out.push(boxes[i].group_id + ":" + boxes[i].split_part);
        }
        return out;
    }

    Component.onCompleted: {
        parseScene();
        parseMeters();
    }

    Connections {
        target: root.controller
        function onPatchbay_jsonChanged() { root.parseScene(); }
        function onMeters_jsonChanged() { root.parseMeters(); }
        function onFocus_patchbay_requested() { root.focusSelected(); }
    }

    // Native KDE Patchbay Toolbar
    Controls.ToolBar {
        Layout.fillWidth: true
        RowLayout {
            anchors.fill: parent
            spacing: 2

            Controls.ToolButton {
                visible: root.controller.show_routing_mode_toggle
                text: qsTr("Plugin Routing")
                checkable: true
                checked: !root.controller.external_patchbay
                onClicked: {
                    root.needsInitialFit = true;
                    root.controller.switch_external_patchbay(false);
                }
            }
            Controls.ToolButton {
                visible: root.controller.show_routing_mode_toggle
                text: qsTr("System Routing")
                checkable: true
                checked: root.controller.external_patchbay
                onClicked: {
                    root.needsInitialFit = true;
                    root.controller.switch_external_patchbay(true);
                }
            }
            Controls.ToolSeparator { visible: root.controller.show_routing_mode_toggle }

            Controls.ToolButton {
                text: qsTr("Refresh")
                icon.name: "view-refresh"
                enabled: root.controller.engine_running
                onClicked: root.controller.refresh_patchbay()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Refresh Patchbay (Ctrl+R)")
            }
            Controls.ToolButton {
                text: qsTr("Arrange")
                icon.name: "view-sort-ascending"
                enabled: root.controller.engine_running
                onClicked: {
                    root.controller.arrange_patchbay();
                    root.zoomFit();
                }
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Auto-arrange left-to-right grid (Ctrl+G)")
            }

            Controls.ToolSeparator {}

            Controls.ToolButton {
                text: qsTr("Undo")
                icon.name: "edit-undo"
                enabled: root.controller.can_undo
                onClicked: root.controller.undo()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: root.controller.undo_label
            }
            Controls.ToolButton {
                text: qsTr("Redo")
                icon.name: "edit-redo"
                enabled: root.controller.can_redo
                onClicked: root.controller.redo()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: root.controller.redo_label
            }

            Controls.ToolSeparator {}

            Controls.ToolButton {
                text: qsTr("Auto-Fit")
                icon.name: "zoom-fit-best"
                onClicked: root.zoomFit()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Fit all nodes in view (Home)")
            }
            Controls.ToolButton {
                icon.name: "zoom-out"
                onClicked: root.zoomOut()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Zoom Out (Ctrl+-)")
            }
            Controls.Label {
                text: Math.round(root.zoom * 100) + "%"
                opacity: 0.75
                Layout.leftMargin: 4
                Layout.rightMargin: 4
            }
            Controls.ToolButton {
                icon.name: "zoom-in"
                onClicked: root.zoomIn()
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: qsTr("Zoom In (Ctrl++)")
            }

            Item { Layout.fillWidth: true }
        }
    }

    // Main Canvas + Optional Right Peak Meters
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 0

        Canvas {
            id: pbCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            function drawCable(ctx, p1, p2, useBezier, color, lineW) {
                var s1 = root.sceneToScreen(p1.x, p1.y);
                var s2 = root.sceneToScreen(p2.x, p2.y);
                ctx.beginPath();
                ctx.moveTo(s1.x, s1.y);
                if (useBezier) {
                    var dx = Math.abs(p2.x - p1.x);
                    var off = Math.max(36, Math.min(240, dx * 0.5)) * root.zoom;
                    ctx.bezierCurveTo(s1.x + off, s1.y, s2.x - off, s2.y, s2.x, s2.y);
                } else {
                    ctx.lineTo(s2.x, s2.y);
                }
                ctx.strokeStyle = color;
                ctx.lineWidth = lineW;
                ctx.stroke();
            }

            onPaint: {
                var ctx = getContext("2d");
                var w = width;
                var h = height;
                ctx.clearRect(0, 0, w, h);

                // Background using KDE Kirigami theme
                ctx.fillStyle = Qt.darker(Kirigami.Theme.backgroundColor, 1.15);
                ctx.fillRect(0, 0, w, h);

                // Subtle grid
                var gridStep = 32 * root.zoom;
                if (gridStep >= 10) {
                    var tl = root.screenToScene(0, 0);
                    var br = root.screenToScene(w, h);
                    var gx0 = Math.floor(tl.x / 32);
                    var gx1 = Math.ceil(br.x / 32);
                    var gy0 = Math.floor(tl.y / 32);
                    var gy1 = Math.ceil(br.y / 32);
                    ctx.strokeStyle = Qt.alpha(Kirigami.Theme.textColor, 0.06);
                    ctx.lineWidth = 1;
                    ctx.beginPath();
                    for (var gx = gx0; gx <= gx1; gx++) {
                        var sx = root.sceneToScreen(gx * 32, 0).x;
                        ctx.moveTo(sx, 0);
                        ctx.lineTo(sx, h);
                    }
                    for (var gy = gy0; gy <= gy1; gy++) {
                        var sy = root.sceneToScreen(0, gy * 32).y;
                        ctx.moveTo(0, sy);
                        ctx.lineTo(w, sy);
                    }
                    ctx.stroke();
                }

                var data = root.sceneData;
                if (!data) return;

                // Canvas boundary
                var c0 = root.sceneToScreen(0, 0);
                var c1 = root.sceneToScreen(data.canvas_width || 3100, data.canvas_height || 2400);
                ctx.strokeStyle = Qt.alpha(Kirigami.Theme.textColor, 0.18);
                ctx.lineWidth = 1;
                ctx.strokeRect(c0.x, c0.y, c1.x - c0.x, c1.y - c0.y);

                // Draw boxes
                var boxes = data.boxes || [];
                var z = root.zoom;
                var tip = Math.max(2, Math.min(9, 6 * z));
                var hdrH = 26 * z;

                for (var i = 0; i < boxes.length; i++) {
                    var b = boxes[i];
                    var tlB = root.sceneToScreen(b.rect.x, b.rect.y);
                    var bw = b.rect.w * z;
                    var bh = b.rect.h * z;
                    if (tlB.x + bw < 0 || tlB.y + bh < 0 || tlB.x > w || tlB.y > h) continue;

                    // Box body
                    ctx.fillStyle = Kirigami.Theme.backgroundColor;
                    ctx.fillRect(tlB.x, tlB.y, bw, bh);

                    // Box header
                    ctx.fillStyle = Kirigami.Theme.alternateBackgroundColor;
                    ctx.fillRect(tlB.x, tlB.y, bw, hdrH);

                    // Border
                    ctx.strokeStyle = b.selected ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.35);
                    ctx.lineWidth = b.selected ? 2 : 1;
                    ctx.strokeRect(tlB.x, tlB.y, bw, bh);

                    // Header title
                    if (z >= 0.35) {
                        var prefix = b.icon === 2 ? "◈ " : (b.icon === 3 ? "◆ " : (b.icon === 1 ? "▪ " : ""));
                        ctx.fillStyle = Kirigami.Theme.textColor;
                        ctx.font = "bold " + Math.max(8, Math.min(16, Math.round(11.5 * z))) + "px sans-serif";
                        ctx.textAlign = "center";
                        ctx.textBaseline = "middle";
                        ctx.fillText(prefix + b.title, tlB.x + bw * 0.5, tlB.y + hdrH * 0.5);
                    }

                    // Input ports
                    var pFont = Math.max(7, Math.min(14, Math.round(10.5 * z))) + "px sans-serif";
                    var ins = b.in_ports || [];
                    for (var j = 0; j < ins.length; j++) {
                        var pi = ins[j];
                        var r0 = root.sceneToScreen(pi.rect.x, pi.rect.y);
                        var pw = pi.rect.w * z;
                        var ph = pi.rect.h * z;
                        ctx.beginPath();
                        ctx.moveTo(r0.x, r0.y);
                        ctx.lineTo(r0.x + pw - tip, r0.y);
                        ctx.lineTo(r0.x + pw, r0.y + ph * 0.5);
                        ctx.lineTo(r0.x + pw - tip, r0.y + ph);
                        ctx.lineTo(r0.x, r0.y + ph);
                        ctx.closePath();
                        ctx.fillStyle = pi.color || "#4E7AA8";
                        ctx.fill();
                        ctx.strokeStyle = "rgba(0,0,0,0.35)";
                        ctx.lineWidth = 0.8;
                        ctx.stroke();

                        if (z >= 0.4) {
                            ctx.fillStyle = "#F8FAFC";
                            ctx.font = pFont;
                            ctx.textAlign = "left";
                            ctx.textBaseline = "middle";
                            ctx.fillText(pi.name, r0.x + 5 * z, r0.y + ph * 0.5);
                        }
                    }

                    // Output ports
                    var outs = b.out_ports || [];
                    for (var k = 0; k < outs.length; k++) {
                        var po = outs[k];
                        var ro0 = root.sceneToScreen(po.rect.x, po.rect.y);
                        var pow = po.rect.w * z;
                        var poh = po.rect.h * z;
                        ctx.beginPath();
                        ctx.moveTo(ro0.x + tip, ro0.y);
                        ctx.lineTo(ro0.x + pow, ro0.y);
                        ctx.lineTo(ro0.x + pow, ro0.y + poh);
                        ctx.lineTo(ro0.x + tip, ro0.y + poh);
                        ctx.lineTo(ro0.x, ro0.y + poh * 0.5);
                        ctx.closePath();
                        ctx.fillStyle = po.color || "#4E7AA8";
                        ctx.fill();
                        ctx.strokeStyle = "rgba(0,0,0,0.35)";
                        ctx.lineWidth = 0.8;
                        ctx.stroke();

                        if (z >= 0.4) {
                            ctx.fillStyle = "#F8FAFC";
                            ctx.font = pFont;
                            ctx.textAlign = "right";
                            ctx.textBaseline = "middle";
                            ctx.fillText(po.name, ro0.x + pow - 5 * z, ro0.y + poh * 0.5);
                        }
                    }
                }

                // Draw cables
                var conns = data.connections || [];
                var useBez = !!data.use_bezier;
                for (var cIdx = 0; cIdx < conns.length; cIdx++) {
                    var c = conns[cIdx];
                    var lw = c.selected ? Math.max(1.8, Math.min(5, 2.8 * z)) : Math.max(1.2, Math.min(4, 2.0 * z));
                    var col = c.selected ? Qt.lighter(c.color || "#60A5FA", 1.2) : (c.color || "#60A5FA");
                    drawCable(ctx, { x: c.x1, y: c.y1 }, { x: c.x2, y: c.y2 }, useBez, col, lw);
                }

                // Active port-drag cable preview
                if (root.dragMode === "port" && root.dragSourcePort) {
                    var sp = root.dragSourcePort;
                    var aPt = { x: sp.anchor_x, y: sp.anchor_y };
                    var curPt = { x: root.dragCurSceneX, y: root.dragCurSceneY };
                    if (sp.is_input) {
                        drawCable(ctx, curPt, aPt, useBez, "#FACC15", 2.2);
                    } else {
                        drawCable(ctx, aPt, curPt, useBez, "#FACC15", 2.2);
                    }
                }

                // Rubberband selection
                if (root.dragMode === "rubberband") {
                    var ra = root.sceneToScreen(root.dragStartSceneX, root.dragStartSceneY);
                    var rb = root.sceneToScreen(root.dragCurSceneX, root.dragCurSceneY);
                    ctx.fillStyle = Qt.alpha(Kirigami.Theme.highlightColor, 0.18);
                    ctx.fillRect(Math.min(ra.x, rb.x), Math.min(ra.y, rb.y), Math.abs(rb.x - ra.x), Math.abs(rb.y - ra.y));
                    ctx.strokeStyle = Kirigami.Theme.highlightColor;
                    ctx.lineWidth = 1;
                    ctx.strokeRect(Math.min(ra.x, rb.x), Math.min(ra.y, rb.y), Math.abs(rb.x - ra.x), Math.abs(rb.y - ra.y));
                }

                // Zoom rectangle
                if (root.dragMode === "zoomrect") {
                    var za = root.sceneToScreen(root.dragStartSceneX, root.dragStartSceneY);
                    var zb = root.sceneToScreen(root.dragCurSceneX, root.dragCurSceneY);
                    ctx.fillStyle = "rgba(250, 204, 21, 0.18)";
                    ctx.fillRect(Math.min(za.x, zb.x), Math.min(za.y, zb.y), Math.abs(zb.x - za.x), Math.abs(zb.y - za.y));
                    ctx.strokeStyle = "#FACC15";
                    ctx.lineWidth = 1.2;
                    ctx.strokeRect(Math.min(za.x, zb.x), Math.min(za.y, zb.y), Math.abs(zb.x - za.x), Math.abs(zb.y - za.y));
                }

                // Cable knife line
                if (root.dragMode === "knife") {
                    var ka = root.sceneToScreen(root.dragStartSceneX, root.dragStartSceneY);
                    var kb = root.sceneToScreen(root.dragCurSceneX, root.dragCurSceneY);
                    ctx.beginPath();
                    ctx.moveTo(ka.x, ka.y);
                    ctx.lineTo(kb.x, kb.y);
                    ctx.strokeStyle = "#EF4444";
                    ctx.lineWidth = 2;
                    ctx.stroke();
                }
            }

            MouseArea {
                id: pbMouse
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                hoverEnabled: true

                onWheel: function(wheel) {
                    var before = root.screenToScene(wheel.x, wheel.y);
                    var factor = wheel.angleDelta.y > 0 ? 1.12 : (1.0 / 1.12);
                    root.zoom = Math.max(0.2, Math.min(3.0, root.zoom * factor));
                    var after = root.screenToScene(wheel.x, wheel.y);
                    root.panX += before.x - after.x;
                    root.panY += before.y - after.y;
                    pbCanvas.requestPaint();
                }

                onPressed: function(mouse) {
                    var ctrl = (mouse.modifiers & Qt.ControlModifier) !== 0;
                    var shift = (mouse.modifiers & Qt.ShiftModifier) !== 0;
                    var sp = root.screenToScene(mouse.x, mouse.y);
                    root.dragStartSceneX = sp.x;
                    root.dragStartSceneY = sp.y;
                    root.dragCurSceneX = sp.x;
                    root.dragCurSceneY = sp.y;
                    root.dragStartScreenX = mouse.x;
                    root.dragStartScreenY = mouse.y;

                    if (mouse.button === Qt.MiddleButton) {
                        if (ctrl) {
                            root.dragMode = "knife";
                        } else {
                            root.dragMode = "pan";
                            root.panOriginX = root.panX;
                            root.panOriginY = root.panY;
                        }
                        pbCanvas.requestPaint();
                        return;
                    }

                    if (mouse.button === Qt.RightButton) {
                        if (ctrl) {
                            root.dragMode = "zoomrect";
                            pbCanvas.requestPaint();
                            return;
                        }
                        var hp = root.hitPort(sp.x, sp.y);
                        if (hp) {
                            portCtxMenu.openForPort(hp);
                            return;
                        }
                        var hb = root.hitBox(sp.x, sp.y);
                        if (hb) {
                            boxCtxMenu.openForBox(hb);
                            return;
                        }
                        canvasCtxMenu.popup();
                        return;
                    }

                    // Left button
                    var port = root.hitPort(sp.x, sp.y);
                    if (port) {
                        root.dragMode = "port";
                        root.dragSourcePort = port;
                        pbCanvas.requestPaint();
                        return;
                    }

                    var box = root.hitBox(sp.x, sp.y);
                    if (box) {
                        var bKey = box.group_id + ":" + box.split_part;
                        var selKeys = root.getSelectedKeysArray();
                        if (ctrl || shift) {
                            var idx = selKeys.indexOf(bKey);
                            if (idx >= 0) selKeys.splice(idx, 1); else selKeys.push(bKey);
                            root.controller.patchbay_select_boxes(selKeys.join(","));
                        } else if (!box.selected) {
                            selKeys = [bKey];
                            root.controller.patchbay_select_boxes(bKey);
                        }
                        var origins = {};
                        var boxes = root.sceneData.boxes || [];
                        for (var i = 0; i < boxes.length; i++) {
                            var k = boxes[i].group_id + ":" + boxes[i].split_part;
                            if (selKeys.indexOf(k) >= 0) {
                                origins[k] = {
                                    group_id: boxes[i].group_id,
                                    split_part: boxes[i].split_part,
                                    x: boxes[i].rect.x,
                                    y: boxes[i].rect.y
                                };
                            }
                        }
                        root.boxOrigins = origins;
                        root.dragMode = "boxes";
                        return;
                    }

                    if (!ctrl && !shift) {
                        root.controller.patchbay_select_boxes("");
                    }
                    root.dragMode = "rubberband";
                    pbCanvas.requestPaint();
                }

                onPositionChanged: function(mouse) {
                    if (root.dragMode === "") return;
                    var sp = root.screenToScene(mouse.x, mouse.y);
                    root.dragCurSceneX = sp.x;
                    root.dragCurSceneY = sp.y;

                    if (root.dragMode === "pan") {
                        root.panX = root.panOriginX - (mouse.x - root.dragStartScreenX) / root.zoom;
                        root.panY = root.panOriginY - (mouse.y - root.dragStartScreenY) / root.zoom;
                        pbCanvas.requestPaint();
                    } else if (root.dragMode === "boxes") {
                        var dx = sp.x - root.dragStartSceneX;
                        var dy = sp.y - root.dragStartSceneY;
                        for (var k in root.boxOrigins) {
                            var o = root.boxOrigins[k];
                            root.controller.patchbay_move_box(o.group_id, o.split_part, o.x + dx, o.y + dy);
                        }
                    } else {
                        pbCanvas.requestPaint();
                    }
                }

                onReleased: function(mouse) {
                    var mode = root.dragMode;
                    root.dragMode = "";
                    var sp = root.screenToScene(mouse.x, mouse.y);

                    if (mode === "boxes") {
                        var committed = {};
                        for (var k in root.boxOrigins) {
                            var gid = root.boxOrigins[k].group_id;
                            if (!committed[gid]) {
                                committed[gid] = true;
                                root.controller.patchbay_commit_group_pos(gid);
                            }
                        }
                    } else if (mode === "port" && root.dragSourcePort) {
                        var dst = root.hitPort(sp.x, sp.y);
                        if (dst && root.portsCompatible(root.dragSourcePort, dst)) {
                            var gOut = root.dragSourcePort.is_input ? dst.group_id : root.dragSourcePort.group_id;
                            var pOut = root.dragSourcePort.is_input ? dst.port_id : root.dragSourcePort.port_id;
                            var gIn = root.dragSourcePort.is_input ? root.dragSourcePort.group_id : dst.group_id;
                            var pIn = root.dragSourcePort.is_input ? root.dragSourcePort.port_id : dst.port_id;
                            var existing = root.findConnection(gOut, pOut, gIn, pIn);
                            if (existing >= 0) {
                                root.controller.patchbay_disconnect(existing);
                            } else {
                                root.controller.patchbay_connect(gOut, pOut, gIn, pIn);
                            }
                        }
                        root.dragSourcePort = null;
                    } else if (mode === "rubberband") {
                        var x0 = Math.min(root.dragStartSceneX, sp.x);
                        var y0 = Math.min(root.dragStartSceneY, sp.y);
                        var x1 = Math.max(root.dragStartSceneX, sp.x);
                        var y1 = Math.max(root.dragStartSceneY, sp.y);
                        if ((x1 - x0) > 4 || (y1 - y0) > 4) {
                            var boxes = root.sceneData.boxes || [];
                            var sel = [];
                            for (var i = 0; i < boxes.length; i++) {
                                var r = boxes[i].rect;
                                if (r.x < x1 && (r.x + r.w) > x0 && r.y < y1 && (r.y + r.h) > y0) {
                                    sel.push(boxes[i].group_id + ":" + boxes[i].split_part);
                                }
                            }
                            root.controller.patchbay_select_boxes(sel.join(","));
                        }
                    } else if (mode === "zoomrect") {
                        var zx0 = Math.min(root.dragStartSceneX, sp.x);
                        var zy0 = Math.min(root.dragStartSceneY, sp.y);
                        var zx1 = Math.max(root.dragStartSceneX, sp.x);
                        var zy1 = Math.max(root.dragStartSceneY, sp.y);
                        if ((zx1 - zx0) > 20 && (zy1 - zy0) > 20) {
                            root.zoom = Math.max(0.2, Math.min(3.0, Math.min(pbCanvas.width / (zx1 - zx0), pbCanvas.height / (zy1 - zy0))));
                            root.panX = (zx0 + zx1) * 0.5;
                            root.panY = (zy0 + zy1) * 0.5;
                        }
                    } else if (mode === "knife") {
                        root.controller.patchbay_cut_line(root.dragStartSceneX, root.dragStartSceneY, sp.x, sp.y);
                    }
                    pbCanvas.requestPaint();
                }
            }
        }

        // Optional vertical Peak Meters panel (Settings -> Show Patchbay Peak Meters)
        Controls.Pane {
            visible: root.controller.show_meters
            Layout.fillHeight: true
            Layout.preferredWidth: 58
            padding: 6
            ColumnLayout {
                anchors.fill: parent
                spacing: 4
                Controls.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("In / Out")
                    opacity: 0.75
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
                RowLayout {
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 4
                    PeakMeter {
                        Layout.preferredWidth: 18
                        Layout.fillHeight: true
                        vertical: true
                        channels: 2
                        baseColor: "#4ADE80"
                        levelL: root.canvasMeters.in_l || 0
                        levelR: root.canvasMeters.in_r || 0
                        clipL: !!root.canvasMeters.in_clip_l
                        clipR: !!root.canvasMeters.in_clip_r
                        tooltipText: qsTr("Selected plugin input peak")
                    }
                    PeakMeter {
                        Layout.preferredWidth: 18
                        Layout.fillHeight: true
                        vertical: true
                        channels: 2
                        baseColor: "#38BDF8"
                        levelL: root.canvasMeters.out_l || 0
                        levelR: root.canvasMeters.out_r || 0
                        clipL: !!root.canvasMeters.out_clip_l
                        clipR: !!root.canvasMeters.out_clip_r
                        tooltipText: qsTr("Selected plugin output peak")
                    }
                }
            }
        }
    }

    // Optional bottom Piano Keyboard (Settings -> Show Patchbay Keyboard)
    Controls.Pane {
        visible: root.controller.show_keyboard
        Layout.fillWidth: true
        padding: 6
        PianoKeyboard {
            anchors.fill: parent
            activeEnabled: root.controller.selected_patchbay_plugin_id >= 0
            externalNotes: root.canvasNotes
            onNoteOn: function(ch, note, vel) { root.controller.send_canvas_midi_note(ch, note, vel); }
            onNoteOff: function(ch, note) { root.controller.send_canvas_midi_note(ch, note, 0); }
        }
    }

    // --- Context Menus ---

    Controls.Menu {
        id: canvasCtxMenu
        Controls.MenuItem {
            text: qsTr("Arrange")
            icon.name: "view-sort-ascending"
            onTriggered: { root.controller.arrange_patchbay(); root.zoomFit(); }
        }
        Controls.MenuItem {
            text: qsTr("Refresh")
            icon.name: "view-refresh"
            onTriggered: root.controller.refresh_patchbay()
        }
        Controls.MenuSeparator {}
        Controls.MenuItem { text: qsTr("Auto-Fit"); icon.name: "zoom-fit-best"; onTriggered: root.zoomFit() }
        Controls.MenuItem { text: qsTr("Zoom 100%"); icon.name: "zoom-original"; onTriggered: root.zoomReset() }
    }

    Controls.Menu {
        id: boxCtxMenu
        property var targetBox: null
        function openForBox(b) { targetBox = b; popup(); }

        Controls.MenuItem {
            text: (boxCtxMenu.targetBox && boxCtxMenu.targetBox.split) ? qsTr("Join") : qsTr("Split")
            enabled: boxCtxMenu.targetBox && (boxCtxMenu.targetBox.split || boxCtxMenu.targetBox.can_split)
            onTriggered: {
                if (boxCtxMenu.targetBox.split) {
                    root.controller.patchbay_join_group(boxCtxMenu.targetBox.group_id);
                } else {
                    root.controller.patchbay_split_group(boxCtxMenu.targetBox.group_id);
                }
            }
        }
        Controls.MenuItem {
            text: qsTr("Disconnect All")
            icon.name: "network-disconnect"
            onTriggered: root.controller.patchbay_disconnect_group(boxCtxMenu.targetBox.group_id)
        }
        Controls.MenuSeparator { visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0 }
        Controls.MenuItem {
            visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0
            text: qsTr("Edit Parameters")
            icon.name: "view-media-equalizer"
            onTriggered: root.openPluginEdit(boxCtxMenu.targetBox.plugin_id)
        }
        Controls.MenuItem {
            visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0
            text: qsTr("Show Custom UI")
            icon.name: "window-new"
            onTriggered: root.controller.toggle_custom_ui(boxCtxMenu.targetBox.plugin_id)
        }
        Controls.MenuSeparator { visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0 }
        Controls.MenuItem {
            visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0
            text: qsTr("Clone")
            icon.name: "edit-copy"
            onTriggered: root.controller.clone_plugin(boxCtxMenu.targetBox.plugin_id)
        }
        Controls.MenuItem {
            visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0
            text: qsTr("Rename…")
            icon.name: "edit-rename"
            onTriggered: root.openRenamePlugin(boxCtxMenu.targetBox.plugin_id, boxCtxMenu.targetBox.title)
        }
        Controls.MenuItem {
            visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0
            text: qsTr("Replace…")
            onTriggered: root.openReplacePlugin(boxCtxMenu.targetBox.plugin_id)
        }
        Controls.MenuItem {
            visible: boxCtxMenu.targetBox && boxCtxMenu.targetBox.plugin_id >= 0
            text: qsTr("Remove")
            icon.name: "edit-delete"
            onTriggered: root.controller.remove_plugin(boxCtxMenu.targetBox.plugin_id)
        }
    }

    Controls.Menu {
        id: portCtxMenu
        property var targetPort: null
        property var connectCandidates: []
        property var disconnectCandidates: []

        function openForPort(p) {
            targetPort = p;
            var boxes = root.sceneData ? (root.sceneData.boxes || []) : [];
            var conns = root.sceneData ? (root.sceneData.connections || []) : [];
            var groupNames = {};
            var allPorts = [];
            for (var i = 0; i < boxes.length; i++) {
                groupNames[boxes[i].group_id] = boxes[i].title;
                var list = (boxes[i].in_ports || []).concat(boxes[i].out_ports || []);
                for (var j = 0; j < list.length; j++) allPorts.push(list[j]);
            }

            var dis = [];
            for (var ci = 0; ci < conns.length; ci++) {
                var c = conns[ci];
                if (!p.is_input && c.group_out === p.group_id && c.port_out === p.port_id) {
                    var targetName = (groupNames[c.group_in] || "?") + " : " + findPortName(allPorts, c.group_in, c.port_in);
                    dis.push({ connection_id: c.connection_id, label: targetName });
                } else if (p.is_input && c.group_in === p.group_id && c.port_in === p.port_id) {
                    var srcName = (groupNames[c.group_out] || "?") + " : " + findPortName(allPorts, c.group_out, c.port_out);
                    dis.push({ connection_id: c.connection_id, label: srcName });
                }
            }
            disconnectCandidates = dis;

            var cand = [];
            for (var pi = 0; pi < allPorts.length; pi++) {
                var other = allPorts[pi];
                if (other.group_id === p.group_id || !root.portsCompatible(p, other)) continue;
                var gOut = p.is_input ? other.group_id : p.group_id;
                var pOut = p.is_input ? other.port_id : p.port_id;
                var gIn = p.is_input ? p.group_id : other.group_id;
                var pIn = p.is_input ? p.port_id : other.port_id;
                if (root.findConnection(gOut, pOut, gIn, pIn) >= 0) continue;
                cand.push({
                    gOut: gOut, pOut: pOut, gIn: gIn, pIn: pIn,
                    label: (groupNames[other.group_id] || "?") + " : " + other.name
                });
            }
            connectCandidates = cand;
            popup();
        }

        function findPortName(allPorts, gid, pid) {
            for (var i = 0; i < allPorts.length; i++) {
                if (allPorts[i].group_id === gid && allPorts[i].port_id === pid) return allPorts[i].name;
            }
            return "?";
        }

        Controls.Menu {
            title: qsTr("Connect to")
            enabled: portCtxMenu.connectCandidates.length > 0
            Repeater {
                model: portCtxMenu.connectCandidates
                delegate: Controls.MenuItem {
                    required property var modelData
                    text: modelData.label
                    onTriggered: root.controller.patchbay_connect(modelData.gOut, modelData.pOut, modelData.gIn, modelData.pIn)
                }
            }
        }
        Controls.Menu {
            title: qsTr("Disconnect")
            enabled: portCtxMenu.disconnectCandidates.length > 0
            Repeater {
                model: portCtxMenu.disconnectCandidates
                delegate: Controls.MenuItem {
                    required property var modelData
                    text: modelData.label
                    onTriggered: root.controller.patchbay_disconnect(modelData.connection_id)
                }
            }
        }
        Controls.MenuItem {
            text: qsTr("Disconnect All")
            enabled: portCtxMenu.disconnectCandidates.length > 0
            onTriggered: root.controller.patchbay_disconnect_port(
                portCtxMenu.targetPort.group_id,
                portCtxMenu.targetPort.port_id,
                portCtxMenu.targetPort.is_input
            )
        }
    }
}
