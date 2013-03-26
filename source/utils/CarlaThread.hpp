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

#ifndef __CARLA_THREAD_HPP__
#define __CARLA_THREAD_HPP__

#include "CarlaJuceUtils.hpp"

// #define CPP11_THREAD

#ifdef CPP11_THREAD
# include <thread>
#else
# include <pthread.h>
#endif

// -------------------------------------------------
// CarlaThread class

class CarlaThread
{
public:
    CarlaThread()
        : fStarted(false),
          fFinished(false)
    {
#ifdef CPP11_THREAD
        cthread = nullptr;
#else
        _zero();
        pthread_attr_init(&pthreadAttr);
        pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_JOINABLE);
#endif
    }

    ~CarlaThread()
    {
        CARLA_ASSERT(! isRunning());

        if (isRunning())
            terminate();

#ifdef CPP11_THREAD
        if (cthread != nullptr)
        {
            cthread->join();
            delete cthread;
        }
#else
        if (! _isNull())
            pthread_join(pthreadId, nullptr);

        pthread_attr_destroy(&pthreadAttr);
#endif
    }

    bool start()
    {
        CARLA_ASSERT(! isRunning());

        if (isRunning())
            return false;

        fStarted  = false;
        fFinished = false;

#ifdef CPP11_THREAD
        CARLA_ASSERT(cthread == nullptr);

        if (cthread != nullptr)
            return false;

        cthread = new std::thread(_cthreadRoutine, this);
        CARLA_ASSERT(cthread->joinable());

        return true;
#else
        CARLA_ASSERT(_isNull());

        if (! _isNull())
            return false;

        return (pthread_create(&pthreadId, &pthreadAttr, _pthreadRoutine, this) == 0);
#endif
    }

    bool stop(const unsigned int timeout = 0)
    {
        CARLA_ASSERT(isRunning());

        if (! isRunning())
            return true;

#ifdef CPP11_THREAD
        if (cthread == nullptr)
            return true;
#else
        if (_isNull())
            return true;
#endif

        if (timeout == 0)
        {
#ifdef CPP11_THREAD
              cthread->join();
#else
              pthread_join(pthreadId, nullptr);
#endif
        }
        else
        {
            for (unsigned int i=0; i < timeout && ! fFinished; i++)
                carla_msleep(1);
        }

        if (! fFinished)
            return false;

#ifdef CPP11_THREAD
        delete cthread;
        cthread = nullptr;
#else
        _zero();
#endif

        return true;
    }

    void terminate()
    {
        CARLA_ASSERT(isRunning());

        if (fFinished)
            return;

#ifdef CPP11_THREAD
        if (cthread == nullptr)
            return;
#else
        if (_isNull())
            return;
#endif

#ifdef CPP11_THREAD
        cthread->detach();
        //cthread->join();
        delete cthread;
        cthread = nullptr;
#else
        pthread_detach(pthreadId);
        //pthread_join(pthreadId, nullptr);
        pthread_cancel(pthreadId);
        _zero();
#endif

        fFinished = true;
    }

    bool isRunning()
    {
        if (fStarted && ! fFinished)
            return true;

        // take the change to clear data
#ifdef CPP11_THREAD
        if (cthread != nullptr)
        {
            cthread->join();
            delete cthread;
            cthread = nullptr;
        }
#else
        if (! _isNull())
        {
            pthread_join(pthreadId, nullptr);
            _zero();
        }
#endif
        return false;
    }

    void waitForStarted(const unsigned int timeout = 0) // ms
    {
        if (fStarted)
            return;

        if (timeout == 0)
        {
            while (! fStarted)
                carla_msleep(1);
        }
        else
        {
            for (unsigned int i=0; i < timeout && ! fStarted; i++)
                carla_msleep(1);
        }
    }

    void waitForFinished()
    {
        waitForStarted();
        stop(0);
    }

protected:
    virtual void run() = 0;

private:
    bool fStarted;
    bool fFinished;

    void handleRoutine()
    {
        fStarted = true;
        run();
        fFinished = true;
    }

#ifdef CPP11_THREAD
    std::thread* cthread;

    static void _cthreadRoutine(CarlaThread* const _this_)
    {
        _this_->handleRoutine();
    }
#else
    pthread_t      pthreadId;
    pthread_attr_t pthreadAttr;

    static void* _pthreadRoutine(void* const _this_)
    {
        ((CarlaThread*)_this_)->handleRoutine();
        pthread_exit(nullptr);
        return nullptr;
    }

    bool _isNull()
    {
#ifdef CARLA_OS_WIN
        return (pthreadId.p == nullptr);
#else
        return (pthreadId == 0);
#endif
    }

    void _zero()
    {
        carla_zeroStruct<pthread_t>(pthreadId);
    }
#endif

    //CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaThread)
};

// -------------------------------------------------

#endif // __CARLA_THREAD_HPP__
