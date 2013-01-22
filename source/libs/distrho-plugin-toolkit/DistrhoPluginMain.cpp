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

#include "src/DistrhoPlugin.cpp"

#if defined(DISTRHO_PLUGIN_TARGET_JACK)
# include "src/DistrhoPluginJACK.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_LADSPA) || defined(DISTRHO_PLUGIN_TARGET_DSSI)
# include "src/DistrhoPluginLADSPA+DSSI.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_LV2)
# include "src/DistrhoPluginLV2.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_VST)
# include "src/DistrhoPluginVST.cpp"
#endif
