#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

from qt_config import qt as qt_config

if qt_config == 5:
    from PyQt5.QtCore import Qt

    Qt.CheckState = int
    Qt.MiddleButton = Qt.MidButton

elif qt_config == 6:
    from PyQt6.QtCore import Qt, QEvent, QEventLoop
    from PyQt6.QtGui import QFont, QPainter, QPalette
    from PyQt6.QtWidgets import (
        QAbstractItemView,
        QAbstractSpinBox,
        QApplication,
        QColorDialog,
        QDialog,
        QDialogButtonBox,
        QFileDialog,
        QFrame,
        QGraphicsItem,
        QGraphicsScene,
        QGraphicsView,
        QHeaderView,
        QLineEdit,
        QListWidgetItem,
        QMenu,
        QMessageBox,
        QSizePolicy,
        QStyle,
    )

    Qt.AlignCenter = Qt.AlignmentFlag.AlignCenter
    Qt.AlignLeft = Qt.AlignmentFlag.AlignLeft

    Qt.AA_DontShowIconsInMenus = Qt.ApplicationAttribute.AA_DontShowIconsInMenus

    Qt.DownArrow = Qt.ArrowType.DownArrow
    Qt.RightArrow = Qt.ArrowType.RightArrow

    Qt.IgnoreAspectRatio = Qt.AspectRatioMode.IgnoreAspectRatio
    Qt.KeepAspectRatio = Qt.AspectRatioMode.KeepAspectRatio

    Qt.NoBrush = Qt.BrushStyle.NoBrush

    Qt.ToolButtonTextBesideIcon = Qt.ToolButtonStyle.ToolButtonTextBesideIcon

    Qt.Checked = Qt.CheckState.Checked
    Qt.Unchecked = Qt.CheckState.Unchecked

    Qt.CustomContextMenu = Qt.ContextMenuPolicy.CustomContextMenu
    Qt.NoContextMenu = Qt.ContextMenuPolicy.NoContextMenu

    Qt.ArrowCursor = Qt.CursorShape.ArrowCursor
    Qt.ClosedHandCursor = Qt.CursorShape.ClosedHandCursor
    Qt.CrossCursor = Qt.CursorShape.CrossCursor
    Qt.OpenHandCursor = Qt.CursorShape.OpenHandCursor
    Qt.PointingHandCursor = Qt.CursorShape.PointingHandCursor
    Qt.SizeAllCursor = Qt.CursorShape.SizeAllCursor
    Qt.SizeHorCursor = Qt.CursorShape.SizeHorCursor

    Qt.black = Qt.GlobalColor.black
    Qt.blue = Qt.GlobalColor.blue
    Qt.cyan = Qt.GlobalColor.cyan
    Qt.darkGray = Qt.GlobalColor.darkGray
    Qt.red = Qt.GlobalColor.red
    Qt.transparent = Qt.GlobalColor.transparent
    Qt.white = Qt.GlobalColor.white
    Qt.yellow = Qt.GlobalColor.yellow

    Qt.Key_0 = Qt.Key.Key_0
    Qt.Key_1 = Qt.Key.Key_1
    Qt.Key_2 = Qt.Key.Key_2
    Qt.Key_3 = Qt.Key.Key_3
    Qt.Key_5 = Qt.Key.Key_5
    Qt.Key_6 = Qt.Key.Key_6
    Qt.Key_7 = Qt.Key.Key_7
    Qt.Key_9 = Qt.Key.Key_9
    Qt.Key_A = Qt.Key.Key_A
    Qt.Key_Agrave = Qt.Key.Key_Agrave
    Qt.Key_B = Qt.Key.Key_B
    Qt.Key_C = Qt.Key.Key_C
    Qt.Key_Ccedilla = Qt.Key.Key_Ccedilla
    Qt.Key_Comma = Qt.Key.Key_Comma
    Qt.Key_D = Qt.Key.Key_D
    Qt.Key_Delete = Qt.Key.Key_Delete
    Qt.Key_E = Qt.Key.Key_E
    Qt.Key_Eacute = Qt.Key.Key_Eacute
    Qt.Key_Egrave = Qt.Key.Key_Egrave
    Qt.Key_End = Qt.Key.Key_End
    Qt.Key_Enter = Qt.Key.Key_Enter
    Qt.Key_Escape = Qt.Key.Key_Escape
    Qt.Key_F = Qt.Key.Key_F
    Qt.Key_G = Qt.Key.Key_G
    Qt.Key_H = Qt.Key.Key_H
    Qt.Key_Home = Qt.Key.Key_Home
    Qt.Key_I = Qt.Key.Key_I
    Qt.Key_J = Qt.Key.Key_J
    Qt.Key_M = Qt.Key.Key_M
    Qt.Key_Minus = Qt.Key.Key_Minus
    Qt.Key_N = Qt.Key.Key_N
    Qt.Key_O = Qt.Key.Key_O
    Qt.Key_P = Qt.Key.Key_P
    Qt.Key_PageDown = Qt.Key.Key_PageDown
    Qt.Key_PageUp = Qt.Key.Key_PageUp
    Qt.Key_ParenLeft = Qt.Key.Key_ParenLeft
    Qt.Key_Plus = Qt.Key.Key_Plus
    Qt.Key_Q = Qt.Key.Key_Q
    Qt.Key_QuoteDbl = Qt.Key.Key_QuoteDbl
    Qt.Key_R = Qt.Key.Key_R
    Qt.Key_Return = Qt.Key.Key_Return
    Qt.Key_S = Qt.Key.Key_S
    Qt.Key_Space = Qt.Key.Key_Space
    Qt.Key_T = Qt.Key.Key_T
    Qt.Key_U = Qt.Key.Key_U
    Qt.Key_V = Qt.Key.Key_V
    Qt.Key_W = Qt.Key.Key_W
    Qt.Key_X = Qt.Key.Key_X
    Qt.Key_Y = Qt.Key.Key_Y
    Qt.Key_Z = Qt.Key.Key_Z

    Qt.AltModifier = Qt.KeyboardModifier.AltModifier
    Qt.ControlModifier = Qt.KeyboardModifier.ControlModifier
    Qt.MetaModifier = Qt.KeyboardModifier.MetaModifier
    Qt.NoModifier = Qt.KeyboardModifier.NoModifier
    Qt.ShiftModifier = Qt.KeyboardModifier.ShiftModifier

    Qt.UserRole = Qt.ItemDataRole.UserRole

    Qt.ItemIsDragEnabled = Qt.ItemFlag.ItemIsDragEnabled
    Qt.ItemIsEnabled = Qt.ItemFlag.ItemIsEnabled
    Qt.ItemIsSelectable = Qt.ItemFlag.ItemIsSelectable

    Qt.ContainsItemShape = Qt.ItemSelectionMode.ContainsItemShape

    Qt.LeftButton = Qt.MouseButton.LeftButton
    Qt.MiddleButton = Qt.MouseButton.MiddleButton
    Qt.RightButton = Qt.MouseButton.RightButton

    Qt.MouseEventSynthesizedByApplication = Qt.MouseEventSource.MouseEventSynthesizedByApplication

    Qt.Horizontal = Qt.Orientation.Horizontal

    Qt.FlatCap = Qt.PenCapStyle.FlatCap
    Qt.RoundCap = Qt.PenCapStyle.RoundCap

    Qt.MiterJoin = Qt.PenJoinStyle.MiterJoin
    Qt.RoundJoin = Qt.PenJoinStyle.RoundJoin

    Qt.DashLine = Qt.PenStyle.DashLine
    Qt.NoPen = Qt.PenStyle.NoPen
    Qt.SolidLine = Qt.PenStyle.SolidLine

    Qt.ScrollBarAlwaysOn = Qt.ScrollBarPolicy.ScrollBarAlwaysOn
    Qt.ScrollBarAlwaysOff = Qt.ScrollBarPolicy.ScrollBarAlwaysOff

    Qt.AscendingOrder = Qt.SortOrder.AscendingOrder

    Qt.TextSelectableByMouse = Qt.TextInteractionFlag.TextSelectableByMouse

    Qt.ToolButtonIconOnly = Qt.ToolButtonStyle.ToolButtonIconOnly
    Qt.ToolButtonTextBesideIcon = Qt.ToolButtonStyle.ToolButtonTextBesideIcon

    Qt.SmoothTransformation = Qt.TransformationMode.SmoothTransformation

    Qt.WA_OpaquePaintEvent = Qt.WidgetAttribute.WA_OpaquePaintEvent

    Qt.WindowModal = Qt.WindowModality.WindowModal

    Qt.WindowActive = Qt.WindowState.WindowActive
    Qt.WindowMinimized = Qt.WindowState.WindowMinimized

    Qt.MSWindowsFixedSizeDialogHint = Qt.WindowType.MSWindowsFixedSizeDialogHint
    Qt.WindowContextHelpButtonHint = Qt.WindowType.WindowContextHelpButtonHint

    QAbstractItemView.DropOnly = QAbstractItemView.DragDropMode.DropOnly

    QAbstractItemView.SingleSelection = QAbstractItemView.SelectionMode.SingleSelection

    QAbstractSpinBox.NoButtons = QAbstractSpinBox.ButtonSymbols.NoButtons
    QAbstractSpinBox.UpDownArrows = QAbstractSpinBox.ButtonSymbols.UpDownArrows

    QAbstractSpinBox.StepNone = QAbstractSpinBox.StepEnabledFlag.StepNone
    QAbstractSpinBox.StepDownEnabled = QAbstractSpinBox.StepEnabledFlag.StepDownEnabled
    QAbstractSpinBox.StepUpEnabled = QAbstractSpinBox.StepEnabledFlag.StepUpEnabled

    QApplication.exec_ = lambda a: a.exec()

    QColorDialog.DontUseNativeDialog = QColorDialog.ColorDialogOption.DontUseNativeDialog

    QDialog.exec_ = lambda d: d.exec()

    QDialogButtonBox.Ok = QDialogButtonBox.StandardButton.Ok
    QDialogButtonBox.Reset = QDialogButtonBox.StandardButton.Reset

    QEvent.EnabledChange = QEvent.Type.EnabledChange
    QEvent.MouseButtonPress = QEvent.Type.MouseButtonPress
    QEvent.PaletteChange = QEvent.Type.PaletteChange
    QEvent.StyleChange = QEvent.Type.StyleChange
    QEvent.User = QEvent.Type.User

    QEventLoop.ExcludeUserInputEvents = QEventLoop.ProcessEventsFlag.ExcludeUserInputEvents

    QFileDialog.AcceptSave = QFileDialog.AcceptMode.AcceptSave

    QFileDialog.AnyFile = QFileDialog.FileMode.AnyFile

    QFileDialog.DontUseCustomDirectoryIcons = QFileDialog.Option.DontUseCustomDirectoryIcons
    QFileDialog.ShowDirsOnly = QFileDialog.Option.ShowDirsOnly

    QFont.AllUppercase = QFont.Capitalization.AllUppercase

    QFont.Bold = QFont.Weight.Bold
    QFont.Normal = QFont.Weight.Normal

    QFrame.HLine = QFrame.Shape.HLine
    QFrame.StyledPanel = QFrame.Shape.StyledPanel

    QFrame.Raised = QFrame.Shadow.Raised
    QFrame.Sunken = QFrame.Shadow.Sunken

    QGraphicsItem.ItemSelectedHasChanged = QGraphicsItem.GraphicsItemChange.ItemSelectedHasChanged

    QGraphicsItem.ItemIsFocusable = QGraphicsItem.GraphicsItemFlag.ItemIsFocusable
    QGraphicsItem.ItemIsMovable = QGraphicsItem.GraphicsItemFlag.ItemIsMovable
    QGraphicsItem.ItemIsSelectable = QGraphicsItem.GraphicsItemFlag.ItemIsSelectable
    QGraphicsItem.ItemSendsGeometryChanges = QGraphicsItem.GraphicsItemFlag.ItemSendsGeometryChanges

    QGraphicsScene.NoIndex = QGraphicsScene.ItemIndexMethod.NoIndex

    QGraphicsView.NoDrag = QGraphicsView.DragMode.NoDrag
    QGraphicsView.ScrollHandDrag = QGraphicsView.DragMode.ScrollHandDrag

    QGraphicsView.FullViewportUpdate = QGraphicsView.ViewportUpdateMode.FullViewportUpdate
    QGraphicsView.MinimalViewportUpdate = QGraphicsView.ViewportUpdateMode.MinimalViewportUpdate

    QHeaderView.Fixed = QHeaderView.ResizeMode.Fixed
    QHeaderView.ResizeToContents = QHeaderView.ResizeMode.ResizeToContents

    QLineEdit.Normal = QLineEdit.EchoMode.Normal

    QListWidgetItem.UserType = QListWidgetItem.ItemType.UserType

    QMenu.exec_ = lambda m, p: m.exec(p)

    QMessageBox.exec_ = lambda mb: mb.exec()

    QMessageBox.Cancel = QMessageBox.StandardButton.Cancel
    QMessageBox.No = QMessageBox.StandardButton.No
    QMessageBox.Ok = QMessageBox.StandardButton.Ok
    QMessageBox.Yes = QMessageBox.StandardButton.Yes

    QMessageBox.Critical = QMessageBox.Icon.Critical
    QMessageBox.Information = QMessageBox.Icon.Information
    QMessageBox.Warning = QMessageBox.Icon.Warning

    QPainter.CompositionMode_Difference = QPainter.CompositionMode.CompositionMode_Difference
    QPainter.CompositionMode_Multiply = QPainter.CompositionMode.CompositionMode_Multiply
    QPainter.CompositionMode_Plus = QPainter.CompositionMode.CompositionMode_Plus
    QPainter.CompositionMode_SourceOver = QPainter.CompositionMode.CompositionMode_SourceOver

    QPainter.HighQualityAntialiasing = None

    QPainter.Antialiasing = QPainter.RenderHint.Antialiasing
    QPainter.SmoothPixmapTransform = QPainter.RenderHint.SmoothPixmapTransform
    QPainter.TextAntialiasing = QPainter.RenderHint.TextAntialiasing

    QPalette.Active = QPalette.ColorGroup.Active
    QPalette.Disabled = QPalette.ColorGroup.Disabled
    QPalette.Inactive = QPalette.ColorGroup.Inactive

    QPalette.AlternateBase = QPalette.ColorRole.AlternateBase
    QPalette.Base = QPalette.ColorRole.Base
    QPalette.BrightText = QPalette.ColorRole.BrightText
    QPalette.Button = QPalette.ColorRole.Button
    QPalette.ButtonText = QPalette.ColorRole.ButtonText
    QPalette.Dark = QPalette.ColorRole.Dark
    QPalette.Highlight = QPalette.ColorRole.Highlight
    QPalette.HighlightedText = QPalette.ColorRole.HighlightedText
    QPalette.Light = QPalette.ColorRole.Light
    QPalette.Link = QPalette.ColorRole.Link
    QPalette.LinkVisited = QPalette.ColorRole.LinkVisited
    QPalette.Mid = QPalette.ColorRole.Mid
    QPalette.Midlight = QPalette.ColorRole.Midlight
    QPalette.Shadow = QPalette.ColorRole.Shadow
    QPalette.Text = QPalette.ColorRole.Text
    QPalette.ToolTipBase = QPalette.ColorRole.ToolTipBase
    QPalette.ToolTipText = QPalette.ColorRole.ToolTipText
    QPalette.Window = QPalette.ColorRole.Window
    QPalette.WindowText = QPalette.ColorRole.WindowText

    # TODO remove this QPalette.Background is deprecated already in Qt5
    QPalette.Background = QPalette.Window

    QSizePolicy.Expanding = QSizePolicy.Policy.Expanding
    QSizePolicy.Preferred = QSizePolicy.Policy.Preferred

    QStyle.State_Selected = QStyle.StateFlag.State_Selected
