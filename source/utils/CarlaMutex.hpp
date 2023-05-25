/*
 * Carla Mutex
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

#ifndef CARLA_MUTEX_HPP_INCLUDED
#define CARLA_MUTEX_HPP_INCLUDED

#include "CarlaUtils.hpp"

#include <pthread.h>

class CarlaSignal;

// -----------------------------------------------------------------------
// CarlaMutex class

class CARLA_API CarlaMutex
{
public:
    /*
     * Constructor.
     */
    CarlaMutex(const bool inheritPriority = true) noexcept
        : fMutex(),
          fTryLockWasCalled(false)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
       #ifndef PTW32_DLLPORT
        pthread_mutexattr_setprotocol(&attr, inheritPriority ? PTHREAD_PRIO_INHERIT : PTHREAD_PRIO_NONE);
       #else
        // unsupported?
        (void)inheritPriority;
       #endif
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&fMutex, &attr);
        pthread_mutexattr_destroy(&attr);
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
    bool lock() const noexcept
    {
        return (pthread_mutex_lock(&fMutex) == 0);
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

private:
    mutable pthread_mutex_t fMutex;
    mutable volatile bool fTryLockWasCalled; // true if "tryLock()" was called at least once

    CARLA_DECLARE_NON_COPYABLE(CarlaMutex)
};

// -----------------------------------------------------------------------
// CarlaRecursiveMutex class

class CarlaRecursiveMutex
{
public:
    /*
     * Constructor.
     */
    CarlaRecursiveMutex() noexcept
       #ifdef CARLA_OS_WIN
        : fSection()
       #else
        : fMutex()
       #endif
    {
       #ifdef CARLA_OS_WIN
        InitializeCriticalSection(&fSection);
       #else
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&fMutex, &attr);
        pthread_mutexattr_destroy(&attr);
       #endif
    }

    /*
     * Destructor.
     */
    ~CarlaRecursiveMutex() noexcept
    {
       #ifdef CARLA_OS_WIN
        DeleteCriticalSection(&fSection);
       #else
        pthread_mutex_destroy(&fMutex);
       #endif
    }

    /*
     * Lock the mutex.
     */
    bool lock() const noexcept
    {
       #ifdef CARLA_OS_WIN
        EnterCriticalSection(&fSection);
        return true;
       #else
        return (pthread_mutex_lock(&fMutex) == 0);
       #endif
    }

    /*
     * Try to lock the mutex.
     * Returns true if successful.
     */
    bool tryLock() const noexcept
    {
       #ifdef CARLA_OS_WIN
        return (TryEnterCriticalSection(&fSection) != FALSE);
       #else
        return (pthread_mutex_trylock(&fMutex) == 0);
       #endif
    }

    /*
     * Unlock the mutex.
     */
    void unlock() const noexcept
    {
       #ifdef CARLA_OS_WIN
        LeaveCriticalSection(&fSection);
       #else
        pthread_mutex_unlock(&fMutex);
       #endif
    }

private:
   #ifdef CARLA_OS_WIN
    mutable CRITICAL_SECTION fSection;
   #else
    mutable pthread_mutex_t fMutex;
   #endif

    CARLA_DECLARE_NON_COPYABLE(CarlaRecursiveMutex)
};

// -----------------------------------------------------------------------
// CarlaSignal class

class CarlaSignal
{
public:
    /*
     * Constructor.
     */
    CarlaSignal() noexcept
        : fCondition(),
          fMutex(),
          fTriggered(false)
    {
        pthread_condattr_t cattr;
        pthread_condattr_init(&cattr);
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_PRIVATE);
        pthread_cond_init(&fCondition, &cattr);
        pthread_condattr_destroy(&cattr);

        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
       #ifndef PTW32_DLLPORT
        pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
       #endif
        pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&fMutex, &mattr);
        pthread_mutexattr_destroy(&mattr);
    }

    /*
     * Destructor.
     */
    ~CarlaSignal() noexcept
    {
        pthread_cond_destroy(&fCondition);
        pthread_mutex_destroy(&fMutex);
    }

    /*
     * Wait for a signal.
     */
    void wait() noexcept
    {
        pthread_mutex_lock(&fMutex);

        while (! fTriggered)
        {
            try {
                pthread_cond_wait(&fCondition, &fMutex);
            } CARLA_SAFE_EXCEPTION("pthread_cond_wait");
        }

        fTriggered = false;

        pthread_mutex_unlock(&fMutex);
    }

    /*
     * Wake up all waiting threads.
     */
    void signal() noexcept
    {
        pthread_mutex_lock(&fMutex);

        if (! fTriggered)
        {
            fTriggered = true;
            pthread_cond_broadcast(&fCondition);
        }

        pthread_mutex_unlock(&fMutex);
    }

private:
    pthread_cond_t  fCondition;
    pthread_mutex_t fMutex;
    volatile bool   fTriggered;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaSignal)
};

// -----------------------------------------------------------------------
// Helper class to lock&unlock a mutex during a function scope.

template <class Mutex>
class CarlaScopeLocker
{
public:
    CarlaScopeLocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.lock();
    }

    ~CarlaScopeLocker() noexcept
    {
        fMutex.unlock();
    }

private:
    const Mutex& fMutex;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaScopeLocker)
};

// -----------------------------------------------------------------------
// Helper class to try-lock&unlock a mutex during a function scope.

template <class Mutex>
class CarlaScopeTryLocker
{
public:
    CarlaScopeTryLocker(const Mutex& mutex) noexcept
        : fMutex(mutex),
          fLocked(mutex.tryLock()) {}

    CarlaScopeTryLocker(const Mutex& mutex, const bool forceLock) noexcept
        : fMutex(mutex),
          fLocked(forceLock ? mutex.lock() : mutex.tryLock()) {}

    ~CarlaScopeTryLocker() noexcept
    {
        if (fLocked)
            fMutex.unlock();
    }

    bool wasLocked() const noexcept
    {
        return fLocked;
    }

    bool wasNotLocked() const noexcept
    {
        return !fLocked;
    }

    bool tryAgain() const noexcept
    {
        return fMutex.tryLock();
    }

private:
    const Mutex& fMutex;
    const bool   fLocked;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaScopeTryLocker)
};

// -----------------------------------------------------------------------
// Helper class to unlock&lock a mutex during a function scope.

template <class Mutex>
class CarlaScopeUnlocker
{
public:
    CarlaScopeUnlocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.unlock();
    }

    ~CarlaScopeUnlocker() noexcept
    {
        fMutex.lock();
    }

private:
    const Mutex& fMutex;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaScopeUnlocker)
};

// -----------------------------------------------------------------------
// Define types

typedef CarlaScopeLocker<CarlaMutex>          CarlaMutexLocker;
typedef CarlaScopeLocker<CarlaRecursiveMutex> CarlaRecursiveMutexLocker;

typedef CarlaScopeTryLocker<CarlaMutex>          CarlaMutexTryLocker;
typedef CarlaScopeTryLocker<CarlaRecursiveMutex> CarlaRecursiveMutexTryLocker;

typedef CarlaScopeUnlocker<CarlaMutex>          CarlaMutexUnlocker;
typedef CarlaScopeUnlocker<CarlaRecursiveMutex> CarlaRecursiveMutexUnlocker;

// -----------------------------------------------------------------------

#endif // CARLA_MUTEX_HPP_INCLUDED
