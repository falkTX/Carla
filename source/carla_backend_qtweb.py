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

import os
from time import sleep

# ---------------------------------------------------------------------------------------------------------------------
# Iterates over the content of a file-like object line-by-line.
# Based on code by Lars Kellogg-Stedman, see https://github.com/requests/requests/issues/2433

def iterate_stream_nonblock(stream, chunk_size=128):
    pending = None

    while True:
        try:
            chunk = os.read(stream.raw.fileno(), chunk_size)
        except BlockingIOError:
            break
        if not chunk:
            break

        if pending is not None:
            chunk = pending + chunk
            pending = None

        lines = chunk.splitlines()

        if lines and lines[-1]:
            pending = lines.pop()

        for line in lines:
            yield line

        if not pending:
            break

    if pending:
        yield pending

# ---------------------------------------------------------------------------------------------------------------------

def create_stream(baseurl):
    stream = requests.get("{}/stream".format(baseurl), stream=True, timeout=0.1)

    if stream.encoding is None:
        stream.encoding = 'utf-8'

    return stream

# ---------------------------------------------------------------------------------------------------------------------
# Carla Host object for connecting to the REST API backend

class CarlaHostQtWeb(CarlaHostQtNull):
    def __init__(self):
        CarlaHostQtNull.__init__(self)

        self.baseurl = "http://localhost:2228"
        self.stream  = create_stream(self.baseurl)

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
        return requests.get("{}/engine_init/{}/{}".format(self.baseurl, driverName, clientName)).status_code == 200

    def engine_close(self):
        return requests.get("{}/engine_close".format(self.baseurl)).status_code == 200

    def engine_idle(self):
        closed = False
        stream = self.stream

        for line in iterate_stream_nonblock(stream):
            line = line.decode('utf-8', errors='ignore')

            if line.startswith("Carla: "):
                if self.fEngineCallback is None:
                    continue

                # split values from line
                action, pluginId, value1, value2, value3, valueStr = line[7:].split(" ",5)

                # convert to proper types
                action   = int(action)
                pluginId = int(pluginId)
                value1   = int(value1)
                value2   = int(value2)
                value3   = float(value3)

                # pass to callback
                self.fEngineCallback(None, action, pluginId, value1, value2, value3, valueStr)

            elif line == "Connection: close":
                if not closed:
                    self.stream = create_stream(self.baseurl)
                    closed = True

        if closed:
            stream.close()

    def is_engine_running(self):
        try:
            return requests.get("{}/is_engine_running".format(self.baseurl)).status_code == 200
        except requests.exceptions.ConnectionError:
            if self.fEngineCallback is None:
                self.fEngineCallback(None, ENGINE_CALLBACK_QUIT, 0, 0, 0, 0.0, "")

    def set_engine_about_to_close(self):
        return requests.get("{}/set_engine_about_to_close".format(self.baseurl)).status_code == 200

    def set_engine_option(self, option, value, valueStr):
        requests.get("{}/set_engine_option/{}/{}/{}".format(self.baseurl, option, value, valueStr))

    def load_file(self, filename):
        return requests.get("{}/load_file/{}".format(self.baseurl, filename)).status_code == 200

    def load_project(self, filename):
        return requests.get("{}/load_project/{}".format(self.baseurl, filename)).status_code == 200

    def save_project(self, filename):
        return requests.get("{}/save_project/{}".format(self.baseurl, filename)).status_code == 200

    def patchbay_connect(self, groupIdA, portIdA, groupIdB, portIdB):
        return requests.get("{}/patchbay_connect/{}/{}/{}/{}".format(self.baseurl, groupIdA, portIdA, groupIdB, portIdB)).status_code == 200

    def patchbay_disconnect(self, connectionId):
        return requests.get("{}/patchbay_disconnect/{}".format(self.baseurl, connectionId)).status_code == 200

    def patchbay_refresh(self, external):
        return requests.get("{}/patchbay_refresh/{}".format(self.baseurl, external)).status_code == 200

    def transport_play(self):
        requests.get("{}/transport_play".format(self.baseurl))

    def transport_pause(self):
        requests.get("{}/transport_pause".format(self.baseurl))

    def transport_bpm(self, bpm):
        requests.get("{}/transport_bpm/{}".format(self.baseurl, bpm))

    def transport_relocate(self, frame):
        requests.get("{}/transport_relocate/{}".format(self.baseurl, frame))

    def get_current_transport_frame(self):
        return int(requests.get("{}/get_current_transport_frame".format(self.baseurl)).text)

    def get_transport_info(self):
        return requests.get("{}/get_transport_info".format(self.baseurl)).json()

    def get_current_plugin_count(self):
        return int(requests.get("{}/get_current_plugin_count".format(self.baseurl)).text)

    def get_max_plugin_number(self):
        return int(requests.get("{}/get_max_plugin_number".format(self.baseurl)).text)

    def add_plugin(self, btype, ptype, filename, name, label, uniqueId, extraPtr, options):
        return requests.get("{}/add_plugin/{}/{}/{}/{}/{}/{}/{}/{}".format(self.baseurl, btype, ptype, filename, name, label, uniqueId, extraPtr, options)).status_code == 200

    def remove_plugin(self, pluginId):
        return requests.get("{}/remove_plugin/{}".format(self.baseurl, pluginId)).status_code == 200

    def remove_all_plugins(self):
        return requests.get("{}/remove_all_plugins".format(self.baseurl)).status_code == 200

    def rename_plugin(self, pluginId, newName):
        return requests.get("{}/rename_plugin/{}/{}".format(self.baseurl, pluginId, newName)).text

    def clone_plugin(self, pluginId):
        return requests.get("{}/clone_plugin/{}".format(self.baseurl, pluginId)).status_code == 200

    def replace_plugin(self, pluginId):
        return requests.get("{}/replace_plugin/{}".format(self.baseurl, pluginId)).status_code == 200

    def switch_plugins(self, pluginIdA, pluginIdB):
        return requests.get("{}/switch_plugins/{}/{}".format(self.baseurl, pluginIdA, pluginIdB)).status_code == 200

    def load_plugin_state(self, pluginId, filename):
        return requests.get("{}/load_plugin_state/{}/{}".format(self.baseurl, pluginId, filename)).status_code == 200

    def save_plugin_state(self, pluginId, filename):
        return requests.get("{}/save_plugin_state/{}/{}".format(self.baseurl, pluginId, filename)).status_code == 200

    def export_plugin_lv2(self, pluginId, lv2path):
        return requests.get("{}/export_plugin_lv2/{}/{}".format(self.baseurl, pluginId, lv2path)).status_code == 200

    def get_plugin_info(self, pluginId):
        return requests.get("{}/get_plugin_info/{}".format(self.baseurl, pluginId)).json()

    def get_audio_port_count_info(self, pluginId):
        return requests.get("{}/get_audio_port_count_info/{}".format(self.baseurl, pluginId)).json()

    def get_midi_port_count_info(self, pluginId):
        return requests.get("{}/get_midi_port_count_info/{}".format(self.baseurl, pluginId)).json()

    def get_parameter_count_info(self, pluginId):
        return requests.get("{}/get_parameter_count_info/{}".format(self.baseurl, pluginId)).json()

    def get_parameter_info(self, pluginId, parameterId):
        return requests.get("{}/get_parameter_info/{}/{}".format(self.baseurl, pluginId, parameterId)).json()

    def get_parameter_scalepoint_info(self, pluginId, parameterId, scalePointId):
        return requests.get("{}/get_parameter_scalepoint_info/{}/{}/{}".format(self.baseurl, pluginId, parameterId, scalePointId)).json()

    def get_parameter_data(self, pluginId, parameterId):
        return requests.get("{}/get_parameter_data/{}/{}".format(self.baseurl, pluginId, parameterId)).json()

    def get_parameter_ranges(self, pluginId, parameterId):
        return requests.get("{}/get_parameter_ranges/{}/{}".format(self.baseurl, pluginId, parameterId)).json()

    def get_midi_program_data(self, pluginId, midiProgramId):
        return requests.get("{}/get_midi_program_data/{}/{}".format(self.baseurl, pluginId, midiProgramId)).json()

    def get_custom_data(self, pluginId, customDataId):
        return requests.get("{}/get_custom_data/{}/{}".format(self.baseurl, pluginId, customDataId)).json()

    def get_chunk_data(self, pluginId):
        return requests.get("{}/get_chunk_data/{}".format(self.baseurl, pluginId)).text

    def get_parameter_count(self, pluginId):
        return int(requests.get("{}/get_parameter_count/{}".format(self.baseurl, pluginId)).text)

    def get_program_count(self, pluginId):
        return int(requests.get("{}/get_program_count/{}".format(self.baseurl, pluginId)).text)

    def get_midi_program_count(self, pluginId):
        return int(requests.get("{}/get_midi_program_count/{}".format(self.baseurl, pluginId)).text)

    def get_custom_data_count(self, pluginId):
        return int(requests.get("{}/get_custom_data_count/{}".format(self.baseurl, pluginId)).text)

    def get_parameter_text(self, pluginId, parameterId):
        return requests.get("{}/get_parameter_text/{}/{}".format(self.baseurl, pluginId, parameterId)).text

    def get_program_name(self, pluginId, programId):
        return requests.get("{}/get_program_name/{}/{}".format(self.baseurl, pluginId, programId)).text

    def get_midi_program_name(self, pluginId, midiProgramId):
        return requests.get("{}/get_midi_program_name/{}/{}".format(self.baseurl, pluginId, midiProgramId)).text

    def get_real_plugin_name(self, pluginId):
        return requests.get("{}/get_real_plugin_name/{}".format(self.baseurl, pluginId)).text

    def get_current_program_index(self, pluginId):
        return int(requests.get("{}/get_custom_data_count/{}".format(self.baseurl, pluginId)).text)

    def get_current_midi_program_index(self, pluginId):
        return int(requests.get("{}/get_custom_data_count/{}".format(self.baseurl, pluginId)).text)

    def get_default_parameter_value(self, pluginId, parameterId):
        return float(requests.get("{}/get_default_parameter_value/{}/{}".format(self.baseurl, pluginId, parameterId)).text)

    def get_current_parameter_value(self, pluginId, parameterId):
        return float(requests.get("{}/get_current_parameter_value/{}/{}".format(self.baseurl, pluginId, parameterId)).text)

    def get_internal_parameter_value(self, pluginId, parameterId):
        return float(requests.get("{}/get_internal_parameter_value/{}/{}".format(self.baseurl, pluginId, parameterId)).text)

    def get_input_peak_value(self, pluginId, isLeft):
        return float(requests.get("{}/get_input_peak_value/{}/{}".format(self.baseurl, pluginId, isLeft)).text)

    def get_output_peak_value(self, pluginId, isLeft):
        return float(requests.get("{}/get_output_peak_value/{}/{}".format(self.baseurl, pluginId, isLeft)).text)

    def set_option(self, pluginId, option, yesNo):
        requests.get("{}/set_option/{}/{}/{}".format(self.baseurl, pluginId, option, yesNo))

    def set_active(self, pluginId, onOff):
        requests.get("{}/set_active/{}/{}".format(self.baseurl, pluginId, onOff))

    def set_drywet(self, pluginId, value):
        requests.get("{}/set_drywet/{}/{}".format(self.baseurl, pluginId, value))

    def set_volume(self, pluginId, value):
        requests.get("{}/set_volume/{}/{}".format(self.baseurl, pluginId, value))

    def set_balance_left(self, pluginId, value):
        requests.get("{}/set_balance_left/{}/{}".format(self.baseurl, pluginId, value))

    def set_balance_right(self, pluginId, value):
        requests.get("{}/set_balance_right/{}/{}".format(self.baseurl, pluginId, value))

    def set_panning(self, pluginId, value):
        requests.get("{}/set_panning/{}/{}".format(self.baseurl, pluginId, value))

    def set_ctrl_channel(self, pluginId, channel):
        requests.get("{}/set_ctrl_channel/{}/{}".format(self.baseurl, pluginId, channel))

    def set_parameter_value(self, pluginId, parameterId, value):
        requests.get("{}/set_parameter_value/{}/{}/{}".format(self.baseurl, pluginId, parameterId, value))

    def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        requests.get("{}/set_parameter_midi_channel/{}/{}/{}".format(self.baseurl, pluginId, parameterId, channel))

    def set_parameter_midi_cc(self, pluginId, parameterId, cc):
        requests.get("{}/set_parameter_midi_cc/{}/{}/{}".format(self.baseurl, pluginId, parameterId, cc))

    def set_program(self, pluginId, programId):
        requests.get("{}/set_program/{}/{}".format(self.baseurl, pluginId, programId))

    def set_midi_program(self, pluginId, midiProgramId):
        requests.get("{}/set_midi_program/{}/{}".format(self.baseurl, pluginId, midiProgramId))

    def set_custom_data(self, pluginId, type_, key, value):
        requests.get("{}/set_custom_data/{}/{}/{}/{}".format(self.baseurl, pluginId, type_, key, value))

    def set_chunk_data(self, pluginId, chunkData):
        requests.get("{}/set_chunk_data/{}/{}".format(self.baseurl, pluginId, chunkData))

    def prepare_for_save(self, pluginId):
        requests.get("{}/prepare_for_save/{}".format(self.baseurl, pluginId))

    def reset_parameters(self, pluginId):
        requests.get("{}/reset_parameters/{}".format(self.baseurl, pluginId))

    def randomize_parameters(self, pluginId):
        requests.get("{}/randomize_parameters/{}".format(self.baseurl, pluginId))

    def send_midi_note(self, pluginId, channel, note, velocity):
        requests.get("{}/send_midi_note/{}/{}/{}/{}".format(self.baseurl, pluginId, channel, note, velocity))

    def get_buffer_size(self):
        return int(requests.get("{}/get_buffer_size".format(self.baseurl)).text)

    def get_sample_rate(self):
        return float(requests.get("{}/get_sample_rate".format(self.baseurl)).text)

    def get_last_error(self):
        return requests.get("{}/get_last_error".format(self.baseurl)).text

    def get_host_osc_url_tcp(self):
        return requests.get("{}/get_host_osc_url_tcp".format(self.baseurl)).text

    def get_host_osc_url_udp(self):
        return requests.get("{}/get_host_osc_url_udp".format(self.baseurl)).text

