/*
 * Carla Thread
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

#ifndef CARLA_THREAD_HPP_INCLUDED
#define CARLA_THREAD_HPP_INCLUDED

#include "CarlaMutex.hpp"
#include "CarlaString.hpp"

#if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012
// has pthread_setname_np
#elif defined(CARLA_OS_LINUX)
# include <sys/prctl.h>
#endif

// -----------------------------------------------------------------------
// CarlaThread class

class CarlaThread
{
protected:
    /*
     * Constructor.
     */
    CarlaThread(const char* const threadName = nullptr) noexcept
        : fName(threadName),
          fShouldExit(false)
    {
        _init();
    }

    /*
     * Destructor.
     */
    virtual ~CarlaThread() /*noexcept*/
    {
        CARLA_SAFE_ASSERT(! isThreadRunning());

        stopThread(-1);
    }

    /*
     * Virtual function to be implemented by the subclass.
     */
    virtual void run() = 0;

public:
    /*
     * Check if the thread is running.
     */
    bool isThreadRunning() const noexcept
    {
#ifdef CARLA_OS_WIN
        return (fHandle.p != nullptr);
#else
        return (fHandle != 0);
#endif
    }

    /*
     * Check if the thread should exit.
     */
    bool shouldThreadExit() const noexcept
    {
        return fShouldExit;
    }

    /*
     * Start the thread.
     */
    bool startThread() noexcept
    {
        // check if already running
        CARLA_SAFE_ASSERT_RETURN(! isThreadRunning(), true);

        const CarlaMutexLocker cml(fLock);

        fShouldExit = false;

        if (pthread_create(&fHandle, nullptr, _entryPoint, this) == 0)
        {
#ifdef CARLA_OS_WIN
            CARLA_SAFE_ASSERT_RETURN(fHandle.p != nullptr, false);
#else
            CARLA_SAFE_ASSERT_RETURN(fHandle != 0, false);
#endif

#if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012
            if (fName.isNotEmpty())
                pthread_setname_np(fHandle, fName);
#endif
            pthread_detach(fHandle);

            // wait for thread to start
            fLock.lock();

            return true;
        }

        return false;
    }

    /*
     * Stop the thread.
     * In the 'timeOutMilliseconds':
     * = 0 -> no wait
     * > 0 -> wait timeout value
     * < 0 -> wait forever
     */
    bool stopThread(const int timeOutMilliseconds) noexcept
    {
        const CarlaMutexLocker cml(fLock);

        if (isThreadRunning())
        {
            signalThreadShouldExit();

            if (timeOutMilliseconds != 0)
            {
                // Wait for the thread to stop
                int timeOutCheck = (timeOutMilliseconds == 1 || timeOutMilliseconds == -1) ? timeOutMilliseconds : timeOutMilliseconds/2;

                for (; isThreadRunning();)
                {
                    carla_msleep(2);

                    if (timeOutCheck < 0)
                        continue;

                    if (timeOutCheck > 0)
                        timeOutCheck -= 1;
                    else
                        break;
                }
            }

            if (isThreadRunning())
            {
                // should never happen!
                carla_stderr2("Carla assertion failure: \"! isThreadRunning()\" in file %s, line %i", __FILE__, __LINE__);

                // use a copy thread id so we can clear our own one
                pthread_t threadId;
                carla_copyStruct<pthread_t>(threadId, fHandle);
                _init();

                try {
                    pthread_cancel(threadId);
                } catch(...) {}

                return false;
            }
        }

        return true;
    }

    /*
     * Tell the thread to stop as soon as possible.
     */
    void signalThreadShouldExit() noexcept
    {
        fShouldExit = true;
    }

    static void setCurrentThreadName(const char* const name) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

#if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012
        if (fName.isNotEmpty())
            pthread_setname_np(pthread_self(), fName);
#elif defined(CARLA_OS_LINUX)
        prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
    }

private:
    CarlaMutex         fLock;       // Thread lock
    const CarlaString  fName;       // Thread name
    pthread_t          fHandle;     // Handle for this thread
    volatile bool      fShouldExit; // true if thread should exit

    void _init() noexcept
    {
#ifdef CARLA_OS_WIN
        fHandle.p = nullptr;
        fHandle.x = 0;
#else
        fHandle = 0;
#endif
    }

    void _runEntryPoint() noexcept
    {
        // report ready
        fLock.unlock();

        try {
            run();
        } catch(...) {}

        // done
        _init();
    }

    static void* _entryPoint(void* userData) noexcept
    {
        static_cast<CarlaThread*>(userData)->_runEntryPoint();
        return nullptr;
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaThread)
};

// -----------------------------------------------------------------------

#endif // CARLA_THREAD_HPP_INCLUDED
