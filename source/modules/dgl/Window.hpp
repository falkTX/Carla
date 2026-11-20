/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
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

#ifdef DGL_USE_FILE_BROWSER
# include "FileBrowserDialog.hpp"
#endif

#ifdef DGL_USE_WEB_VIEW
# include "WebView.hpp"
#endif

#include <vector>

#ifdef DISTRHO_NAMESPACE
START_NAMESPACE_DISTRHO
class PluginWindow;
END_NAMESPACE_DISTRHO
#endif

START_NAMESPACE_DGL

class Application;
class TopLevelWidget;

// -----------------------------------------------------------------------

/**
   DGL Window class.

   This is the where all OS-related events initially happen, before being propagated to any widgets.

   A Window MUST have an Application instance tied to it.
   It is not possible to swap Application instances from within the lifetime of a Window.
   But it is possible to completely change the Widgets that a Window contains during its lifetime.

   Typically the event handling functions as following:
   Application -> Window -> Top-Level-Widget -> SubWidgets

   Please note that, unlike many other graphical toolkits out there,
   DGL makes a clear distinction between a Window and a Widget.
   You cannot directly draw in a Window, you need to create a Widget for that.

   Also, a Window MUST have a single top-level Widget.
   The Window will take care of global screen positioning and resizing, everything else is sent for widgets to handle.

   ...
 */
class DISTRHO_API Window
{
   struct PrivateData;

public:
   /**
      Window graphics context as a scoped struct.
      This class gives graphics context drawing time to a window's widgets.
      Typically used for allowing OpenGL drawing operations during a window + widget constructor.

      Unless you are subclassing the Window or StandaloneWindow classes, you do not need to care.
      In such cases you will need to use this struct as a way to get a valid OpenGL context.
      For example in a standalone application:
      ```
      int main()
      {
          Application app;
          Window win(app);
          ScopedPointer<MyCustomTopLevelWidget> widget;
          {
              const Window::ScopedGraphicsContext sgc(win);
              widget = new MyCustomTopLevelWidget(win);
          }
          app.exec();
          return 0;
      }
      ```

      This struct is necessary because we cannot automatically make the window leave the OpenGL context in custom code.
      And we must always cleanly enter and leave the OpenGL context.
      So in order to avoid messing up the global host context, this class is used around widget creation.
    */
    struct ScopedGraphicsContext
    {
        /** Constructor that will make the @a window graphics context the current one */
        explicit ScopedGraphicsContext(Window& window);

        /** Overloaded constructor, gives back context to its transient parent when done */
        explicit ScopedGraphicsContext(Window& window, Window& transientParentWindow);

        /** Desstructor for clearing current context, if not done yet */
        ~ScopedGraphicsContext();

        /** Early context clearing, useful for standalone windows not created by you. */
        void done();

        /** Get a valid context back again. */
        void reinit();

        DISTRHO_DECLARE_NON_COPYABLE(ScopedGraphicsContext)
        DISTRHO_PREVENT_HEAP_ALLOCATION

    private:
        Window& window;
        Window::PrivateData* const ppData;
        bool active;
        bool reenter;
    };

   /**
      Constructor for a regular, standalone window.
    */
    explicit Window(Application& app);

   /**
      Constructor for a modal window, by having another window as its transient parent.
      The Application instance must be the same between the 2 windows.
    */
    explicit Window(Application& app, Window& transientParentWindow);

   /**
      Constructor for an embed Window without known size,
      typically used in modules or plugins that run inside another host.
    */
    explicit Window(Application& app,
                    uintptr_t parentWindowHandle,
                    double scaleFactor,
                    bool resizable);

   /**
      Constructor for an embed Window with known size,
      typically used in modules or plugins that run inside another host.
    */
    explicit Window(Application& app,
                    uintptr_t parentWindowHandle,
                    uint width,
                    uint height,
                    double scaleFactor,
                    bool resizable);

   /**
      Destructor.
    */
    virtual ~Window();

   /**
      Whether this Window is embed into another (usually not DGL-controlled) Window.
    */
    bool isEmbed() const noexcept;

   /**
      Check if this window is visible / mapped.
      Invisible windows do not receive events except resize.
      @see setVisible(bool)
    */
    bool isVisible() const noexcept;

