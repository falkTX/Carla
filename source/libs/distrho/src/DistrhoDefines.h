/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DISTRHO_DEFINES_H__
#define __DISTRHO_DEFINES_H__

#include "DistrhoPluginInfo.h"

#ifndef DISTRHO_PLUGIN_NAME
# error DISTRHO_PLUGIN_NAME undefined!
#endif

#ifndef DISTRHO_PLUGIN_HAS_UI
# error DISTRHO_PLUGIN_HAS_UI undefined!
#endif

#ifndef DISTRHO_PLUGIN_IS_SYNTH
# error DISTRHO_PLUGIN_IS_SYNTH undefined!
#endif

#ifndef DISTRHO_PLUGIN_NUM_INPUTS
# error DISTRHO_PLUGIN_NUM_INPUTS undefined!
#endif

#ifndef DISTRHO_PLUGIN_NUM_OUTPUTS
# error DISTRHO_PLUGIN_NUM_OUTPUTS undefined!
#endif

#ifndef DISTRHO_PLUGIN_WANT_LATENCY
# error DISTRHO_PLUGIN_WANT_LATENCY undefined!
#endif

#ifndef DISTRHO_PLUGIN_WANT_PROGRAMS
# error DISTRHO_PLUGIN_WANT_PROGRAMS undefined!
#endif

#ifndef DISTRHO_PLUGIN_WANT_STATE
# error DISTRHO_PLUGIN_WANT_STATE undefined!
#endif

#if DISTRHO_PLUGIN_HAS_UI
# if ! (defined(DISTRHO_UI_EXTERNAL) || defined(DISTRHO_UI_OPENGL) || defined(DISTRHO_UI_QT))
#  error DISTRHO_PLUGIN_HAS_UI is defined but not its type; please define one of DISTRHO_UI_EXTERNAL, DISTRHO_UI_OPENGL or DISTRHO_UI_QT
# endif
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define DISTRHO_PLUGIN_EXPORT extern "C" __declspec (dllexport)
# define DISTRHO_OS_WINDOWS    1
# define DISTRHO_DLL_EXTENSION "dll"
#else
# define DISTRHO_PLUGIN_EXPORT extern "C" __attribute__ ((visibility("default")))
# if defined(__APPLE__)
#  define DISTRHO_OS_MAC        1
#  define DISTRHO_DLL_EXTENSION "dylib"
# elif defined(__HAIKU__)
#  define DISTRHO_OS_HAIKU      1
#  define DISTRHO_DLL_EXTENSION "so"
# elif defined(__linux__)
#  define DISTRHO_OS_LINUX      1
#  define DISTRHO_DLL_EXTENSION "so"
# endif
#endif

#ifndef DISTRHO_DLL_EXTENSION
# define DISTRHO_DLL_EXTENSION "so"
#endif

#if defined(HAVE_CPP11_SUPPORT)
# define PROPER_CPP11_SUPPORT
#elif defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__)
# if  (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
#  define PROPER_CPP11_SUPPORT
#  if  (__GNUC__ * 100 + __GNUC_MINOR__) < 407
#   define override // gcc4.7+ only
#  endif
# endif
#endif

#ifndef PROPER_CPP11_SUPPORT
# define override
# define noexcept
# define nullptr (0)
#endif

#ifndef DISTRHO_NO_NAMESPACE
# ifndef DISTRHO_NAMESPACE
#  define DISTRHO_NAMESPACE DISTRHO
# endif
# define START_NAMESPACE_DISTRHO namespace DISTRHO_NAMESPACE {
# define END_NAMESPACE_DISTRHO }
# define USE_NAMESPACE_DISTRHO using namespace DISTRHO_NAMESPACE;
#else
# define START_NAMESPACE_DISTRHO
# define END_NAMESPACE_DISTRHO
# define USE_NAMESPACE_DISTRHO
#endif

#define DISTRHO_UI_URI DISTRHO_PLUGIN_URI "#UI"

#endif // __DISTRHO_DEFINES_H__
