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

#ifndef WATER_TIME_H_INCLUDED
#define WATER_TIME_H_INCLUDED

#include "../water.h"

namespace water {
namespace Time {

//==============================================================================
// Static methods for getting system timers directly..

/** Returns the current system time.

    Returns the number of milliseconds since midnight Jan 1st 1970 UTC.

    Should be accurate to within a few millisecs, depending on platform,
    hardware, etc.
*/
int64 currentTimeMillis() noexcept;

/** Returns the number of millisecs since a fixed event (usually system startup).

    This returns a monotonically increasing value which it unaffected by changes to the
    system clock. It should be accurate to within a few millisecs, depending on platform,
    hardware, etc.

    Being a 32-bit return value, it will of course wrap back to 0 after 2^32 seconds of
    uptime, so be careful to take that into account. If you need a 64-bit time, you can
    use currentTimeMillis() instead.
*/
uint32 getMillisecondCounter() noexcept;

}
}

#endif // WATER_TIME_H_INCLUDED