   /**
      Set window visible (or not) according to @a visible.
      Only valid for standalones, embed windows are always visible.
      @see isVisible(), hide(), show()
    */
    void setVisible(bool visible);

   /**
      Show window.
      This is the same as calling setVisible(true).
      @see isVisible(), setVisible(bool)
    */
    void show();

   /**
      Hide window.
      This is the same as calling setVisible(false).
      @see isVisible(), setVisible(bool)
    */
    void hide();

   /**
      Hide window and notify application of a window close event.
      The application event-loop will stop when all windows have been closed.
      For standalone windows only, has no effect if window is embed.
      @see isEmbed()

      @note It is possible to hide the window while not stopping the event-loop.
            A closed window is always hidden, but the reverse is not always true.
    */
    void close();

   /**
      Check if this window is resizable (by the user or window manager).
      @see setResizable
    */
    bool isResizable() const noexcept;

   /**
      Set window as resizable (by the user or window manager).
      It is always possible to resize a window programmatically, which is not the same as the user being allowed to it.
      @note This function does nothing for plugins, where the resizable state is set via macro.
      @see DISTRHO_UI_USER_RESIZABLE
    */
    void setResizable(bool resizable);

   /**
      Get X offset, typically 0.
    */
    int getOffsetX() const noexcept;

   /**
      Get Y offset, typically 0.
    */
    int getOffsetY() const noexcept;

   /**
      Get offset.
    */
    Point<int> getOffset() const noexcept;

   /**
      Set X offset.
    */
    void setOffsetX(int x);

   /**
      Set Y offset.
    */
    void setOffsetY(int y);

   /**
      Set offset using @a x and @a y values.
    */
    void setOffset(int x, int y);

   /**
      Set offset.
    */
    void setOffset(const Point<int>& offset);

   /**
      Get width.
    */
    uint getWidth() const noexcept;

   /**
      Get height.
    */
    uint getHeight() const noexcept;

   /**
      Get size.
    */
    Size<uint> getSize() const noexcept;

   /**
      Set width.
    */
    void setWidth(uint width);

   /**
      Set height.
    */
    void setHeight(uint height);

   /**
      Set size using @a width and @a height values.
    */
    void setSize(uint width, uint height);

   /**
      Set size.
    */
    void setSize(const Size<uint>& size);

   /**
      Get the title of the window previously set with setTitle().
    */
    const char* getTitle() const noexcept;

   /**
      Set the title of the window, typically displayed in the title bar or in window switchers.

      This only makes sense for non-embedded windows.
    */
    void setTitle(const char* title);

   /**
      Check if key repeat events are ignored.
    */
    bool isIgnoringKeyRepeat() const noexcept;

   /**
      Set to ignore (or not) key repeat events according to @a ignore.
    */
    void setIgnoringKeyRepeat(bool ignore) noexcept;

   /**
      Get the clipboard contents.

      This gets the system clipboard contents,
      which may have been set with setClipboard() or copied from another application.

      Returns the clipboard contents, or null.

      @note By default only "text/plain" mimetype is supported and returned.
            Override onClipboardDataOffer for supporting other types.
    */
    const void* getClipboard(size_t& dataSize);

   /**
      Set the clipboard contents.

      This sets the system clipboard contents,
      which can be retrieved with getClipboard() or pasted into other applications.

      If using a string, the use of a null terminator is required (and must be part of dataSize).@n
      The MIME type of the data "text/plain" is assumed if null is used.
    */
    bool setClipboard(const char* mimeType, const void* data, size_t dataSize);

   /**
      Set the mouse cursor.

      This changes the system cursor that is displayed when the pointer is inside the window.
      May fail if setting the cursor is not supported on this system,
      for example if compiled on X11 without Xcursor support.
    */
    bool setCursor(MouseCursor cursor);

   /**
      Add a callback function to be triggered on every idle cycle or on a specific timer frequency.
      You can add more than one, and remove them at anytime with removeIdleCallback().
      This can be used to perform some action at a regular interval with relatively low frequency.

      If providing a timer frequency, there are a few things to note:
       1. There is a platform-specific limit to the number of supported timers, and overhead associated with each,
          so you should create only a few timers and perform several tasks in one if necessary.
       2. This timer frequency is not guaranteed to have a resolution better than 10ms
          (the maximum timer resolution on Windows) and may be rounded up if it is too short.
          On X11 and MacOS, a resolution of about 1ms can usually be relied on.
    */
    bool addIdleCallback(IdleCallback* callback, uint timerFrequencyInMs = 0);

