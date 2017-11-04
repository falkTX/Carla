/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2016 - ROLI Ltd.

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

   -----------------------------------------------------------------------------

   To release a closed-source product which uses other parts of JUCE not
   licensed under the ISC terms, commercial licenses are available: visit
   www.juce.com for more information.

  ==============================================================================
*/

#ifndef JUCE_TIME_H_INCLUDED
#define JUCE_TIME_H_INCLUDED

namespace water {

//==============================================================================
/**
    Holds an absolute date and time.

    Internally, the time is stored at millisecond precision.

    @see RelativeTime
*/
class Time
{
public:
    //==============================================================================
    /** Creates a Time object.

        This default constructor creates a time of midnight Jan 1st 1970 UTC, (which is
        represented internally as 0ms).

        To create a time object representing the current time, use getCurrentTime().

        @see getCurrentTime
    */
    Time() noexcept;

    /** Creates a time based on a number of milliseconds.

        To create a time object set to the current time, use getCurrentTime().

        @param millisecondsSinceEpoch   the number of milliseconds since the unix
                                        'epoch' (midnight Jan 1st 1970 UTC).
        @see getCurrentTime, currentTimeMillis
    */
    explicit Time (int64 millisecondsSinceEpoch) noexcept;

    /** Creates a copy of another Time object. */
    Time (const Time& other) noexcept;

    /** Destructor. */
    ~Time() noexcept;

    /** Copies this time from another one. */
    Time& operator= (const Time& other) noexcept;

    //==============================================================================
    /** Returns a Time object that is set to the current system time.

        This may not be monotonic, as the system time can change at any moment.
        You should therefore not use this method for measuring time intervals.

        @see currentTimeMillis
    */
    static Time getCurrentTime() noexcept;

    //==============================================================================
    // Static methods for getting system timers directly..

    /** Returns the current system time.

        Returns the number of milliseconds since midnight Jan 1st 1970 UTC.

        Should be accurate to within a few millisecs, depending on platform,
        hardware, etc.
    */
    static int64 currentTimeMillis() noexcept;

    /** Returns the number of millisecs since a fixed event (usually system startup).

        This returns a monotonically increasing value which it unaffected by changes to the
        system clock. It should be accurate to within a few millisecs, depending on platform,
        hardware, etc.

        Being a 32-bit return value, it will of course wrap back to 0 after 2^32 seconds of
        uptime, so be careful to take that into account. If you need a 64-bit time, you can
        use currentTimeMillis() instead.

        @see getApproximateMillisecondCounter
    */
    static uint32 getMillisecondCounter() noexcept;

    /** Less-accurate but faster version of getMillisecondCounter().

        This will return the last value that getMillisecondCounter() returned, so doesn't
        need to make a system call, but is less accurate - it shouldn't be more than
        100ms away from the correct time, though, so is still accurate enough for a
        lot of purposes.

        @see getMillisecondCounter
    */
    static uint32 getApproximateMillisecondCounter() noexcept;

private:
    //==============================================================================
    int64 millisSinceEpoch;
};

}

#endif   // JUCE_TIME_H_INCLUDED
