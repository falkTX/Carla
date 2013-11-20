/*
 * Carla Thread
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
    CarlaThread(const char* const threadName = nullptr)
        : fName(threadName),
          fShouldExit(false)
    {
        _init();
    }

    /*
     * Destructor.
     */
    ~CarlaThread()
    {
        CARLA_SAFE_ASSERT(! isRunning());

        stop(-1);
    }

    /*
     * Virtual function to be implemented by the subclass.
     */
    virtual void run() = 0;

public:
    /*
     * Check if the thread is running.
     */
    bool isRunning() const noexcept
    {
#ifdef CARLA_OS_WIN32
        return (fHandle.p != nullptr);
#else
        return (fHandle != 0);
#endif
    }

    /*
     * Check if the thread should exit.
     */
    bool shouldExit() const noexcept
    {
        return fShouldExit;
    }

    /*
     * Start the thread.
     */
    void start()
    {
        CARLA_SAFE_ASSERT_RETURN(! isRunning(),);

        const CarlaMutex::ScopedLocker sl(fLock);

        fShouldExit = false;

        if (pthread_create(const_cast<pthread_t*>(&fHandle), nullptr, _entryPoint, this) == 0)
        {
#if (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012
            if (fName.isNotEmpty())
                pthread_setname_np(fHandle, fName.getBuffer());
#endif
            pthread_detach(fHandle);

            // wait for thread to start
            fLock.lock();
        }
    }

    /*
     * Stop the thread.
     * In the 'timeOutMilliseconds':
     * =0 -> no wait
     * >0 -> wait timeout value
     * <0 -> wait forever
     */
    bool stop(const int timeOutMilliseconds)
    {
        const CarlaMutex::ScopedLocker sl(fLock);

        if (isRunning())
        {
            signalShouldExit();

            if (timeOutMilliseconds != 0)
            {
                // Wait for the thread to stop
                int timeOutCheck = (timeOutMilliseconds == 1 || timeOutMilliseconds == -1) ? timeOutMilliseconds : timeOutMilliseconds/2;

                while (isRunning())
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

            if (isRunning())
            {
                // should never happen!
                carla_stderr2("Carla assertion failure: \"! isRunning()\" in file %s, line %i", __FILE__, __LINE__);

                pthread_cancel(fHandle);
                _init();
                return false;
            }
        }

        return true;
    }

    /*
     * Tell the thread to stop as soon as possible.
     */
    void signalShouldExit() noexcept
    {
        fShouldExit = true;
    }

private:
    const CarlaString   fName;       // Thread name
    volatile CarlaMutex fLock;       // Thread lock
    volatile pthread_t  fHandle;     // Handle for this thread
    volatile bool       fShouldExit; // true if thread should exit

    void _init() noexcept
    {
#ifdef CARLA_OS_WIN32
        fHandle.p = nullptr;
        fHandle.x = 0;
#else
        fHandle = 0;
#endif
    }

    void _runEntryPoint()
    {
        // tell dad we're ready
        fLock.unlock();

        run();

        // done
        _init();
    }

    static void* _entryPoint(void* userData)
    {
        static_cast<CarlaThread*>(userData)->_runEntryPoint();
        return nullptr;
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaThread)
};

// -----------------------------------------------------------------------

#endif // CARLA_THREAD_HPP_INCLUDED
