/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_WINDOW_HPP_INCLUDED
#define DGL_WINDOW_HPP_INCLUDED

#include "Geometry.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class Application;
class Widget;
class StandaloneWindow;

class Window
{
public:
   /**
      File browser options.
    */
    struct FileBrowserOptions {
        const char* startDir;
        const char* title;
        uint width;
        uint height;

      /**
         File browser buttons.

         0 means hidden.
         1 means visible and unchecked.
         2 means visible and checked.
        */
        struct Buttons {
            uint listAllFiles;
            uint showHidden;
            uint showPlaces;

            /** Constuctor for default values */
            Buttons()
                : listAllFiles(2),
                  showHidden(1),
                  showPlaces(1) {}
        } buttons;

        /** Constuctor for default values */
        FileBrowserOptions()
            : startDir(nullptr),
              title(nullptr),
              width(0),
              height(0),
              buttons() {}
    };

    explicit Window(Application& app);
    explicit Window(Application& app, Window& parent);
    explicit Window(Application& app, intptr_t parentId);
    virtual ~Window();

    void show();
    void hide();
    void close();
    void exec(bool lockWait = false);

    void focus();
    void repaint() noexcept;

    bool openFileBrowser(const FileBrowserOptions& options);

    bool isVisible() const noexcept;
    void setVisible(bool yesNo);

    bool isResizable() const noexcept;
    void setResizable(bool yesNo);

    uint getWidth() const noexcept;
    uint getHeight() const noexcept;
    Size<uint> getSize() const noexcept;
    void setSize(uint width, uint height);
    void setSize(Size<uint> size);

    const char* getTitle() const noexcept;
    void setTitle(const char* title);

    void setTransientWinId(uintptr_t winId);

    Application& getApp() const noexcept;
    intptr_t getWindowId() const noexcept;

    void addIdleCallback(IdleCallback* const callback);
    void removeIdleCallback(IdleCallback* const callback);

protected:
    virtual void onDisplayBefore();
    virtual void onDisplayAfter();
    virtual void onReshape(uint width, uint height);
    virtual void onClose();

    virtual void fileBrowserSelected(const char* filename);

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Application;
    friend class Widget;
    friend class StandaloneWindow;

    virtual void _addWidget(Widget* const widget);
    virtual void _removeWidget(Widget* const widget);
    void _idle();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Window)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WINDOW_HPP_INCLUDED
