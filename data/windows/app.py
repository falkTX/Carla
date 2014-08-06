#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import VERSION

# ------------------------------------------------------------------------------------------------------------

options = {
  "icon": ".\\resources\\ico\\carla.ico",
  "packages": ["re", "sip", "subprocess", "inspect"]
}

setup(name = "Carla",
      version = VERSION,
      description = "Carla Plugin Host",
      options = {"build_exe": options},
      executables = [Executable(".\\source\\Carla.pyw", base="Win32GUI")])

# ------------------------------------------------------------------------------------------------------------
