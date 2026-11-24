#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

import os
import unittest

from patchcanvas.arrange import LayoutEdge, LayoutNode, compute_layout, edge_y_range_over_x


def make_node(key, name, in_ports=(), out_ports=(), width=160.0):
    port_step = 20.0
    header = 30.0
    in_map = {pid: header + idx * port_step + 8.0 for idx, pid in enumerate(in_ports)}
    out_map = {pid: header + idx * port_step + 8.0 for idx, pid in enumerate(out_ports)}
    rows = max(len(in_ports), len(out_ports), 1)
    height = header + rows * port_step + 6.0
    return LayoutNode(
        key=key,
        name=name,
        width=width,
        height=height,
        has_inputs=bool(in_ports),
        has_outputs=bool(out_ports),
        in_port_y=in_map,
        out_port_y=out_map,
    )


def assert_no_box_overlaps(test_case, nodes, positions, min_gap=0.0):
    by_key = {n.key: n for n in nodes}
    keys = list(positions.keys())
    for i in range(len(keys)):
        k1 = keys[i]
        n1 = by_key[k1]
        x1, y1 = positions[k1]
        for j in range(i + 1, len(keys)):
            k2 = keys[j]
            n2 = by_key[k2]
            x2, y2 = positions[k2]
            overlap_x = (x1 < x2 + n2.width + min_gap) and (x2 < x1 + n1.width + min_gap)
            overlap_y = (y1 < y2 + n2.height + min_gap) and (y2 < y1 + n1.height + min_gap)
            test_case.assertFalse(
                overlap_x and overlap_y,
                f"Boxes {n1.name} ({x1},{y1},{n1.width}x{n1.height}) and "
                f"{n2.name} ({x2},{y2},{n2.width}x{n2.height}) overlap",
            )


def count_edge_box_collisions(nodes, edges, positions, use_bezier=True):
    by_key = {n.key: n for n in nodes}
    hits = []
    for e in edges:
        if e.src_key == e.dst_key:
            continue
        sn = by_key[e.src_key]
        dn = by_key[e.dst_key]
        sx, sy = positions[e.src_key]
        dx, dy = positions[e.dst_key]
        x1 = sx + sn.width
        y1 = sy + sn.out_port_y[e.src_port_id]
        x2 = dx
        y2 = dy + dn.in_port_y[e.dst_port_id]
        if x2 <= x1:
            continue
        for n in nodes:
            if n.key in (e.src_key, e.dst_key):
                continue
            bx, by = positions[n.key]
            yrange = edge_y_range_over_x(x1, y1, x2, y2, bx, bx + n.width, use_bezier=use_bezier)
            if yrange is None:
                continue
            ey_min, ey_max = yrange
            if ey_max >= by and ey_min <= by + n.height:
                hits.append((e, n.name))
    return hits


def count_edge_crossings(nodes, edges, positions):
    """Count crossings between pairs of forward edges connecting adjacent/overlapping x-intervals."""
    by_key = {n.key: n for n in nodes}
    segs = []
    for e in edges:
        sn = by_key[e.src_key]
        dn = by_key[e.dst_key]
        sx, sy = positions[e.src_key]
        dx, dy = positions[e.dst_key]
        segs.append((
            sx + sn.width,
            sy + sn.out_port_y[e.src_port_id],
            dx,
            dy + dn.in_port_y[e.dst_port_id],
        ))

    def orient(ax, ay, bx, by, cx, cy):
        return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)

    crossings = 0
    for i in range(len(segs)):
        x1, y1, x2, y2 = segs[i]
        for j in range(i + 1, len(segs)):
            x3, y3, x4, y4 = segs[j]
            # Shared endpoints don't count as crossings
            if (x1 == x3 and y1 == y3) or (x2 == x4 and y2 == y4):
                continue
            o1 = orient(x1, y1, x2, y2, x3, y3)
            o2 = orient(x1, y1, x2, y2, x4, y4)
            o3 = orient(x3, y3, x4, y4, x1, y1)
            o4 = orient(x3, y3, x4, y4, x2, y2)
            if o1 * o2 < 0 and o3 * o4 < 0:
                crossings += 1
    return crossings


