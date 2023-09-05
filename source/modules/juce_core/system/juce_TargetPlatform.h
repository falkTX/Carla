/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

//==============================================================================
/*  This file figures out which platform is being built, and defines some macros
    that the rest of the code can use for OS-specific compilation.

    Macros that will be set here are:

    - One of JUCE_WINDOWS, JUCE_MAC JUCE_LINUX, JUCE_IOS, JUCE_ANDROID, etc.
    - Either JUCE_32BIT or JUCE_64BIT, depending on the architecture.
    - Either JUCE_LITTLE_ENDIAN or JUCE_BIG_ENDIAN.
    - Either JUCE_INTEL or JUCE_ARM
    - Either JUCE_GCC or JUCE_CLANG or JUCE_MSVC
    - Either JUCE_GLIBC or JUCE_MUSL will be defined on Linux depending on the system's libc implementation.
*/

//==============================================================================
#include "AppConfig.h"

//==============================================================================
#if defined (_WIN32) || defined (_WIN64)
  #define       JUCE_WINDOWS 1
#elif defined (JUCE_ANDROID)
  #undef        JUCE_ANDROID
  #define       JUCE_ANDROID 1
#elif defined (__FreeBSD__) || (__OpenBSD__)
  #define       JUCE_BSD 1
#elif defined (__wasm__)
  #define       JUCE_WASM 1
#elif defined (LINUX) || defined (__linux__)
  #define       JUCE_LINUX 1
#elif defined (__APPLE_CPP__) || defined (__APPLE_CC__)
  #define CF_EXCLUDE_CSTD_HEADERS 1
  #include <TargetConditionals.h> // (needed to find out what platform we're using)
  #include <AvailabilityMacros.h>

  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #define     JUCE_IPHONE 1
    #define     JUCE_IOS 1
  #else
    #define     JUCE_MAC 1
  #endif
#else
  #error "Unknown platform!"
#endif

//==============================================================================
#if JUCE_WINDOWS
  #ifdef _MSC_VER
    #ifdef _WIN64
      #define JUCE_64BIT 1
    #else
      #define JUCE_32BIT 1
    #endif
  #endif

  #ifdef _DEBUG
    #define JUCE_DEBUG 1
  #endif

  #ifdef __MINGW32__
    #define JUCE_MINGW 1
    #ifdef __MINGW64__
      #define JUCE_64BIT 1
    #else
      #define JUCE_32BIT 1
    #endif
  #endif

  /** If defined, this indicates that the processor is little-endian. */
  #define JUCE_LITTLE_ENDIAN 1

  #define JUCE_INTEL 1
#endif

//==============================================================================
#if JUCE_MAC || JUCE_IOS

  #if defined (DEBUG) || defined (_DEBUG) || ! (defined (NDEBUG) || defined (_NDEBUG))
    #define JUCE_DEBUG 1
  #endif

  #if ! (defined (DEBUG) || defined (_DEBUG) || defined (NDEBUG) || defined (_NDEBUG))
    #warning "Neither NDEBUG or DEBUG has been defined - you should set one of these to make it clear whether this is a release build,"
  #endif

  #ifdef __LITTLE_ENDIAN__
    #define JUCE_LITTLE_ENDIAN 1
  #else
    #define JUCE_BIG_ENDIAN 1
  #endif

  #ifdef __LP64__
    #define JUCE_64BIT 1
  #else
    #define JUCE_32BIT 1
  #endif

  #if defined (__ppc__) || defined (__ppc64__)
    #error "PowerPC is no longer supported by JUCE!"
  #elif defined (__arm__) || defined (__arm64__)
    #define JUCE_ARM 1
  #else
    #define JUCE_INTEL 1
  #endif

  #if JUCE_MAC
    #if ! defined (MAC_OS_X_VERSION_10_14)
      #error "The 10.14 SDK (Xcode 10.1+) is required to build JUCE apps. You can create apps that run on macOS 10.7+ by changing the deployment target."
    #elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_7
      #error "Building for OSX 10.6 is no longer supported!"
    #endif
  #endif
#endif

//==============================================================================
#if JUCE_LINUX || JUCE_ANDROID || JUCE_BSD

  #ifdef _DEBUG
    #define JUCE_DEBUG 1
  #endif

  // Allow override for big-endian Linux platforms
  #if defined (__LITTLE_ENDIAN__) || ! defined (JUCE_BIG_ENDIAN)
    #define JUCE_LITTLE_ENDIAN 1
    #undef JUCE_BIG_ENDIAN
  #else
    #undef JUCE_LITTLE_ENDIAN
    #define JUCE_BIG_ENDIAN 1
  #endif

  #if defined (__LP64__) || defined (_LP64) || defined (__arm64__)
    #define JUCE_64BIT 1
  #else
    #define JUCE_32BIT 1
  #endif

  #if defined (__arm__) || defined (__arm64__) || defined (__aarch64__)
    #define JUCE_ARM 1
  #elif __MMX__ || __SSE__ || __amd64__
    #define JUCE_INTEL 1
  #endif

  #if JUCE_LINUX
    #ifdef __GLIBC__
      #define JUCE_GLIBC 1
    #else
      #define JUCE_MUSL 1
    #endif
  #endif
#endif

//==============================================================================
// Compiler type macros.

#if defined (__clang__)
  #define JUCE_CLANG 1

#elif defined (__GNUC__)
  #define JUCE_GCC 1

#elif defined (_MSC_VER)
  #define JUCE_MSVC 1

#else
  #error unknown compiler
#endif
