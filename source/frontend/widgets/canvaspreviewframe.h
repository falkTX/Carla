/*
 * Custom Mini Canvas Preview, a custom Qt widget
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CANVASPREVIEWFRAME_H_INCLUDED
#define CANVASPREVIEWFRAME_H_INCLUDED

#include <QtWidgets/QFrame>

class CanvasPreviewFrame : public QFrame
{
public:
    CanvasPreviewFrame(QWidget* const parent)
        : QFrame(parent) {}
};

#endif // CANVASPREVIEWFRAME_H_INCLUDED
