/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_MUTEX_HPP_INCLUDED
#define DISTRHO_MUTEX_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
# include <winsock2.h>
# include <windows.h>
#endif

#include <pthread.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Mutex class

class Mutex
{
public:
    /*
     * Constructor.
     */
    Mutex() noexcept
    {
        pthread_mutex_init(&fMutex, nullptr);
    }

    /*
     * Destructor.
     */
    ~Mutex() noexcept
    {
        pthread_mutex_destroy(&fMutex);
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
        return (pthread_mutex_trylock(&fMutex) == 0);
    }

    /*
     * Unlock the mutex.
     */
    void unlock() const noexcept
    {
        pthread_mutex_unlock(&fMutex);
    }

private:
    mutable pthread_mutex_t fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(Mutex)
};

// -----------------------------------------------------------------------
// RecursiveMutex class

class RecursiveMutex
{
public:
    /*
     * Constructor.
     */
    RecursiveMutex() noexcept
    {
#ifdef DISTRHO_OS_WINDOWS
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
    ~RecursiveMutex() noexcept
    {
#ifdef DISTRHO_OS_WINDOWS
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
#ifdef DISTRHO_OS_WINDOWS
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
#ifdef DISTRHO_OS_WINDOWS
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
#ifdef DISTRHO_OS_WINDOWS
        LeaveCriticalSection(&fSection);
#else
        pthread_mutex_unlock(&fMutex);
#endif
    }

private:
#ifdef DISTRHO_OS_WINDOWS
    mutable CRITICAL_SECTION fSection;
#else
    mutable pthread_mutex_t fMutex;
#endif

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(RecursiveMutex)
};

// -----------------------------------------------------------------------
// Helper class to lock&unlock a mutex during a function scope.

template <class Mutex>
class ScopedLocker
{
public:
    ScopedLocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.lock();
    }

    ~ScopedLocker() noexcept
    {
        fMutex.unlock();
    }

private:
    const Mutex& fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(ScopedLocker)
};

// -----------------------------------------------------------------------
// Helper class to unlock&lock a mutex during a function scope.

template <class Mutex>
class ScopedUnlocker
{
public:
    ScopedUnlocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.unlock();
    }

    ~ScopedUnlocker() noexcept
    {
        fMutex.lock();
    }

private:
    const Mutex& fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(ScopedUnlocker)
};

// -----------------------------------------------------------------------
// Define types

typedef ScopedLocker<Mutex>          MutexLocker;
typedef ScopedLocker<RecursiveMutex> RecursiveMutexLocker;

typedef ScopedUnlocker<Mutex>          MutexUnlocker;
typedef ScopedUnlocker<RecursiveMutex> RecursiveMutexUnlocker;

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_MUTEX_HPP_INCLUDED
