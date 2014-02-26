#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# QGraphicsEmbedScene class, based on Qt3D C++ code
# Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
# Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

# ------------------------------------------------------------------------------------------------------------
# Imports (Config)

from carla_config import *

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

if config_UseQt5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, qRound, Qt, QPoint, QPointF, QRectF, QSize
    from PyQt5.QtGui import QPainter
    from PyQt5.QtOpenGL import QGLFramebufferObject, QGLFramebufferObjectFormat
    from PyQt5.QtWidgets import QApplication, QEvent, QGraphicsItem, QGraphicsScene
else:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, qRound, Qt, QEvent, QObject, QPoint, QPointF, QRectF, QSize
    from PyQt4.QtOpenGL import QGLFramebufferObject, QGLFramebufferObjectFormat
    from PyQt4.QtGui import QApplication, QPainter, QGraphicsItem, QGraphicsScene

# ------------------------------------------------------------------------------------------------------------

# Returns the next power of two that is greater than or
# equal to @a value.  The @a value must be positive or the
# result is undefined.
#
# This is a convenience function for use with GL texture
# handling code.

def nextPowerOfTwo(value):
    value -= 1
    value |= value >> 1
    value |= value >> 2
    value |= value >> 4
    value |= value >> 8
    value |= value >> 16
    value += 1
    return value

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class QGraphicsEmbedScene(QGraphicsScene):
    def __init__(self, parent):
        QGraphicsScene.__init__(self, parent)

        self.dirty = True
        self.fbo = None

        self.format = QGLFramebufferObjectFormat()
        self.format.setAttachment(QGLFramebufferObject.CombinedDepthStencil)

        self.pressedPos = QPoint()

        self.changed.connect(self.slot_update)
        self.sceneRectChanged.connect(self.slot_update)

    def format(self):
        return self.format

    def setFormat(self, format):
        self.format = format

    def renderToTexture(self, levelOfDetail = 1.0):
        # Determine the fbo size we will need.
        size = (self.sceneRect().size() * levelOfDetail).toSize()
        fboSize = nextPowerOfTwo(size)
        if fboSize.isEmpty():
            fboSize = QSize(16, 16)

        # Create or re-create the fbo.
        if self.fbo is None or self.fbo.size() != fboSize:
            #del self.fbo
            self.fbo = QGLFramebufferObject(fboSize, self.format)
            if not self.fbo.isValid():
                #del self.fbo
                self.fbo = None
                return 0
            self.dirty = True

        # Return the previous texture contents if the scene hasn't changed.
        if self.fbo is not None and not self.dirty:
            return self.fbo.texture()

        # Render the scene into the fbo, scaling the QPainter's view
        # transform up to the power-of-two fbo size.
        painter = QPainter(self.fbo)
        painter.setWindow(0, 0, size.width(), size.height())
        painter.setViewport(0, 0, fboSize.width(), fboSize.height())
        self.render(painter)
        painter.end()
        self.dirty = False
        return self.fbo.texture()

    def deliverEvent(self, event, texCoord):
        # Map the texture co-ordinate into "screen" co-ordinates.
        # Mouse move and release events can extend beyond the boundaries
        # of the scene, for "click and drag off-screen" operations.
        # Mouse press and double-click events need to be constrained.
        bounds = self.sceneRect()
        screenX = qRound(texCoord.x() * bounds.width())
        screenY = qRound((1.0 - texCoord.y()) * bounds.height())
        if event.type() in (QEvent.GraphicsSceneMousePress,
                            QEvent.GraphicsSceneMouseDoubleClick,
                            QEvent.MouseButtonPress,
                            QEvent.MouseButtonDblClick):
            if screenX < 0:
                screenX = 0
            elif screenX >= bounds.width():
                screenX = qRound(bounds.width() - 1)
            if screenY < 0:
                screenY = 0
            elif screenY >= bounds.height():
                screenY = qRound(bounds.height() - 1)
            self.pressedPos = QPoint(screenX, screenY)

        # Convert the event and deliver it to the scene.
        eventType = event.type()

        if eventType in (QEvent.GraphicsSceneMouseMove,
                         QEvent.GraphicsSceneMousePress,
                         QEvent.GraphicsSceneMouseRelease,
                         QEvent.GraphicsSceneMouseDoubleClick):
            pass
            #QGraphicsSceneMouseEvent *ev =
                #static_cast<QGraphicsSceneMouseEvent *>(event)
            #QGraphicsSceneMouseEvent e(ev->type())
            #e.setPos(QPointF(screenX, screenY))
            #e.setScenePos(QPointF(screenX + bounds.x(), screenY + bounds.y()))
            #e.setScreenPos(QPoint(screenX, screenY))
            #e.setButtonDownScreenPos(ev->button(), d->pressedPos)
            #e.setButtonDownScenePos
                #(ev->button(), QPointF(d->pressedPos.x() + bounds.x(),
                                      #d->pressedPos.y() + bounds.y()))
            #e.setButtons(ev->buttons())
            #e.setButton(ev->button())
            #e.setModifiers(ev->modifiers())
            #e.setAccepted(false)
            #QApplication::sendEvent(this, &e)

        elif eventType == QEvent.GraphicsSceneWheel:
            pass
            #QGraphicsSceneWheelEvent *ev =
                #static_cast<QGraphicsSceneWheelEvent *>(event)
            #QGraphicsSceneWheelEvent e(QEvent::GraphicsSceneWheel)
            #e.setPos(QPointF(screenX, screenY))
            #e.setScenePos(QPointF(screenX + bounds.x(), screenY + bounds.y()))
            #e.setScreenPos(QPoint(screenX, screenY))
            #e.setButtons(ev->buttons())
            #e.setModifiers(ev->modifiers())
            #e.setDelta(ev->delta())
            #e.setOrientation(ev->orientation())
            #e.setAccepted(false)
            #QApplication::sendEvent(this, &e)

        elif eventType in (QEvent.MouseButtonPress,
                           QEvent.MouseButtonRelease,
                           QEvent.MouseButtonDblClick,
                           QEvent.MouseMove):
            pass
            #QMouseEvent *ev = static_cast<QMouseEvent *>(event)
            #QEvent::Type type
            #if (ev->type() == QEvent::MouseButtonPress)
                #type = QEvent::GraphicsSceneMousePress
            #else if (ev->type() == QEvent::MouseButtonRelease)
                #type = QEvent::GraphicsSceneMouseRelease
            #else if (ev->type() == QEvent::MouseButtonDblClick)
                #type = QEvent::GraphicsSceneMouseDoubleClick
            #else
                #type = QEvent::GraphicsSceneMouseMove
            #QGraphicsSceneMouseEvent e(type)
            #e.setPos(QPointF(screenX, screenY))
            #e.setScenePos(QPointF(screenX + bounds.x(), screenY + bounds.y()))
            #e.setScreenPos(QPoint(screenX, screenY))
            #e.setButtonDownScreenPos(ev->button(), d->pressedPos)
            #e.setButtonDownScenePos
                #(ev->button(), QPointF(d->pressedPos.x() + bounds.x(),
                                      #d->pressedPos.y() + bounds.y()))
            #e.setButtons(ev->buttons())
            #e.setButton(ev->button())
            #e.setModifiers(ev->modifiers())
            #e.setAccepted(false)
            #QApplication::sendEvent(this, &e)

        elif eventType == QEvent.Wheel:
            pass
            #QWheelEvent *ev = static_cast<QWheelEvent *>(event)
            #QGraphicsSceneWheelEvent e(QEvent::GraphicsSceneWheel)
            #e.setPos(QPointF(screenX, screenY))
            #e.setScenePos(QPointF(screenX + bounds.x(), screenY + bounds.y()))
            #e.setScreenPos(QPoint(screenX, screenY))
            #e.setButtons(ev->buttons())
            #e.setModifiers(ev->modifiers())
            #e.setDelta(ev->delta())
            #e.setOrientation(ev->orientation())
            #e.setAccepted(false)
            #QApplication::sendEvent(this, &e)

        #else:
            # Send the event directly without any conversion.
            # Typically used for keyboard, focus, and enter/leave events.
            #QApplication.sendEvent(self, event)

        QApplication.sendEvent(self, event)

    def drawBackground(self, painter, rect):
        if self.backgroundBrush().style() == Qt.NoBrush:
            # Fill the fbo with the transparent color as there won't
            # be a window or graphics item drawing a previous background.
            painter.save()
            painter.setCompositionMode(QPainter.CompositionMode_Source)
            painter.fillRect(rect, Qt.transparent)
            painter.restore()
        else:
            QGraphicsScene.drawBackground(painter, rect)

    @pyqtSlot()
    def slot_update(self):
        self.dirty = True

