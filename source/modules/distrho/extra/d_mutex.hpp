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

#include <pthread.h>

// -----------------------------------------------------------------------
// d_mutex class

class d_mutex
{
public:
    /*
     * Constructor.
     */
    d_mutex() noexcept
    {
        pthread_mutex_init(&fMutex, nullptr);
    }

    /*
     * Destructor.
     */
    ~d_mutex() noexcept
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
     * Unlock the mutex, optionally resetting the tryLock check.
     */
    void unlock() const noexcept
    {
        pthread_mutex_unlock(&fMutex);
    }

private:
    mutable pthread_mutex_t fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(d_mutex)
};

// -----------------------------------------------------------------------
// d_rmutex class

class d_rmutex
{
public:
    /*
     * Constructor.
     */
    d_rmutex() noexcept
    {
#ifdef DISTRHO_OS_WIN
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
    ~d_rmutex() noexcept
    {
#ifdef DISTRHO_OS_WIN
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
#ifdef DISTRHO_OS_WIN
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
#ifdef DISTRHO_OS_WIN
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
#ifdef DISTRHO_OS_WIN
        LeaveCriticalSection(&fSection);
#else
        pthread_mutex_unlock(&fMutex);
#endif
    }

private:
#ifdef DISTRHO_OS_WIN
    mutable CRITICAL_SECTION fSection;
#else
    mutable pthread_mutex_t fMutex;
#endif

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(d_rmutex)
};

// -----------------------------------------------------------------------
// Helper class to lock&unlock a mutex during a function scope.

template <class Mutex>
class d_scopeLocker
{
public:
    d_scopeLocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.lock();
    }

    ~d_scopeLocker() noexcept
    {
        fMutex.unlock();
    }

private:
    const Mutex& fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(d_scopeLocker)
};

// -----------------------------------------------------------------------
// Helper class to unlock&lock a mutex during a function scope.

template <class Mutex>
class d_scopeUnlocker
{
public:
    d_scopeUnlocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.unlock();
    }

    ~d_scopeUnlocker() noexcept
    {
        fMutex.lock();
    }

private:
    const Mutex& fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(d_scopeUnlocker)
};

// -----------------------------------------------------------------------
// Define types

typedef d_scopeLocker<d_mutex>  d_mutexLocker;
typedef d_scopeLocker<d_rmutex> d_rmutexLocker;

typedef d_scopeUnlocker<d_mutex>  d_mutexUnlocker;
typedef d_scopeUnlocker<d_rmutex> d_rmutexUnlocker;

// -----------------------------------------------------------------------

#endif // DISTRHO_MUTEX_HPP_INCLUDED
