#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

import os
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtWidgets import QApplication
else:
    from PyQt6.QtWidgets import QApplication

import franca_logging
from carla_backend import (
    BINARY_NATIVE,
    CUSTOM_DATA_TYPE_PROPERTY,
    ENGINE_OPTION_OSC_ENABLED,
    ENGINE_OPTION_PROCESS_MODE,
    ENGINE_OPTION_TRANSPORT_MODE,
    ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
    ENGINE_TRANSPORT_MODE_INTERNAL,
    PARAMETER_ACTIVE,
    PLUGIN_CATEGORY_DELAY,
    PLUGIN_CATEGORY_DYNAMICS,
    PLUGIN_CATEGORY_EQ,
    PLUGIN_CATEGORY_NONE,
    PLUGIN_CATEGORY_SYNTH,
    PLUGIN_INTERNAL,
    PLUGIN_OPTIONS_NULL,
)
from carla_skin import (
    ClipWarningIndicator,
    FRANCA_CATEGORY_PROPERTY,
    FRANCA_PLUGIN_CATEGORIES,
    createCategoryColorIcon,
    getCategoryColor,
)
from widgets.digitalpeakmeter import DigitalPeakMeter


class MeterAndClipWarningTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def test_peak_meter_turns_red_on_0db_even_with_smoothing(self):
        meter = DigitalPeakMeter(None)
        meter.setChannelCount(2)
        meter.setUseSystemColors(True, colorLevels=True)
        meter.setSmoothMultiplier(2)

        meter.displayMeter(1, 0.3)
        self.assertFalse(meter.isClipping())

        # Single 0 dB hit must immediately register as clipping without being smoothed away
        meter.displayMeter(1, 1.0)
        self.assertTrue(meter.isClipping())
        self.assertEqual(meter.fChannelData[0], 1.0)

        # Forced zero clears clip hold
        meter.displayMeter(1, 0.0, True)
        self.assertFalse(meter.isClipping())

    def test_clip_warning_triangle_triggers_on_repeated_0db_hits(self):
        ind = ClipWarningIndicator()
        self.assertFalse(ind.isWarningActive())

        # Isolated hit should NOT trigger the persistent warning triangle
        ind.recordPeak(1.0, now=10.0)
        self.assertFalse(ind.isWarningActive())

        # Repeated 0 dB hits within the window trigger the warning triangle
        ind.recordPeak(1.0, now=10.2)
        ind.recordPeak(1.0, now=10.4)
        ind.recordPeak(1.0, now=10.6)
        self.assertTrue(ind.isWarningActive())

        # Clean signal for CLEAR_AFTER_SECONDS auto-clears the warning
        ind.recordPeak(0.2, now=10.6 + ClipWarningIndicator.CLEAR_AFTER_SECONDS + 0.1)
        self.assertFalse(ind.isWarningActive())

        # Manual dismiss works immediately
        for t in (20.0, 20.1, 20.2, 20.3):
            ind.recordPeak(1.0, now=t)
        self.assertTrue(ind.isWarningActive())
        ind.clearWarning()
        self.assertFalse(ind.isWarningActive())


class CategoryAndLoggingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def test_all_categories_have_distinct_colors_and_icons(self):
        colored = [(cid, color.name()) for cid, _lbl, _key, color in FRANCA_PLUGIN_CATEGORIES if color is not None]
        self.assertEqual(len(colored), 9)
        self.assertEqual(len({hex_c for _, hex_c in colored}), 9)
        self.assertIsNone(getCategoryColor(PLUGIN_CATEGORY_NONE))
        for _cid, _lbl, _key, color in FRANCA_PLUGIN_CATEGORIES:
            icon = createCategoryColorIcon(color)
            self.assertFalse(icon.isNull())

    def test_rotating_session_logging_caps_file_size(self):
        with TemporaryDirectory() as tmp:
            log_file = franca_logging.setup_logging(
                log_dir=tmp,
                max_bytes=1024,
                backup_count=2,
                capture_stdio=False,
                install_qt=False,
            )
            for i in range(120):
                franca_logging.log_message(f"Test log entry #{i} " + ("x" * 40))

            files = sorted(Path(tmp).glob("franca.log*"))
            self.assertGreaterEqual(len(files), 2)
            self.assertLessEqual(len(files), 3)  # franca.log + at most 2 backups
            self.assertTrue(log_file.exists())
            total_size = sum(f.stat().st_size for f in files)
            self.assertLess(total_size, 6 * 1024)


class FrancaWindowUiAndRackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def test_rack_layout_categories_patchbay_undo_and_bypass(self):
        import resources_rc  # noqa: F401
        import carla_shared
        from carla_host import initHost
        from carla_shared import CARLA_KEY_ENGINE_AUDIO_DRIVER, gCarla
        from franca_window import FrancaWindow
        from patchcanvas import patchcanvas

        root_dir = Path(__file__).resolve().parents[2]
        bin_dir = root_dir / "bin"
        if not (bin_dir / "libcarla_standalone2.so").exists():
            self.skipTest("libcarla_standalone2.so not built")

        tmp_cfg = TemporaryDirectory()
        tmp_logs = TemporaryDirectory()
        self.addCleanup(tmp_cfg.cleanup)
        self.addCleanup(tmp_logs.cleanup)

        franca_logging.setup_logging(
            log_dir=tmp_logs.name,
            max_bytes=64 * 1024,
            backup_count=2,
            capture_stdio=False,
            install_qt=False,
        )

        prev_xdg = os.environ.get("XDG_CONFIG_HOME")
        os.environ["XDG_CONFIG_HOME"] = tmp_cfg.name
        prev_cwd = carla_shared.CWD
        prev_argv = list(sys.argv)
        carla_shared.CWD = str(root_dir / "source" / "frontend")
        sys.argv = ["franca"]
        try:
            host = initHost("Franca", None, False, False, False)
            self.assertIsNotNone(host)
            host.isFranca = True
            host.audioDriverForced = "Dummy"
            host.processMode = ENGINE_PROCESS_MODE_CONTINUOUS_RACK
            host.nextProcessMode = ENGINE_PROCESS_MODE_CONTINUOUS_RACK
            host.processModeForced = True
            host.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL
            host.set_engine_option(ENGINE_OPTION_OSC_ENABLED, 0, "")

            gui = FrancaWindow(host)
        finally:
            carla_shared.CWD = prev_cwd
            sys.argv = prev_argv

        gui.fSavedSettings[CARLA_KEY_ENGINE_AUDIO_DRIVER] = "Dummy"
        gCarla.gui = gui
        gui.resize(1000, 640)
        try:
            QApplication.processEvents()
            self.assertTrue(host.is_engine_running(), host.get_last_error())

            # 1. Expand/Minimize actions are hidden in Franca
            self.assertFalse(gui.ui.act_plugins_compact.isVisible())
            self.assertFalse(gui.ui.act_plugins_expand.isVisible())

            # 2. Patchbay toolbar has Undo & Redo actions
            pb_actions = gui.patchbayToolbar.actions()
            self.assertIn(gui.editHistory.undoAction, pb_actions)
            self.assertIn(gui.editHistory.redoAction, pb_actions)

            # 3. Add two plugins with very different name lengths and verify aligned button column
            self.assertTrue(host.add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, "", "EQ", "audiogain_s",
                                            0, None, PLUGIN_OPTIONS_NULL), host.get_last_error())
            self.assertTrue(host.add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, "", "Very Long Vocal Compressor Chain",
                                            "audiogain_s", 0, None, PLUGIN_OPTIONS_NULL), host.get_last_error())
            QApplication.processEvents()

            slot0 = gui.getPluginSlotWidget(0)
            slot1 = gui.getPluginSlotWidget(1)
            self.assertIsNotNone(slot0)
            self.assertIsNotNone(slot1)

            slot0.resize(960, slot0.getFixedHeight())
            slot1.resize(960, slot1.getFixedHeight())
            slot0.layout().activate()
            slot1.layout().activate()

            # Buttons sit right next to plugin name, to the LEFT of format & meters, in an aligned vertical column
            self.assertGreater(slot0.b_gui.x(), slot0.label_name.x())
            self.assertLess(slot0.b_more.x(), slot0.label_type.x())
            self.assertLess(slot0.label_type.x(), slot0.peak_in.x())
            self.assertEqual(slot0.b_gui.x(), slot1.b_gui.x())
            self.assertEqual(slot0.b_edit.x(), slot1.b_edit.x())
            self.assertEqual(slot0.b_more.x(), slot1.b_more.x())

            # 4. Category color coding + custom data persistence + Undo/Redo
            gui.changePluginCategory(0, PLUGIN_CATEGORY_EQ)
            gui.changePluginCategory(1, PLUGIN_CATEGORY_DYNAMICS)
            self.assertEqual(slot0.getPluginCategory(), PLUGIN_CATEGORY_EQ)
            self.assertEqual(slot1.getPluginCategory(), PLUGIN_CATEGORY_DYNAMICS)
            self.assertEqual(
                host.get_custom_data_value(0, CUSTOM_DATA_TYPE_PROPERTY, FRANCA_CATEGORY_PROPERTY),
                "eq",
            )
            gui.editHistory.undoAction.trigger()
            self.assertNotEqual(slot1.getPluginCategory(), PLUGIN_CATEGORY_DYNAMICS)
            gui.editHistory.redoAction.trigger()
            self.assertEqual(slot1.getPluginCategory(), PLUGIN_CATEGORY_DYNAMICS)

            # 5. Rack bypass toggle (active=False) works cleanly and is undoable
            slot0.b_enable.click()
            QApplication.processEvents()
            self.assertEqual(host.get_internal_parameter_value(0, PARAMETER_ACTIVE), 0.0)
            self.assertEqual(host.get_internal_parameter_value(1, PARAMETER_ACTIVE), 1.0)

            # 6. Patchbay connect + Undo/Redo from patchbay toolbar
            cap_group = next(g for g in patchcanvas.canvas.group_list if "Capture" in g.group_name)
            carla_group = next(g for g in patchcanvas.canvas.group_list
                               if "Capture" not in g.group_name and "Playback" not in g.group_name)
            cap_port = next(p for p in patchcanvas.canvas.port_list if p.group_id == cap_group.group_id)
            in_port = next(p for p in patchcanvas.canvas.port_list
                           if p.group_id == carla_group.group_id and p.port_mode == patchcanvas.PORT_MODE_INPUT)

            self.assertTrue(gui.connectPatchbayPorts(cap_group.group_id, cap_port.port_id,
                                                     carla_group.group_id, in_port.port_id))
            QApplication.processEvents()
            self.assertGreaterEqual(len(patchcanvas.canvas.connection_list), 1)

            gui.editHistory.undoAction.trigger()
            QApplication.processEvents()
            self.assertEqual(len(patchcanvas.canvas.connection_list), 0)

            gui.editHistory.redoAction.trigger()
            QApplication.processEvents()
            self.assertGreaterEqual(len(patchcanvas.canvas.connection_list), 1)

            # 7. UI log messages were mirrored to the rotating session log file
            log_text = (Path(tmp_logs.name) / "franca.log").read_text(encoding="utf-8")
            self.assertIn("Franca session started", log_text)
            self.assertIn("Session log file:", log_text)
        finally:
            gui.killTimers()
            if host.is_engine_running():
                host.engine_close()
            QApplication.processEvents()
            gui.hide()
            gCarla.gui = None
            if patchcanvas.canvas.initiated:
                patchcanvas.clear()
            if prev_xdg is None:
                os.environ.pop("XDG_CONFIG_HOME", None)
            else:
                os.environ["XDG_CONFIG_HOME"] = prev_xdg


if __name__ == "__main__":
    unittest.main()
