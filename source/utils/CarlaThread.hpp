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
        : fLock(),
          fName(threadName),
#ifdef PTW32_DLLPORT
          fHandle(nullptr, 0)
#else
          fHandle(0),
#endif
          fShouldExit(false) {}

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

    // -------------------------------------------------------------------

public:
    /*
     * Check if the thread is running.
     */
    bool isThreadRunning() const noexcept
    {
#ifdef PTW32_DLLPORT
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

        pthread_t handle;

        if (pthread_create(&handle, nullptr, _entryPoint, this) == 0)
        {
#ifdef PTW32_DLLPORT
            CARLA_SAFE_ASSERT_RETURN(handle.p != nullptr, false);
#else
            CARLA_SAFE_ASSERT_RETURN(handle != 0, false);
#endif
            pthread_detach(handle);
            _copyFrom(handle);

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

                // copy thread id so we can clear our one
                pthread_t threadId;
                _copyTo(threadId);
                _init();

                try {
                    pthread_cancel(threadId);
                } CARLA_SAFE_EXCEPTION("pthread_cancel");

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

    // -------------------------------------------------------------------

    /*
     * Returns the name of the thread.
     * This is the name that gets set in the constructor.
     */
    const CarlaString& getThreadName() const noexcept
    {
        return fName;
    }

    /*
     * Changes the name of the caller thread.
     */
    static void setCurrentThreadName(const char* const name) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

#if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012
        pthread_setname_np(pthread_self(), name);
#elif defined(CARLA_OS_LINUX)
        prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
    }

    // -------------------------------------------------------------------

private:
    CarlaMutex         fLock;       // Thread lock
    const CarlaString  fName;       // Thread name
    volatile pthread_t fHandle;     // Handle for this thread
    volatile bool      fShouldExit; // true if thread should exit

    /*
     * Init pthread type.
     */
    void _init() noexcept
    {
#ifdef PTW32_DLLPORT
        fHandle.p = nullptr;
        fHandle.x = 0;
#else
        fHandle = 0;
#endif
    }

    /*
     * Copy our pthread type from another var.
     */
    void _copyFrom(const pthread_t& handle) noexcept
    {
#ifdef PTW32_DLLPORT
        fHandle.p = handle.p;
        fHandle.x = handle.x;
#else
        fHandle = handle;
#endif
    }

    /*
     * Copy our pthread type to another var.
     */
    void _copyTo(volatile pthread_t& handle) const noexcept
    {
#ifdef PTW32_DLLPORT
        handle.p = fHandle.p;
        handle.x = fHandle.x;
#else
        handle = fHandle;
#endif
    }

    /*
     * Thread entry point.
     */
    void _runEntryPoint() noexcept
    {
        // report ready
        fLock.unlock();

        setCurrentThreadName(fName);

        try {
            run();
        } catch(...) {}

        // done
        _init();
    }

    /*
     * Thread entry point.
     */
    static void* _entryPoint(void* userData) noexcept
    {
        static_cast<CarlaThread*>(userData)->_runEntryPoint();
        return nullptr;
    }

    CARLA_DECLARE_NON_COPY_CLASS(CarlaThread)
};

// -----------------------------------------------------------------------

#endif // CARLA_THREAD_HPP_INCLUDED
