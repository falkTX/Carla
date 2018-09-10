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

#ifdef CARLA_OS_MAC
# include "text/String.h"
# import <Foundation/NSAutoreleasePool.h>
# import <Foundation/NSString.h>
#endif

#include <cerrno>

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

HINSTANCE getCurrentModuleInstanceHandle() noexcept;
#else
static inline
Result getResultForErrno()
{
#ifdef CARLA_PROPER_CPP11_SUPPORT
    return Result::fail (String (std::strerror (errno)));
#else
    return Result::fail (String (strerror (errno)));
#endif
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

#ifdef CARLA_OS_MAC
static inline
String nsStringToWater (NSString* s)
{
    return CharPointer_UTF8 ([s UTF8String]);
}

static inline
NSString* waterStringToNS (const String& s)
{
    return [NSString stringWithUTF8String: s.toUTF8()];
}

class AutoNSAutoreleasePool {
public:
    AutoNSAutoreleasePool() : pool([NSAutoreleasePool new]) {}
    ~AutoNSAutoreleasePool() { [pool drain]; }

private:
    NSAutoreleasePool* const pool;
};
#endif

}
