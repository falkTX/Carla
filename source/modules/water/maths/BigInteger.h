/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>

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

#ifndef WATER_BIGINTEGER_H_INCLUDED
#define WATER_BIGINTEGER_H_INCLUDED

#include "../memory/HeapBlock.h"

#include "CarlaJuceUtils.hpp"

namespace water {

//==============================================================================
/**
    An arbitrarily large integer class.

    A BigInteger can be used in a similar way to a normal integer, but has no size
    limit (except for memory and performance constraints).

    Negative values are possible, but the value isn't stored as 2s-complement, so
    be careful if you use negative values and look at the values of individual bits.
*/
class BigInteger
{
public:
    //==============================================================================
    /** Creates an empty BigInteger */
    BigInteger() noexcept;

    /** Creates a BigInteger containing an integer value in its low bits.
        The low 32 bits of the number are initialised with this value.
    */
    BigInteger (uint32 value) noexcept;

    /** Creates a BigInteger containing an integer value in its low bits.
        The low 32 bits of the number are initialised with the absolute value
        passed in, and its sign is set to reflect the sign of the number.
    */
    BigInteger (int32 value) noexcept;

    /** Creates a BigInteger containing an integer value in its low bits.
        The low 64 bits of the number are initialised with the absolute value
        passed in, and its sign is set to reflect the sign of the number.
    */
    BigInteger (int64 value) noexcept;

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    BigInteger (BigInteger&&) noexcept;
    BigInteger& operator= (BigInteger&&) noexcept;
   #endif

    /** Destructor. */
    ~BigInteger() noexcept;

    //==============================================================================
    /** Returns the value of a specified bit in the number.
        If the index is out-of-range, the result will be false.
    */
    bool operator[] (int bit) const noexcept;

    //==============================================================================
    /** Resets the value to 0. */
    void clear() noexcept;

    /** Clears a particular bit in the number. */
    bool clearBit (int bitNumber) noexcept;

    /** Sets a specified bit to 1. */
    bool setBit (int bitNumber) noexcept;

    /** Sets or clears a specified bit. */
    bool setBit (int bitNumber, bool shouldBeSet) noexcept;

    //==============================================================================
    /** Returns the index of the highest set bit in the number.
        If the value is zero, this will return -1.
    */
    int getHighestBit() const noexcept;

private:
    //==============================================================================
    enum { numPreallocatedInts = 4 };
    HeapBlock<uint32> heapAllocation;
    uint32 preallocated[numPreallocatedInts];
    size_t allocatedSize;
    int highestBit;

    uint32* getValues() const noexcept;
    uint32* ensureSize (size_t) noexcept;

    CARLA_LEAK_DETECTOR (BigInteger)
};

}

#endif   // WATER_BIGINTEGER_H_INCLUDED
