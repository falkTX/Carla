// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaDefines.h"

#ifdef CARLA_OS_MAC

#include "CarlaMacUtils.hpp"

#include "extra/String.hpp"

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

    static String ret;
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

       #pragma clang diagnostic push
       #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        CFBundleCloseBundleResourceMap(ref, refNum);
       #pragma clang diagnostic pop

        if (CFGetRetainCount(ref) == 1)
            CFBundleUnloadExecutable(ref);

        if (CFGetRetainCount(ref) > 0)
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

       #pragma clang diagnostic push
       #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        refNum = CFBundleOpenBundleResourceMap(ref);
       #pragma clang diagnostic pop
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
