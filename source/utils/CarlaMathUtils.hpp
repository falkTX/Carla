/*
 * Carla math utils
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifdef HAVE_JUCE
# include "juce_audio_basics.h"
using juce::FloatVectorOperations;
#endif

// -----------------------------------------------------------------------
// Float operations

#ifdef HAVE_JUCE
# define FLOAT_ADD(bufDst, bufSrc, frames)  FloatVectorOperations::add(bufDst, bufSrc, static_cast<int>(frames))
# define FLOAT_COPY(bufDst, bufSrc, frames) FloatVectorOperations::copy(bufDst, bufSrc, static_cast<int>(frames))
# define FLOAT_CLEAR(buf, frames)           FloatVectorOperations::clear(buf, static_cast<int>(frames))
#else
# define FLOAT_ADD(bufDst, bufSrc, frames)  carla_addFloat(bufDst, bufSrc, frames)
# define FLOAT_COPY(bufDst, bufSrc, frames) carla_copyFloat(bufDst, bufSrc, frames)
# define FLOAT_CLEAR(buf, frames)           carla_zeroFloat(buf, frames)
#endif

// -----------------------------------------------------------------------
// math functions (base)

/*
 * Return the lower of 2 values, with 'min' as the minimum possible value.
 */
template<typename T>
static inline
const T& carla_min(const T& v1, const T& v2, const T& min) noexcept
{
    return ((v1 <= min || v2 <= min) ? min : (v1 < v2 ? v1 : v2));
}

/*
 * Return the higher of 2 values, with 'max' as the maximum possible value.
 */
template<typename T>
static inline
const T& carla_max(const T& v1, const T& v2, const T& max) noexcept
{
    return ((v1 >= max || v2 >= max) ? max : (v1 > v2 ? v1 : v2));
}

/*
 * Fix bounds of 'value', between 'min' and 'max'.
 */
template<typename T>
static inline
const T& carla_fixValue(const T& min, const T& max, const T& value) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(max > min, max);

    if (value <= min)
        return min;
    if (value >= max)
        return max;
    return value;
}

/*
 * Add array values to another array.
 */
template<typename T>
static inline
void carla_add(T* dataDst, T* dataSrc, const size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataDst != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSrc != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);

    for (size_t i=0; i < size; ++i)
        *dataDst++ += *dataSrc++;
}

/*
 * Add array values to another array.
 */
template<typename T>
static inline
void carla_add(T* dataDst, const T* dataSrc, const size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataDst != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSrc != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);

    for (size_t i=0; i < size; ++i)
        *dataDst++ += *dataSrc++;
}

/*
 * Copy array values to another array.
 */
template<typename T>
static inline
void carla_copy(T* dataDst, T* dataSrc, const size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataDst != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSrc != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);

    std::memcpy(dataDst, dataSrc, size*sizeof(T));
}

/*
 * Copy array values to another array.
 */
template<typename T>
static inline
void carla_copy(T* dataDst, const T* dataSrc, const size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataDst != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSrc != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);

    std::memcpy(dataDst, dataSrc, size*sizeof(T));
}

// -----------------------------------------------------------------------
// math functions (extended)

/*
 * Add float array values to another float array.
 */
static inline
void carla_addFloat(float* dataDst, float* dataSrc, const size_t numSamples) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataDst != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSrc != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(numSamples > 0,);

    for (size_t i=0; i < numSamples; ++i)
        *dataDst++ += *dataSrc++;
}

/*
 * Copy float array values to another float array.
 */
static inline
void carla_copyFloat(float* const dataDst, float* const dataSrc, const size_t numSamples) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(dataDst != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSrc != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(numSamples > 0,);

    std::memcpy(dataDst, dataSrc, numSamples*sizeof(float));
}

#if defined(CARLA_OS_MAC) && ! defined(DISTRHO_OS_MAC)
/*
 * Missing functions in OSX.
 */
namespace std {
inline float
  fmin(float __x, float __y)
  { return __builtin_fminf(__x, __y); }
inline float
  fmax(float __x, float __y)
  { return __builtin_fmaxf(__x, __y); }
inline float
  rint(float __x)
  { return __builtin_rintf(__x); }
}
#endif

// -----------------------------------------------------------------------

#endif // CARLA_MATH_UTILS_HPP_INCLUDED
