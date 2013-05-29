/*
 * Pixmap Keyboard, a custom Qt4 widget
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __PIXMAPKEYBOARD_HPP__
#define __PIXMAPKEYBOARD_HPP__

#include "CarlaDefines.hpp"

#include <map>
#include <QtGui/QPixmap>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QWidget>
#else
# include <QtGui/QWidget>
#endif

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
    QPixmap     fPixmap;
    Orientation fPixmapMode;

    QString fColorStr;
    QFont   fFont;

    int fOctaves;
    int fLastMouseNote;
    int fWidth;
    int fHeight;

    bool fNeedsUpdate;
    QList<int> fEnabledKeys;
    std::map<int, QRectF>& fMidiMap;

    bool _isNoteBlack(int note) const;
    const QRectF& _getRectFromMidiNote(int note) const;
};

#endif // __PIXMAPKEYBOARD_HPP__
