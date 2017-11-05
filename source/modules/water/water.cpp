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

#include "water.h"

#include "CarlaMutex.hpp"

#include <locale>
#include <ctime>
#include <cctype>
#include <cstdarg>
#include <iostream>
#include <sys/time.h>

#ifdef CARLA_OS_WIN
 #include <mmsystem.h>
 #include <shlobj.h>
#else
 #include <dlfcn.h>
 #include <fcntl.h>
 #include <pwd.h>
 #include <signal.h>
 #include <sys/stat.h>
 #include <sys/wait.h>
 #ifdef CARLA_OS_MAC
 #else
  #include <dirent.h>
  #include <fnmatch.h>
 #endif
#endif

// #include <wctype.h>

#include "misc/Result.h"

//==============================================================================
namespace water
{

#ifdef CARLA_OS_WIN
static Result getResultForLastError()
{
    TCHAR messageBuffer [256] = { 0 };

    FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, GetLastError(), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                    messageBuffer, (DWORD) numElementsInArray (messageBuffer) - 1, nullptr);

    return Result::fail (String (messageBuffer));
}

static int64 juce_fileSetPosition (void* handle, int64 pos)
{
    LARGE_INTEGER li;
    li.QuadPart = pos;
    li.LowPart = SetFilePointer ((HANDLE) handle, (LONG) li.LowPart, &li.HighPart, FILE_BEGIN);  // (returns -1 if it fails)
    return li.QuadPart;
}

static void* currentModuleHandle = nullptr;

void* Process::getCurrentModuleInstanceHandle() noexcept
{
    if (currentModuleHandle == nullptr)
        currentModuleHandle = GetModuleHandleA (nullptr);

    return currentModuleHandle;
}

#else
static Result getResultForErrno()
{
    return Result::fail (String (strerror (errno)));
}

static int getFD (void* handle) noexcept        { return (int) (pointer_sized_int) handle; }
static void* fdToVoidPointer (int fd) noexcept  { return (void*) (pointer_sized_int) fd; }

static int64 juce_fileSetPosition (void* handle, int64 pos)
{
    if (handle != 0 && lseek (getFD (handle), pos, SEEK_SET) == pos)
        return pos;

    return -1;
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

#include "text/CharacterFunctions.cpp"
#include "text/Identifier.cpp"
#include "text/StringArray.cpp"
#include "text/StringPool.cpp"
#include "text/String.cpp"

#include "threads/ChildProcess.cpp"

#include "xml/XmlDocument.cpp"
#include "xml/XmlElement.cpp"
