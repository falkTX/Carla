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

#include "maths/MathsFunctions.h"
#include "misc/Result.h"

//==============================================================================
namespace water
{

#ifdef CARLA_OS_WIN
static inline
Result getResultForLastError()
{
    TCHAR messageBuffer [256] = { 0 };

    FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, GetLastError(), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                    messageBuffer, (DWORD) numElementsInArray (messageBuffer) - 1, nullptr);

    return Result::fail (String (messageBuffer));
}

static inline
int64 water_fileSetPosition (void* handle, int64 pos)
{
    LARGE_INTEGER li;
    li.QuadPart = pos;
    li.LowPart = SetFilePointer ((HANDLE) handle, (LONG) li.LowPart, &li.HighPart, FILE_BEGIN);  // (returns -1 if it fails)
    return li.QuadPart;
}
#else
static inline
Result getResultForErrno()
{
    return Result::fail (String (strerror (errno)));
}

static inline
int getFD (void* handle) noexcept        { return (int) (pointer_sized_int) handle; }

static inline
void* fdToVoidPointer (int fd) noexcept  { return (void*) (pointer_sized_int) fd; }

static inline
int64 water_fileSetPosition (void* handle, int64 pos)
{
    if (handle != nullptr && lseek (getFD (handle), pos, SEEK_SET) == pos)
        return pos;

    return -1;
}
#endif

}
