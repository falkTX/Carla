/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "DistrhoUIInternal.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// QEmbedWidget

QEmbedWidget::QEmbedWidget(WId id)
#ifdef Q_WS_X11
    : QX11EmbedWidget()
#else
    : QWidget()
#endif
{
#ifdef Q_WS_X11
    embedInto(id);
#else
    create(id, false, false);
#endif
}

QEmbedWidget::~QEmbedWidget()
{
}

WId QEmbedWidget::containerWinId() const
{
#ifdef Q_WS_X11
    return QX11EmbedWidget::containerWinId();
#else
    return winId();
#endif
}

// -------------------------------------------------
// Qt4UI

Qt4UI::Qt4UI()
    : UI(),
      QWidget(nullptr)
{
}

Qt4UI::~Qt4UI()
{
}

// -------------------------------------------------
// Implement resize internally

void Qt4UI::d_uiResize(unsigned int width, unsigned int height)
{
    UI::d_uiResize(width, height);
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
