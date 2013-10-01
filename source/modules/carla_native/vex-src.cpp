/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"

#include "juce_core.h"

using namespace juce;

// -----------------------------------------------------------------------

#include "vex/freeverb/allpass.cpp"
#include "vex/freeverb/comb.cpp"
#include "vex/freeverb/revmodel.cpp"

#include "vex/ResourceFile.cpp"
#include "vex/VexVoice.cpp"
#include "vex/VexWaveRenderer.cpp"

// -----------------------------------------------------------------------
