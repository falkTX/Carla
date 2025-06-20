/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_STANDALONE_WINDOW_HPP_INCLUDED
#define DGL_STANDALONE_WINDOW_HPP_INCLUDED

#include "TopLevelWidget.hpp"
#include "Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class StandaloneWindow : public Window,
                         public TopLevelWidget
{
public:
   /**
      Constructor without parent.
    */
    StandaloneWindow(Application& app)
      : Window(app),
        TopLevelWidget((Window&)*this),
        sgc((Window&)*this) {}

   /**
      Constructor with a transient parent window, typically used to run as modal.
    */
    StandaloneWindow(Application& app, Window& transientParentWindow)
      : Window(app, transientParentWindow),
        TopLevelWidget((Window&)*this),
        sgc((Window&)*this, transientParentWindow) {}

   /**
      Clear current graphics context.
      Must be called at the end of your StandaloneWindow constructor.
    */
    void done()
    {
        sgc.done();
    }

   /**
      Get a graphics context back again.
      Called when a valid graphics context is needed outside the constructor.
    */
    void reinit()
    {
        sgc.reinit();
    }

   /**
      Overloaded functions to ensure they apply to the Window class.
    */
    bool isVisible() const noexcept { return Window::isVisible(); }
    void setVisible(bool yesNo) { Window::setVisible(yesNo); }
    void hide() { Window::hide(); }
    void show() { Window::show(); }
    uint getWidth() const noexcept { return Window::getWidth(); }
    uint getHeight() const noexcept { return Window::getHeight(); }
    const Size<uint> getSize() const noexcept { return Window::getSize(); }
    void repaint() noexcept { Window::repaint(); }
    void repaint(const Rectangle<uint>& rect) noexcept { Window::repaint(rect); }
    void setWidth(uint width) { Window::setWidth(width); }
    void setHeight(uint height) { Window::setHeight(height); }
    void setSize(uint width, uint height) { Window::setSize(width, height); }
    void setSize(const Size<uint>& size) { Window::setSize(size); }
    bool addIdleCallback(IdleCallback* callback, uint timerFrequencyInMs = 0)
    { return Window::addIdleCallback(callback, timerFrequencyInMs); }
    bool removeIdleCallback(IdleCallback* callback) { return Window::removeIdleCallback(callback); }
    Application& getApp() const noexcept { return Window::getApp(); }
    const GraphicsContext& getGraphicsContext() const noexcept { return Window::getGraphicsContext(); }
    double getScaleFactor() const noexcept { return Window::getScaleFactor(); }
    void setGeometryConstraints(uint minimumWidth, uint minimumHeight,
                                bool keepAspectRatio = false, bool automaticallyScale = false)
    { Window::setGeometryConstraints(minimumWidth, minimumHeight, keepAspectRatio, automaticallyScale); }

private:
    ScopedGraphicsContext sgc;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneWindow)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_STANDALONE_WINDOW_HPP_INCLUDED
