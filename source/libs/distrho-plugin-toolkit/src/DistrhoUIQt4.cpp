/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoDefines.h"

#ifdef DISTRHO_UI_QT4

#include "DistrhoUIInternal.h"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// QEmbedWidget

QEmbedWidget::QEmbedWidget()
{
}

QEmbedWidget::~QEmbedWidget()
{
}

void QEmbedWidget::embedInto(WId id)
{
#ifdef Q_WS_X11
    QX11EmbedWidget::embedInto(id);
#endif
}

WId QEmbedWidget::containerWinId() const
{
#ifdef Q_WS_X11
    return QX11EmbedWidget::containerWinId();
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
// UI Callbacks

void Qt4UI::d_uiIdle()
{
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_QT4
