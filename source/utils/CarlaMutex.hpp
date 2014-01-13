/*
 * Carla Mutex
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

#ifndef CARLA_MUTEX_HPP_INCLUDED
#define CARLA_MUTEX_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <pthread.h>

// -----------------------------------------------------------------------
// CarlaMutex class

class CarlaMutex
{
public:
    /*
     * Constructor.
     */
    CarlaMutex() noexcept
        : fTryLockWasCalled(false)
    {
        pthread_mutex_init(&fMutex, nullptr);
    }

    /*
     * Destructor.
     */
    ~CarlaMutex() noexcept
    {
        pthread_mutex_destroy(&fMutex);
    }

    /*
     * Check if "tryLock()" was called before.
     */
    bool wasTryLockCalled() const noexcept
    {
        const bool ret(fTryLockWasCalled);
        fTryLockWasCalled = false;
        return ret;
    }

    /*
     * Lock the mutex.
     */
    void lock() const noexcept
    {
        pthread_mutex_lock(&fMutex);
    }

    /*
     * Try to lock the mutex.
     * Returns true if successful.
     */
    bool tryLock() const noexcept
    {
        fTryLockWasCalled = true;

        return (pthread_mutex_trylock(&fMutex) == 0);
    }

    /*
     * Unlock the mutex, optionally resetting the tryLock check.
     */
    void unlock(const bool resetTryLock = false) const noexcept
    {
        if (resetTryLock)
            fTryLockWasCalled = false;

        pthread_mutex_unlock(&fMutex);
    }

    /*
     * Helper class to lock&unlock a mutex during a function scope.
     */
    class ScopedLocker
    {
    public:
        ScopedLocker(CarlaMutex& mutex) noexcept
            : fMutex(mutex)
        {
            fMutex.lock();
        }

        ~ScopedLocker() noexcept
        {
            fMutex.unlock();
        }

    private:
        CarlaMutex& fMutex;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(ScopedLocker)
    };

    /*
     * Helper class to unlock&lock a mutex during a function scope.
     */
    class ScopedUnlocker
    {
    public:
        ScopedUnlocker(CarlaMutex& mutex) noexcept
            : fMutex(mutex)
        {
            fMutex.unlock();
        }

        ~ScopedUnlocker() noexcept
        {
            fMutex.lock();
        }

    private:
        CarlaMutex& fMutex;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(ScopedUnlocker)
    };

private:
    mutable pthread_mutex_t fMutex;            // The mutex
    mutable volatile bool   fTryLockWasCalled; // true if "tryLock()" was called at least once

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaMutex)
};

// -----------------------------------------------------------------------

#endif // CARLA_MUTEX_HPP_INCLUDED