   /**
      Remove an idle callback previously added via addIdleCallback().
    */
    bool removeIdleCallback(IdleCallback* callback);

   /**
      Get the application associated with this window.
    */
    Application& getApp() const noexcept;

   /**
      Get the graphics context associated with this window.
      GraphicsContext is an empty struct and needs to be casted into a different type in order to be usable,
      for example GraphicsContext.
      @see CairoSubWidget, CairoTopLevelWidget
    */
    const GraphicsContext& getGraphicsContext() const noexcept;

   /**
      Get the "native" window handle.
      Returned value depends on the platform:
       - HaikuOS: This is a pointer to a `BView`.
       - MacOS: This is a pointer to an `NSView*`.
       - Windows: This is a `HWND`.
       - Everything else: This is an [X11] `Window`.
    */
    uintptr_t getNativeWindowHandle() const noexcept;

   /**
      Get the scale factor requested for this window.
      This is purely informational, and up to developers to choose what to do with it.

      If you do not want to deal with this yourself,
      consider using setGeometryConstraints() where you can specify to automatically scale the window contents.
      @see setGeometryConstraints
    */
    double getScaleFactor() const noexcept;

   /**
      Enable automatic internal scaling based on a custom width and height.
      This will internally scale up drawing and scale down user events coordinates and sizes,
      but the result is likely blurry.

      This can be used as an early step for high-dpi support,
      that scales up the window contents in a blurry but usable way.
    */
    void enableInternalScalingWithSize(uint baseWidth, uint baseHeight, bool keepAspectRatio = false);

   /**
      Grab the keyboard input focus.
    */
    void focus();

   #ifdef DGL_USE_FILE_BROWSER
   /**
      Open a file browser dialog with this window as transient parent.
      A few options can be specified to setup the dialog.

      If a path is selected, onFileSelected() will be called with the user chosen path.
      If the user cancels or does not pick a file, onFileSelected() will be called with nullptr as filename.

      This function does not block the event loop.
    */
    bool openFileBrowser(const DGL_NAMESPACE::FileBrowserOptions& options = FileBrowserOptions());
   #endif

   #ifdef DGL_USE_WEB_VIEW
   /**
      Create a new web view.

      The web view will be added on top of this window.
      This means it will draw on top of whatever is below it,
      something to take into consideration if mixing regular widgets with web views.

      Provided metrics in @p options must have scale factor pre-applied.

      @p url:     The URL to open, assumed to be in encoded form (e.g spaces converted to %20)
      @p options: Extra options, optional
    */
    bool createWebView(const char* url, const DGL_NAMESPACE::WebViewOptions& options = WebViewOptions());

   /**
      Evaluate/run JavaScript on the web view.
    */
    void evaluateJS(const char* js);
   #endif

   /**
      Request repaint of this window, for the entire area.
    */
    void repaint() noexcept;

   /**
      Request partial repaint of this window, with bounds according to @a rect.
    */
    void repaint(const Rectangle<uint>& rect) noexcept;

   /**
      Render this window's content into a picture file, specified by @a filename.
      Window must be visible and on screen.
      Written picture format is PPM.
    */
    void renderToPicture(const char* filename);

   /**
      Run this window as a modal, blocking input events from the parent.
      Only valid for windows that have been created with another window as parent (as passed in the constructor).
      Can optionally block-wait, but such option is only available if the application is running as standalone.
    */
    void runAsModal(bool blockWait = false);

   /**
      Get the geometry constraints set for the Window.
      @see setGeometryConstraints
    */
    Size<uint> getGeometryConstraints(bool& keepAspectRatio);

   /**
      Set geometry constraints for the Window when resized by the user.
      Also whether to keep aspect ratio based on this size.
    */
    void setGeometryConstraints(uint minimumWidth, uint minimumHeight, bool keepAspectRatio = false);

