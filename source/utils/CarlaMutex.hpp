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
        : fMutex(),
          fTryLockWasCalled(false)
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

private:
    mutable pthread_mutex_t fMutex;            // The mutex
    mutable volatile bool   fTryLockWasCalled; // true if "tryLock()" was called at least once

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaMutex)
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
        pthread_mutexattr_t atts;
        pthread_mutexattr_init(&atts);
        pthread_mutexattr_settype(&atts, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&fMutex, &atts);
        pthread_mutexattr_destroy(&atts);
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
    void lock() const noexcept
    {
#ifdef CARLA_OS_WIN
        EnterCriticalSection(&fSection);
#else
        pthread_mutex_lock(&fMutex);
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

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaRecursiveMutex)
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
    CARLA_DECLARE_NON_COPY_CLASS(CarlaScopeLocker)
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

    ~CarlaScopeTryLocker() noexcept
    {
        if (fLocked)
            fMutex.unlock();
    }

private:
    const Mutex& fMutex;
    const bool   fLocked;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaScopeTryLocker)
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
    CARLA_DECLARE_NON_COPY_CLASS(CarlaScopeUnlocker)
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
