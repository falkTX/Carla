/*
 * Carla Log thread
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_LOG_THREAD_HPP__
#define __CARLA_LOG_THREAD_HPP__

#include "CarlaBackend.hpp"
#include "CarlaUtils.hpp"

#include <fcntl.h>
#include <QtCore/QThread>

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Log thread

class LogThread : public QThread
{
public:
    LogThread()
        : fStop(false),
          fCallback(nullptr),
          fCallbackPtr(nullptr)
    {
        pipe(fPipe);

        fflush(stdout);
        fflush(stderr);

        //fPipe[1] = ::dup(STDOUT_FILENO);
        //fPipe[1] = ::dup(STDERR_FILENO);
        dup2(fPipe[1], STDOUT_FILENO);
        dup2(fPipe[1], STDERR_FILENO);

        fcntl(fPipe[0], F_SETFL, O_NONBLOCK);
    }

    ~LogThread()
    {
        fflush(stdout);
        fflush(stderr);

        close(fPipe[0]);
        close(fPipe[1]);
    }

    void ready(CallbackFunc callback, void* callbackPtr)
    {
        CARLA_ASSERT(callback != nullptr);

        fCallback    = callback;
        fCallbackPtr = callbackPtr;

        start();
    }

    void stop()
    {
        fStop = true;

        if (isRunning())
            wait();
    }

protected:
    void run()
    {
        if (fCallback == nullptr)
            return;

        while (! fStop)
        {
            int i, r, lastRead;

            static char bufTemp[1024+1] = { '\0' };
            static char bufRead[1024+1];
            static char bufSend[2048+1];

            while ((r = read(fPipe[0], bufRead, sizeof(char)*1024)) > 0)
            {
                bufRead[r] = '\0';
                lastRead = 0;

                for (i=0; i < r; ++i)
                {
                    CARLA_ASSERT(bufRead[i] != '\0');

                    if (bufRead[i] == '\n')
                    {
                        std::strcpy(bufSend, bufTemp);
                        std::strncat(bufSend, bufRead+lastRead, i-lastRead);
                        bufSend[std::strlen(bufTemp)+i-lastRead] = '\0';

                        lastRead = i;
                        bufTemp[0] = '\0';

                        fCallback(fCallbackPtr, CarlaBackend::CALLBACK_DEBUG, 0, 0, 0, 0.0f, bufSend);
                    }
                }

                CARLA_ASSERT(i == r);
                CARLA_ASSERT(lastRead < r);

                if (lastRead > 0 && lastRead < r-1)
                {
                    std::strncpy(bufTemp, bufRead+lastRead, r-lastRead);
                    bufTemp[r-lastRead] = '\0';
                }
            }

            carla_msleep(20);
        }
    }

private:
    int  fPipe[2];
    bool fStop;

    CallbackFunc fCallback;
    void*        fCallbackPtr;
};

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_LOG_THREAD_HPP__
