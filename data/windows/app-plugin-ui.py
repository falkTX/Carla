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
  "build_exe": ".\\Carla\\resources\\",
  "optimize": True,
}

exe_options = {
  "script": "..\\..\\bin\\resources\\{}".format(name),
  "icon": "..\\..\\resources\\ico\\carla.ico",
  "copyright": "Copyright (C) 2011-2019 Filipe Coelho",
  "base": "Win32GUI",
  "targetName": "{}.exe".format(name),
}

setup(name = name,
      version = VERSION,
      description = name,
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------
