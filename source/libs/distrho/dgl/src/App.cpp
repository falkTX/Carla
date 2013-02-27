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

#include "AppPrivate.hpp"

#include "../Window.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

App::App()
    : kPrivate(new Private)
{
}

App::~App()
{
    delete kPrivate;
}

void App::idle()
{
    for (auto it = kPrivate->fWindows.begin(); it != kPrivate->fWindows.end(); it++)
    {
        Window* window = *it;
        window->idle();
    }
}

void App::exec()
{
    while (kPrivate->fDoLoop)
    {
        idle();
        dgl_msleep(10);
    }
}

// -------------------------------------------------

END_NAMESPACE_DGL
