/*
 * Carla time utils
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_TIME_UTILS_HPP_INCLUDED
#define CARLA_TIME_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <ctime>

#ifdef CARLA_OS_WIN
# include <mmsystem.h>
#endif

// --------------------------------------------------------------------------------------------------------------------
// carla_*sleep

/*
 * Sleep for 'secs' seconds.
 */
static inline
void carla_sleep(const uint secs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(secs > 0,);

    try {
       #ifdef CARLA_OS_WIN
        ::Sleep(secs * 1000);
       #else
        ::sleep(secs);
       #endif
    } CARLA_SAFE_EXCEPTION("carla_sleep");
}

/*
 * Sleep for 'msecs' milliseconds.
 */
static inline
void carla_msleep(const uint msecs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(msecs > 0,);

    try {
       #ifdef CARLA_OS_WIN
        ::Sleep(msecs);
       #else
        ::usleep(msecs * 1000);
       #endif
    } CARLA_SAFE_EXCEPTION("carla_msleep");
}

// --------------------------------------------------------------------------------------------------------------------
// carla_gettime_*

/*
 * Get a monotonically-increasing time in milliseconds.
 */
static inline
uint32_t carla_gettime_ms() noexcept
{
   #if defined(CARLA_OS_MAC)
    static const time_t s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000;
    return (clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000) - s;
   #elif defined(CARLA_OS_WIN)
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
uint64_t carla_gettime_us() noexcept
{
   #if defined(CARLA_OS_MAC)
    static const uint64_t s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000;
    return (clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000) - s;
   #elif defined(CARLA_OS_WIN)
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
uint64_t carla_gettime_ns() noexcept
{
   #if defined(CARLA_OS_MAC)
    static const uint64_t s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - s;
   #elif defined(CARLA_OS_WIN)
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

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_TIME_UTILS_HPP_INCLUDED
