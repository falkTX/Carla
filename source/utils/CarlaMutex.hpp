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
// CarlaCriticalSection class

class CarlaCriticalSection
{
public:
    /*
     * Constructor.
     */
    CarlaCriticalSection() noexcept
    {
#ifdef CARLA_OS_WIN
        InitializeCriticalSection(&fSection);
#else
        fCounter = 0;
        fOwnerThread = 0;
        pthread_mutex_init(&fMutex, nullptr);
#endif
    }

    /*
     * Destructor.
     */
    ~CarlaCriticalSection() noexcept
    {
#ifdef CARLA_OS_WIN
        DeleteCriticalSection(&fSection);
#else
        pthread_mutex_destroy(&fMutex);
#endif
    }

    /*
     * Enter section.
     */
    void enter() noexcept
    {
#ifdef CARLA_OS_WIN
        EnterCriticalSection(&fSection);
#else
        const pthread_t thisThread(pthread_self());

        if (fOwnerThread == thisThread)
        {
            ++fCounter;
        }
        else
        {
            pthread_mutex_lock(&fMutex);
            fOwnerThread = thisThread;
            fCounter = 0;
        }
#endif
    }

    /*
     * Leave section.
     */
    void leave() noexcept
    {
#ifdef CARLA_OS_WIN
        LeaveCriticalSection(&fSection);
#else
        if (--fCounter < 0)
        {
            fOwnerThread = 0;
            pthread_mutex_unlock(&fMutex);
        }
#endif
    }

    /*
     * Helper class to enter&leave during a function scope.
     */
    class Scope
    {
    public:
        Scope(CarlaCriticalSection& cs) noexcept
            : fSection(cs)
        {
            fSection.enter();
        }

        ~Scope() noexcept
        {
            fSection.leave();
        }

    private:
        CarlaCriticalSection& fSection;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(Scope)
    };

private:
#ifdef CARLA_OS_WIN
    CRITICAL_SECTION fSection;
#else
    int             fCounter;
    pthread_t       fOwnerThread;
    pthread_mutex_t fMutex;
#endif

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaCriticalSection)
};

// -----------------------------------------------------------------------

#endif // CARLA_MUTEX_HPP_INCLUDED
