/*
 * Carla Log Thread
 * Copyright (C) 2013-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_LOG_THREAD_HPP_INCLUDED
#define CARLA_LOG_THREAD_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaString.hpp"
#include "CarlaThread.hpp"

#include <fcntl.h>

using CarlaBackend::EngineCallbackFunc;

// -----------------------------------------------------------------------
// Log thread

class CarlaLogThread : private CarlaThread
{
public:
    CarlaLogThread()
        : CarlaThread("CarlaLogThread"),
          fStdOut(-1),
          fStdErr(-1),
          fCallback(nullptr),
          fCallbackPtr(nullptr) {}

    ~CarlaLogThread()
    {
        stop();
    }

    void init()
    {
        CARLA_SAFE_ASSERT_RETURN(pipe(fPipe) == 0,);
        CARLA_SAFE_ASSERT_RETURN(fcntl(fPipe[0], F_SETFL, O_NONBLOCK) == 0,);

        std::fflush(stdout);
        std::fflush(stderr);

        fStdOut = dup(STDOUT_FILENO);
        fStdErr = dup(STDERR_FILENO);

        dup2(fPipe[1], STDOUT_FILENO);
        dup2(fPipe[1], STDERR_FILENO);

        startThread();
    }

    void stop()
    {
        if (fStdOut != -1)
            return;

        stopThread(5000);

        std::fflush(stdout);
        std::fflush(stderr);

        close(fPipe[0]);
        close(fPipe[1]);

        dup2(fStdOut, STDOUT_FILENO);
        dup2(fStdErr, STDERR_FILENO);
        close(fStdOut);
        close(fStdErr);
        fStdOut = -1;
        fStdErr = -1;
    }

    void setCallback(EngineCallbackFunc callback, void* callbackPtr)
    {
        CARLA_SAFE_ASSERT_RETURN(callback != nullptr,);

        fCallback    = callback;
        fCallbackPtr = callbackPtr;
    }

protected:
    void run()
    {
        CARLA_SAFE_ASSERT_RETURN(fCallback != nullptr,);

        size_t k, bufTempPos;
        ssize_t r, lastRead;
        char bufTemp[1024+1];
        char bufRead[1024+1];
        char bufSend[2048+1];

        bufTemp[0] = '\0';
        bufTempPos = 0;

        while (! shouldThreadExit())
        {
            bufRead[0] = '\0';

            while ((r = read(fPipe[0], bufRead, 1024)) > 0)
            {
                CARLA_SAFE_ASSERT_CONTINUE(r <= 1024);

                bufRead[r] = '\0';
                lastRead = 0;

                for (ssize_t i=0; i<r; ++i)
                {
                    CARLA_SAFE_ASSERT_BREAK(bufRead[i] != '\0');

                    if (bufRead[i] != '\n')
                        continue;

                    k = static_cast<size_t>(i-lastRead);

                    if (bufTempPos != 0)
                    {
                        std::memcpy(bufSend, bufTemp, bufTempPos);
                        std::memcpy(bufSend+bufTempPos, bufRead+lastRead, k);
                        k += bufTempPos;
                    }
                    else
                    {
                        std::memcpy(bufSend, bufRead+lastRead, k);
                    }

                    lastRead   = i+1;
                    bufSend[k] = '\0';
                    bufTemp[0] = '\0';
                    bufTempPos = 0;

                    fCallback(fCallbackPtr, CarlaBackend::ENGINE_CALLBACK_DEBUG, 0, 0, 0, 0.0f, bufSend);
                }

                if (lastRead > 0 && lastRead != r)
                {
                    k = static_cast<size_t>(r-lastRead);
                    std::memcpy(bufTemp, bufRead+lastRead, k);
                    bufTemp[k] = '\0';
                    bufTempPos = k;
                }
            }

            carla_msleep(20);
        }
    }

private:
    int fPipe[2];
    int fStdOut;
    int fStdErr;

    EngineCallbackFunc fCallback;
    void*              fCallbackPtr;

    //CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaLogThread)
};

// -----------------------------------------------------------------------

#endif // CARLA_LOG_THREAD_HPP_INCLUDED
