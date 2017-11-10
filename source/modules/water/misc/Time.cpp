/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#include "Time.h"

#include <ctime>
#include <sys/time.h>

#if defined(CARLA_OS_MAC)
# include <mach/mach_time.h>
#elif defined(CARLA_OS_WIN)
# include <mmsystem.h>
#endif

namespace water {

namespace TimeHelpers
{
    static uint32 lastMSCounterValue = 0;

   #ifdef CARLA_OS_MAC
    /*  NB: these are kept outside the HiResCounterInfo struct and initialised to 1 to avoid
        division-by-zero errors if some other static constructor calls us before this file's
        static constructors have had a chance to fill them in correctly..
    */
    static uint64 hiResCounterNumerator = 0, hiResCounterDenominator = 1;

    struct HiResCounterInfo {
        HiResCounterInfo()
        {
            mach_timebase_info_data_t timebase;
            (void) mach_timebase_info (&timebase);

            if (timebase.numer % 1000000 == 0)
            {
                hiResCounterNumerator   = timebase.numer / 1000000;
                hiResCounterDenominator = timebase.denom;
            }
            else
            {
                hiResCounterNumerator   = timebase.numer;
                hiResCounterDenominator = timebase.denom * (uint64) 1000000;
            }
        }
    };

    static HiResCounterInfo hiResCounterInfo;
   #endif
}

//==============================================================================

static uint32 water_millisecondsSinceStartup() noexcept
{
#if defined(CARLA_OS_MAC)
    return (uint32) ((mach_absolute_time() * TimeHelpers::hiResCounterNumerator) / TimeHelpers::hiResCounterDenominator);
#elif defined(CARLA_OS_WIN)
    return (uint32) timeGetTime();
#else
    timespec t;
    clock_gettime (CLOCK_MONOTONIC, &t);

    return (uint32) (t.tv_sec * 1000 + t.tv_nsec / 1000000);
#endif
}

//==============================================================================
Time::Time() noexcept  : millisSinceEpoch (0)
{
}

Time::Time (const Time& other) noexcept  : millisSinceEpoch (other.millisSinceEpoch)
{
}

Time::Time (const int64 ms) noexcept  : millisSinceEpoch (ms)
{
}

Time::~Time() noexcept
{
}

Time& Time::operator= (const Time& other) noexcept
{
    millisSinceEpoch = other.millisSinceEpoch;
    return *this;
}

//==============================================================================

Time Time::getCurrentTime() noexcept
{
    return Time (currentTimeMillis());
}

int64 Time::currentTimeMillis() noexcept
{
    struct timeval tv;
    gettimeofday (&tv, nullptr);
    return ((int64) tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

//==============================================================================

uint32 Time::getMillisecondCounter() noexcept
{
    const uint32 now = water_millisecondsSinceStartup();

    if (now < TimeHelpers::lastMSCounterValue)
    {
        // in multi-threaded apps this might be called concurrently, so
        // make sure that our last counter value only increases and doesn't
        // go backwards..
        if (now < TimeHelpers::lastMSCounterValue - 1000)
            TimeHelpers::lastMSCounterValue = now;
    }
    else
    {
        TimeHelpers::lastMSCounterValue = now;
    }

    return now;
}

uint32 Time::getApproximateMillisecondCounter() noexcept
{
    if (TimeHelpers::lastMSCounterValue == 0)
        getMillisecondCounter();

    return TimeHelpers::lastMSCounterValue;
}

}
