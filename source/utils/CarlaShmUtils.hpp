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
# include <vector>
# include <windows.h>
struct map_t { HANDLE map; void* ptr; };
struct shm_t { HANDLE shm; std::vector<map_t> maps; };
#else
# include <fcntl.h>
# include <sys/mman.h>
typedef int shm_t;
#endif

// -------------------------------------------------
// shared memory calls

static inline
bool carla_is_shm_valid(shm_t shm)
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
    shm.maps.clear();
#else
    shm = -1;
#endif
}

#ifdef CARLA_OS_WIN
static inline
shm_t carla_shm_attach_linux(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

    char shmName[std::strlen(name)+10];
    std::strcpy(shmName, "/dev/shm/");
    std::strcat(shmName, name);

    shm_t ret;
    ret.shm = CreateFileA(shmName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    return ret;
}
#else
static inline
shm_t carla_shm_create(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

    return shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
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
    CARLA_ASSERT(shm.maps.empty());

    CloseHandle(shm.shm);
    shm.shm = nullptr;
#else
    close(shm);
    shm = -1;
#endif
}

static inline
void* carla_shm_map(shm_t shm, const size_t size)
{
    CARLA_ASSERT(carla_is_shm_valid(shm));
    CARLA_ASSERT(size > 0);

#ifdef CARLA_OS_WIN
    HANDLE map = CreateFileMapping(shm.shm, NULL, PAGE_READWRITE, size, size, NULL);

    if (map == nullptr)
        return nullptr;

    HANDLE ptr = MapViewOfFile(map, FILE_MAP_COPY, 0, 0, size);

    if (ptr == nullptr)
        return nullptr;

    map_t newMap;
    newMap.map = map;
    newMap.ptr = ptr;

    shm.maps.push_back(newMap);

    return ptr;
#else
    return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
#endif
}

static inline
void carla_shm_unmap(const shm_t shm, void* const ptr, const size_t size)
{
    CARLA_ASSERT(carla_is_shm_valid(shm));
    CARLA_ASSERT(ptr != nullptr);
    CARLA_ASSERT(size > 0);

#ifdef CARLA_OS_WIN
    CARLA_ASSERT(! shm.maps.empty());

    for (auto it = shm.maps.begin(); it != shm.maps.end(); ++it)
    {
        map_t map(*it);

        if (map.ptr == ptr)
        {
            UnmapViewOfFile(ptr);
            CloseHandle(map.map);

            shm.maps.erase(it);
            break;
        }
    }

    CARLA_ASSERT(false);

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
void carla_shm_map(shm_t shm, T* value)
{
    value = carla_shm_map(shm, sizeof(value));
}

template<typename T>
static inline
void carla_shm_unmap(const shm_t shm, T* value)
{
    carla_shm_unmap(shm, value, sizeof(value));
    value = nullptr;
}

// -------------------------------------------------

#endif // __CARLA_SHM_UTILS_HPP__
