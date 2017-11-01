/*
 * Standalone Juce AudioProcessorGraph
 * Copyright (C) 2015 ROLI Ltd.
 * Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaMathUtils.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaMutex.hpp"

#include <algorithm>

//==============================================================================

#define JUCE_API
#define JUCE_CALLTYPE

#define jassertfalse        carla_safe_assert("jassertfalse triggered", __FILE__, __LINE__);
#define jassert(expression) CARLA_SAFE_ASSERT(expression)

#define static_jassert(expression) static_assert(expression, #expression);

#if defined (__LP64__) || defined (_LP64) || defined (__arm64__) || defined(__MINGW64__)
  #define JUCE_64BIT 1
#else
  #define JUCE_32BIT 1
#endif

#define JUCE_ALIGN(bytes)   __attribute__ ((aligned (bytes)))

#if (__cplusplus >= 201103L || defined (__GXX_EXPERIMENTAL_CXX0X__)) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
 #define JUCE_COMPILER_SUPPORTS_NOEXCEPT 1
 #define JUCE_COMPILER_SUPPORTS_NULLPTR 1
 #define JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS 1
 #define JUCE_COMPILER_SUPPORTS_INITIALIZER_LISTS 1
 #define JUCE_COMPILER_SUPPORTS_VARIADIC_TEMPLATES 1

 #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407 && ! defined (JUCE_COMPILER_SUPPORTS_OVERRIDE_AND_FINAL)
  #define JUCE_COMPILER_SUPPORTS_OVERRIDE_AND_FINAL 1
 #endif

 #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407 && ! defined (JUCE_DELETED_FUNCTION)
  #define JUCE_DELETED_FUNCTION = delete
 #endif

 #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406 && ! defined (JUCE_COMPILER_SUPPORTS_LAMBDAS)
  #define JUCE_COMPILER_SUPPORTS_LAMBDAS 1
 #endif
#endif

#ifndef JUCE_DELETED_FUNCTION
 /** This macro can be placed after a method declaration to allow the use of
     the C++11 feature "= delete" on all compilers.
     On newer compilers that support it, it does the C++11 "= delete", but on
     older ones it's just an empty definition.
 */
 #define JUCE_DELETED_FUNCTION
#endif

#if ! JUCE_COMPILER_SUPPORTS_NOEXCEPT
 #ifdef noexcept
  #undef noexcept
 #endif
 #define noexcept  throw()
#endif

#if ! JUCE_COMPILER_SUPPORTS_NULLPTR
 #ifdef nullptr
  #undef nullptr
 #endif
 #define nullptr (0)
#endif

#if ! JUCE_COMPILER_SUPPORTS_OVERRIDE_AND_FINAL
 #undef  override
 #define override
#endif

#define JUCE_DECLARE_NON_COPYABLE(className) CARLA_DECLARE_NON_COPY_CLASS(className)
#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(className) \
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(className)
#define JUCE_LEAK_DETECTOR(className) CARLA_LEAK_DETECTOR(className)
#define JUCE_PREVENT_HEAP_ALLOCATION         CARLA_PREVENT_HEAP_ALLOCATION

#define NEEDS_TRANS(x) (x)

//==============================================================================
namespace water
{

class DynamicObject;
class File;
class FileInputStream;
class FileOutputStream;
class InputSource;
class InputStream;
class MidiMessage;
class MemoryBlock;
class OutputStream;
class Result;
class StringRef;
class XmlElement;

#include "memory/juce_Memory.h"
#include "maths/juce_MathsFunctions.h"
#include "memory/juce_ByteOrder.h"
#include "memory/juce_Atomic.h"
#include "text/juce_CharacterFunctions.h"
#include "text/juce_CharPointer_UTF8.h"

#include "text/juce_String.h"

#include "memory/juce_HeapBlock.h"
#include "containers/juce_ElementComparator.h"
#include "containers/juce_ArrayAllocationBase.h"
#include "containers/juce_Array.h"
#include "text/juce_StringArray.h"

#include "text/juce_StringRef.h"

#include "memory/juce_MemoryBlock.h"
#include "memory/juce_ReferenceCountedObject.h"
#include "text/juce_Identifier.h"
#include "text/juce_NewLine.h"

#include "threads/juce_ScopedLock.h"
#include "threads/juce_SpinLock.h"
#include "memory/juce_SharedResourcePointer.h"

#include "containers/juce_LinkedListPointer.h"
#include "containers/juce_OwnedArray.h"
#include "containers/juce_ReferenceCountedArray.h"
#include "containers/juce_SortedSet.h"
#include "containers/juce_Variant.h"
#include "containers/juce_NamedValueSet.h"

#include "streams/juce_InputSource.h"
#include "streams/juce_InputStream.h"
#include "streams/juce_OutputStream.h"
#include "streams/juce_MemoryOutputStream.h"

#include "maths/juce_Random.h"
#include "misc/juce_Result.h"

#include "text/juce_StringPool.h"

#include "time/juce_Time.h"

#include "files/juce_File.h"
#include "files/juce_DirectoryIterator.h"
#include "files/juce_TemporaryFile.h"
#include "streams/juce_FileInputStream.h"
#include "streams/juce_FileInputSource.h"
#include "streams/juce_FileOutputStream.h"

#include "buffers/juce_AudioSampleBuffer.h"
#include "midi/juce_MidiBuffer.h"
#include "midi/juce_MidiMessage.h"

class AudioProcessor;
#include "processors/juce_AudioPlayHead.h"
#include "processors/juce_AudioProcessor.h"
#include "processors/juce_AudioProcessorGraph.h"

#include "threads/juce_ChildProcess.h"
#include "threads/juce_Process.h"

#include "xml/juce_XmlElement.h"
#include "xml/juce_XmlDocument.h"

}

#endif // WATER_H_INCLUDED
