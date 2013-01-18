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

#ifndef PIXMAPKEYBOARD_HPP
#define PIXMAPKEYBOARD_HPP

#include <QtGui/QPixmap>
#include <QtGui/QWidget>

class PixmapKeyboard : public QWidget
{
    Q_OBJECT

public:
    enum Color {
        COLOR_CLASSIC = 0,
        COLOR_ORANGE  = 1
    };

    enum Orientation {
        HORIZONTAL = 0,
        VERTICAL   = 1
    };

    PixmapKeyboard(QWidget* parent);

    void allNotesOff();
    void sendNoteOn(int note, bool sendSignal=true);
    void sendNoteOff(int note, bool sendSignal=true);

    void setMode(Orientation mode, Color color=COLOR_ORANGE);
    void setOctaves(int octaves);

signals:
    void noteOn(int);
    void noteOff(int);
    void notesOn();
    void notesOff();

protected:
    void handleMousePos(const QPoint&);
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void paintEvent(QPaintEvent*);

private Q_SLOTS:
    void updateOnce();

private:
    QPixmap     m_pixmap;
    Orientation m_pixmap_mode;

    QString m_colorStr;
    QFont   m_font;

    int m_octaves;
    int m_lastMouseNote;
    int p_width, p_height;

    bool m_needsUpdate;
    QList<int> m_enabledKeys;
    QMap<int, QRectF> *m_midi_map;

    bool   _isNoteBlack(int note);
    QRectF _getRectFromMidiNote(int note);
};

#endif // PIXMAPKEYBOARD_HPP
