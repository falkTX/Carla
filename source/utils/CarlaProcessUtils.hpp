/*
 * Carla process utils
 * Copyright (C) 2019-2022 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_PROCESS_UTILS_HPP_INCLUDED
#define CARLA_PROCESS_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#ifdef CARLA_OS_LINUX
# include <sys/prctl.h>
#endif

#ifdef CARLA_OS_MAC
# include <dispatch/dispatch.h>
#endif

#ifdef CARLA_OS_HAIKU
typedef __sighandler_t sig_t;
#endif

#ifndef CARLA_OS_WIN
# include <csignal>
# include <csetjmp>
#endif

// --------------------------------------------------------------------------------------------------------------------
// process functions

/*
 * Set current process name.
 */
static inline
void carla_setProcessName(const char* const name) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

#ifdef CARLA_OS_LINUX
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
}

#ifdef CARLA_OS_MAC
static inline
void carla_macOS_proc_exit_handler_kill(void*)
{
    carla_stdout("Carla bridge parent has died, killing ourselves now");
    ::kill(::getpid(), SIGKILL);
}
static inline
void carla_macOS_proc_exit_handler_term(void*)
{
    carla_stdout("Carla bridge parent has died, terminating ourselves now");
    ::kill(::getpid(), SIGTERM);
}
#endif

/*
 * Set flag to automatically terminate ourselves if parent process dies.
 */
static inline
void carla_terminateProcessOnParentExit(const bool kill) noexcept
{
#if defined(CARLA_OS_LINUX)
    ::prctl(PR_SET_PDEATHSIG, kill ? SIGKILL : SIGTERM);
#elif defined(CARLA_OS_MAC)
    const dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC,
                                                            ::getppid(),
                                                            DISPATCH_PROC_EXIT,
                                                            nullptr);

    dispatch_source_set_event_handler_f(source, kill ? carla_macOS_proc_exit_handler_kill
                                                     : carla_macOS_proc_exit_handler_term);

    dispatch_resume(source);
#endif

    // maybe unused
    return; (void)kill;
}

// --------------------------------------------------------------------------------------------------------------------
// process utility classes

#if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
/*
 * Catches SIGABRT for a function scope.
 */
class ScopedAbortCatcher {
public:
    ScopedAbortCatcher();
    ~ScopedAbortCatcher();

    inline bool wasTriggered() const
    {
        return s_triggered;
    }

private:
    static bool s_triggered;
    static jmp_buf s_env;
    static sig_t s_oldsig;
    static void sig_handler(const int signum);

    CARLA_DECLARE_NON_COPYABLE(ScopedAbortCatcher)
    CARLA_PREVENT_HEAP_ALLOCATION
};
#endif

/*
 * Store and restore all signal handlers for a function scope.
 */
class CarlaSignalRestorer {
public:
  CarlaSignalRestorer();
  ~CarlaSignalRestorer();

private:
#if !(defined(CARLA_OS_WASM) || defined(CARLA_OS_WIN))
    struct ::sigaction sigs[16];
#endif

    CARLA_DECLARE_NON_COPYABLE(CarlaSignalRestorer)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_PROCESS_UTILS_HPP_INCLUDED
