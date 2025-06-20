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

#ifndef DISTRHO_TIME_HPP_INCLUDED
#define DISTRHO_TIME_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
# include <winsock2.h>
# include <windows.h>
# include <mmsystem.h>
#else
# include <ctime>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------
// d_gettime_*

/*
 * Get a monotonically-increasing time in milliseconds.
 */
static inline
uint32_t d_gettime_ms() noexcept
{
   #if defined(DISTRHO_OS_MAC)
    static const time_t s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000;
    return (clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000) - s;
   #elif defined(DISTRHO_OS_WINDOWS)
    return static_cast<uint32_t>(timeGetTime());
   #else
    static struct {
        timespec ts;
        int r;
        uint32_t ms;
    } s = { {}, clock_gettime(CLOCK_MONOTONIC, &s.ts), static_cast<uint32_t>(s.ts.tv_sec * 1000 +
                                                                             s.ts.tv_nsec / 1000000) };
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - s.ms;
   #endif
}

/*
 * Get a monotonically-increasing time in microseconds.
 */
static inline
uint64_t d_gettime_us() noexcept
{
   #if defined(DISTRHO_OS_MAC)
    static const uint64_t s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000;
    return (clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000) - s;
   #elif defined(DISTRHO_OS_WINDOWS)
    static struct {
      LARGE_INTEGER freq;
      LARGE_INTEGER counter;
      BOOL r1, r2;
    } s = { {}, {}, QueryPerformanceFrequency(&s.freq), QueryPerformanceCounter(&s.counter) };

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart - s.counter.QuadPart) * 1000000 / s.freq.QuadPart;
   #else
    static struct {
        timespec ts;
        int r;
        uint64_t us;
    } s = { {}, clock_gettime(CLOCK_MONOTONIC, &s.ts), static_cast<uint64_t>(s.ts.tv_sec * 1000000 +
                                                                             s.ts.tv_nsec / 1000) };
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000) - s.us;
   #endif
}

/*
 * Get a monotonically-increasing time in nanoseconds.
 */
static inline
uint64_t d_gettime_ns() noexcept
{
   #if defined(DISTRHO_OS_MAC)
    static const uint64_t s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - s;
   #elif defined(DISTRHO_OS_WINDOWS)
    static struct {
      LARGE_INTEGER freq;
      LARGE_INTEGER counter;
      BOOL r1, r2;
    } s = { {}, {}, QueryPerformanceFrequency(&s.freq), QueryPerformanceCounter(&s.counter) };

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart - s.counter.QuadPart) * 1000000000ULL / s.freq.QuadPart;
   #else
    static struct {
        timespec ts;
        int r;
        uint64_t ns;
    } s = { {}, clock_gettime(CLOCK_MONOTONIC, &s.ts), static_cast<uint64_t>(s.ts.tv_sec * 1000000000ULL +
                                                                             s.ts.tv_nsec) };
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000000ULL + ts.tv_nsec) - s.ns;
   #endif
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_TIME_HPP_INCLUDED
