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

#ifndef __DGL_APP_HPP__
#define __DGL_APP_HPP__

#include "Base.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class App
{
public:
    App();
    ~App();

    void idle();
    void exec();

private:
    class Private;
    Private* const kPrivate;
    friend class Window;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DGL_APP_HPP__
