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
  "packages": [],
  "includes": ["re", "sip", "subprocess", "inspect"],
  "build_exe": ".\\data\\windows\\Carla\\",
  "optimize": True,
  "compressed": True
}

setup(name = "Carla",
      version = VERSION,
      description = "Carla Plugin Host",
      options = {"build_exe": options},
      executables = [Executable(".\\source\\Carla.pyw", base="Win32GUI")])

# ------------------------------------------------------------------------------------------------------------
