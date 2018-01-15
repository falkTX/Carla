/*
 * Cross-platform C++ library for Carla, based on Juce v4
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

#include "files/File.cpp"
#include "misc/Result.cpp"
#include "text/CharacterFunctions.cpp"
#include "text/StringArray.cpp"
#include "text/String.cpp"

#if defined(DEBUG) || defined(BUILDING_CARLA_FOR_WINDOWS)
# include "files/DirectoryIterator.cpp"
# include "files/FileInputStream.cpp"
# include "files/FileOutputStream.cpp"
# include "files/TemporaryFile.cpp"
# include "maths/Random.cpp"
# include "memory/MemoryBlock.cpp"
# include "misc/Time.cpp"
# include "streams/InputStream.cpp"
# include "streams/MemoryOutputStream.cpp"
# include "streams/OutputStream.cpp"
#endif
