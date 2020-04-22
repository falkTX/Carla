/*
 * Carla process utils
 * Copyright (C) 2019-2020 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_OS_WIN
# include <csignal>
# include <csetjmp>
#endif

// --------------------------------------------------------------------------------------------------------------------
// process functions

/*
 * Set current process name.
 */
void carla_setProcessName(const char* const name) noexcept;

/*
 * Set flag to automatically terminate ourselves if parent process dies.
 */
void carla_terminateProcessOnParentExit(const bool kill) noexcept;

// --------------------------------------------------------------------------------------------------------------------
// process utility classes

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
#ifndef CARLA_OS_WIN
    static jmp_buf s_env;
    static sig_t s_oldsig;
    static void sig_handler(const int signum);
#endif

    CARLA_DECLARE_NON_COPY_CLASS(ScopedAbortCatcher)
    CARLA_PREVENT_HEAP_ALLOCATION
};

/*
 * Store and restore all signal handlers for a function scope.
 */
class CarlaSignalRestorer {
public:
  CarlaSignalRestorer();
  ~CarlaSignalRestorer();

private:
#ifndef CARLA_OS_WIN
    struct ::sigaction sigs[16];
#endif

    CARLA_DECLARE_NON_COPY_CLASS(CarlaSignalRestorer)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_PROCESS_UTILS_HPP_INCLUDED
