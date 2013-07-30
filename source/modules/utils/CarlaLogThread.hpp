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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_LOG_THREAD_HPP_INCLUDED
#define CARLA_LOG_THREAD_HPP_INCLUDED

#include "CarlaBackend.hpp"
#include "CarlaString.hpp"

#include <fcntl.h>
#include <QtCore/QThread>

using CarlaBackend::CallbackFunc;

// -----------------------------------------------------------------------
// Log thread

class CarlaLogThread : public QThread
{
public:
    CarlaLogThread()
        : QThread(nullptr),
          fStop(false),
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

        QThread::start(LowPriority);
    }

    ~CarlaLogThread()
    {
        fCallback    = nullptr;
        fCallbackPtr = nullptr;

        stop();

        fflush(stdout);
        fflush(stderr);

        close(fPipe[0]);
        close(fPipe[1]);
    }

    void setCallback(CallbackFunc callback, void* callbackPtr)
    {
        CARLA_ASSERT(callback != nullptr);

        fCallback    = callback;
        fCallbackPtr = callbackPtr;
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
        while (! fStop)
        {
            size_t r, lastRead;
            ssize_t r2; // to avoid sign/unsign conversions

            static char bufTemp[1024+1] = { '\0' };
            static char bufRead[1024+1];
            static char bufSend[2048+1];

            while ((r2 = read(fPipe[0], bufRead, sizeof(char)*1024)) > 0)
            {
                r = static_cast<size_t>(r2);

                bufRead[r] = '\0';
                lastRead = 0;

                for (size_t i=0; i < r; ++i)
                {
                    CARLA_ASSERT(bufRead[i] != '\0');

                    if (bufRead[i] == '\n')
                    {
                        std::strcpy(bufSend, bufTemp);
                        std::strncat(bufSend, bufRead+lastRead, i-lastRead);
                        bufSend[std::strlen(bufTemp)+i-lastRead] = '\0';

                        lastRead = i;
                        bufTemp[0] = '\0';

                        if (fCallback != nullptr)
                        {
                            if (fOldBuffer.isNotEmpty())
                            {
                                fCallback(fCallbackPtr, CarlaBackend::CALLBACK_DEBUG, 0, 0, 0, 0.0f, (const char*)fOldBuffer);
                                fOldBuffer = nullptr;
                            }

                            fCallback(fCallbackPtr, CarlaBackend::CALLBACK_DEBUG, 0, 0, 0, 0.0f, bufSend);
                        }
                        else
                            fOldBuffer += bufSend;
                    }
                }

                CARLA_ASSERT(lastRead < r);

                if (lastRead > 0 && r > 0 && lastRead+1 < r)
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
    CarlaString  fOldBuffer;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaLogThread)
};

// -----------------------------------------------------------------------

#endif // CARLA_LOG_THREAD_HPP_INCLUDED
