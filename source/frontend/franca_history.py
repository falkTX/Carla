# SPDX-License-Identifier: GPL-2.0-or-later

from time import monotonic

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtGui import QIcon, QKeySequence
    from PyQt5.QtWidgets import QAction, QMenu, QMessageBox
else:
    from PyQt6.QtGui import QAction, QIcon, QKeySequence
    from PyQt6.QtWidgets import QMenu, QMessageBox

from carla_shared import CustomMessageBox


class EditHistory:
    def __init__(self, window):
        self.window = window
        self.undo = []
        self.redo = []
        self.busy = False
        self.blocked = False
        self.mergeUntil = 0
        menu = QMenu(window.tr("&Edit"), window)
        window.menuBar().insertMenu(window.ui.menu_Engine.menuAction(), menu)
        self.undoAction = QAction(QIcon.fromTheme("edit-undo-symbolic", QIcon.fromTheme("edit-undo")), "", window)
        self.undoAction.setIconText(window.tr("Undo"))
        self.undoAction.setShortcut(QKeySequence("Ctrl+Z"))
        self.undoAction.triggered.connect(lambda: self.replay(self.undo, self.redo, True))
        self.redoAction = QAction(QIcon.fromTheme("edit-redo-symbolic", QIcon.fromTheme("edit-redo")), "", window)
        self.redoAction.setIconText(window.tr("Redo"))
        self.redoAction.setShortcuts([QKeySequence("Ctrl+Shift+Z"), QKeySequence("Ctrl+Y")])
        self.redoAction.triggered.connect(lambda: self.replay(self.redo, self.undo, False))
        self.actions = (self.undoAction, self.redoAction)
        menu.addActions(self.actions)
        self.updateActions()

    def updateActions(self):
        for action, stack, title in ((self.undoAction, self.undo, self.window.tr("&Undo")),
                                     (self.redoAction, self.redo, self.window.tr("&Redo"))):
            action.setText(title + " " + stack[-1][0] if stack else title)
            action.setEnabled(bool(stack) and not (self.busy or self.blocked))

    def clear(self, *_):
        if not self.busy:
            self.undo.clear()
            self.redo.clear()
            self.mergeUntil = 0
            self.updateActions()

    def block(self, _pluginId, started, _label):
        self.blocked = started
        self.updateActions()

    def separate(self, *_):
        self.mergeUntil = 0

    def record(self, label, undo, redo, mergeKey=None):
        if self.busy or self.blocked:
            return

        now = monotonic()
        if mergeKey is not None and self.undo and self.undo[-1][3] == mergeKey and now < self.mergeUntil:
            undo = self.undo.pop()[1]
        self.undo.append((label, undo, redo, mergeKey))
        if len(self.undo) > 50:
            del self.undo[0]
        self.redo.clear()
        self.mergeUntil = now + 0.4 if mergeKey is not None else 0
        self.updateActions()

    def run(self, operation):
        wasBusy = self.busy
        self.busy = True
        self.updateActions()
        try:
            return operation()
        finally:
            self.busy = wasBusy
            self.updateActions()

    def replay(self, source, destination, undo):
        if not source or self.busy or self.blocked or not self.window.host.is_engine_running():
            return

        self.mergeUntil = 0
        entry = source[-1]
        error = self.run(entry[1 if undo else 2])
        if error:
            CustomMessageBox(self.window, QMessageBox.Warning, self.window.tr("Edit history"),
                             self.window.tr("Could not undo or redo this edit"),
                             error, QMessageBox.Ok, QMessageBox.Ok)
            return
        destination.append(source.pop())
        self.updateActions()
