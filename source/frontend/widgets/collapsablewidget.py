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
from PyQt5.QtWidgets import QGroupBox, QSizePolicy, QToolButton, QVBoxLayout, QWidget

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class CollapsibleBox(QGroupBox):
    def __init__(self, title, parent):
        QGroupBox.__init__(self, parent)

        self.toggle_button = QToolButton(self)
        self.toggle_button.setText(title)
        self.toggle_button.setCheckable(True)
        self.toggle_button.setChecked(True)
        self.toggle_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.toggle_button.setStyleSheet("border: none;")
        self.toggle_button.setToolButtonStyle(Qt.ToolButtonTextBesideIcon)
        self.toggle_button.setArrowType(Qt.RightArrow)
        self.toggle_button.toggled.connect(self.toolButtonPressed)

        self.content_area   = QWidget(self)
        self.content_layout = QVBoxLayout(self.content_area)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_layout.setSpacing(0)

        lay = QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(0)
        lay.addWidget(self.toggle_button)
        lay.addWidget(self.content_area)

    @pyqtSlot(bool)
    def toolButtonPressed(self, toggled):
        self.content_area.setVisible(toggled)
        self.toggle_button.setArrowType(Qt.RightArrow if toggled else Qt.DownArrow)

    def getContentLayout(self):
        return self.content_layout
