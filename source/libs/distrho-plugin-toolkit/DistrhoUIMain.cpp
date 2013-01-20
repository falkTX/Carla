/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "src/DistrhoUI.cpp"

#if defined(DISTRHO_PLUGIN_TARGET_JACK)
// nothing
#elif defined(DISTRHO_PLUGIN_TARGET_DSSI)
# include "src/DistrhoUIDSSI.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_LV2)
# include "src/DistrhoUILV2.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_VST)
// nothing
#endif

#ifdef DISTRHO_UI_QT4
# include "src/DistrhoUIQt4.cpp"
#else
# include "src/DistrhoUIOpenGL.cpp"
# include "src/DistrhoUIOpenGLExt.cpp"
#endif
