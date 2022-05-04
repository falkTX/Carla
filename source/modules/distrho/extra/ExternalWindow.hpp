/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
#define DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED

#include "String.hpp"

#ifndef DISTRHO_OS_WINDOWS
# include <cerrno>
# include <signal.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// ExternalWindow class

/**
   External Window class.

   This is a standalone TopLevelWidget/Window-compatible class, but without any real event handling.
   Being compatible with TopLevelWidget/Window, it allows to be used as DPF UI target.

   It can be used to embed non-DPF things or to run a tool in a new process as the "UI".
   The uiIdle() function will be called at regular intervals to keep UI running.
   There are helper methods in place to launch external tools and keep track of its running state.

   External windows can be setup to run in 3 different modes:
     * Embed:
        Embed into the host UI, even-loop driven by the host.
        This is basically working as a regular plugin UI, as you typically expect them to.
        The plugin side does not get control over showing, hiding or closing the window (as usual for plugins).
        No restrictions on supported plugin format, everything should work.
        Requires DISTRHO_PLUGIN_HAS_EMBED_UI to be set to 1.

     * Semi-external:
        The UI is not embed into the host, but the even-loop is still driven by it.
        In this mode the host does not have control over the UI except for showing, hiding and setting transient parent.
        It is possible to close the window from the plugin, the host will be notified of such case.
        Host regularly calls isQuitting() to check if the UI got closed by the user or plugin side.
        This mode is only possible in LV2 plugin formats, using lv2ui:showInterface extension.

     * Standalone:
        The UI is not embed into the host or uses its event-loop, basically running as standalone.
        The host only has control over showing and hiding the window, nothing else.
        The UI is still free to close itself at any point.
        DPF will keep calling isRunning() to check if it should keep the event-loop running.
        Only possible in JACK and DSSI targets, as the UIs are literally standalone applications there.

   Please note that for non-embed windows, you cannot show the window yourself.
   The plugin window is only allowed to hide or close itself, a "show" action needs to come from the host.

   A few callbacks are provided so that implementations do not need to care about checking for state changes.
   They are not called on construction, but will be everytime something changes either by the host or the window itself.
 */
class ExternalWindow
{
    struct PrivateData;

public:
   /**
      Constructor.
    */
    explicit ExternalWindow()
       : pData() {}

   /**
      Constructor for DPF internal use.
    */
    explicit ExternalWindow(const PrivateData& data)
       : pData(data) {}

   /**
      Destructor.
    */
    virtual ~ExternalWindow()
    {
        DISTRHO_SAFE_ASSERT(!pData.visible);
    }

   /* --------------------------------------------------------------------------------------------------------
    * ExternalWindow specific calls - Host side calls that you can reimplement for fine-grained funtionality */

   /**
      Check if main-loop is running.
      This is used under standalone mode to check whether to keep things running.
      Returning false from this function will stop the event-loop and close the window.
    */
    virtual bool isRunning() const
    {
#ifndef DISTRHO_OS_WINDOWS
        if (ext.inUse)
            return ext.isRunning();
#endif
        return isVisible();
    }

   /**
      Check if we are about to close.
      This is used when the event-loop is provided by the host to check if it should close the window.
      It is also used in standalone mode right after isRunning() returns false to verify if window needs to be closed.
    */
    virtual bool isQuitting() const
    {
#ifndef DISTRHO_OS_WINDOWS
        return ext.inUse ? ext.isQuitting : pData.isQuitting;
#else
        return pData.isQuitting;
#endif
    }

   /**
      Get the "native" window handle.
      This can be reimplemented in order to pass the native window to hosts that can use such informaton.

      Returned value type depends on the platform:
       - HaikuOS: This is a pointer to a `BView`.
       - MacOS: This is a pointer to an `NSView*`.
       - Windows: This is a `HWND`.
       - Everything else: This is an [X11] `Window`.

      @note Only available to override if DISTRHO_PLUGIN_HAS_EMBED_UI is set to 1.
    */
    virtual uintptr_t getNativeWindowHandle() const noexcept
    {
        return 0;
    }

   /**
      Grab the keyboard input focus.
      Typically you would setup OS-native methods to bring the window to front and give it focus.
      Default implementation does nothing.
    */
    virtual void focus() {}

   /* --------------------------------------------------------------------------------------------------------
    * TopLevelWidget-like calls - Information, can be called by either host or plugin */

#if DISTRHO_PLUGIN_HAS_EMBED_UI
   /**
      Whether this Window is embed into another (usually not DGL-controlled) Window.
    */
    bool isEmbed() const noexcept
    {
        return pData.parentWindowHandle != 0;
    }
#endif

