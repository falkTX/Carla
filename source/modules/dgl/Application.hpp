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

#ifndef DGL_APP_HPP_INCLUDED
#define DGL_APP_HPP_INCLUDED

#include "Base.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Forward class names

class Window;

// -----------------------------------------------------------------------

/**
   Base DGL Application class.

   One application instance is required for creating a window.
   There's no single/global application instance in DGL, and multiple
   windows can share the same app instance.

   In standalone mode an application will automatically quit its
   event-loop when all its windows are closed.
 */
class Application
{
public:
   /**
      Constructor.
    */
    Application();

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
    void exec(int idleTime = 10);

   /**
      Quit the application.
      This stops the event-loop and closes all Windows.
    */
    void quit();

   /**
      Check if the application is about to quit.
      Returning true means there's no event-loop running at the moment (or it's just about to stop).
    */
    bool isQuiting() const noexcept;

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Window;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Application)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_HPP_INCLUDED
