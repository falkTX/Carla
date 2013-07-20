/*
 * Carla shared memory utils
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_SHM_UTILS_HPP__
#define __CARLA_SHM_UTILS_HPP__

#include "CarlaUtils.hpp"

#ifdef CARLA_OS_WIN
struct shm_t { HANDLE shm; HANDLE map; };
#else
# include <fcntl.h>
# include <sys/mman.h>
typedef int shm_t;
#endif

// -----------------------------------------------------------------------
// shared memory calls

static inline
bool carla_is_shm_valid(const shm_t& shm)
{
#ifdef CARLA_OS_WIN
    return (shm.shm != nullptr && shm.shm != INVALID_HANDLE_VALUE);
#else
    return (shm >= 0);
#endif
}

static inline
void carla_shm_init(shm_t& shm)
{
#ifdef CARLA_OS_WIN
    shm.shm = nullptr;
    shm.map = nullptr;
#else
    shm = -1;
#endif
}

#ifdef CARLA_OS_WIN
static inline
shm_t carla_shm_create(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

    shm_t ret;
    ret.shm = nullptr; // TODO
    ret.map = nullptr;

    return ret;
}

static inline
shm_t carla_shm_attach(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

    shm_t ret;
    ret.shm = CreateFileA(name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ret.map = nullptr;

    return ret;
}

static inline
shm_t carla_shm_attach_linux(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

    char shmName[std::strlen(name)+10];
    std::strcpy(shmName, "/dev/shm/");
    std::strcat(shmName, name);

    shm_t ret;
    ret.shm = CreateFileA(shmName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ret.map = nullptr;

    return ret;
}
#else
static inline
shm_t carla_shm_create(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

    return shm_open(name, O_RDWR|O_CREAT|O_EXCL, 0600);
}

static inline
shm_t carla_shm_attach(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

    return shm_open(name, O_RDWR, 0);
}
#endif

static inline
void carla_shm_close(shm_t& shm)
{
    CARLA_ASSERT(carla_is_shm_valid(shm));

#ifdef CARLA_OS_WIN
    CARLA_ASSERT(shm.map == nullptr);

    CloseHandle(shm.shm);
    shm.shm = nullptr;
#else
    close(shm);
    shm = -1;
#endif
}

static inline
void* carla_shm_map(shm_t& shm, const size_t size)
{
    CARLA_ASSERT(carla_is_shm_valid(shm));
    CARLA_ASSERT(size > 0);

#ifdef CARLA_OS_WIN
    CARLA_ASSERT(shm.map == nullptr);

    HANDLE map = CreateFileMapping(shm.shm, NULL, PAGE_READWRITE, size, size, NULL);

    if (map == nullptr)
        return nullptr;

    HANDLE ptr = MapViewOfFile(map, FILE_MAP_COPY, 0, 0, size);

    if (ptr == nullptr)
    {
        CloseHandle(map);
        return nullptr;
    }

    shm.map = map;

    return ptr;
#else
    if (ftruncate(shm, size) != 0)
        return nullptr;

    return mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
#endif
}

static inline
void carla_shm_unmap(shm_t& shm, void* const ptr, const size_t size)
{
    CARLA_ASSERT(carla_is_shm_valid(shm));
    CARLA_ASSERT(ptr != nullptr);
    CARLA_ASSERT(size > 0);

#ifdef CARLA_OS_WIN
    CARLA_ASSERT(shm.map != nullptr);

    UnmapViewOfFile(ptr);
    CloseHandle(shm.map);
    shm.map = nullptr;
    return;

    // unused
    (void)size;
#else
    munmap(ptr, size);
    return;

    // unused
    (void)shm;
#endif
}

// -------------------------------------------------
// shared memory, templated calls

template<typename T>
static inline
bool carla_shm_map(shm_t& shm, T*& value)
{
    value = (T*)carla_shm_map(shm, sizeof(T));
    return (value != nullptr);
}

template<typename T>
static inline
void carla_shm_unmap(shm_t& shm, T*& value)
{
    carla_shm_unmap(shm, value, sizeof(T));
    value = nullptr;
}

// -------------------------------------------------

#endif // __CARLA_SHM_UTILS_HPP__
