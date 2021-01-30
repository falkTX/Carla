#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import VERSION
from os import getenv

# ------------------------------------------------------------------------------------------------------------

name = getenv("TARGET_NAME")

options = {
  "zip_include_packages": ["*"],
  "zip_exclude_packages": ["PyQt5"],
  "replace_paths": [["*",".\\lib\\"]],
  "build_exe": ".\\build\\Carla\\",
  "optimize": True,
}

exe_options = {
  "script": ".\\source\\frontend\\carla",
  "icon": ".\\resources\\ico\\carla.ico",
  "copyright": "Copyright (C) 2011-2021 Filipe Coelho",
  "base": "Win32GUI",
  "targetName": "Carla.exe",
}

setup(name = "Carla",
      version = VERSION,
      description = "Carla Plugin Host",
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------
