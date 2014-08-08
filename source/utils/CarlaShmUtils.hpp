/*
 * Carla shared memory utils
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_SHM_UTILS_HPP_INCLUDED
#define CARLA_SHM_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#ifdef CARLA_OS_WIN
struct shm_t { HANDLE shm; HANDLE map; };
# define shm_t_INIT {nullptr, nullptr}
#else
# include <fcntl.h>
# include <sys/mman.h>
struct shm_t { int fd; const char* filename; };
# define shm_t_INIT {-1, nullptr}
#endif

// -----------------------------------------------------------------------
// shared memory calls

/*
 * Null object returned when a shared memory operation fails.
 */
#ifdef CARLA_OS_WIN
static const shm_t gNullCarlaShm = { nullptr, nullptr };
#else
static const shm_t gNullCarlaShm = { -1, nullptr };
#endif

/*
 * Check if a shared memory object is valid.
 */
static inline
bool carla_is_shm_valid(const shm_t& shm) noexcept
{
#ifdef CARLA_OS_WIN
    return (shm.shm != nullptr && shm.shm != INVALID_HANDLE_VALUE);
#else
    return (shm.fd >= 0);
#endif
}

/*
 * Initialize a shared memory object to an invalid state.
 */
static inline
void carla_shm_init(shm_t& shm) noexcept
{
#ifdef CARLA_OS_WIN
    shm.shm = nullptr;
    shm.map = nullptr;
#else
    shm.fd       = -1;
    shm.filename = nullptr;
#endif
}

/*
 * Create and open a new shared memory object.
 * Returns an invalid object if the operation failed or the filename already exists.
 */
static inline
shm_t carla_shm_create(const char* const filename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', gNullCarlaShm);

    shm_t ret;

    try {
#ifdef CARLA_OS_WIN
        ret.shm = nullptr; // TODO
        ret.map = nullptr;
#else
        ret.fd       = ::shm_open(filename, O_CREAT|O_EXCL|O_RDWR, 0600);
        ret.filename = (ret.fd >= 0) ? carla_strdup_safe(filename) : nullptr;

        if (ret.filename == nullptr)
        {
            ::close(ret.fd);
            ::shm_unlink(filename);
            ret.fd = -1;
        }
#endif
    }
    catch(...) {
        carla_safe_exception("carla_shm_create", __FILE__, __LINE__);
        ret = gNullCarlaShm;
    }

    return ret;
}

/*
 * Attach to an existing shared memory object.
 */
static inline
shm_t carla_shm_attach(const char* const filename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', gNullCarlaShm);

    shm_t ret;

    try {
#ifdef CARLA_OS_WIN
        ret.shm = ::CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        ret.map = nullptr;
#else
        ret.fd       = ::shm_open(filename, O_RDWR, 0);
        ret.filename = nullptr;
#endif
    }
    catch(...) {
        ret = gNullCarlaShm;
    }

    return ret;
}

/*
 * Close a shared memory object and invalidate it.
 */
static inline
void carla_shm_close(shm_t& shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm),);
#ifdef CARLA_OS_WIN
    CARLA_SAFE_ASSERT(shm.map == nullptr);
#endif

    try {
#ifdef CARLA_OS_WIN
        ::CloseHandle(shm.shm);
#else
        ::close(shm.fd);

        if (shm.filename != nullptr)
        {
            ::shm_unlink(shm.filename);
            delete[] shm.filename;
        }
#endif
    } CARLA_SAFE_EXCEPTION("carla_shm_close");

    shm = gNullCarlaShm;
}

/*
 * Map a shared memory object to @a size bytes and return the memory address.
 * @note One shared memory object can only have one mapping at a time.
 */
static inline
void* carla_shm_map(shm_t& shm, const std::size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm), nullptr);
    CARLA_SAFE_ASSERT_RETURN(size > 0, nullptr);
#ifdef CARLA_OS_WIN
    CARLA_SAFE_ASSERT_RETURN(shm.map == nullptr, nullptr);
#endif

    try {
#ifdef CARLA_OS_WIN
        const HANDLE map = ::CreateFileMapping(shm.shm, NULL, PAGE_READWRITE, size, size, NULL);
        CARLA_SAFE_ASSERT_RETURN(map != nullptr, nullptr);

        HANDLE ptr = ::MapViewOfFile(map, FILE_MAP_COPY, 0, 0, size);

        if (ptr == nullptr)
        {
            carla_safe_assert("ptr != nullptr", __FILE__, __LINE__);
            ::CloseHandle(map);
            return nullptr;
        }

        shm.map = map;
        return ptr;
#else
        if (shm.filename != nullptr)
        {
            const int ret = ::ftruncate(shm.fd, static_cast<off_t>(size));
            CARLA_SAFE_ASSERT_RETURN(ret == 0, nullptr);
        }

        return ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, shm.fd, 0);
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("carla_shm_map", nullptr);
}

/*
 * Unmap a shared memory object address.
 */
static inline
void carla_shm_unmap(shm_t& shm, void* const ptr, const std::size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm),);
    CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(size > 0,);
#ifdef CARLA_OS_WIN
    CARLA_SAFE_ASSERT_RETURN(shm.map != nullptr,);
#endif

    try {
#ifdef CARLA_OS_WIN
        const HANDLE map = shm.map;
        shm.map = nullptr;

        ::UnmapViewOfFile(ptr);
        ::CloseHandle(map);
#else
        const int ret = ::munmap(ptr, size);
        CARLA_SAFE_ASSERT(ret == 0);
#endif
    } CARLA_SAFE_EXCEPTION("carla_shm_unmap");

    // unused depending on platform
    return; (void)shm; (void)size;
}

// -----------------------------------------------------------------------
// shared memory, templated calls

/*
 * Map a shared memory object, handling object type and size.
 */
template<typename T>
static inline
T* carla_shm_map(shm_t& shm) noexcept
{
    return (T*)carla_shm_map(shm, sizeof(T));
}

/*
 * Map a shared memory object and return if it's non-null.
 */
template<typename T>
static inline
bool carla_shm_map(shm_t& shm, T*& value) noexcept
{
    value = (T*)carla_shm_map(shm, sizeof(T));
    return (value != nullptr);
}

/*
 * Unmap a shared memory object address and set it as null.
 */
template<typename T>
static inline
void carla_shm_unmap(shm_t& shm, T*& value) noexcept
{
    carla_shm_unmap(shm, value, sizeof(T));
    value = nullptr;
}

// -----------------------------------------------------------------------

#endif // CARLA_SHM_UTILS_HPP_INCLUDED
