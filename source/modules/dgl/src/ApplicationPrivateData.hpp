/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_APP_PRIVATE_DATA_HPP_INCLUDED
#define DGL_APP_PRIVATE_DATA_HPP_INCLUDED

#include "../Application.hpp"

#include <list>

#ifdef DISTRHO_OS_WINDOWS
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <winsock2.h>
# include <windows.h>
typedef HANDLE d_ThreadHandle;
#else
# include <pthread.h>
typedef pthread_t d_ThreadHandle;
#endif

#ifdef DISTRHO_OS_MAC
typedef struct PuglWorldImpl PuglWorld;
#endif

START_NAMESPACE_DGL

class Window;

#ifndef DISTRHO_OS_MAC
typedef struct PuglWorldImpl PuglWorld;
#endif

// --------------------------------------------------------------------------------------------------------------------

struct Application::PrivateData {
    /** Pugl world instance. */
    PuglWorld* const world;

    /** Whether the applicating uses modern backend, otherwise classic. */
    const bool isModern;

    /** Whether the application is running as standalone, otherwise it is part of a plugin. */
    const bool isStandalone;

    /** Whether the applicating is starting up, that is, no windows have been made visible yet. Defaults to true. */
    bool isStarting;

    /** Whether the applicating is about to quit, or already stopped. Defaults to false. */
    bool isQuitting;

    /** Helper for safely close everything from main thread. */
    bool isQuittingInNextCycle;

    /** When true force all windows to be repainted on next idle. */
    bool needsRepaint;

    /** Counter of visible windows, only used in standalone mode.
        If 0->1, application is starting. If 1->0, application is quitting/stopping. */
    uint visibleWindows;

    /** Handle that identifies the main thread. Used to check if calls belong to current thread or not. */
    d_ThreadHandle mainThreadHandle;

    /** List of windows for this application. Only used during `close`. */
    std::list<DGL_NAMESPACE::Window*> windows;

    /** List of idle callbacks for this application. */
    std::list<DGL_NAMESPACE::IdleCallback*> idleCallbacks;

    /** Constructor and destructor */
    explicit PrivateData(bool standalone, Type type);
    ~PrivateData();

    /** Flag one window as shown, which increments @a visibleWindows.
        Sets @a isQuitting and @a isStarting as false if this is the first window.
        For standalone mode only. */
    void oneWindowShown() noexcept;

    /** Flag one window as closed, which decrements @a visibleWindows.
        Sets @a isQuitting as true if this is the last window.
        For standalone mode only. */
    void oneWindowClosed() noexcept;

    /** Run Pugl world update for @a timeoutInMs, and then each idle callback in order of registration. */
    void idle(uint timeoutInMs);

    /** Run each idle callback without updating pugl world. */
    void triggerIdleCallbacks();

    /** Trigger a repaint of all windows if @a needsRepaint is true. */
    void repaintIfNeeeded();

    /** Set flag indicating application is quitting, and close all windows in reverse order of registration.
        For standalone mode only. */
    void quit();

    /** Get time via pugl */
    double getTime() const;

    /** Set pugl world class name. */
    void setClassName(const char* name);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_PRIVATE_DATA_HPP_INCLUDED
