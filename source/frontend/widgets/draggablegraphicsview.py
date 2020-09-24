#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Middle-click draggable QGraphicsView
# Copyright (C) 2016-2020 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the doc/GPL.txt file.

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt5.QtCore import Qt, QEvent, QTimer
from PyQt5.QtGui import QCursor, QMouseEvent
from PyQt5.QtWidgets import QGraphicsView

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class DraggableGraphicsView(QGraphicsView):
    def __init__(self, parent):
        QGraphicsView.__init__(self, parent)

        self.fPanning = False

        try:
            self.fMiddleButton = Qt.MiddleButton
        except:
            self.fMiddleButton = Qt.MidButton

        exts = gCarla.utils.get_supported_file_extensions()

        self.fSupportedExtensions = tuple(("." + i) for i in exts)
        self.fWasLastDragValid = False

        self.setAcceptDrops(True)

    # --------------------------------------------------------------------------------------------------------

    def isDragUrlValid(self, filename):
        lfilename = filename.lower()

        if os.path.isdir(filename):
            #if os.path.exists(os.path.join(filename, "manifest.ttl")):
                #return True
            if MACOS and lfilename.endswith(".vst"):
                return True
            elif lfilename.endswith(".vst3") and ".vst3" in self.fSupportedExtensions:
                return True

        elif os.path.isfile(filename):
            if lfilename.endswith(self.fSupportedExtensions):
                return True

        return False

    # --------------------------------------------------------------------------------------------------------

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

    # --------------------------------------------------------------------------------------------------------

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

    # --------------------------------------------------------------------------------------------------------

    def mousePressEvent(self, event):
        if event.button() == self.fMiddleButton and not (event.modifiers() & Qt.ControlModifier):
            buttons  = event.buttons()
            buttons &= ~self.fMiddleButton
            buttons |= Qt.LeftButton
            timestamp = event.timestamp()
            self.fPanning = True
            self.setDragMode(QGraphicsView.ScrollHandDrag)
        else:
            timestamp = None

        QGraphicsView.mousePressEvent(self, event)

        if timestamp is None:
            return

        event = QMouseEvent(QEvent.MouseButtonPress,
                            event.localPos(), event.windowPos(), event.screenPos(),
                            Qt.LeftButton, Qt.LeftButton,
                            Qt.NoModifier,
                            Qt.MouseEventSynthesizedByApplication)
        event.setTimestamp(timestamp)
        QGraphicsView.mousePressEvent(self, event)

    def mouseReleaseEvent(self, event):
        QGraphicsView.mouseReleaseEvent(self, event)

        if event.button() == self.fMiddleButton and self.fPanning:
            self.fPanning = False
            self.setDragMode(QGraphicsView.NoDrag)
            self.setCursor(QCursor(Qt.ArrowCursor))

    def wheelEvent(self, event):
        if event.buttons() & self.fMiddleButton:
            event.ignore()
            return
        QGraphicsView.wheelEvent(self, event)

    # --------------------------------------------------------------------------------------------------------