# ------------------------------------------------------------------------------------------------------------
# PatchScene compatible class

# object types
CanvasBoxType           = QGraphicsItem.UserType + 1
CanvasIconType          = QGraphicsItem.UserType + 2
CanvasPortType          = QGraphicsItem.UserType + 3
CanvasLineType          = QGraphicsItem.UserType + 4
CanvasBezierLineType    = QGraphicsItem.UserType + 5
CanvasLineMovType       = QGraphicsItem.UserType + 6
CanvasBezierLineMovType = QGraphicsItem.UserType + 7

class PatchScene3D(QGraphicsEmbedScene):
    scaleChanged    = pyqtSignal(float)
    sceneGroupMoved = pyqtSignal(int, int, QPointF)
    pluginSelected  = pyqtSignal(list)

    def __init__(self, parent, view):
        QGraphicsEmbedScene.__init__(self, parent)

        self.m_ctrl_down = False
        self.m_mouse_down_init = False
        self.m_mouse_rubberband = False

        self.addRubberBand()

        self.m_view = view
        #if not self.m_view:
            #qFatal("PatchCanvas::PatchScene() - invalid view")

        self.selectionChanged.connect(self.slot_selectionChanged)

    def addRubberBand(self):
        self.m_rubberband = self.addRect(QRectF(0, 0, 0, 0))
        self.m_rubberband.setZValue(-1)
        self.m_rubberband.hide()
        self.m_rubberband_selection = False
        self.m_rubberband_orig_point = QPointF(0, 0)

    def clear(self):
        QGraphicsEmbedScene.clear(self)

        # Re-add rubberband, that just got deleted
        self.addRubberBand()

    def fixScaleFactor(self):
        pass

    def updateTheme(self):
        pass

    def zoom_fit(self):
        pass

    def zoom_in(self):
        pass

    def zoom_out(self):
        pass

    def zoom_reset(self):
        pass

    @pyqtSlot()
    def slot_selectionChanged(self):
        items_list = self.selectedItems()

        if len(items_list) == 0:
            self.pluginSelected.emit([])
            return

        plugin_list = []

        for item in items_list:
            if item and item.isVisible():
                group_item = None

                if item.type() == CanvasBoxType:
                    group_item = item
                elif item.type() == CanvasPortType:
                    group_item = item.parentItem()
                #elif item.type() in (CanvasLineType, CanvasBezierLineType, CanvasLineMovType, CanvasBezierLineMovType):
                    #plugin_list = []
                    #break

                if group_item is not None and group_item.m_plugin_id >= 0:
                    plugin_list.append(group_item.m_plugin_id)

        self.pluginSelected.emit(plugin_list)
