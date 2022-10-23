#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin host
# Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QDialog

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Carla)

from common import MACOS, WINDOWS
from carla_shared import gCarla

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Local)

from aboutjucedialog_ui import Ui_AboutJuceDialog

# ------------------------------------------------------------------------------------------------------------
# About JUCE dialog

class AboutJuceDialog(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = Ui_AboutJuceDialog()
        self.ui.setupUi(self)

        self.ui.l_text2.setText(self.tr("This program uses JUCE version %s." % gCarla.utils.get_juce_version()))

        self.adjustSize()
        self.setFixedSize(self.size())

        flags  = self.windowFlags()
        flags &= ~Qt.WindowContextHelpButtonHint

        if WINDOWS:
            flags |= Qt.MSWindowsFixedSizeDialogHint

        self.setWindowFlags(flags)

        if MACOS:
            self.setWindowModality(Qt.WindowModal)

# ---------------------------------------------------------------------------------------------------------------------
# Testing

if __name__ == '__main__':
    import sys
    # pylint: disable=ungrouped-imports
    from PyQt5.QtWidgets import QApplication
    # pylint: enable=ungrouped-imports

    # from carla_utils import CarlaUtils
    # gCarla.utils = CarlaUtils(os.path.dirname(__file__) + "/../../../bin/libcarla_utils.dylib")

    _app = QApplication(sys.argv)
    _gui = AboutJuceDialog(None)
    _gui.exec_()

# ---------------------------------------------------------------------------------------------------------------------
