/*
 * Carla shared memory utils
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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
struct carla_shm_t { HANDLE map; bool isServer; const char* filename; };
# define carla_shm_t_INIT { INVALID_HANDLE_VALUE, true, nullptr }
#else
# ifndef __WINE__
#  include <cerrno>
# endif
# include <fcntl.h>
# include <sys/mman.h>
struct carla_shm_t { int fd; const char* filename; std::size_t size; };
# define carla_shm_t_INIT { -1, nullptr, 0 }
#endif

// -----------------------------------------------------------------------
// shared memory calls

/*
 * Null object returned when a shared memory operation fails.
 */
static const carla_shm_t gNullCarlaShm = carla_shm_t_INIT;

/*
 * Check if a shared memory object is valid.
 */
static inline
bool carla_is_shm_valid(const carla_shm_t& shm) noexcept
{
   #ifdef CARLA_OS_WIN
    return (shm.filename != nullptr);
   #else
    return (shm.fd >= 0);
   #endif
}

/*
 * Initialize a shared memory object to an invalid state.
 */
static inline
void carla_shm_init(carla_shm_t& shm) noexcept
{
    shm = gNullCarlaShm;
}

/*
 * Create and open a new shared memory object.
 * Returns an invalid object if the operation failed or the filename already exists.
 */
static inline
carla_shm_t carla_shm_create(const char* const filename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', gNullCarlaShm);

    carla_shm_t ret;

   #ifdef CARLA_OS_WIN
    ret.map      = INVALID_HANDLE_VALUE;
    ret.isServer = true;
    ret.filename = carla_strdup_safe(filename);
   #else
    try {
        ret.fd       = ::shm_open(filename, O_CREAT|O_EXCL|O_RDWR, 0600);
        ret.filename = (ret.fd >= 0) ? carla_strdup_safe(filename) : nullptr;
        ret.size     = 0;

        if (ret.fd >= 0 && ret.filename == nullptr)
        {
            ::close(ret.fd);
            ::shm_unlink(filename);
            ret.fd = -1;
        }
    } CARLA_SAFE_EXCEPTION_RETURN("carla_shm_create", gNullCarlaShm);
   #endif

    return ret;
}

/*
 * Attach to an existing shared memory object.
 */
static inline
carla_shm_t carla_shm_attach(const char* const filename) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', gNullCarlaShm);

    carla_shm_t ret;

   #ifdef CARLA_OS_WIN
    ret.map      = INVALID_HANDLE_VALUE;
    ret.isServer = false;
    ret.filename = carla_strdup_safe(filename);
   #else
    try {
        ret.fd       = ::shm_open(filename, O_RDWR, 0);
        ret.filename = nullptr;
        ret.size     = 0;
    } CARLA_SAFE_EXCEPTION_RETURN("carla_shm_attach", gNullCarlaShm);
   #endif

    return ret;
}

/*
 * Close a shared memory object and invalidate it.
 */
static inline
void carla_shm_close(carla_shm_t& shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm),);
   #ifdef CARLA_OS_WIN
    if (shm.isServer) {
        CARLA_SAFE_ASSERT(shm.map == INVALID_HANDLE_VALUE);
    }
   #endif

   #ifdef CARLA_OS_WIN
    if (shm.filename != nullptr)
        delete[] shm.filename;
   #else
    try {
        ::close(shm.fd);

        if (shm.filename != nullptr)
        {
            ::shm_unlink(shm.filename);
            delete[] shm.filename;
        }
    } CARLA_SAFE_EXCEPTION("carla_shm_close");
   #endif

    shm = gNullCarlaShm;
}

/*
 * Map a shared memory object to @a size bytes and return the memory address.
 * @note One shared memory object can only have one mapping at a time.
 */
static inline
void* carla_shm_map(carla_shm_t& shm, const std::size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm), nullptr);
    CARLA_SAFE_ASSERT_RETURN(size > 0, nullptr);
   #ifdef CARLA_OS_WIN
    CARLA_SAFE_ASSERT_RETURN(shm.map == INVALID_HANDLE_VALUE, nullptr);
   #else
    CARLA_SAFE_ASSERT_RETURN(shm.size == 0, nullptr);
   #endif

    try {
      #ifdef CARLA_OS_WIN
        HANDLE map;

        if (shm.isServer)
        {
            SECURITY_ATTRIBUTES sa;
            carla_zeroStruct(sa);
            sa.nLength = sizeof(sa);
            sa.bInheritHandle = TRUE;

            map = ::CreateFileMappingA(INVALID_HANDLE_VALUE, &sa,
                                       PAGE_READWRITE|SEC_COMMIT, 0, static_cast<DWORD>(size), shm.filename);

            if (map == nullptr || map == INVALID_HANDLE_VALUE)
            {
                const DWORD errorCode = ::GetLastError();
                carla_stderr2("CreateFileMapping failed for '%s', size:%lu, isServer:%i, errorCode:%x",
                              shm.filename, size, shm.isServer, errorCode);
                return nullptr;
            }
        }
        else
        {
            map = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm.filename);

            CARLA_SAFE_ASSERT_RETURN(map != nullptr, nullptr);
            CARLA_SAFE_ASSERT_RETURN(map != INVALID_HANDLE_VALUE, nullptr);
        }

        void* const ptr = ::MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, size);

        if (ptr == nullptr)
        {
            const DWORD errorCode = ::GetLastError();
            carla_stderr2("MapViewOfFile failed for '%s', size:%lu, isServer:%i, errorCode:%u",
                          shm.filename, size, shm.isServer, errorCode);
            ::CloseHandle(map);
            return nullptr;
        }

        shm.map = map;
        return ptr;
      #else
        if (shm.filename != nullptr)
        {
            const int ret(::ftruncate(shm.fd, static_cast<off_t>(size)));
            CARLA_SAFE_ASSERT_RETURN(ret == 0, nullptr);
        }

        void* ptr;

       #ifdef MAP_LOCKED
        ptr = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, shm.fd, 0);
        CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, nullptr);

        if (ptr == MAP_FAILED)
       #endif
        {
            ptr = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, shm.fd, 0);
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, nullptr);

            if (ptr == MAP_FAILED)
            {
                carla_stderr2("carla_shm_map() - mmap failed: %s", std::strerror(errno));
                return nullptr;
            }

           #ifndef MAP_LOCKED
            try {
                ::mlock(ptr, size);
            } CARLA_SAFE_EXCEPTION("carla_shm_map mlock");
           #endif
        }

        shm.size = size;
        return ptr;
      #endif
    } CARLA_SAFE_EXCEPTION_RETURN("carla_shm_map", nullptr);
}

