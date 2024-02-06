/*
 * Carla VST3 utils
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_VST3_UTILS_HPP_INCLUDED
#define CARLA_VST3_UTILS_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaUtils.hpp"

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"
#include "travesty/host.h"

// --------------------------------------------------------------------------------------------------------------------

#if !(defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
 #if defined(__aarch64__) || defined(__arm64__)
  #define V3_ARCHITECTURE "aarch64"
 #elif defined(__TARGET_ARCH_ARM)
  #if __TARGET_ARCH_ARM == 9
   #define V3_ARCHITECTURE "armv9l"
  #elif __TARGET_ARCH_ARM == 8
   #define V3_ARCHITECTURE "armv8l"
  #elif __TARGET_ARCH_ARM == 7
   #define V3_ARCHITECTURE "armv7l"
  #elif _TARGET_ARCH_ARM == 6
   #define V3_ARCHITECTURE "armv6l"
  #elif __TARGET_ARCH_ARM == 5
   #define V3_ARCHITECTURE "armv5l"
  #else
   #define V3_ARCHITECTURE "arm"
  #endif
 #elif  defined(__arm__)
  #define V3_ARCHITECTURE "arm"
 #elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64)
  #define V3_ARCHITECTURE "x86_64"
 #elif defined(__i386) || defined(__i386__)
  #define V3_ARCHITECTURE "i386"
 #elif defined(__ia64) || defined(__ia64__)
  #define V3_ARCHITECTURE "ia64"
 #elif defined(__mips64)
  #define V3_ARCHITECTURE "mips64"
 #elif defined(__mips) || defined(__mips__)
  #define V3_ARCHITECTURE "mips"
 #elif defined(__ppc64) || defined(__ppc64__) || defined(__powerpc64__)
  #ifdef __LITTLE_ENDIAN__
   #define V3_ARCHITECTURE "ppc64le"
  #else
   #define V3_ARCHITECTURE "ppc64"
  #endif
 #elif defined(__ppc) || defined(__ppc__) || defined(__powerpc__)
  #define V3_ARCHITECTURE "ppc"
 #elif defined(__riscv)
  #if __riscv_xlen == 64
   #define V3_ARCHITECTURE "riscv64"
  #else
   #define V3_ARCHITECTURE "riscv"
  #endif
 #else
  #define V3_ARCHITECTURE "unknown"
 #endif
 #if defined(__HAIKU__)
  #define V3_PLATFORM "haiku"
 #elif defined(__linux__) || defined(__linux)
  #define V3_PLATFORM "linux"
 #elif defined(__FreeBSD__)
  #define V3_PLATFORM "freebsd"
 #elif defined(__NetBSD__)
  #define V3_PLATFORM "netbsd"
 #elif defined(__OpenBSD__)
  #define V3_PLATFORM "openbsd"
 #elif defined(__GNU__)
  #define V3_PLATFORM "hurd"
 #else
  #define V3_PLATFORM "unknown"
 #endif
#endif

#if defined(CARLA_OS_MAC)
 #define V3_CONTENT_DIR "MacOS"
#elif defined(_M_ARM64)
 #define V3_CONTENT_DIR "aarch64-win" /* TBD */
#elif defined(_M_ARM)
 #define V3_CONTENT_DIR "arm-win" /* TBD */
#elif defined(CARLA_OS_WIN64)
 #define V3_CONTENT_DIR "x86_64-win"
#elif defined(CARLA_OS_WIN32)
 #define V3_CONTENT_DIR "x86-win"
#else
 #define V3_CONTENT_DIR V3_ARCHITECTURE "-" V3_PLATFORM
#endif

#if defined(CARLA_OS_MAC)
# define V3_ENTRYFNNAME "bundleEntry"
# define V3_EXITFNNAME "bundleExit"
typedef void (*V3_ENTRYFN)(void*);
typedef void (*V3_EXITFN)(void);
#elif defined(CARLA_OS_WIN)
# define V3_ENTRYFNNAME "InitDll"
# define V3_EXITFNNAME "ExitDll"
typedef void (*V3_ENTRYFN)(void);
typedef void (*V3_EXITFN)(void);
#else
# define V3_ENTRYFNNAME "ModuleEntry"
# define V3_EXITFNNAME "ModuleExit"
typedef void (*V3_ENTRYFN)(void*);
typedef void (*V3_EXITFN)(void);
#endif

#define V3_GETFNNAME "GetPluginFactory"
typedef v3_plugin_factory** (*V3_GETFN)(void);

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

static inline
PluginCategory getPluginCategoryFromV3SubCategories(const char* const subcategories) noexcept
{
    if (std::strstr(subcategories, "Instrument") != nullptr)
        return PLUGIN_CATEGORY_SYNTH;

    return subcategories[0] != '\0' ? PLUGIN_CATEGORY_OTHER : PLUGIN_CATEGORY_NONE;
}

static inline constexpr
uint32_t v3_cconst(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) noexcept
{
    return static_cast<uint32_t>((a << 24) | (b << 16) | (c << 8) | (d << 0));
}

static inline
const char* tuid2str(const v3_tuid iid) noexcept
{
    static char buf[44];
    std::snprintf(buf, sizeof(buf), "0x%08X,0x%08X,0x%08X,0x%08X",
                  v3_cconst(iid[ 0], iid[ 1], iid[ 2], iid[ 3]),
                  v3_cconst(iid[ 4], iid[ 5], iid[ 6], iid[ 7]),
                  v3_cconst(iid[ 8], iid[ 9], iid[10], iid[11]),
                  v3_cconst(iid[12], iid[13], iid[14], iid[15]));
    return buf;
}

// --------------------------------------------------------------------------------------------------------------------

struct v3_bus_mini_info {
    uint16_t offset;
    int32_t bus_type;
    uint32_t flags;
};

// --------------------------------------------------------------------------------------------------------------------

template<class T>
T** v3_create_class_ptr()
{
    T** const clsptr = new T*;
    *clsptr = new T;
    return clsptr;
}

// --------------------------------------------------------------------------------------------------------------------

template<class v3_class, const v3_tuid cid>
static inline
v3_result V3_API v3_query_interface(void* const self, const v3_tuid iid, void** const iface)
{
    v3_class* const cls = *static_cast<v3_class**>(self);

    if (v3_tuid_match(iid, v3_funknown_iid) || v3_tuid_match(iid, cid))
    {
        ++cls->refcounter;
        *iface = self;
        return V3_OK;
    }

    *iface = nullptr;
    return V3_NO_INTERFACE;
}

template<class v3_class>
static inline
uint32_t V3_API v3_ref(void* const self)
{
    v3_class* const cls = *static_cast<v3_class**>(self);
    return ++cls->refcounter;
}

template<class v3_class>
static inline
uint32_t V3_API v3_unref(void* const self)
{
    v3_class** const clsptr = static_cast<v3_class**>(self);
    v3_class* const cls = *clsptr;

    if (const int refcount = --cls->refcounter)
        return refcount;

    delete cls;
    delete clsptr;
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

template<const v3_tuid cid>
static inline
v3_result V3_API v3_query_interface_static(void* const self, const v3_tuid iid, void** const iface)
{
    if (v3_tuid_match(iid, v3_funknown_iid) || v3_tuid_match(iid, cid))
    {
        *iface = self;
        return V3_OK;
    }

    *iface = nullptr;
    return V3_NO_INTERFACE;
}

static inline
uint32_t V3_API v3_ref_static(void*) { return 1; }

static inline
uint32_t V3_API v3_unref_static(void*) { return 0; }

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_VST3_UTILS_HPP_INCLUDED
