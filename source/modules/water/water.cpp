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

#include "memory/juce_MemoryBlock.cpp"

#include "text/juce_CharacterFunctions.cpp"
#include "text/juce_String.cpp"

#include "containers/juce_NamedValueSet.cpp"
#include "containers/juce_Variant.cpp"

#include "files/juce_DirectoryIterator.cpp"
#include "files/juce_File.cpp"
#include "files/juce_TemporaryFile.cpp"

#include "midi/juce_MidiBuffer.cpp"
#include "midi/juce_MidiMessage.cpp"
#include "midi/juce_MidiMessageSequence.cpp"
#include "midi/juce_MidiFile.cpp"

#include "maths/juce_Random.cpp"
#include "misc/juce_Result.cpp"

#include "processors/juce_AudioProcessor.cpp"
#include "processors/juce_AudioProcessorGraph.cpp"

#include "streams/juce_FileInputSource.cpp"
#include "streams/juce_FileInputStream.cpp"
#include "streams/juce_FileOutputStream.cpp"
#include "streams/juce_InputStream.cpp"
#include "streams/juce_MemoryOutputStream.cpp"
#include "streams/juce_OutputStream.cpp"

#include "text/juce_Identifier.cpp"
#include "text/juce_StringArray.cpp"
#include "text/juce_StringPool.cpp"
#include "threads/juce_ChildProcess.cpp"
#include "time/juce_Time.cpp"

#include "xml/juce_XmlElement.cpp"
#include "xml/juce_XmlDocument.cpp"
