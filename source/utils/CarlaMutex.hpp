/*
 * Carla common utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_MUTEX_HPP__
#define __CARLA_MUTEX_HPP__

#include "CarlaJuceUtils.hpp"

// #define CPP11_MUTEX

#ifdef CPP11_MUTEX
# include <mutex>
#else
# include <pthread.h>
#endif

// -------------------------------------------------
// CarlaMutex class

class CarlaMutex
{
public:
    CarlaMutex()
        : fTryLockCalled(false)
    {
#ifndef CPP11_MUTEX
        pthread_mutex_init(&pmutex, nullptr);
#endif
    }

    ~CarlaMutex()
    {
#ifndef CPP11_MUTEX
        pthread_mutex_destroy(&pmutex);
#endif
    }

    void lock()
    {
#ifdef CPP11_MUTEX
        cmutex.lock();
#else
        pthread_mutex_lock(&pmutex);
#endif
    }

    bool tryLock()
    {
        fTryLockCalled = true;

#ifdef CPP11_MUTEX
        return cmutex.try_lock();
#else
        return (pthread_mutex_trylock(&pmutex) == 0);
#endif
    }

    void unlock()
    {
#ifdef CPP11_MUTEX
        cmutex.unlock();
#else
        pthread_mutex_unlock(&pmutex);
#endif
    }

    bool wasTryLockCalled()
    {
        const bool ret = fTryLockCalled;
        fTryLockCalled = false;
        return ret;
    }

    class ScopedLocker
    {
    public:
        ScopedLocker(CarlaMutex* const mutex)
            : fMutex(mutex)
        {
            fMutex->lock();
        }

        ~ScopedLocker()
        {
            fMutex->unlock();
        }

    private:
        CarlaMutex* const fMutex;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedLocker)
    };

private:
#ifdef CPP11_MUTEX
    std::mutex cmutex;
#else
    pthread_mutex_t pmutex;
#endif
    bool fTryLockCalled;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaMutex)
};

// -------------------------------------------------

#endif // __CARLA_MUTEX_HPP__
