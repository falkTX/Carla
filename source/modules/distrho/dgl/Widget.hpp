/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DGL_WIDGET_HPP_INCLUDED
#define DGL_WIDGET_HPP_INCLUDED

#include "Geometry.hpp"

#ifdef PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

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

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WIDGET_HPP_INCLUDED
