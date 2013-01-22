/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the license see the GPL.txt file
 */

#ifndef __DISTRHO_MACROS_H__
#define __DISTRHO_MACROS_H__

#include "DistrhoDefines.h"
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

#ifndef DISTRHO_UI_QT4
# define DISTRHO_UI_OPENGL
#endif

#define DISTRHO_UI_URI DISTRHO_PLUGIN_URI "#UI"

#endif // __DISTRHO_MACROS_H__
