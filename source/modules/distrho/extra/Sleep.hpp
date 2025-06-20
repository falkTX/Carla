/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_SLEEP_HPP_INCLUDED
#define DISTRHO_SLEEP_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <winsock2.h>
# include <windows.h>
#else
# include <unistd.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------
// d_*sleep

/*
 * Sleep for 'secs' seconds.
 */
static inline
void d_sleep(const uint secs) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(secs > 0,);

    try {
#ifdef DISTRHO_OS_WINDOWS
        ::Sleep(secs * 1000);
#else
        ::sleep(secs);
#endif
    } DISTRHO_SAFE_EXCEPTION("d_sleep");
}

/*
 * Sleep for 'msecs' milliseconds.
 */
static inline
void d_msleep(const uint msecs) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(msecs > 0,);

    try {
#ifdef DISTRHO_OS_WINDOWS
        ::Sleep(msecs);
#else
        ::usleep(msecs * 1000);
#endif
    } DISTRHO_SAFE_EXCEPTION("d_msleep");
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_SLEEP_HPP_INCLUDED
