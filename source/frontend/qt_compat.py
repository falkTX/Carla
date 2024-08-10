#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

from qt_config import qt as qt_config

if qt_config == 5:
    from PyQt5.QtCore import Qt

    Qt.CheckState = int
    Qt.MiddleButton = Qt.MidButton

elif qt_config == 6:
    from PyQt6.QtCore import Qt
    from PyQt6.QtWidgets import QMessageBox

    # TODO fill up everything else

    QMessageBox.No = QMessageBox.StandardButton.No
    QMessageBox.Yes = QMessageBox.StandardButton.Yes
