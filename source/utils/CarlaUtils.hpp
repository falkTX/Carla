/*
 * Carla common utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_UTILS_HPP_INCLUDED
#define CARLA_UTILS_HPP_INCLUDED

#include "CarlaDefines.hpp"

#include <cstring> // for memcpy and memset

// -----------------------------------------------------------------------
// misc functions

/*
 * Return "true" or "false" according to yesNo.
 */
inline
const char* bool2str(const bool yesNo) noexcept
{
    return yesNo ? "true" : "false";
}

/*
 * Dummy function.
 */
inline
void pass() noexcept {}

// -----------------------------------------------------------------------
// string print functions

/*
 * Print a string to stdout with newline (gray color).
 * Does nothing if DEBUG is not defined.
 */
#ifndef DEBUG
# define carla_debug(...)
#else
void carla_debug(const char* const fmt, ...);
#endif

/*
 * Print a string to stdout with newline.
 */
void carla_stdout(const char* const fmt, ...);

/*
 * Print a string to stderr with newline.
 */
void carla_stderr(const char* const fmt, ...);

/*
 * Print a string to stderr with newline (red color).
 */
void carla_stderr2(const char* const fmt, ...);

// -----------------------------------------------------------------------
// carla_safe_assert*

/*
 * Print a safe assertion error message.
 */
void carla_safe_assert(const char* const assertion, const char* const file, const int line)
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

/*
 * Print a safe assertion error message, with 1 extra integer value.
 */
void carla_safe_assert_int(const char* const assertion, const char* const file, const int line, const int value)
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, value %i", assertion, file, line, value);
}

/*
 * Print a safe assertion error message, with 2 extra integer values.
 */
void carla_safe_assert_int2(const char* const assertion, const char* const file, const int line, const int v1, const int v2)
{
    carla_stderr2("Carla assertion failure: \"%s\" in file %s, line %i, v1 %i, v2 %i", assertion, file, line, v1, v2);
}

// -----------------------------------------------------------------------
// carla_*sleep

/*
 * Sleep for 'secs' seconds.
 */
void carla_sleep(const unsigned int secs);

/*
 * Sleep for 'msecs' milliseconds.
 */
void carla_msleep(const unsigned int msecs);

// -----------------------------------------------------------------------
// carla_setenv

/*
 * Set environment variable 'key' to 'value'.
 */
void carla_setenv(const char* const key, const char* const value);

// -----------------------------------------------------------------------
// carla_strdup

/*
 * Custom 'strdup' function.
 * Return value is always valid, and must be freed with "delete[] var".
 */
const char* carla_strdup(const char* const strBuf);

/*
 * Custom 'strdup' function.
 * Calls "std::free(strBuf)".
 * Return value is always valid, and must be freed with "delete[] var".
 */
const char* carla_strdup_free(char* const strBuf);

// -----------------------------------------------------------------------
// math functions

/*
 * Return the lower of 2 values, with 'min' as the minimum possible value.
 */
template<typename T> inline
const T& carla_min(const T& v1, const T& v2, const T& min) noexcept
{
    return ((v1 <= min || v2 <= min) ? min : (v1 < v2 ? v1 : v2));
}

/*
 * Return the higher of 2 values, with 'max' as the maximum possible value.
 */
template<typename T> inline
const T& carla_max(const T& v1, const T& v2, const T& max) noexcept
{
    return ((v1 >= max || v2 >= max) ? max : (v1 > v2 ? v1 : v2));
}

/*
 * Fix bounds of 'value', between 'min' and 'max'.
 */
template<typename T> inline
const T& carla_fixValue(const T& min, const T& max, const T& value)
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
template<typename T> inline
void carla_add(T* dataDst, T* dataSrc, const size_t size)
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
template<typename T> inline
void carla_add(T* dataDst, const T* dataSrc, const size_t size)
{
    CARLA_SAFE_ASSERT_RETURN(dataDst != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(dataSrc != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);

    for (size_t i=0; i < size; ++i)
        *dataDst++ += *dataSrc++;
}

/*
 * Fill an array with a fixed value.
 */
template<typename T> inline
void carla_fill(T* data, const size_t size, const T v)
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);

    for (size_t i=0; i < size; ++i)
        *data++ = v;
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
// memory functions

/*
 * Clear a char array.
 */
template<typename C = char> inline
void carla_zeroChar(C* const data, const size_t numChars)
{
    CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(numChars > 0,);

    std::memset(data, 0, numChars*sizeof(C));
}

/*
 * Clear a memory location.
 */
inline
void carla_zeroMem(void* const memory, const size_t numBytes)
{
    CARLA_SAFE_ASSERT_RETURN(memory != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(numBytes > 0,);

    std::memset(memory, 0, numBytes);
}

/*
 * Clear a single struct/class.
 */
template <typename T> inline
void carla_zeroStruct(T& structure)
{
    std::memset(&structure, 0, sizeof(T));
}

/*
 * Clear an array of struct/class.
 */
template <typename T> inline
void carla_zeroStruct(T* const structure, const size_t count)
{
    CARLA_SAFE_ASSERT_RETURN(structure != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count > 0,);

    std::memset(structure, 0, count*sizeof(T));
}

// -----------------------------------------------------------------------

/*
 * Copy a single struct/class.
 */
template <typename T> inline
void carla_copyStruct(T& struct1, T& struct2)
{
    std::memcpy(&struct1, &struct2, sizeof(T));
}

/*
 * Copy an array of struct/class.
 */
template <typename T> inline
void carla_copyStruct(T* const struct1, T* const struct2, const size_t count)
{
    CARLA_SAFE_ASSERT_RETURN(struct1 != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(struct2 != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(count > 0,);

    std::memcpy(struct1, struct2, count*sizeof(T));
}

// -----------------------------------------------------------------------

#endif // CARLA_UTILS_HPP_INCLUDED
