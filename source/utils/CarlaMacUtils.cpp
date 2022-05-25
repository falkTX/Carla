/*
 * Carla macOS utils
 * Copyright (C) 2018-2022 Filipe Coelho <falktx@falktx.com>
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

#include <sys/xattr.h>

#ifdef __MAC_10_12
# define Component CocoaComponent
# define MemoryBlock CocoaMemoryBlock
# define Point CocoaPoint
#endif

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#undef Component
#undef MemoryBlock
#undef Point

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

void initStandaloneApplication()
{
    [[NSApplication sharedApplication] retain];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

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

    const NSString* strRef = (NSString*)CFURLCopyFileSystemPath(absoluteURL, kCFURLPOSIXPathStyle);
    CARLA_SAFE_ASSERT_RETURN(strRef != nullptr, nullptr);

    static CarlaString ret;
    ret = [strRef UTF8String];

    CFRelease(absoluteURL);
    CFRelease(exeRef);
    CFRelease(bundleRef);
    CFRelease(urlRef);

    return ret.buffer();
}

bool removeFileFromQuarantine(const char* const filename)
{
    return removexattr(filename, "com.apple.quarantine", 0) == 0;
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

struct BundleLoader::PrivateData {
    CFBundleRef ref;
    CFBundleRefNum refNum;

    PrivateData() noexcept
      : ref(nullptr),
        refNum(0) {}

    ~PrivateData()
    {
        if (ref == nullptr)
            return;

        CFBundleCloseBundleResourceMap(ref, refNum);
        CFBundleUnloadExecutable(ref);
        CFRelease(ref);
    }

    bool load(const char* const filename)
    {
        const CFURLRef urlRef = CFURLCreateFromFileSystemRepresentation(0, (const UInt8*)filename, (CFIndex)std::strlen(filename), true);
        CARLA_SAFE_ASSERT_RETURN(urlRef != nullptr, false);

        ref = CFBundleCreate(kCFAllocatorDefault, urlRef);
        CFRelease(urlRef);
        CARLA_SAFE_ASSERT_RETURN(ref != nullptr, false);

        if (! CFBundleLoadExecutable(ref))
        {
            CFRelease(ref);
            ref = nullptr;
            return false;
        }

        refNum = CFBundleOpenBundleResourceMap(ref);
        return true;
    }
};

BundleLoader::BundleLoader()
    : pData(new PrivateData){}

BundleLoader::~BundleLoader()
{
    delete pData;
}

bool BundleLoader::load(const char* const filename)
{
    return pData->load(filename);
}

CFBundleRef BundleLoader::getRef() const noexcept
{
    return pData->ref;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_OS_MAC
