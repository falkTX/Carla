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

#include "juce_audio_graph.h"

// #include "AppConfig.h"

// #define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1

// #include "juce_audio_processors.h"
// #include <juce_gui_extra/juce_gui_extra.h>

#include <locale>
#include <cctype>
#include <cstdarg>
#include <iostream>

// #include <wctype.h>

//==============================================================================
namespace juce
{

#include "memory/juce_MemoryBlock.cpp"

#include "text/juce_CharacterFunctions.cpp"
#include "text/juce_String.cpp"
// #include "streams/juce_InputStream.cpp"
// #include "streams/juce_OutputStream.cpp"

// static inline bool arrayContainsPlugin (const OwnedArray<PluginDescription>& list,
//                                         const PluginDescription& desc)
// {
//     for (int i = list.size(); --i >= 0;)
//         if (list.getUnchecked(i)->isDuplicateOf (desc))
//             return true;
//
//     return false;
// }

#include "midi/juce_MidiBuffer.cpp"
#include "midi/juce_MidiMessage.cpp"

// #include "format/juce_AudioPluginFormat.cpp"
// #include "format/juce_AudioPluginFormatManager.cpp"
#include "processors/juce_AudioProcessor.cpp"
#include "processors/juce_AudioProcessorGraph.cpp"
// #include "processors/juce_GenericAudioProcessorEditor.cpp"
// #include "processors/juce_PluginDescription.cpp"
// #include "format_types/juce_LADSPAPluginFormat.cpp"
// #include "format_types/juce_VSTPluginFormat.cpp"
// #include "format_types/juce_VST3PluginFormat.cpp"
// #include "format_types/juce_AudioUnitPluginFormat.mm"
// #include "scanning/juce_KnownPluginList.cpp"
// #include "scanning/juce_PluginDirectoryScanner.cpp"
// #include "scanning/juce_PluginListComponent.cpp"
// #include "utilities/juce_AudioProcessorValueTreeState.cpp"
// #include "utilities/juce_AudioProcessorParameters.cpp"

}
