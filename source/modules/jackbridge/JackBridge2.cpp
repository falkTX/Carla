/*
 * JackBridge (Part 2, Semaphore functions)
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "CarlaDefines.h"

#include "JackBridge.hpp"
#include "CarlaShmUtils.hpp"

#ifndef JACKBRIDGE_HPP_INCLUDED
// don't include the whole JACK API in this file
CARLA_EXPORT bool jackbridge_sem_init(void* sem) noexcept;
CARLA_EXPORT bool jackbridge_sem_destroy(void* sem) noexcept;
CARLA_EXPORT bool jackbridge_sem_post(void* sem) noexcept;
CARLA_EXPORT bool jackbridge_sem_timedwait(void* sem, int secs) noexcept;

CARLA_EXPORT bool  jackbridge_shm_is_valid(const void* shm) noexcept;
CARLA_EXPORT void  jackbridge_shm_init(void* shm) noexcept;
CARLA_EXPORT void  jackbridge_shm_attach(void* shm, const char* name) noexcept;
CARLA_EXPORT void  jackbridge_shm_close(void* shm) noexcept;
CARLA_EXPORT void* jackbridge_shm_map(void* shm, size_t size) noexcept;
#endif

// -----------------------------------------------------------------------------

#ifdef JACKBRIDGE_DUMMY
bool jackbridge_sem_init(void*) noexcept
{
    return false;
}

bool jackbridge_sem_destroy(void*) noexcept
{
    return false;
}

bool jackbridge_sem_post(void*) noexcept
{
    return false;
}

bool jackbridge_sem_timedwait(void*, int) noexcept
{
    return false;
}

bool jackbridge_shm_is_valid(const void*) noexcept
{
    return false;
}

void jackbridge_shm_init(void*) noexcept
{
}

void jackbridge_shm_attach(void*, const char*) noexcept
{
}

void jackbridge_shm_close(void*) noexcept
{
}

void* jackbridge_shm_map(void*, size_t) noexcept
{
    return nullptr;
}
#else //JACKBRIDGE_DUMMY

#include <cerrno>
#include <ctime>
#include <sys/time.h>
#include <sys/types.h>
#include <semaphore.h>

#ifdef CARLA_OS_MAC
extern "C" {
#include "osx_sem_timedwait.c"
}
#endif

// TODO - check noexcept on OSX

bool jackbridge_sem_init(void* sem) noexcept
{
#ifdef CARLA_OS_MAC
    static ulong sCounter = 0;
    ++sCounter;

    std::srand(static_cast<uint>(std::time(nullptr)));

    char strBuf[0xff+1];
    carla_zeroChar(strBuf, 0xff+1);
    std::sprintf(strBuf, "carla-sem-%i-%li", std::rand(), sCounter);

    sem_unlink(strBuf);

    sem_t* const sema = sem_open(strBuf, O_CREAT, O_RDWR, 0);
    std::memcpy(sem, &sema, sizeof(sem_t*));
    return (sema != nullptr && sema != SEM_FAILED);
#else
    return (sem_init((sem_t*)sem, 1, 0) == 0);
#endif
}

bool jackbridge_sem_destroy(void* sem) noexcept
{
#ifdef CARLA_OS_MAC
    return (sem_close(*(sem_t**)sem) == 0);
#else
    return (sem_destroy((sem_t*)sem) == 0);
#endif
}

bool jackbridge_sem_post(void* sem) noexcept
{
#ifdef CARLA_OS_MAC
    sem_t* const sema = *(sem_t**)sem;
#else
    sem_t* const sema = (sem_t*)sem;
#endif
    return (sem_post(sema) == 0);
}

bool jackbridge_sem_timedwait(void* sem, int secs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(secs > 0, false);
#ifdef CARLA_OS_MAC
    sem_t* const sema = *(sem_t**)sem;
#else
    sem_t* const sema = (sem_t*)sem;
#endif

    timespec timeout;

#ifdef CARLA_OS_LINUX
    clock_gettime(CLOCK_REALTIME, &timeout);
#else
    timeval now;
    gettimeofday(&now, nullptr);
    timeout.tv_sec  = now.tv_sec;
    timeout.tv_nsec = now.tv_usec * 1000;
#endif
    timeout.tv_sec += secs;

    try {
        return (sem_timedwait(sema, &timeout) == 0);
    } CARLA_SAFE_EXCEPTION_RETURN("sem_timedwait", false);
}

bool jackbridge_shm_is_valid(const void* shm) noexcept
{
    return carla_is_shm_valid(*(const shm_t*)shm);
}

void jackbridge_shm_init(void* shm) noexcept
{
    carla_shm_init(*(shm_t*)shm);
}

void jackbridge_shm_attach(void* shm, const char* name) noexcept
{
    *(shm_t*)shm = carla_shm_attach(name);
}

void jackbridge_shm_close(void* shm) noexcept
{
    carla_shm_close(*(shm_t*)shm);
}

void* jackbridge_shm_map(void* shm, size_t size) noexcept
{
    return carla_shm_map(*(shm_t*)shm, size);
}

#endif // ! JACKBRIDGE_DUMMY

// -----------------------------------------------------------------------------
