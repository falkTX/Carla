/*
 * Mini version of juce_audio_processors
 * Copyright (C) 2014-2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.h"

// -------------------------------------------------------------------------------------------------------------------

#if ! (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))

#undef KeyPress
#include "juce_audio_processors/juce_audio_processors.h"

#include "juce_audio_processors/processors/juce_AudioProcessor.cpp"
#include "juce_audio_processors/processors/juce_AudioProcessorGraph.cpp"
#include "juce_audio_processors/utilities/juce_AudioProcessorParameters.cpp"

#endif // ! CARLA_OS_MAC || CARLA_OS_WIN
