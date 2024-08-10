#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSlot, Qt
    from PyQt5.QtWidgets import QFrame, QSizePolicy, QToolButton, QVBoxLayout, QWidget
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSlot, Qt
    from PyQt6.QtWidgets import QFrame, QSizePolicy, QToolButton, QVBoxLayout, QWidget

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

# ------------------------------------------------------------------------------------------------------------
