#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from platform import architecture
from sys import platform, maxsize

# ---------------------------------------------------------------------------------------------------------------------
# Set Version

CARLA_VERSION_HEX    = 0x020591
CARLA_VERSION_STRING = "2.6.0-alpha1"
CARLA_VERSION_STRMIN = "2.6"

# ---------------------------------------------------------------------------------------------------------------------
# Set Platform

CARLA_OS_BSD = False
CARLA_OS_GNU_HURD = False
CARLA_OS_HAIKU = False
CARLA_OS_LINUX = False
CARLA_OS_MAC = False
CARLA_OS_UNIX = False
CARLA_OS_WASM = False
CARLA_OS_WIN = False
CARLA_OS_WIN32 = False
CARLA_OS_WIN64 = False

if platform == "darwin":
    CARLA_OS_MAC = True
elif platform == "haiku":
    CARLA_OS_HAIKU = True
elif platform == "linux":
    CARLA_OS_LINUX = True
elif platform == "win32":
    CARLA_OS_WIN32 = True
elif platform == "win64":
    CARLA_OS_WIN64 = True

if CARLA_OS_WIN32 and CARLA_OS_WIN64:
    CARLA_OS_WIN = True
elif CARLA_OS_BSD or CARLA_OS_GNU_HURD or CARLA_OS_LINUX or CARLA_OS_MAC:
    CARLA_OS_UNIX = True

# ---------------------------------------------------------------------------------------------------------------------
# 64bit check

CARLA_OS_64BIT = bool(architecture()[0] == "64bit" and maxsize > 2**32)

# ---------------------------------------------------------------------------------------------------------------------
