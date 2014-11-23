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

#include "JackBridge.hpp"

#ifdef JACKBRIDGE_DUMMY
# include "CarlaUtils.hpp"
#else
# include <ctime>
# include <sys/time.h>
# include <sys/types.h>
# include <semaphore.h>
# ifdef CARLA_OS_MAC
extern "C" {
#  include "osx_sem_timedwait.c"
}
# endif
# include "CarlaShmUtils.hpp"
#endif // ! JACKBRIDGE_DUMMY

// -----------------------------------------------------------------------------

// TODO - check noexcept on OSX

bool jackbridge_sem_init(void* sem) noexcept
{
#if defined(JACKBRIDGE_DUMMY)
    return false;
#elif defined(CARLA_OS_MAC)
    static ulong sCounter = 0;
    ++sCounter;

    std::srand(static_cast<uint>(std::time(nullptr)));

    char strBuf[0xff+1];
    carla_zeroChar(strBuf, 0xff+1);
    std::sprintf(strBuf, "carla-sem-%i-%lu", std::rand(), sCounter);

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
#if defined(JACKBRIDGE_DUMMY)
    return false;
#elif defined(CARLA_OS_MAC)
    return (sem_close(*(sem_t**)sem) == 0);
#else
    return (sem_destroy((sem_t*)sem) == 0);
#endif
}

bool jackbridge_sem_post(void* sem) noexcept
{
#ifdef JACKBRIDGE_DUMMY
    return false;
#else
# ifdef CARLA_OS_MAC
    sem_t* const sema = *(sem_t**)sem;
# else
    sem_t* const sema = (sem_t*)sem;
# endif
    return (sem_post(sema) == 0);
#endif
}

bool jackbridge_sem_timedwait(void* sem, int secs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(secs > 0, false);

#ifdef JACKBRIDGE_DUMMY
    return false;
#else
# ifdef CARLA_OS_MAC
    sem_t* const sema = *(sem_t**)sem;
# else
    sem_t* const sema = (sem_t*)sem;
# endif

    timespec timeout;

# ifdef CARLA_OS_LINUX
    clock_gettime(CLOCK_REALTIME, &timeout);
# else
    timeval now;
    gettimeofday(&now, nullptr);
    timeout.tv_sec  = now.tv_sec;
    timeout.tv_nsec = now.tv_usec * 1000;
# endif
    timeout.tv_sec += secs;

    try {
        return (sem_timedwait(sema, &timeout) == 0);
    } CARLA_SAFE_EXCEPTION_RETURN("sem_timedwait", false);
#endif
}

bool jackbridge_shm_is_valid(const void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr, false);
#ifdef JACKBRIDGE_DUMMY
    return false;
#else
    return carla_is_shm_valid(*(const shm_t*)shm);
#endif
}

void jackbridge_shm_init(void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_shm_init(*(shm_t*)shm);
#endif
}

void jackbridge_shm_attach(void* shm, const char* name) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    *(shm_t*)shm = carla_shm_attach(name);
#endif
}

void jackbridge_shm_close(void* shm) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr,);

#ifndef JACKBRIDGE_DUMMY
    carla_shm_close(*(shm_t*)shm);
#endif
}

void* jackbridge_shm_map(void* shm, size_t size) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(shm != nullptr, nullptr);

#ifdef JACKBRIDGE_DUMMY
    return nullptr;
#else
    return carla_shm_map(*(shm_t*)shm, size);
#endif
}

// -----------------------------------------------------------------------------
