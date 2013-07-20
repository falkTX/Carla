/*
 * Carla Mutex
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaJuceUtils.hpp"

#include <pthread.h>

// -----------------------------------------------------------------------
// CarlaMutex class

class CarlaMutex
{
public:
    CarlaMutex()
        : fTryLockWasCalled(false)
    {
        pthread_mutex_init(&pmutex, nullptr);
    }

    ~CarlaMutex()
    {
        pthread_mutex_destroy(&pmutex);
    }

    void lock()
    {
        pthread_mutex_lock(&pmutex);
    }

    bool tryLock()
    {
        fTryLockWasCalled = true;

        return (pthread_mutex_trylock(&pmutex) == 0);
    }

    void unlock(/*const bool resetTryLock*/)
    {
        //if (resetTryLock)
        //    fTryLockWasCalled = false;

        pthread_mutex_unlock(&pmutex);
    }

    bool wasTryLockCalled()
    {
        const bool ret(fTryLockWasCalled);
        fTryLockWasCalled = false;
        return ret;
    }

    class ScopedLocker
    {
    public:
        ScopedLocker(CarlaMutex& mutex)
            : fMutex(mutex)
        {
            fMutex.lock();
        }

        ~ScopedLocker()
        {
            fMutex.unlock();
        }

    private:
        CarlaMutex& fMutex;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedLocker)
    };

private:
    pthread_mutex_t pmutex;
    bool fTryLockWasCalled;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaMutex)
};

#endif // CARLA_MUTEX_HPP_INCLUDED
