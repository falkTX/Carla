/*
 * Carla Runner
 * Copyright (C) 2022-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_RUNNER_HPP_INCLUDED
#define CARLA_RUNNER_HPP_INCLUDED

#include "CarlaUtils.hpp"

#ifndef CARLA_OS_WASM
# include "CarlaThread.hpp"
#else
# include "CarlaString.hpp"
# include <emscripten/html5.h>
#endif

// -------------------------------------------------------------------------------------------------------------------
// CarlaRunner class

/**
   This is a handy class that handles "idle" time in either background or main thread,
   whichever is more suitable to the target platform.
   Typically background threads on desktop platforms, main thread on web.

   A single function is expected to be implemented by subclasses,
   which directly allows it to stop the runner by returning false.

   You can use it for quick operations that do not need to be handled in the main thread if possible.
   The target is to spread out execution over many runs, instead of spending a lot of time on a single task.
 */
class CarlaRunner
{
protected:
    /*
     * Constructor.
     */
    CarlaRunner(const char* const runnerName = nullptr) noexcept
       #ifndef CARLA_OS_WASM
        : fRunnerThread(this, runnerName),
          fTimeInterval(0)
       #else
        : fRunnerName(runnerName),
          fIntervalId(0)
       #endif
    {
    }

    /*
     * Destructor.
     */
    virtual ~CarlaRunner() /*noexcept*/
    {
        CARLA_SAFE_ASSERT(! isRunnerActive());

        stopRunner();
    }

    /*
     * Virtual function to be implemented by the subclass.
     * Return true to keep running, false to stop execution.
     */
    virtual bool run() = 0;

    /*
     * Check if the runner should stop.
     * To be called from inside the runner to know if a stop request has been made.
     */
    bool shouldRunnerStop() const noexcept
    {
       #ifndef CARLA_OS_WASM
        return fRunnerThread.shouldThreadExit();
       #else
        return fIntervalId == 0;
       #endif
    }

    // ---------------------------------------------------------------------------------------------------------------

public:
    /*
     * Check if the runner is active.
     */
    bool isRunnerActive() noexcept
    {
       #ifndef CARLA_OS_WASM
        return fRunnerThread.isThreadRunning();
       #else
        return fIntervalId != 0;
       #endif
    }

    /*
     * Start the thread.
     */
    bool startRunner(const uint timeIntervalMilliseconds = 0) noexcept
    {
       #ifndef CARLA_OS_WASM
        CARLA_SAFE_ASSERT_RETURN(!fRunnerThread.isThreadRunning(), false);
        fTimeInterval = timeIntervalMilliseconds;
        return fRunnerThread.startThread();
       #else
        CARLA_SAFE_ASSERT_RETURN(fIntervalId == 0, false);
        fIntervalId = emscripten_set_interval(_entryPoint, timeIntervalMilliseconds, this);
        return true;
       #endif
    }

    /*
     * Stop the runner.
     * This will signal the runner to stop if active, and wait until it finishes.
     */
    bool stopRunner() noexcept
    {
       #ifndef CARLA_OS_WASM
        return fRunnerThread.stopThread(-1);
       #else
        signalRunnerShouldStop();
        return true;
       #endif
    }

    /*
     * Tell the runner to stop as soon as possible.
     */
    void signalRunnerShouldStop() noexcept
    {
       #ifndef CARLA_OS_WASM
        fRunnerThread.signalThreadShouldExit();
       #else
        if (fIntervalId != 0)
        {
            emscripten_clear_interval(fIntervalId);
            fIntervalId = 0;
        }
       #endif
    }

    // ---------------------------------------------------------------------------------------------------------------

    /*
     * Returns the name of the runner.
     * This is the name that gets set in the constructor.
     */
    const CarlaString& getRunnerName() const noexcept
    {
       #ifndef CARLA_OS_WASM
        return fRunnerThread.getThreadName();
       #else
        return fRunnerName;
       #endif
    }

    // ---------------------------------------------------------------------------------------------------------------

private:
#ifndef CARLA_OS_WASM
    class RunnerThread : public CarlaThread
    {
        CarlaRunner* const runner;

    public:
        RunnerThread(CarlaRunner* const r, const char* const rn)
            : CarlaThread(rn),
              runner(r) {}

    protected:
        void run() override
        {
            const uint timeInterval = runner->fTimeInterval;

            while (!shouldThreadExit())
            {
                bool stillRunning = false;

                try {
                    stillRunning = runner->run();
                } catch(...) {}

                if (stillRunning && !shouldThreadExit())
                {
                    if (timeInterval != 0)
                        carla_msleep(timeInterval);

                    // FIXME
                    // pthread_yield();
                    continue;
                }

                break;
            }
        }

        CARLA_DECLARE_NON_COPYABLE(RunnerThread)
    } fRunnerThread;

    uint fTimeInterval;
#else
    const CarlaString fRunnerName;
    long fIntervalId;

    void _runEntryPoint() noexcept
    {
        bool stillRunning = false;

        try {
            stillRunning = run();
        } catch(...) {}

        if (fIntervalId != 0 && !stillRunning)
        {
            emscripten_clear_interval(fIntervalId);
            fIntervalId = 0;
        }
    }

    static void _entryPoint(void* const userData) noexcept
    {
        static_cast<CarlaRunner*>(userData)->_runEntryPoint();
    }
#endif

    CARLA_DECLARE_NON_COPYABLE(CarlaRunner)
};

// -------------------------------------------------------------------------------------------------------------------

#endif // CARLA_RUNNER_HPP_INCLUDED
