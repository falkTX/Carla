# SPDX-License-Identifier: GPL-2.0-or-later

from math import isclose
from pathlib import Path
from tempfile import TemporaryDirectory
from xml.etree import ElementTree

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import Qt, pyqtSlot
    from PyQt5.QtGui import QFontDatabase, QIcon
    from PyQt5.QtWidgets import QAbstractItemView, QApplication, QFrame, QLabel
else:
    from PyQt6.QtCore import Qt, pyqtSlot
    from PyQt6.QtGui import QFontDatabase, QIcon
    from PyQt6.QtWidgets import QAbstractItemView, QApplication, QFrame, QLabel

from carla_host import HostWindow
from carla_backend import ENGINE_PROCESS_MODE_CONTINUOUS_RACK, PARAMETER_ACTIVE, PARAMETER_INPUT, PLUGIN_JACK
from franca_history import EditHistory
from patchcanvas import patchcanvas


class FrancaWindow(HostWindow):
    def __init__(self, host):
        super().__init__(host, True, displayName="Franca")
        self.editHistory = EditHistory(self)
        self.historyPluginCount = self.getPluginCount()
        host.CancelableActionCallback.connect(self.editHistory.block)
        host.ProjectLoadFinishedCallback.connect(self.editHistory.clear)
        host.EngineStartedCallback.connect(self.resetEditHistory)
        host.EngineStoppedCallback.connect(self.resetEditHistory)
        host.PluginAddedCallback.connect(self.recordPluginAdded)
        host.PluginRemovedCallback.connect(self.recordPluginRemoved)
        icon = QIcon(":/256x256/franca.png")
        QApplication.setWindowIcon(icon)
        self.setWindowIcon(icon)
        self.ui.frame.setFrameShape(QFrame.Shape.NoFrame)
        self.ui.listWidget.setMinimumWidth(0)
        self.ui.listWidget.setAlternatingRowColors(True)
        self.ui.listWidget.setVerticalScrollMode(QAbstractItemView.ScrollMode.ScrollPerPixel)
        self.ui.listWidget.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.ui.rackScrollBar.hide()
        self.ui.pad_left.hide()
        self.ui.pad_right.hide()
        self.ui.centralwidget.layout().setContentsMargins(0, 0, 0, 0)
        self.ui.tabWidget.tabBar().setExpanding(True)
        self.ui.tabWidget.setTabBarAutoHide(True)
        self.ui.text_logs.setFont(QFontDatabase.systemFont(QFontDatabase.SystemFont.FixedFont))

        toolbar = self.ui.toolBar
        toolbar.setWindowTitle(self.tr("Main Toolbar"))
        toolbar.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonFollowStyle)
        toolbar.clear()
        toolbar.addActions([self.ui.act_file_new, self.ui.act_file_open, self.ui.act_file_save])
        toolbar.addSeparator()
        toolbar.addActions(self.editHistory.actions)
        toolbar.addSeparator()
        toolbar.addAction(self.ui.act_plugin_add)

        self.ui.act_settings_show_side_panel.setText(self.tr("Show Files, Transport and Minimap"))
        self.ui.act_settings_configure.setText(self.tr("Configure Franca…"))
        self.ui.act_plugins_compact.setVisible(False)
        self.ui.act_plugins_expand.setVisible(False)

        self.patchbayToolbar.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonFollowStyle)
        self.patchbayToolbar.addSeparator()
        self.patchbayToolbar.addActions(self.editHistory.actions)
        self.patchbayToolbar.addSeparator()
        self.patchbayToolbar.addActions([self.ui.act_canvas_zoom_fit, self.ui.act_canvas_zoom_out,
                                       self.ui.act_canvas_zoom_in])
        for action, iconName in ((self.ui.act_file_new, "document-new"), (self.ui.act_file_open, "document-open"),
                                 (self.ui.act_file_save, "document-save"), (self.ui.act_plugin_add, "list-add"),
                                 (self.ui.act_canvas_refresh, "view-refresh"),
                                 (self.ui.act_canvas_arrange, "view-sort-ascending"),
                                 (self.ui.act_canvas_zoom_fit, "zoom-fit-best"),
                                 (self.ui.act_canvas_zoom_in, "zoom-in"), (self.ui.act_canvas_zoom_out, "zoom-out")):
            action.setIcon(QIcon.fromTheme(iconName + "-symbolic", action.icon()))

        self.ui.graphicsView.setFrameShape(QFrame.Shape.NoFrame)
        for meter in (self.ui.peak_in, self.ui.peak_out):
            meter.setUseSystemColors(True)

        status = self.statusBar()
        engine = QLabel(self.tr("Audio stopped"), status)
        engine.setTextFormat(Qt.TextFormat.PlainText)
        status.addWidget(engine)
        host.EngineStartedCallback.connect(
            lambda *args: engine.setText(self.tr("%s running") % args[-1]))
        host.EngineStoppedCallback.connect(lambda: engine.setText(self.tr("Audio stopped")))
        count = QLabel(self.tr("0 plugins"), status)
        count.setObjectName("francaPluginCount")
        status.addWidget(count, 1)

        self.ui.tw_statusbar.setFrameShape(QFrame.Shape.NoFrame)
        self.ui.tw_statusbar.layout().setContentsMargins(0, 0, 0, 0)
        self.ui.centralwidget.layout().removeWidget(self.ui.tw_statusbar)
        status.addPermanentWidget(self.ui.tw_statusbar)
        self.ui.label_buffer_size.setText(self.tr("Buffer"))
        self.ui.label_sample_rate.setText(self.tr("Sample rate"))
        self.ui.pb_dsp_load.setFormat(self.tr("DSP %p%"))

        def updateCount(*_):
            plugins = self.getPluginCount()
            count.setText(self.tr("%d plugin") % plugins if plugins == 1 else self.tr("%d plugins") % plugins)

        host.PluginAddedCallback.connect(updateCount)
        host.PluginRemovedCallback.connect(updateCount)
        host.EngineStoppedCallback.connect(updateCount)

    def resetEditHistory(self, *_):
        self.historyPluginCount = self.getPluginCount()
        self.editHistory.clear()
        self.editHistory.block(0, False, "")

    def loadProjectNow(self):
        self.editHistory.clear()
        try:
            return super().loadProjectNow()
        finally:
            self.editHistory.block(0, False, "")

    def pluginRemoveAll(self):
        self.editHistory.clear()
        return super().pluginRemoveAll()

    def movePlugin(self, source, target):
        moved = super().movePlugin(source, target)
        if moved != source:
            current = moved

            def move(destination):
                nonlocal current
                current = super(FrancaWindow, self).movePlugin(current, destination)
                if current != destination:
                    return self.host.get_last_error() or self.tr("The effect could not be moved.")

            self.editHistory.record(self.tr("Move Effect"), lambda: move(source), lambda: move(moved))
        return moved

    def renameRackPlugin(self, pluginId, name):
        before = self.host.get_plugin_info(pluginId)['name']
        if not super().renameRackPlugin(pluginId, name):
            return False
        after = self.host.get_plugin_info(pluginId)['name']
        if before != after:
            def rename(value):
                if not super(FrancaWindow, self).renameRackPlugin(pluginId, value):
                    return self.host.get_last_error() or self.tr("The effect could not be renamed.")

            self.editHistory.record(self.tr("Rename Effect"), lambda: rename(before), lambda: rename(after))
        return True

    def changePluginValue(self, pluginId, parameterId, value, setter):
        getter = self.host.get_internal_parameter_value if parameterId < 0 else self.host.get_current_parameter_value

        def setValue(newValue):
            if parameterId < 0:
                setter(pluginId, bool(newValue) if parameterId == PARAMETER_ACTIVE else newValue)
            else:
                setter(pluginId, parameterId, newValue)

        def replay(newValue):
            slot = self.getPluginSlotWidget(pluginId)
            if slot is None:
                return self.tr("The effect is no longer in the rack.")
            setValue(newValue)
            current = getter(pluginId, parameterId)
            slot.setParameterValue(parameterId, current, True)
            if not isclose(current, newValue, rel_tol=1e-6, abs_tol=1e-7):
                return self.tr("The effect did not accept the parameter value.")

        before = getter(pluginId, parameterId)
        setValue(value)
        after = getter(pluginId, parameterId)
        if before != after:
            if parameterId == PARAMETER_ACTIVE:
                label = self.tr("Enable Effect") if after else self.tr("Disable Effect")
                key = None
            else:
                label = self.tr("Change Parameter")
                key = (pluginId, parameterId)
            self.editHistory.record(label, lambda: replay(before), lambda: replay(after), key)

    def canRestorePlugin(self, pluginId):
        # shortcut: rack-only structural undo, extend when plugin-specific graph links can be restored.
        return (self.host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK
                and self.host.get_plugin_info(pluginId)['type'] != PLUGIN_JACK)

    def changePluginParameters(self, pluginId, label, operation):
        inputs = [i for i in range(self.host.get_parameter_count(pluginId))
                  if self.host.get_parameter_data(pluginId, i)['type'] == PARAMETER_INPUT]
        before = {i: self.host.get_current_parameter_value(pluginId, i) for i in inputs}
        operation(pluginId)
        after = {i: self.host.get_current_parameter_value(pluginId, i) for i in inputs}
        changed = [i for i in inputs if before[i] != after[i]]
        if changed:
            def apply(values):
                slot = self.getPluginSlotWidget(pluginId)
                if slot is None:
                    return self.tr("The effect is no longer in the rack.")
                for i in changed:
                    self.host.set_parameter_value(pluginId, i, values[i])
                    slot.setParameterValue(i, values[i], True)

            self.editHistory.record(label, lambda: apply(before), lambda: apply(after))

    def recordPluginAdded(self, pluginId, _pluginType, _name):
        before = self.historyPluginCount
        self.historyPluginCount = self.getPluginCount()
        if self.editHistory.busy or self.editHistory.blocked or self.fIsProjectLoading:
            return
        if self.historyPluginCount != before + 1 or not self.canRestorePlugin(pluginId):
            self.editHistory.clear()
            return
        state = None

        def undo():
            nonlocal state
            state = self.snapshotPlugin(pluginId)
            if state is None:
                return self.tr("Could not capture the effect state.")
            if not self.host.remove_plugin(pluginId):
                return self.host.get_last_error() or self.tr("The effect could not be removed.")

        self.editHistory.record(self.tr("Add Effect"), undo, lambda: self.restorePlugin(pluginId, state))

    def recordPluginRemoved(self, _pluginId):
        self.historyPluginCount = self.getPluginCount()
        self.editHistory.clear()

    def snapshotPlugin(self, pluginId):
        try:
            directory = TemporaryDirectory(prefix="franca-undo-")
            path = Path(directory.name) / "effect.carxs"
            if not self.host.save_plugin_state(pluginId, str(path)):
                return None
            preset = ElementTree.parse(path).getroot()
            # Appending a single-plugin project keeps the rack's project and JACK client settings.
            project = ElementTree.Element("CARLA-PROJECT", VERSION="2.6")
            ElementTree.SubElement(project, "Plugin").extend(preset)
            ElementTree.ElementTree(project).write(path.with_suffix(".carxp"), encoding="utf-8", xml_declaration=True)
            path.unlink()
            return directory
        except (OSError, ElementTree.ParseError):
            return None

    def restorePlugin(self, pluginId, state):
        previousCount = self.getPluginCount()
        try:
            path = Path(state.name) / "effect.carxp"
            if not self.host.load_file(str(path)):
                error = self.host.get_last_error() or self.tr("The effect could not be restored.")
                if self.getPluginCount() > previousCount:
                    self.host.remove_plugin(previousCount)
                return error
        finally:
            self.editHistory.block(0, False, "")
        if self.getPluginCount() != previousCount + 1:
            return self.tr("The effect could not be restored.")
        restored = previousCount
        moved = super().movePlugin(restored, pluginId)
        if moved != pluginId:
            self.host.remove_plugin(moved)
            return self.tr("Could not restore the effect to its place in the rack.")

    def removeRackPlugin(self, pluginId):
        if not self.canRestorePlugin(pluginId):
            return super().removeRackPlugin(pluginId)

        state = self.editHistory.run(lambda: self.snapshotPlugin(pluginId))
        if state is None:
            return False
        if not self.editHistory.run(lambda: self.host.remove_plugin(pluginId)):
            return False

        def remove():
            nonlocal state
            newState = self.snapshotPlugin(pluginId)
            if newState is None:
                return self.tr("Could not capture the effect state.")
            if not self.host.remove_plugin(pluginId):
                return self.host.get_last_error() or self.tr("The effect could not be removed.")
            state = newState

        self.editHistory.record(self.tr("Remove Effect"), lambda: self.restorePlugin(pluginId, state), remove)
        return True

    def patchbayEndpoints(self, ports):
        groups = {group.group_id: group.group_name for group in patchcanvas.canvas.group_list}
        names = {(port.group_id, port.port_id): (groups.get(port.group_id), port.port_name, port.port_mode)
                 for port in patchcanvas.canvas.port_list}
        return names.get(ports[:2]), names.get(ports[2:])

    def recordPatchbayEdit(self, endpoints, connected):
        if None in endpoints:
            return
        external = self.fExternalPatchbay

        def lookup_ids():
            names = {group.group_id: group.group_name for group in patchcanvas.canvas.group_list}
            return {(names.get(port.group_id), port.port_name, port.port_mode): (port.group_id, port.port_id)
                    for port in patchcanvas.canvas.port_list}

        def apply(connect):
            previous = self.fExternalPatchbay
            if external != previous:
                (self.slot_canvasShowExternal if external else self.slot_canvasShowInternal)()
                QApplication.processEvents()
            try:
                ids = lookup_ids()
                if any(endpoint not in ids for endpoint in endpoints):
                    self.slot_canvasRefresh()
                    QApplication.processEvents()
                    ids = lookup_ids()
                if any(endpoint not in ids for endpoint in endpoints):
                    return self.tr("One of the patchbay ports is unavailable.")
                output, input_ = (ids[endpoint] for endpoint in endpoints)
                connection = next((c.connection_id for c in patchcanvas.canvas.connection_list
                                   if (c.group_out_id, c.port_out_id) == output
                                   and (c.group_in_id, c.port_in_id) == input_), None)
                if connect and connection is None:
                    if not self.host.patchbay_connect(external, *output, *input_):
                        return self.host.get_last_error() or self.tr("The ports could not be connected.")
                if not connect and connection is not None:
                    if not self.host.patchbay_disconnect(external, connection):
                        return self.host.get_last_error() or self.tr("The ports could not be disconnected.")
            finally:
                if external != previous:
                    (self.slot_canvasShowExternal if previous else self.slot_canvasShowInternal)()
                    QApplication.processEvents()

        label = self.tr("Connect Ports") if connected else self.tr("Disconnect Ports")
        self.editHistory.record(label, lambda: apply(not connected), lambda: apply(connected))

    def connectPatchbayPorts(self, outputGroup, outputPort, inputGroup, inputPort):
        endpoints = self.patchbayEndpoints((outputGroup, outputPort, inputGroup, inputPort))
        if not super().connectPatchbayPorts(outputGroup, outputPort, inputGroup, inputPort):
            return False
        self.recordPatchbayEdit(endpoints, True)
        return True

    def disconnectPatchbayPorts(self, connectionId):
        connection = next((c for c in patchcanvas.canvas.connection_list if c.connection_id == connectionId), None)
        endpoints = (self.patchbayEndpoints((connection.group_out_id, connection.port_out_id,
                                             connection.group_in_id, connection.port_in_id))
                     if connection is not None else (None, None))
        if not super().disconnectPatchbayPorts(connectionId):
            return False
        self.recordPatchbayEdit(endpoints, False)
        return True

    def capturePatchbayPositions(self):
        return [
            (
                group.group_name,
                group.split,
                (int(round(group.widgets[0].x())), int(round(group.widgets[0].y()))),
                (int(round(group.widgets[1].x())), int(round(group.widgets[1].y())))
                if group.split and group.widgets[1] else (0, 0),
            )
            for group in patchcanvas.canvas.group_list
        ]

    def applyPatchbayPositions(self, positions):
        byName = {group.group_name: group for group in patchcanvas.canvas.group_list}
        for name, _split, (x1, y1), (x2, y2) in positions:
            group = byName.get(name)
            if group is None:
                continue
            patchcanvas.setGroupPosFull(group.group_id, x1, y1, x2, y2)
            bx1 = int(round(group.widgets[0].x()))
            by1 = int(round(group.widgets[0].y()))
            if group.split and group.widgets[1]:
                bx2 = int(round(group.widgets[1].x()))
                by2 = int(round(group.widgets[1].y()))
            else:
                bx2 = by2 = 0
            patchcanvas.CanvasCallback(
                patchcanvas.ACTION_GROUP_POSITION, group.group_id, 0,
                "%i:%i:%i:%i" % (bx1, by1, bx2, by2),
            )
        for group in patchcanvas.canvas.group_list:
            group.widgets[0].repaintLines(True)
            if group.split and group.widgets[1]:
                group.widgets[1].repaintLines(True)
        patchcanvas.canvas.scene.update()
        self.scene.zoom_fit()
        self.updateMiniCanvasLater()

    @pyqtSlot()
    def slot_canvasArrange(self):
        before = self.capturePatchbayPositions()
        super().slot_canvasArrange()
        after = self.capturePatchbayPositions()
        if before != after:
            external = self.fExternalPatchbay

            def apply(positions):
                previous = self.fExternalPatchbay
                if external != previous:
                    (self.slot_canvasShowExternal if external else self.slot_canvasShowInternal)()
                self.applyPatchbayPositions(positions)

            self.editHistory.record(self.tr("Arrange Patchbay"),
                                    lambda: apply(before), lambda: apply(after))
