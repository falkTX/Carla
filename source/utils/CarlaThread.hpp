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
    void startThread() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(! isThreadRunning(),);

        const CarlaMutexLocker sl(fLock);

        fShouldExit = false;

        pthread_t threadId;

        if (pthread_create(&threadId, nullptr, _entryPoint, this) == 0)
        {
#if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012
            if (fName.isNotEmpty())
                pthread_setname_np(threadId, fName);
#endif
            pthread_detach(threadId);

#ifdef CARLA_OS_WIN
            *(const_cast<pthread_t*>(&fHandle)) = threadId;
#else
            fHandle = threadId;
#endif

            // wait for thread to start
            fLock.lock();
        }
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
        const CarlaMutexLocker sl(fLock);

        if (isThreadRunning())
        {
            signalThreadShouldExit();

            if (timeOutMilliseconds != 0)
            {
                // Wait for the thread to stop
                int timeOutCheck = (timeOutMilliseconds == 1 || timeOutMilliseconds == -1) ? timeOutMilliseconds : timeOutMilliseconds/2;

                while (isThreadRunning())
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
                carla_stderr2("Carla assertion failure: \"! isRunning()\" in file %s, line %i", __FILE__, __LINE__);

                pthread_t threadId = *(const_cast<pthread_t*>(&fHandle));
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

private:
    CarlaMutex         fLock;       // Thread lock
    const CarlaString  fName;       // Thread name
    volatile pthread_t fHandle;     // Handle for this thread
    volatile bool      fShouldExit; // true if thread should exit

    /*
     * Static null thread.
     */
    static pthread_t& _null() noexcept
    {
#ifdef CARLA_OS_WIN
        static pthread_t sThread = { nullptr, 0 };
#else
        static pthread_t sThread = 0;
#endif
        return sThread;
    }

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

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaThread)
};

// -----------------------------------------------------------------------

#endif // CARLA_THREAD_HPP_INCLUDED
