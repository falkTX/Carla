#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import VERSION

# ------------------------------------------------------------------------------------------------------------

name = "bigmeter-ui"

# ------------------------------------------------------------------------------------------------------------

options = {
  "icon": ".\\resources\\ico\\carla.ico",
  "packages": [],
  "includes": ["re", "sip", "subprocess", "inspect"],
  "build_exe": ".\\data\\windows\\%s\\" % name,
  "optimize": False,
  "compressed": False
}

setup(name = name,
      version = VERSION,
      description = name,
      options = {"build_exe": options},
      executables = [Executable(".\\bin\\resources\\%s.pyw" % name, base="Win32GUI")])

# ------------------------------------------------------------------------------------------------------------
