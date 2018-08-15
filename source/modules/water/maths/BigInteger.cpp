/*
  ==============================================================================

   This file is part of the JUCE library.
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

#include "BigInteger.h"

namespace water
{

inline uint32 bitToMask  (const int bit) noexcept           { return (uint32) 1 << (bit & 31); }
inline size_t bitToIndex (const int bit) noexcept           { return (size_t) (bit >> 5); }
inline size_t sizeNeededToHold (int highestBit) noexcept    { return (size_t) (highestBit >> 5) + 1; }

int findHighestSetBit (uint32 n) noexcept
{
    jassert (n != 0); // (the built-in functions may not work for n = 0)

  #if defined(__GNUC__) || defined(__clang__)
    return 31 - __builtin_clz (n);
  #elif _MSVC_VER
    unsigned long highest;
    _BitScanReverse (&highest, n);
    return (int) highest;
  #else
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    return countNumberOfBits (n >> 1);
  #endif
}


//==============================================================================
BigInteger::BigInteger() noexcept
    : allocatedSize (numPreallocatedInts),
      highestBit (-1)
{
    for (int i = 0; i < numPreallocatedInts; ++i)
        preallocated[i] = 0;
}

BigInteger::BigInteger (const int32 value) noexcept
    : allocatedSize (numPreallocatedInts),
      highestBit (31)
{
    preallocated[0] = (uint32) std::abs (value);

    for (int i = 1; i < numPreallocatedInts; ++i)
        preallocated[i] = 0;

    highestBit = getHighestBit();
}

BigInteger::BigInteger (const uint32 value) noexcept
    : allocatedSize (numPreallocatedInts),
      highestBit (31)
{
    preallocated[0] = value;

    for (int i = 1; i < numPreallocatedInts; ++i)
        preallocated[i] = 0;

    highestBit = getHighestBit();
}

BigInteger::BigInteger (int64 value) noexcept
    : allocatedSize (numPreallocatedInts),
      highestBit (63)
{
    if (value < 0)
        value = -value;

    preallocated[0] = (uint32) value;
    preallocated[1] = (uint32) (value >> 32);

    for (int i = 2; i < numPreallocatedInts; ++i)
        preallocated[i] = 0;

    highestBit = getHighestBit();
}

#if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
BigInteger::BigInteger (BigInteger&& other) noexcept
    : heapAllocation (static_cast<HeapBlock<uint32>&&> (other.heapAllocation)),
      allocatedSize (other.allocatedSize),
      highestBit (other.highestBit)
{
    std::memcpy (preallocated, other.preallocated, sizeof (preallocated));
}

BigInteger& BigInteger::operator= (BigInteger&& other) noexcept
{
    heapAllocation = static_cast<HeapBlock<uint32>&&> (other.heapAllocation);
    std::memcpy (preallocated, other.preallocated, sizeof (preallocated));
    allocatedSize = other.allocatedSize;
    highestBit = other.highestBit;
    return *this;
}
#endif

BigInteger::~BigInteger() noexcept
{
}

uint32* BigInteger::getValues() const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(heapAllocation != nullptr || allocatedSize <= numPreallocatedInts, nullptr);

    return heapAllocation != nullptr ? heapAllocation
                                     : (uint32*) preallocated;
}

uint32* BigInteger::ensureSize (const size_t numVals) noexcept
{
    if (numVals <= allocatedSize)
        return getValues();

    size_t oldSize = allocatedSize;
    allocatedSize = ((numVals + 2) * 3) / 2;

    if (heapAllocation == nullptr)
    {
        CARLA_SAFE_ASSERT_RETURN(heapAllocation.calloc (allocatedSize), nullptr);
        std::memcpy (heapAllocation, preallocated, sizeof (uint32) * numPreallocatedInts);
    }
    else
    {
        CARLA_SAFE_ASSERT_RETURN(heapAllocation.realloc (allocatedSize), nullptr);

        for (uint32* values = heapAllocation; oldSize < allocatedSize; ++oldSize)
            values[oldSize] = 0;
    }

    return heapAllocation;
}

//==============================================================================
bool BigInteger::operator[] (const int bit) const noexcept
{
    if (bit > highestBit || bit < 0)
        return false;

    if (const uint32* const values = getValues())
        return (values [bitToIndex (bit)] & bitToMask (bit)) != 0;

    return false;
}

//==============================================================================
void BigInteger::clear() noexcept
{
    heapAllocation.free();
    allocatedSize = numPreallocatedInts;
    highestBit = -1;

    for (int i = 0; i < numPreallocatedInts; ++i)
        preallocated[i] = 0;
}

bool BigInteger::setBit (const int bit) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(bit >= 0, false);

    if (bit > highestBit)
    {
        if (ensureSize (sizeNeededToHold (bit)) == nullptr)
            return false;

        highestBit = bit;
    }

    if (uint32* const values = getValues())
        return values [bitToIndex (bit)] |= bitToMask (bit);

    return false;
}

bool BigInteger::setBit (const int bit, const bool shouldBeSet) noexcept
{
    if (shouldBeSet)
    {
        return setBit (bit);
    }
    else
    {
        clearBit (bit);
        return true;
    }
}

bool BigInteger::clearBit (const int bit) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(bit <= highestBit && bit >= 0, false);

    uint32* const values = getValues();
    CARLA_SAFE_ASSERT_RETURN(values != nullptr, false);

    values [bitToIndex (bit)] &= ~bitToMask (bit);

    if (bit == highestBit)
        highestBit = getHighestBit();

    return true;
}

int BigInteger::getHighestBit() const noexcept
{
    const uint32* values = getValues();
    CARLA_SAFE_ASSERT_RETURN(values != nullptr, -1);

    for (int i = (int) bitToIndex (highestBit); i >= 0; --i)
        if (uint32 n = values[i])
            return findHighestSetBit (n) + (i << 5);

    return -1;
}

}