   /**
      Check if this window is visible.
      @see setVisible(bool)
    */
    bool isVisible() const noexcept
    {
        return pData.visible;
    }

   /**
      Whether this Window is running as standalone, that is, without being coupled to a host event-loop.
      When in standalone mode, isRunning() is called to check if the event-loop should keep running.
    */
    bool isStandalone() const noexcept
    {
        return pData.isStandalone;
    }

   /**
      Get width of this window.
      Only relevant to hosts when the UI is embedded.
    */
    uint getWidth() const noexcept
    {
        return pData.width;
    }

   /**
      Get height of this window.
      Only relevant to hosts when the UI is embedded.
    */
    uint getHeight() const noexcept
    {
        return pData.height;
    }

   /**
      Get the scale factor requested for this window.
      This is purely informational, and up to developers to choose what to do with it.
    */
    double getScaleFactor() const noexcept
    {
        return pData.scaleFactor;
    }

   /**
      Get the title of the window previously set with setTitle().
      This is typically displayed in the title bar or in window switchers.
    */
    const char* getTitle() const noexcept
    {
        return pData.title;
    }

#if DISTRHO_PLUGIN_HAS_EMBED_UI
   /**
      Get the "native" window handle that this window should embed itself into.
      Returned value type depends on the platform:
       - HaikuOS: This is a pointer to a `BView`.
       - MacOS: This is a pointer to an `NSView*`.
       - Windows: This is a `HWND`.
       - Everything else: This is an [X11] `Window`.
    */
    uintptr_t getParentWindowHandle() const noexcept
    {
        return pData.parentWindowHandle;
    }
#endif

   /**
      Get the transient window that we should attach ourselves to.
      TODO what id? also NSView* on macOS, or NSWindow?
    */
    uintptr_t getTransientWindowId() const noexcept
    {
        return pData.transientWinId;
    }

   /* --------------------------------------------------------------------------------------------------------
    * TopLevelWidget-like calls - actions called by either host or plugin */

   /**
      Hide window.
      This is the same as calling setVisible(false).
      Embed windows should never call this!
      @see isVisible(), setVisible(bool)
    */
    void hide()
    {
        setVisible(false);
    }

   /**
      Hide the UI and gracefully terminate.
      Embed windows should never call this!
    */
    virtual void close()
    {
        pData.isQuitting = true;
        hide();
#ifndef DISTRHO_OS_WINDOWS
        if (ext.inUse)
            terminateAndWaitForExternalProcess();
#endif
    }

   /**
      Set width of this window.
      Can trigger a sizeChanged callback.
      Only relevant to hosts when the UI is embedded.
    */
    void setWidth(uint width)
    {
        setSize(width, getHeight());
    }

   /**
      Set height of this window.
      Can trigger a sizeChanged callback.
      Only relevant to hosts when the UI is embedded.
    */
    void setHeight(uint height)
    {
        setSize(getWidth(), height);
    }

   /**
      Set size of this window using @a width and @a height values.
      Can trigger a sizeChanged callback.
      Only relevant to hosts when the UI is embedded.
    */
    void setSize(uint width, uint height)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(width > 1, width,);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(height > 1, height,);

        if (pData.width == width && pData.height == height)
            return;

