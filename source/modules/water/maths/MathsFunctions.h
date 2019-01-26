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

#ifndef WATER_MATHSFUNCTIONS_H_INCLUDED
#define WATER_MATHSFUNCTIONS_H_INCLUDED

#include "../water.h"

#include "CarlaUtils.hpp"

#include <algorithm>
#include <cmath>

namespace water {

//==============================================================================
/*
    This file sets up some handy mathematical typdefs and functions.
*/

//==============================================================================
// Some indispensable min/max functions

/** Returns the larger of two values. */
template <typename Type>
Type jmax (const Type a, const Type b)                                               { return (a < b) ? b : a; }

/** Returns the larger of three values. */
template <typename Type>
Type jmax (const Type a, const Type b, const Type c)                                 { return (a < b) ? ((b < c) ? c : b) : ((a < c) ? c : a); }

/** Returns the larger of four values. */
template <typename Type>
Type jmax (const Type a, const Type b, const Type c, const Type d)                   { return jmax (a, jmax (b, c, d)); }

/** Returns the smaller of two values. */
template <typename Type>
Type jmin (const Type a, const Type b)                                               { return (b < a) ? b : a; }

/** Returns the smaller of three values. */
template <typename Type>
Type jmin (const Type a, const Type b, const Type c)                                 { return (b < a) ? ((c < b) ? c : b) : ((c < a) ? c : a); }

/** Returns the smaller of four values. */
template <typename Type>
Type jmin (const Type a, const Type b, const Type c, const Type d)                   { return jmin (a, jmin (b, c, d)); }

/** Remaps a normalised value (between 0 and 1) to a target range.
    This effectively returns (targetRangeMin + value0To1 * (targetRangeMax - targetRangeMin)).
*/
template <typename Type>
Type jmap (Type value0To1, Type targetRangeMin, Type targetRangeMax)
{
    return targetRangeMin + value0To1 * (targetRangeMax - targetRangeMin);
}

/** Remaps a value from a source range to a target range. */
template <typename Type>
Type jmap (Type sourceValue, Type sourceRangeMin, Type sourceRangeMax, Type targetRangeMin, Type targetRangeMax)
{
    wassert (sourceRangeMax != sourceRangeMin); // mapping from a range of zero will produce NaN!
    return targetRangeMin + ((targetRangeMax - targetRangeMin) * (sourceValue - sourceRangeMin)) / (sourceRangeMax - sourceRangeMin);
}

/** Scans an array of values, returning the minimum value that it contains. */
template <typename Type>
Type findMinimum (const Type* data, int numValues)
{
    if (numValues <= 0)
        return Type();

    Type result (*data++);

    while (--numValues > 0) // (> 0 rather than >= 0 because we've already taken the first sample)
    {
        const Type& v = *data++;
        if (v < result)  result = v;
    }

    return result;
}

/** Scans an array of values, returning the maximum value that it contains. */
template <typename Type>
Type findMaximum (const Type* values, int numValues)
{
    if (numValues <= 0)
        return Type();

    Type result (*values++);

    while (--numValues > 0) // (> 0 rather than >= 0 because we've already taken the first sample)
    {
        const Type& v = *values++;
        if (result < v)  result = v;
    }

    return result;
}

/** Scans an array of values, returning the minimum and maximum values that it contains. */
template <typename Type>
void findMinAndMax (const Type* values, int numValues, Type& lowest, Type& highest)
{
    if (numValues <= 0)
    {
        lowest = Type();
        highest = Type();
    }
    else
    {
        Type mn (*values++);
        Type mx (mn);

        while (--numValues > 0) // (> 0 rather than >= 0 because we've already taken the first sample)
        {
            const Type& v = *values++;

            if (mx < v)  mx = v;
            if (v < mn)  mn = v;
        }

        lowest = mn;
        highest = mx;
    }
}


//==============================================================================
/** Constrains a value to keep it within a given range.

    This will check that the specified value lies between the lower and upper bounds
    specified, and if not, will return the nearest value that would be in-range. Effectively,
    it's like calling jmax (lowerLimit, jmin (upperLimit, value)).

    Note that it expects that lowerLimit <= upperLimit. If this isn't true,
    the results will be unpredictable.

    @param lowerLimit           the minimum value to return
    @param upperLimit           the maximum value to return
    @param valueToConstrain     the value to try to return
    @returns    the closest value to valueToConstrain which lies between lowerLimit
                and upperLimit (inclusive)
    @see jmin, jmax, jmap
*/
template <typename Type>
Type jlimit (const Type lowerLimit,
             const Type upperLimit,
             const Type valueToConstrain) noexcept
{
    // if these are in the wrong order, results are unpredictable..
    CARLA_SAFE_ASSERT_RETURN(lowerLimit <= upperLimit, lowerLimit);

    return (valueToConstrain < lowerLimit) ? lowerLimit
                                           : ((upperLimit < valueToConstrain) ? upperLimit
                                                                              : valueToConstrain);
}

/** Returns true if a value is at least zero, and also below a specified upper limit.
    This is basically a quicker way to write:
    @code valueToTest >= 0 && valueToTest < upperLimit
    @endcode
*/
template <typename Type>
bool isPositiveAndBelow (Type valueToTest, Type upperLimit) noexcept
{
    // makes no sense to call this if the upper limit is itself below zero..
    CARLA_SAFE_ASSERT_RETURN(Type() <= upperLimit, false);

    return Type() <= valueToTest && valueToTest < upperLimit;
}

template <>
inline bool isPositiveAndBelow (const int valueToTest, const int upperLimit) noexcept
{
    // makes no sense to call this if the upper limit is itself below zero..
    CARLA_SAFE_ASSERT_RETURN(upperLimit >= 0, false);

    return static_cast<unsigned int> (valueToTest) < static_cast<unsigned int> (upperLimit);
}

/** Returns true if a value is at least zero, and also less than or equal to a specified upper limit.
    This is basically a quicker way to write:
    @code valueToTest >= 0 && valueToTest <= upperLimit
    @endcode
*/
template <typename Type>
bool isPositiveAndNotGreaterThan (Type valueToTest, Type upperLimit) noexcept
{
    // makes no sense to call this if the upper limit is itself below zero..
    CARLA_SAFE_ASSERT_RETURN(Type() <= upperLimit, false);

    return Type() <= valueToTest && valueToTest <= upperLimit;
}

template <>
inline bool isPositiveAndNotGreaterThan (const int valueToTest, const int upperLimit) noexcept
{
    // makes no sense to call this if the upper limit is itself below zero..
    CARLA_SAFE_ASSERT_RETURN(upperLimit >= 0, false);

    return static_cast<unsigned int> (valueToTest) <= static_cast<unsigned int> (upperLimit);
}

//==============================================================================
/** Handy function to swap two values. */
template <typename Type>
void swapVariables (Type& variable1, Type& variable2)
{
    std::swap (variable1, variable2);
}

/** Handy function for avoiding unused variables warning. */
template <typename Type1>
void ignoreUnused (const Type1&) noexcept {}

template <typename Type1, typename Type2>
void ignoreUnused (const Type1&, const Type2&) noexcept {}

template <typename Type1, typename Type2, typename Type3>
void ignoreUnused (const Type1&, const Type2&, const Type3&) noexcept {}

template <typename Type1, typename Type2, typename Type3, typename Type4>
void ignoreUnused (const Type1&, const Type2&, const Type3&, const Type4&) noexcept {}

/** Handy function for getting the number of elements in a simple const C array.
    E.g.
    @code
    static size_t myArray[] = { 1, 2, 3 };

    size_t numElements = numElementsInArray (myArray) // returns 3
    @endcode
*/
template <typename Type, size_t N>
size_t numElementsInArray (Type (&array)[N])
{
    ignoreUnused (array);
    (void) sizeof (0[array]); // This line should cause an error if you pass an object with a user-defined subscript operator
    return N;
}

//==============================================================================
// Some useful maths functions that aren't always present with all compilers and build settings.

/** Using water_hypot is easier than dealing with the different types of hypot function
    that are provided by the various platforms and compilers. */
template <typename Type>
Type water_hypot (Type a, Type b) noexcept
{
    return static_cast<Type> (hypot (a, b));
}

template <>
inline float water_hypot (float a, float b) noexcept
{
    return hypotf (a, b);
}

/** 64-bit abs function. */
inline int64 abs64 (const int64 n) noexcept
{
    return (n >= 0) ? n : -n;
}

//==============================================================================
/** A predefined value for Pi, at double-precision.
    @see float_Pi
*/
const double  double_Pi  = 3.1415926535897932384626433832795;

/** A predefined value for Pi, at single-precision.
    @see double_Pi
*/
const float   float_Pi   = 3.14159265358979323846f;


/** Converts an angle in degrees to radians. */
inline float degreesToRadians (float degrees) noexcept     { return degrees * (float_Pi / 180.0f); }

/** Converts an angle in degrees to radians. */
inline double degreesToRadians (double degrees) noexcept   { return degrees * (double_Pi / 180.0); }

/** Converts an angle in radians to degrees. */
inline float radiansToDegrees (float radians) noexcept     { return radians * (180.0f / float_Pi); }

/** Converts an angle in radians to degrees. */
inline double radiansToDegrees (double radians) noexcept   { return radians * (180.0 / double_Pi); }


//==============================================================================
/** The isfinite() method seems to vary between platforms, so this is a
    platform-independent function for it.
*/
template <typename NumericType>
bool water_isfinite (NumericType) noexcept
{
    return true; // Integer types are always finite
}

template <>
inline bool water_isfinite (float value) noexcept
{
    return std::isfinite (value);
}

template <>
inline bool water_isfinite (double value) noexcept
{
    return std::isfinite (value);
}

//==============================================================================
/** Fast floating-point-to-integer conversion.

    This is faster than using the normal c++ cast to convert a float to an int, and
    it will round the value to the nearest integer, rather than rounding it down
    like the normal cast does.

    Note that this routine gets its speed at the expense of some accuracy, and when
    rounding values whose floating point component is exactly 0.5, odd numbers and
    even numbers will be rounded up or down differently.
*/
template <typename FloatType>
int roundToInt (const FloatType value) noexcept
{
    union { int asInt[2]; double asDouble; } n;
    n.asDouble = ((double) value) + 6755399441055744.0;

   #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return n.asInt [0];
   #else
    return n.asInt [1];
   #endif
}

inline int roundToInt (int value) noexcept
{
    return value;
}

/** Fast floating-point-to-integer conversion.

    This is a slightly slower and slightly more accurate version of roundDoubleToInt(). It works
    fine for values above zero, but negative numbers are rounded the wrong way.
*/
inline int roundToIntAccurate (double value) noexcept
{
    return roundToInt (value + 1.5e-8);
}

/** Fast floating-point-to-integer conversion.

    This is faster than using the normal c++ cast to convert a double to an int, and
    it will round the value to the nearest integer, rather than rounding it down
    like the normal cast does.

    Note that this routine gets its speed at the expense of some accuracy, and when
    rounding values whose floating point component is exactly 0.5, odd numbers and
    even numbers will be rounded up or down differently. For a more accurate conversion,
    see roundDoubleToIntAccurate().
*/
inline int roundDoubleToInt (double value) noexcept
{
    return roundToInt (value);
}

/** Fast floating-point-to-integer conversion.

    This is faster than using the normal c++ cast to convert a float to an int, and
    it will round the value to the nearest integer, rather than rounding it down
    like the normal cast does.

    Note that this routine gets its speed at the expense of some accuracy, and when
    rounding values whose floating point component is exactly 0.5, odd numbers and
    even numbers will be rounded up or down differently.
*/
inline int roundFloatToInt (float value) noexcept
{
    return roundToInt (value);
}

//==============================================================================
/** Returns true if the specified integer is a power-of-two. */
template <typename IntegerType>
bool isPowerOfTwo (IntegerType value)
{
   return (value & (value - 1)) == 0;
}

/** Returns the smallest power-of-two which is equal to or greater than the given integer. */
inline int nextPowerOfTwo (int n) noexcept
{
    --n;
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    return n + 1;
}

/** Returns the index of the highest set bit in a (non-zero) number.
    So for n=3 this would return 1, for n=7 it returns 2, etc.
    An input value of 0 is illegal!
*/
int findHighestSetBit (uint32 n) noexcept;

/** Returns the number of bits in a 32-bit integer. */
inline int countNumberOfBits (uint32 n) noexcept
{
    n -= ((n >> 1) & 0x55555555);
    n =  (((n >> 2) & 0x33333333) + (n & 0x33333333));
    n =  (((n >> 4) + n) & 0x0f0f0f0f);
    n += (n >> 8);
    n += (n >> 16);
    return (int) (n & 0x3f);
}

/** Returns the number of bits in a 64-bit integer. */
inline int countNumberOfBits (uint64 n) noexcept
{
    return countNumberOfBits ((uint32) n) + countNumberOfBits ((uint32) (n >> 32));
}

/** Performs a modulo operation, but can cope with the dividend being negative.
    The divisor must be greater than zero.
*/
template <typename IntegerType>
IntegerType negativeAwareModulo (IntegerType dividend, const IntegerType divisor) noexcept
{
    wassert (divisor > 0);
    dividend %= divisor;
    return (dividend < 0) ? (dividend + divisor) : dividend;
}

/** Returns the square of its argument. */
template <typename NumericType>
NumericType square (NumericType n) noexcept
{
    return n * n;
}

//==============================================================================
/** Writes a number of bits into a memory buffer at a given bit index.
    The buffer is treated as a sequence of 8-bit bytes, and the value is encoded in little-endian order,
    so for example if startBit = 10, and numBits = 11 then the lower 6 bits of the value would be written
    into bits 2-8 of targetBuffer[1], and the upper 5 bits of value into bits 0-5 of targetBuffer[2].

    @see readLittleEndianBitsInBuffer
*/
void writeLittleEndianBitsInBuffer (void* targetBuffer, uint32 startBit, uint32 numBits, uint32 value) noexcept;

/** Reads a number of bits from a buffer at a given bit index.
    The buffer is treated as a sequence of 8-bit bytes, and the value is encoded in little-endian order,
    so for example if startBit = 10, and numBits = 11 then the lower 6 bits of the result would be read
    from bits 2-8 of sourceBuffer[1], and the upper 5 bits of the result from bits 0-5 of sourceBuffer[2].

    @see writeLittleEndianBitsInBuffer
*/
uint32 readLittleEndianBitsInBuffer (const void* sourceBuffer, uint32 startBit, uint32 numBits) noexcept;

//==============================================================================

}

#endif // WATER_MATHSFUNCTIONS_H_INCLUDED
