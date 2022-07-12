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

#ifndef DGL_APP_HPP_INCLUDED
#define DGL_APP_HPP_INCLUDED

#include "Base.hpp"

#ifdef DISTRHO_NAMESPACE
START_NAMESPACE_DISTRHO
class PluginApplication;
END_NAMESPACE_DISTRHO
#endif

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Base DGL Application class.

   One application instance is required for creating a window.
   There's no single/global application instance in DGL, and multiple windows can share the same app instance.

   In standalone mode an application will automatically quit its event-loop when all its windows are closed.

   Unless stated otherwise, functions within this class are not thread-safe.
 */
class DISTRHO_API Application
{
public:
   /**
      Constructor.
    */
    // NOTE: the default value is not yet passed, so we catch where we use this
    Application(bool isStandalone = true);

   /**
      Destructor.
    */
    virtual ~Application();

   /**
      Idle function.
      This runs the application event-loop once.
    */
    void idle();

   /**
      Run the application event-loop until all Windows are closed.
      idle() is called at regular intervals.
      @note This function is meant for standalones only, *never* call this from plugins.
    */
    void exec(uint idleTimeInMs = 30);

   /**
      Quit the application.
      This stops the event-loop and closes all Windows.
      This function is thread-safe.
    */
    void quit();

   /**
      Check if the application is about to quit.
      Returning true means there's no event-loop running at the moment (or it's just about to stop).
      This function is thread-safe.
    */
    bool isQuitting() const noexcept;

   /**
      Check if the application is standalone, otherwise running as a module or plugin.
      This function is thread-safe.
    */
    bool isStandalone() const noexcept;

   /**
      Return the time in seconds.

      This is a monotonically increasing clock with high resolution.@n
      The returned time is only useful to compare against other times returned by this function,
      its absolute value has no meaning.
   */
    double getTime() const;

   /**
      Add a callback function to be triggered on every idle cycle.
      You can add more than one, and remove them at anytime with removeIdleCallback().
      Idle callbacks trigger right after OS event handling and Window idle events (within the same cycle).
      There are no guarantees in terms of timing, use Window::addIdleCallback for time-relative callbacks.
    */
    void addIdleCallback(IdleCallback* callback);

   /**
      Remove an idle callback previously added via addIdleCallback().
    */
    void removeIdleCallback(IdleCallback* callback);

   /**
      Get the class name of the application.

      This is a stable identifier for the application, used as the window class/instance name on X11 and Windows.
      It is not displayed to the user, but can be used in scripts and by window managers,
      so it should be the same for every instance of the application, but different from other applications.

      Plugins created with DPF have their class name automatically set based on DGL_NAMESPACE and plugin name.
    */
    const char* getClassName() const noexcept;

   /**
      Set the class name of the application.
      @see getClassName
    */
    void setClassName(const char* name);

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Window;
   #ifdef DISTRHO_NAMESPACE
    friend class DISTRHO_NAMESPACE::PluginApplication;
   #endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Application)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_HPP_INCLUDED