        pData.width = width;
        pData.height = height;
        sizeChanged(width, height);
    }

   /**
      Set the title of the window, typically displayed in the title bar or in window switchers.
      Can trigger a titleChanged callback.
      Only relevant to hosts when the UI is not embedded.
    */
    void setTitle(const char* title)
    {
        if (pData.title == title)
            return;

        pData.title = title;
        titleChanged(title);
    }

   /**
      Set geometry constraints for the Window when resized by the user.
    */
    void setGeometryConstraints(uint minimumWidth, uint minimumHeight, bool keepAspectRatio = false)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(minimumWidth > 0, minimumWidth,);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(minimumHeight > 0, minimumHeight,);

        pData.minWidth = minimumWidth;
        pData.minHeight = minimumHeight;
        pData.keepAspectRatio = keepAspectRatio;
    }

   /* --------------------------------------------------------------------------------------------------------
    * TopLevelWidget-like calls - actions called by the host */

   /**
      Show window.
      This is the same as calling setVisible(true).
      @see isVisible(), setVisible(bool)
    */
    void show()
    {
        setVisible(true);
    }

   /**
      Set window visible (or not) according to @a visible.
      @see isVisible(), hide(), show()
    */
    void setVisible(bool visible)
    {
        if (pData.visible == visible)
            return;

        pData.visible = visible;
        visibilityChanged(visible);
    }

   /**
      Called by the host to set the transient parent window that we should attach ourselves to.
      TODO what id? also NSView* on macOS, or NSWindow?
    */
    void setTransientWindowId(uintptr_t winId)
    {
        if (pData.transientWinId == winId)
            return;

        pData.transientWinId = winId;
        transientParentWindowChanged(winId);
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * ExternalWindow special calls for running externals tools */

    bool startExternalProcess(const char* args[])
    {
#ifndef DISTRHO_OS_WINDOWS
        ext.inUse = true;

        return ext.start(args);
#else
        (void)args;
        return false; // TODO
#endif
    }

    void terminateAndWaitForExternalProcess()
    {
#ifndef DISTRHO_OS_WINDOWS
        ext.isQuitting = true;
        ext.terminateAndWait();
#else
        // TODO
#endif
    }

   /* --------------------------------------------------------------------------------------------------------
    * ExternalWindow specific callbacks */

   /**
      A callback for when the window size changes.
      @note WIP this might need to get fed back into the host somehow.
    */
    virtual void sizeChanged(uint /* width */, uint /* height */)
    {
        // unused, meant for custom implementations
    }

   /**
      A callback for when the window title changes.
      @note WIP this might need to get fed back into the host somehow.
    */
    virtual void titleChanged(const char* /* title */)
    {
        // unused, meant for custom implementations
    }

   /**
      A callback for when the window visibility changes.
      @note WIP this might need to get fed back into the host somehow.
    */
    virtual void visibilityChanged(bool /* visible */)
    {
        // unused, meant for custom implementations
    }

   /**
      A callback for when the transient parent window changes.
    */
    virtual void transientParentWindowChanged(uintptr_t /* winId */)
    {
        // unused, meant for custom implementations
    }

private:
    friend class PluginWindow;
    friend class UI;

#ifndef DISTRHO_OS_WINDOWS
    struct ExternalProcess {
        bool inUse;
        bool isQuitting;
        mutable pid_t pid;

        ExternalProcess()
            : inUse(false),
              isQuitting(false),
              pid(0) {}

        bool isRunning() const noexcept
        {
            if (pid <= 0)
                return false;

            const pid_t p = ::waitpid(pid, nullptr, WNOHANG);

            if (p == pid || (p == -1 && errno == ECHILD))
            {
                d_stdout("NOTICE: Child process exited while idle");
                pid = 0;
                return false;
            }

            return true;
        }

        bool start(const char* args[])
        {
            terminateAndWait();

            pid = vfork();

            switch (pid)
            {
            case 0:
                execvp(args[0], (char**)args);
                _exit(1);
                return false;

            case -1:
                d_stderr("Could not start external ui");
                return false;

            default:
                return true;
            }
        }

        void terminateAndWait()
        {
            if (pid <= 0)
                return;

            d_stdout("Waiting for external process to stop,,,");

            bool sendTerm = true;

            for (pid_t p;;)
            {
                p = ::waitpid(pid, nullptr, WNOHANG);

                switch (p)
                {
                case 0:
                    if (sendTerm)
                    {
                        sendTerm = false;
                        ::kill(pid, SIGTERM);
                    }
                    break;

                case -1:
                    if (errno == ECHILD)
                    {
                        d_stdout("Done! (no such process)");
                        pid = 0;
                        return;
                    }
                    break;

                default:
                    if (p == pid)
                    {
                        d_stdout("Done! (clean wait)");
                        pid = 0;
                        return;
                    }
                    break;
                }

                // 5 msec
                usleep(5*1000);
            }
        }
    } ext;
#endif

    struct PrivateData {
        uintptr_t parentWindowHandle;
        uintptr_t transientWinId;
        uint width;
        uint height;
        double scaleFactor;
        String title;
        uint minWidth;
        uint minHeight;
        bool keepAspectRatio;
        bool isQuitting;
        bool isStandalone;
        bool visible;

        PrivateData()
            : parentWindowHandle(0),
              transientWinId(0),
              width(1),
              height(1),
              scaleFactor(1.0),
              title(),
              minWidth(0),
              minHeight(0),
              keepAspectRatio(false),
              isQuitting(false),
              isStandalone(false),
              visible(false) {}
    } pData;

    DISTRHO_DECLARE_NON_COPYABLE(ExternalWindow)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
