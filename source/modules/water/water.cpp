/*
 * Cross-platform C++ library for Carla, based on Juce v4
 * Copyright (C) 2015-2016 ROLI Ltd.
 * Copyright (C) 2017-2018 Filipe Coelho <falktx@falktx.com>
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

#include "common.hpp"

//==============================================================================
namespace water
{

#ifdef CARLA_OS_WIN
static HINSTANCE currentModuleHandle = nullptr;

HINSTANCE water_getCurrentModuleInstanceHandle() noexcept
{
    if (currentModuleHandle == nullptr)
        currentModuleHandle = GetModuleHandleA (nullptr);

    return currentModuleHandle;
}
#endif

}

#include "containers/NamedValueSet.cpp"
#include "containers/Variant.cpp"

#include "files/DirectoryIterator.cpp"
#include "files/File.cpp"
#include "files/FileInputStream.cpp"
#include "files/FileOutputStream.cpp"
#include "files/TemporaryFile.cpp"

#include "maths/BigInteger.cpp"

#include "maths/Random.cpp"

#include "memory/MemoryBlock.cpp"

#include "midi/MidiBuffer.cpp"
#include "midi/MidiFile.cpp"
#include "midi/MidiMessage.cpp"
#include "midi/MidiMessageSequence.cpp"

#include "misc/Result.cpp"
#include "misc/Time.cpp"

#include "processors/AudioProcessor.cpp"
#include "processors/AudioProcessorGraph.cpp"

#include "streams/FileInputSource.cpp"
#include "streams/InputStream.cpp"
#include "streams/MemoryOutputStream.cpp"
#include "streams/OutputStream.cpp"

#include "synthesisers/Synthesiser.cpp"

#include "text/CharacterFunctions.cpp"
#include "text/Identifier.cpp"
#include "text/StringArray.cpp"
#include "text/StringPool.cpp"
#include "text/String.cpp"

#include "threads/ChildProcess.cpp"

#include "xml/XmlDocument.cpp"
#include "xml/XmlElement.cpp"