   #if DGL_ALLOW_DEPRECATED_METHODS
   /** DEPRECATED DO NOT USE.
    * The old deprecated function allowed for optional `bool automaticallyScale` and `bool resizeNowIfAutoScaling`.
    * This turned out to be not be a good idea; now the scaling on constraints is never done automatically.
    */
    DISTRHO_DEPRECATED_BY("setGeometryConstraints(uint, uint, bool)")
    void setGeometryConstraints(uint minimumWidth,
                                uint minimumHeight,
                                bool keepAspectRatio,
                                bool automaticallyScale,
                                bool resizeNowIfAutoScaling);
   #endif

   /**
      Set the transient parent of the window.

      Set this for transient children like dialogs, to have them properly associated with their parent window.
      This should be not be called for embed windows, or after making the window visible.
    */
    void setTransientParent(uintptr_t transientParentWindowHandle);

   #if DGL_ALLOW_DEPRECATED_METHODS
   /** DEPRECATED Use isIgnoringKeyRepeat(). */
    DISTRHO_DEPRECATED_BY("isIgnoringKeyRepeat()")
    inline bool getIgnoringKeyRepeat() const noexcept { return isIgnoringKeyRepeat(); }

   /** DEPRECATED Use getScaleFactor(). */
    DISTRHO_DEPRECATED_BY("getScaleFactor()")
    inline double getScaling() const noexcept { return getScaleFactor(); }

   /** DEPRECATED Use runAsModal(bool). */
    DISTRHO_DEPRECATED_BY("runAsModal(bool)")
    inline void exec(bool blockWait = false) { runAsModal(blockWait); }
   #endif

protected:
   /**
      Get the types available for the data in a clipboard.
      Must only be called within the context of onClipboardDataOffer.
    */
    std::vector<ClipboardDataOffer> getClipboardDataOfferTypes();

   /**
      A function called when clipboard has data present, possibly with several datatypes.
      While handling this event, the data types can be investigated with getClipboardDataOfferTypes() to decide whether to accept the offer.

      Reimplement and return a non-zero id to accept the clipboard data offer for a particular type.
      Applications must ignore any type they do not recognize.

      The default implementation accepts the "text/plain" mimetype.
    */
    virtual uint32_t onClipboardDataOffer();

   /**
      A function called when the window is attempted to be closed.
      Returning true closes the window, which is the default behaviour.
      Override this method and return false to prevent the window from being closed by the user.

      This method is not used for embed windows, and not even made available in DISTRHO_NAMESPACE::UI.
      For embed windows, closing is handled by the host/parent process and we have no control over it.
      As such, a close action on embed windows will always succeed and cannot be cancelled.

      NOTE: This currently does not work under macOS.
    */
    virtual bool onClose();

   /**
      A function called when the window gains or loses the keyboard focus.
      The default implementation does nothing.
    */
    virtual void onFocus(bool focus, CrossingMode mode);

   #if DGL_ALLOW_DEPRECATED_METHODS
   /** DEPRECATED DO NOT USE */
    DISTRHO_DEPRECATED
    virtual void onReshape(uint width, uint height);
   #endif

   /**
      A function called when scale factor requested for this window changes.
      The default implementation does nothing.
      WARNING function needs a proper name
    */
    virtual void onScaleFactorChanged(double scaleFactor);

  #ifdef DGL_USE_FILE_BROWSER
   /**
      A function called when a path is selected by the user, as triggered by openFileBrowser().
      This action happens after the user confirms the action, so the file browser dialog will be closed at this point.
      The default implementation does nothing.
    */
    virtual void onFileSelected(const char* filename);

   #if DGL_ALLOW_DEPRECATED_METHODS
   /** DEPRECATED Use onFileSelected(). */
    DISTRHO_DEPRECATED_BY("onFileSelected(const char*)")
    inline virtual void fileBrowserSelected(const char* filename) { return onFileSelected(filename); }
   #endif
  #endif

private:
    PrivateData* const pData;
    friend class Application;
    friend class TopLevelWidget;
   #ifdef DISTRHO_NAMESPACE
    friend class DISTRHO_NAMESPACE::PluginWindow;
   #endif

   /** @internal */
    explicit Window(Application& app,
                    uintptr_t parentWindowHandle,
                    uint width,
                    uint height,
                    double scaleFactor,
                    bool resizable,
                    bool usesScheduledRepaints,
                    bool usesSizeRequest,
                    bool doPostInit);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Window)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WINDOW_HPP_INCLUDED
