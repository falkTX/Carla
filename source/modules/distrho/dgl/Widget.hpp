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

#include "Geometry.hpp"

#ifdef PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

START_NAMESPACE_DGL

// -------------------------------------------------

class App;
class Window;

class Widget
{
public:
    Widget(Window* parent);
    virtual ~Widget();

    bool isVisible() const;
    void setVisible(bool yesNo);

    void show()
    {
        setVisible(true);
    }

    void hide()
    {
        setVisible(false);
    }

    int getX() const;
    int getY() const;
    const Point<int>& getPos() const;

    void setX(int x);
    void setY(int y);
    void setPos(int x, int y);
    void setPos(const Point<int>& pos);

    void move(int x, int y);
    void move(const Point<int>& pos);

    int getWidth() const;
    int getHeight() const;
    const Size<int>& getSize() const;

    void setWidth(int width);
    void setHeight(int height);
    void setSize(int width, int height);
    void setSize(const Size<int>& size);

    const Rectangle<int>& getArea() const;

    int getModifiers();

    App* getApp() const;
    Window* getParent() const;
    void repaint();

protected:
    virtual void onDisplay();
    virtual bool onKeyboard(bool press, uint32_t key);
    virtual bool onMouse(int button, bool press, int x, int y);
    virtual bool onMotion(int x, int y);
    virtual bool onScroll(float dx, float dy);
    virtual bool onSpecial(bool press, Key key);
    virtual void onReshape(int width, int height);
    virtual void onClose();

private:
    Window* fParent;
    bool    fVisible;
    Rectangle<int> fArea;

    friend class Window;
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_WIDGET_HPP__
