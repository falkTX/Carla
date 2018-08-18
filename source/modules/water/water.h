/*
 * Cross-platform C++ library for Carla, based on Juce v4
 * Copyright (C) 2015-2016 ROLI Ltd.
 * Copyright (C) 2017-2018 Filipe Coelho <falktx@falktx.com>
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

#ifndef WATER_H_INCLUDED
#define WATER_H_INCLUDED

#include "CarlaDefines.h"

//==============================================================================

#define jassertfalse        carla_safe_assert("jassertfalse triggered", __FILE__, __LINE__);
#define jassert(expression) CARLA_SAFE_ASSERT(expression)

#define static_jassert(expression) static_assert(expression, #expression);

//==============================================================================
// Compiler support

#if (__cplusplus >= 201103L || defined (__GXX_EXPERIMENTAL_CXX0X__)) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
 #define WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS 1

 #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407 && ! defined (WATER_DELETED_FUNCTION)
  #define WATER_DELETED_FUNCTION = delete
 #endif
#endif

#ifdef __clang__
 #if __has_feature (cxx_rvalue_references)
  #define WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS 1
 #endif

 #if __has_feature (cxx_deleted_functions)
  #define WATER_DELETED_FUNCTION = delete
 #endif
#endif

//==============================================================================
// Declare some fake versions of nullptr and noexcept, for older compilers:

#ifndef WATER_DELETED_FUNCTION
 /** This macro can be placed after a method declaration to allow the use of
     the C++11 feature "= delete" on all compilers.
     On newer compilers that support it, it does the C++11 "= delete", but on
     older ones it's just an empty definition.
 */
 #define WATER_DELETED_FUNCTION
#endif

//==============================================================================
// FIXME

/* WATER_PACKED needs to be a macro which
    expands into a compiler directive. The directive must
    tell the compiler to arrange the preceding structure
    declaration so that it is packed on byte-boundaries rather
    than use the natural alignment of the processor and/or
    compiler.
*/
#if defined(__GNUC__)
 #define WATER_PACKED __attribute__((__packed__))
#elif defined(_MSC_VER)
 #define WATER_PACKED ;__pragma(pack(pop))
#else
 #define WATER_PACKED
#endif

//==============================================================================

namespace water
{

class AudioFormatManager;
class AudioFormatReader;
class AudioProcessor;
class AudioSampleBuffer;
class File;
class FileInputStream;
class FileInputSource;
class FileOutputStream;
class Identifier;
class InputStream;
class MidiBuffer;
class MidiMessage;
class MemoryBlock;
class MemoryOutputStream;
class NewLine;
class OutputStream;
class Result;
class String;
class StringArray;
class StringRef;
class Time;
class XmlElement;
class var;

//==============================================================================
// Definitions for the int8, int16, int32, int64 and pointer_sized_int types.

/** A platform-independent 8-bit signed integer type. */
typedef signed char                 int8;
/** A platform-independent 8-bit unsigned integer type. */
typedef unsigned char               uint8;
/** A platform-independent 16-bit signed integer type. */
typedef signed short                int16;
/** A platform-independent 16-bit unsigned integer type. */
typedef unsigned short              uint16;
/** A platform-independent 32-bit signed integer type. */
typedef signed int                  int32;
/** A platform-independent 32-bit unsigned integer type. */
typedef unsigned int                uint32;
/** A platform-independent 64-bit integer type. */
typedef long long                   int64;
/** A platform-independent 64-bit unsigned integer type. */
typedef unsigned long long          uint64;

#ifdef CARLA_OS_64BIT
  /** A signed integer type that's guaranteed to be large enough to hold a pointer without truncating it. */
  typedef int64                     pointer_sized_int;
  /** An unsigned integer type that's guaranteed to be large enough to hold a pointer without truncating it. */
  typedef uint64                    pointer_sized_uint;
#else
  /** A signed integer type that's guaranteed to be large enough to hold a pointer without truncating it. */
  typedef int                       pointer_sized_int;
  /** An unsigned integer type that's guaranteed to be large enough to hold a pointer without truncating it. */
  typedef unsigned int              pointer_sized_uint;
#endif

//==============================================================================
namespace NumberToStringConverters
{
    enum
    {
        charsNeededForInt = 32,
        charsNeededForDouble = 48
    };

    template <typename Type>
    static inline
    char* printDigits (char* t, Type v) noexcept
    {
        *--t = 0;

        do
        {
            *--t = '0' + (char) (v % 10);
            v /= 10;

        } while (v > 0);

        return t;
    }

    // pass in a pointer to the END of a buffer..
    static inline
    char* numberToString (char* t, const int64 n) noexcept
    {
        if (n >= 0)
            return printDigits (t, static_cast<uint64> (n));

        // NB: this needs to be careful not to call -std::numeric_limits<int64>::min(),
        // which has undefined behaviour
        t = printDigits (t, static_cast<uint64> (-(n + 1)) + 1);
        *--t = '-';
        return t;
    }
}

//==============================================================================
/** This namespace contains a few template classes for helping work out class type variations.
*/
namespace TypeHelpers
{
    /** The ParameterType struct is used to find the best type to use when passing some kind
        of object as a parameter.

        Of course, this is only likely to be useful in certain esoteric template situations.

        Because "typename TypeHelpers::ParameterType<SomeClass>::type" is a bit of a mouthful, there's
        a PARAMETER_TYPE(SomeClass) macro that you can use to get the same effect.

        E.g. "myFunction (PARAMETER_TYPE (int), PARAMETER_TYPE (MyObject))"
        would evaluate to "myfunction (int, const MyObject&)", keeping any primitive types as
        pass-by-value, but passing objects as a const reference, to avoid copying.
    */
    template <typename Type> struct ParameterType                   { typedef const Type& type; };
    template <typename Type> struct ParameterType <Type&>           { typedef Type& type; };
    template <typename Type> struct ParameterType <Type*>           { typedef Type* type; };
    template <>              struct ParameterType <char>            { typedef char type; };
    template <>              struct ParameterType <unsigned char>   { typedef unsigned char type; };
    template <>              struct ParameterType <short>           { typedef short type; };
    template <>              struct ParameterType <unsigned short>  { typedef unsigned short type; };
    template <>              struct ParameterType <int>             { typedef int type; };
    template <>              struct ParameterType <unsigned int>    { typedef unsigned int type; };
    template <>              struct ParameterType <long>            { typedef long type; };
    template <>              struct ParameterType <unsigned long>   { typedef unsigned long type; };
    template <>              struct ParameterType <int64>           { typedef int64 type; };
    template <>              struct ParameterType <uint64>          { typedef uint64 type; };
    template <>              struct ParameterType <bool>            { typedef bool type; };
    template <>              struct ParameterType <float>           { typedef float type; };
    template <>              struct ParameterType <double>          { typedef double type; };

    /** A helpful macro to simplify the use of the ParameterType template.
        @see ParameterType
    */
    #define PARAMETER_TYPE(a)    typename TypeHelpers::ParameterType<a>::type


    /** These templates are designed to take a type, and if it's a double, they return a double
        type; for anything else, they return a float type.
    */
    template <typename Type> struct SmallestFloatType             { typedef float  type; };
    template <>              struct SmallestFloatType <double>    { typedef double type; };
}

}

#endif // WATER_H_INCLUDED
