/*
 * Pixmap Keyboard, a custom Qt4 widget
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "pixmapkeyboard.hpp"

#include <QtCore/QMap>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

QMap<int, QRectF> midi_key2rect_map_horizontal;
QMap<int, QRectF> midi_key2rect_map_vertical;
QMap<int, int> midi_keyboard2key_map;
QVector<int> blackNotes;

static bool pixmapkeyboard_initiated = false;
void pixmapkeyboard_init()
{
    if (pixmapkeyboard_initiated)
        return;

    pixmapkeyboard_initiated = true;

    // midi_key2rect_map_horizontal ------
    midi_key2rect_map_horizontal[0] =  QRectF(0,   0, 18, 64); // C
    midi_key2rect_map_horizontal[1] =  QRectF(13,  0, 11, 42); // C#
    midi_key2rect_map_horizontal[2] =  QRectF(18,  0, 25, 64); // D
    midi_key2rect_map_horizontal[3] =  QRectF(37,  0, 11, 42); // D#
    midi_key2rect_map_horizontal[4] =  QRectF(42,  0, 18, 64); // E
    midi_key2rect_map_horizontal[5] =  QRectF(60,  0, 18, 64); // F
    midi_key2rect_map_horizontal[6] =  QRectF(73,  0, 11, 42); // F#
    midi_key2rect_map_horizontal[7] =  QRectF(78,  0, 25, 64); // G
    midi_key2rect_map_horizontal[8] =  QRectF(97,  0, 11, 42); // G#
    midi_key2rect_map_horizontal[9] =  QRectF(102, 0, 25, 64); // A
    midi_key2rect_map_horizontal[10] = QRectF(121, 0, 11, 42); // A#
    midi_key2rect_map_horizontal[11] = QRectF(126, 0, 18, 64); // B

    // midi_key2rect_map_vertical --------
    midi_key2rect_map_vertical[11] = QRectF(0,  0,  64, 18); // B
    midi_key2rect_map_vertical[10] = QRectF(0, 14,  42,  7); // A#
    midi_key2rect_map_vertical[9] =  QRectF(0, 18,  64, 24); // A
    midi_key2rect_map_vertical[8] =  QRectF(0, 38,  42,  7); // G#
    midi_key2rect_map_vertical[7] =  QRectF(0, 42,  64, 24); // G
    midi_key2rect_map_vertical[6] =  QRectF(0, 62,  42,  7); // F#
    midi_key2rect_map_vertical[5] =  QRectF(0, 66,  64, 18); // F
    midi_key2rect_map_vertical[4] =  QRectF(0, 84,  64, 18); // E
    midi_key2rect_map_vertical[3] =  QRectF(0, 98,  42,  7); // D#
    midi_key2rect_map_vertical[2] =  QRectF(0, 102, 64, 24); // D
    midi_key2rect_map_vertical[1] =  QRectF(0, 122, 42,  7); // C#
    midi_key2rect_map_vertical[0] =  QRectF(0, 126, 64, 18); // C

    // midi_keyboard2key_map -------------
    // 3th octave
    midi_keyboard2key_map[Qt::Key_Z] = 48;
    midi_keyboard2key_map[Qt::Key_S] = 49;
    midi_keyboard2key_map[Qt::Key_X] = 50;
    midi_keyboard2key_map[Qt::Key_D] = 51;
    midi_keyboard2key_map[Qt::Key_C] = 52;
    midi_keyboard2key_map[Qt::Key_V] = 53;
    midi_keyboard2key_map[Qt::Key_G] = 54;
    midi_keyboard2key_map[Qt::Key_B] = 55;
    midi_keyboard2key_map[Qt::Key_H] = 56;
    midi_keyboard2key_map[Qt::Key_N] = 57;
    midi_keyboard2key_map[Qt::Key_J] = 58;
    midi_keyboard2key_map[Qt::Key_M] = 59;
    // 4th octave
    midi_keyboard2key_map[Qt::Key_Q] = 60;
    midi_keyboard2key_map[Qt::Key_2] = 61;
    midi_keyboard2key_map[Qt::Key_W] = 62;
    midi_keyboard2key_map[Qt::Key_3] = 63;
    midi_keyboard2key_map[Qt::Key_E] = 64;
    midi_keyboard2key_map[Qt::Key_R] = 65;
    midi_keyboard2key_map[Qt::Key_5] = 66;
    midi_keyboard2key_map[Qt::Key_T] = 67;
    midi_keyboard2key_map[Qt::Key_6] = 68;
    midi_keyboard2key_map[Qt::Key_Y] = 69;
    midi_keyboard2key_map[Qt::Key_7] = 70;
    midi_keyboard2key_map[Qt::Key_U] = 71;

    blackNotes << 1;
    blackNotes << 3;
    blackNotes << 6;
    blackNotes << 8;
    blackNotes << 10;
}

PixmapKeyboard::PixmapKeyboard(QWidget* parent)
    : QWidget(parent),
      m_font("Monospace", 8, QFont::Normal)
{
    pixmapkeyboard_init();

    m_octaves = 6;
    m_lastMouseNote = -1;
    m_needsUpdate = false;

    setCursor(Qt::PointingHandCursor);
    setMode(HORIZONTAL);
}

void PixmapKeyboard::allNotesOff()
{
    m_enabledKeys.clear();

    m_needsUpdate = true;
    QTimer::singleShot(0, this, SLOT(updateOnce()));

    emit notesOff();
}

void PixmapKeyboard::sendNoteOn(int note, bool sendSignal)
{
    if (0 <= note && note <= 127 && ! m_enabledKeys.contains(note))
    {
        m_enabledKeys.append(note);
        if (sendSignal)
            emit noteOn(note);

        m_needsUpdate = true;
        QTimer::singleShot(0, this, SLOT(updateOnce()));
    }

    if (m_enabledKeys.count() == 1)
        emit notesOn();
}

void PixmapKeyboard::sendNoteOff(int note, bool sendSignal)
{
    if (note >= 0 && note <= 127 && m_enabledKeys.contains(note))
    {
        m_enabledKeys.removeOne(note);
        if (sendSignal)
            emit noteOff(note);

        m_needsUpdate = true;
        QTimer::singleShot(0, this, SLOT(updateOnce()));
    }

    if (m_enabledKeys.count() == 0)
        emit notesOff();
}

void PixmapKeyboard::setMode(Orientation mode, Color color)
{
    if (color == COLOR_CLASSIC)
    {
        m_colorStr = "classic";
    }
    else if (color == COLOR_ORANGE)
    {
        m_colorStr = "orange";
    }
    else
    {
        qCritical("PixmapKeyboard::setMode(%i, %i) - invalid color", mode, color);
        return setMode(mode);
    }

    if (mode == HORIZONTAL)
    {
        m_midi_map = &midi_key2rect_map_horizontal;
        m_pixmap.load(QString(":/bitmaps/kbd_h_%1.png").arg(m_colorStr));
        m_pixmap_mode = HORIZONTAL;
        p_width  = m_pixmap.width();
        p_height = m_pixmap.height() / 2;
    }
    else if (mode == VERTICAL)
    {
        m_midi_map = &midi_key2rect_map_vertical;
        m_pixmap.load(QString(":/bitmaps/kbd_v_%1.png").arg(m_colorStr));
        m_pixmap_mode = VERTICAL;
        p_width  = m_pixmap.width() / 2;
        p_height = m_pixmap.height();
    }
    else
    {
        qCritical("PixmapKeyboard::setMode(%i, %i) - invalid mode", mode, color);
        return setMode(HORIZONTAL);
    }

    setOctaves(m_octaves);
}

void PixmapKeyboard::setOctaves(int octaves)
{
    Q_ASSERT(octaves >= 1 && octaves <= 6);

    if (octaves < 1)
        octaves = 1;
    else if (octaves > 6)
        octaves = 6;
    m_octaves = octaves;

    if (m_pixmap_mode == HORIZONTAL)
    {
        setMinimumSize(p_width * m_octaves, p_height);
        setMaximumSize(p_width * m_octaves, p_height);
    }
    else if (m_pixmap_mode == VERTICAL)
    {
        setMinimumSize(p_width, p_height * m_octaves);
        setMaximumSize(p_width, p_height * m_octaves);
    }

    update();
}

void PixmapKeyboard::handleMousePos(const QPoint& pos)
{
    int note, octave;
    QPointF n_pos;

    if (m_pixmap_mode == HORIZONTAL)
    {
        if (pos.x() < 0 or pos.x() > m_octaves * 144)
            return;
        int posX = pos.x() - 1;
        octave = posX / p_width;
        n_pos  = QPointF(posX % p_width, pos.y());
    }
    else if (m_pixmap_mode == VERTICAL)
    {
        if (pos.y() < 0 or pos.y() > m_octaves * 144)
            return;
        int posY = pos.y() - 1;
        octave = m_octaves - posY / p_height;
        n_pos  = QPointF(pos.x(), posY % p_height);
    }
    else
        return;

    octave += 3;

    if ((*m_midi_map)[1].contains(n_pos))      // C#
        note = 1;
    else if ((*m_midi_map)[3].contains(n_pos)) // D#
        note = 3;
    else if ((*m_midi_map)[6].contains(n_pos)) // F#
        note = 6;
    else if ((*m_midi_map)[8].contains(n_pos)) // G#
        note = 8;
    else if ((*m_midi_map)[10].contains(n_pos))// A#
        note = 10;
    else if ((*m_midi_map)[0].contains(n_pos)) // C
        note = 0;
    else if ((*m_midi_map)[2].contains(n_pos)) // D
        note = 2;
    else if ((*m_midi_map)[4].contains(n_pos)) // E
        note = 4;
    else if ((*m_midi_map)[5].contains(n_pos)) // F
        note = 5;
    else if ((*m_midi_map)[7].contains(n_pos)) // G
        note = 7;
    else if ((*m_midi_map)[9].contains(n_pos)) // A
        note = 9;
    else if ((*m_midi_map)[11].contains(n_pos))// B
        note = 11;
    else
        note = -1;

    if (note != -1)
    {
        note += octave * 12;
        if (m_lastMouseNote != note)
        {
            sendNoteOff(m_lastMouseNote);
            sendNoteOn(note);
        }
    }
    else
        sendNoteOff(m_lastMouseNote);

    m_lastMouseNote = note;
}

void PixmapKeyboard::keyPressEvent(QKeyEvent* event)
{
    if (! event->isAutoRepeat())
    {
        int qKey = event->key();
        if (midi_keyboard2key_map.keys().contains(qKey))
            sendNoteOn(midi_keyboard2key_map[qKey]);
    }
    QWidget::keyPressEvent(event);
}

void PixmapKeyboard::keyReleaseEvent(QKeyEvent* event)
{
    if (! event->isAutoRepeat())
    {
        int qKey = event->key();
        if (midi_keyboard2key_map.keys().contains(qKey))
            sendNoteOff(midi_keyboard2key_map[qKey]);
    }
    QWidget::keyReleaseEvent(event);
}

void PixmapKeyboard::mousePressEvent(QMouseEvent* event)
{
    m_lastMouseNote = -1;
    handleMousePos(event->pos());
    setFocus();
    QWidget::mousePressEvent(event);
}

void PixmapKeyboard::mouseMoveEvent(QMouseEvent* event)
{
    handleMousePos(event->pos());
    QWidget::mousePressEvent(event);
}

void PixmapKeyboard::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_lastMouseNote != -1)
    {
        sendNoteOff(m_lastMouseNote);
        m_lastMouseNote = -1;
    }
    QWidget::mouseReleaseEvent(event);
}

void PixmapKeyboard::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    // -------------------------------------------------------------
    // Paint clean keys (as background)

    for (int octave=0; octave < m_octaves; octave++)
    {
        QRectF target;

        if (m_pixmap_mode == HORIZONTAL)
            target = QRectF(p_width * octave, 0, p_width, p_height);
        else if (m_pixmap_mode == VERTICAL)
            target = QRectF(0, p_height * octave, p_width, p_height);
        else
            return;

        QRectF source = QRectF(0, 0, p_width, p_height);
        painter.drawPixmap(target, m_pixmap, source);
    }

    // -------------------------------------------------------------
    // Paint (white) pressed keys

    bool paintedWhite = false;

    for (int i=0; i < m_enabledKeys.count(); i++)
    {
        int octave, note = m_enabledKeys[i];
        QRectF pos = _getRectFromMidiNote(note);

        if (_isNoteBlack(note))
            continue;

        if (note < 36)
            // cannot paint this note
            continue;
        else if (note < 48)
            octave = 0;
        else if (note < 60)
            octave = 1;
        else if (note < 72)
            octave = 2;
        else if (note < 84)
            octave = 3;
        else if (note < 96)
            octave = 4;
        else if (note < 108)
            octave = 5;
        else
            // cannot paint this note either
            continue;

        if (m_pixmap_mode == VERTICAL)
            octave = m_octaves - octave - 1;

        QRectF target, source;

        if (m_pixmap_mode == HORIZONTAL)
        {
            target = QRectF(pos.x() + (p_width * octave), 0, pos.width(), pos.height());
            source = QRectF(pos.x(), p_height, pos.width(), pos.height());
        }
        else if (m_pixmap_mode == VERTICAL)
        {
            target = QRectF(pos.x(), pos.y() + (p_height * octave), pos.width(), pos.height());
            source = QRectF(p_width, pos.y(), pos.width(), pos.height());
        }
        else
            return;

        paintedWhite = true;
        painter.drawPixmap(target, m_pixmap, source);
    }

    // -------------------------------------------------------------
    // Clear white keys border

    if (paintedWhite)
    {
        for (int octave=0; octave < m_octaves; octave++)
        {
            foreach (int note, blackNotes)
            {
                QRectF target, source;
                QRectF pos = _getRectFromMidiNote(note);
                if (m_pixmap_mode == HORIZONTAL)
                {
                    target = QRectF(pos.x() + (p_width * octave), 0, pos.width(), pos.height());
                    source = QRectF(pos.x(), 0, pos.width(), pos.height());
                }
                else if (m_pixmap_mode == VERTICAL)
                {
                    target = QRectF(pos.x(), pos.y() + (p_height * octave), pos.width(), pos.height());
                    source = QRectF(0, pos.y(), pos.width(), pos.height());
                }
                else
                    return;

                painter.drawPixmap(target, m_pixmap, source);
            }
        }
    }

    // -------------------------------------------------------------
    // Paint (black) pressed keys

    for (int i=0; i < m_enabledKeys.count(); i++)
    {
        int octave, note = m_enabledKeys[i];
        QRectF pos = _getRectFromMidiNote(note);

        if (! _isNoteBlack(note))
            continue;

        if (note < 36)
            // cannot paint this note
            continue;
        else if (note < 48)
            octave = 0;
        else if (note < 60)
            octave = 1;
        else if (note < 72)
            octave = 2;
        else if (note < 84)
            octave = 3;
        else if (note < 96)
            octave = 4;
        else if (note < 108)
            octave = 5;
        else
            // cannot paint this note either
            continue;

        if (m_pixmap_mode == VERTICAL)
            octave = m_octaves - octave - 1;

        QRectF target, source;

        if (m_pixmap_mode == HORIZONTAL)
        {
            target = QRectF(pos.x() + (p_width * octave), 0, pos.width(), pos.height());
            source = QRectF(pos.x(), p_height, pos.width(), pos.height());
        }
        else if (m_pixmap_mode == VERTICAL)
        {
            target = QRectF(pos.x(), pos.y() + (p_height * octave), pos.width(), pos.height());
            source = QRectF(p_width, pos.y(), pos.width(), pos.height());
        }
        else
            return;

        painter.drawPixmap(target, m_pixmap, source);
    }

    // Paint C-number note info
    painter.setFont(m_font);
    painter.setPen(Qt::black);

    for (int i=0; i < m_octaves; i++)
    {
        if (m_pixmap_mode == HORIZONTAL)
            painter.drawText(i * 144, 48, 18, 18, Qt::AlignCenter, QString("C%1").arg(i + 2));
        else if (m_pixmap_mode == VERTICAL)
            painter.drawText(45, (m_octaves * 144) - (i * 144) - 16, 18, 18, Qt::AlignCenter, QString("C%1").arg(i + 2));
    }
}

void PixmapKeyboard::updateOnce()
{
    if (m_needsUpdate)
    {
        update();
        m_needsUpdate = false;
    }
}

bool PixmapKeyboard::_isNoteBlack(int note)
{
    int baseNote = note % 12;
    return blackNotes.contains(baseNote);
}

QRectF PixmapKeyboard::_getRectFromMidiNote(int note)
{
    return (*m_midi_map)[note % 12];
}
