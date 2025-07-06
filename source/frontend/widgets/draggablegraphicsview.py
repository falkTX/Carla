#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import Qt, QT_VERSION, QEvent
    from PyQt5.QtGui import QCursor, QMouseEvent
    from PyQt5.QtWidgets import QGraphicsView, QMessageBox
elif qt_config == 6:
    from PyQt6.QtCore import Qt, QT_VERSION, QEvent
    from PyQt6.QtGui import QCursor, QMouseEvent
    from PyQt6.QtWidgets import QGraphicsView, QMessageBox

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_backend import CARLA_OS_MAC
from carla_shared import CustomMessageBox, gCarla

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

class DraggableGraphicsView(QGraphicsView):
    def __init__(self, parent):
        QGraphicsView.__init__(self, parent)

        self.fPanning = False

        exts = gCarla.utils.get_supported_file_extensions()

        self.fSupportedExtensions = tuple(("." + i) for i in exts)
        self.fWasLastDragValid = False

        self.setAcceptDrops(True)

    # -----------------------------------------------------------------------------------------------------------------

    def isDragUrlValid(self, filename):
        lfilename = filename.lower()

        if os.path.isdir(filename):
            #if os.path.exists(os.path.join(filename, "manifest.ttl")):
                #return True
            if CARLA_OS_MAC and lfilename.endswith(".vst"):
                return True
            if lfilename.endswith(".vst3") and ".vst3" in self.fSupportedExtensions:
                return True

        elif os.path.isfile(filename):
            if lfilename.endswith(self.fSupportedExtensions):
                return True

        return False

    # -----------------------------------------------------------------------------------------------------------------

    def dragEnterEvent(self, event):
        urls = event.mimeData().urls()

        for url in urls:
            if self.isDragUrlValid(url.toLocalFile()):
                self.fWasLastDragValid = True
                event.acceptProposedAction()
                return

        self.fWasLastDragValid = False
        QGraphicsView.dragEnterEvent(self, event)

    def dragMoveEvent(self, event):
        if not self.fWasLastDragValid:
            QGraphicsView.dragMoveEvent(self, event)
            return

        event.acceptProposedAction()

    def dragLeaveEvent(self, event):
        self.fWasLastDragValid = False
        QGraphicsView.dragLeaveEvent(self, event)

    # -----------------------------------------------------------------------------------------------------------------

    def dropEvent(self, event):
        event.acceptProposedAction()

        urls = event.mimeData().urls()

        if len(urls) == 0:
            return

        for url in urls:
            filename = url.toLocalFile()

            if not gCarla.gui.host.load_file(filename):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                                 self.tr("Failed to load file"),
                                 gCarla.gui.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    # -----------------------------------------------------------------------------------------------------------------

    def mousePressEvent(self, event):
        if event.button() == Qt.MiddleButton and not (event.modifiers() & Qt.ControlModifier):
            buttons  = event.buttons()
            buttons &= ~Qt.MiddleButton
            buttons |= Qt.LeftButton
            timestamp = event.timestamp()
            self.fPanning = True
            self.setDragMode(QGraphicsView.ScrollHandDrag)
        else:
            timestamp = None

        QGraphicsView.mousePressEvent(self, event)

        if timestamp is None:
            return

        if qt_config == 5:
            event = QMouseEvent(QEvent.MouseButtonPress,
                                event.localPos(), event.windowPos(), event.screenPos(),
                                Qt.LeftButton, Qt.LeftButton,
                                Qt.NoModifier,
                                Qt.MouseEventSynthesizedByApplication)
            event.setTimestamp(timestamp)
        else:
            event = QMouseEvent(QEvent.MouseButtonPress,
                                event.position(), event.scenePosition(), event.globalPosition(),
                                Qt.LeftButton, Qt.LeftButton,
                                Qt.NoModifier)

        QGraphicsView.mousePressEvent(self, event)

    def mouseReleaseEvent(self, event):
        QGraphicsView.mouseReleaseEvent(self, event)

        if event.button() == Qt.MiddleButton and self.fPanning:
            self.fPanning = False
            self.setDragMode(QGraphicsView.NoDrag)
            self.setCursor(QCursor(Qt.ArrowCursor))

    def wheelEvent(self, event):
        if event.buttons() & Qt.MiddleButton:
            event.ignore()
            return
        QGraphicsView.wheelEvent(self, event)

# ---------------------------------------------------------------------------------------------------------------------
