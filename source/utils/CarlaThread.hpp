/*
 * Carla Thread
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaProcessUtils.hpp"
#include "CarlaTimeUtils.hpp"

#ifdef CARLA_OS_WASM
# error Threads do not work under wasm!
#endif

// -----------------------------------------------------------------------
// CarlaThread class

class CarlaThread
{
protected:
    /*
     * Constructor.
     */
    CarlaThread(const char* const threadName) noexcept
        : fLock(),
          fSignal(),
          fName(threadName),
         #ifdef PTW32_DLLPORT
          fHandle({nullptr, 0}),
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
        return fHandle.p != nullptr;
       #else
        return fHandle != 0;
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
    bool startThread(bool withRealtimePriority = false) noexcept
    {
        // check if already running
        CARLA_SAFE_ASSERT_RETURN(! isThreadRunning(), true);

        if (withRealtimePriority && std::getenv("CARLA_BRIDGE_DUMMY") != nullptr)
            withRealtimePriority = false;

        pthread_t handle;

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        struct sched_param sched_param;
        carla_zeroStruct(sched_param);

        if (withRealtimePriority)
        {
            sched_param.sched_priority = 80;

           #ifndef CARLA_OS_HAIKU
            if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)          == 0  &&
                pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0  &&
              #ifndef CARLA_OS_WIN
               (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)              == 0  ||
                pthread_attr_setschedpolicy(&attr, SCHED_RR)                == 0) &&
              #endif
                pthread_attr_setschedparam(&attr, &sched_param)             == 0)
            {
                carla_stdout("CarlaThread setup with realtime priority successful");
            }
            else
           #endif
            {
                carla_stdout("CarlaThread setup with realtime priority failed, going with normal priority instead");
                pthread_attr_destroy(&attr);
                pthread_attr_init(&attr);
            }
        }

        pthread_attr_setdetachstate(&attr, 1);

        const CarlaMutexLocker cml(fLock);

        fShouldExit = false;

        bool ok = pthread_create(&handle, &attr, _entryPoint, this) == 0;
        pthread_attr_destroy(&attr);

        if (withRealtimePriority && !ok)
       {
            carla_stdout("CarlaThread with realtime priority failed on creation, going with normal priority instead");
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, 1);
            ok = pthread_create(&handle, &attr, _entryPoint, this) == 0;
            pthread_attr_destroy(&attr);
       }

        CARLA_SAFE_ASSERT_RETURN(ok, false);
       #ifdef PTW32_DLLPORT
        CARLA_SAFE_ASSERT_RETURN(handle.p != nullptr, false);
       #else
        CARLA_SAFE_ASSERT_RETURN(handle != 0, false);
       #endif
        _copyFrom(handle);

        // wait for thread to start
        fSignal.wait();
        return true;
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

                pthread_detach(threadId);
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
     * Returns the Id/handle of the thread.
     */
    pthread_t getThreadId() const noexcept
    {
       #ifdef PTW32_DLLPORT
        const pthread_t handle = { fHandle.p, fHandle.x };
        return handle;
       #else
        return fHandle;
       #endif
    }

    /*
     * Changes the name of the caller thread.
     */
    static void setCurrentThreadName(const char* const name) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

        carla_setProcessName(name);
       #if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012 && !defined(CARLA_OS_GNU_HURD)
        pthread_setname_np(pthread_self(), name);
       #endif
    }

    // -------------------------------------------------------------------

private:
    CarlaMutex         fLock;       // Thread lock
    CarlaSignal        fSignal;     // Thread start wait signal
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
        if (fName.isNotEmpty())
            setCurrentThreadName(fName);

        // report ready
        fSignal.signal();

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

    CARLA_DECLARE_NON_COPYABLE(CarlaThread)
};

// -----------------------------------------------------------------------

#endif // CARLA_THREAD_HPP_INCLUDED
