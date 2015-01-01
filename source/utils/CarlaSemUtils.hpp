/*
 * Carla semaphore utils
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

#ifndef CARLA_SEM_UTILS_HPP_INCLUDED
#define CARLA_SEM_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <ctime>

#ifdef CARLA_OS_WIN
struct sem_t { HANDLE handle; };
#else
# include <sys/time.h>
# include <sys/types.h>
# include <semaphore.h>
# ifdef CARLA_OS_MAC
#  include <fcntl.h>
extern "C" {
#  include "osx_sem_timedwait.c"
};
# endif
#endif

/*
 * Create a new semaphore.
 */
static inline
sem_t* carla_sem_create() noexcept
{
#if defined(CARLA_OS_WIN)
    sem_t* const sem = (sem_t*)std::malloc(sizeof(sem_t));
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, nullptr);

    SECURITY_ATTRIBUTES sa;
    carla_zeroStruct(sa);
    sa.nLength        = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    sem->handle = ::CreateSemaphore(&sa, 0, 1, nullptr);

    return sem;
#elif defined(CARLA_OS_MAC)
    static ulong sCounter = 0;
    ++sCounter;

    std::srand(static_cast<uint>(std::time(nullptr)));

    char strBuf[0xff+1];
    carla_zeroChar(strBuf, 0xff+1);
    std::snprintf(strBuf, 0xff, "carla-sem-%lu-%lu-%i", static_cast<ulong>(::getpid()), sCounter, std::rand());

    ::sem_unlink(strBuf);

    return ::sem_open(strBuf, O_CREAT, O_RDWR, 0);
#else
    sem_t sem;

    if (::sem_init(&sem, 1, 0) != 0)
        return nullptr;

    // can't return temporary variable, so allocate a new one
    if (sem_t* const sem2 = (sem_t*)std::malloc(sizeof(sem_t)))
    {
        std::memcpy(sem2, &sem, sizeof(sem_t));
        return sem2;
    }

    ::sem_destroy(&sem);
    return nullptr;
#endif
}

/*
 * Destroy a semaphore.
 */
static inline
void carla_sem_destroy(sem_t* const sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr,);

#if defined(CARLA_OS_WIN)
      ::CloseHandle(sem->handle);
      std::free(sem);
#elif defined(CARLA_OS_MAC)
    ::sem_close(sem);
#else
    // we can't call "sem_destroy(sem)" directly because it will free memory which we allocated during carla_sem_create()
    // so we create a temp variable, free our memory, and finally pass the temp variable to sem_destroy()

    // temp var
    sem_t sem2;
    std::memcpy(&sem2, sem, sizeof(sem_t));

    // destroy semaphore
    ::sem_destroy(&sem2);

    // free memory allocated in carla_sem_create()
    // FIXME
    //std::free(sem);
#endif
}

/*
 * Post semaphore (unlock).
 */
static inline
bool carla_sem_post(sem_t* const sem) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, false);

#ifdef CARLA_OS_WIN
    return (::ReleaseSemaphore(sem->handle, 1, nullptr) != FALSE);
#else
    return (::sem_post(sem) == 0);
#endif
}

/*
 * Wait for a semaphore (lock).
 */
static inline
bool carla_sem_timedwait(sem_t* const sem, const uint secs) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(sem != nullptr, false);
    CARLA_SAFE_ASSERT_RETURN(secs > 0, false);

#if defined(CARLA_OS_WIN)
    const DWORD result = ::WaitForSingleObject(sem->handle, secs*1000);

    switch (result)
    {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        errno = ETIMEDOUT;
        return false;
    default:
        errno = EINTR;
        return false;
    }
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
        return (::sem_timedwait(sem, &timeout) == 0);
    } CARLA_SAFE_EXCEPTION_RETURN("sem_timedwait", false);
#endif
}

// -----------------------------------------------------------------------

#endif // CARLA_SEM_UTILS_HPP_INCLUDED
