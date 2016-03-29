/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

class Signal;

// -----------------------------------------------------------------------
// Mutex class

class Mutex
{
public:
    /*
     * Constructor.
     */
    Mutex(bool inheritPriority = true) noexcept
        : fMutex()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setprotocol(&attr, inheritPriority ? PTHREAD_PRIO_INHERIT : PTHREAD_PRIO_NONE);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
        pthread_mutex_init(&fMutex, &attr);
        pthread_mutexattr_destroy(&attr);
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

    friend class Signal;
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
// Signal class

class Signal
{
public:
    /*
     * Constructor.
     */
    Signal(Mutex& mutex) noexcept
        : fCondition(),
          fMutex(mutex.fMutex)
    {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
        pthread_cond_init(&fCondition, &attr);
        pthread_condattr_destroy(&attr);
    }

    /*
     * Destructor.
     */
    ~Signal() noexcept
    {
        pthread_cond_destroy(&fCondition);
    }

    /*
     * Wait for a broadcast.
     */
    bool wait() noexcept
    {
        try {
            return (pthread_cond_wait(&fCondition, &fMutex) == 0);
        } DISTRHO_SAFE_EXCEPTION_RETURN("pthread_cond_wait", false);
    }

    /*
     * Wake up one waiting thread.
     */
    void signal() noexcept
    {
        pthread_cond_signal(&fCondition);
    }

    /*
     * Wake up all waiting threads.
     */
    void broadcast() noexcept
    {
        pthread_cond_broadcast(&fCondition);
    }

private:
    pthread_cond_t   fCondition;
    pthread_mutex_t& fMutex;

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_CLASS(Signal)
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
