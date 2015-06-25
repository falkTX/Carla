/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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
        : fMutex()
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
#ifdef DISTRHO_OS_WINDOWS
        : fSection()
#else
        : fMutex()
#endif
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
class ScopeLocker
{
public:
    ScopeLocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.lock();
    }

    ~ScopeLocker() noexcept
    {
        fMutex.unlock();
    }

private:
    const Mutex& fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(ScopeLocker)
};

// -----------------------------------------------------------------------
// Helper class to try-lock&unlock a mutex during a function scope.

template <class Mutex>
class ScopeTryLocker
{
public:
    ScopeTryLocker(const Mutex& mutex) noexcept
        : fMutex(mutex),
          fLocked(mutex.tryLock()) {}

    ~ScopeTryLocker() noexcept
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

private:
    const Mutex& fMutex;
    const bool   fLocked;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(ScopeTryLocker)
};

// -----------------------------------------------------------------------
// Helper class to unlock&lock a mutex during a function scope.

template <class Mutex>
class ScopeUnlocker
{
public:
    ScopeUnlocker(const Mutex& mutex) noexcept
        : fMutex(mutex)
    {
        fMutex.unlock();
    }

    ~ScopeUnlocker() noexcept
    {
        fMutex.lock();
    }

private:
    const Mutex& fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(ScopeUnlocker)
};

// -----------------------------------------------------------------------
// Define types

typedef ScopeLocker<Mutex>          MutexLocker;
typedef ScopeLocker<RecursiveMutex> RecursiveMutexLocker;

typedef ScopeTryLocker<Mutex>          MutexTryLocker;
typedef ScopeTryLocker<RecursiveMutex> RecursiveMutexTryLocker;

typedef ScopeUnlocker<Mutex>          MutexUnlocker;
typedef ScopeUnlocker<RecursiveMutex> RecursiveMutexUnlocker;

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_MUTEX_HPP_INCLUDED