class ArrangeLayoutTests(unittest.TestCase):
    def test_reference_three_role_layout_unconnected_and_connected(self):
        mic = make_node("mic", "SSL 2+ Mic/Line/Inst 2", out_ports=[1], width=210)
        carla = make_node("carla", "Carla", in_ports=[1, 2, 3], out_ports=[4, 5, 6], width=190)
        dac = make_node("dac", "SMSL USB AUDIO Analog Stereo", in_ports=[1, 2], width=230)
        nodes = [dac, carla, mic]

        # Unconnected
        pos_unconn = compute_layout(nodes, [], canvas_rect=(0, 0, 3100, 2400))
        assert_no_box_overlaps(self, nodes, pos_unconn, min_gap=10)
        self.assertLess(pos_unconn["mic"][0] + mic.width, pos_unconn["carla"][0])
        self.assertLess(pos_unconn["carla"][0] + carla.width, pos_unconn["dac"][0])
        self.assertEqual(pos_unconn["mic"][1], pos_unconn["carla"][1])
        self.assertEqual(pos_unconn["carla"][1], pos_unconn["dac"][1])

        # Connected
        edges = [
            LayoutEdge("mic", 1, "carla", 1),
            LayoutEdge("carla", 4, "dac", 1),
            LayoutEdge("carla", 5, "dac", 2),
        ]
        pos_conn = compute_layout(nodes, edges, canvas_rect=(0, 0, 3100, 2400))
        assert_no_box_overlaps(self, nodes, pos_conn, min_gap=10)
        self.assertLess(pos_conn["mic"][0] + mic.width, pos_conn["carla"][0])
        self.assertLess(pos_conn["carla"][0] + carla.width, pos_conn["dac"][0])
        self.assertEqual(count_edge_box_collisions(nodes, edges, pos_conn), [])
        self.assertEqual(count_edge_crossings(nodes, edges, pos_conn), 0)

    def test_parallel_chains_have_zero_crossings(self):
        # Deliberately scrambled input order and names
        mic = make_node("mic", "Zeta Mic", out_ports=[1])
        synth = make_node("synth", "Alpha Synth", out_ports=[1])
        browser = make_node("browser", "Mid Browser", out_ports=[1])

        fx_mic = make_node("fx_mic", "A Vocal FX", in_ports=[1], out_ports=[2])
        fx_synth = make_node("fx_synth", "Z Synth FX", in_ports=[1], out_ports=[2])
        fx_browser = make_node("fx_browser", "M Browser EQ", in_ports=[1], out_ports=[2])

        out_mic = make_node("out_mic", "Stream", in_ports=[1])
        out_synth = make_node("out_synth", "Monitors", in_ports=[1])
        out_browser = make_node("out_browser", "Headphones", in_ports=[1])

        nodes = [out_synth, fx_mic, browser, out_mic, synth, fx_browser, mic, out_browser, fx_synth]
        edges = [
            LayoutEdge("mic", 1, "fx_mic", 1),
            LayoutEdge("fx_mic", 2, "out_mic", 1),
            LayoutEdge("synth", 1, "fx_synth", 1),
            LayoutEdge("fx_synth", 2, "out_synth", 1),
            LayoutEdge("browser", 1, "fx_browser", 1),
            LayoutEdge("fx_browser", 2, "out_browser", 1),
        ]

        pos = compute_layout(nodes, edges, canvas_rect=(0, 0, 3100, 2400))
        assert_no_box_overlaps(self, nodes, pos, min_gap=10)
        self.assertEqual(count_edge_box_collisions(nodes, edges, pos), [])
        self.assertEqual(count_edge_crossings(nodes, edges, pos), 0)

    def test_skip_layer_dry_wet_edge_avoids_intermediate_box(self):
        mic = make_node("mic", "Mic", out_ports=[1, 2])
        eq = make_node("eq", "EQ", in_ports=[1], out_ports=[2])
        reverb = make_node("reverb", "Reverb", in_ports=[1], out_ports=[2])
        spk = make_node("spk", "Speakers", in_ports=[1, 2])
        nodes = [mic, eq, reverb, spk]
        edges = [
            LayoutEdge("mic", 1, "spk", 1),     # Direct dry path skipping EQ and Reverb
            LayoutEdge("mic", 2, "eq", 1),
            LayoutEdge("eq", 2, "reverb", 1),
            LayoutEdge("reverb", 2, "spk", 2),
        ]

        for use_bezier in (True, False):
            pos = compute_layout(nodes, edges, canvas_rect=(0, 0, 3100, 2400), use_bezier=use_bezier)
            assert_no_box_overlaps(self, nodes, pos, min_gap=10)
            self.assertEqual(count_edge_box_collisions(nodes, edges, pos, use_bezier=use_bezier), [])
            self.assertEqual(count_edge_crossings(nodes, edges, pos), 0)
            # Column progression left to right
            self.assertLess(pos["mic"][0], pos["eq"][0])
            self.assertLess(pos["eq"][0], pos["reverb"][0])
            self.assertLess(pos["reverb"][0], pos["spk"][0])

    def test_port_order_fanout_avoids_crossing(self):
        hub = make_node("hub", "Hub", in_ports=[1], out_ports=[10, 11, 12])
        # Names deliberately reverse of port order to verify port-level crossing minimization
        top_sink = make_node("s1", "Zebra Audio", in_ports=[1, 2])
        bot_sink = make_node("s2", "Alpha MIDI", in_ports=[1])
        nodes = [hub, top_sink, bot_sink]
        edges = [
            LayoutEdge("hub", 10, "s1", 1),
            LayoutEdge("hub", 11, "s1", 2),
            LayoutEdge("hub", 12, "s2", 1),
        ]

        pos = compute_layout(nodes, edges, canvas_rect=(0, 0, 3100, 2400))
        assert_no_box_overlaps(self, nodes, pos, min_gap=10)
        self.assertLess(pos["s1"][1], pos["s2"][1])
        self.assertEqual(count_edge_crossings(nodes, edges, pos), 0)

    def test_multi_bus_skip_edges_and_isolated_nodes(self):
        mic = make_node("mic", "Mic", out_ports=[1])
        guitar = make_node("guitar", "Guitar", out_ports=[1])
        bass = make_node("bass", "Bass", out_ports=[1])
        unused_mic = make_node("unused_mic", "Webcam", out_ports=[1])

        vfx = make_node("vfx", "Vocal FX", in_ports=[1], out_ports=[2])
        amp = make_node("amp", "Amp Sim", in_ports=[1], out_ports=[2])
        mixer = make_node("mixer", "Mixer", in_ports=[1, 2, 3], out_ports=[4, 5])
        unused_fx = make_node("unused_fx", "Spare EQ", in_ports=[1], out_ports=[2])

        spk = make_node("spk", "Speakers", in_ports=[1, 2])
        unused_spk = make_node("unused_spk", "HDMI Out", in_ports=[1, 2])

        nodes = [unused_spk, spk, mixer, amp, vfx, unused_fx, bass, guitar, mic, unused_mic]
        edges = [
            LayoutEdge("mic", 1, "vfx", 1),
            LayoutEdge("vfx", 2, "mixer", 1),
            LayoutEdge("guitar", 1, "amp", 1),
            LayoutEdge("amp", 2, "mixer", 2),
            LayoutEdge("bass", 1, "mixer", 3),  # skips col 1
            LayoutEdge("mixer", 4, "spk", 1),
            LayoutEdge("mixer", 5, "spk", 2),
        ]

        pos = compute_layout(nodes, edges, canvas_rect=(0, 0, 3100, 2400))
        assert_no_box_overlaps(self, nodes, pos, min_gap=10)
        self.assertEqual(count_edge_box_collisions(nodes, edges, pos), [])
        self.assertEqual(count_edge_crossings(nodes, edges, pos), 0)

        # All output-only nodes share the leftmost column (right-aligned)
        source_rights = {pos[k][0] + next(n.width for n in nodes if n.key == k)
                         for k in ("mic", "guitar", "bass", "unused_mic")}
        self.assertEqual(len(source_rights), 1)
        # All input-only nodes share the rightmost column (left-aligned)
        self.assertEqual(pos["spk"][0], pos["unused_spk"][0])
        # Isolated nodes sit below the connected component
        for iso_k in ("unused_mic", "unused_fx", "unused_spk"):
            self.assertGreater(pos[iso_k][1], pos["spk"][1])

    def test_cycle_feedback_loop_does_not_hang_or_overlap(self):
        src = make_node("src", "Input", out_ports=[1])
        a = make_node("a", "Delay", in_ports=[1, 2], out_ports=[3])
        b = make_node("b", "Filter", in_ports=[1], out_ports=[2, 3])
        sink = make_node("sink", "Output", in_ports=[1])
        nodes = [src, a, b, sink]
        edges = [
            LayoutEdge("src", 1, "a", 1),
            LayoutEdge("a", 3, "b", 1),
            LayoutEdge("b", 2, "a", 2),  # feedback loop
            LayoutEdge("b", 3, "sink", 1),
        ]

        pos = compute_layout(nodes, edges, canvas_rect=(0, 0, 3100, 2400))
        assert_no_box_overlaps(self, nodes, pos, min_gap=10)
        self.assertLess(pos["src"][0], pos["a"][0])
        self.assertLess(pos["a"][0], pos["b"][0])
        self.assertLess(pos["b"][0], pos["sink"][0])


class PatchCanvasQtIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
        from qt_compat import qt_config
        if qt_config == 5:
            from PyQt5.QtWidgets import QApplication
        else:
            from PyQt6.QtWidgets import QApplication
        cls.app = QApplication.instance() or QApplication([])

    def test_patchcanvas_arrange_moves_boxes_and_updates_lines(self):
        from qt_compat import qt_config
        if qt_config == 5:
            from PyQt5.QtWidgets import QGraphicsView
        else:
            from PyQt6.QtWidgets import QGraphicsView
        from patchcanvas import patchcanvas

        events = []

        def callback(action, val1, val2, val_str):
            events.append((action, val1, val2, val_str))

        view = QGraphicsView()
        view.resize(1200, 800)
        scene = patchcanvas.PatchScene(None, view)
        view.setScene(scene)

        opts = patchcanvas.options_t()
        opts.theme_name = "System"
        opts.auto_hide_groups = True
        opts.auto_select_items = False
        opts.use_bezier_lines = True
        opts.antialiasing = patchcanvas.ANTIALIASING_SMALL
        opts.eyecandy = patchcanvas.EYECANDY_NONE
        opts.inline_displays = False

        feats = patchcanvas.features_t()
        feats.group_info = False
        feats.group_rename = False
        feats.port_info = False
        feats.port_rename = False
        feats.handle_group_pos = False

        if patchcanvas.canvas.initiated:
            patchcanvas.clear()
        patchcanvas.setOptions(opts)
        patchcanvas.setFeatures(feats)
        patchcanvas.init("FrancaArrangeTest", scene, callback, False)
        patchcanvas.setCanvasSize(0, 0, 3100, 2400)
        patchcanvas.setInitialPos(100, 100)

        try:
            # Split hardware group (output capture + input playback) + middle plugin group
            hw = patchcanvas.addGroup(1, "System", patchcanvas.SPLIT_YES, patchcanvas.ICON_HARDWARE)
            plug = patchcanvas.addGroup(2, "Franca", patchcanvas.SPLIT_NO, patchcanvas.ICON_APPLICATION)

            patchcanvas.addPort(1, 1, "capture_1", patchcanvas.PORT_MODE_OUTPUT, patchcanvas.PORT_TYPE_AUDIO_JACK)
            patchcanvas.addPort(1, 2, "playback_1", patchcanvas.PORT_MODE_INPUT, patchcanvas.PORT_TYPE_AUDIO_JACK)
            patchcanvas.addPort(2, 1, "audio-in1", patchcanvas.PORT_MODE_INPUT, patchcanvas.PORT_TYPE_AUDIO_JACK)
            patchcanvas.addPort(2, 2, "audio-out1", patchcanvas.PORT_MODE_OUTPUT, patchcanvas.PORT_TYPE_AUDIO_JACK)

            patchcanvas.connectPorts(1, 1, 1, 2, 1)
            patchcanvas.connectPorts(2, 2, 2, 1, 2)

            # Intentionally pile all boxes on top of each other
            patchcanvas.setGroupPosFull(1, 500, 500, 500, 500)
            patchcanvas.setGroupPos(2, 500, 500)
            events.clear()

            changed = patchcanvas.arrange()
            self.assertTrue(changed)

            cap_box = hw.widgets[0]
            pb_box = hw.widgets[1]
            plug_box = plug.widgets[0]

            # Capture (output-only) on left, Franca (in+out) in middle, Playback (input-only) on right
            self.assertLess(cap_box.x() + cap_box.boundingRect().width(), plug_box.x())
            self.assertLess(plug_box.x() + plug_box.boundingRect().width(), pb_box.x())

            # Backend position callbacks emitted with non-zero split coords
            pos_events = [e for e in events if e[0] == patchcanvas.ACTION_GROUP_POSITION]
            self.assertEqual(len(pos_events), 2)
            hw_evt = next(e for e in pos_events if e[1] == 1)
            x1, y1, x2, y2 = (int(v) for v in hw_evt[3].split(":"))
            self.assertTrue(x1 > 0 and y1 > 0 and x2 > 0 and y2 > 0)

            # Second arrange is idempotent
            events.clear()
            self.assertFalse(patchcanvas.arrange())
            self.assertEqual(events, [])
        finally:
            patchcanvas.clear()

    def test_franca_window_arrange_action_and_undo_redo(self):
        from pathlib import Path
        from qt_compat import qt_config
        if qt_config == 5:
            from PyQt5.QtWidgets import QApplication
        else:
            from PyQt6.QtWidgets import QApplication

        import resources_rc  # noqa: F401
        from carla_backend import (
            ENGINE_OPTION_OSC_ENABLED,
            ENGINE_OPTION_PROCESS_MODE,
            ENGINE_OPTION_TRANSPORT_MODE,
            ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
            ENGINE_TRANSPORT_MODE_INTERNAL,
        )
        import sys
        from tempfile import TemporaryDirectory
        import carla_shared
        from carla_backend import (
            ENGINE_OPTION_OSC_ENABLED,
            ENGINE_OPTION_PROCESS_MODE,
            ENGINE_OPTION_TRANSPORT_MODE,
            ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
            ENGINE_TRANSPORT_MODE_INTERNAL,
        )
        from carla_host import initHost
        from carla_shared import (
            CARLA_KEY_ENGINE_AUDIO_DRIVER,
            CARLA_KEY_ENGINE_PROCESS_MODE,
            CARLA_KEY_ENGINE_TRANSPORT_MODE,
            gCarla,
        )
        from franca_window import FrancaWindow
        from patchcanvas import patchcanvas

        root_dir = Path(__file__).resolve().parents[2]
        bin_dir = root_dir / "bin"
        lib_path = bin_dir / "libcarla_standalone2.so"
        if not lib_path.exists():
            self.skipTest("libcarla_standalone2.so not built")

        tmp_cfg = TemporaryDirectory()
        self.addCleanup(tmp_cfg.cleanup)
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
        try:
            self.assertIn(gui.ui.act_canvas_arrange, gui.patchbayToolbar.actions())
            self.assertTrue(gui.ui.act_canvas_arrange.isEnabled())

            # Let FrancaWindow's queued slot_engineStart() start the Dummy engine cleanly
            QApplication.processEvents()
            self.assertTrue(host.is_engine_running(), host.get_last_error())

            self.assertGreaterEqual(len(patchcanvas.canvas.group_list), 3)

            # Pile all groups at (400, 400) so Arrange has real work to do
            for group in patchcanvas.canvas.group_list:
                patchcanvas.setGroupPosFull(group.group_id, 400, 400, 400, 400)
            messy = gui.capturePatchbayPositions()

            gui.ui.act_canvas_arrange.trigger()
            QApplication.processEvents()
            arranged = gui.capturePatchbayPositions()
            self.assertNotEqual(messy, arranged)

            # Verify Capture (output-only) < Franca/Carla (in+out) < Playback (input-only)
            visible_x = {
                g.group_name: w.x()
                for g in patchcanvas.canvas.group_list
                for w in g.widgets
                if w is not None and w.isVisible()
            }
            cap_x = next(x for name, x in visible_x.items() if "Capture" in name)
            pb_x = next(x for name, x in visible_x.items() if "Playback" in name)
            mid_x = next(x for name, x in visible_x.items() if "Capture" not in name and "Playback" not in name)
            self.assertLess(cap_x, mid_x)
            self.assertLess(mid_x, pb_x)

            # Undo restores messy positions; Redo restores arranged positions
            self.assertTrue(gui.editHistory.undoAction.isEnabled())
            gui.editHistory.undoAction.trigger()
            QApplication.processEvents()
            self.assertEqual(gui.capturePatchbayPositions(), messy)

            self.assertTrue(gui.editHistory.redoAction.isEnabled())
            gui.editHistory.redoAction.trigger()
            QApplication.processEvents()
            self.assertEqual(gui.capturePatchbayPositions(), arranged)
        finally:
            if prev_xdg is None:
                os.environ.pop("XDG_CONFIG_HOME", None)
            else:
                os.environ["XDG_CONFIG_HOME"] = prev_xdg
            if host.is_engine_running():
                host.engine_close()
            QApplication.processEvents()
            gui.killTimers()
            gui.hide()
            gCarla.gui = None
            if patchcanvas.canvas.initiated:
                patchcanvas.clear()


if __name__ == "__main__":
    unittest.main()
