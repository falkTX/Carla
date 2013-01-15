/*
 * Carla Engine Thread
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef CARLA_ENGINE_THREAD_HPP
#define CARLA_ENGINE_THREAD_HPP

#include "carla_backend.hpp"

#include <QtCore/QMutex>
#include <QtCore/QThread>

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

class CarlaEngineThread : public QThread
{
public:
    CarlaEngineThread(CarlaEngine* const engine, QObject* const parent = nullptr);
    ~CarlaEngineThread();

    void startNow();
    void stopNow();

    // ----------------------------------------------

protected:
    void run();

    // ----------------------------------------------

private:
    CarlaEngine* const engine;

    QMutex m_mutex;
    bool   m_stopNow;

    // ----------------------------------------------

    class ScopedLocker
    {
    public:
        ScopedLocker(CarlaEngineThread* const thread)
            : mutex(&thread->m_mutex)
        {
            mutex->lock();
        }

        ~ScopedLocker()
        {
            mutex->unlock();
        }

    private:
        QMutex* const mutex;
    };
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_THREAD_HPP
