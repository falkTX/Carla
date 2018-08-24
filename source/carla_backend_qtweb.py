#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code (Web stuff)
# Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>
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

import requests

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend_qt import *

# ---------------------------------------------------------------------------------------------------------------------
# Carla Host object for connecting to the REST API

class CarlaHostQtWeb(CarlaHostQtNull):
    def __init__(self):
        CarlaHostQtNull.__init__(self)

        self.baseurl = "http://localhost:2228"

    def get_engine_driver_count(self):
        # FIXME
        return int(requests.get("{}/get_engine_driver_count".format(self.baseurl)).text) - 1

    def get_engine_driver_name(self, index):
        return requests.get("{}/get_engine_driver_name/{}".format(self.baseurl, index)).text

    def get_engine_driver_device_names(self, index):
        return requests.get("{}/get_engine_driver_device_names/{}".format(self.baseurl, index)).text.split("\n")

    def get_engine_driver_device_info(self, index, name):
        return requests.get("{}/get_engine_driver_device_info/{}/{}".format(self.baseurl, index, name)).json()

    def engine_init(self, driverName, clientName):
        resp = requests.get("{}/engine_init/{}/{}".format(self.baseurl, driverName, clientName)).status_code

        # TESTING
        if self.fEngineCallback is not None:
            self.fEngineCallback(None, ENGINE_CALLBACK_ENGINE_STARTED, 0, self.processMode, self.transportMode, 0.0, driverName)

        return resp == 200

    def engine_close(self):
        # TESTING
        if self.fEngineCallback is not None:
            self.fEngineCallback(None, ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0, "")

        return requests.get("{}/engine_close".format(self.baseurl)).status_code == 200

    def engine_idle(self):
        requests.get("{}/engine_idle".format(self.baseurl))

    def is_engine_running(self):
        return requests.get("{}/is_engine_running".format(self.baseurl)).status_code == 200

    def set_engine_about_to_close(self):
        return requests.get("{}/set_engine_about_to_close".format(self.baseurl)).status_code == 200

    def get_last_error(self):
        return requests.get("{}/get_last_error".format(self.baseurl)).text

# ---------------------------------------------------------------------------------------------------------------------
# TESTING

if __name__ == '__main__':

    baseurl = "http://localhost:2228"

    driver_count = int(requests.get("{}/get_engine_driver_count".format(baseurl)).text)

    # FIXME
    driver_count -= 1

    print("Driver names:")
    for index in range(driver_count):
        print("\t -", requests.get("{}/get_engine_driver_name/{}".format(baseurl, index)).text)

    print("Driver device names:")
    for index in range(driver_count):
        for name in requests.get("{}/get_engine_driver_device_names/{}".format(baseurl, index)).text.split("\n"):
            print("\t {}:".format(name), requests.get("{}/get_engine_driver_device_info/{}/{}".format(baseurl, index, name)).json())

# ---------------------------------------------------------------------------------------------------------------------