/*
 * Unmap a shared memory object address.
 */
static inline
void carla_shm_unmap(carla_shm_t& shm, void* const ptr) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(carla_is_shm_valid(shm),);
    CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);
   #ifdef CARLA_OS_WIN
    CARLA_SAFE_ASSERT_RETURN(shm.map != INVALID_HANDLE_VALUE,);
   #else
    CARLA_SAFE_ASSERT_RETURN(shm.size > 0,);
   #endif

    try {
       #ifdef CARLA_OS_WIN
        const HANDLE map = shm.map;
        shm.map = INVALID_HANDLE_VALUE;

        ::UnmapViewOfFile(ptr);
        ::CloseHandle(map);
       #else
        const std::size_t size(shm.size);
        shm.size = 0;

        const int ret(::munmap(ptr, size));
        CARLA_SAFE_ASSERT(ret == 0);
       #endif
    } CARLA_SAFE_EXCEPTION("carla_shm_unmap");
}

#ifndef __WINE__
// -----------------------------------------------------------------------
// advanced calls

/*
 * Create and open a new shared memory object for a XXXXXX temp filename.
 * Will keep trying until a free random filename is obtained.
 */
static inline
carla_shm_t carla_shm_create_temp(char* const fileBase) noexcept
{
    // check if the fileBase name is valid
    CARLA_SAFE_ASSERT_RETURN(fileBase != nullptr, gNullCarlaShm);

    const std::size_t fileBaseLen(std::strlen(fileBase));

    CARLA_SAFE_ASSERT_RETURN(fileBaseLen > 6, gNullCarlaShm);
    CARLA_SAFE_ASSERT_RETURN(std::strcmp(fileBase + (fileBaseLen - 6), "XXXXXX") == 0, gNullCarlaShm);

    // character set to use randomly
    static const char charSet[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789";
    static const int charSetLen = static_cast<int>(std::strlen(charSet) - 1); // -1 to avoid trailing '\0'

    // try until getting a valid shm is obtained or an error occurs
    for (;;)
    {
        // fill the XXXXXX characters randomly
        for (std::size_t c = fileBaseLen - 6; c < fileBaseLen; ++c)
            fileBase[c] = charSet[std::rand() % charSetLen];

       #ifdef CARLA_OS_WIN
        // Windows: check if file already exists
        const HANDLE h = ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr,
                                              PAGE_READWRITE|SEC_COMMIT, 0, 8, fileBase);

        if (h == INVALID_HANDLE_VALUE)
        {
            carla_stderr("carla_shm_create_temp(%s) - file mapping test error", fileBase);
            return gNullCarlaShm;
        }

        const DWORD error = ::GetLastError();
        ::CloseHandle(h);

        if (error == ERROR_ALREADY_EXISTS)
        {
            carla_stderr("carla_shm_create_temp(%s) - file exists, retrying", fileBase);
            continue;
        }
       #endif

        // (try to) create new shm for this filename
        const carla_shm_t shm = carla_shm_create(fileBase);

        // all ok!
        if (carla_is_shm_valid(shm))
            return shm;

       #ifndef CARLA_OS_WIN
        // Non-Windows: if file already exists, keep trying
        if (errno == EEXIST)
        {
            carla_stderr("carla_shm_create_temp(%s) - file exists, retrying", fileBase);
            continue;
        }
        const int localerrno = errno;
        carla_stderr("carla_shm_create_temp(%s) - failed, error code %i", fileBase, localerrno);
       #endif

        // some unknown error occurred, return null
        return gNullCarlaShm;
    }
}

// -----------------------------------------------------------------------
// shared memory, templated calls

/*
 * Map a shared memory object, handling object type and size.
 */
template<typename T>
static inline
T* carla_shm_map(carla_shm_t& shm) noexcept
{
    return (T*)carla_shm_map(shm, sizeof(T));
}

/*
 * Map a shared memory object and return if it's non-null.
 */
template<typename T>
static inline
bool carla_shm_map(carla_shm_t& shm, T*& value) noexcept
{
    value = (T*)carla_shm_map(shm, sizeof(T));
    return (value != nullptr);
}

#endif // __WINE__

// -----------------------------------------------------------------------

#endif // CARLA_SHM_UTILS_HPP_INCLUDED
