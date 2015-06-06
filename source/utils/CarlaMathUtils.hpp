/*
 * Carla math utils
 * Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_MATH_UTILS_HPP_INCLUDED
#define CARLA_MATH_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <cmath>
#include <limits>

// --------------------------------------------------------------------------------------------------------------------
// math functions (base)

/*
 * Return the lower of 2 values, with 'min' as the minimum possible value (ie, constrain).
 */
template<typename T>
static inline
const T& carla_minConstrained(const T& v1, const T& v2, const T& min) noexcept
{
    return (v1 <= min || v2 <= min) ? min : (v1 < v2 ? v1 : v2);
}

/*
 * Return the lower positive of 2 values.
 * If one of the values is zero, returns zero.
 * If one value is negative but the other positive, returns the positive.
 * Returned value is guaranteed to be >= 0.
 */
template<typename T>
static inline
T carla_minPositive(const T& v1, const T& v2) noexcept
{
    if (v1 == 0 || v2 == 0)
        return 0;
    if (v1 < 0)
        return (v2 > 0) ? v2 : 0;
    if (v2 < 0)
        return (v1 > 0) ? v1 : 0;
    return (v1 < v2) ? v1 : v2;
}

/*
 * Return the higher of 2 values, with 'max' as the maximum possible value (ie, limit).
 */
template<typename T>
static inline
const T& carla_maxLimited(const T& v1, const T& v2, const T& max) noexcept
{
    return (v1 >= max || v2 >= max) ? max : (v1 > v2 ? v1 : v2);
}

/*
 * Return the higher negative of 2 values.
 * If one of the values is zero, returns zero.
 * If one value is positive but the other negative, returns the negative.
 * Returned value is guaranteed to be <= 0.
 */
template<typename T>
static inline
T carla_maxNegative(const T& v1, const T& v2) noexcept
{
    if (v1 == 0 || v2 == 0)
        return 0;
    if (v1 > 0)
        return (v2 < 0) ? v2 : 0;
    if (v2 > 0)
        return (v1 < 0) ? v1 : 0;
    return (v1 > v2) ? v1 : v2;
}

/*
 * Fix bounds of 'value' between 'min' and 'max'.
 */
template<typename T>
static inline
const T& carla_fixedValue(const T& min, const T& max, const T& value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(max > min, max);

    if (value <= min)
        return min;
    if (value >= max)
        return max;
    return value;
}

/*
 * Get next power of 2.
 */
static inline
uint32_t carla_nextPowerOf2(uint32_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(size > 0, 0);

    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --size;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return ++size;
}

/*
 * Safely check if 2 numbers are equal.
 */
template<typename T>
static inline
bool carla_isEqual(const T& v1, const T& v2)
{
    return std::abs(v1-v2) < std::numeric_limits<T>::epsilon();
}

/*
 * Safely check if 2 numbers are not equal.
 */
template<typename T>
static inline
bool carla_isNotEqual(const T& v1, const T& v2)
{
    return std::abs(v1-v2) >= std::numeric_limits<T>::epsilon();
}

/*
 * Safely check if a number is zero.
 */
template<typename T>
static inline
bool carla_isZero(const T& value)
{
    return std::abs(value) < std::numeric_limits<T>::epsilon();
}

/*
 * Safely check if a number is not zero.
 */
template<typename T>
static inline
bool carla_isNotZero(const T& value)
{
    return std::abs(value) >= std::numeric_limits<T>::epsilon();
}

// --------------------------------------------------------------------------------------------------------------------
// math functions (extended)

/*
 * Add float array values to another float array.
 */
static inline
void carla_addFloats(float dest[], const float src[], const std::size_t count) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dest != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(src != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count > 0,);

    for (std::size_t i=0; i<count; ++i)
        *dest++ += *src++;
}

/*
 * Copy float array values to another float array.
 */
static inline
void carla_copyFloats(float dest[], const float src[], const std::size_t count) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dest != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(src != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count > 0,);

    std::memcpy(dest, src, count*sizeof(float));
}

/*
 * Clear a float array.
 */
static inline
void carla_zeroFloats(float floats[], const std::size_t count) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(floats != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count > 0,);

    std::memset(floats, 0, count*sizeof(float));
}

// --------------------------------------------------------------------------------------------------------------------
// Missing functions in OSX.

#if defined(CARLA_OS_MAC) && ! defined(DISTRHO_OS_MAC)
namespace std {
inline float fmin(float __x, float __y)
  { return __builtin_fminf(__x, __y); }
inline float fmax(float __x, float __y)
  { return __builtin_fmaxf(__x, __y); }
inline float rint(float __x)
  { return __builtin_rintf(__x); }
inline float round(float __x)
  { return __builtin_roundf(__x); }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_MATH_UTILS_HPP_INCLUDED
