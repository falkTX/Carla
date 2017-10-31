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

#include <locale>
#include <cctype>
#include <cstdarg>
#include <iostream>

// #include <wctype.h>

//==============================================================================
namespace juce2
{

#include "memory/juce_MemoryBlock.cpp"

#include "text/juce_CharacterFunctions.cpp"
#include "text/juce_String.cpp"
// #include "streams/juce_InputStream.cpp"
// #include "streams/juce_OutputStream.cpp"

#include "midi/juce_MidiBuffer.cpp"
#include "midi/juce_MidiMessage.cpp"

#include "processors/juce_AudioProcessor.cpp"
#include "processors/juce_AudioProcessorGraph.cpp"

#include "xml/juce_XmlElement.cpp"
#include "xml/juce_XmlDocument.cpp"

}
