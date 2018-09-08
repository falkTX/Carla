#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable
from os import getenv

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import VERSION

# ------------------------------------------------------------------------------------------------------------

options = {
  "packages": ["re", "sip", "subprocess", "inspect"],
  "excludes": ["PyQt5.QtNetwork", "PyQt5.QtSql", "PyQt5.QtTest", "PyQt5.QtXml", "XCTest"],
  "create_shared_zip":    False,
  "append_script_to_exe": True,
  "optimize":   True,
  "compressed": True
}

boptions = {
  "iconfile": "./resources/ico/carla.icns"
}

setup(name = "Carla",
      version = VERSION,
      description = "Carla Plugin Host",
      options = {"build_exe": options, "bdist_mac": boptions},
      executables = [Executable("./source/frontend/%s.pyw" % getenv("SCRIPT_NAME"))])

# ------------------------------------------------------------------------------------------------------------
