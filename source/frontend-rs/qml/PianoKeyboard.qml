// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root
    property bool activeEnabled: true
    property int baseOctave: 2
    property int numOctaves: 5
    property int midiChannel: 0
    property int velocity: 100
    property var externalNotes: []
    property int pressedNote: -1

    signal noteOn(int channel, int note, int velocity)
    signal noteOff(int channel, int note)

    spacing: 4

    onExternalNotesChanged: kbCanvas.requestPaint()
    onBaseOctaveChanged: kbCanvas.requestPaint()
    onActiveEnabledChanged: {
        if (!activeEnabled && pressedNote >= 0) {
            noteOff(midiChannel, pressedNote);
            pressedNote = -1;
        }
        kbCanvas.requestPaint();
    }

    function isBlackKey(n) {
        var m = n % 12;
        return m === 1 || m === 3 || m === 6 || m === 8 || m === 10;
    }

    function noteAtPos(mx, my) {
        var whiteCount = numOctaves * 7;
        var whiteW = kbCanvas.width / whiteCount;
        var blackW = whiteW * 0.62;
        var blackH = kbCanvas.height * 0.60;
        var startMidi = Math.max(0, Math.min(108, (baseOctave + 1) * 12));
        var total = numOctaves * 12;

        // Check black keys first
        var wi = 0;
        for (var i = 0; i < total; i++) {
            var n = startMidi + i;
            if (n > 127) break;
            if (!isBlackKey(n)) {
                wi++;
            } else {
                var bx = wi * whiteW - blackW * 0.5;
                if (mx >= bx && mx <= bx + blackW && my >= 0 && my <= blackH) {
                    return n;
                }
            }
        }
        // Check white keys
        wi = 0;
        for (var j = 0; j < total; j++) {
            var wn = startMidi + j;
            if (wn > 127) break;
            if (!isBlackKey(wn)) {
                var wx = wi * whiteW;
                if (mx >= wx && mx <= wx + whiteW) {
                    return wn;
                }
                wi++;
            }
        }
        return -1;
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6
        Controls.Label { text: qsTr("Octave:") }
        Controls.ToolButton {
            text: "−"
            enabled: root.baseOctave > 0
            onClicked: root.baseOctave--
        }
        Controls.Label { text: "C" + root.baseOctave }
        Controls.ToolButton {
            text: "+"
            enabled: root.baseOctave < 6
            onClicked: root.baseOctave++
        }
        Controls.ToolSeparator {}
        Controls.Label { text: qsTr("MIDI Channel:") }
        Controls.SpinBox {
            from: 1
            to: 16
            value: root.midiChannel + 1
            onValueModified: root.midiChannel = value - 1
        }
        Item { Layout.fillWidth: true }
    }

    Canvas {
        id: kbCanvas
        Layout.fillWidth: true
        Layout.preferredHeight: 68
        Layout.minimumHeight: 56

        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;
            ctx.clearRect(0, 0, w, h);

            var whiteCount = root.numOctaves * 7;
            var whiteW = w / whiteCount;
            var blackW = whiteW * 0.62;
            var blackH = h * 0.60;
            var startMidi = Math.max(0, Math.min(108, (root.baseOctave + 1) * 12));
            var total = root.numOctaves * 12;

            function isActive(n) {
                if (n === root.pressedNote) return true;
                return root.externalNotes && root.externalNotes.indexOf(n) >= 0;
            }

            // Draw white keys
            var wi = 0;
            for (var i = 0; i < total; i++) {
                var n = startMidi + i;
                if (n > 127) break;
                if (!root.isBlackKey(n)) {
                    var x = wi * whiteW;
                    if (!root.activeEnabled) {
                        ctx.fillStyle = "#A0A0A6";
                    } else if (isActive(n)) {
                        ctx.fillStyle = Kirigami.Theme.highlightColor;
                    } else {
                        ctx.fillStyle = "#F4F4F6";
                    }
                    ctx.fillRect(x, 0, whiteW, h);
                    ctx.strokeStyle = "#33363C";
                    ctx.lineWidth = 1;
                    ctx.strokeRect(x + 0.5, 0.5, whiteW - 1, h - 1);

                    if (n % 12 === 0) {
                        var oct = Math.floor(n / 12) - 1;
                        ctx.fillStyle = "#555860";
                        ctx.font = "10px sans-serif";
                        ctx.textAlign = "center";
                        ctx.fillText("C" + oct, x + whiteW * 0.5, h - 6);
                    }
                    wi++;
                }
            }

            // Draw black keys on top
            wi = 0;
            for (var j = 0; j < total; j++) {
                var bn = startMidi + j;
                if (bn > 127) break;
                if (!root.isBlackKey(bn)) {
                    wi++;
                } else {
                    var bx = wi * whiteW - blackW * 0.5;
                    if (!root.activeEnabled) {
                        ctx.fillStyle = "#484A50";
                    } else if (isActive(bn)) {
                        ctx.fillStyle = Qt.darker(Kirigami.Theme.highlightColor, 1.15);
                    } else {
                        ctx.fillStyle = "#1E2026";
                    }
                    ctx.fillRect(bx, 0, blackW, blackH);
                    ctx.strokeStyle = "#000000";
                    ctx.lineWidth = 1;
                    ctx.strokeRect(bx + 0.5, 0.5, blackW - 1, blackH - 1);
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.activeEnabled
            onPressed: function(mouse) {
                var n = root.noteAtPos(mouse.x, mouse.y);
                if (n >= 0) {
                    root.pressedNote = n;
                    root.noteOn(root.midiChannel, n, root.velocity);
                    kbCanvas.requestPaint();
                }
            }
            onPositionChanged: function(mouse) {
                if (!pressed) return;
                var n = root.noteAtPos(mouse.x, mouse.y);
                if (n !== root.pressedNote) {
                    if (root.pressedNote >= 0) {
                        root.noteOff(root.midiChannel, root.pressedNote);
                    }
                    root.pressedNote = n;
                    if (n >= 0) {
                        root.noteOn(root.midiChannel, n, root.velocity);
                    }
                    kbCanvas.requestPaint();
                }
            }
            onReleased: {
                if (root.pressedNote >= 0) {
                    root.noteOff(root.midiChannel, root.pressedNote);
                    root.pressedNote = -1;
                    kbCanvas.requestPaint();
                }
            }
            onCanceled: {
                if (root.pressedNote >= 0) {
                    root.noteOff(root.midiChannel, root.pressedNote);
                    root.pressedNote = -1;
                    kbCanvas.requestPaint();
                }
            }
        }
    }
}
