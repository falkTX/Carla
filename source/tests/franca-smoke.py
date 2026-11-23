#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

from ctypes import CDLL, CFUNCTYPE, c_bool, c_char_p, c_int, c_void_p
from os import fsdecode, fsencode
from pathlib import Path
from platform import machine
from tempfile import TemporaryDirectory
from unittest import TestCase, main
from xml.etree import ElementTree

from carla_backend import (
    BINARY_NATIVE,
    BINARY_WIN64,
    CarlaHostDLL,
    ENGINE_OPTION_OSC_ENABLED,
    ENGINE_OPTION_PATH_RESOURCES,
    ENGINE_OPTION_PROCESS_MODE,
    ENGINE_OPTION_TRANSPORT_MODE,
    ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
    ENGINE_TRANSPORT_MODE_INTERNAL,
    PLUGIN_INTERNAL,
    PLUGIN_OPTIONS_NULL,
    PLUGIN_VST3,
)


BIN = Path(__file__).resolve().parents[2] / "bin"


class ProjectTests(TestCase):
    def setUp(self):
        directory = TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.project = Path(directory.name) / "mic.carxp"
        self.host = CarlaHostDLL(str(BIN / "libcarla_standalone2.so"), False)
        self.host.set_engine_option(ENGINE_OPTION_OSC_ENABLED, 0, "")
        self.host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, str(BIN / "resources"))
        self.host.set_engine_option(ENGINE_OPTION_PROCESS_MODE, ENGINE_PROCESS_MODE_CONTINUOUS_RACK, "")
        self.host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, ENGINE_TRANSPORT_MODE_INTERNAL, "")
        self.assertTrue(self.host.engine_init("Dummy", "Franca tests"), self.host.get_last_error())
        self.addCleanup(self.host.engine_close)

    def save_rack(self):
        for name in ("Mic & gain", "Output gain"):
            self.assertTrue(self.host.add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, "", name, "audiogain_s",
                                                 0, None, PLUGIN_OPTIONS_NULL), self.host.get_last_error())
        self.host.set_parameter_value(0, 0, 0.75)
        self.host.set_parameter_value(1, 0, 0.5)
        self.assertTrue(self.host.save_project(str(self.project)), self.host.get_last_error())
        self.assertTrue(self.host.remove_all_plugins(), self.host.get_last_error())

    def test_legacy_rack_round_trip(self):
        self.save_rack()
        project = ElementTree.parse(self.project)
        project.getroot().set("VERSION", "2.0")
        project.write(self.project, encoding="utf-8")

        for _ in range(3):
            self.assertTrue(self.host.load_project(str(self.project)), self.host.get_last_error())
            self.assertEqual(self.host.get_current_plugin_count(), 2)
            self.assertEqual(self.host.get_plugin_info(0)["name"], "Mic & gain")
            self.assertEqual(self.host.get_plugin_info(1)["name"], "Output gain")
            self.assertAlmostEqual(self.host.get_current_parameter_value(0, 0), 0.75)
            self.assertAlmostEqual(self.host.get_current_parameter_value(1, 0), 0.5)
            self.assertTrue(self.host.save_project(str(self.project)), self.host.get_last_error())
            self.assertTrue(self.host.remove_all_plugins(), self.host.get_last_error())

    def test_incomplete_load_protects_original(self):
        self.save_rack()
        project = ElementTree.parse(self.project)
        project.find("Plugin/Info/Label").text = "unavailable-plugin"
        project.write(self.project, encoding="utf-8")
        original = self.project.read_bytes()

        self.assertFalse(self.host.load_project(str(self.project)))
        self.assertEqual(self.host.get_current_plugin_count(), 1)
        self.assertFalse(self.host.save_project(str(self.project)))
        self.assertEqual(self.project.read_bytes(), original)
        self.assertTrue(self.host.save_project(str(self.project.with_name("recovered.carxp"))),
                        self.host.get_last_error())


class DiscoveryTests(TestCase):
    def test_discovery_uses_vst3_bundle_binary(self):
        utils = CDLL(str(BIN / "libcarla_utils.so"))
        discovery_callback = CFUNCTYPE(None, c_void_p, c_void_p, c_char_p)
        cache_callback = CFUNCTYPE(c_bool, c_void_p, c_char_p, c_char_p)
        start = utils.carla_plugin_discovery_start
        start.argtypes = [c_char_p, c_int, c_int, c_char_p, discovery_callback, cache_callback, c_void_p]
        start.restype = c_void_p
        utils.carla_plugin_discovery_idle.argtypes = [c_void_p]
        utils.carla_plugin_discovery_idle.restype = c_bool
        utils.carla_plugin_discovery_stop.argtypes = [c_void_p]
        cached = []

        @discovery_callback
        def ignore_plugin(_context, _info, _key):
            pass

        @cache_callback
        def capture_key(_context, filename, key):
            cached.append((Path(fsdecode(filename)), key))
            return True

        with TemporaryDirectory() as directory:
            bundle = Path(directory) / "Gain.vst3"
            for platform, suffix, binary_type in ((machine() + "-linux", ".so", BINARY_NATIVE),
                                                  ("x86_64-win", ".vst3", BINARY_WIN64)):
                binary = bundle / "Contents" / platform / ("Gain" + suffix)
                binary.parent.mkdir(parents=True)
                for contents in (b"old", b"updated"):
                    binary.write_bytes(contents)
                    handle = start(fsencode(BIN / "carla-discovery-native"), binary_type, PLUGIN_VST3,
                                   fsencode(directory), ignore_plugin, capture_key, None)
                    self.assertTrue(handle)
                    try:
                        self.assertFalse(utils.carla_plugin_discovery_idle(handle))
                    finally:
                        utils.carla_plugin_discovery_stop(handle)

        self.assertEqual([path for path, _ in cached], [bundle] * 4)
        self.assertEqual(len({key for _, key in cached}), 4)


if __name__ == "__main__":
    main()
