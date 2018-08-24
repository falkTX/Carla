/*
 * Carla macOS utils
 * Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>
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

#ifdef CARLA_OS_MAC

#include "CarlaMacUtils.hpp"
#include "CarlaString.hpp"

#import <Foundation/Foundation.h>

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

const char* findBinaryInBundle(const char* const bundleDir)
{
    const CFURLRef urlRef = CFURLCreateFromFileSystemRepresentation(0, (const UInt8*)bundleDir, (CFIndex)strlen(bundleDir), true);
    CARLA_SAFE_ASSERT_RETURN(urlRef != nullptr, nullptr);

    const CFBundleRef bundleRef = CFBundleCreate(kCFAllocatorDefault, urlRef);
    CARLA_SAFE_ASSERT_RETURN(bundleRef != nullptr, nullptr);

    const CFURLRef exeRef = CFBundleCopyExecutableURL(bundleRef);
    CARLA_SAFE_ASSERT_RETURN(exeRef != nullptr, nullptr);

    const CFURLRef absoluteURL = CFURLCopyAbsoluteURL(exeRef);
    CARLA_SAFE_ASSERT_RETURN(absoluteURL != nullptr, nullptr);

    const NSString* strRef = (NSString*)CFURLCopyFileSystemPath(absoluteURL, nil);
    CARLA_SAFE_ASSERT_RETURN(strRef != nullptr, nullptr);

    static CarlaString ret;
    ret = [strRef UTF8String];

    CFRelease(absoluteURL);
    CFRelease(exeRef);
    CFRelease(bundleRef);
    CFRelease(urlRef);

    return ret.buffer();
}

// --------------------------------------------------------------------------------------------------------------------

AutoNSAutoreleasePool::AutoNSAutoreleasePool()
   : pool([NSAutoreleasePool new]) {}

AutoNSAutoreleasePool::~AutoNSAutoreleasePool()
{
    NSAutoreleasePool* rpool = (NSAutoreleasePool*)pool;
    [rpool drain];
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_OS_MAC
