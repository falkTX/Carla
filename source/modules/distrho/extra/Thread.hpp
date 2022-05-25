/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_THREAD_HPP_INCLUDED
#define DISTRHO_THREAD_HPP_INCLUDED

#include "Mutex.hpp"
#include "Sleep.hpp"
#include "String.hpp"

#ifdef DISTRHO_OS_LINUX
# include <sys/prctl.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Thread class

class Thread
{
protected:
    /*
     * Constructor.
     */
    Thread(const char* const threadName = nullptr) noexcept
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
    virtual ~Thread() /*noexcept*/
    {
        DISTRHO_SAFE_ASSERT(! isThreadRunning());

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
    bool startThread(const bool withRealtimePriority = false) noexcept
    {
        // check if already running
        DISTRHO_SAFE_ASSERT_RETURN(! isThreadRunning(), true);

        pthread_t handle;

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        struct sched_param sched_param;
        std::memset(&sched_param, 0, sizeof(sched_param));

        if (withRealtimePriority)
        {
            sched_param.sched_priority = 80;

#ifndef DISTRHO_OS_HAIKU
            if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)          == 0  &&
                pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0  &&
# ifndef DISTRHO_OS_WINDOWS
               (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)              == 0  ||
                pthread_attr_setschedpolicy(&attr, SCHED_RR)                == 0) &&
# endif
                pthread_attr_setschedparam(&attr, &sched_param)             == 0)
            {
                d_stdout("Thread setup with realtime priority successful");
            }
            else
#endif
            {
                d_stdout("Thread setup with realtime priority failed, going with normal priority instead");
                pthread_attr_destroy(&attr);
                pthread_attr_init(&attr);
            }
        }

        const MutexLocker ml(fLock);

        fShouldExit = false;

        bool ok = pthread_create(&handle, &attr, _entryPoint, this) == 0;
        pthread_attr_destroy(&attr);

        if (withRealtimePriority && !ok)
       {
            d_stdout("Thread with realtime priority failed on creation, going with normal priority instead");
            pthread_attr_init(&attr);
            ok = pthread_create(&handle, &attr, _entryPoint, this) == 0;
            pthread_attr_destroy(&attr);
       }

        DISTRHO_SAFE_ASSERT_RETURN(ok, false);
#ifdef PTW32_DLLPORT
        DISTRHO_SAFE_ASSERT_RETURN(handle.p != nullptr, false);
#else
        DISTRHO_SAFE_ASSERT_RETURN(handle != 0, false);
#endif
        pthread_detach(handle);
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
        const MutexLocker ml(fLock);

        if (isThreadRunning())
        {
            signalThreadShouldExit();

            if (timeOutMilliseconds != 0)
            {
                // Wait for the thread to stop
                int timeOutCheck = (timeOutMilliseconds == 1 || timeOutMilliseconds == -1) ? timeOutMilliseconds : timeOutMilliseconds/2;

                for (; isThreadRunning();)
                {
                    d_msleep(2);

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
                d_stderr2("assertion failure: \"! isThreadRunning()\" in file %s, line %i", __FILE__, __LINE__);

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
    const String& getThreadName() const noexcept
    {
        return fName;
    }

    /*
     * Returns the Id/handle of the thread.
     */
    pthread_t getThreadId() const noexcept
    {
        return fHandle;
    }

    /*
     * Changes the name of the caller thread.
     */
    static void setCurrentThreadName(const char* const name) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

#ifdef DISTRHO_OS_LINUX
        prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
#if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012 && !defined(DISTRHO_OS_GNU_HURD)
        pthread_setname_np(pthread_self(), name);
#endif
    }

    // -------------------------------------------------------------------

private:
    Mutex              fLock;       // Thread lock
    Signal             fSignal;     // Thread start wait signal
    const String       fName;       // Thread name
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
        static_cast<Thread*>(userData)->_runEntryPoint();
        return nullptr;
    }

    DISTRHO_DECLARE_NON_COPYABLE(Thread)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_THREAD_HPP_INCLUDED
