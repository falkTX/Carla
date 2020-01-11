#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Collapsible Box, a custom Qt widget
# Copyright (C) 2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtGui import QCursor, QFont
from PyQt5.QtWidgets import QFrame, QSizePolicy, QToolButton, QVBoxLayout, QWidget

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class QToolButtonWithMouseTracking(QToolButton):
    def __init__(self, parent):
        QToolButton.__init__(self, parent)
        self._font = self.font()

    def enterEvent(self, event):
        self._font.setBold(True)
        self.setFont(self._font)
        QToolButton.enterEvent(self, event)

    def leaveEvent(self, event):
        self._font.setBold(False)
        self.setFont(self._font)
        QToolButton.leaveEvent(self, event)

class CollapsibleBox(QFrame):
    def __init__(self, title, parent):
        QFrame.__init__(self, parent)

        self.setFrameShape(QFrame.StyledPanel)
        self.setFrameShadow(QFrame.Raised)

        self.toggle_button = QToolButtonWithMouseTracking(self)
        self.toggle_button.setText(title)
        self.toggle_button.setCheckable(True)
        self.toggle_button.setChecked(True)
        self.toggle_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.toggle_button.setToolButtonStyle(Qt.ToolButtonTextBesideIcon)
        self.toggle_button.setArrowType(Qt.DownArrow)
        self.toggle_button.toggled.connect(self.toolButtonPressed)

        self.content_area   = QWidget(self)
        self.content_layout = QVBoxLayout(self.content_area)
        self.content_layout.setContentsMargins(6, 2, 6, 0)
        self.content_layout.setSpacing(3)

        lay = QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 4)
        lay.setSpacing(1)
        lay.addWidget(self.toggle_button)
        lay.addWidget(self.content_area)

    @pyqtSlot(bool)
    def toolButtonPressed(self, toggled):
        self.content_area.setVisible(toggled)
        self.toggle_button.setArrowType(Qt.DownArrow if toggled else Qt.RightArrow)

    def getContentLayout(self):
        return self.content_layout
