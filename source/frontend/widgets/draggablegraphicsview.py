#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Middle-click draggable QGraphicsView
# Copyright (C) 2016-2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QCursor, QMouseEvent
from PyQt5.QtWidgets import QGraphicsView

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class DraggableGraphicsView(QGraphicsView):
    def __init__(self, parent):
        QGraphicsView.__init__(self, parent)

        self.fPanning = False
        self.fCtrlDown = False

        try:
            self.fMiddleButton = Qt.MiddleButton
        except:
            self.fMiddleButton = Qt.MidButton

    def mousePressEvent(self, event):
        if event.button() == self.fMiddleButton and not self.fCtrlDown:
            self.fPanning = True
            self.setDragMode(QGraphicsView.ScrollHandDrag)
            event = QMouseEvent(event.type(), event.pos(), Qt.LeftButton, Qt.LeftButton, event.modifiers())

        QGraphicsView.mousePressEvent(self, event)

    def mouseReleaseEvent(self, event):
        QGraphicsView.mouseReleaseEvent(self, event)

        if not self.fPanning:
            return

        self.fPanning = False
        self.setDragMode(QGraphicsView.NoDrag)
        self.setCursor(QCursor(Qt.ArrowCursor))

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Control:
            self.fCtrlDown = True
        QGraphicsView.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key_Control:
            self.fCtrlDown = False
        QGraphicsView.keyReleaseEvent(self, event)
