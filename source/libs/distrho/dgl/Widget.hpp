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

#ifndef __DGL_WIDGET_HPP__
#define __DGL_WIDGET_HPP__

#include "Base.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class Window;

class Widget
{
public:
    Widget(Window* parent);
    virtual ~Widget();

    bool isVisible();
    void setVisible(bool yesNo);

    void show()
    {
        setVisible(true);
    }

    void hide()
    {
        setVisible(false);
    }

protected:
    virtual void onDisplay();
    virtual void onKeyboard(bool press, uint32_t key);
    virtual void onMouse(int button, bool press, int x, int y);
    virtual void onMotion(int x, int y);
    virtual void onScroll(float dx, float dy);
    virtual void onSpecial(bool press, Key key);
    virtual void onReshape(int width, int height);
    virtual void onClose();

private:
    Window* fParent;
    bool fVisible;
    friend class Window;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DGL_WIDGET_HPP__
