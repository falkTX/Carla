#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import QSettings
elif qt_config == 6:
    from PyQt6.QtCore import QSettings

# ---------------------------------------------------------------------------------------------------------------------
# Safer QSettings class, which does not throw if type mismatches

class QSafeSettings(QSettings):
    def value(self, key, defaultValue, valueType):
        if not isinstance(defaultValue, valueType):
            print("QSafeSettings.value() - defaultValue type mismatch for key", key)

        try:
            return QSettings.value(self, key, defaultValue, valueType)
        except:
            return defaultValue

# ---------------------------------------------------------------------------------------------------------------------
