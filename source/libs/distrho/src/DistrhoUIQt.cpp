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

#include <QtGui/QResizeEvent>

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Qt4UI

QtUI::QtUI()
    : UI(),
      QWidget(nullptr)
{
}

QtUI::~QtUI()
{
}

void QtUI::setSize(unsigned int width, unsigned int height)
{
    if (d_resizable())
        resize(width, height);
    else
        setFixedSize(width, height);

    d_uiResize(width, height);
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