# ---------------------------------------------------------------------------------------------------------------------
# TESTING

if __name__ == '__main__':

    baseurl = "http://localhost:2228"

    #driver_count = int(requests.get("{}/get_engine_driver_count".format(baseurl)).text)

    ## FIXME
    #driver_count -= 1

    #print("Driver names:")
    #for index in range(driver_count):
        #print("\t -", requests.get("{}/get_engine_driver_name/{}".format(baseurl, index)).text)

    #print("Driver device names:")
    #for index in range(driver_count):
        #for name in requests.get("{}/get_engine_driver_device_names/{}".format(baseurl, index)).text.split("\n"):
            #print("\t {}:".format(name), requests.get("{}/get_engine_driver_device_info/{}/{}".format(baseurl, index, name)).json())

    requests.get("{}/engine_close".format(baseurl)).status_code

    if requests.get("{}/engine_init/{}/{}".format(baseurl, "JACK", "test")).status_code == 200:
        stream = requests.get("{}/stream".format(baseurl), stream=True, timeout=0.25)
        #stream.add_done_callback(cb)
        #print(stream, dir(stream))

        #for line in stream.iter_lines():
            #print("line", stream, line)

        while True:
            print("idle")
            for line in iterate_stream_nonblock(stream):
                print("line", line)
            sleep(0.5)

        requests.get("{}/engine_close".format(baseurl)).status_code

# ---------------------------------------------------------------------------------------------------------------------
