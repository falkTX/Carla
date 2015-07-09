/*
 * Carla semaphore utils
 * Copyright (C) 2013-2015 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_SEM_UTILS_HPP_INCLUDED
#define CARLA_SEM_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <ctime>

#ifdef CARLA_OS_WIN
struct carla_sem_t { HANDLE handle; };
#elif defined(CARLA_OS_MAC)
// TODO
struct carla_sem_t { char dummy; };
#else
# include <sys/time.h>
# include <sys/types.h>
# include <semaphore.h>
struct carla_sem_t { sem_t sem; };
#endif

/*
 * Create a new semaphore, pre-allocated version.
 */
static inline
bool carla_sem_create2(carla_sem_t& sem) noexcept
{
#if defined(CARLA_OS_WIN)
    SECURITY_ATTRIBUTES sa;
    carla_zeroStruct(sa);
    sa.nLength        = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    sem.handle = ::CreateSemaphore(&sa, 0, 1, nullptr);

    return (sem.handle != INVALID_HANDLE_VALUE);
#elif defined(CARLA_OS_MAC)
    return false; // TODO
#else
    return (::sem_init(&sem.sem, 1, 0) == 0);
#endif
}

/*
 * Create a new semaphore.
 */
static inline
carla_sem_t* carla_sem_create() noexcept
{
    if (carla_sem_t* const sem = (carla_sem_t*)std::malloc(sizeof(carla_sem_t)))
    {
        if (carla_sem_create2(*sem))
            return sem;

        std::free(sem);
    }

    return nullptr;
}

/*
 * Destroy a semaphore, pre-allocated version.
 */
static inline
void carla_sem_destroy2(carla_sem_t& sem) noexcept
{
#if defined(CARLA_OS_WIN)
    ::CloseHandle(sem.handle);
#elif defined(CARLA_OS_MAC)
    // TODO
#else
    ::sem_destroy(&sem.sem);
#endif
}

/*
 * Destroy a semaphore.
 */
static inline
void carla_sem_destroy(carla_sem_t* const sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr,);

    carla_sem_destroy2(*sem);
    std::free(sem);
}

/*
 * Post semaphore (unlock).
 */
static inline
void carla_sem_post(carla_sem_t& sem) noexcept
{
#ifdef CARLA_OS_WIN
    ::ReleaseSemaphore(sem.handle, 1, nullptr);
#elif defined(CARLA_OS_MAC)
    // TODO
#else
    ::sem_post(&sem.sem);
#endif
}

/*
 * Wait for a semaphore (lock).
 */
static inline
bool carla_sem_timedwait(carla_sem_t& sem, const uint secs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(secs > 0, false);

#if defined(CARLA_OS_WIN)
    return (::WaitForSingleObject(sem.handle, secs*1000) == WAIT_OBJECT_0);
#elif defined(CARLA_OS_MAC)
    // TODO
#else
    timespec timeout;
# ifdef CARLA_OS_LINUX
    ::clock_gettime(CLOCK_REALTIME, &timeout);
# else
    timeval now;
    ::gettimeofday(&now, nullptr);
    timeout.tv_sec  = now.tv_sec;
    timeout.tv_nsec = now.tv_usec * 1000;
# endif
    timeout.tv_sec += static_cast<time_t>(secs);

    try {
        return (::sem_timedwait(&sem.sem, &timeout) == 0);
    } CARLA_SAFE_EXCEPTION_RETURN("sem_timedwait", false);
#endif
}

// -----------------------------------------------------------------------

#endif // CARLA_SEM_UTILS_HPP_INCLUDED
